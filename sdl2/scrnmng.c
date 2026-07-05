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
	BYTE			*shadow;
	int				shadow_pitch;
	BOOL			visible;
	int				scale;
	BOOL			aspect;
	int				menu_height;
	BOOL			dirty;
} SCRNMNG;

typedef struct {
	int		width;
	int		height;
} SCRNSTAT;

static const char app_name[] = "88VA Eternal Grafx";
enum {
	SCRNMNG_CANVAS_WIDTH	= 640,
	SCRNMNG_CANVAS_HEIGHT	= 400
};

static	SCRNMNG		scrnmng;
static	SCRNSTAT	scrnstat;
static	SCRNSURF	scrnsurf;

static BOOL scrnmng_upload_shadow(void) {

	if ((scrnmng.texture == NULL) || (scrnmng.shadow == NULL)) {
		return(FAILURE);
	}
	if (SDL_UpdateTexture(scrnmng.texture, NULL, scrnmng.shadow,
						  scrnmng.shadow_pitch) != 0) {
		fprintf(stderr, "Error: SDL_UpdateTexture: %s\n", SDL_GetError());
		return(FAILURE);
	}
	scrnmng.dirty = TRUE;
	return(SUCCESS);
}

static void scrnmng_clear_shadow(void) {

	if (scrnmng.shadow == NULL) {
		return;
	}
	ZeroMemory(scrnmng.shadow, scrnmng.shadow_pitch * scrnmng.height);
	(void)scrnmng_upload_shadow();
}

static void scrnmng_get_guest_rect(SDL_Rect *dst) {

	dst->x = 0;
	dst->y = scrnmng.menu_height;
	dst->w = scrnmng.width * scrnmng.scale;
	dst->h = scrnmng.height * scrnmng.scale;
}

void scrnmng_log_geometry(const char *reason) {

	int		window_w;
	int		window_h;
	int		renderer_w;
	int		renderer_h;
	SDL_Rect	dst;

	if ((!scrnmng.window) || (!scrnmng.renderer)) {
		return;
	}
	if (reason == NULL) {
		reason = "unknown";
	}
	window_w = 0;
	window_h = 0;
	renderer_w = 0;
	renderer_h = 0;
	SDL_GetWindowSize(scrnmng.window, &window_w, &window_h);
	SDL_GetRendererOutputSize(scrnmng.renderer, &renderer_w, &renderer_h);
	scrnmng_get_guest_rect(&dst);
	fprintf(stderr,
			"SDL2 geometry [%s]: window=%dx%d renderer=%dx%d "
			"guest=%d,%d %dx%d scale=%d menu=%d\n",
			reason, window_w, window_h, renderer_w, renderer_h,
			dst.x, dst.y, dst.w, dst.h,
			scrnmng.scale, scrnmng.menu_height);
}

BOOL scrnmng_texture_uniform(BOOL *uniform) {

	const BYTE	*base;
	BYTE		first0;
	BYTE		first1;
	int			x;
	int			y;

	if (uniform == NULL) {
		return(FAILURE);
	}
	*uniform = TRUE;
	if ((!scrnmng.enable) || (scrnmng.shadow == NULL)) {
		return(FAILURE);
	}
	base = scrnmng.shadow;
	first0 = base[0];
	first1 = base[1];
	for (y=0; y<scrnmng.height; y++) {
		const BYTE	*row;

		row = base + (y * scrnmng.shadow_pitch);
		for (x=0; x<scrnmng.width; x++) {
			const BYTE	*pixel;

			pixel = row + (x * 2);
			if ((pixel[0] != first0) || (pixel[1] != first1)) {
				*uniform = FALSE;
				return(SUCCESS);
			}
		}
	}
	return(SUCCESS);
}

void scrnmng_initialize(void) {

	ZeroMemory(&scrnmng, sizeof(scrnmng));
	scrnmng.scale = 1;
	scrnstat.width = 640;
	scrnstat.height = 400;
}

