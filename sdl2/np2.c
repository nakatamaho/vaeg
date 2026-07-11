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
#include	"sgp.h"
#include	"diskdrv.h"
#include	"fdc.h"
#include	"timing.h"
#include	"keystat.h"
#include	"bkupmemva.h"
#include	"gui/gui.h"
#include	"romcheck.h"
#include	"selftest.h"
#include	"splash.h"
#include	"np2ver.h"

		NP2OSCFG	np2oscfg = {0, 0, 0, 0, 0, 1, 0, "", "", {"", ""},
								"", "", 0, 0, "", "ymfm"};

static const UINT smoke_timeout_frames = 600;
static const UINT startup_splash_ms = 1500;
static const UINT max_catchup_frames = 15;
static const char backup_memory_file[] = "vabkupmem.dat";
typedef struct {
	const char *name;
	UINT32 size;
	UINT32 crc32;
	const char *sha1;
} ROMEXPECTED;

/* MAME src/mame/nec/pc88va.cpp ROM_START(pc88va). */
static const ROMEXPECTED va_required_roms[] = {
	{"vafont.rom", 0x50000, 0xfaf7c466,
		"196b3d5b7407cb4f286ffe5c1e34ebb1f6905a8c"},
	{"vadic.rom", 0x80000, 0xf913c605,
		"5ba1f3578d0aaacdaf7194a80e6d520c81ae55fb"},
	{"varom00.rom", 0x80000, 0x8a853b00,
		"1266ba969959ff25433ecc900a2caced26ef1a9e"},
	{"varom08.rom", 0x20000, 0x154803cc,
		"7e6591cd465cbb35d6d3446c5a83b46d30fafe95"},
	{"varom1.rom", 0x20000, 0x0783b16a,
		"54536dc03238b4668c8bb76337efade001ec7826"},
	{NULL, 0, 0, NULL}
};

/* MAME src/mame/nec/pc88va.cpp ROM_START(pc88va2), without fallback. */
static const ROMEXPECTED va2_required_roms[] = {
	{"vafont_va2.rom", 0x50000, 0xb40d34e4,
		"a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f"},
	{"vadic_va2.rom", 0x80000, 0xa6108f4d,
		"3665db538598abb45d9dfe636423e6728a812b12"},
	{"varom00_va2.rom", 0x80000, 0x98c9959a,
		"bcaea28c58816602ca1e8290f534360f1ca03fe8"},
	{"varom08_va2.rom", 0x20000, 0xeef6d4a0,
		"47e5f89f8b0ce18ff8d5d7b7aef8ca0a2a8e3345"},
	{"varom1_va2.rom", 0x20000, 0x7e767f00,
		"dd4f4521bfbb068f15ab3bcdb8d47c7d82b9d1d4"},
	{NULL, 0, 0, NULL}
};

/* MAME lists this ROM in a disabled FDD subsystem declaration. */
static const ROMEXPECTED extra_roms[] = {
	{"vasubsys.rom", 0x2000, 0x08962850,
		"a9375aa480f85e1422a0e1385acb0ea170c5c2e0"},
	{NULL, 0, 0, NULL}
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

	printf("88VA Eternal Grafx %s (%s)\n", VAEGREL_CORE, NP2VER_CORE);
	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]       : print this message\n");
	printf("\t--smoke             : initialize SDL2, run a short core loop, exit\n");
	printf("\t--selftest          : run ROM-less unit tests and exit\n");
	printf("\t--fdctrace          : print one FDC trace line per command to stderr\n");
	printf("\t--pacelog           : print pacing counters once per second\n");
	printf("\timage1 [image2]     : mount FDD images in drive 1 and 2\n");
}

static void smoke_configure_va(void) {

	file_cpyname(np2cfg.model, str_VA2, sizeof(np2cfg.model));
	np2cfg.baseclock = PCBASECLOCK40;
	np2cfg.multiple = 2;
	np2cfg.sgp_speed_mode = SGP_SPEED_MODEL_DEFAULT;
	np2cfg.sgp_multiplier = 1;
	np2cfg.ITF_WORK = 1;
	np2cfg.EXTMEM = 1;
	np2cfg.SOUND_SW = FMBOARD_VA_OPNA;
}

UINT16 np2_default_sound_for_model(const char *model) {

	if (milstr_cmp(model, str_VA1) == 0) {
		return(FMBOARD_VA_OPN);
	}
	if (milstr_cmp(model, str_VA2) == 0) {
		return(FMBOARD_VA_OPNA);
	}
	return(0x0004);
}

