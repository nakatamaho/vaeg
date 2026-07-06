/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include	"compiler.h"
#include	"sdlapi.h"
#include	"strres.h"
#include	"np2.h"
#include	"dosio.h"
#include	"commng.h"
#include	"inputmng.h"
#include	"scrnmng.h"
#include	"soundmng.h"
#include	"sysmng.h"
#include	"taskmng.h"
#include	"sdlkbd.h"
#include	"ini.h"
#include	"pccore.h"
#include	"scrndraw.h"
#include	"s98.h"
#include	"diskdrv.h"
#include	"fdc.h"
#include	"timing.h"
#include	"keystat.h"
#include	"bkupmemva.h"
#include	"gui/gui.h"

		NP2OSCFG	np2oscfg = {0, 0, 0, 0, 0, 1, 0};

static const UINT smoke_timeout_frames = 600;
static const UINT max_catchup_frames = 15;
static const char *smoke_required_roms[] = {
	"FONT.ROM",
	"VAFONT.ROM",
	"VADIC.ROM",
	"VAROM00.ROM",
	"VAROM08.ROM",
	"VAROM1.ROM",
	"VASUBSYS.ROM",
	NULL
};

typedef struct {
	UINT32	tick;
	UINT	frames_executed;
	UINT	frames_skipped;
	UINT	max_pending;
} PACELOG;

static	UINT	framecnt;
static	UINT	waitcnt;
static	UINT	framemax = 1;

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]       : print this message\n");
	printf("\t--smoke             : initialize SDL2, run a short core loop, exit\n");
	printf("\t--fdctrace          : print one FDC trace line per command to stderr\n");
	printf("\t--pacelog           : print pacing counters once per second\n");
	printf("\timage1 [image2]     : mount FDD images in drive 1 and 2\n");
}

static void set_default_rompath(void) {

	if (np2cfg.biospath[0] != '\0') {
		return;
	}
	if ((file_attr("romimage") & FILEATTR_DIRECTORY) != 0) {
		file_cpyname(np2cfg.biospath, "romimage", sizeof(np2cfg.biospath));
	}
}

static void smoke_configure_va(void) {

#if defined(SUPPORT_PC88VA)
	file_cpyname(np2cfg.model, str_VA2, sizeof(np2cfg.model));
	np2cfg.baseclock = PCBASECLOCK40;
	np2cfg.multiple = 2;
	np2cfg.ITF_WORK = 1;
	np2cfg.EXTMEM = 1;
	np2cfg.SOUND_SW = 0x0200;
#endif
}

static BOOL config_selects_va(void) {

#if defined(SUPPORT_PC88VA)
	return((milstr_cmp(np2cfg.model, str_VA1) == 0) ||
		   (milstr_cmp(np2cfg.model, str_VA2) == 0));
#else
	return(FALSE);
#endif
}

static void warn_va_config_sanity(void) {

#if defined(SUPPORT_PC88VA)
	UINT32	va_threshold;

	if (!config_selects_va()) {
		return;
	}
	va_threshold = (PCBASECLOCK40 + PCBASECLOCK25) / 2;
	if (np2cfg.baseclock < va_threshold) {
		fprintf(stderr,
				"WARNING: PC-88VA config expects clk_base=3993600 "
				"(current=%u); stale configs can run in the wrong "
				"clock domain.\n", np2cfg.baseclock);
	}
	if (np2cfg.multiple != 2) {
		fprintf(stderr,
				"WARNING: PC-88VA config expects clk_mult=2 "
				"(current=%u); stale configs can run in the wrong "
				"clock domain.\n", np2cfg.multiple);
	}
	if (np2cfg.SOUND_SW != 0x0200) {
		fprintf(stderr,
				"WARNING: PC-88VA config expects SNDboard=200 "
				"(current=%03x); stale configs can leave the VA "
				"Sound Board II unbound.\n", np2cfg.SOUND_SW);
	}
#endif
}

static void make_rom_path(char *path, int size, const char *dir,
													const char *name) {

	if ((dir == NULL) || (dir[0] == '\0')) {
		file_cpyname(path, name, size);
		return;
	}
	file_cpyname(path, dir, size);
	file_setseparator(path, size);
	file_catname(path, name, size);
}

