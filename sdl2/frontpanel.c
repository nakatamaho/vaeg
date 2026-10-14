/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Front panel lamp state (vaeg M106): the FDD access lamps, lit while a
 * drive is accessed (red for 2D/2DD media, green for 2HD), and the V1/V2/V3
 * mode lamps following port 1CDh. sdl2/scrnmng.c draws them as a slim bar
 * below the guest screen.
 */

#include "compiler.h"
#include "frontpanel.h"
#include "np2.h"
#include "fdd/fddfile.h"

#include <SDL.h>

enum {
	FRONTPANEL_DRIVES = 2,
	FRONTPANEL_MODES = 3,
	/* how long one sector access keeps the lamp lit */
	FRONTPANEL_ACCESS_MS = 120
};

static struct {
	BOOL mode_on[FRONTPANEL_MODES];
	UINT32 access_tick[FRONTPANEL_DRIVES];
	BOOL accessed[FRONTPANEL_DRIVES];
} panel;

void frontpanel_set_modeled(UINT num, BOOL on) {
	if (num < FRONTPANEL_MODES) {
		panel.mode_on[num] = on;
	}
}

void frontpanel_fdd_access(UINT drv) {
	if (drv < FRONTPANEL_DRIVES) {
		panel.access_tick[drv] = SDL_GetTicks();
		panel.accessed[drv] = TRUE;
	}
}

BOOL frontpanel_mode_lit(UINT num) {
	return (num < FRONTPANEL_MODES) ? panel.mode_on[num] : FALSE;
}

BOOL frontpanel_drive_lit(UINT drv, UINT32 now) {
	if ((drv >= FRONTPANEL_DRIVES) || !panel.accessed[drv]) {
		return FALSE;
	}
	return ((UINT32)(now - panel.access_tick[drv]) < FRONTPANEL_ACCESS_MS) ? TRUE : FALSE;
}

BOOL frontpanel_drive_is_2hd(UINT drv) {
	const _FDDFILE *fdd;

	if (drv >= MAX_FDDFILE) {
		return FALSE;
	}
	fdd = &fddfile[drv];
	if (fdd->type == DISKTYPE_D88) {
		return (fdd->inf.d88.fdtype_major == DISKTYPE_2HD) ? TRUE : FALSE;
	}
	if (fdd->type == DISKTYPE_BETA) {
		return (fdd->inf.xdf.disktype == DISKTYPE_2HD) ? TRUE : FALSE;
	}
	return FALSE;
}

int frontpanel_simple_scale(int width) {
	const int scale = (width + 320) / 640;

	return (scale < 1) ? 1 : scale;
}

int frontpanel_height(int width) {
	if ((np2oscfg.front_panel == FRONTPANEL_OFF) || (width <= 0)) {
		return 0;
	}
	return 14 * frontpanel_simple_scale(width);
}
