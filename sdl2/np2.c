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

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]       : print this message\n");
	printf("\t--smoke             : initialize SDL2, run a short core loop, exit\n");
	printf("\t--fdctrace          : print one FDC trace line per command to stderr\n");
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

static UINT pending_guest_frames(BOOL *first_frame, BOOL smoke) {

	UINT	pending;

	if (*first_frame) {
		*first_frame = FALSE;
		return(1);
	}
	if ((!smoke) && np2oscfg.NOWAIT) {
		timing_setcount(0);
		return(1);
	}
	pending = timing_getcount();
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

static BOOL runloop(BOOL smoke) {

	UINT	frames;
	BOOL	first_frame;

	frames = 0;
	first_frame = TRUE;
	while(taskmng_isavail()) {
		UINT	pending;

		taskmng_rol();
		pending = pending_guest_frames(&first_frame, smoke);
		if (pending == 0) {
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
	BOOL	run_ok;
	int		disks;
	char	*disk[2];

	smoke = FALSE;
	fdctrace = FALSE;
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
