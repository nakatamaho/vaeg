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
#include <string.h>
#include "compiler.h"
#include "sdlapi.h"
#include "scrnmng.h"
#include "np2.h"
#include "np2ver.h"
#include "machine/pccore.h"
#include "sgp.h"
#include "videova.h"
#include "tsp.h"
#include "fontdata.h"
#include "diskdrv.h"
#include "fddfile.h"
#include "appicon.h"
#include "framedisp.h"

typedef struct {
	BOOL enable;
	int width;
	int height;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	char renderer_backend[64];
	BYTE *shadow;
	int shadow_pitch;
	BOOL visible;
	int scale;
	BOOL aspect;
	int menu_height;
	int scaling;
	int effect;
	int display_mode;
	int window_x;
	int window_y;
	int window_width;
	int window_height;
	BOOL window_maximized;
	UINT8 fscrnmod;
	BOOL dirty;
	BOOL framedisp_enabled;
	VAEG_FRAMEDISP framedisp;
	BOOL rendered_capture_enabled;
	SDL_Surface *rendered_frame;
} SCRNMNG;

typedef struct {
	int width;
	int height;
} SCRNSTAT;

typedef struct {
	BOOL valid;
	UINT32 tick;
	UINT32 frames;
	UINT32 fps_tenths;
} VAEG_SPEEDMETER;

static const char app_name[] = "88VA Eternal Grafx " VAEGREL_CORE;
static const UINT32 vaeg_nominal_frame_rate = 60;
enum {
	SCRNMNG_CANVAS_WIDTH = 640,
	SCRNMNG_CANVAS_HEIGHT = 400
};

static SCRNMNG scrnmng;
static SCRNSTAT scrnstat;
static SCRNSURF scrnsurf;
static VAEG_SPEEDMETER speedmeter;

static BOOL scrnmng_calculate_viewport(VAEG_VIEWPORT *viewport);
static void scrnmng_draw_video_info_overlay(const VAEG_VIEWPORT *viewport);
static void scrnmng_draw_framebuffer_info_overlay(const VAEG_VIEWPORT *viewport);

static void scrnmng_capture_dummy_frame(void) {
	VAEG_VIEWPORT viewport;
	SDL_Surface *source;
	SDL_Rect dst;

	if ((scrnmng.rendered_frame == NULL) || (scrnmng.shadow == NULL)) {
		return;
	}
	source = SDL_CreateRGBSurfaceWithFormatFrom(scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                                            scrnmng.width, scrnmng.height, 16,
	                                            scrnmng.shadow_pitch, SDL_PIXELFORMAT_RGB565);
	if (source == NULL) {
		fprintf(stderr, "scsitrace rendered-screen-source-failed error=%s\n", SDL_GetError());
		return;
	}
	(void)SDL_FillRect(scrnmng.rendered_frame, NULL,
	                   SDL_MapRGB(scrnmng.rendered_frame->format, 0, 0, 0));
	if (scrnmng_calculate_viewport(&viewport) == SUCCESS) {
		dst.x = viewport.x;
		dst.y = viewport.y;
		dst.w = viewport.width;
		dst.h = viewport.height;
		(void)SDL_BlitScaled(source, NULL, scrnmng.rendered_frame, &dst);
	}
	SDL_FreeSurface(source);
}

static void scrnmng_capture_rendered_frame(void) {
	int width;
	int height;

	if (scrnmng.renderer == NULL) {
		return;
	}
	if (SDL_GetRendererOutputSize(scrnmng.renderer, &width, &height) != 0 || (width <= 0) ||
	    (height <= 0)) {
		fprintf(stderr, "scsitrace rendered-screen-size-failed error=%s\n", SDL_GetError());
		return;
	}
	if ((scrnmng.rendered_frame == NULL) || (scrnmng.rendered_frame->w != width) ||
	    (scrnmng.rendered_frame->h != height)) {
		if (scrnmng.rendered_frame != NULL) {
			SDL_FreeSurface(scrnmng.rendered_frame);
			scrnmng.rendered_frame = NULL;
		}
		scrnmng.rendered_frame =
		    SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
		if (scrnmng.rendered_frame == NULL) {
			fprintf(stderr, "scsitrace rendered-screen-surface-failed error=%s\n", SDL_GetError());
			return;
		}
	}
	if ((SDL_GetCurrentVideoDriver() != NULL) && !strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
		scrnmng_capture_dummy_frame();
		return;
	}
	if (SDL_RenderReadPixels(scrnmng.renderer, NULL, scrnmng.rendered_frame->format->format,
	                         scrnmng.rendered_frame->pixels, scrnmng.rendered_frame->pitch) != 0) {
		fprintf(stderr, "scsitrace rendered-screen-read-failed error=%s\n", SDL_GetError());
	}
}

