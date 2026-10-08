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
 * Front panel below the guest screen (vaeg M106): the maintainer's drawings
 * of the PC-88VA, VA2 and VA3 fronts (assets/front-panel-*.png), with the
 * FDD access lamps lit while a drive is accessed (red for 2D/2DD media,
 * green for 2HD) and the V1/V2/V3 mode lamps following port 1CDh.
 */

#include "compiler.h"
#include "frontpanel.h"
#include "pngdecode.h"
#include "np2.h"
#include "machine/pccore.h"
#include "fdd/fddfile.h"

#include <SDL.h>
#include <stdlib.h>

extern const unsigned char vaeg_front_panel_va_png[];
extern const unsigned int vaeg_front_panel_va_png_size;
extern const unsigned char vaeg_front_panel_va2_png[];
extern const unsigned int vaeg_front_panel_va2_png_size;
extern const unsigned char vaeg_front_panel_va3_png[];
extern const unsigned int vaeg_front_panel_va3_png_size;

enum {
	FRONTPANEL_ART_VA = 0,
	FRONTPANEL_ART_VA2,
	FRONTPANEL_ART_VA3,
	FRONTPANEL_ARTS,
	FRONTPANEL_DRIVES = 2,
	FRONTPANEL_MODES = 3,
	/* how long one sector access keeps the lamp lit */
	FRONTPANEL_ACCESS_MS = 120
};

typedef struct {
	const unsigned char *png;
	const unsigned int *png_size;
	int width;
	int height;
	/* lamp positions in artwork pixels: drive 1, drive 2, V1, V2, V3 */
	SDL_Rect drive[FRONTPANEL_DRIVES];
	SDL_Rect mode[FRONTPANEL_MODES];
} FRONTPANEL_ART;

static const FRONTPANEL_ART arts[FRONTPANEL_ARTS] = {
    {vaeg_front_panel_va_png,
     &vaeg_front_panel_va_png_size,
     1900,
     600,
     {{1210, 61, 42, 17}, {1210, 292, 42, 17}},
     {{591, 304, 33, 15}, {593, 347, 31, 16}, {592, 391, 32, 16}}},
    {vaeg_front_panel_va2_png,
     &vaeg_front_panel_va2_png_size,
     1900,
     750,
     {{1043, 131, 40, 17}, {1044, 349, 39, 17}},
     {{1697, 152, 27, 15}, {1697, 195, 27, 15}, {1697, 238, 27, 15}}},
    {vaeg_front_panel_va3_png,
     &vaeg_front_panel_va3_png_size,
     1900,
     750,
     {{1044, 131, 42, 20}, {1044, 356, 42, 20}},
     {{1698, 151, 28, 17}, {1698, 196, 28, 17}, {1698, 239, 28, 17}}},
};

static struct {
	SDL_Renderer *renderer;
	SDL_Texture *texture[FRONTPANEL_ARTS];
	BOOL failed[FRONTPANEL_ARTS];
	BOOL mode_on[FRONTPANEL_MODES];
	UINT32 access_tick[FRONTPANEL_DRIVES];
	BOOL accessed[FRONTPANEL_DRIVES];
} panel;

static int frontpanel_art(void) {
	if (pccore.model_va == PCMODEL_VA1) {
		return FRONTPANEL_ART_VA;
	}
	return np2oscfg.front_panel_va3 ? FRONTPANEL_ART_VA3 : FRONTPANEL_ART_VA2;
}

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

int frontpanel_height(int width) {
	const FRONTPANEL_ART *art;

	if (!np2oscfg.front_panel || (width <= 0)) {
		return 0;
	}
	art = &arts[frontpanel_art()];
	return (int)(((SINT64)width * art->height + art->width / 2) / art->width);
}

void frontpanel_release(void) {
	int i;

	for (i = 0; i < FRONTPANEL_ARTS; i++) {
		if (panel.texture[i] != NULL) {
			SDL_DestroyTexture(panel.texture[i]);
			panel.texture[i] = NULL;
		}
		panel.failed[i] = FALSE;
	}
	panel.renderer = NULL;
}

static SDL_Texture *frontpanel_texture(SDL_Renderer *renderer, int index) {
	const FRONTPANEL_ART *art = &arts[index];
	BYTE *pixels;
	UINT width;
	UINT height;
	SDL_Texture *texture;

	if (panel.renderer != renderer) {
		frontpanel_release();
		panel.renderer = renderer;
	}
	if ((panel.texture[index] != NULL) || panel.failed[index]) {
		return panel.texture[index];
	}
	pixels = vaeg_png_decode_rgba(art->png, *art->png_size, &width, &height);
	if ((pixels == NULL) || ((int)width != art->width) || ((int)height != art->height)) {
		free(pixels);
		panel.failed[index] = TRUE;
		return NULL;
	}
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC,
	                            (int)width, (int)height);
	if (texture != NULL) {
		SDL_UpdateTexture(texture, NULL, pixels, (int)width * 4);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(texture, SDL_ScaleModeBest);
	}
	free(pixels);
	panel.texture[index] = texture;
	panel.failed[index] = (texture == NULL) ? TRUE : FALSE;
	return texture;
}