static BOOL smoke_romdir_complete(const char *dir, char *missing,
														int missing_size) {

	int		i;

	if ((dir == NULL) || (dir[0] == '\0')) {
		return(FAILURE);
	}
	for (i=0; smoke_required_roms[i] != NULL; i++) {
		char	path[MAX_PATH];
		short	attr;

		make_rom_path(path, sizeof(path), dir, smoke_required_roms[i]);
		attr = file_attr(path);
		if ((attr == (short)-1) || (attr & FILEATTR_DIRECTORY)) {
			file_cpyname(missing, path, missing_size);
			return(FAILURE);
		}
	}
	return(SUCCESS);
}

static BOOL smoke_try_romdir(const char *dir, char *missing,
														int missing_size) {

	if (smoke_romdir_complete(dir, missing, missing_size) != SUCCESS) {
		return(FAILURE);
	}
	file_cpyname(np2cfg.biospath, dir, sizeof(np2cfg.biospath));
	return(SUCCESS);
}

static BOOL smoke_try_romimage_under(const char *dir, char *missing,
														int missing_size) {

	char	path[MAX_PATH];

	if ((dir == NULL) || (dir[0] == '\0')) {
		return(FAILURE);
	}
	file_cpyname(path, dir, sizeof(path));
	file_setseparator(path, sizeof(path));
	file_catname(path, "romimage", sizeof(path));
	return(smoke_try_romdir(path, missing, missing_size));
}

static BOOL smoke_try_disk_romdirs(char *disk[2], char *missing,
														int missing_size) {

	char	dir[MAX_PATH];

	if (disk[0] == NULL) {
		return(FAILURE);
	}
	file_cpyname(dir, disk[0], sizeof(dir));
	file_cutname(dir);
	if (dir[0] == '\0') {
		file_cpyname(dir, ".", sizeof(dir));
	}
	if (smoke_try_romdir(dir, missing, missing_size) == SUCCESS) {
		return(SUCCESS);
	}
	return(smoke_try_romimage_under(dir, missing, missing_size));
}

static BOOL smoke_try_basepath_romdirs(char *missing, int missing_size) {

	char	*base;
	BOOL	ret;

	base = SDL_GetBasePath();
	if (base == NULL) {
		return(FAILURE);
	}
	ret = smoke_try_romdir(base, missing, missing_size);
	if (ret != SUCCESS) {
		ret = smoke_try_romimage_under(base, missing, missing_size);
	}
	SDL_free(base);
	return(ret);
}

static BOOL smoke_resolve_rompath(char *disk[2]) {

	char	missing[MAX_PATH];

	missing[0] = '\0';
	if (smoke_try_romdir(np2cfg.biospath, missing, sizeof(missing))
															== SUCCESS) {
		return(SUCCESS);
	}
	if (smoke_try_romdir(".", missing, sizeof(missing)) == SUCCESS) {
		return(SUCCESS);
	}
	if (smoke_try_romdir("romimage", missing, sizeof(missing)) == SUCCESS) {
		return(SUCCESS);
	}
	if (smoke_try_disk_romdirs(disk, missing, sizeof(missing)) == SUCCESS) {
		return(SUCCESS);
	}
	if (smoke_try_basepath_romdirs(missing, sizeof(missing)) == SUCCESS) {
		return(SUCCESS);
	}
	fprintf(stderr,
			"Error: smoke ROM set incomplete: missing %s; set biospath "
			"or run from a directory containing VA ROMs\n",
			(missing[0] != '\0') ? missing : smoke_required_roms[0]);
	return(FAILURE);
}

static BOOL check_fdd_image(const char *path) {

	short	attr;

	attr = file_attr(path);
	if (attr == (short)-1) {
		fprintf(stderr, "Error: FDD image not found: %s\n", path);
		return(FAILURE);
	}
	if (attr & FILEATTR_DIRECTORY) {
		fprintf(stderr, "Error: FDD image is a directory: %s\n", path);
		return(FAILURE);
	}
	return(SUCCESS);
}

static void mount_fdd_images(char *disk[2]) {

	if (disk[0]) {
		diskdrv_setfdd(0, disk[0], 0);
	}
	if (disk[1]) {
		diskdrv_setfdd(1, disk[1], 0);
	}
}

static BOOL smoke_check_screen(UINT frames, BOOL *done) {

	BOOL	uniform;

	*done = FALSE;
	if (scrnmng_texture_uniform(&uniform) != SUCCESS) {
		fprintf(stderr,
				"Error: smoke screen detector could not read guest texture\n");
		return(FAILURE);
	}
	if (!uniform) {
		*done = TRUE;
		return(SUCCESS);
	}
	if (frames >= smoke_timeout_frames) {
		fprintf(stderr,
				"Error: smoke screen detector: guest texture is uniform "
				"after %u frames\n",
				frames);
		return(FAILURE);
	}
	return(SUCCESS);
}