static UINT32 scrnmng_measured_clock(UINT32 configured_clock) {
	UINT64 measured;

	if (!speedmeter.valid || (speedmeter.fps_tenths == 0)) {
		return configured_clock;
	}
	measured = (UINT64)configured_clock * speedmeter.fps_tenths;
	measured /= (UINT64)vaeg_nominal_frame_rate * 10;
	if (measured > 0xffffffffULL) {
		return 0xffffffffU;
	}
	return (UINT32)measured;
}

static void scrnmng_update_speedmeter(UINT32 tick, UINT32 frames) {
	UINT32 elapsed;
	UINT32 frame_delta;
	UINT64 fps_tenths;

	if (!speedmeter.valid || (frames < speedmeter.frames)) {
		speedmeter.valid = TRUE;
		speedmeter.tick = tick;
		speedmeter.frames = frames;
		speedmeter.fps_tenths = 0;
		return;
	}
	elapsed = tick - speedmeter.tick;
	if (elapsed < 1000) {
		return;
	}
	frame_delta = frames - speedmeter.frames;
	fps_tenths = ((UINT64)frame_delta * 10000) / elapsed;
	if (fps_tenths > 0xffffffffULL) {
		fps_tenths = 0xffffffffULL;
	}
	speedmeter.tick = tick;
	speedmeter.frames = frames;
	speedmeter.fps_tenths = (UINT32)fps_tenths;
}

static const char *scrnmng_fdd_title_path(int drive) {
	const char *path;

	path = fdd_diskname((REG8)drive);
	if ((path == NULL) || (path[0] == '\0')) {
		path = diskdrv_fname[drive];
	}
	return ((path != NULL) ? path : "");
}

static const char *scrnmng_fdd_display_name(const char *path) {
	const char *name;
	const char *separator;

	name = path;
	separator = strrchr(name, '/');
	if (separator != NULL) {
		name = separator + 1;
	}
	separator = strrchr(name, '\\');
	if (separator != NULL) {
		name = separator + 1;
	}
	return name;
}

static void scrnmng_append_fdd_title(char *title, size_t title_size, int *length, int drive) {
	const char *path;
	const char *name;

	if ((title == NULL) || (length == NULL) || (*length < 0) ||
	    ((size_t)*length >= title_size)) {
		return;
	}
	path = scrnmng_fdd_title_path(drive);
	name = (path[0] != '\0') ? scrnmng_fdd_display_name(path) : "Empty";
	*length += snprintf(title + *length, title_size - (size_t)*length, " - FDD%d: %s", drive + 1,
	                    name);
}

static void scrnmng_update_title(void) {
	char title[512];
	UINT32 cpu_clock;
	UINT32 sgp_clock;
	int length;

	if (scrnmng.window == NULL) {
		return;
	}
	cpu_clock = scrnmng_measured_clock(pccore_cpu_clock());
	sgp_clock = scrnmng_measured_clock(sgp_effective_clock());
	length = snprintf(title, sizeof(title), "%s", app_name);
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_FDD) != 0) {
		scrnmng_append_fdd_title(title, sizeof(title), &length, 0);
		scrnmng_append_fdd_title(title, sizeof(title), &length, 1);
	}
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_FPS) && (length >= 0) &&
	    ((size_t)length < sizeof(title))) {
		length += snprintf(title + length, sizeof(title) - (size_t)length, " - %u.%1uFPS",
		                   (unsigned int)(scrnmng.framedisp.fps_tenths / 10),
		                   (unsigned int)(scrnmng.framedisp.fps_tenths % 10));
	}
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_CPU_CLOCK) && (length >= 0) &&
	    ((size_t)length < sizeof(title))) {
		length += snprintf(title + length, sizeof(title) - (size_t)length, " - CPU %u.%04uMHz",
		                   (unsigned int)(cpu_clock / 1000000),
		                   (unsigned int)((cpu_clock % 1000000) / 100));
	}
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_SGP_CLOCK) && (length >= 0) &&
	    ((size_t)length < sizeof(title))) {
		length += snprintf(title + length, sizeof(title) - (size_t)length, " - SGP %u.%04uMHz",
		                   (unsigned int)(sgp_clock / 1000000),
		                   (unsigned int)((sgp_clock % 1000000) / 100));
	}
	if (scrnmng.framedisp_enabled && (length >= 0) && ((size_t)length < sizeof(title))) {
		length += snprintf(title + length, sizeof(title) - (size_t)length, " - FRAME %u",
		                   (unsigned int)drawcount);
	}
	SDL_SetWindowTitle(scrnmng.window, title);
}