static void frontpanel_lamp(SDL_Renderer *renderer, const SDL_Rect *dst, const FRONTPANEL_ART *art,
                            const SDL_Rect *lamp, Uint8 r, Uint8 g, Uint8 b) {
	SDL_Rect rect;
	int k;

	rect.x = dst->x + (int)(((SINT64)lamp->x * dst->w) / art->width);
	rect.y = dst->y + (int)(((SINT64)lamp->y * dst->h) / art->height);
	rect.w = max(1, (int)(((SINT64)lamp->w * dst->w) / art->width));
	rect.h = max(1, (int)(((SINT64)lamp->h * dst->h) / art->height));
	/* glow: widening translucent rectangles, then the lit lens */
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
	for (k = 4; k >= 1; k--) {
		SDL_Rect glow;
		const int grow = max(1, (rect.h * k) / 4);

		glow.x = rect.x - grow;
		glow.y = rect.y - grow;
		glow.w = rect.w + grow * 2;
		glow.h = rect.h + grow * 2;
		SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(18 + (4 - k) * 8));
		SDL_RenderFillRect(renderer, &glow);
	}
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	SDL_RenderFillRect(renderer, &rect);
	rect.h = max(1, rect.h / 4);
	rect.x += rect.w / 8;
	rect.w -= rect.w / 4;
	rect.y += 1;
	SDL_SetRenderDrawColor(renderer, (Uint8)min(255, r + 110), (Uint8)min(255, g + 110),
	                       (Uint8)min(255, b + 110), 255);
	SDL_RenderFillRect(renderer, &rect);
}

void frontpanel_render(SDL_Renderer *renderer, const SDL_Rect *dst) {
	const int index = frontpanel_art();
	const FRONTPANEL_ART *art = &arts[index];
	SDL_Texture *texture;
	const UINT32 now = SDL_GetTicks();
	UINT i;

	if ((renderer == NULL) || (dst == NULL) || (dst->w <= 0) || (dst->h <= 0)) {
		return;
	}
	texture = frontpanel_texture(renderer, index);
	if (texture == NULL) {
		return;
	}
	SDL_RenderCopy(renderer, texture, NULL, dst);
	for (i = 0; i < FRONTPANEL_DRIVES; i++) {
		if (frontpanel_drive_lit(i, now)) {
			if (frontpanel_drive_is_2hd(i)) {
				frontpanel_lamp(renderer, dst, art, &art->drive[i], 60, 235, 80);
			} else {
				frontpanel_lamp(renderer, dst, art, &art->drive[i], 255, 45, 30);
			}
		}
	}
	for (i = 0; i < FRONTPANEL_MODES; i++) {
		if (panel.mode_on[i]) {
			frontpanel_lamp(renderer, dst, art, &art->mode[i], 120, 255, 130);
		}
	}
}

BOOL frontpanel_selftest_decode(char *problem, size_t size) {
	int i;

	for (i = 0; i < FRONTPANEL_ARTS; i++) {
		const FRONTPANEL_ART *art = &arts[i];
		UINT width;
		UINT height;
		BYTE *pixels = vaeg_png_decode_rgba(art->png, *art->png_size, &width, &height);
		int k;

		if ((pixels == NULL) || ((int)width != art->width) || ((int)height != art->height)) {
			free(pixels);
			SDL_snprintf(problem, size, "front panel %d did not decode at its size", i);
			return FAILURE;
		}
		/* every lamp sits on an opaque, dark lens of the drawing */
		for (k = 0; k < FRONTPANEL_DRIVES + FRONTPANEL_MODES; k++) {
			const SDL_Rect *lamp =
			    (k < FRONTPANEL_DRIVES) ? &art->drive[k] : &art->mode[k - FRONTPANEL_DRIVES];
			const BYTE *p =
			    pixels +
			    (((size_t)(lamp->y + lamp->h / 2) * width) + (size_t)(lamp->x + lamp->w / 2)) * 4;

			if ((p[3] < 0xf0) || ((p[0] + p[1] + p[2]) > 300)) { /* the art is ~253 */
				free(pixels);
				SDL_snprintf(problem, size, "front panel %d lamp %d is not on a lens", i, k);
				return FAILURE;
			}
		}
		/* the corners are transparent */
		if (pixels[3] != 0) {
			free(pixels);
			SDL_snprintf(problem, size, "front panel %d corner is not transparent", i);
			return FAILURE;
		}
		free(pixels);
	}
	return SUCCESS;
}
