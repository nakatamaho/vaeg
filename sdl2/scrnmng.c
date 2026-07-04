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
#include	"scrnmng.h"

typedef struct {
	BOOL			enable;
	int				width;
	int				height;
	SDL_Window		*window;
	SDL_Renderer	*renderer;
	SDL_Texture		*texture;
	int				scale;
	BOOL			aspect;
	int				menu_height;
} SCRNMNG;

typedef struct {
	int		width;
	int		height;
} SCRNSTAT;

static const char app_name[] = "88VA Eternal Grafx";

static	SCRNMNG		scrnmng;
static	SCRNSTAT	scrnstat;
static	SCRNSURF	scrnsurf;

void scrnmng_initialize(void) {

	ZeroMemory(&scrnmng, sizeof(scrnmng));
	scrnmng.scale = 1;
	scrnstat.width = 640;
	scrnstat.height = 400;
}

BOOL scrnmng_create(int width, int height) {

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL video init: %s\n", SDL_GetError());
		return(FAILURE);
	}
	scrnmng.window = SDL_CreateWindow(app_name, SDL_WINDOWPOS_CENTERED,
							SDL_WINDOWPOS_CENTERED, width, height, 0);
	if (scrnmng.window == NULL) {
		fprintf(stderr, "Error: SDL_CreateWindow: %s\n", SDL_GetError());
		return(FAILURE);
	}
	scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1,
							SDL_RENDERER_SOFTWARE);
	if (scrnmng.renderer == NULL) {
		fprintf(stderr, "Error: SDL_CreateRenderer: %s\n", SDL_GetError());
		return(FAILURE);
	}
	SDL_RenderSetLogicalSize(scrnmng.renderer, 0, 0);
	scrnmng.texture = SDL_CreateTexture(scrnmng.renderer,
							SDL_PIXELFORMAT_RGB565,
							SDL_TEXTUREACCESS_STREAMING, width, height);
	if (scrnmng.texture == NULL) {
		fprintf(stderr, "Error: SDL_CreateTexture: %s\n", SDL_GetError());
		return(FAILURE);
	}
	SDL_SetTextureScaleMode(scrnmng.texture, SDL_ScaleModeNearest);
	scrnmng.enable = TRUE;
	scrnmng.width = width;
	scrnmng.height = height;
	scrnmng_set_display(scrnmng.scale, scrnmng.aspect);
	return(SUCCESS);
}

void scrnmng_destroy(void) {

	scrnmng.enable = FALSE;
	if (scrnmng.texture) {
		SDL_DestroyTexture(scrnmng.texture);
		scrnmng.texture = NULL;
	}
	if (scrnmng.renderer) {
		SDL_DestroyRenderer(scrnmng.renderer);
		scrnmng.renderer = NULL;
	}
	if (scrnmng.window) {
		SDL_DestroyWindow(scrnmng.window);
		scrnmng.window = NULL;
	}
	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
}

void *scrnmng_get_window(void) {

	return(scrnmng.window);
}

void *scrnmng_get_renderer(void) {

	return(scrnmng.renderer);
}

static void scrnmng_update_window_size(void) {

	if (scrnmng.window) {
		SDL_SetWindowSize(scrnmng.window,
							scrnmng.width * scrnmng.scale,
							scrnmng.menu_height +
								(scrnmng.height * scrnmng.scale));
	}
}

void scrnmng_set_menu_height(int height) {

	if (height < 0) {
		height = 0;
	}
	if (scrnmng.menu_height == height) {
		return;
	}
	scrnmng.menu_height = height;
	scrnmng_update_window_size();
}

void scrnmng_set_display(int scale, BOOL aspect) {

	if (scale < 1) {
		scale = 1;
	}
	else if (scale > 3) {
		scale = 3;
	}
	scrnmng.scale = scale;
	scrnmng.aspect = aspect ? TRUE : FALSE;
	scrnmng_update_window_size();
}

int scrnmng_get_display_scale(void) {

	return(scrnmng.scale);
}

BOOL scrnmng_get_display_aspect(void) {

	return(scrnmng.aspect);
}

RGB16 scrnmng_makepal16(RGB32 pal32) {

	RGB16	ret;

	ret = (pal32.p.r & 0xf8) << 8;
#if defined(SIZE_QVGA)
	ret += (pal32.p.g & 0xfc) << (3 + 16);
#else
	ret += (pal32.p.g & 0xfc) << 3;
#endif
	ret += pal32.p.b >> 3;
	return(ret);
}

void scrnmng_setwidth(int posx, int width) {

	scrnstat.width = width;
	(void)posx;
}

void scrnmng_setheight(int posy, int height) {

	scrnstat.height = height;
	(void)posy;
}

const SCRNSURF *scrnmng_surflock(void) {

	void	*pixels;
	int		pitch;

	if ((!scrnmng.enable) || (scrnmng.texture == NULL)) {
		return(NULL);
	}
	if (SDL_LockTexture(scrnmng.texture, NULL, &pixels, &pitch) != 0) {
		return(NULL);
	}
	scrnsurf.ptr = (BYTE *)pixels;
	scrnsurf.xalign = 2;
	scrnsurf.yalign = pitch;
	scrnsurf.bpp = 16;
	scrnsurf.width = min(scrnstat.width, 640);
	scrnsurf.height = min(scrnstat.height, 400);
	scrnsurf.extend = 0;
	return(&scrnsurf);
}

void scrnmng_surfunlock(const SCRNSURF *surf) {

	if ((surf == NULL) || (scrnmng.texture == NULL)) {
		return;
	}
	SDL_UnlockTexture(scrnmng.texture);
	scrnmng_present_begin();
	scrnmng_present_end();
}

void scrnmng_present_begin(void) {

	if ((!scrnmng.enable) || (scrnmng.renderer == NULL) ||
		(scrnmng.texture == NULL)) {
		return;
	}
	SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, 255);
	SDL_RenderClear(scrnmng.renderer);
	SDL_Rect dst;
	dst.x = 0;
	dst.y = scrnmng.menu_height;
	dst.w = scrnmng.width * scrnmng.scale;
	dst.h = scrnmng.height * scrnmng.scale;
	SDL_RenderCopy(scrnmng.renderer, scrnmng.texture, NULL, &dst);
}

void scrnmng_present_end(void) {

	if ((!scrnmng.enable) || (scrnmng.renderer == NULL)) {
		return;
	}
	SDL_RenderPresent(scrnmng.renderer);
}

BOOL scrnmng_entermenu(SCRNMENU *smenu) {

	if (smenu) {
		smenu->width = scrnmng.width;
		smenu->height = scrnmng.height;
		smenu->bpp = 16;
	}
	return(FAILURE);
}

void scrnmng_leavemenu(void) {
}

void scrnmng_menudraw(const RECT_T *rct) {

	(void)rct;
}
