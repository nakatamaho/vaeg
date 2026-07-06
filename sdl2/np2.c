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

static UINT pending_guest_frames(BOOL *first_frame, BOOL smoke,
								 UINT *observed_pending) {

	UINT	pending;

	*observed_pending = 0;
	if (*first_frame) {
		*first_frame = FALSE;
		*observed_pending = 1;
		return(1);
	}
	if ((!smoke) && np2oscfg.NOWAIT) {
		timing_setcount(0);
		*observed_pending = 1;
		return(1);
	}
	pending = timing_getcount();
	*observed_pending = pending;
	if (pending == 0) {
		return(0);
	}
	if (pending > max_catchup_frames) {
		pending = max_catchup_frames;
		timing_reset();
	}
	else {
		timing_setcount(0);
	}
	return(pending);
}

static void run_guest_frames(UINT pending) {

	UINT	i;

	for (i=0; (i<pending) && taskmng_isavail(); i++) {
		pccore_exec((i + 1) == pending);
	}
}

static BOOL runloop(BOOL smoke, BOOL pacelog_enabled) {

	UINT	frames;
	BOOL	first_frame;
	PACELOG	pacelog;

	frames = 0;
	first_frame = TRUE;
	pacelog_initialize(&pacelog);
	while(taskmng_isavail()) {
		UINT	pending;
		UINT	observed_pending;

		taskmng_rol();
		timing_hosttick();
		pending = pending_guest_frames(&first_frame, smoke, &observed_pending);
		if (pending == 0) {
			pacelog_update(&pacelog, pacelog_enabled, 0, 0,
						   observed_pending);
			taskmng_sleep(1);
			continue;
		}
		gui_new_frame();
		run_guest_frames(pending);
		gui_draw();
		scrnmng_present_begin();
		gui_render();
		scrnmng_present_end();
		frames += pending;
		pacelog_update(&pacelog, pacelog_enabled, pending, pending - 1,
					   observed_pending);
		if (smoke) {
			BOOL	done;
			BOOL	ret;

			ret = smoke_check_screen(frames, &done);
			if ((ret != SUCCESS) || done) {
				taskmng_exit();
				return(ret);
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
	S98_init();

	pccore_reset();
	scrndraw_redraw();
	mount_fdd_images(disk);
	run_ok = runloop(smoke, pacelog);

	pccore_cfgupdate();
	if ((!smoke) && (sys_updates & (SYS_UPDATECFG | SYS_UPDATEOSCFG))) {
		initsave();
	}
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