BOOL np2_sound_hardware_valid(const char *model, UINT16 sound) {

	if (milstr_cmp(model, str_VA1) == 0) {
		return((sound == FMBOARD_VA_OPN) ||
			   (sound == FMBOARD_VA_OPNA));
	}
	if (milstr_cmp(model, str_VA2) == 0) {
		return(sound == FMBOARD_VA_OPNA);
	}
	return(sound != FMBOARD_NONE);
}

static BOOL config_selects_va(void) {

	return((milstr_cmp(np2cfg.model, str_VA1) == 0) ||
		   (milstr_cmp(np2cfg.model, str_VA2) == 0));
}

static void warn_va_config_sanity(void) {

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
	if (!np2_sound_hardware_valid(np2cfg.model, np2cfg.SOUND_SW)) {
		fprintf(stderr,
					"WARNING: PC-88VA sound hardware is invalid for %s "
					"(SNDboard=%03x; expected=%03x).\n", np2cfg.model,
					np2cfg.SOUND_SW,
					np2_default_sound_for_model(np2cfg.model));
	}
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

static const char *rom_model_label(const char *model) {

	return((milstr_cmp(model, str_VA1) == 0) ? "VA" : "VA2/VA3");
}

static const ROMEXPECTED *rom_expected_set(const char *model) {

	return((milstr_cmp(model, str_VA1) == 0) ?
								va_required_roms : va2_required_roms);
}

static BOOL romset_complete(const char *dir, const char *model, char *missing,
														int missing_size) {

	int		i;
	const ROMEXPECTED *required;

	if ((dir == NULL) || (dir[0] == '\0')) {
		return(FAILURE);
	}
	required = rom_expected_set(model);
	for (i=0; required[i].name != NULL; i++) {
		char	path[MAX_PATH];
		short	attr;

		make_rom_path(path, sizeof(path), dir, required[i].name);
		attr = file_attr(path);
		if ((attr == (short)-1) || (attr & FILEATTR_DIRECTORY)) {
			file_cpyname(missing, path, missing_size);
			return(FAILURE);
		}
	}
	for (i=0; extra_roms[i].name != NULL; i++) {
		char	path[MAX_PATH];
		short	attr;

		make_rom_path(path, sizeof(path), dir, extra_roms[i].name);
		attr = file_attr(path);
		if ((attr == (short)-1) || (attr & FILEATTR_DIRECTORY)) {
			file_cpyname(missing, path, missing_size);
			return(FAILURE);
		}
	}
	return(SUCCESS);
}

static void select_backup_memory_path(void) {

	char	path[MAX_PATH];
	char	*base;
	short	attr;

	base = SDL_GetBasePath();
	if (base != NULL) {
		file_cpyname(path, base, sizeof(path));
		SDL_free(base);
		file_catname(path, backup_memory_file, sizeof(path));
		attr = file_attr(path);
		if ((attr >= 0) && !(attr & FILEATTR_DIRECTORY)) {
			bkupmemva_setpath(path);
			SDL_Log("Backup memory: %s", path);
			return;
		}
	}
	file_getstatepath(path, sizeof(path), backup_memory_file);
	bkupmemva_setpath(path);
	SDL_Log("Backup memory: %s", path);
}

static void verify_rom(const char *dir, const char *source,
											const ROMEXPECTED *expected) {

	char	path[MAX_PATH];
	char	sha1[41];
	ROMCHECKSUM actual;

	make_rom_path(path, sizeof(path), dir, expected->name);
	if (romcheck_file(path, &actual) != SUCCESS) {
		fprintf(stderr, "WARNING: ROM checksum read failed: %s\n", path);
		return;
	}
	romcheck_sha1_string(actual.sha1, sha1);
	if ((actual.size != expected->size) ||
		(actual.crc32 != expected->crc32) || strcmp(sha1, expected->sha1)) {
		fprintf(stderr,
				"WARNING: ROM differs from MAME %s: %s; "
				"expected size=%u crc32=%08x sha1=%s; "
				"actual size=%u crc32=%08x sha1=%s\n",
				source, path, expected->size, expected->crc32, expected->sha1,
				actual.size, actual.crc32, sha1);
	}
}

static void verify_romset(const char *dir, const char *model) {

	const ROMEXPECTED *required;
	const char *source;
	int i;

	required = rom_expected_set(model);
	source = (milstr_cmp(model, str_VA1) == 0) ? "pc88va" : "pc88va2";
	for (i=0; required[i].name != NULL; i++) {
		verify_rom(dir, source, &required[i]);
	}
	for (i=0; extra_roms[i].name != NULL; i++) {
		verify_rom(dir, "pc88va extra", &extra_roms[i]);
	}
}

static BOOL resolve_model_rompath(char *missing, int missing_size) {

	char	*base;
	char	primary[MAX_PATH];
	char	fallback[MAX_PATH];
	char	primary_missing[MAX_PATH];
	char	fallback_missing[MAX_PATH];
	BOOL	primary_available;

	primary[0] = '\0';
	primary_missing[0] = '\0';
	fallback_missing[0] = '\0';
	primary_available = FALSE;
	base = SDL_GetBasePath();
	if (base != NULL) {
		file_cpyname(primary, base, sizeof(primary));
		SDL_free(base);
		primary_available = TRUE;
		if (romset_complete(primary, np2cfg.model, primary_missing,
										sizeof(primary_missing)) == SUCCESS) {
			file_cpyname(np2cfg.biospath, primary,
												sizeof(np2cfg.biospath));
			verify_romset(primary, np2cfg.model);
			missing[0] = '\0';
			return(SUCCESS);
		}
	}

	file_cpyname(fallback, ".", sizeof(fallback));
	if (romset_complete(fallback, np2cfg.model, fallback_missing,
										sizeof(fallback_missing)) == SUCCESS) {
		file_cpyname(np2cfg.biospath, fallback, sizeof(np2cfg.biospath));
		verify_romset(fallback, np2cfg.model);
		missing[0] = '\0';
		return(SUCCESS);
	}

	if (primary_available) {
		file_cpyname(np2cfg.biospath, primary, sizeof(np2cfg.biospath));
		file_cpyname(missing, primary_missing, missing_size);
	}
	else {
		file_cpyname(np2cfg.biospath, fallback, sizeof(np2cfg.biospath));
		file_cpyname(missing, fallback_missing, missing_size);
	}
	return(FAILURE);
}

static void report_model_rompath(BOOL result, const char *missing) {

	if (result == SUCCESS) {
		fprintf(stderr,
				"INFO: PC-88VA %s ROM path: %s\n",
				rom_model_label(np2cfg.model), np2cfg.biospath);
	}
	else {
		const ROMEXPECTED *required;

		required = rom_expected_set(np2cfg.model);
		fprintf(stderr,
					"WARNING: PC-88VA %s ROM set not found or incomplete; "
					"expected root: %s; missing: %s\n",
					rom_model_label(np2cfg.model), np2cfg.biospath,
					(missing[0] != '\0') ? missing : required[0].name);
	}
}

BOOL np2_select_boot_model(const char *model) {

	char	missing[MAX_PATH];
	BOOL	result;
	BOOL	changed;

	if ((milstr_cmp(model, str_VA1) != 0) &&
		(milstr_cmp(model, str_VA2) != 0)) {
		return(FAILURE);
	}
	changed = (milstr_cmp(np2cfg.model, model) != 0);
	file_cpyname(np2cfg.model, model, sizeof(np2cfg.model));
	if (changed) {
		np2cfg.SOUND_SW = np2_default_sound_for_model(model);
	}
	result = resolve_model_rompath(missing, sizeof(missing));
	report_model_rompath(result, missing);
	return(result);
}

static BOOL smoke_resolve_rompath(void) {

	char	missing[MAX_PATH];
	BOOL	result;
	const ROMEXPECTED *required;

	required = rom_expected_set(np2cfg.model);
	result = resolve_model_rompath(missing, sizeof(missing));
	if (result == SUCCESS) {
		fprintf(stderr,
				"smoke: PC-88VA %s ROM set found at %s; "
				"uniform-screen detector enabled\n",
				rom_model_label(np2cfg.model), np2cfg.biospath);
		return(SUCCESS);
	}
	fprintf(stderr,
			"smoke: PC-88VA %s ROM-less mode: expected root %s; missing %s; "
			"uniform-screen detector disabled\n",
			rom_model_label(np2cfg.model), np2cfg.biospath,
			(missing[0] != '\0') ? missing : required[0].name);
	return(FAILURE);
}

static BOOL check_fdd_image(const char *path, const char *level) {

	short	attr;

	attr = file_attr(path);
	if (attr == (short)-1) {
		fprintf(stderr, "%s: FDD image not found: %s\n", level, path);
		return(FAILURE);
	}
	if (attr & FILEATTR_DIRECTORY) {
		fprintf(stderr, "%s: FDD image is a directory: %s\n", level, path);
		return(FAILURE);
	}
	return(SUCCESS);
}

static void mount_fdd_images(char *disk[2]) {

	int drive;

	for (drive=0; drive<2; drive++) {
		const char *path;

		path = disk[drive];
		if (path == NULL) {
			path = np2oscfg.fdd_image[drive];
			if ((path[0] != '\0') &&
				(check_fdd_image(path, "Warning") != SUCCESS)) {
				continue;
			}
		}
		if ((path == NULL) || (path[0] == '\0')) {
			continue;
		}
		diskdrv_setfdd((REG8)drive, path, 0);
		if (disk[drive] != NULL) {
			file_cpyname(np2oscfg.fdd_image[drive], path,
									sizeof(np2oscfg.fdd_image[drive]));
			sysmng_update(SYS_UPDATEOSCFG);
		}
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

static BOOL smoke_after_frame(BOOL smoke, UINT frames, BOOL detect_screen) {

	BOOL	done;
	BOOL	ret;

	if (!smoke) {
		return(SUCCESS);
	}
	if (!detect_screen) {
		if (frames >= 1) {
			taskmng_exit();
		}
		return(SUCCESS);
	}
	ret = smoke_check_screen(frames, &done);
	if ((ret != SUCCESS) || done) {
		taskmng_exit();
		return(ret);
	}
	return(SUCCESS);
}

static BOOL runloop(BOOL smoke, BOOL pacelog_enabled, BOOL detect_screen) {

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
			if (smoke_after_frame(smoke, frames, detect_screen) != SUCCESS) {
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
				if (smoke_after_frame(smoke, frames, detect_screen)
															!= SUCCESS) {
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
				if (smoke_after_frame(smoke, frames, detect_screen)
															!= SUCCESS) {
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

static void wait_startup_splash(UINT32 started) {

	while(taskmng_isavail()) {
		UINT32 elapsed;
		UINT32 remaining;

		elapsed = GETTICK() - started;
		if (elapsed >= startup_splash_ms) {
			break;
		}
		taskmng_rol();
		remaining = startup_splash_ms - elapsed;
		SDL_Delay((remaining < 10) ? remaining : 10);
	}
}

int main(int argc, char **argv) {

	int		pos;
	char	*p;
	BOOL	smoke;
	BOOL	selftest;
	BOOL	fdctrace;
	BOOL	pacelog;
	BOOL	smoke_detect_screen;
	BOOL	splash_visible;
	BOOL	run_ok;
	UINT32	splash_started;
	int		disks;
	char	*disk[2];

	smoke = FALSE;
	selftest = FALSE;
	fdctrace = FALSE;
	pacelog = FALSE;
	smoke_detect_screen = FALSE;
	splash_visible = FALSE;
	run_ok = SUCCESS;
	splash_started = 0;
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
		else if (!milstr_cmp(p, "--selftest")) {
			selftest = TRUE;
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
	if (selftest) {
		run_ok = vaeg_selftest_run();
		SDL_Quit();
		dosio_term();
		return(run_ok);
	}
	for (pos=0; pos<disks; pos++) {
		if (check_fdd_image(disk[pos], "Error") != SUCCESS) {
			SDL_Quit();
			dosio_term();
			return(FAILURE);
		}
	}
	initload();
	select_backup_memory_path();
	if (smoke) {
		smoke_configure_va();
		smoke_detect_screen = (smoke_resolve_rompath() == SUCCESS);
	}
	else {
		char missing[MAX_PATH];
		BOOL result;

		result = resolve_model_rompath(missing, sizeof(missing));
		report_model_rompath(result, missing);
	}
	warn_va_config_sanity();
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
	SDL_PumpEvents();
	if ((!smoke) && (splash_show() == SUCCESS)) {
		splash_visible = TRUE;
		splash_started = GETTICK();
	}

	soundmng_initialize();
	commng_initialize();
	sysmng_initialize();
	taskmng_initialize();
	pccore_init();
	bkupmemva_load();
	S98_init();
	if (splash_visible) {
		wait_startup_splash(splash_started);
	}

	if (taskmng_isavail()) {
		pccore_reset();
		sdlkbd_reset_state();
		scrndraw_redraw();
		mount_fdd_images(disk);
		run_ok = runloop(smoke, pacelog, smoke_detect_screen);
	}

	pccore_cfgupdate();
	if ((!smoke) && (sys_updates & (SYS_UPDATECFG | SYS_UPDATEOSCFG))) {
		initsave();
	}
	bkupmemva_save();
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