static void pacelog_initialize(PACELOG *log) {

	ZeroMemory(log, sizeof(*log));
	log->tick = GETTICK();
}

static void pacelog_update(PACELOG *log, BOOL enabled, UINT executed,
						   UINT skipped, UINT pending) {

	UINT32	now;

	if (!enabled) {
		return;
	}
	log->frames_executed += executed;
	log->frames_skipped += skipped;
	if (log->max_pending < pending) {
		log->max_pending = pending;
	}
	now = GETTICK();
	if ((UINT32)(now - log->tick) >= 1000) {
		fprintf(stderr,
				"pacelog frames_executed=%u frames_skipped=%u "
				"max_pending=%u renderer=%s\n",
				log->frames_executed, log->frames_skipped,
				log->max_pending, scrnmng_get_renderer_backend());
		log->tick = now;
		log->frames_executed = 0;
		log->frames_skipped = 0;
		log->max_pending = 0;
	}
}

static void framereset(UINT cnt) {

	framecnt = 0;
	(void)cnt;
}

static void service_host_idle(void) {

	taskmng_rol();
	timing_hosttick();
	if (taskmng_isavail()) {
		SDL_Delay(1);
	}
}

static void processwait(UINT cnt, PACELOG *pacelog, BOOL pacelog_enabled) {

	UINT	pending;

	pending = timing_getcount();
	pacelog_update(pacelog, pacelog_enabled, 0, 0, pending);
	if (pending >= cnt) {
		timing_setcount(0);
		framereset(cnt);
	}
	else {
		service_host_idle();
	}
	soundmng_sync();
}

static void run_guest_frame(BOOL draw) {

	if (draw) {
		gui_new_frame();
	}
	pccore_exec(draw);
	if (draw) {
		gui_draw();
		scrnmng_present_begin();
		gui_render();
		scrnmng_present_end();
	}
}

static BOOL smoke_after_frame(BOOL smoke, UINT frames) {

	BOOL	done;
	BOOL	ret;

	if (!smoke) {
		return(SUCCESS);
	}
	ret = smoke_check_screen(frames, &done);
	if ((ret != SUCCESS) || done) {
		taskmng_exit();
		return(ret);
	}
	return(SUCCESS);
}

static BOOL runloop(BOOL smoke, BOOL pacelog_enabled) {

	UINT	frames;
	PACELOG	pacelog;

	frames = 0;
	framecnt = 0;
	waitcnt = 0;
	framemax = 1;
	pacelog_initialize(&pacelog);
	while(taskmng_isavail()) {
		taskmng_rol();
		timing_hosttick();
		if (np2oscfg.NOWAIT) {
			BOOL	draw;

			draw = (framecnt == 0);
			run_guest_frame(draw);
			frames++;
			pacelog_update(&pacelog, pacelog_enabled, 1, draw ? 0 : 1, 0);
			if (smoke_after_frame(smoke, frames) != SUCCESS) {
				return(FAILURE);
			}
			if (np2oscfg.DRAW_SKIP) {
				framecnt++;
				if (framecnt >= np2oscfg.DRAW_SKIP) {
					processwait(0, &pacelog, pacelog_enabled);
				}
			}
			else {
				UINT	cnt;

				framecnt = 1;
				cnt = timing_getcount();
				pacelog_update(&pacelog, pacelog_enabled, 0, 0, cnt);
				if (cnt) {
					processwait(0, &pacelog, pacelog_enabled);
				}
			}
		}
		else if (np2oscfg.DRAW_SKIP) {
			if (framecnt < np2oscfg.DRAW_SKIP) {
				BOOL	draw;

				draw = (framecnt == 0);
				run_guest_frame(draw);
				frames++;
				pacelog_update(&pacelog, pacelog_enabled, 1,
							   draw ? 0 : 1, 0);
				if (smoke_after_frame(smoke, frames) != SUCCESS) {
					return(FAILURE);
				}
				framecnt++;
			}
			else {
				processwait(np2oscfg.DRAW_SKIP, &pacelog,
							pacelog_enabled);
			}
		}
		else {
			if (!waitcnt) {
				BOOL	draw;
				UINT	cnt;

				draw = (framecnt == 0);
				run_guest_frame(draw);
				frames++;
				pacelog_update(&pacelog, pacelog_enabled, 1,
							   draw ? 0 : 1, 0);
				if (smoke_after_frame(smoke, frames) != SUCCESS) {
					return(FAILURE);
				}
				framecnt++;
				cnt = timing_getcount();
				pacelog_update(&pacelog, pacelog_enabled, 0, 0, cnt);
				if (framecnt > cnt) {
					waitcnt = framecnt;
					if (framemax > 1) {
						framemax--;
					}
				}
				else if (framecnt >= framemax) {
					if (framemax < 12) {
						framemax++;
					}
					if (cnt >= max_catchup_frames) {
						timing_reset();
					}
					else {
						timing_setcount(cnt - framecnt);
					}
					framereset(0);
				}
			}
			else {
				processwait(waitcnt, &pacelog, pacelog_enabled);
				waitcnt = framecnt;
			}
		}
	}
	return(SUCCESS);
}

