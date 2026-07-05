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
#include	"timing.h"
#include	"keystat.h"
#include	"gui/gui.h"

		NP2OSCFG	np2oscfg = {0, 0, 0, 0, 0, 1, 0};

static const UINT smoke_frames = 60;

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]       : print this message\n");
	printf("\t--smoke             : initialize SDL2, run a short core loop, exit\n");
	printf("\timage1 [image2]     : mount FDD images in drive 1 and 2\n");
}

static BOOL path_is_absolute(const char *path) {

	if ((path == NULL) || (path[0] == '\0')) {
		return(FAILURE);
	}
	if ((path[0] == '/') || (path[0] == '\\')) {
		return(SUCCESS);
	}
	if (((path[0] >= 'A') && (path[0] <= 'Z')) ||
		((path[0] >= 'a') && (path[0] <= 'z'))) {
		if (path[1] == ':') {
			return(SUCCESS);
		}
	}
	return(FAILURE);
}

static void set_executable_current_dir(void) {

	char	*base;

	base = SDL_GetBasePath();
	if (base != NULL) {
		file_setcd(base);
		SDL_free(base);
	}
	else {
		file_setcd("./");
	}
}

static void set_default_rompath(void) {

	char	path[MAX_PATH];

	if (np2cfg.biospath[0] != '\0') {
		if (path_is_absolute(np2cfg.biospath) != SUCCESS) {
			file_cpyname(path, file_getcd(np2cfg.biospath), sizeof(path));
			file_cpyname(np2cfg.biospath, path, sizeof(np2cfg.biospath));
		}
		return;
	}
	if ((file_attr_c("romimage") & FILEATTR_DIRECTORY) != 0) {
		file_cpyname(np2cfg.biospath, file_getcd("romimage"),
					 sizeof(np2cfg.biospath));
	}
	else {
		file_cpyname(np2cfg.biospath, file_getcd(""),
					 sizeof(np2cfg.biospath));
	}
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

static void wait_next_frame(Uint64 *next_tick) {

	Uint64	now;
	Uint64	freq;
	Uint64	frame_ticks;

	freq = SDL_GetPerformanceFrequency();
	frame_ticks = freq / 60;
	*next_tick += frame_ticks;
	now = SDL_GetPerformanceCounter();
	while(taskmng_isavail() && (now < *next_tick)) {
		Uint64	remaining;
		remaining = ((*next_tick - now) * 1000) / freq;
		if (remaining > 1) {
			SDL_Delay((Uint32)(remaining - 1));
		}
		else {
			SDL_Delay(0);
		}
		taskmng_rol();
		timing_hosttick();
		now = SDL_GetPerformanceCounter();
	}
}

static BOOL smoke_check_screen(UINT frames) {

	BOOL	uniform;

	if (scrnmng_texture_uniform(&uniform) != SUCCESS) {
		fprintf(stderr,
				"Error: smoke screen detector could not read guest texture\n");
		return(FAILURE);
	}
	if (uniform) {
		fprintf(stderr,
				"Error: smoke screen detector: guest texture is uniform "
				"after %u frames\n",
				frames);
		return(FAILURE);
	}
	return(SUCCESS);
}

static BOOL runloop(BOOL smoke) {

	UINT	frames;
	Uint64	next_tick;

	frames = 0;
	next_tick = SDL_GetPerformanceCounter();
	while(taskmng_isavail()) {
		taskmng_rol();
		gui_new_frame();
		pccore_exec(TRUE);
		gui_draw();
		scrnmng_present_begin();
		gui_render();
		scrnmng_present_end();
		frames++;
		if (smoke && (frames >= smoke_frames)) {
			BOOL	ret;

			ret = smoke_check_screen(frames);
			taskmng_exit();
			return(ret);
		}
		wait_next_frame(&next_tick);
	}
	return(SUCCESS);
}

int main(int argc, char **argv) {

	int		pos;
	char	*p;
	BOOL	smoke;
	BOOL	run_ok;
	int		disks;
	char	*disk[2];

	smoke = FALSE;
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
	set_executable_current_dir();
	for (pos=0; pos<disks; pos++) {
		if (check_fdd_image(disk[pos]) != SUCCESS) {
			SDL_Quit();
			dosio_term();
			return(FAILURE);
		}
	}
	initload();
	set_default_rompath();
	if (smoke) {
		np2oscfg.NOWAIT = 1;
		np2oscfg.resume = 0;
	}

	TRACEINIT();
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
	run_ok = runloop(smoke);

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