static int scrnmng_video_bpp(UINT mode) {
	switch (mode & 3) {
	case 0:
		return 1;
	case 1:
		return 4;
	case 2:
		return 8;
	default:
		return 16;
	}
}
static void scrnmng_format_video_line(char *line, size_t line_size, const char *label, int width,
                                      int height, int bpp) {
	(void)snprintf(line, line_size, "%s %dx%d %dbpp", label, width, height, bpp);
}
static BOOL scrnmng_framebuffer_valid(FRAMEBUFFER framebuffer, int framebuffer_no) {
	if ((framebuffer == NULL) || (framebuffer->fbw == 0xffff) || (framebuffer->dsh == 0)) {
		return FALSE;
	}
	/* FB1 has no writable FSA/FBL/OFX/OFY fields in the VA model. */
	if ((framebuffer_no != 1) && (framebuffer->fsa == 0xffffffffL)) {
		return FALSE;
	}
	return TRUE;
}
static int scrnmng_framebuffer_width(FRAMEBUFFER framebuffer, int bpp) {
	if ((framebuffer == NULL) || (bpp <= 0) || (framebuffer->fbw == 0xffff)) {
		return 0;
	}
	return ((int)framebuffer->fbw * 8) / bpp;
}
static int scrnmng_framebuffer_height(FRAMEBUFFER framebuffer) {
	if ((framebuffer == NULL) || (framebuffer->dsh == 0)) {
		return 0;
	}
	return framebuffer->dsh;
}
static void scrnmng_format_graphics_line(char *line, size_t line_size, const char *label,
                                         BOOL active, int logical_width, int logical_height,
                                         int bpp, int framebuffer_no, FRAMEBUFFER framebuffer) {
	int framebuffer_width;
	int framebuffer_height;

	if (!active) {
		(void)snprintf(line, line_size, "%s OFF", label);
		return;
	}
	if (!scrnmng_framebuffer_valid(framebuffer, framebuffer_no)) {
		(void)snprintf(line, line_size, "%s OFF", label);
		return;
	}
	framebuffer_width = scrnmng_framebuffer_width(framebuffer, bpp);
	framebuffer_height = scrnmng_framebuffer_height(framebuffer);
	(void)snprintf(line, line_size, "%s %dx%d FB %dx%d %dbpp", label, logical_width, logical_height,
	               framebuffer_width, framebuffer_height, bpp);
}

static void scrnmng_format_framebuffer_line(char *line, size_t line_size, int framebuffer_no,
                                            int bpp, FRAMEBUFFER framebuffer) {
	int width;
	int height;

	if (!scrnmng_framebuffer_valid(framebuffer, framebuffer_no)) {
		(void)snprintf(line, line_size, "FB%d OFF", framebuffer_no);
		return;
	}
	width = scrnmng_framebuffer_width(framebuffer, bpp);
	height = scrnmng_framebuffer_height(framebuffer);
	(void)snprintf(line, line_size, "FB%d %dx%d %dbpp", framebuffer_no, width, height, bpp);
}