int main(int argc, char **argv) {

	int		pos;
	char	*p;
	BOOL	smoke;
	BOOL	fdctrace;
	BOOL	pacelog;
	BOOL	run_ok;
	int		disks;
	char	*disk[2];

	smoke = FALSE;
	fdctrace = FALSE;
	pacelog = FALSE;
	run_ok = SUCCESS;
	disks = 0;
	disk[0] = NULL;
	disk[1] = NULL;
	pos = 1;
	while(pos < argc) {
		p = argv[pos++];
		if ((!milstr_cmp(p, "-h")) || (!milstr_cmp(p, "--help"))) {
			usage(argv[0]);
			return(SUCCESS);
		}
		else if (!milstr_cmp(p, "--smoke")) {
			smoke = TRUE;
		}
		else if (!milstr_cmp(p, "--fdctrace")) {
			fdctrace = TRUE;
		}
		else if (!milstr_cmp(p, "--pacelog")) {
			pacelog = TRUE;
		}
		else if (p[0] == '-') {
			fprintf(stderr, "error command: %s\n", p);
			return(FAILURE);
		}
		else {
			if (disks >= 2) {
				fprintf(stderr, "Error: too many FDD images: %s\n", p);
				return(FAILURE);
			}
			disk[disks++] = p;
		}
	}

	SDL_SetMainReady();
	if (SDL_Init(0) < 0) {
		fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
		return(FAILURE);
	}

	dosio_init();
	file_setcd("./");
	for (pos=0; pos<disks; pos++) {
		if (check_fdd_image(disk[pos]) != SUCCESS) {
			SDL_Quit();
			dosio_term();
			return(FAILURE);
		}
	}
	initload();
	if (smoke) {
		smoke_configure_va();
		if (smoke_resolve_rompath(disk) != SUCCESS) {
			SDL_Quit();
			dosio_term();
			return(FAILURE);
		}
	}
	warn_va_config_sanity();
	set_default_rompath();
	if (smoke) {
		np2oscfg.NOWAIT = 1;
		np2oscfg.resume = 0;
	}

	TRACEINIT();
	fdc_trace_enable(fdctrace);
	sdlkbd_initialize();
	inputmng_init();
	keystat_initialize();

	scrnmng_initialize();
	if (scrnmng_create(FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT) != SUCCESS) {
		goto np2main_err2;
	}
	scrnmng_set_display(np2oscfg.gui_scale, np2oscfg.gui_aspect);
	if (gui_initialize(scrnmng_get_window(), scrnmng_get_renderer(),
					   argv[0]) != SUCCESS) {
		goto np2main_err3;
	}
	scrnmng_show();

	soundmng_initialize();
	commng_initialize();
	sysmng_initialize();
	taskmng_initialize();
	pccore_init();
#if defined(SUPPORT_PC88VA)
	bkupmemva_load();
#endif
	S98_init();

	pccore_reset();
	scrndraw_redraw();
	mount_fdd_images(disk);
	run_ok = runloop(smoke, pacelog);

	pccore_cfgupdate();
	if ((!smoke) && (sys_updates & (SYS_UPDATECFG | SYS_UPDATEOSCFG))) {
		initsave();
	}
#if defined(SUPPORT_PC88VA)
	bkupmemva_save();
#endif
	pccore_term();
	S98_trash();
	soundmng_deinitialize();
	gui_shutdown();
	scrnmng_destroy();
	TRACETERM();
	SDL_Quit();
	dosio_term();
	return(run_ok);

np2main_err3:
	gui_shutdown();
	scrnmng_destroy();

np2main_err2:
	TRACETERM();
	SDL_Quit();
	dosio_term();
	return(FAILURE);
}
