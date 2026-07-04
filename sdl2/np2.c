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

		NP2OSCFG	np2oscfg = {0, 0, 0, 0, 0};

static const UINT smoke_frames = 60;

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]       : print this message\n");
	printf("\t--smoke             : initialize SDL2, run a short core loop, exit\n");
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
		now = SDL_GetPerformanceCounter();
	}
}

static void runloop(BOOL smoke) {

	UINT	frames;
	Uint64	next_tick;

	frames = 0;
	next_tick = SDL_GetPerformanceCounter();
	while(taskmng_isavail()) {
		taskmng_rol();
		pccore_exec(TRUE);
		frames++;
		if (smoke && (frames >= smoke_frames)) {
			taskmng_exit();
			break;
		}
		wait_next_frame(&next_tick);
	}
}

int main(int argc, char **argv) {

	int		pos;
	char	*p;
	BOOL	smoke;
	int		disks;
	char	*disk[2];

	smoke = FALSE;
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
	file_setcd("./");
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

	soundmng_initialize();
	commng_initialize();
	sysmng_initialize();
	taskmng_initialize();
	pccore_init();
	S98_init();

	scrndraw_redraw();
	pccore_reset();
	mount_fdd_images(disk);
	runloop(smoke);

	pccore_cfgupdate();
	if ((!smoke) && (sys_updates & (SYS_UPDATECFG | SYS_UPDATEOSCFG))) {
		initsave();
	}
	pccore_term();
	S98_trash();
	soundmng_deinitialize();
	scrnmng_destroy();
	TRACETERM();
	SDL_Quit();
	dosio_term();
	return(SUCCESS);

np2main_err2:
	TRACETERM();
	SDL_Quit();
	dosio_term();
	return(FAILURE);
}