static void scrnmng_draw_text_glyphs(int x, int y, int scale, const char *text, SDL_Color color) {
	const unsigned char *glyph;
	int character;
	int row;
	int bit;

	if ((text == NULL) || (scale <= 0)) {
		return;
	}
	SDL_SetRenderDrawColor(scrnmng.renderer, color.r, color.g, color.b, color.a);
	while (*text != '\0') {
		character = (unsigned char)*text++;
		glyph = fontdata_8 + (character * 8);
		for (row = 0; row < 8; row++) {
			for (bit = 0; bit < 8; bit++) {
				if ((glyph[row] & (0x80 >> bit)) != 0) {
					SDL_Rect pixel = {x + bit * scale, y + row * scale, scale, scale};
					SDL_RenderFillRect(scrnmng.renderer, &pixel);
				}
			}
		}
		x += 8 * scale;
	}
}
static int scrnmng_menu_offset(void) {
	int window_width;
	int window_height;
	int output_width;
	int output_height;

	if ((scrnmng.window == NULL) || (scrnmng.renderer == NULL)) {
		return 0;
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	if ((SDL_GetRendererOutputSize(scrnmng.renderer, &output_width, &output_height) != 0) ||
	    (window_height <= 0) || (output_height <= 0)) {
		return scrnmng.menu_height;
	}
	return (int)(((SINT64)scrnmng.menu_height * output_height + (window_height / 2)) /
	             window_height);
}

static void scrnmng_draw_video_info_overlay(const VAEG_VIEWPORT *viewport) {
	char lines[4][96];
	int graphics_height;
	int text_height;
	int g0_width;
	int g1_width;
	int g0_bpp;
	int g1_bpp;
	BOOL g0_active;
	BOOL g1_active;
	int scale;
	int line_height;
	int width;
	int height;
	int output_width;
	int output_height;
	int i;
	SDL_Rect background;
	SDL_Color text_color;

	if ((viewport == NULL) || (!viewport->valid) ||
	    ((np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO) == 0)) {
		return;
	}
	if ((SDL_GetRendererOutputSize(scrnmng.renderer, &output_width, &output_height) != 0) ||
	    (output_width <= 0) || (output_height <= 0)) {
		return;
	}
	graphics_height = (videova.grmode & 0x0002) ? 200 : 400;
	text_height = (tsp.screenlines != 0) ? tsp.screenlines : graphics_height;
	g0_width = (videova.grres & 0x0010) ? 320 : 640;
	g1_width = (videova.grres & 0x1000) ? 320 : 640;
	g0_bpp = scrnmng_video_bpp(videova.grres);
	g1_bpp = scrnmng_video_bpp(videova.grres >> 8);
	/* G1 is a separate screen only in single-plane, two-screen mode. */
	g0_active = (videova.grmode & 0x8000) != 0;
	g1_active = g0_active && ((videova.grmode & 0x0c00) == 0x0c00);
	scrnmng_format_video_line(lines[0], sizeof(lines[0]), "TEXT", 640, text_height, 4);
	scrnmng_format_video_line(lines[1], sizeof(lines[1]), "SPRITE", 640, text_height, 4);
	scrnmng_format_graphics_line(lines[2], sizeof(lines[2]), "G0", g0_active, g0_width,
	                             graphics_height, g0_bpp, 0, &videova.framebuffer[0]);
	scrnmng_format_graphics_line(lines[3], sizeof(lines[3]), "G1", g1_active, g1_width,
	                             graphics_height, g1_bpp, 1, &videova.framebuffer[1]);

	scale = (int)viewport->scale_x;
	if (scale < 1) {
		scale = 1;
	}
	if (scale > 3) {
		scale = 3;
	}
	line_height = 8 * scale;
	width = 0;
	for (i = 0; i < 4; i++) {
		const int length = (int)strlen(lines[i]);
		if (length > width) {
			width = length;
		}
	}
	width = width * 8 * scale + 8 * scale;
	height = line_height * 4 + 8 * scale;
	if ((width >= output_width) || (height >= output_height)) {
		return;
	}
	background.x = output_width - width;
	background.y = scrnmng_menu_offset() + 2;
	background.w = width;
	background.h = height;
	SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, 190);
	SDL_RenderFillRect(scrnmng.renderer, &background);
	text_color.r = 255;
	text_color.g = 255;
	text_color.b = 192;
	text_color.a = 255;
	for (i = 0; i < 4; i++) {
		scrnmng_draw_text_glyphs(background.x + 4 * scale,
		                         background.y + 4 * scale + i * line_height, scale, lines[i],
		                         text_color);
	}
	SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
}

static void scrnmng_draw_framebuffer_info_overlay(const VAEG_VIEWPORT *viewport) {
	char lines[VIDEOVA_FRAMEBUFFERS][48];
	int g0_bpp;
	int g1_bpp;
	int scale;
	int line_height;
	int width;
	int height;
	int output_width;
	int output_height;
	int y_offset;
	int i;
	SDL_Rect background;
	SDL_Color text_color;

	if ((viewport == NULL) || (!viewport->valid) ||
	    ((np2oscfg.DISPCLK & VAEG_DISPINFO_FRAMEBUFFER) == 0)) {
		return;
	}
	if ((SDL_GetRendererOutputSize(scrnmng.renderer, &output_width, &output_height) != 0) ||
	    (output_width <= 0) || (output_height <= 0)) {
		return;
	}
	g0_bpp = scrnmng_video_bpp(videova.grres);
	g1_bpp = scrnmng_video_bpp(videova.grres >> 8);
	for (i = 0; i < VIDEOVA_FRAMEBUFFERS; i++) {
		int bpp = (i & 1) ? g1_bpp : g0_bpp;
		scrnmng_format_framebuffer_line(lines[i], sizeof(lines[i]), i, bpp,
		                                &videova.framebuffer[i]);
	}
	scale = (int)viewport->scale_x;
	if (scale < 1) {
		scale = 1;
	}
	if (scale > 3) {
		scale = 3;
	}
	line_height = 8 * scale;
	width = 0;
	for (i = 0; i < VIDEOVA_FRAMEBUFFERS; i++) {
		const int length = (int)strlen(lines[i]);
		if (length > width) {
			width = length;
		}
	}
	width = width * 8 * scale + 8 * scale;
	height = line_height * VIDEOVA_FRAMEBUFFERS + 8 * scale;
	y_offset = scrnmng_menu_offset() + 2;
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO) != 0) {
		y_offset += 4 * line_height + 8 * scale + 4;
	}
	if ((width >= output_width) || (y_offset + height >= output_height)) {
		return;
	}
	background.x = output_width - width;
	background.y = y_offset;
	background.w = width;
	background.h = height;
	SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, 190);
	SDL_RenderFillRect(scrnmng.renderer, &background);
	text_color.r = 255;
	text_color.g = 255;
	text_color.b = 192;
	text_color.a = 255;
	for (i = 0; i < VIDEOVA_FRAMEBUFFERS; i++) {
		scrnmng_draw_text_glyphs(background.x + 4 * scale,
		                         background.y + 4 * scale + i * line_height, scale, lines[i],
		                         text_color);
	}
	SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
}

