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
#include	"np2.h"
#include	"np2ver.h"
#include	"appicon.h"

typedef struct {
	BOOL			enable;
	int				width;
	int				height;
	SDL_Window		*window;
	SDL_Renderer	*renderer;
	SDL_Texture		*texture;
	char			renderer_backend[64];
	BYTE			*shadow;
	int				shadow_pitch;
	BOOL			visible;
	int				scale;
	BOOL			aspect;
	int				menu_height;
	int				scaling;
	int				effect;
	int				display_mode;
	int				window_x;
	int				window_y;
	int				window_width;
	int				window_height;
	BOOL			window_maximized;
	UINT8			fscrnmod;
	BOOL			dirty;
} SCRNMNG;

typedef struct {
	int		width;
	int		height;
} SCRNSTAT;

static const char app_name[] = "88VA Eternal Grafx " VAEGREL_CORE;
enum {
	SCRNMNG_CANVAS_WIDTH	= 640,
	SCRNMNG_CANVAS_HEIGHT	= 400
};

static	SCRNMNG		scrnmng;
static	SCRNSTAT	scrnstat;
static	SCRNSURF	scrnsurf;

static void scrnmng_log_renderer(void) {

	SDL_RendererInfo	info;
	const char			*name;
	const char			*kind;

	if ((scrnmng.renderer == NULL) ||
		(SDL_GetRendererInfo(scrnmng.renderer, &info) != 0)) {
		SDL_Log("SDL renderer backend: unknown");
		return;
	}
	name = (info.name) ? info.name : "unknown";
	snprintf(scrnmng.renderer_backend, sizeof(scrnmng.renderer_backend),
			 "%s", name);
	kind = (info.flags & SDL_RENDERER_ACCELERATED) ?
					"accelerated" : "software";
	SDL_Log("SDL renderer backend: %s (%s)", name, kind);
}