BOOL scrnmng_create(int width, int height) {

	(void)width;
	(void)height;
	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL video init: %s\n", SDL_GetError());
		return(FAILURE);
	}
	scrnmng.window = SDL_CreateWindow(app_name, SDL_WINDOWPOS_CENTERED,
							SDL_WINDOWPOS_CENTERED,
							SCRNMNG_CANVAS_WIDTH,
							SCRNMNG_CANVAS_HEIGHT,
							SDL_WINDOW_HIDDEN);
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
							SDL_TEXTUREACCESS_STATIC,
							SCRNMNG_CANVAS_WIDTH,
							SCRNMNG_CANVAS_HEIGHT);
	if (scrnmng.texture == NULL) {
		fprintf(stderr, "Error: SDL_CreateTexture: %s\n", SDL_GetError());
		return(FAILURE);
	}
	SDL_SetTextureScaleMode(scrnmng.texture, SDL_ScaleModeNearest);
	scrnmng.width = SCRNMNG_CANVAS_WIDTH;
	scrnmng.height = SCRNMNG_CANVAS_HEIGHT;
	scrnmng.shadow_pitch = (SCRNMNG_CANVAS_WIDTH + 1) * 2;
	scrnmng.shadow = (BYTE *)calloc(SCRNMNG_CANVAS_HEIGHT,
									scrnmng.shadow_pitch);
	if (scrnmng.shadow == NULL) {
		fprintf(stderr, "Error: shadow framebuffer allocation failed\n");
		scrnmng_destroy();
		return(FAILURE);
	}
	scrnmng_clear_shadow();
	scrnmng.enable = TRUE;
	scrnmng_set_display(scrnmng.scale, scrnmng.aspect);
	return(SUCCESS);
}

void scrnmng_show(void) {

	if ((scrnmng.window) && (!scrnmng.visible)) {
		SDL_ShowWindow(scrnmng.window);
		scrnmng.visible = TRUE;
		scrnmng_log_geometry("startup");
	}
}

void scrnmng_destroy(void) {

	scrnmng.enable = FALSE;
	scrnmng.visible = FALSE;
	if (scrnmng.texture) {
		SDL_DestroyTexture(scrnmng.texture);
		scrnmng.texture = NULL;
	}
	if (scrnmng.shadow) {
		free(scrnmng.shadow);
		scrnmng.shadow = NULL;
	}
	scrnmng.shadow_pitch = 0;
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

static BOOL scrnmng_update_window_size(void) {

	if (scrnmng.window) {
		int		current_w;
		int		current_h;
		int		target_w;
		int		target_h;

		current_w = 0;
		current_h = 0;
		target_w = scrnmng.width * scrnmng.scale;
		target_h = scrnmng.menu_height +
					(scrnmng.height * scrnmng.scale);
		SDL_GetWindowSize(scrnmng.window, &current_w, &current_h);
		if ((current_w == target_w) && (current_h == target_h)) {
			return(FALSE);
		}
		SDL_SetWindowSize(scrnmng.window, target_w, target_h);
		return(TRUE);
	}
	return(FALSE);
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
	if (scrnmng.visible) {
		scrnmng_log_geometry("menu-height");
	}
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
	if (scrnmng.visible) {
		scrnmng_log_geometry("scale-change");
	}
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

	if (width < 1) {
		width = 1;
	}
	if (scrnstat.width == width) {
		return;
	}
	scrnstat.width = width;
	scrnmng_clear_shadow();
	if (scrnmng.visible) {
		scrnmng_log_geometry("mode-width");
	}
	(void)posx;
}

void scrnmng_setheight(int posy, int height) {

	if (height < 1) {
		height = 1;
	}
	if (scrnstat.height == height) {
		return;
	}
	scrnstat.height = height;
	scrnmng_clear_shadow();
	if (scrnmng.visible) {
		scrnmng_log_geometry("mode-height");
	}
	(void)posy;
}

const SCRNSURF *scrnmng_surflock(void) {

	if ((!scrnmng.enable) || (scrnmng.shadow == NULL)) {
		return(NULL);
	}
	scrnsurf.ptr = scrnmng.shadow;
	scrnsurf.xalign = 2;
	scrnsurf.yalign = scrnmng.shadow_pitch;
	scrnsurf.bpp = 16;
	scrnsurf.width = min(scrnstat.width, scrnmng.width);
	scrnsurf.height = min(scrnstat.height, scrnmng.height);
	scrnsurf.extend = 0;
	return(&scrnsurf);
}

void scrnmng_surfunlock(const SCRNSURF *surf) {

	if ((surf == NULL) || (scrnmng.texture == NULL)) {
		return;
	}
	(void)scrnmng_upload_shadow();
}

void scrnmng_present_begin(void) {

	if ((!scrnmng.enable) || (scrnmng.renderer == NULL) ||
		(scrnmng.texture == NULL)) {
		return;
	}
	SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, 255);
	SDL_RenderClear(scrnmng.renderer);
	SDL_Rect dst;
	scrnmng_get_guest_rect(&dst);
	SDL_RenderCopy(scrnmng.renderer, scrnmng.texture, NULL, &dst);
	scrnmng.dirty = FALSE;
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