static void scrnmng_log_renderer(void) {
	SDL_RendererInfo info;
	const char *name;
	const char *kind;

	if ((scrnmng.renderer == NULL) || (SDL_GetRendererInfo(scrnmng.renderer, &info) != 0)) {
		SDL_Log("SDL renderer backend: unknown");
		return;
	}
	name = (info.name) ? info.name : "unknown";
	snprintf(scrnmng.renderer_backend, sizeof(scrnmng.renderer_backend), "%s", name);
	kind = (info.flags & SDL_RENDERER_ACCELERATED) ? "accelerated" : "software";
	SDL_Log("SDL renderer backend: %s (%s)", name, kind);
}

static BOOL scrnmng_upload_shadow(void) {
	if ((scrnmng.texture == NULL) || (scrnmng.shadow == NULL)) {
		return (FAILURE);
	}
	if (SDL_UpdateTexture(scrnmng.texture, NULL, scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                      scrnmng.shadow_pitch) != 0) {
		fprintf(stderr, "Error: SDL_UpdateTexture: %s\n", SDL_GetError());
		return (FAILURE);
	}
	scrnmng.dirty = TRUE;
	return (SUCCESS);
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
		return (FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	if ((SDL_GetRendererOutputSize(scrnmng.renderer, &output_width, &output_height) != 0) ||
	    (window_width <= 0) || (window_height <= 0)) {
		return (FAILURE);
	}
	input.guest_width = scrnmng.width;
	input.guest_height = scrnmng.height;
	input.drawable_width = output_width;
	input.drawable_height = output_height;
	input.menu_inset =
	    (int)(((SINT64)scrnmng.menu_height * output_height + (window_height / 2)) / window_height);
	if (scrnmng.display_mode == VAEG_DISPLAY_WINDOWED) {
		input.scaling = scrnmng.scaling;
	} else {
		switch (scrnmng.fscrnmod & 3) {
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
	return (vaeg_viewport_calculate(&input, viewport));
}

void scrnmng_log_geometry(const char *reason) {
	int window_w;
	int window_h;
	int renderer_w;
	int renderer_h;
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
	        reason, window_w, window_h, renderer_w, renderer_h, viewport.x, viewport.y,
	        viewport.width, viewport.height, scrnmng.scale, scrnmng.menu_height, scrnmng.scaling,
	        scrnmng.effect);
}

BOOL scrnmng_texture_uniform(BOOL *uniform) {
	const BYTE *base;
	BYTE first0;
	BYTE first1;
	int x;
	int y;

	if (uniform == NULL) {
		return (FAILURE);
	}
	*uniform = TRUE;
	if ((!scrnmng.enable) || (scrnmng.shadow == NULL)) {
		return (FAILURE);
	}
	base = scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2);
	first0 = base[0];
	first1 = base[1];
	for (y = 0; y < scrnmng.height; y++) {
		const BYTE *row;

		row = base + (y * scrnmng.shadow_pitch);
		for (x = 0; x < scrnmng.width; x++) {
			const BYTE *pixel;

			pixel = row + (x * 2);
			if ((pixel[0] != first0) || (pixel[1] != first1)) {
				*uniform = FALSE;
				return (SUCCESS);
			}
		}
	}
	return (SUCCESS);
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
		return (FAILURE);
	}
	scrnmng.window =
	    SDL_CreateWindow(app_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
	                     SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (scrnmng.window == NULL) {
		fprintf(stderr, "Error: SDL_CreateWindow: %s\n", SDL_GetError());
		return (FAILURE);
	}
	SDL_EventState(SDL_DROPBEGIN, SDL_ENABLE);
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	SDL_EventState(SDL_DROPCOMPLETE, SDL_ENABLE);
	appicon_set_window(scrnmng.window);
	SDL_SetWindowMinimumSize(scrnmng.window, 320, 240);
	SDL_GetWindowPosition(scrnmng.window, &scrnmng.window_x, &scrnmng.window_y);
	scrnmng.window_width = width;
	scrnmng.window_height = height;
	scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_ACCELERATED);
	if (scrnmng.renderer == NULL) {
		scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_SOFTWARE);
	}
	if (scrnmng.renderer == NULL) {
		fprintf(stderr, "Error: SDL_CreateRenderer: %s\n", SDL_GetError());
		return (FAILURE);
	}
	scrnmng_log_renderer();
	SDL_RenderSetLogicalSize(scrnmng.renderer, 0, 0);
	scrnmng.texture =
	    SDL_CreateTexture(scrnmng.renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC,
	                      SCRNMNG_CANVAS_WIDTH, SCRNMNG_CANVAS_HEIGHT);
	if (scrnmng.texture == NULL) {
		fprintf(stderr, "Error: SDL_CreateTexture: %s\n", SDL_GetError());
		return (FAILURE);
	}
	SDL_SetTextureScaleMode(scrnmng.texture, (scrnmng.effect == VAEG_EFFECT_UNFILTERED)
	                                             ? SDL_ScaleModeNearest
	                                             : SDL_ScaleModeLinear);
	scrnmng.width = SCRNMNG_CANVAS_WIDTH;
	scrnmng.height = SCRNMNG_CANVAS_HEIGHT;
	scrnmng.shadow_pitch = (SCRNMNG_CANVAS_WIDTH + SCRNMNG_SURFACE_GUARD_LEFT) * 2;
	scrnmng.shadow = (BYTE *)calloc(SCRNMNG_CANVAS_HEIGHT, scrnmng.shadow_pitch);
	{
		const char *rendered_path;

		rendered_path = getenv("VAEG_SCREEN_DUMP");
		scrnmng.rendered_capture_enabled = (rendered_path != NULL) && (rendered_path[0] != '\0');
	}
	if (scrnmng.shadow == NULL) {
		fprintf(stderr, "Error: shadow framebuffer allocation failed\n");
		scrnmng_destroy();
		return (FAILURE);
	}
	scrnmng_clear_shadow();
	scrnmng.enable = TRUE;
	scrnmng_update_title();
	return (SUCCESS);
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
	if (scrnmng.rendered_frame != NULL) {
		SDL_FreeSurface(scrnmng.rendered_frame);
		scrnmng.rendered_frame = NULL;
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
	return (scrnmng.window);
}