static BOOL scrnmng_upload_shadow(void) {

	if ((scrnmng.texture == NULL) || (scrnmng.shadow == NULL)) {
		return(FAILURE);
	}
	if (SDL_UpdateTexture(scrnmng.texture, NULL,
						  scrnmng.shadow +
							(SCRNMNG_SURFACE_GUARD_LEFT * 2),
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

static BOOL scrnmng_calculate_viewport(VAEG_VIEWPORT *viewport) {

	VAEG_VIEWPORT_INPUT input;
	int window_width;
	int window_height;
	int output_width;
	int output_height;

	if ((scrnmng.window == NULL) || (scrnmng.renderer == NULL)) {
		return(FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	if ((SDL_GetRendererOutputSize(scrnmng.renderer,
						&output_width, &output_height) != 0) ||
		(window_width <= 0) || (window_height <= 0)) {
		return(FAILURE);
	}
	input.guest_width = scrnmng.width;
	input.guest_height = scrnmng.height;
	input.drawable_width = output_width;
	input.drawable_height = output_height;
	input.menu_inset = (int)(((SINT64)scrnmng.menu_height * output_height +
									(window_height / 2)) / window_height);
	if (scrnmng.display_mode == VAEG_DISPLAY_WINDOWED) {
		input.scaling = scrnmng.scaling;
	}
	else {
		switch(scrnmng.fscrnmod & 3) {
			case 0:
				input.scaling = VAEG_SCALING_NATIVE;
				break;
			case 1:
				input.scaling = VAEG_SCALING_FIT_8DOT;
				break;
			case 3:
				input.scaling = VAEG_SCALING_STRETCH;
				break;
			default:
				input.scaling = VAEG_SCALING_FIT;
				break;
		}
	}
	input.aspect = scrnmng.aspect;
	return(vaeg_viewport_calculate(&input, viewport));
}

void scrnmng_log_geometry(const char *reason) {

	int		window_w;
	int		window_h;
	int		renderer_w;
	int		renderer_h;
	VAEG_VIEWPORT viewport;

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
	ZeroMemory(&viewport, sizeof(viewport));
	(void)scrnmng_calculate_viewport(&viewport);
	fprintf(stderr,
			"SDL2 geometry [%s]: window=%dx%d renderer=%dx%d "
			"guest=%d,%d %dx%d scale=%d menu=%d mode=%d effect=%d\n",
			reason, window_w, window_h, renderer_w, renderer_h,
			viewport.x, viewport.y, viewport.width, viewport.height,
			scrnmng.scale, scrnmng.menu_height, scrnmng.scaling,
			scrnmng.effect);
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
	base = scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2);
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
	scrnmng.scaling = VAEG_SCALING_FIT;
	scrnmng.effect = VAEG_EFFECT_UNFILTERED;
	scrnmng.display_mode = VAEG_DISPLAY_WINDOWED;
	scrnmng.fscrnmod = 2;
	scrnstat.width = 640;
	scrnstat.height = 400;
}

BOOL scrnmng_create(int width, int height) {

	width = max(320, width);
	height = max(240, height);
	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL video init: %s\n", SDL_GetError());
		return(FAILURE);
	}
	scrnmng.window = SDL_CreateWindow(app_name, SDL_WINDOWPOS_CENTERED,
							SDL_WINDOWPOS_CENTERED,
								width,
								height,
								SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE |
								SDL_WINDOW_ALLOW_HIGHDPI);
	if (scrnmng.window == NULL) {
		fprintf(stderr, "Error: SDL_CreateWindow: %s\n", SDL_GetError());
		return(FAILURE);
	}
	SDL_EventState(SDL_DROPBEGIN, SDL_ENABLE);
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	SDL_EventState(SDL_DROPCOMPLETE, SDL_ENABLE);
	appicon_set_window(scrnmng.window);
	SDL_SetWindowMinimumSize(scrnmng.window, 320, 240);
	SDL_GetWindowPosition(scrnmng.window, &scrnmng.window_x,
									&scrnmng.window_y);
	scrnmng.window_width = width;
	scrnmng.window_height = height;
	scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1,
							SDL_RENDERER_ACCELERATED);
	if (scrnmng.renderer == NULL) {
		scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1,
							SDL_RENDERER_SOFTWARE);
	}
	if (scrnmng.renderer == NULL) {
		fprintf(stderr, "Error: SDL_CreateRenderer: %s\n", SDL_GetError());
		return(FAILURE);
	}
	scrnmng_log_renderer();
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
	SDL_SetTextureScaleMode(scrnmng.texture,
				(scrnmng.effect == VAEG_EFFECT_UNFILTERED) ?
							SDL_ScaleModeNearest : SDL_ScaleModeLinear);
	scrnmng.width = SCRNMNG_CANVAS_WIDTH;
	scrnmng.height = SCRNMNG_CANVAS_HEIGHT;
	scrnmng.shadow_pitch =
		(SCRNMNG_CANVAS_WIDTH + SCRNMNG_SURFACE_GUARD_LEFT) * 2;
	scrnmng.shadow = (BYTE *)calloc(SCRNMNG_CANVAS_HEIGHT,
									scrnmng.shadow_pitch);
	if (scrnmng.shadow == NULL) {
		fprintf(stderr, "Error: shadow framebuffer allocation failed\n");
		scrnmng_destroy();
		return(FAILURE);
	}
	scrnmng_clear_shadow();
	scrnmng.enable = TRUE;
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

const char *scrnmng_get_renderer_backend(void) {

	if (scrnmng.renderer_backend[0] == '\0') {
		return("unknown");
	}
	return(scrnmng.renderer_backend);
}

static BOOL scrnmng_update_window_size(void) {

	if (scrnmng.window &&
		(scrnmng.display_mode == VAEG_DISPLAY_WINDOWED)) {
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

void scrnmng_set_scaling(int scaling) {

	if ((scaling < 0) || (scaling >= VAEG_SCALING_COUNT)) {
		scaling = VAEG_SCALING_FIT;
	}
	scrnmng.scaling = scaling;
	if (scrnmng.visible) {
		scrnmng_log_geometry("scaling-change");
	}
}

int scrnmng_get_scaling(void) {

	return(scrnmng.scaling);
}

void scrnmng_set_effect(int effect) {

	if ((effect < 0) || (effect >= VAEG_EFFECT_COUNT)) {
		effect = VAEG_EFFECT_UNFILTERED;
	}
	scrnmng.effect = effect;
	if (scrnmng.texture != NULL) {
		SDL_SetTextureScaleMode(scrnmng.texture,
				(effect == VAEG_EFFECT_UNFILTERED) ?
							SDL_ScaleModeNearest : SDL_ScaleModeLinear);
	}
}

int scrnmng_get_effect(void) {

	return(scrnmng.effect);
}

BOOL scrnmng_get_viewport(VAEG_VIEWPORT *viewport) {

	if (viewport == NULL) {
		return(FAILURE);
	}
	return(scrnmng_calculate_viewport(viewport));
}

BOOL scrnmng_map_window_point(int window_x, int window_y,
								int *guest_x, int *guest_y) {

	VAEG_VIEWPORT viewport;
	int window_width;
	int window_height;
	int output_width;
	int output_height;
	int drawable_x;
	int drawable_y;

	if (scrnmng_calculate_viewport(&viewport) != SUCCESS) {
		return(FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	SDL_GetRendererOutputSize(scrnmng.renderer, &output_width, &output_height);
	if ((window_width <= 0) || (window_height <= 0)) {
		return(FAILURE);
	}
	drawable_x = (int)(((SINT64)window_x * output_width) / window_width);
	drawable_y = (int)(((SINT64)window_y * output_height) / window_height);
	return(vaeg_viewport_map_point(&viewport, scrnmng.width, scrnmng.height,
							drawable_x, drawable_y, guest_x, guest_y));
}

static void scrnmng_remember_window(void) {

	if ((scrnmng.window == NULL) ||
		(scrnmng.display_mode != VAEG_DISPLAY_WINDOWED)) {
		return;
	}
	SDL_GetWindowPosition(scrnmng.window, &scrnmng.window_x,
									&scrnmng.window_y);
	SDL_GetWindowSize(scrnmng.window, &scrnmng.window_width,
									&scrnmng.window_height);
	scrnmng.window_maximized =
		(SDL_GetWindowFlags(scrnmng.window) & SDL_WINDOW_MAXIMIZED) ?
											TRUE : FALSE;
}

static void scrnmng_restore_window(void) {

	SDL_RestoreWindow(scrnmng.window);
	SDL_SetWindowPosition(scrnmng.window, scrnmng.window_x,
									scrnmng.window_y);
	SDL_SetWindowSize(scrnmng.window, max(320, scrnmng.window_width),
								max(240, scrnmng.window_height));
	if (scrnmng.window_maximized) {
		SDL_MaximizeWindow(scrnmng.window);
	}
}

static BOOL scrnmng_find_display_mode(int monitor, int width, int height,
								int refresh, SDL_DisplayMode *result) {

	int count;
	int index;

	count = SDL_GetNumDisplayModes(monitor);
	for (index=0; index<count; index++) {
		SDL_DisplayMode mode;

		if ((SDL_GetDisplayMode(monitor, index, &mode) == 0) &&
			(mode.w == width) && (mode.h == height) &&
			((refresh == 0) || (mode.refresh_rate == refresh))) {
			*result = mode;
			return(SUCCESS);
		}
	}
	SDL_SetError("Requested display mode is not available");
	return(FAILURE);
}

BOOL scrnmng_set_display_mode(int mode, int monitor, UINT width, UINT height,
								UINT refresh, UINT8 fscrnmod) {

	SDL_Rect bounds;
	SDL_DisplayMode desktop;
	SDL_DisplayMode selected;
	int target_width;
	int target_height;
	int display_count;

	if ((scrnmng.window == NULL) || (mode < 0) ||
		(mode >= VAEG_DISPLAY_MODE_COUNT)) {
		return(FAILURE);
	}
	display_count = SDL_GetNumVideoDisplays();
	if (display_count <= 0) {
		return(mode == VAEG_DISPLAY_WINDOWED ? SUCCESS : FAILURE);
	}
	if ((monitor < 0) || (monitor >= display_count)) {
		monitor = 0;
	}
	if ((SDL_GetDisplayBounds(monitor, &bounds) != 0) ||
		(SDL_GetDesktopDisplayMode(monitor, &desktop) != 0)) {
		return(FAILURE);
	}
	if (scrnmng.display_mode == VAEG_DISPLAY_WINDOWED) {
		scrnmng_remember_window();
	}
	if (SDL_SetWindowFullscreen(scrnmng.window, 0) != 0) {
		return(FAILURE);
	}
	scrnmng.fscrnmod = vaeg_fscrnmod_sanitize(fscrnmod, NULL);
	if (mode == VAEG_DISPLAY_WINDOWED) {
		scrnmng.display_mode = mode;
		scrnmng_restore_window();
		scrnmng_log_geometry("windowed");
		return(SUCCESS);
	}
	SDL_RestoreWindow(scrnmng.window);
	SDL_SetWindowPosition(scrnmng.window, bounds.x, bounds.y);
	if (mode == VAEG_DISPLAY_BORDERLESS) {
		if (SDL_SetWindowFullscreen(scrnmng.window,
								SDL_WINDOW_FULLSCREEN_DESKTOP) == 0) {
			scrnmng.display_mode = mode;
			scrnmng_log_geometry("borderless");
			return(SUCCESS);
		}
	}
	else {
		vaeg_fullscreen_size(width, height, scrnmng.fscrnmod,
					desktop.w, desktop.h, &target_width, &target_height);
		if ((scrnmng_find_display_mode(monitor, target_width, target_height,
									refresh, &selected) == SUCCESS) &&
			(SDL_SetWindowDisplayMode(scrnmng.window, &selected) == 0) &&
			(SDL_SetWindowFullscreen(scrnmng.window,
								SDL_WINDOW_FULLSCREEN) == 0)) {
			scrnmng.display_mode = mode;
			scrnmng_log_geometry("exclusive");
			return(SUCCESS);
		}
	}
	(void)SDL_SetWindowFullscreen(scrnmng.window, 0);
	scrnmng.display_mode = VAEG_DISPLAY_WINDOWED;
	scrnmng_restore_window();
	return(FAILURE);
}

int scrnmng_get_display_mode(void) {

	return(scrnmng.display_mode);
}

BOOL scrnmng_isfullscreen(void) {

	return(scrnmng.display_mode != VAEG_DISPLAY_WINDOWED);
}

BOOL scrnmng_capture_window_size(int *width, int *height) {

	if ((scrnmng.window == NULL) ||
		(scrnmng.display_mode != VAEG_DISPLAY_WINDOWED) ||
		(width == NULL) || (height == NULL)) {
		return(FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, width, height);
	return(SUCCESS);
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

	VAEG_VIEWPORT viewport;
	SDL_Rect dst;
	int row;
	int x;

	if ((!scrnmng.enable) || (scrnmng.renderer == NULL) ||
		(scrnmng.texture == NULL)) {
		return;
	}
	SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, 255);
	SDL_RenderClear(scrnmng.renderer);
	if (scrnmng_calculate_viewport(&viewport) != SUCCESS) {
		return;
	}
	dst.x = viewport.x;
	dst.y = viewport.y;
	dst.w = viewport.width;
	dst.h = viewport.height;
	SDL_RenderCopy(scrnmng.renderer, scrnmng.texture, NULL, &dst);
	SDL_RenderSetClipRect(scrnmng.renderer, &dst);
	if (((scrnmng.effect == VAEG_EFFECT_SCANLINE) ||
		 (scrnmng.effect == VAEG_EFFECT_CRT_LITE)) &&
		(dst.h >= scrnmng.height)) {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0,
				(scrnmng.effect == VAEG_EFFECT_CRT_LITE) ? 42 : 34);
		for (row=1; row<scrnmng.height; row+=2) {
			int y0;
			int y1;
			SDL_Rect line;

			y0 = dst.y + (row * dst.h) / scrnmng.height;
			y1 = dst.y + ((row + 1) * dst.h) / scrnmng.height;
			if (y1 <= y0) {
				y1 = y0 + 1;
			}
			line.x = dst.x;
			line.y = y0;
			line.w = dst.w;
			line.h = y1 - y0;
			SDL_RenderFillRect(scrnmng.renderer, &line);
		}
	}
	if (scrnmng.effect == VAEG_EFFECT_CRT_LITE) {
		if (dst.w >= (scrnmng.width * 2)) {
			SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_ADD);
			for (x=dst.x; x<(dst.x + dst.w); x+=3) {
				SDL_SetRenderDrawColor(scrnmng.renderer, 24, 2, 2, 10);
				SDL_RenderDrawLine(scrnmng.renderer, x, dst.y,
											x, dst.y + dst.h - 1);
				if ((x + 1) < (dst.x + dst.w)) {
					SDL_SetRenderDrawColor(scrnmng.renderer, 2, 24, 2, 10);
					SDL_RenderDrawLine(scrnmng.renderer, x + 1, dst.y,
											x + 1, dst.y + dst.h - 1);
				}
				if ((x + 2) < (dst.x + dst.w)) {
					SDL_SetRenderDrawColor(scrnmng.renderer, 2, 2, 24, 10);
					SDL_RenderDrawLine(scrnmng.renderer, x + 2, dst.y,
											x + 2, dst.y + dst.h - 1);
				}
			}
		}
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		for (x=0; x<8; x++) {
			SDL_Rect left = {dst.x + x, dst.y, 1, dst.h};
			SDL_Rect right = {dst.x + dst.w - 1 - x, dst.y, 1, dst.h};
			SDL_Rect top = {dst.x, dst.y + x, dst.w, 1};
			SDL_Rect bottom = {dst.x, dst.y + dst.h - 1 - x, dst.w, 1};
			SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0,
										(UINT8)(38 - x * 4));
			SDL_RenderFillRect(scrnmng.renderer, &left);
			SDL_RenderFillRect(scrnmng.renderer, &right);
			SDL_RenderFillRect(scrnmng.renderer, &top);
			SDL_RenderFillRect(scrnmng.renderer, &bottom);
		}
	}
	SDL_RenderSetClipRect(scrnmng.renderer, NULL);
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