void *scrnmng_get_renderer(void) {
	return (scrnmng.renderer);
}

const char *scrnmng_get_renderer_backend(void) {
	if (scrnmng.renderer_backend[0] == '\0') {
		return ("unknown");
	}
	return (scrnmng.renderer_backend);
}

static BOOL scrnmng_update_window_size(void) {
	if (scrnmng.window && (scrnmng.display_mode == VAEG_DISPLAY_WINDOWED)) {
		int current_w;
		int current_h;
		int target_w;
		int target_h;

		current_w = 0;
		current_h = 0;
		target_w = scrnmng.width * scrnmng.scale;
		target_h = scrnmng.menu_height + (scrnmng.height * scrnmng.scale);
		SDL_GetWindowSize(scrnmng.window, &current_w, &current_h);
		if ((current_w == target_w) && (current_h == target_h)) {
			return (FALSE);
		}
		SDL_SetWindowSize(scrnmng.window, target_w, target_h);
		return (TRUE);
	}
	return (FALSE);
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
	} else if (scale > 3) {
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
	return (scrnmng.scale);
}

BOOL scrnmng_get_display_aspect(void) {
	return (scrnmng.aspect);
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
	return (scrnmng.scaling);
}

void scrnmng_set_effect(int effect) {
	if ((effect < 0) || (effect >= VAEG_EFFECT_COUNT)) {
		effect = VAEG_EFFECT_UNFILTERED;
	}
	scrnmng.effect = effect;
	if (scrnmng.texture != NULL) {
		SDL_SetTextureScaleMode(scrnmng.texture, (effect == VAEG_EFFECT_UNFILTERED)
		                                             ? SDL_ScaleModeNearest
		                                             : SDL_ScaleModeLinear);
	}
}

int scrnmng_get_effect(void) {
	return (scrnmng.effect);
}

BOOL scrnmng_get_viewport(VAEG_VIEWPORT *viewport) {
	if (viewport == NULL) {
		return (FAILURE);
	}
	return (scrnmng_calculate_viewport(viewport));
}

BOOL scrnmng_map_window_point(int window_x, int window_y, int *guest_x, int *guest_y) {
	VAEG_VIEWPORT viewport;
	int window_width;
	int window_height;
	int drawable_x;
	int drawable_y;
	int output_width;
	int output_height;

	if (scrnmng_calculate_viewport(&viewport) != SUCCESS) {
		return (FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	SDL_GetRendererOutputSize(scrnmng.renderer, &output_width, &output_height);
	if ((window_width <= 0) || (window_height <= 0)) {
		return (FAILURE);
	}
	drawable_x = (int)(((SINT64)window_x * output_width) / window_width);
	drawable_y = (int)(((SINT64)window_y * output_height) / window_height);
	return (vaeg_viewport_map_point(&viewport, scrnmng.width, scrnmng.height, drawable_x,
	                                drawable_y, guest_x, guest_y));
}

static void scrnmng_remember_window(void) {
	if ((scrnmng.window == NULL) || (scrnmng.display_mode != VAEG_DISPLAY_WINDOWED)) {
		return;
	}
	SDL_GetWindowPosition(scrnmng.window, &scrnmng.window_x, &scrnmng.window_y);
	SDL_GetWindowSize(scrnmng.window, &scrnmng.window_width, &scrnmng.window_height);
	scrnmng.window_maximized =
	    (SDL_GetWindowFlags(scrnmng.window) & SDL_WINDOW_MAXIMIZED) ? TRUE : FALSE;
}

static void scrnmng_restore_window(void) {
	SDL_RestoreWindow(scrnmng.window);
	SDL_SetWindowPosition(scrnmng.window, scrnmng.window_x, scrnmng.window_y);
	SDL_SetWindowSize(scrnmng.window, max(320, scrnmng.window_width),
	                  max(240, scrnmng.window_height));
	if (scrnmng.window_maximized) {
		SDL_MaximizeWindow(scrnmng.window);
	}
}

static BOOL scrnmng_find_display_mode(int monitor, int width, int height, int refresh,
                                      SDL_DisplayMode *result) {
	int count;
	int index;

	count = SDL_GetNumDisplayModes(monitor);
	for (index = 0; index < count; index++) {
		SDL_DisplayMode mode;

		if ((SDL_GetDisplayMode(monitor, index, &mode) == 0) && (mode.w == width) &&
		    (mode.h == height) && ((refresh == 0) || (mode.refresh_rate == refresh))) {
			*result = mode;
			return (SUCCESS);
		}
	}
	SDL_SetError("Requested display mode is not available");
	return (FAILURE);
}

BOOL scrnmng_set_display_mode(int mode, int monitor, UINT width, UINT height, UINT refresh,
                              UINT8 fscrnmod) {
	SDL_Rect bounds;
	SDL_DisplayMode desktop;
	SDL_DisplayMode selected;
	int target_width;
	int target_height;
	int display_count;

	if ((scrnmng.window == NULL) || (mode < 0) || (mode >= VAEG_DISPLAY_MODE_COUNT)) {
		return (FAILURE);
	}
	display_count = SDL_GetNumVideoDisplays();
	if (display_count <= 0) {
		return (mode == VAEG_DISPLAY_WINDOWED ? SUCCESS : FAILURE);
	}
	if ((monitor < 0) || (monitor >= display_count)) {
		monitor = 0;
	}
	if ((SDL_GetDisplayBounds(monitor, &bounds) != 0) ||
	    (SDL_GetDesktopDisplayMode(monitor, &desktop) != 0)) {
		return (FAILURE);
	}
	if (scrnmng.display_mode == VAEG_DISPLAY_WINDOWED) {
		scrnmng_remember_window();
	}
	if (SDL_SetWindowFullscreen(scrnmng.window, 0) != 0) {
		return (FAILURE);
	}
	scrnmng.fscrnmod = vaeg_fscrnmod_sanitize(fscrnmod, NULL);
	if (mode == VAEG_DISPLAY_WINDOWED) {
		scrnmng.display_mode = mode;
		scrnmng_restore_window();
		scrnmng_log_geometry("windowed");
		return (SUCCESS);
	}
	SDL_RestoreWindow(scrnmng.window);
	SDL_SetWindowPosition(scrnmng.window, bounds.x, bounds.y);
	if (mode == VAEG_DISPLAY_BORDERLESS) {
		if (SDL_SetWindowFullscreen(scrnmng.window, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0) {
			scrnmng.display_mode = mode;
			scrnmng_log_geometry("borderless");
			return (SUCCESS);
		}
	} else {
		vaeg_fullscreen_size(width, height, scrnmng.fscrnmod, desktop.w, desktop.h, &target_width,
		                     &target_height);
		if ((scrnmng_find_display_mode(monitor, target_width, target_height, refresh, &selected) ==
		     SUCCESS) &&
		    (SDL_SetWindowDisplayMode(scrnmng.window, &selected) == 0) &&
		    (SDL_SetWindowFullscreen(scrnmng.window, SDL_WINDOW_FULLSCREEN) == 0)) {
			scrnmng.display_mode = mode;
			scrnmng_log_geometry("exclusive");
			return (SUCCESS);
		}
	}
	(void)SDL_SetWindowFullscreen(scrnmng.window, 0);
	scrnmng.display_mode = VAEG_DISPLAY_WINDOWED;
	scrnmng_restore_window();
	return (FAILURE);
}

int scrnmng_get_display_mode(void) {
	return (scrnmng.display_mode);
}

BOOL scrnmng_isfullscreen(void) {
	return (scrnmng.display_mode != VAEG_DISPLAY_WINDOWED);
}

BOOL scrnmng_capture_window_size(int *width, int *height) {
	if ((scrnmng.window == NULL) || (scrnmng.display_mode != VAEG_DISPLAY_WINDOWED) ||
	    (width == NULL) || (height == NULL)) {
		return (FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, width, height);
	return (SUCCESS);
}

RGB16 scrnmng_makepal16(RGB32 pal32) {
	RGB16 ret;

	ret = (pal32.p.r & 0xf8) << 8;
#if defined(SIZE_QVGA)
	ret += (pal32.p.g & 0xfc) << (3 + 16);
#else
	ret += (pal32.p.g & 0xfc) << 3;
#endif
	ret += pal32.p.b >> 3;
	return (ret);
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
		return (NULL);
	}
	scrnsurf.ptr = scrnmng.shadow;
	scrnsurf.xalign = 2;
	scrnsurf.yalign = scrnmng.shadow_pitch;
	scrnsurf.bpp = 16;
	scrnsurf.width = min(scrnstat.width, scrnmng.width);
	scrnsurf.height = min(scrnstat.height, scrnmng.height);
	scrnsurf.extend = 0;
	return (&scrnsurf);
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

	if ((!scrnmng.enable) || (scrnmng.renderer == NULL) || (scrnmng.texture == NULL)) {
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
	if (((scrnmng.effect == VAEG_EFFECT_SCANLINE) || (scrnmng.effect == VAEG_EFFECT_CRT_LITE)) &&
	    (dst.h >= scrnmng.height)) {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0,
		                       (scrnmng.effect == VAEG_EFFECT_CRT_LITE) ? 42 : 34);
		for (row = 1; row < scrnmng.height; row += 2) {
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
			for (x = dst.x; x < (dst.x + dst.w); x += 3) {
				SDL_SetRenderDrawColor(scrnmng.renderer, 24, 2, 2, 10);
				SDL_RenderDrawLine(scrnmng.renderer, x, dst.y, x, dst.y + dst.h - 1);
				if ((x + 1) < (dst.x + dst.w)) {
					SDL_SetRenderDrawColor(scrnmng.renderer, 2, 24, 2, 10);
					SDL_RenderDrawLine(scrnmng.renderer, x + 1, dst.y, x + 1, dst.y + dst.h - 1);
				}
				if ((x + 2) < (dst.x + dst.w)) {
					SDL_SetRenderDrawColor(scrnmng.renderer, 2, 2, 24, 10);
					SDL_RenderDrawLine(scrnmng.renderer, x + 2, dst.y, x + 2, dst.y + dst.h - 1);
				}
			}
		}
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		for (x = 0; x < 8; x++) {
			SDL_Rect left = {dst.x + x, dst.y, 1, dst.h};
			SDL_Rect right = {dst.x + dst.w - 1 - x, dst.y, 1, dst.h};
			SDL_Rect top = {dst.x, dst.y + x, dst.w, 1};
			SDL_Rect bottom = {dst.x, dst.y + dst.h - 1 - x, dst.w, 1};
			SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, (UINT8)(38 - x * 4));
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
	VAEG_VIEWPORT viewport;

	if ((!scrnmng.enable) || (scrnmng.renderer == NULL)) {
		return;
	}
	if (scrnmng_calculate_viewport(&viewport) == SUCCESS) {
		scrnmng_draw_video_info_overlay(&viewport);
		scrnmng_draw_framebuffer_info_overlay(&viewport);
	}
	if (scrnmng.rendered_capture_enabled) {
		scrnmng_capture_rendered_frame();
	}
	SDL_RenderPresent(scrnmng.renderer);
}

BOOL scrnmng_save_rendered_frame(const char *path) {
	if ((path == NULL) || (path[0] == '\0') || (scrnmng.renderer == NULL)) {
		return (FAILURE);
	}
	scrnmng_capture_rendered_frame();
	if (scrnmng.rendered_frame == NULL) {
		return (FAILURE);
	}
	if (SDL_SaveBMP(scrnmng.rendered_frame, path) != 0) {
		fprintf(stderr, "scsitrace rendered-screen-save-failed path=%s error=%s\n", path,
		        SDL_GetError());
		return (FAILURE);
	}
	return (SUCCESS);
}

void scrnmng_set_framedisp(BOOL enabled) {
	scrnmng.framedisp_enabled = enabled ? TRUE : FALSE;
	scrnmng_reset_metrics();
}

void scrnmng_reset_metrics(void) {
	vaeg_framedisp_reset(&scrnmng.framedisp, SDL_GetTicks(), drawcount);
	speedmeter.valid = FALSE;
	scrnmng_update_title();
}

void scrnmng_refresh_title(void) {
	scrnmng_update_title();
}

void scrnmng_framedisp_tick(UINT32 tick, UINT32 draws, UINT32 frames) {
	if (scrnmng.framedisp_enabled) {
		(void)vaeg_framedisp_update(&scrnmng.framedisp, tick, draws);
	}
	scrnmng_update_speedmeter(tick, frames);
	scrnmng_update_title();
}

BOOL scrnmng_entermenu(SCRNMENU *smenu) {
	if (smenu) {
		smenu->width = scrnmng.width;
		smenu->height = scrnmng.height;
		smenu->bpp = 16;
	}
	return (FAILURE);
}

void scrnmng_leavemenu(void) {
}

void scrnmng_menudraw(const RECT_T *rct) {
	(void)rct;
}
