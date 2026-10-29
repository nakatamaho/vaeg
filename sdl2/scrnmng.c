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
#include <stdlib.h>
#include "compiler.h"
#include "sdlapi.h"
#include "dosio.h"
#include "scrnmng.h"
#include "np2.h"
#include "np2ver.h"
#include "machine/pccore.h"
#include "memoryva.h"
#include "sgp.h"
#include "videova.h"
#include "tsp.h"
#include "fontdata.h"
#include "diskdrv.h"
#include "fddfile.h"
#include "appicon.h"
#include "framedisp.h"
#include "librashader/native_presenter_controller.h"
#include "librashader/builtin_shaders.h"
#include "gui/gui.h"
#include "recording/recording_native_composition.h"
#include "recording/recording_native_session.h"
#include "recording/recording_runtime.h"
#include "recording/recording_osd_snapshot.h"
#include "frontpanel.h"
#include "machine_status.h"
#include "winutf.h"

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
	int rotation;
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
	VAEG_NATIVE_PRESENTER *native_presenter;
	BOOL native_active;
	BOOL native_fallback_pending;
	UINT64 native_frame_number;
	BOOL native_capture_warning;
	BOOL native_change_pending;
	BOOL native_reload_pending;
	char native_status[256];
	char display_capture_path[MAX_PATH];
	BOOL display_capture_ready;
	VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK displayed_capture_callback;
	void *displayed_capture_context;
	BYTE *displayed_capture_pixels;
	UINT64 displayed_capture_size_bytes;
	UINT32 displayed_capture_pitch_bytes;
	UINT32 displayed_capture_width;
	UINT32 displayed_capture_height;
	UINT32 displayed_capture_generation;
	UINT32 displayed_capture_expected_generation;
	UINT64 displayed_capture_last_token;
	BOOL displayed_capture_have_token;
	BOOL displayed_capture_native;
	BOOL displayed_capture_active;
	VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR displayed_capture_error;
	UINT32 sdl_renderer_generation; /* Also identifies a native presenter lifetime. */
	BOOL sdl_overlays_drawn;
	VAEG_RECORDING_OSD_SNAPSHOT osd_snapshot;
	BOOL osd_snapshot_valid;
	UINT8 osd_snapshot_dispclk;
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
	SCRNMNG_CANVAS_HEIGHT = 400,
	/* Taller frames (up to 480 rasters, measured in M104) grow the canvas. */
	SCRNMNG_CANVAS_MAX_HEIGHT = 480
};

static SCRNMNG scrnmng;
static BOOL recording_include_osd = TRUE;
static BOOL recording_follow_rotation = TRUE;
static SCRNSTAT scrnstat;
/* M117: the guest picture turned right, for Native CRT and dummy capture */
static UINT16 scrnmng_turned[SCRNMNG_CANVAS_WIDTH * SCRNMNG_CANVAS_MAX_HEIGHT];
static SCRNSURF scrnsurf;
static VAEG_SPEEDMETER speedmeter;

static BOOL scrnmng_calculate_viewport(VAEG_VIEWPORT *viewport);
static void scrnmng_draw_video_info_overlay(const VAEG_VIEWPORT *viewport);
static void scrnmng_draw_framebuffer_info_overlay(const VAEG_VIEWPORT *viewport);
static void scrnmng_draw_front_panel(void);
static BOOL scrnmng_get_drawable_size(int *width, int *height);
static BOOL scrnmng_create_sdl_resources(void);
static BOOL scrnmng_native_fallback(void);
static BOOL scrnmng_ensure_osd_snapshot(const VAEG_VIEWPORT *viewport);
static void scrnmng_displayed_capture_fail(
	VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR error);
static BOOL scrnmng_capture_displayed_recording_frame(void);

/* NULL selects the session's vaeg.cfg-backed CRT parameter store. */
static const char *const scrnmng_native_parameter_state = NULL;

const char *scrnmng_native_preset_path(void) {
#if defined(VAEG_STATIC_LIBRASHADER)
	const char *configured = np2oscfg.gui_shader_preset;
	if (configured[0] && strcmp(configured, VAEG_DEFAULT_SHADER_PRESET) != 0)
		return configured;
	{
		char cache_dir[MAX_PATH];
		(void)file_getappdir(cache_dir, sizeof(cache_dir), "vaeg-cache");
		vaeg_builtin_shader_set_cache_root(cache_dir);
	}
	return vaeg_builtin_shader_preset();
#else
	static char bundled_path[4096];
	const char *configured = np2oscfg.gui_shader_preset;
	SDL_RWops *file;
	char *base;
	if (configured[0] && strcmp(configured, VAEG_DEFAULT_SHADER_PRESET) != 0)
		return configured;
	// Shipped assets follow the executable, independently of the launch directory.
	base = SDL_GetBasePath();
	if (base) {
		snprintf(bundled_path, sizeof(bundled_path), "%s%s", base, VAEG_DEFAULT_SHADER_PRESET);
		SDL_free(base);
		file = SDL_RWFromFile(bundled_path, "rb");
		if (file) {
			SDL_RWclose(file);
			return bundled_path;
		}
	}
	return VAEG_DEFAULT_SHADER_PRESET;
#endif
}

/*
 * Front panel lamp bar below the guest picture (sdl2/frontpanel.c): in a visible
 * window only, not in full screen and not under a headless video driver, so
 * captures and headless runs are unchanged. With the native CRT presenter
 * the bar is drawn through the GUI overlay.
 */
static BOOL scrnmng_visible_ui(void) {
	const char *driver = SDL_GetCurrentVideoDriver();

	return driver != NULL && !vaeg_native_presenter_is_headless_video_driver(driver);
}

BOOL scrnmng_status_visible(void) {
	return np2oscfg.machine_status && scrnmng_visible_ui();
}

/* Both footers reserve space outside guest pixels. The lamp bar retains its
 * windowed-only policy; the independently selected status also works fullscreen. */
static int scrnmng_front_panel_height(int width, BOOL drawable) {
	const int status = scrnmng_status_visible() ? gui_status_bar_height() : 0;
	const int lamps = scrnmng.display_mode == VAEG_DISPLAY_WINDOWED ? frontpanel_height(width) : 0;
	int window_width, window_height, output_width, output_height, top, inset;

	if (!scrnmng_visible_ui())
		return 0;
	if (drawable && scrnmng.window != NULL) {
		SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
		if (scrnmng_get_drawable_size(&output_width, &output_height) == SUCCESS &&
		    machine_status_layout(window_height, output_height, status, lamps, &top, &inset))
			return inset;
	}
	return status + lamps;
}

BOOL scrnmng_status_rect(RECT_T *rect) {
	int width, height, output_width, output_height;
	int inset;

	if (rect == NULL || scrnmng.window == NULL || !scrnmng_status_visible())
		return FALSE;
	SDL_GetWindowSize(scrnmng.window, &width, &height);
	if (scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS || output_height <= 0)
		return FALSE;
	if (!machine_status_layout(
	        height, output_height, gui_status_bar_height(),
	        scrnmng.display_mode == VAEG_DISPLAY_WINDOWED ? frontpanel_height(output_width) : 0,
	        &rect->top, &inset))
		return FALSE;
	rect->left = 0;
	rect->right = width;
	rect->bottom = min(height, rect->top + gui_status_bar_height());
	return TRUE;
}

/* Rows of the window height taken by the panel now (windowed mode). The
 * saved window size excludes them, and showing or hiding the panel adds or
 * removes them from the current window instead of resetting its size. */
static int scrnmng_panel_applied = 0;

int scrnmng_front_panel_applied(void) {
	return scrnmng_panel_applied;
}

static void scrnmng_fit_front_panel(void) {
	int width;
	int height;
	int panel;

	if ((scrnmng.window == NULL) || (scrnmng.display_mode != VAEG_DISPLAY_WINDOWED)) {
		return;
	}
	SDL_GetWindowSize(scrnmng.window, &width, &height);
	panel = scrnmng_front_panel_height(width, FALSE);
	if (panel == scrnmng_panel_applied) {
		return;
	}
	height = max(240, height - scrnmng_panel_applied + panel);
	scrnmng_panel_applied = panel;
	SDL_SetWindowSize(scrnmng.window, width, height);
}

/* A filled rectangle on the SDL renderer, or the GUI overlay when native. */
static void scrnmng_panel_fill(const SDL_Rect *r, Uint8 red, Uint8 green, Uint8 blue) {
	if (scrnmng.native_active) {
		gui_overlay_rect(r->x, r->y, r->w, r->h, red, green, blue, 255);
	} else {
		SDL_SetRenderDrawColor(scrnmng.renderer, red, green, blue, 255);
		SDL_RenderFillRect(scrnmng.renderer, r);
	}
}

static BOOL scrnmng_native_requested(void) {
	const char *video_driver;

	const char *override = SDL_getenv("VAEG_NATIVE_CRT");
	if (override ? strcmp(override, "1") != 0 : !np2oscfg.gui_native_crt) {
		return FALSE;
	}
	video_driver = SDL_GetCurrentVideoDriver();
	return (video_driver == NULL) ||
	       !vaeg_native_presenter_is_headless_video_driver(video_driver);
}

/* The shadow canvas turned 90 degrees right: height x width pixels, guest
 * (x, y) at (height - 1 - y, x). */
static const UINT16 *scrnmng_turn_shadow(void) {
	const BYTE *row;
	int x;
	int y;
	int w;
	int h;

	w = scrnmng.width;
	h = scrnmng.height;
	for (y = 0; y < h; y++) {
		row = scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2) + (y * scrnmng.shadow_pitch);
		for (x = 0; x < w; x++) {
			scrnmng_turned[(x * h) + (h - 1 - y)] = ((const UINT16 *)row)[x];
		}
	}
	return (scrnmng_turned);
}

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
	if (scrnmng.rotation == VAEG_ROTATE_RIGHT) {
		SDL_FreeSurface(source);
		source = SDL_CreateRGBSurfaceWithFormatFrom((void *)scrnmng_turn_shadow(), scrnmng.height,
		                                            scrnmng.width, 16, scrnmng.height * 2,
		                                            SDL_PIXELFORMAT_RGB565);
		if (source == NULL) {
			return;
		}
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

static UINT32 scrnmng_png_crc32_update(UINT32 crc, const BYTE *data, size_t length) {
	size_t i;
	int bit;

	for (i = 0; i < length; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++) {
			crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320U) : (crc >> 1);
		}
	}
	return crc;
}

static UINT32 scrnmng_png_adler32(const BYTE *data, size_t length) {
	UINT32 a = 1;
	UINT32 b = 0;
	size_t i;

	for (i = 0; i < length; i++) {
		a += data[i];
		if (a >= 65521U) {
			a -= 65521U;
		}
		b += a;
		if (b >= 65521U) {
			b -= 65521U;
		}
	}
	return (b << 16) | a;
}

static BOOL scrnmng_png_write_u32(FILE *fp, UINT32 value) {
	BYTE bytes[4];

	bytes[0] = (BYTE)(value >> 24);
	bytes[1] = (BYTE)(value >> 16);
	bytes[2] = (BYTE)(value >> 8);
	bytes[3] = (BYTE)value;
	return (fwrite(bytes, 1, sizeof(bytes), fp) == sizeof(bytes)) ? SUCCESS : FAILURE;
}

static BOOL scrnmng_png_write_chunk(FILE *fp, const char type[4], const BYTE *data,
	                                  UINT32 length) {
	UINT32 crc;

	if (scrnmng_png_write_u32(fp, length) != SUCCESS || fwrite(type, 1, 4, fp) != 4) {
		return FAILURE;
	}
	if ((length != 0) && (fwrite(data, 1, length, fp) != length)) {
		return FAILURE;
	}
	crc = scrnmng_png_crc32_update(0xffffffffU, (const BYTE *)type, 4);
	if (length != 0) {
		crc = scrnmng_png_crc32_update(crc, data, length);
	}
	crc ^= 0xffffffffU;
	return scrnmng_png_write_u32(fp, crc);
}

/* fopen on a UTF-8 path (Windows needs the wide-character call for
 * non-ASCII names such as the per-software state directories, M110). */
static FILE *scrnmng_fopen(const char *path, BOOL write) {
#if defined(WIN32)
	wchar_t *wide = winutf_from_utf8(path);
	FILE *fp;

	if (wide == NULL) {
		return (NULL);
	}
	fp = _wfopen(wide, write ? L"wb" : L"rb");
	winutf_free(wide);
	return (fp);
#else
	return (fopen(path, write ? "wb" : "rb"));
#endif
}

static BOOL scrnmng_png_save_surface(SDL_Surface *surface, const char *path) {
	BYTE ihdr[13];
	BYTE *raw = NULL;
	BYTE *compressed = NULL;
	FILE *fp = NULL;
	size_t row_bytes;
	size_t raw_length;
	size_t compressed_length;
	size_t blocks;
	size_t source_y;
	size_t source_x;
	size_t compressed_pos;
	size_t raw_pos;
	size_t remaining;
	BOOL locked = FALSE;
	BOOL result = FAILURE;

	if ((surface == NULL) || (path == NULL) || (surface->w <= 0) || (surface->h <= 0)) {
		return FAILURE;
	}
	row_bytes = (size_t)surface->w * 4 + 1;
	if ((row_bytes > (SIZE_MAX / (size_t)surface->h)) ||
	    ((raw_length = row_bytes * (size_t)surface->h) > UINT32_MAX)) {
		return FAILURE;
	}
	blocks = (raw_length + 65534) / 65535;
	if ((blocks > (SIZE_MAX - raw_length - 6) / 5) ||
	    ((compressed_length = raw_length + 6 + blocks * 5) > UINT32_MAX)) {
		return FAILURE;
	}
	raw = (BYTE *)malloc(raw_length);
	compressed = (BYTE *)malloc(compressed_length);
	if ((raw == NULL) || (compressed == NULL)) {
		goto cleanup;
	}
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) != 0) {
			goto cleanup;
		}
		locked = TRUE;
	}
	for (source_y = 0; source_y < (size_t)surface->h; source_y++) {
		BYTE *destination = raw + source_y * row_bytes;
		const BYTE *source = (const BYTE *)surface->pixels + source_y * (size_t)surface->pitch;

		destination[0] = 0;
		for (source_x = 0; source_x < (size_t)surface->w; source_x++) {
			const BYTE *pixel_bytes = source + source_x * (size_t)surface->format->BytesPerPixel;
			UINT32 pixel = 0;
			UINT8 red;
			UINT8 green;
			UINT8 blue;
			UINT8 alpha;

			switch (surface->format->BytesPerPixel) {
			case 1:
				pixel = pixel_bytes[0];
				break;
			case 2:
				memcpy(&pixel, pixel_bytes, 2);
				break;
			case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				pixel = ((UINT32)pixel_bytes[0] << 16) | ((UINT32)pixel_bytes[1] << 8) |
				        pixel_bytes[2];
#else
				pixel = pixel_bytes[0] | ((UINT32)pixel_bytes[1] << 8) |
				        ((UINT32)pixel_bytes[2] << 16);
#endif
				break;
			default:
				memcpy(&pixel, pixel_bytes, 4);
				break;
			}
			SDL_GetRGBA(pixel, surface->format, &red, &green, &blue, &alpha);
			destination[1 + source_x * 4 + 0] = red;
			destination[1 + source_x * 4 + 1] = green;
			destination[1 + source_x * 4 + 2] = blue;
			destination[1 + source_x * 4 + 3] = alpha;
		}
	}
	if (locked) {
		SDL_UnlockSurface(surface);
		locked = FALSE;
	}

	compressed[0] = 0x78;
	compressed[1] = 0x01;
	compressed_pos = 2;
	raw_pos = 0;
	remaining = raw_length;
	while (remaining != 0) {
		size_t block_length = (remaining > 65535) ? 65535 : remaining;
		compressed[compressed_pos++] = (remaining == block_length) ? 1 : 0;
		compressed[compressed_pos++] = (BYTE)block_length;
		compressed[compressed_pos++] = (BYTE)(block_length >> 8);
		compressed[compressed_pos++] = (BYTE)~block_length;
		compressed[compressed_pos++] = (BYTE)(~block_length >> 8);
		memcpy(compressed + compressed_pos, raw + raw_pos, block_length);
		compressed_pos += block_length;
		raw_pos += block_length;
		remaining -= block_length;
	}
	{
		UINT32 adler = scrnmng_png_adler32(raw, raw_length);
		compressed[compressed_pos++] = (BYTE)(adler >> 24);
		compressed[compressed_pos++] = (BYTE)(adler >> 16);
		compressed[compressed_pos++] = (BYTE)(adler >> 8);
		compressed[compressed_pos++] = (BYTE)adler;
	}
	if (compressed_pos != compressed_length) {
		goto cleanup;
	}
	ihdr[0] = (BYTE)((UINT32)surface->w >> 24);
	ihdr[1] = (BYTE)((UINT32)surface->w >> 16);
	ihdr[2] = (BYTE)((UINT32)surface->w >> 8);
	ihdr[3] = (BYTE)surface->w;
	ihdr[4] = (BYTE)((UINT32)surface->h >> 24);
	ihdr[5] = (BYTE)((UINT32)surface->h >> 16);
	ihdr[6] = (BYTE)((UINT32)surface->h >> 8);
	ihdr[7] = (BYTE)surface->h;
	ihdr[8] = 8;
	ihdr[9] = 6;
	ihdr[10] = 0;
	ihdr[11] = 0;
	ihdr[12] = 0;
	fp = scrnmng_fopen(path, TRUE);
	if (fp == NULL) {
		goto cleanup;
	}
	if (fwrite("\x89PNG\r\n\x1a\n", 1, 8, fp) != 8 ||
	    scrnmng_png_write_chunk(fp, "IHDR", ihdr, sizeof(ihdr)) != SUCCESS ||
	    scrnmng_png_write_chunk(fp, "IDAT", compressed, (UINT32)compressed_length) != SUCCESS ||
	    scrnmng_png_write_chunk(fp, "IEND", NULL, 0) != SUCCESS) {
		goto cleanup;
	}
	if (fclose(fp) != 0) {
		fp = NULL;
		goto cleanup;
	}
	fp = NULL;
	result = SUCCESS;

cleanup:
	if (locked) {
		SDL_UnlockSurface(surface);
	}
	if (fp != NULL) {
		(void)fclose(fp);
	}
	free(compressed);
	free(raw);
	return result;
}

static BOOL scrnmng_path_is_png(const char *path) {
	const char *extension;
	unsigned char first;
	unsigned char second;
	unsigned char third;

	if (path == NULL) {
		return FALSE;
	}
	extension = strrchr(path, '.');
	if ((extension == NULL) || (strlen(extension) != 4)) {
		return FALSE;
	}
	first = (unsigned char)extension[1];
	second = (unsigned char)extension[2];
	third = (unsigned char)extension[3];
	if ((first >= 'A') && (first <= 'Z')) {
		first = (unsigned char)(first - 'A' + 'a');
	}
	if ((second >= 'A') && (second <= 'Z')) {
		second = (unsigned char)(second - 'A' + 'a');
	}
	if ((third >= 'A') && (third <= 'Z')) {
		third = (unsigned char)(third - 'A' + 'a');
	}
	return (first == 'p') && (second == 'n') && (third == 'g') && (extension[4] == '\0');
}

UINT32 scrnmng_draw_fps_tenths(void) {
	return scrnmng.framedisp.fps_tenths;
}

UINT32 scrnmng_guest_fps_tenths(void) {
	return speedmeter.fps_tenths;
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
	int length;

	if (scrnmng.window == NULL)
		return;
	length = snprintf(title, sizeof(title), "%s", app_name);
	/* Preserve the independent FDD title preference. Machine and clocks now
	 * have one presentation, in the information bar, not duplicated here. */
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_FDD) != 0) {
		scrnmng_append_fdd_title(title, sizeof(title), &length, 0);
		scrnmng_append_fdd_title(title, sizeof(title), &length, 1);
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
	if (!active) {
		(void)snprintf(line, line_size, "%s OFF", label);
		return;
	}
	if (!scrnmng_framebuffer_valid(framebuffer, framebuffer_no)) {
		(void)snprintf(line, line_size, "%s OFF", label);
		return;
	}
	(void)snprintf(line, line_size, "%s ON %dx%d %dbpp", label, logical_width, logical_height, bpp);
}

static int scrnmng_format_video_info_lines(char lines[][96]) {
	const int graphics_height = (videova.grmode & 0x0002) ? 200 : 400;
	const int text_height = (tsp.screenlines != 0) ? tsp.screenlines : graphics_height;
	const int g0_width = (videova.grres & 0x0010) ? 320 : 640;
	const int g1_width = (videova.grres & 0x1000) ? 320 : 640;
	const int g0_bpp = scrnmng_video_bpp(videova.grres);
	const int g1_bpp = scrnmng_video_bpp(videova.grres >> 8);
	const BOOL g0_active = (videova.grmode & 0x8000) != 0;
	/* G1 is a separate screen only in single-plane, two-screen mode. */
	const BOOL g1_active = g0_active && ((videova.grmode & 0x0c00) == 0x0c00);

	scrnmng_format_video_line(lines[0], sizeof(lines[0]), "TEXT", 640, text_height, 4);
	scrnmng_format_video_line(lines[1], sizeof(lines[1]), "SPRITE", 640, text_height, 4);
	scrnmng_format_graphics_line(lines[2], sizeof(lines[2]), "G0", g0_active, g0_width,
	                             graphics_height, g0_bpp, 0, &videova.framebuffer[0]);
	scrnmng_format_graphics_line(lines[3], sizeof(lines[3]), "G1", g1_active, g1_width,
	                             graphics_height, g1_bpp, 1, &videova.framebuffer[1]);
	return 4;
}

static int scrnmng_format_framebuffer_lines(char lines[][48], int framebuffer_no, int bpp,
                                            FRAMEBUFFER framebuffer) {
	int width;
	int source_height;
	int line_count;

	if (!scrnmng_framebuffer_valid(framebuffer, framebuffer_no)) {
		(void)snprintf(lines[0], 48, "FB%d OFF", framebuffer_no);
		return 1;
	}
	width = scrnmng_framebuffer_width(framebuffer, bpp);
	line_count = 0;
	if (framebuffer->fbl == 0xffff) {
		(void)snprintf(lines[line_count++], 48, "FB%d source %dxN/A", framebuffer_no, width);
	} else {
		source_height = (int)framebuffer->fbl + 1;
		(void)snprintf(lines[line_count++], 48, "FB%d source %dx%d", framebuffer_no, width,
		               source_height);
	}
	(void)snprintf(lines[line_count++], 48, "FB%d view %dx%d", framebuffer_no, width,
	               scrnmng_framebuffer_height(framebuffer));
	(void)snprintf(lines[line_count++], 48, "FB%d DSA %06Xh", framebuffer_no,
	               (unsigned int)(framebuffer->dsa & 0x03ffffu));
	return line_count;
}

static void scrnmng_draw_text_glyphs(int x, int y, int scale, const char *text, SDL_Color color) {
	const unsigned char *glyph;
	int character;
	int row;
	int bit;

	if ((text == NULL) || (scale <= 0)) {
		return;
	}
	if (!scrnmng.native_active)
		SDL_SetRenderDrawColor(scrnmng.renderer, color.r, color.g, color.b, color.a);
	while (*text != '\0') {
		character = (unsigned char)*text++;
		glyph = fontdata_8 + (character * 8);
		for (row = 0; row < 8; row++) {
			for (bit = 0; bit < 8; bit++) {
				if ((glyph[row] & (0x80 >> bit)) != 0) {
					SDL_Rect pixel = {x + bit * scale, y + row * scale, scale, scale};
					if (scrnmng.native_active)
						gui_overlay_rect(pixel.x, pixel.y, pixel.w, pixel.h, color.r, color.g, color.b, color.a);
					else
						SDL_RenderFillRect(scrnmng.renderer, &pixel);
				}
			}
		}
		x += 8 * scale;
	}
}

static void scrnmng_draw_text_glyphs_surface(SDL_Surface *surface, int x, int y, int scale,
                                             const char *text, SDL_Color color) {
	const unsigned char *glyph;
	UINT32 pixel_value;
	int character;
	int row;
	int bit;

	if ((surface == NULL) || (text == NULL) || (scale <= 0)) {
		return;
	}
	pixel_value = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
	while (*text != '\0') {
		character = (unsigned char)*text++;
		glyph = fontdata_8 + (character * 8);
		for (row = 0; row < 8; row++) {
			for (bit = 0; bit < 8; bit++) {
				if ((glyph[row] & (0x80 >> bit)) != 0) {
					SDL_Rect pixel = {x + bit * scale, y + row * scale, scale, scale};
					(void)SDL_FillRect(surface, &pixel, pixel_value);
				}
			}
		}
		x += 8 * scale;
	}
}

static BOOL scrnmng_draw_graphics_analysis(SDL_Surface *surface) {
	char video_lines[4][96];
	char framebuffer_lines[VIDEOVA_FRAMEBUFFERS * 3][48];
	const int video_count = (np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO)
	                            ? scrnmng_format_video_info_lines(video_lines) : 0;
	const int line_height = 8;
	const int padding = 4;
	const SDL_Color text_color = {255, 255, 192, 255};
	int framebuffer_count = 0;
	int line_count;
	int width = 0;
	int panel_width;
	int panel_height;
	int panel_y;
	int i;
	SDL_Surface *panel;
	SDL_Rect destination;

	if ((surface == NULL) || (surface->w <= 0) || (surface->h <= 0)) {
		return FAILURE;
	}
	for (i = 0; (np2oscfg.DISPCLK & VAEG_DISPINFO_FRAMEBUFFER) && i < VIDEOVA_FRAMEBUFFERS; i++) {
		const int bpp = (i & 1) ? scrnmng_video_bpp(videova.grres >> 8)
		                       : scrnmng_video_bpp(videova.grres);
		framebuffer_count += scrnmng_format_framebuffer_lines(&framebuffer_lines[framebuffer_count], i,
	                                                     bpp, &videova.framebuffer[i]);
	}
	line_count = video_count + framebuffer_count;
	if (line_count == 0) return SUCCESS;
	for (i = 0; i < video_count; i++) {
		const int length = (int)strlen(video_lines[i]);
		if (length > width) {
			width = length;
		}
	}
	for (i = 0; i < framebuffer_count; i++) {
		const int length = (int)strlen(framebuffer_lines[i]);
		if (length > width) {
			width = length;
		}
	}
	panel_width = width * 8 + padding * 2;
	panel_height = line_count * line_height + padding * 2;
	if (panel_width > surface->w) {
		panel_width = surface->w;
	}
	if (panel_height > surface->h) {
		panel_height = surface->h;
	}
	panel_y = (surface->h > panel_height) ? 2 : 0;
	if (panel_y + panel_height > surface->h) {
		panel_y = surface->h - panel_height;
	}
	panel = SDL_CreateRGBSurfaceWithFormat(0, panel_width, panel_height, 32,
	                                       SDL_PIXELFORMAT_ARGB8888);
	if (panel == NULL) {
		return FAILURE;
	}
	if (SDL_FillRect(panel, NULL, SDL_MapRGBA(panel->format, 0, 0, 0, 190)) != 0) {
		SDL_FreeSurface(panel);
		return FAILURE;
	}
	SDL_SetSurfaceBlendMode(panel, SDL_BLENDMODE_BLEND);
	destination.x = surface->w - panel_width;
	destination.y = panel_y;
	destination.w = panel_width;
	destination.h = panel_height;
	if (SDL_BlitSurface(panel, NULL, surface, &destination) != 0) {
		SDL_FreeSurface(panel);
		return FAILURE;
	}
	SDL_FreeSurface(panel);

	for (i = 0; i < video_count; i++) {
		scrnmng_draw_text_glyphs_surface(surface, destination.x + padding,
		                                  destination.y + padding + i * line_height, 1,
		                                  video_lines[i], text_color);
	}
	for (i = 0; i < framebuffer_count; i++) {
		scrnmng_draw_text_glyphs_surface(surface, destination.x + padding,
		                                  destination.y + padding + (video_count + i) * line_height,
		                                  1, framebuffer_lines[i], text_color);
	}
	return SUCCESS;
}

static int scrnmng_menu_offset(void) {
	int window_width;
	int window_height;
	int output_width;
	int output_height;

	if (scrnmng.window == NULL) {
		return 0;
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	if ((scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) ||
	    (window_height <= 0) || (output_height <= 0)) {
		return scrnmng.menu_height;
	}
	return (int)(((SINT64)scrnmng.menu_height * output_height + (window_height / 2)) /
	             window_height);
}

static int scrnmng_osd_scale(const VAEG_VIEWPORT *viewport) {
	int scale;

	if (viewport == NULL) {
		return 1;
	}
	scale = (int)viewport->scale_x;
	if (scale < 1) {
		scale = 1;
	}
	if (scale > 3) {
		scale = 3;
	}
	return scale;
}

static int scrnmng_osd_width(const char *const *lines, int line_count, int scale) {
	int width = 0;
	int i;

	for (i = 0; i < line_count; ++i) {
		const int length = (int)strlen(lines[i]);
		if (length > width) {
			width = length;
		}
	}
	return width * 8 * scale + 8 * scale;
}

static BOOL scrnmng_ensure_osd_snapshot(const VAEG_VIEWPORT *viewport) {
	char video_lines[4][96];
	char framebuffer_lines[VIDEOVA_FRAMEBUFFERS * 3][48];
	const char *video_line_ptrs[4];
	const char *framebuffer_line_ptrs[VIDEOVA_FRAMEBUFFERS * 3];
	VAEG_RECORDING_OSD_LAYER_INPUT layers[VAEG_RECORDING_OSD_MAX_LAYERS];
	VAEG_RECORDING_OSD_SNAPSHOT_INPUT input;
	int video_count = 0;
	int framebuffer_count = 0;
	int output_width;
	int output_height;
	int scale;
	int line_height;
	int width;
	int height;
	int y_offset;
	int i;
	VAEG_RECORDING_OSD_ERROR error;
	const BOOL video_enabled = (np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO) != 0;
	const BOOL framebuffer_enabled = (np2oscfg.DISPCLK & VAEG_DISPINFO_FRAMEBUFFER) != 0;

	if ((viewport == NULL) || (!viewport->valid) ||
	    (scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) ||
	    (output_width <= 0) || (output_height <= 0)) {
		scrnmng.osd_snapshot_valid = FALSE;
		return FAILURE;
	}
	if (scrnmng.osd_snapshot_valid &&
	    (scrnmng.osd_snapshot_dispclk == np2oscfg.DISPCLK) &&
	    (scrnmng.osd_snapshot.frame_token == (UINT64)drawcount) &&
	    (scrnmng.osd_snapshot.guest_width == (UINT32)scrnmng.width) &&
	    (scrnmng.osd_snapshot.guest_height == (UINT32)scrnmng.height) &&
	    (scrnmng.osd_snapshot.output_width == (UINT32)output_width) &&
	    (scrnmng.osd_snapshot.output_height == (UINT32)output_height)) {
		return SUCCESS;
	}

	ZeroMemory(video_lines, sizeof(video_lines));
	ZeroMemory(framebuffer_lines, sizeof(framebuffer_lines));
	ZeroMemory(video_line_ptrs, sizeof(video_line_ptrs));
	ZeroMemory(framebuffer_line_ptrs, sizeof(framebuffer_line_ptrs));
	ZeroMemory(layers, sizeof(layers));
	scale = scrnmng_osd_scale(viewport);
	line_height = 8 * scale;
	if (video_enabled) {
		video_count = scrnmng_format_video_info_lines(video_lines);
		for (i = 0; i < video_count; ++i) {
			video_line_ptrs[i] = video_lines[i];
		}
	}
	if (framebuffer_enabled) {
		const int g0_bpp = scrnmng_video_bpp(videova.grres);
		const int g1_bpp = scrnmng_video_bpp(videova.grres >> 8);
		for (i = 0; i < VIDEOVA_FRAMEBUFFERS; ++i) {
			const int bpp = (i & 1) ? g1_bpp : g0_bpp;
			framebuffer_count += scrnmng_format_framebuffer_lines(
			    &framebuffer_lines[framebuffer_count], i, bpp, &videova.framebuffer[i]);
		}
		for (i = 0; i < framebuffer_count; ++i) {
			framebuffer_line_ptrs[i] = framebuffer_lines[i];
		}
	}

	ZeroMemory(&layers[0], sizeof(layers[0]));
	layers[0].kind = VAEG_RECORDING_OSD_VIDEO_INFO;
	layers[0].coordinate_space = VAEG_RECORDING_OSD_COORDINATE_OUTPUT;
	layers[0].anchor = VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT;
	layers[0].enabled = video_enabled ? 1u : 0u;
	layers[0].visible = 0u;
	layers[0].scale = (UINT8)scale;
	layers[0].background_alpha = 190u;
	layers[0].text_alpha = 255u;
	layers[0].line_count = (UINT32)video_count;
	layers[0].lines = video_line_ptrs;
	width = scrnmng_osd_width(video_line_ptrs, video_count, scale);
	height = line_height * video_count + 8 * scale;
	layers[0].x = output_width - width;
	layers[0].y = scrnmng_menu_offset() + 2;
	layers[0].width = width;
	layers[0].height = height;
	layers[0].clip_x = 0;
	layers[0].clip_y = 0;
	layers[0].clip_width = output_width;
	layers[0].clip_height = output_height;
	if (video_enabled && (width < output_width) && (height < output_height)) {
		layers[0].visible = 1u;
	}

	ZeroMemory(&layers[1], sizeof(layers[1]));
	layers[1].kind = VAEG_RECORDING_OSD_FRAMEBUFFER_INFO;
	layers[1].coordinate_space = VAEG_RECORDING_OSD_COORDINATE_OUTPUT;
	layers[1].anchor = VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT;
	layers[1].enabled = framebuffer_enabled ? 1u : 0u;
	layers[1].visible = 0u;
	layers[1].scale = (UINT8)scale;
	layers[1].background_alpha = 190u;
	layers[1].text_alpha = 255u;
	layers[1].line_count = (UINT32)framebuffer_count;
	layers[1].lines = framebuffer_line_ptrs;
	width = scrnmng_osd_width(framebuffer_line_ptrs, framebuffer_count, scale);
	height = line_height * framebuffer_count + 8 * scale;
	y_offset = scrnmng_menu_offset() + 2;
	if (video_enabled) {
		y_offset += 4 * line_height + 8 * scale + 4;
	}
	layers[1].x = output_width - width;
	layers[1].y = y_offset;
	layers[1].width = width;
	layers[1].height = height;
	layers[1].clip_x = 0;
	layers[1].clip_y = 0;
	layers[1].clip_width = output_width;
	layers[1].clip_height = output_height;
	if (framebuffer_enabled && (width < output_width) &&
	    (y_offset + height < output_height)) {
		layers[1].visible = 1u;
	}

	ZeroMemory(&input, sizeof(input));
	input.frame_token = (UINT64)drawcount;
	input.guest_width = (UINT32)scrnmng.width;
	input.guest_height = (UINT32)scrnmng.height;
	input.output_width = (UINT32)output_width;
	input.output_height = (UINT32)output_height;
	input.layer_count = VAEG_RECORDING_OSD_MAX_LAYERS;
	input.layers = layers;
	error = vaeg_recording_osd_snapshot_build(&input, &scrnmng.osd_snapshot);
	if (error != VAEG_RECORDING_OSD_ERROR_OK) {
		scrnmng.osd_snapshot_valid = FALSE;
		fprintf(stderr, "OSD snapshot failed: %s\n", vaeg_recording_osd_error_name(error));
		return FAILURE;
	}
	scrnmng.osd_snapshot_valid = TRUE;
	scrnmng.osd_snapshot_dispclk = np2oscfg.DISPCLK;
	return SUCCESS;
}

static void scrnmng_draw_video_info_overlay(const VAEG_VIEWPORT *viewport) {
	const VAEG_RECORDING_OSD_LAYER *layer;
	int i;
	SDL_Rect background;
	SDL_Color text_color;

	if ((viewport == NULL) || (!viewport->valid) ||
	    ((np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO) == 0)) {
		return;
	}
	if (scrnmng_ensure_osd_snapshot(viewport) != SUCCESS) {
		return;
	}
	layer = vaeg_recording_osd_snapshot_find(&scrnmng.osd_snapshot,
	                                          VAEG_RECORDING_OSD_VIDEO_INFO);
	if ((layer == NULL) || (!layer->enabled) || (!layer->visible)) {
		return;
	}
	background.x = layer->x;
	background.y = layer->y;
	background.w = layer->width;
	background.h = layer->height;
	if (scrnmng.native_active) {
		gui_overlay_rect(background.x, background.y, background.w, background.h, 0, 0, 0,
		                layer->background_alpha);
	} else {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, layer->background_alpha);
		SDL_RenderFillRect(scrnmng.renderer, &background);
	}
	text_color.r = 255;
	text_color.g = 255;
	text_color.b = 192;
	text_color.a = layer->text_alpha;
	for (i = 0; i < (int)layer->line_count; i++) {
		scrnmng_draw_text_glyphs(background.x + 4 * layer->scale,
		                         background.y + 4 * layer->scale + i * 8 * layer->scale,
		                         layer->scale, layer->lines[i], text_color);
	}
	if (!scrnmng.native_active)
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
}

static void scrnmng_draw_framebuffer_info_overlay(const VAEG_VIEWPORT *viewport) {
	const VAEG_RECORDING_OSD_LAYER *layer;
	int i;
	SDL_Rect background;
	SDL_Color text_color;

	if ((viewport == NULL) || (!viewport->valid) ||
	    ((np2oscfg.DISPCLK & VAEG_DISPINFO_FRAMEBUFFER) == 0)) {
		return;
	}
	if (scrnmng_ensure_osd_snapshot(viewport) != SUCCESS) {
		return;
	}
	layer = vaeg_recording_osd_snapshot_find(&scrnmng.osd_snapshot,
	                                          VAEG_RECORDING_OSD_FRAMEBUFFER_INFO);
	if ((layer == NULL) || (!layer->enabled) || (!layer->visible)) {
		return;
	}
	background.x = layer->x;
	background.y = layer->y;
	background.w = layer->width;
	background.h = layer->height;
	if (scrnmng.native_active) {
		gui_overlay_rect(background.x, background.y, background.w, background.h, 0, 0, 0,
		                layer->background_alpha);
	} else {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, layer->background_alpha);
		SDL_RenderFillRect(scrnmng.renderer, &background);
	}
	text_color.r = 255;
	text_color.g = 255;
	text_color.b = 192;
	text_color.a = layer->text_alpha;
	for (i = 0; i < (int)layer->line_count; i++) {
		scrnmng_draw_text_glyphs(background.x + 4 * layer->scale,
		                         background.y + 4 * layer->scale + i * 8 * layer->scale,
		                         layer->scale, layer->lines[i], text_color);
	}
	if (!scrnmng.native_active)
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
}

void scrnmng_draw_native_overlays(void) {
	VAEG_VIEWPORT viewport;
	if (scrnmng.native_active && scrnmng_calculate_viewport(&viewport) == SUCCESS) {
		scrnmng_draw_video_info_overlay(&viewport);
		scrnmng_draw_framebuffer_info_overlay(&viewport);
	}
	if (scrnmng.native_active) {
		scrnmng_draw_front_panel();
	}
}

BOOL scrnmng_get_osd_snapshot(VAEG_RECORDING_OSD_SNAPSHOT *snapshot) {
	VAEG_VIEWPORT viewport;

	if ((snapshot == NULL) || (scrnmng_calculate_viewport(&viewport) != SUCCESS) ||
	    (scrnmng_ensure_osd_snapshot(&viewport) != SUCCESS)) {
		return FAILURE;
	}
	memcpy(snapshot, &scrnmng.osd_snapshot, sizeof(*snapshot));
	return SUCCESS;
}

BOOL scrnmng_configure_native_recording(BOOL include_osd, BOOL follow_rotation) {
	if (vaeg_recording_native_session_is_active()) {
		return FAILURE;
	}
	recording_include_osd = include_osd ? TRUE : FALSE;
	recording_follow_rotation = follow_rotation ? TRUE : FALSE;
	return SUCCESS;
}

BOOL scrnmng_capture_native_recording_frame(VAEG_RECORDING_NATIVE_FRAME *frame) {
	VAEG_RECORDING_OSD_SNAPSHOT snapshot;
	return scrnmng_capture_native_recording_frame_with_snapshot(frame, &snapshot);
}

BOOL scrnmng_capture_native_recording_frame_with_snapshot(
	VAEG_RECORDING_NATIVE_FRAME *frame, VAEG_RECORDING_OSD_SNAPSHOT *snapshot) {
	VAEG_RECORDING_NATIVE_RGB565_SOURCE source;
	VAEG_RECORDING_NATIVE_FRAME_ERROR error;

	if ((frame == NULL) || (scrnmng.shadow == NULL) || !scrnmng.enable ||
	    (snapshot == NULL) || (scrnmng.width <= 0) || (scrnmng.height <= 0) ||
	    (scrnmng.shadow_pitch <= 0)) {
		return FAILURE;
	}
	if (scrnmng_get_osd_snapshot(snapshot) != SUCCESS) {
		return FAILURE;
	}
	source.pixels = scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2);
	source.width = (UINT32)scrnmng.width;
	source.height = (UINT32)scrnmng.height;
	source.pitch_bytes = (UINT32)scrnmng.shadow_pitch;
	source.source_size_bytes = (UINT64)scrnmng.shadow_pitch * (UINT32)scrnmng.height -
	                           (SCRNMNG_SURFACE_GUARD_LEFT * 2);
	error = vaeg_recording_native_frame_copy_rgb565(&source, frame);
	if (error != VAEG_RECORDING_NATIVE_FRAME_OK) {
		fprintf(stderr, "Native recording frame copy failed: %s\n",
		        vaeg_recording_native_frame_error_name(error));
		return FAILURE;
	}
	if (recording_follow_rotation && scrnmng.rotation == VAEG_ROTATE_RIGHT) {
		error = vaeg_recording_native_frame_rotate_right(frame);
		if (error != VAEG_RECORDING_NATIVE_FRAME_OK) {
			vaeg_recording_native_frame_destroy(frame);
			return FAILURE;
		}
		snapshot->guest_width = frame->width;
		snapshot->guest_height = frame->height;
	}
	if (!recording_include_osd) {
		UINT32 layer;
		for (layer = 0; layer < snapshot->layer_count; ++layer) {
			snapshot->layers[layer].enabled = 0;
			snapshot->layers[layer].visible = 0;
		}
	}
	error = vaeg_recording_native_frame_apply_osd(frame, snapshot, fontdata_8);
	if (error != VAEG_RECORDING_NATIVE_FRAME_OK) {
		fprintf(stderr, "Native recording OSD composition failed: %s\n",
		        vaeg_recording_native_frame_error_name(error));
		vaeg_recording_native_frame_destroy(frame);
		return FAILURE;
	}
	return SUCCESS;
}

BOOL scrnmng_get_native_recording_video_descriptor(VAEG_RECORDING_VIDEO_DESCRIPTOR *descriptor) {
	if ((descriptor == NULL) || (scrnmng.width <= 0) || (scrnmng.height <= 0)) {
		return FAILURE;
	}
	ZeroMemory(descriptor, sizeof(*descriptor));
	if (recording_follow_rotation && scrnmng.rotation == VAEG_ROTATE_RIGHT) {
		descriptor->width = (UINT32)scrnmng.height;
		descriptor->height = (UINT32)scrnmng.width;
	} else {
		descriptor->width = (UINT32)scrnmng.width;
		descriptor->height = (UINT32)scrnmng.height;
	}
	descriptor->stride_bytes = (UINT64)descriptor->width * 3u;
	descriptor->pixel_format = VAEG_RECORDING_PIXEL_RGB24;
	return vaeg_recording_validate_video_descriptor(descriptor) == VAEG_RECORDING_ERROR_OK
	           ? SUCCESS
	           : FAILURE;
}

UINT32 scrnmng_native_recording_descriptor_generation(void) {
	UINT32 generation;

	if ((scrnmng.width <= 0) || (scrnmng.height <= 0) || (scrnmng.shadow_pitch <= 0)) {
		return 0u;
	}
	/* The native coded descriptor is fixed by the owned canonical RGB24 frame. */
	generation = 2166136261u;
	generation = (generation ^ (UINT32)scrnmng.width) * 16777619u;
	generation = (generation ^ (UINT32)scrnmng.height) * 16777619u;
	generation = (generation ^ (UINT32)scrnmng.shadow_pitch) * 16777619u;
	/* Equal canvas sizes can still cross a guest 15/24 kHz timing boundary. */
	generation = (generation ^ (UINT32)videova_hsyncmode()) * 16777619u;
	generation = (generation ^ 1u) * 16777619u;
	generation =
	    (generation ^ (UINT32)(recording_follow_rotation ? scrnmng.rotation : 0)) * 16777619u;
	return generation;
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
	SDL_Rect rect;

	rect.x = 0;
	rect.y = 0;
	rect.w = scrnmng.width;
	rect.h = scrnmng.height;
	if (SDL_UpdateTexture(scrnmng.texture, &rect, scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                      scrnmng.shadow_pitch) != 0) {
		fprintf(stderr, "Error: SDL_UpdateTexture: %s\n", SDL_GetError());
		return (FAILURE);
	}
	scrnmng.dirty = TRUE;
	return (SUCCESS);
}

/* Canvas rows for a guest frame of the given height: 400 to 480. */
static int scrnmng_canvas_height(int height) {
	if (height <= SCRNMNG_CANVAS_HEIGHT) {
		return SCRNMNG_CANVAS_HEIGHT;
	}
	if (height > SCRNMNG_CANVAS_MAX_HEIGHT) {
		return SCRNMNG_CANVAS_MAX_HEIGHT;
	}
	return height;
}

static void scrnmng_clear_shadow(void) {
	if (scrnmng.shadow == NULL) {
		return;
	}
	ZeroMemory(scrnmng.shadow, scrnmng.shadow_pitch * SCRNMNG_CANVAS_MAX_HEIGHT);
	(void)scrnmng_upload_shadow();
}

static BOOL scrnmng_calculate_viewport(VAEG_VIEWPORT *viewport) {
	VAEG_VIEWPORT_INPUT input;
	int window_width;
	int window_height;
	int output_width;
	int output_height;

	if (scrnmng.window == NULL) {
		return (FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	if ((scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) ||
	    (window_width <= 0) || (window_height <= 0)) {
		return (FAILURE);
	}
	input.guest_width = scrnmng.width;
	input.guest_height = scrnmng.height;
	input.drawable_width = output_width;
	input.drawable_height = output_height;
	input.menu_inset =
	    (int)(((SINT64)scrnmng.menu_height * output_height + (window_height / 2)) / window_height);
	input.bottom_inset = scrnmng_front_panel_height(output_width, TRUE);
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
	input.rotate = (scrnmng.rotation == VAEG_ROTATE_RIGHT) ? TRUE : FALSE;
	return (vaeg_viewport_calculate(&input, viewport));
}

void scrnmng_log_geometry(const char *reason) {
	int window_w;
	int window_h;
	int drawable_w;
	int drawable_h;
	VAEG_VIEWPORT viewport;

	if (!scrnmng.window) {
		return;
	}
	if (reason == NULL) {
		reason = "unknown";
	}
	window_w = 0;
	window_h = 0;
	drawable_w = 0;
	drawable_h = 0;
	SDL_GetWindowSize(scrnmng.window, &window_w, &window_h);
	(void)scrnmng_get_drawable_size(&drawable_w, &drawable_h);
	ZeroMemory(&viewport, sizeof(viewport));
	(void)scrnmng_calculate_viewport(&viewport);
	fprintf(stderr,
	        "SDL2 geometry [%s]: window=%dx%d drawable=%dx%d "
	        "guest=%d,%d %dx%d scale=%d menu=%d mode=%d effect=%d\n",
	        reason, window_w, window_h, drawable_w, drawable_h, viewport.x, viewport.y,
	        viewport.width, viewport.height, scrnmng.scale, scrnmng.menu_height, scrnmng.scaling,
	        scrnmng.effect);
}

static BOOL scrnmng_get_drawable_size(int *width, int *height) {
	int drawable_width;
	int drawable_height;

	if ((width == NULL) || (height == NULL) || (scrnmng.window == NULL)) {
		return FAILURE;
	}
	drawable_width = 0;
	drawable_height = 0;
	if ((scrnmng.renderer != NULL) &&
	    (SDL_GetRendererOutputSize(scrnmng.renderer, &drawable_width, &drawable_height) == 0)) {
		/* SDL_Renderer reports the physical drawable size for high-DPI windows. */
	} else {
		SDL_GetWindowSizeInPixels(scrnmng.window, &drawable_width, &drawable_height);
	}
	if ((drawable_width <= 0) || (drawable_height <= 0)) {
		return FAILURE;
	}
	*width = drawable_width;
	*height = drawable_height;
	return SUCCESS;
}

static void scrnmng_displayed_capture_release(void) {
	free(scrnmng.displayed_capture_pixels);
	scrnmng.displayed_capture_pixels = NULL;
	scrnmng.displayed_capture_size_bytes = 0u;
	scrnmng.displayed_capture_pitch_bytes = 0u;
	scrnmng.displayed_capture_width = 0u;
	scrnmng.displayed_capture_height = 0u;
	scrnmng.displayed_capture_generation = 0u;
	scrnmng.displayed_capture_expected_generation = 0u;
	scrnmng.displayed_capture_last_token = 0u;
	scrnmng.displayed_capture_have_token = FALSE;
	scrnmng.displayed_capture_native = FALSE;
	scrnmng.displayed_capture_active = FALSE;
	scrnmng.displayed_capture_callback = NULL;
	scrnmng.displayed_capture_context = NULL;
}

static void scrnmng_displayed_capture_fail(
	VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR error) {
	scrnmng.displayed_capture_error = error;
	scrnmng_displayed_capture_release();
}

BOOL scrnmng_get_displayed_recording_video_descriptor(
    VAEG_RECORDING_VIDEO_DESCRIPTOR *descriptor, UINT32 *generation) {
	int width, height;
	const char *driver = SDL_GetCurrentVideoDriver();
	if (descriptor == NULL || generation == NULL || !scrnmng.enable ||
	    (!scrnmng.native_active && scrnmng.renderer == NULL) ||
	    driver == NULL || !strcmp(driver, "dummy") || !strcmp(driver, "offscreen") ||
	    scrnmng.sdl_renderer_generation == 0u ||
	    scrnmng_get_drawable_size(&width, &height) != SUCCESS || width <= 0 || height <= 0 ||
	    (UINT64)(UINT32)width * (UINT32)height * 4u >
	        VAEG_RECORDING_DISPLAYED_FRAME_MAX_BYTES) {
		return FAILURE;
	}
	descriptor->width = (UINT32)width;
	descriptor->height = (UINT32)height;
	descriptor->stride_bytes = (UINT32)width * 3u;
	descriptor->pixel_format = VAEG_RECORDING_PIXEL_RGB24;
	*generation = scrnmng.sdl_renderer_generation;
	return SUCCESS;
}

BOOL scrnmng_start_displayed_recording_capture(
	VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK callback, void *context) {
	int width;
	int height;
	UINT64 pitch;
	UINT64 size;

	if (callback == NULL) {
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_NULL_INPUT;
		return FAILURE;
	}
	if (scrnmng.displayed_capture_active) {
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_BUSY;
		return FAILURE;
	}
	scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
	if (!scrnmng.enable || (!scrnmng.native_active && (scrnmng.renderer == NULL))) {
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_UNAVAILABLE;
		return FAILURE;
	}
	if ((SDL_GetCurrentVideoDriver() != NULL) &&
	    !strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_DUMMY_DRIVER;
		return FAILURE;
	}
	if ((scrnmng_get_drawable_size(&width, &height) != SUCCESS) || (width <= 0) ||
	    (height <= 0) || (scrnmng.sdl_renderer_generation == 0u)) {
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_UNAVAILABLE;
		return FAILURE;
	}
	pitch = (UINT64)(UINT32)width * 4u;
	if ((UINT64)(UINT32)height > (UINT64_MAX / pitch)) {
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_SIZE_OVERFLOW;
		return FAILURE;
	}
	size = pitch * (UINT32)height;
	if (size > VAEG_RECORDING_DISPLAYED_FRAME_MAX_BYTES) {
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_TOO_LARGE;
		return FAILURE;
	}
	scrnmng.displayed_capture_pixels = (BYTE *)malloc((size_t)size);
	if (scrnmng.displayed_capture_pixels == NULL) {
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_ALLOCATION_FAILED;
		return FAILURE;
	}
	scrnmng.displayed_capture_callback = callback;
	scrnmng.displayed_capture_context = context;
	scrnmng.displayed_capture_size_bytes = size;
	scrnmng.displayed_capture_pitch_bytes = (UINT32)pitch;
	scrnmng.displayed_capture_width = (UINT32)width;
	scrnmng.displayed_capture_height = (UINT32)height;
	scrnmng.displayed_capture_generation = scrnmng.sdl_renderer_generation;
	scrnmng.displayed_capture_expected_generation = scrnmng.sdl_renderer_generation;
	scrnmng.displayed_capture_last_token = (UINT64)drawcount;
	scrnmng.displayed_capture_have_token = TRUE;
	scrnmng.displayed_capture_native = scrnmng.native_active;
	scrnmng.displayed_capture_active = TRUE;
	return SUCCESS;
}

void scrnmng_stop_displayed_recording_capture(void) {
	scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
	scrnmng_displayed_capture_release();
}

BOOL scrnmng_displayed_recording_capture_active(void) {
	return scrnmng.displayed_capture_active;
}

VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR scrnmng_displayed_recording_capture_error(void) {
	return scrnmng.displayed_capture_error;
}

/* Presentation-thread preflight only: never render or allocate here. A false
 * result means either no new guest token or an explicitly stopped capture. */
static BOOL scrnmng_prepare_displayed_recording_frame(VAEG_OUTPUT_CAPTURE *output) {
	int width;
	int height;

	ZeroMemory(output, sizeof(*output));
	if (!scrnmng.displayed_capture_active) {
		return FALSE;
	}
	if ((scrnmng.native_active != scrnmng.displayed_capture_native) ||
	    (!scrnmng.native_active && (scrnmng.renderer == NULL))) {
		scrnmng_displayed_capture_fail(
		    VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED);
		return FALSE;
	}
	if ((scrnmng_get_drawable_size(&width, &height) != SUCCESS) ||
	    ((UINT32)width != scrnmng.displayed_capture_width) ||
	    ((UINT32)height != scrnmng.displayed_capture_height)) {
		scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_GEOMETRY_CHANGED);
		return FALSE;
	}
	if (scrnmng.sdl_renderer_generation != scrnmng.displayed_capture_expected_generation) {
		scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED);
		return FALSE;
	}
	if (scrnmng.displayed_capture_have_token &&
	    (scrnmng.displayed_capture_last_token == (UINT64)drawcount)) {
		return FALSE;
	}
	output->pixels = scrnmng.displayed_capture_pixels;
	output->width = scrnmng.displayed_capture_width;
	output->height = scrnmng.displayed_capture_height;
	output->pitch_bytes = scrnmng.displayed_capture_pitch_bytes;
	return TRUE;
}

static BOOL scrnmng_finish_displayed_recording_frame(const VAEG_OUTPUT_CAPTURE *output) {
	VAEG_RECORDING_DISPLAYED_FRAME capture;
	VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR error;
	if (!scrnmng.displayed_capture_active) {
		return FAILURE;
	}
	if (!output->complete || output->pixels != scrnmng.displayed_capture_pixels ||
	    output->width != scrnmng.displayed_capture_width ||
	    output->height != scrnmng.displayed_capture_height ||
	    output->pitch_bytes != scrnmng.displayed_capture_pitch_bytes) {
		scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_READBACK_FAILED);
		return FAILURE;
	}
	ZeroMemory(&capture, sizeof(capture));
	capture.pixels = scrnmng.displayed_capture_pixels;
	capture.width = scrnmng.displayed_capture_width;
	capture.height = scrnmng.displayed_capture_height;
	capture.pitch_bytes = scrnmng.displayed_capture_pitch_bytes;
	capture.size_bytes = scrnmng.displayed_capture_size_bytes;
	capture.pixel_format = VAEG_RECORDING_DISPLAYED_PIXEL_RGBA32;
	capture.row_origin = VAEG_RECORDING_DISPLAYED_ROWS_TOP_DOWN;
	capture.frame_token = (UINT64)drawcount;
	capture.descriptor_generation = scrnmng.displayed_capture_generation;
	error = vaeg_recording_displayed_capture_validate(&capture);
	if (error != VAEG_RECORDING_DISPLAYED_CAPTURE_OK) {
		scrnmng_displayed_capture_fail(error);
		return FAILURE;
	}
	if (scrnmng.displayed_capture_callback(scrnmng.displayed_capture_context, &capture) == 0) {
		scrnmng_displayed_capture_fail(
		    VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK_REJECTED);
		return FAILURE;
	}
	if (scrnmng.displayed_capture_active) {
		scrnmng.displayed_capture_last_token = (UINT64)drawcount;
		scrnmng.displayed_capture_have_token = TRUE;
	}
	return SUCCESS;
}

static BOOL scrnmng_capture_displayed_recording_frame(void) {
	VAEG_OUTPUT_CAPTURE output;
	if (!scrnmng.displayed_capture_active) {
		return SUCCESS;
	}
	if (!scrnmng_prepare_displayed_recording_frame(&output)) {
		return scrnmng.displayed_capture_active ? SUCCESS : FAILURE;
	}
	output.complete = SDL_RenderReadPixels(scrnmng.renderer, NULL, SDL_PIXELFORMAT_RGBA32,
	                                       output.pixels, (int)output.pitch_bytes) == 0;
	return scrnmng_finish_displayed_recording_frame(&output);
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

BOOL scrnmng_native_active(void) {
	return scrnmng.native_active;
}

BOOL scrnmng_take_native_fallback(void) {
	const BOOL pending = scrnmng.native_fallback_pending;

	scrnmng.native_fallback_pending = FALSE;
	return pending;
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

static BOOL scrnmng_create_sdl_resources(void) {
	if (scrnmng.displayed_capture_active) {
		scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED);
	}
	scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_ACCELERATED);
	if (scrnmng.renderer == NULL) {
		scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_SOFTWARE);
	}
	if (scrnmng.renderer == NULL) {
		fprintf(stderr, "Error: SDL_CreateRenderer: %s\n", SDL_GetError());
		return FAILURE;
	}
	scrnmng_log_renderer();
	SDL_RenderSetLogicalSize(scrnmng.renderer, 0, 0);
	scrnmng.texture =
	    SDL_CreateTexture(scrnmng.renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC,
	                      SCRNMNG_CANVAS_WIDTH, SCRNMNG_CANVAS_MAX_HEIGHT);
	if (scrnmng.texture == NULL) {
		fprintf(stderr, "Error: SDL_CreateTexture: %s\n", SDL_GetError());
		SDL_DestroyRenderer(scrnmng.renderer);
		scrnmng.renderer = NULL;
		return FAILURE;
	}
	SDL_SetTextureScaleMode(scrnmng.texture, (scrnmng.effect == VAEG_EFFECT_UNFILTERED)
	                                             ? SDL_ScaleModeNearest
	                                             : SDL_ScaleModeLinear);
	if (++scrnmng.sdl_renderer_generation == 0u) {
		scrnmng.sdl_renderer_generation = 1u;
	}
	return SUCCESS;
}

/* A native DXGI swap chain and SDL's selected presentation API must not
 * inherit each other's HWND state. Keep guest pixels/state, replace only
 * the host window when returning from native presentation on Windows. */
static BOOL scrnmng_detach_native_window(void) {
	SDL_Window *old_window = scrnmng.window;
	SDL_Window *new_window;
	SDL_DisplayMode mode = {0};
	Uint32 flags = SDL_GetWindowFlags(old_window);
	int x, y, width, height;
	SDL_GetWindowPosition(old_window, &x, &y);
	SDL_GetWindowSize(old_window, &width, &height);
	SDL_GetWindowDisplayMode(old_window, &mode);
	new_window = SDL_CreateWindow(app_name, x, y, width, height,
	    SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
	    (flags & (SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP)));
	if (!new_window) {
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Renderer switch: new window failed: %s", SDL_GetError());
		return FAILURE;
	}
	SDL_CaptureMouse(SDL_FALSE);
	SDL_DestroyWindow(old_window);
	scrnmng.window = new_window;
	appicon_set_window(new_window);
	SDL_SetWindowMinimumSize(new_window, 320, 240);
	if (flags & SDL_WINDOW_FULLSCREEN) {
		if (SDL_SetWindowDisplayMode(new_window, &mode) != 0 ||
		    SDL_SetWindowFullscreen(new_window, flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Renderer switch: fullscreen restore failed: %s", SDL_GetError());
			return FAILURE;
		}
	} else if (flags & SDL_WINDOW_MAXIMIZED) {
		SDL_MaximizeWindow(new_window);
	}
	if (!(flags & SDL_WINDOW_HIDDEN)) {
		SDL_ShowWindow(new_window);
		SDL_RaiseWindow(new_window);
	}
	SDL_Log("Renderer switch: native window detached; new SDL window=%u", SDL_GetWindowID(new_window));
	return SUCCESS;
}

static BOOL scrnmng_native_fallback(void) {
	if (!scrnmng.native_active || (scrnmng.native_presenter == NULL)) {
		return FAILURE;
	}
	if (scrnmng.displayed_capture_active) {
		scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED);
	}
	snprintf(scrnmng.native_status, sizeof(scrnmng.native_status), "CRT fallback: %s",
	         vaeg_native_presenter_error(scrnmng.native_presenter));
	gui_shutdown();
	vaeg_native_presenter_destroy(scrnmng.native_presenter);
	scrnmng.native_presenter = NULL;
	scrnmng.native_active = FALSE;
	scrnmng.native_fallback_pending = TRUE;
#if defined(_WIN32)
	if (scrnmng_detach_native_window() != SUCCESS) return FAILURE;
#endif
	if (scrnmng_create_sdl_resources() != SUCCESS) {
		fprintf(stderr, "Error: SDL fallback renderer creation failed: %s\n", SDL_GetError());
		return FAILURE;
	}
	(void)scrnmng_upload_shadow();
	scrnmng_update_title();
	fprintf(stderr, "Native CRT disabled after failure; SDL presentation restored\n");
	return SUCCESS;
}

BOOL scrnmng_window_rebind_selftest(void) {
	SDL_Window *saved_window = scrnmng.window;
	const BOOL video_was_active = SDL_WasInit(SDL_INIT_VIDEO) != 0;
	BOOL result = SUCCESS;
	int i;
	if (!video_was_active && SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return FAILURE;
	scrnmng.window = SDL_CreateWindow("renderer rebind test", 0, 0, 640, 400, SDL_WINDOW_HIDDEN);
	if (!scrnmng.window) result = FAILURE;
	for (i = 0; i < 10 && result == SUCCESS; ++i) {
		const Uint32 old_id = SDL_GetWindowID(scrnmng.window);
		int width, height;
		result = scrnmng_detach_native_window();
		if (result != SUCCESS) break;
		SDL_GetWindowSize(scrnmng.window, &width, &height);
		if (SDL_GetWindowID(scrnmng.window) == old_id || width != 640 || height != 400 ||
		    !(SDL_GetWindowFlags(scrnmng.window) & SDL_WINDOW_HIDDEN)) result = FAILURE;
		if (result == SUCCESS) {
			SDL_Renderer *renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_SOFTWARE);
			SDL_Rect pixel = {0, 0, 1, 1};
			BYTE rgba[4] = {0};
			if (!renderer) { result = FAILURE; break; }
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			if (SDL_RenderClear(renderer) != 0 ||
			    SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGBA32, rgba, 4) != 0 ||
			    rgba[0] != 255 || rgba[1] != 0 || rgba[2] != 0) result = FAILURE;
			SDL_RenderPresent(renderer);
			SDL_DestroyRenderer(renderer);
		}
	}
	if (scrnmng.window) SDL_DestroyWindow(scrnmng.window);
	scrnmng.window = saved_window;
	if (!video_was_active) SDL_QuitSubSystem(SDL_INIT_VIDEO);
	if (result != SUCCESS) fprintf(stderr, "selftest: WINDOW_REBIND_FAILED: %s\n", SDL_GetError());
	return result;
}

BOOL scrnmng_create(int width, int height) {
	const char *preset_path;
	Uint32 window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	width = max(320, width);
	height = max(240, height);
	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL video init: %s\n", SDL_GetError());
		return (FAILURE);
	}
#if defined(__linux__) && defined(VAEG_ENABLE_LIBRASHADER)
	if (scrnmng_native_requested()) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		window_flags |= SDL_WINDOW_OPENGL;
	}
#endif
	scrnmng.window = SDL_CreateWindow(app_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                  width, height, window_flags);
#if defined(__linux__) && defined(VAEG_ENABLE_LIBRASHADER)
	if (scrnmng.window == NULL && (window_flags & SDL_WINDOW_OPENGL)) {
		/* An unavailable GL visual must not prevent the ordinary SDL fallback. */
		SDL_GL_ResetAttributes();
		scrnmng.window = SDL_CreateWindow(app_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                                  width, height, window_flags & ~SDL_WINDOW_OPENGL);
	}
#endif
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
	scrnmng.width = SCRNMNG_CANVAS_WIDTH;
	scrnmng.height = scrnmng_canvas_height(scrnstat.height);
	scrnmng.shadow_pitch = (SCRNMNG_CANVAS_WIDTH + SCRNMNG_SURFACE_GUARD_LEFT) * 2;
	scrnmng.shadow = (BYTE *)calloc(SCRNMNG_CANVAS_MAX_HEIGHT, scrnmng.shadow_pitch);
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
	scrnmng.enable = TRUE;
	snprintf(scrnmng.native_status, sizeof(scrnmng.native_status), "SDL");
	if (scrnmng_native_requested()) {
		preset_path = scrnmng_native_preset_path();
		scrnmng.native_presenter = vaeg_native_presenter_create(scrnmng.window, 0, 0, preset_path,
		                                                        scrnmng_native_parameter_state);
		if (scrnmng.native_presenter != NULL) {
			scrnmng.native_active = TRUE;
			if (!np2oscfg.gui_native_filter &&
			    !vaeg_native_presenter_set_filter(scrnmng.native_presenter, 0)) {
				vaeg_native_presenter_destroy(scrnmng.native_presenter);
				scrnmng.native_presenter = NULL;
				scrnmng.native_active = FALSE;
				snprintf(scrnmng.native_status, sizeof(scrnmng.native_status),
				         "SDL fallback: native unfiltered mode unavailable");
			}
		}
		if (scrnmng.native_active) {
			/* Native startup never creates SDL renderer resources. Give its
			 * capture descriptor a nonzero lifetime generation as well. */
			if (++scrnmng.sdl_renderer_generation == 0u) {
				scrnmng.sdl_renderer_generation = 1u;
			}
			scrnmng.renderer_backend[0] = '\0';
			fprintf(stderr, "Native librashader selected: backend=%s mode=%s preset=%s\n",
			        vaeg_native_presenter_backend(scrnmng.native_presenter),
			        np2oscfg.gui_native_filter ? "CRT" : "unfiltered", preset_path);
		} else {
			if (scrnmng.native_status[0] == '\0' ||
			    strncmp(scrnmng.native_status, "SDL fallback:", 13) != 0) {
				snprintf(scrnmng.native_status, sizeof(scrnmng.native_status),
				         "SDL fallback: %s", vaeg_native_presenter_creation_error());
			}
			fprintf(stderr, "Native librashader selected but unavailable; using SDL fallback\n");
		}
	}
	if (scrnmng.native_active) {
		scrnmng_apply_native_mask_profile();
	}
	if (!scrnmng.native_active && (scrnmng_create_sdl_resources() != SUCCESS)) {
		scrnmng_destroy();
		return (FAILURE);
	}
	scrnmng_clear_shadow();
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
	if (scrnmng.displayed_capture_active) {
		scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_UNAVAILABLE);
	} else {
		scrnmng_displayed_capture_release();
	}
	scrnmng.enable = FALSE;
	scrnmng.visible = FALSE;
	if (scrnmng.native_presenter != NULL) {
		vaeg_native_presenter_destroy(scrnmng.native_presenter);
		scrnmng.native_presenter = NULL;
	}
	scrnmng.native_active = FALSE;
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

BOOL scrnmng_get_output_size(int *width, int *height) {
	return scrnmng_get_drawable_size(width, height);
}

BOOL scrnmng_present_startup_image(const void *pixels, int width, int height, int pitch,
                                    int x, int y, int output_width, int output_height) {
	VAEG_FRAME_INPUT frame;
	VAEG_VIEWPORT viewport;
	int drawable_width, drawable_height;
	BOOL filtered;
	VAEG_NATIVE_PRESENTER_RESULT result;
	if (!scrnmng.native_active || !pixels || width <= 0 || height <= 0 || pitch <= 0 ||
	    scrnmng_get_drawable_size(&drawable_width, &drawable_height) != SUCCESS) return FAILURE;
	if (vaeg_native_presenter_resize(scrnmng.native_presenter, drawable_width, drawable_height)
	    != VAEG_NATIVE_PRESENTER_PRESENTED) return FAILURE;
	filtered = strcmp(vaeg_native_presenter_state(scrnmng.native_presenter), "filtered") == 0;
	if (!vaeg_native_presenter_set_filter(scrnmng.native_presenter, 0)) return FAILURE;
	vaeg_native_presenter_set_output_viewport(scrnmng.native_presenter, x, y, output_width, output_height);
	vaeg_frame_input_initialize(&frame, pixels, width, height, pitch, VAEG_FRAME_PIXEL_ARGB8888,
	    VAEG_FRAME_ROWS_TOP_DOWN, width, height, 60, 1, 0, 16666667U);
	result = vaeg_native_presenter_present(scrnmng.native_presenter, &frame);
	/* The startup image is not guest VRAM and must not seed CRT history. */
	if (filtered) (void)vaeg_native_presenter_set_filter(scrnmng.native_presenter, 1);
	if (scrnmng_calculate_viewport(&viewport) == SUCCESS)
		vaeg_native_presenter_set_output_viewport(scrnmng.native_presenter, viewport.x,
		    viewport.y, viewport.width, viewport.height);
	return result == VAEG_NATIVE_PRESENTER_PRESENTED ? SUCCESS : FAILURE;
}

void *scrnmng_get_renderer(void) {
	return (scrnmng.renderer);
}

BOOL scrnmng_native_gui_prepare(void) {
	return vaeg_native_presenter_gui_prepare(scrnmng.native_presenter) ? SUCCESS : FAILURE;
}

BOOL scrnmng_fallback_to_sdl(void) {
	return scrnmng_native_fallback();
}

void scrnmng_native_gui_shutdown(void) {
	vaeg_native_presenter_gui_shutdown(scrnmng.native_presenter);
}

const char *scrnmng_native_status(void) {
	if (scrnmng.native_active) {
		const char *error = vaeg_native_presenter_error(scrnmng.native_presenter);
		if (strcmp(error, "none") != 0) {
			snprintf(scrnmng.native_status, sizeof(scrnmng.native_status),
			         "CRT unavailable: %s / native unfiltered", error);
			return scrnmng.native_status;
		}
		return strcmp(vaeg_native_presenter_state(scrnmng.native_presenter), "filtered") == 0
		           ? "Native CRT ON"
		           : "Native librashader / unfiltered";
	}
	return scrnmng.native_status;
}

BOOL scrnmng_native_set_parameter(const char *name, float value) {
	return vaeg_native_presenter_set_parameter(scrnmng.native_presenter, name, value) ? SUCCESS
	                                                                                  : FAILURE;
}

void scrnmng_request_native_crt(BOOL enabled, BOOL reload) {
	if (reload || ((np2oscfg.gui_native_crt != 0) != (enabled != 0)))
		vaeg_recording_runtime_screen_changed();
	np2oscfg.gui_native_crt = enabled ? 1 : 0;
	scrnmng.native_change_pending = TRUE;
	scrnmng.native_reload_pending = reload;
}

BOOL scrnmng_native_filter_enabled(void) {
	return scrnmng.native_active &&
	       strcmp(vaeg_native_presenter_state(scrnmng.native_presenter), "filtered") == 0;
}

BOOL scrnmng_set_native_filter(BOOL enabled) {
	// Presentation-thread operation: keep the device, GUI and preset intact.
	if (!scrnmng.native_active)
		return FAILURE;
	if ((enabled != 0) != (scrnmng_native_filter_enabled() != 0))
		vaeg_recording_runtime_screen_changed();
	if (!vaeg_native_presenter_set_filter(scrnmng.native_presenter, enabled)) {
		return FAILURE;
	}
	if (enabled) {
		scrnmng_apply_native_mask_profile();
	}
	scrnmng_update_title();
	return SUCCESS;
}

float scrnmng_native_mask_intensity(void) {
	if (scrnmng.scale <= 1) {
		return 0.15f;
	}
	if (scrnmng.scale == 2) {
		return 0.20f;
	}
	return 0.30f;
}

void scrnmng_apply_native_mask_profile(void) {
	if (!np2oscfg.gui_native_mask_auto || !scrnmng.native_active ||
	    !scrnmng_native_filter_enabled()) {
		return;
	}
	(void)scrnmng_native_set_parameter("MASK_INTENSITY", scrnmng_native_mask_intensity());
}

BOOL scrnmng_apply_native_crt_request(void) {
#if defined(_WIN32) && defined(VAEG_ENABLE_LIBRASHADER)
	const char *preset;
	BOOL was_native;
	if (!scrnmng.native_change_pending)
		return SUCCESS;
	scrnmng.native_change_pending = FALSE;
	if (scrnmng.native_active && np2oscfg.gui_native_crt && !scrnmng.native_reload_pending) {
		if (!vaeg_native_presenter_set_filter(scrnmng.native_presenter,
		                                      np2oscfg.gui_native_filter)) {
			fprintf(stderr, "Native CRT toggle failed\n");
		}
		if (np2oscfg.gui_native_filter) {
			scrnmng_apply_native_mask_profile();
		}
		scrnmng_update_title();
		return SUCCESS;
	}
	if (!scrnmng.native_active && !np2oscfg.gui_native_crt)
		return SUCCESS;
	// Called between ImGui frames on the presentation thread.
	if (scrnmng.displayed_capture_active) {
		scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED);
	}
	was_native = scrnmng.native_active;
	SDL_Log("Renderer switch: releasing GUI");
	gui_shutdown();
	SDL_Log("Renderer switch: releasing native presenter");
	vaeg_native_presenter_destroy(scrnmng.native_presenter);
	scrnmng.native_presenter = NULL;
	scrnmng.native_active = FALSE;
	if (scrnmng.texture)
		SDL_DestroyTexture(scrnmng.texture);
	scrnmng.texture = NULL;
	if (scrnmng.renderer)
		SDL_DestroyRenderer(scrnmng.renderer);
	scrnmng.renderer = NULL;
	preset = scrnmng_native_preset_path();
	if (np2oscfg.gui_native_crt) {
		scrnmng.native_presenter =
		    vaeg_native_presenter_create(scrnmng.window, 0, 0, preset, scrnmng_native_parameter_state);
	}
	scrnmng.native_active = scrnmng.native_presenter != NULL;
	if (scrnmng.native_active) {
		scrnmng_apply_native_mask_profile();
	}
	if (!scrnmng.native_active) {
		if (was_native && scrnmng_detach_native_window() != SUCCESS) return FAILURE;
		SDL_Log("Renderer switch: creating SDL renderer");
		if (np2oscfg.gui_native_crt) {
			snprintf(scrnmng.native_status, sizeof(scrnmng.native_status),
			         "SDL fallback: %s", vaeg_native_presenter_creation_error());
		} else {
			snprintf(scrnmng.native_status, sizeof(scrnmng.native_status), "SDL");
		}
		if (scrnmng_create_sdl_resources() != SUCCESS)
			return FAILURE;
		(void)scrnmng_upload_shadow();
	}
	scrnmng_update_title();
	SDL_Log("Renderer switch: initializing GUI");
	if (gui_initialize(scrnmng.window, scrnmng.renderer, NULL) == SUCCESS) {
		SDL_Log("Renderer switch complete: %s", scrnmng_get_renderer_backend());
		return SUCCESS;
	}
	if (!scrnmng.native_active || scrnmng_native_fallback() != SUCCESS) return FAILURE;
	(void)scrnmng_take_native_fallback();
	return gui_initialize(scrnmng.window, scrnmng.renderer, NULL);
#else
	scrnmng.native_change_pending = FALSE;
	return SUCCESS;
#endif
}

const char *scrnmng_get_renderer_backend(void) {
	if (scrnmng.native_active && (scrnmng.native_presenter != NULL)) {
		return vaeg_native_presenter_backend(scrnmng.native_presenter);
	}
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
		target_h = scrnmng.height * scrnmng.scale;
		if (scrnmng.rotation == VAEG_ROTATE_RIGHT) {
			target_w = scrnmng.height * scrnmng.scale;
			target_h = scrnmng.width * scrnmng.scale;
		}
		scrnmng_panel_applied = scrnmng_front_panel_height(target_w, FALSE);
		target_h += scrnmng.menu_height + scrnmng_panel_applied;
		SDL_GetWindowSize(scrnmng.window, &current_w, &current_h);
		if ((current_w == target_w) && (current_h == target_h)) {
			return (FALSE);
		}
		SDL_SetWindowSize(scrnmng.window, target_w, target_h);
		return (TRUE);
	}
	return (FALSE);
}

/* The simple front panel: a dark bar with the drive and mode lamps. */
static void scrnmng_draw_simple_panel_state(const SDL_Rect *bar, BOOL recording, BOOL finalizing,
                                            UINT64 ticks, UINT64 clock) {
	static const char *const mode_labels[3] = {"V1", "V2", "V3"};
	static const char *const lock_labels[2] = {"KANA", "CAPS"};
	const int s = frontpanel_simple_scale(bar->w);
	const UINT32 now = SDL_GetTicks();
	SDL_Color label = {200, 196, 180, 255};
	SDL_Rect r;
	char elapsed[32];
	int recording_x;
	int x = bar->x + 8 * s;
	int i;

	frontpanel_recording_time(elapsed, sizeof(elapsed), ticks, clock);
	/* Reserve a stable five-character timer, expanding for long recordings. */
	recording_x = bar->x + bar->w - (136 + 48 + (int)SDL_strlen(elapsed) * 8) * s;
	if (!scrnmng.native_active) {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
	}
	scrnmng_panel_fill(bar, 34, 34, 36);
	r.x = bar->x;
	r.y = bar->y;
	r.w = bar->w;
	r.h = s;
	scrnmng_panel_fill(&r, 70, 70, 74);
	for (i = 0; i < 7; i++) {
		const BOOL drive = (i < 2) ? TRUE : FALSE;
		char text[5];
		BOOL lit;
		Uint8 cr;
		Uint8 cg;
		Uint8 cb;

		/* Preserve the complete REC/timer/lock group in portrait windows. */
		if (i >= 2 && i < 5 && recording_x < bar->x + 272 * s) {
			continue;
		}
		if (drive && x + 40 * s > recording_x - 4 * s) {
			continue;
		}
		if (i == 2) {
			x += 16 * s; /* gap between the drives and the mode lamps */
		} else if (i == 5) {
			/* the lock lamps are right-aligned: two of "KANA" + lamp, the
			 * same 12-dot spacing between them, 8 dots from the right edge */
			x = bar->x + bar->w - 8 * s - (2 * (4 * 8 * s + 4 * s + 12 * s) + 12 * s);
		}
		if (drive) {
			text[0] = 'F';
			text[1] = 'D';
			text[2] = (char)('1' + i);
			text[3] = '\0';
			lit = frontpanel_drive_lit((UINT)i, now);
			if (frontpanel_drive_is_2hd((UINT)i)) {
				cr = 60;
				cg = 235;
				cb = 80;
			} else {
				cr = 255;
				cg = 45;
				cb = 30;
			}
		} else if (i < 5) {
			SDL_strlcpy(text, mode_labels[i - 2], sizeof(text));
			lit = frontpanel_mode_lit((UINT)(i - 2));
			cr = 120;
			cg = 255;
			cb = 130;
		} else {
			/* M109: the keyboard's KANA and CAPS lock lamps. */
			SDL_strlcpy(text, lock_labels[i - 5], sizeof(text));
			lit = frontpanel_lock_lit((UINT)(i - 5));
			cr = 255;
			cg = 170;
			cb = 40;
		}
		scrnmng_draw_text_glyphs(x, bar->y + 4 * s, s, text, label);
		x += (int)SDL_strlen(text) * 8 * s + 4 * s;
		r.x = x;
		r.y = bar->y + 5 * s;
		r.w = 12 * s;
		r.h = 6 * s;
		if (lit) {
			scrnmng_panel_fill(&r, cr, cg, cb);
		} else {
			scrnmng_panel_fill(&r, 52, 60, 52);
		}
		x += r.w + ((drive && bar->w < 344 * s) ? 4 : 12) * s;
	}
	/* Keep KANA/CAPS right-aligned; REC, lamp and movie time sit just left. */
	x = recording_x;
	scrnmng_draw_text_glyphs(x, bar->y + 4 * s, s, "REC", label);
	r.x = x + 28 * s;
	r.y = bar->y + 5 * s;
	r.w = 12 * s;
	r.h = 6 * s;
	if (finalizing) {
		scrnmng_panel_fill(&r, 255, 180, 40);
	} else if (recording) {
		scrnmng_panel_fill(&r, 255, 45, 30);
	} else {
		scrnmng_panel_fill(&r, 52, 60, 52);
	}
	if (recording || finalizing) {
		scrnmng_draw_text_glyphs(x + 48 * s, bar->y + 4 * s, s, elapsed, label);
	}
}

static void scrnmng_draw_simple_panel(const SDL_Rect *bar) {
	scrnmng_draw_simple_panel_state(bar, vaeg_recording_native_session_is_active(),
	                                vaeg_recording_native_session_is_finalizing(),
	                                vaeg_recording_native_session_elapsed_ticks(),
	                                pccore.realclock);
}

/* Draw the front panel across the bottom of the drawable, and fit the
 * window when the setting, display mode or presenter changes. */
static void scrnmng_draw_front_panel(void) {
	static int last_layout = -1;
	int layout;
	int output_w;
	int output_h;
	SDL_Rect panel;

	if ((scrnmng.window == NULL) || (!scrnmng.native_active && (scrnmng.renderer == NULL))) {
		return;
	}
	/* Only on a change of the panel setting, display mode or presenter: the
	 * bar grows with the window width, so following its height would fight a
	 * window the user is resizing. */
	layout = (np2oscfg.front_panel * 8) + (scrnmng.display_mode * 2) +
	         (scrnmng.native_active ? 1 : 0) + (np2oscfg.machine_status ? 64 : 0) +
	         gui_status_bar_height() * 128;
	if (layout != last_layout) {
		last_layout = layout;
		scrnmng_fit_front_panel();
	}
	if ((scrnmng_get_drawable_size(&output_w, &output_h) != SUCCESS) || (output_w <= 0)) {
		return;
	}
	panel.h = (scrnmng_visible_ui() && scrnmng.display_mode == VAEG_DISPLAY_WINDOWED)
	              ? frontpanel_height(output_w)
	              : 0;
	if (panel.h <= 0) {
		return;
	}
	panel.x = 0;
	panel.y = output_h - panel.h;
	panel.w = output_w;
	scrnmng_draw_simple_panel(&panel);
}

/* Selftest: the simple bar's lamps on a software renderer (drive 1 and V3
 * lit by the caller); out_path, if set, receives the picture as BMP. */
BOOL scrnmng_simple_panel_selftest(int width, const char *out_path, char *problem, size_t size) {
	SDL_Renderer *saved_renderer = scrnmng.renderer;
	const BOOL saved_native = scrnmng.native_active;
	SDL_Surface *surface;
	SDL_Renderer *renderer;
	SDL_Rect bar;
	const Uint32 *px;
	Uint8 r;
	Uint8 g;
	Uint8 b;
	const int s = frontpanel_simple_scale(width);
	BOOL ok = TRUE;

	bar.x = 0;
	bar.y = 0;
	bar.w = width;
	bar.h = 14 * s;
	surface = SDL_CreateRGBSurfaceWithFormat(0, bar.w, bar.h, 32, SDL_PIXELFORMAT_ARGB8888);
	renderer = (surface != NULL) ? SDL_CreateSoftwareRenderer(surface) : NULL;
	if (renderer == NULL) {
		SDL_snprintf(problem, size, "no software renderer");
		if (surface != NULL) {
			SDL_FreeSurface(surface);
		}
		return FAILURE;
	}
	scrnmng.renderer = renderer;
	scrnmng.native_active = FALSE;
	scrnmng_draw_simple_panel(&bar);
	SDL_RenderPresent(renderer);
	px = (const Uint32 *)surface->pixels;
	/* lamps start after their label: FD1 at 36, FD2 at 88, V1 at 148 (+16 gap) */
	SDL_GetRGB(px[(8 * s) * (surface->pitch / 4) + (8 + 24 + 4 + 6) * s], surface->format, &r, &g,
	           &b);
	if ((r + g + b) < 250) {
		SDL_snprintf(problem, size, "simple panel drive 1 lamp is not lit");
		ok = FALSE;
	}
	SDL_GetRGB(px[(8 * s) * (surface->pitch / 4) + (148 + 6) * s], surface->format, &r, &g, &b);
	if (ok && ((r + g + b) > 250)) {
		SDL_snprintf(problem, size, "simple panel V1 lamp is lit");
		ok = FALSE;
	}
	/* M109: the CAPS lamp ends 8 dots from the right edge (lit or not, it
	 * differs from the bar's background) */
	SDL_GetRGB(px[(8 * s) * (surface->pitch / 4) + (width - 14 * s)], surface->format, &r, &g, &b);
	if (ok && (r == 34) && (g == 34) && (b == 36)) {
		SDL_snprintf(problem, size, "simple panel CAPS lamp is not at the right edge");
		ok = FALSE;
	}
	/* REC is directly left of the lock lamps and dim while idle. */
	SDL_GetRGB(px[(8 * s) * (surface->pitch / 4) + (width - 190 * s)], surface->format, &r, &g, &b);
	if (ok && !vaeg_recording_native_session_is_active() && ((r != 52) || (g != 60) || (b != 52))) {
		SDL_snprintf(problem, size,
		             "simple panel REC lamp is not dim at its expected position");
		ok = FALSE;
	}
	/* Compare the real timer area against independently drawn expected glyphs;
	 * exercise start/reset and finalization without modifying session state. */
	{
		static const char *const expected[] = {"01:23", "00:00", "100:00", "01:23"};
		SDL_Rect timer = {width - 176 * s, 4 * s, 40 * s, 8 * s};
		SDL_Surface *actual =
		    SDL_CreateRGBSurfaceWithFormat(0, 48 * s, timer.h, 32, SDL_PIXELFORMAT_ARGB8888);
		int phase;

		if (actual == NULL) {
			SDL_snprintf(problem, size, "no timer comparison surface");
			ok = FALSE;
		}
		for (phase = 0; actual != NULL && phase < 4 && ok; ++phase) {
			const SDL_Color color = {200, 196, 180, 255};
			int row;

			timer.w = (int)SDL_strlen(expected[phase]) * 8 * s;
			timer.x = width - 136 * s - timer.w;
			scrnmng_draw_simple_panel_state(&bar, phase != 3, phase == 3,
			                                phase == 1 ? 0 : (phase == 2 ? 6000000 : 83000), 1000);
			SDL_RenderPresent(renderer);
			SDL_GetRGB(px[(8 * s) * (surface->pitch / 4) + timer.x - 14 * s], surface->format, &r,
			           &g, &b);
			if (r != 255 || g != (phase == 3 ? 180 : 45) || b != (phase == 3 ? 40 : 30)) {
				SDL_snprintf(problem, size, "recording timer/lamp layout overlap");
				ok = FALSE;
			}
			SDL_BlitSurface(surface, &timer, actual, NULL);
			scrnmng_panel_fill(&timer, 34, 34, 36);
			scrnmng_draw_text_glyphs(timer.x, timer.y, s, expected[phase], color);
			SDL_RenderPresent(renderer);
			for (row = 0; row < timer.h && ok; ++row) {
				if (memcmp((const BYTE *)actual->pixels + row * actual->pitch,
				           (const BYTE *)surface->pixels + (timer.y + row) * surface->pitch +
				               timer.x * 4,
				           (size_t)timer.w * 4) != 0) {
					SDL_snprintf(problem, size, "recording MM:SS pixels/reset mismatch");
					ok = FALSE;
				}
			}
		}
		SDL_FreeSurface(actual);
	}
	if (out_path != NULL) {
		SDL_SaveBMP(surface, out_path);
	}
	scrnmng.renderer = saved_renderer;
	scrnmng.native_active = saved_native;
	SDL_DestroyRenderer(renderer);
	SDL_FreeSurface(surface);
	return ok ? SUCCESS : FAILURE;
}

void scrnmng_front_panel_changed(void) {
	vaeg_recording_runtime_screen_changed();
	scrnmng_fit_front_panel();
}

void scrnmng_set_menu_height(int height) {
	if (height < 0) {
		height = 0;
	}
	if (scrnmng.menu_height == height) {
		return;
	}
	vaeg_recording_runtime_screen_changed();
	scrnmng.menu_height = height;
	if (scrnmng.visible) {
		scrnmng_log_geometry("menu-height");
	}
}

void scrnmng_set_display(int scale, BOOL aspect) {
	if (scale < 1) {
		scale = 1;
	} else if (scale > VAEG_WINDOW_SCALE_MAX) {
		scale = VAEG_WINDOW_SCALE_MAX;
	}
	if (scrnmng.scale != scale || scrnmng.aspect != (aspect ? TRUE : FALSE))
		vaeg_recording_runtime_screen_changed();
	scrnmng.scale = scale;
	scrnmng.aspect = aspect ? TRUE : FALSE;
	scrnmng_update_window_size();
	scrnmng_apply_native_mask_profile();
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
	if (scrnmng.scaling != scaling)
		vaeg_recording_runtime_screen_changed();
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
	if (scrnmng.effect != effect)
		vaeg_recording_runtime_screen_changed();
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

void scrnmng_set_rotation(int rotation) {
	if ((rotation < 0) || (rotation >= VAEG_ROTATE_COUNT)) {
		rotation = VAEG_ROTATE_NONE;
	}
	if (scrnmng.rotation == rotation) {
		return;
	}
	vaeg_recording_runtime_screen_changed();
	scrnmng.rotation = rotation;
	scrnmng_update_window_size();
	if (scrnmng.visible) {
		scrnmng_log_geometry("rotation-change");
	}
}

int scrnmng_get_rotation(void) {
	return (scrnmng.rotation);
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
	if (scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) {
		return (FAILURE);
	}
	if ((window_width <= 0) || (window_height <= 0)) {
		return (FAILURE);
	}
	drawable_x = (int)(((SINT64)window_x * output_width) / window_width);
	drawable_y = (int)(((SINT64)window_y * output_height) / window_height);
	return (vaeg_viewport_map_point_rotated(&viewport, scrnmng.width, scrnmng.height,
	                                        (scrnmng.rotation == VAEG_ROTATE_RIGHT) ? TRUE : FALSE,
	                                        drawable_x, drawable_y, guest_x, guest_y));
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
	/* Explicit display-mode/monitor requests change the presentation contract. */
	vaeg_recording_runtime_screen_changed();
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
	*height = max(240, *height - scrnmng_panel_applied); /* saved without the panel */
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
	if (scrnmng.shadow != NULL) {
		scrnmng.height = scrnmng_canvas_height(height);
	}
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
	BOOL turned;
	int row_span; /* screen pixels across the guest rows */
	int col_span; /* screen pixels across the guest columns */
	scrnmng.sdl_overlays_drawn = FALSE;
	scrnmng.osd_snapshot_valid = FALSE;

	if (!scrnmng.enable) {
		return;
	}
	if (scrnmng.native_active) {
		scrnmng.dirty = FALSE;
		return;
	}
	if ((scrnmng.renderer == NULL) || (scrnmng.texture == NULL)) {
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
	{
		SDL_Rect src;

		src.x = 0;
		src.y = 0;
		src.w = scrnmng.width;
		src.h = scrnmng.height;
		if (scrnmng.rotation == VAEG_ROTATE_RIGHT) {
			SDL_Rect landscape;

			/* the unturned rectangle with the viewport's centre */
			landscape.w = dst.h;
			landscape.h = dst.w;
			landscape.x = dst.x + ((dst.w - dst.h) / 2);
			landscape.y = dst.y + ((dst.h - dst.w) / 2);
			SDL_RenderCopyEx(scrnmng.renderer, scrnmng.texture, &src, &landscape, 90.0, NULL,
			                 SDL_FLIP_NONE);
		} else {
			SDL_RenderCopy(scrnmng.renderer, scrnmng.texture, &src, &dst);
		}
	}
	turned = (scrnmng.rotation == VAEG_ROTATE_RIGHT) ? TRUE : FALSE;
	row_span = turned ? dst.w : dst.h;
	col_span = turned ? dst.h : dst.w;
	scrnmng_draw_front_panel();
	SDL_RenderSetClipRect(scrnmng.renderer, &dst);
	if (((scrnmng.effect == VAEG_EFFECT_SCANLINE) || (scrnmng.effect == VAEG_EFFECT_CRT_LITE)) &&
	    (row_span >= scrnmng.height)) {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0,
		                       (scrnmng.effect == VAEG_EFFECT_CRT_LITE) ? 42 : 34);
		for (row = 1; row < scrnmng.height; row += 2) {
			int y0;
			int y1;
			SDL_Rect line;

			y0 = (row * row_span) / scrnmng.height;
			y1 = ((row + 1) * row_span) / scrnmng.height;
			if (y1 <= y0) {
				y1 = y0 + 1;
			}
			if (turned) {
				/* guest row 0 is the rightmost column */
				line.x = dst.x + dst.w - y1;
				line.y = dst.y;
				line.w = y1 - y0;
				line.h = dst.h;
			} else {
				line.x = dst.x;
				line.y = dst.y + y0;
				line.w = dst.w;
				line.h = y1 - y0;
			}
			SDL_RenderFillRect(scrnmng.renderer, &line);
		}
	}
	if (scrnmng.effect == VAEG_EFFECT_CRT_LITE) {
		if (col_span >= (scrnmng.width * 2)) {
			static const UINT8 stripe[3][3] = {{24, 2, 2}, {2, 24, 2}, {2, 2, 24}};

			/* stripes along the guest columns: horizontal once turned */
			SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_ADD);
			for (x = 0; x < col_span; x++) {
				SDL_SetRenderDrawColor(scrnmng.renderer, stripe[x % 3][0], stripe[x % 3][1],
				                       stripe[x % 3][2], 10);
				if (turned) {
					SDL_RenderDrawLine(scrnmng.renderer, dst.x, dst.y + x, dst.x + dst.w - 1,
					                   dst.y + x);
				} else {
					SDL_RenderDrawLine(scrnmng.renderer, dst.x + x, dst.y, dst.x + x,
					                   dst.y + dst.h - 1);
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

BOOL scrnmng_request_display_capture(const char *path) {
	if (!path || !path[0] || strlen(path) >= sizeof(scrnmng.display_capture_path) ||
	    scrnmng.display_capture_path[0]) {
		SDL_SetError("Invalid or already pending screenshot request");
		return FAILURE;
	}
	milstr_ncpy(scrnmng.display_capture_path, path, sizeof(scrnmng.display_capture_path));
	return SUCCESS;
}

BOOL scrnmng_prepare_display_capture(void) {
	scrnmng.display_capture_ready = scrnmng.display_capture_path[0] != '\0';
	return scrnmng.display_capture_ready;
}

static SDL_Surface *scrnmng_crop_display_capture(SDL_Surface *surface) {
	int window_width, window_height;
	if (surface) {
		SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
		const int inset = window_height > 0 ?
		    (int)(((SINT64)scrnmng.menu_height * surface->h + window_height / 2) / window_height) : 0;
		if (inset >= 0 && inset < surface->h) {
			return SDL_CreateRGBSurfaceWithFormatFrom(
			    (UINT8 *)surface->pixels + inset * surface->pitch, surface->w,
			    surface->h - inset, 32, surface->pitch, SDL_PIXELFORMAT_RGBA32);
		}
	}
	return NULL;
}

static void scrnmng_finish_display_capture(SDL_Surface *surface, BOOL captured) {
	BOOL saved = FAILURE;
	SDL_Surface *cropped = NULL;
	if (captured == SUCCESS && surface) {
		cropped = scrnmng_crop_display_capture(surface);
		if (cropped) saved = scrnmng_png_save_surface(cropped, scrnmng.display_capture_path);
	} else {
		SDL_SetError("Display readback failed or is unsupported by this native backend");
	}
	gui_display_capture_result(scrnmng.display_capture_path, saved);
	SDL_FreeSurface(cropped);
	SDL_FreeSurface(surface);
	scrnmng.display_capture_path[0] = '\0';
	scrnmng.display_capture_ready = FALSE;
}

static SDL_Surface *scrnmng_read_sdl_output(void) {
	int width, height;
	SDL_Surface *surface;
	if (SDL_GetRendererOutputSize(scrnmng.renderer, &width, &height) != 0) return NULL;
	surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (surface && SDL_RenderReadPixels(scrnmng.renderer, NULL, SDL_PIXELFORMAT_RGBA32,
	                                   surface->pixels, surface->pitch) != 0) {
		SDL_FreeSurface(surface);
		return NULL;
	}
	return surface;
}

typedef struct {
	BYTE *rgb;
	UINT32 count;
	UINT64 frame_token;
	UINT32 descriptor_generation;
	VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR error;
} SCRNMNG_DISPLAYED_CAPTURE_FIXTURE;

static void scrnmng_displayed_fixture_fill(BYTE *pixels, UINT32 width, UINT32 height,
	                                           UINT32 pitch, int x, int y, int w, int h,
	                                           BYTE red, BYTE green, BYTE blue) {
	int row;
	int column;

	for (row = y; row < y + h; ++row) {
		if ((row < 0) || ((UINT32)row >= height)) {
			continue;
		}
		for (column = x; column < x + w; ++column) {
			BYTE *pixel;
			if ((column < 0) || ((UINT32)column >= width)) {
				continue;
			}
			pixel = pixels + (size_t)row * pitch + (size_t)column * 3u;
			pixel[0] = red;
			pixel[1] = green;
			pixel[2] = blue;
		}
	}
}

static int scrnmng_displayed_fixture_callback(
	void *context, const VAEG_RECORDING_DISPLAYED_FRAME *capture) {
	SCRNMNG_DISPLAYED_CAPTURE_FIXTURE *fixture =
		(SCRNMNG_DISPLAYED_CAPTURE_FIXTURE *)context;

	if ((fixture == NULL) || (capture == NULL)) {
		return 0;
	}
	fixture->error = vaeg_recording_displayed_capture_to_rgb24(
		capture, fixture->rgb, capture->width * 3u,
		(UINT64)capture->width * capture->height * 3u);
	if (fixture->error != VAEG_RECORDING_DISPLAYED_CAPTURE_OK) {
		return 0;
	}
	fixture->count++;
	fixture->frame_token = capture->frame_token;
	fixture->descriptor_generation = capture->descriptor_generation;
	return 1;
}

static int scrnmng_displayed_fixture_reject(
	void *context, const VAEG_RECORDING_DISPLAYED_FRAME *capture) {
	(void)context;
	(void)capture;
	return 0;
}

static int scrnmng_displayed_fixture_stop(
	void *context, const VAEG_RECORDING_DISPLAYED_FRAME *capture) {
	const int accepted = scrnmng_displayed_fixture_callback(context, capture);
	scrnmng_stop_displayed_recording_capture();
	return accepted;
}

/* Public pixels and the actual owned-buffer/callback path, without claiming
 * that this software fixture exercised a physical native GPU. */
static BOOL scrnmng_displayed_native_boundary_selftest(
	SCRNMNG_DISPLAYED_CAPTURE_FIXTURE *fixture, const BYTE *expected) {
	const VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR errors[] = {
	    VAEG_RECORDING_DISPLAYED_CAPTURE_GEOMETRY_CHANGED,
	    VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED,
	    VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED,
	    VAEG_RECORDING_DISPLAYED_CAPTURE_READBACK_FAILED,
	    VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK_REJECTED,
	    VAEG_RECORDING_DISPLAYED_CAPTURE_OK};
	VAEG_OUTPUT_CAPTURE output;
	UINT mutation;
	BOOL ok = FAILURE;

	SDL_SetWindowSize(scrnmng.window, 640, 422);
	/* Recording off: no output pointer and no allocation. */
	if (scrnmng_prepare_displayed_recording_frame(&output) || output.pixels != NULL ||
	    scrnmng.displayed_capture_pixels != NULL) return FAILURE;
	for (mutation = 0; mutation < sizeof(errors) / sizeof(errors[0]); ++mutation) {
		scrnmng.native_active = TRUE;
		scrnmng.sdl_renderer_generation = 1u;
		scrnmng.displayed_capture_native = TRUE;
		scrnmng.displayed_capture_pixels = (BYTE *)malloc((size_t)640u * 422u * 4u);
		if (!scrnmng.displayed_capture_pixels) goto cleanup;
		scrnmng.displayed_capture_width = 640u;
		scrnmng.displayed_capture_height = 422u;
		scrnmng.displayed_capture_pitch_bytes = 640u * 4u;
		scrnmng.displayed_capture_size_bytes = (UINT64)640u * 422u * 4u;
		scrnmng.displayed_capture_generation = 1u;
		scrnmng.displayed_capture_expected_generation = 1u;
		scrnmng.displayed_capture_callback = scrnmng_displayed_fixture_callback;
		scrnmng.displayed_capture_context = fixture;
		scrnmng.displayed_capture_error = VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
		scrnmng.displayed_capture_active = TRUE;
		fixture->count = 0u;
		/* Every negative case first passes the same acquisition/content path. */
		if (!scrnmng_prepare_displayed_recording_frame(&output) ||
		    output.pixels != scrnmng.displayed_capture_pixels ||
		    SDL_ConvertPixels(640, 422, SDL_PIXELFORMAT_RGB24, expected, 640 * 3,
		                      SDL_PIXELFORMAT_RGBA32, output.pixels,
		                      (int)output.pitch_bytes) != 0) goto cleanup;
		output.complete = 1;
		if (scrnmng_finish_displayed_recording_frame(&output) != SUCCESS ||
		    scrnmng.displayed_capture_error != VAEG_RECORDING_DISPLAYED_CAPTURE_OK ||
		    fixture->count != 1u || fixture->descriptor_generation != 1u ||
		    fixture->frame_token != (UINT64)drawcount ||
		    memcmp(fixture->rgb, expected, (size_t)640u * 422u * 3u) != 0) goto cleanup;
		/* A redraw cannot ask the backend to capture the same guest token twice. */
		if (scrnmng_prepare_displayed_recording_frame(&output) || output.pixels != NULL ||
		    !scrnmng.displayed_capture_active || fixture->count != 1u) goto cleanup;
		drawcount++;
		if (!scrnmng_prepare_displayed_recording_frame(&output)) goto cleanup;
		output.complete = 1;
		switch (mutation) {
		case 0: scrnmng.displayed_capture_width++; break;
		case 1: scrnmng.sdl_renderer_generation++; break;
		case 2: scrnmng.native_active = FALSE; break;
		case 3: output.complete = 0; break;
		case 4: scrnmng.displayed_capture_callback = scrnmng_displayed_fixture_reject; break;
		case 5: scrnmng.displayed_capture_callback = scrnmng_displayed_fixture_stop; break;
		}
		if (mutation < 3u) {
			if (scrnmng_prepare_displayed_recording_frame(&output)) goto cleanup;
		} else if (scrnmng_finish_displayed_recording_frame(&output) !=
		           (mutation == 5u ? SUCCESS : FAILURE)) goto cleanup;
		if (scrnmng.displayed_capture_error != errors[mutation] ||
		    scrnmng.displayed_capture_active || scrnmng.displayed_capture_pixels != NULL ||
		    fixture->count != (mutation == 5u ? 2u : 1u)) {
			fprintf(stderr, "selftest: DISPLAYED_NATIVE_BOUNDARY_ERROR case=%u error=%s\n",
			        mutation, vaeg_recording_displayed_capture_error_name(
			                      scrnmng.displayed_capture_error));
			goto cleanup;
		}
	}
	ok = SUCCESS;
cleanup:
	scrnmng_displayed_capture_release();
	scrnmng.native_active = FALSE;
	return ok;
}

static BOOL scrnmng_fps_visibility_selftest(void) {
	const SCRNMNG saved = scrnmng;
	const VAEG_SPEEDMETER saved_speedmeter = speedmeter;
	const UINT8 saved_info = np2oscfg.DISPCLK;
	const UINT saved_drawcount = drawcount;
	const UINT8 modes[] = {VAEG_DISPINFO_FPS, VAEG_DISPINFO_FPS | VAEG_DISPINFO_FRAME,
	                       0, VAEG_DISPINFO_FRAME};
	BOOL result = SUCCESS;
	UINT mode;
	for (mode = 0; mode < sizeof(modes) / sizeof(modes[0]); ++mode) {
		const char *title;
		VAEG_MACHINE_STATUS status;
		char line[256];
		np2oscfg.DISPCLK = modes[mode];
		scrnmng_set_framedisp((modes[mode] & VAEG_DISPINFO_FRAME) != 0);
		vaeg_framedisp_reset(&scrnmng.framedisp, 0, 0);
		drawcount = 0;
		scrnmng_framedisp_tick(0, 0, 0);
		drawcount = 120;
		scrnmng_framedisp_tick(2000, 120, 120);
		title = SDL_GetWindowTitle(scrnmng.window);
		machine_status_get(&status);
		machine_status_line(line, sizeof(line), &status, modes[mode], scrnmng_draw_fps_tenths(),
		                    drawcount);
		if (scrnmng.framedisp.fps_tenths != 600 || strstr(title, "FPS") ||
		    strstr(title, "FRAME ") || strstr(title, "CPU ") ||
		    ((strstr(line, "60.0FPS") != NULL) != ((modes[mode] & VAEG_DISPINFO_FPS) != 0)) ||
		    ((strstr(line, "FRAME 120") != NULL) != ((modes[mode] & VAEG_DISPINFO_FRAME) != 0))) {
			fprintf(stderr, "selftest: FPS_DISPLAY_DEPENDS_ON_FRAME_VISIBILITY mode=%u\n",
			        (unsigned int)modes[mode]);
			result = FAILURE;
			break;
		}
	}
	scrnmng = saved;
	speedmeter = saved_speedmeter;
	np2oscfg.DISPCLK = saved_info;
	drawcount = saved_drawcount;
	scrnmng_update_title();
	return result;
}

BOOL scrnmng_display_capture_selftest(void) {
	const SCRNMNG saved = scrnmng;
	const UINT8 saved_info = np2oscfg.DISPCLK;
	const UINT saved_drawcount = drawcount;
	const SDL_LogPriority saved_log_priority = SDL_LogGetPriority(SDL_LOG_CATEGORY_APPLICATION);
	const BOOL video_active = SDL_WasInit(SDL_INIT_VIDEO) != 0;
	SDL_Surface *white = NULL, *baseline = NULL, *filtered = NULL, *info = NULL;
	BYTE *displayed_expected = NULL;
	SCRNMNG_DISPLAYED_CAPTURE_FIXTURE displayed_fixture;
	VAEG_VIEWPORT viewport;
	BOOL ok = FAILURE;
	ZeroMemory(&displayed_fixture, sizeof(displayed_fixture));
	if (!video_active && SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return FAILURE;
	/* This synthetic fixture must not inject macOS wall-clock/PID-prefixed
	 * screenshot INFO logs into deterministic CPU selftest checkpoints. */
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN);
	ZeroMemory(&scrnmng, sizeof(scrnmng));
	scrnmng.window = SDL_CreateWindow("display capture test", 0, 0, 640, 422, SDL_WINDOW_HIDDEN);
	if (!scrnmng.window) goto cleanup;
	if (scrnmng_fps_visibility_selftest() != SUCCESS) goto cleanup;
	scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_SOFTWARE);
	if (!scrnmng.renderer) goto cleanup;
	white = SDL_CreateRGBSurfaceWithFormat(0, 640, 400, 32, SDL_PIXELFORMAT_RGBA32);
	if (!white) goto cleanup;
	SDL_FillRect(white, NULL, SDL_MapRGBA(white->format, 255, 255, 255, 255));
	scrnmng.texture = SDL_CreateTextureFromSurface(scrnmng.renderer, white);
	if (!scrnmng.texture) goto cleanup;
	scrnmng.enable = TRUE;
	scrnmng.width = 640; scrnmng.height = 400;
	{
		const int saved_pitch = scrnmng.shadow_pitch;
		const UINT8 saved_crtmode = videova.crtmode;
		const WORD saved_grmode = videova.grmode;
		UINT32 generation15, generation24;
		scrnmng.shadow_pitch = (640 + SCRNMNG_SURFACE_GUARD_LEFT) * 2;
		videova.grmode &= ~0x0080;
		videova.crtmode = 0;
		generation15 = scrnmng_native_recording_descriptor_generation();
		videova.crtmode = 1;
		generation24 = scrnmng_native_recording_descriptor_generation();
		videova.crtmode = saved_crtmode;
		videova.grmode = saved_grmode;
		scrnmng.shadow_pitch = saved_pitch;
		if (generation15 == 0 || generation15 == generation24)
			goto cleanup;
	}
	scrnmng.menu_height = 22; scrnmng.scale = 1;
	scrnmng.scaling = VAEG_SCALING_FIT;
	np2oscfg.DISPCLK = 0;
	{
		static const BYTE colors[4][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}};
		static const int right[4] = {2, 0, 3, 1};
		SDL_Surface *cropped;
		int q;
		for (q = 0; q < 4; ++q) {
			SDL_Rect quad = {(q % 2) * 320, (q / 2) * 200, 320, 200};
			SDL_FillRect(white, &quad,
			             SDL_MapRGBA(white->format, colors[q][0], colors[q][1], colors[q][2], 255));
		}
		if (SDL_UpdateTexture(scrnmng.texture, NULL, white->pixels, white->pitch) != 0)
			goto cleanup;
		scrnmng.rotation = VAEG_ROTATE_RIGHT;
		scrnmng_present_begin();
		info = scrnmng_read_sdl_output();
		if (!info || scrnmng_calculate_viewport(&viewport) != SUCCESS)
			goto cleanup;
		cropped = scrnmng_crop_display_capture(info);
		if (!cropped)
			goto cleanup;
		for (q = 0; q < 4; ++q) {
			const int x = viewport.x + viewport.width * (q % 2 ? 3 : 1) / 4;
			const int y = viewport.y - scrnmng.menu_height + viewport.height * (q / 2 ? 3 : 1) / 4;
			if (cropped->w != 640 || cropped->h != 400 || x < 0 || y < 0 || x >= cropped->w ||
			    y >= cropped->h ||
			    memcmp((const BYTE *)cropped->pixels + (size_t)y * cropped->pitch + x * 4,
			           colors[right[q]], 3) != 0) {
				SDL_FreeSurface(cropped);
				fprintf(stderr, "selftest: SCREENSHOT_DISPLAY_ROTATION_MISMATCH\n");
				goto cleanup;
			}
		}
		SDL_FreeSurface(cropped);
		SDL_FreeSurface(info);
		info = NULL;
		scrnmng.rotation = VAEG_ROTATE_NONE;
		SDL_FillRect(white, NULL, SDL_MapRGBA(white->format, 255, 255, 255, 255));
		if (SDL_UpdateTexture(scrnmng.texture, NULL, white->pixels, white->pitch) != 0)
			goto cleanup;
	}
	scrnmng_present_begin();
	baseline = scrnmng_read_sdl_output();
	scrnmng.effect = VAEG_EFFECT_SCANLINE;
	scrnmng_present_begin();
	filtered = scrnmng_read_sdl_output();
	if (!baseline || !filtered || scrnmng_calculate_viewport(&viewport) != SUCCESS) goto cleanup;
	info = scrnmng_crop_display_capture(baseline);
	if (!info || info->w != 640 || info->h != 400 || ((BYTE *)info->pixels)[0] != 255) goto cleanup;
	SDL_FreeSurface(info); info = NULL;
	scrnmng.menu_height = 0;
	info = scrnmng_crop_display_capture(baseline);
	if (!info || info->h != 422 || info->pixels != baseline->pixels) goto cleanup;
	SDL_FreeSurface(info); info = NULL;
	scrnmng.menu_height = 22;
	if (((BYTE *)baseline->pixels)[23 * baseline->pitch] != 255 ||
	    ((BYTE *)filtered->pixels)[23 * filtered->pitch] >= 255) goto cleanup;
	np2oscfg.DISPCLK = VAEG_DISPINFO_VIDEO | VAEG_DISPINFO_FRAMEBUFFER;
	scrnmng_draw_sdl_overlays();
	info = scrnmng_read_sdl_output();
	if (!info || memcmp(info->pixels, filtered->pixels, info->pitch * info->h) == 0) goto cleanup;
	{
		SDL_Surface *twice;
		scrnmng_draw_sdl_overlays();
		twice = scrnmng_read_sdl_output();
		const BOOL matches = twice && memcmp(twice->pixels, info->pixels, info->pitch * info->h) == 0;
		SDL_FreeSurface(twice);
		if (!matches) goto cleanup;
	}
	SDL_FreeSurface(info); info = NULL;
	np2oscfg.DISPCLK = 0;
	scrnmng_present_begin();
	scrnmng_draw_video_info_overlay(&viewport);
	scrnmng_draw_framebuffer_info_overlay(&viewport);
	info = scrnmng_read_sdl_output();
	if (!info || memcmp(info->pixels, filtered->pixels, info->pitch * info->h) != 0) goto cleanup;
	if (scrnmng_request_display_capture("test.png") != SUCCESS ||
	    scrnmng.display_capture_ready || !scrnmng_prepare_display_capture()) goto cleanup;
	if (scrnmng_request_display_capture("second.png") != FAILURE) goto cleanup;
	{
		const UINT8 flags[] = {0, VAEG_DISPINFO_VIDEO, VAEG_DISPINFO_FRAMEBUFFER,
		                       VAEG_DISPINFO_VIDEO | VAEG_DISPINFO_FRAMEBUFFER};
		UINT32 hashes[4] = {0};
		int mode, other;
		for (mode = 0; mode < 4; ++mode) {
			SDL_Surface *raw = SDL_ConvertSurface(white, white->format, 0);
			size_t byte;
			if (!raw) goto cleanup;
			np2oscfg.DISPCLK = flags[mode];
			if (scrnmng_draw_graphics_analysis(raw) != SUCCESS) {
				SDL_FreeSurface(raw); goto cleanup;
			}
			if (mode == 0 && memcmp(raw->pixels, white->pixels, raw->pitch * raw->h) != 0) {
				SDL_FreeSurface(raw); goto cleanup;
			}
			for (byte = 0; byte < (size_t)raw->pitch * raw->h; ++byte)
				hashes[mode] = hashes[mode] * 33U + ((BYTE *)raw->pixels)[byte];
			SDL_FreeSurface(raw);
			for (other = 0; other < mode; ++other)
				if (hashes[mode] == hashes[other]) goto cleanup;
		}
	}
	{
		const int requested[] = {1, 4, 5, VAEG_WINDOW_SCALE_MAX, VAEG_WINDOW_SCALE_MAX + 1, 0};
		const int expected[] = {1, 4, 5, VAEG_WINDOW_SCALE_MAX, VAEG_WINDOW_SCALE_MAX, 1};
		unsigned i;
		/* Small synthetic source avoids allocating enormous test windows. */
		scrnmng.width = 32;
		scrnmng.height = 20;
		scrnmng.display_mode = VAEG_DISPLAY_WINDOWED;
		for (i = 0; i < sizeof(requested) / sizeof(requested[0]); ++i) {
			int width, height;
			scrnmng_set_display(requested[i], TRUE);
			SDL_GetWindowSize(scrnmng.window, &width, &height);
			if (scrnmng_get_display_scale() != expected[i] ||
			    width != 32 * expected[i] || height != 20 * expected[i] + scrnmng.menu_height) {
				fprintf(stderr, "selftest: WINDOW_MULTIPLIER_MISMATCH\n");
				goto cleanup;
			}
		}
	}
	/*
	 * Fixture-only final-client target: the public start function rejects the
	 * dummy driver, so arm the internal seam directly after constructing a
	 * real SDL software render target. This exercises acquisition and ownership
	 * without presenting the dummy driver as platform evidence.
	 */
	scrnmng.width = 640;
	scrnmng.height = 400;
	scrnmng.menu_height = 22;
	scrnmng.display_mode = VAEG_DISPLAY_WINDOWED;
	scrnmng.scale = 1;
	scrnmng_set_display(1, FALSE);
	scrnmng.effect = VAEG_EFFECT_UNFILTERED;
	np2oscfg.DISPCLK = 0;
	scrnmng_present_begin();
	SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(scrnmng.renderer, 255, 255, 255, 255);
	{
		SDL_Rect guest = {0, 22, 640, 400};
		SDL_RenderFillRect(scrnmng.renderer, &guest);
	}
	SDL_SetRenderDrawColor(scrnmng.renderer, 17, 34, 51, 255);
	{
		SDL_Rect menu = {0, 0, 640, 22};
		SDL_RenderFillRect(scrnmng.renderer, &menu);
	}
	SDL_SetRenderDrawColor(scrnmng.renderer, 170, 16, 32, 255);
	{
		SDL_Rect osd = {8, 30, 96, 16};
		SDL_RenderFillRect(scrnmng.renderer, &osd);
	}
	SDL_SetRenderDrawColor(scrnmng.renderer, 32, 204, 64, 255);
	{
		SDL_Rect status = {500, 390, 120, 20};
		SDL_RenderFillRect(scrnmng.renderer, &status);
	}
	displayed_expected = (BYTE *)malloc((size_t)640u * 422u * 3u);
	displayed_fixture.rgb = (BYTE *)malloc((size_t)640u * 422u * 3u);
	if ((displayed_expected == NULL) || (displayed_fixture.rgb == NULL)) goto cleanup;
	memset(displayed_expected, 0, (size_t)640u * 422u * 3u);
	for (int row = 22; row < 422; ++row) {
		memset(displayed_expected + (size_t)row * 640u * 3u, 255, 640u * 3u);
	}
	scrnmng_displayed_fixture_fill(displayed_expected, 640u, 422u, 640u * 3u,
	                                0, 0, 640, 22, 17, 34, 51);
	scrnmng_displayed_fixture_fill(displayed_expected, 640u, 422u, 640u * 3u,
	                                8, 30, 96, 16, 170, 16, 32);
	scrnmng_displayed_fixture_fill(displayed_expected, 640u, 422u, 640u * 3u,
	                                500, 390, 120, 20, 32, 204, 64);
	displayed_fixture.error = VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
	scrnmng.sdl_renderer_generation = 1u;
	scrnmng.displayed_capture_callback = scrnmng_displayed_fixture_callback;
	scrnmng.displayed_capture_context = &displayed_fixture;
	scrnmng.displayed_capture_pixels = (BYTE *)malloc((size_t)640u * 422u * 4u);
	if (scrnmng.displayed_capture_pixels == NULL) goto cleanup;
	scrnmng.displayed_capture_size_bytes = (UINT64)640u * 422u * 4u;
	scrnmng.displayed_capture_pitch_bytes = 640u * 4u;
	scrnmng.displayed_capture_width = 640u;
	scrnmng.displayed_capture_height = 422u;
	scrnmng.displayed_capture_generation = 1u;
	scrnmng.displayed_capture_expected_generation = 1u;
	scrnmng.displayed_capture_last_token = (UINT64)drawcount;
	scrnmng.displayed_capture_have_token = TRUE;
	scrnmng.displayed_capture_active = TRUE;
	drawcount++;
	scrnmng_present_end();
	if (!scrnmng.displayed_capture_active || (displayed_fixture.count != 1u) ||
	    (displayed_fixture.frame_token != (UINT64)drawcount) ||
	    (displayed_fixture.descriptor_generation != 1u)) {
		fprintf(stderr, "selftest: DISPLAYED_CAPTURE_METADATA_MISMATCH active=%d count=%u "
		                "token=%llu expected=%u generation=%u error=%s\n",
		        scrnmng.displayed_capture_active, displayed_fixture.count,
		        (unsigned long long)displayed_fixture.frame_token, drawcount,
		        displayed_fixture.descriptor_generation,
		        vaeg_recording_displayed_capture_error_name(displayed_fixture.error));
		goto cleanup;
	}
	if (memcmp(displayed_fixture.rgb, displayed_expected, (size_t)640u * 422u * 3u) != 0) {
		size_t byte;
		for (byte = 0; byte < (size_t)640u * 422u * 3u; ++byte) {
			if (displayed_fixture.rgb[byte] != displayed_expected[byte]) {
				fprintf(stderr, "selftest: DISPLAYED_CAPTURE_PIXEL_MISMATCH byte=%zu got=%u expected=%u\n",
				        byte, displayed_fixture.rgb[byte], displayed_expected[byte]);
				break;
			}
		}
		goto cleanup;
	}
	/* A host-only redraw with the same guest token must not create a frame. */
	scrnmng_present_begin();
	SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(scrnmng.renderer, 99, 88, 77, 255);
	{
		SDL_Rect menu = {0, 0, 640, 22};
		SDL_RenderFillRect(scrnmng.renderer, &menu);
	}
	scrnmng_present_end();
	if (scrnmng.displayed_capture_active && (displayed_fixture.count != 1u)) goto cleanup;
	/* A changed final-client geometry ends the fixed-descriptor capture. */
	SDL_SetWindowSize(scrnmng.window, 800, 500);
	drawcount++;
	scrnmng_present_begin();
	scrnmng_present_end();
	if (scrnmng.displayed_capture_active ||
	    (scrnmng.displayed_capture_error != VAEG_RECORDING_DISPLAYED_CAPTURE_GEOMETRY_CHANGED)) {
		goto cleanup;
	}
	if (scrnmng_displayed_native_boundary_selftest(&displayed_fixture,
	                                               displayed_expected) != SUCCESS) goto cleanup;
	if ((SDL_GetCurrentVideoDriver() != NULL) &&
	    !strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
		displayed_fixture.error = VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
		if (scrnmng_start_displayed_recording_capture(scrnmng_displayed_fixture_callback,
		                                               &displayed_fixture) != FAILURE ||
		    scrnmng.displayed_capture_error != VAEG_RECORDING_DISPLAYED_CAPTURE_DUMMY_DRIVER) {
			goto cleanup;
		}
	}
	ok = SUCCESS;
cleanup:
	if (ok == SUCCESS) {
		fprintf(stderr, "selftest: displayed capture fixture ok (video=%s)\n",
		        SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "unknown");
	}
	scrnmng_displayed_capture_release();
	free(displayed_fixture.rgb);
	free(displayed_expected);
	SDL_FreeSurface(info); SDL_FreeSurface(filtered); SDL_FreeSurface(baseline);
	SDL_FreeSurface(white);
	SDL_DestroyTexture(scrnmng.texture);
	SDL_DestroyRenderer(scrnmng.renderer);
	SDL_DestroyWindow(scrnmng.window);
	scrnmng = saved;
	np2oscfg.DISPCLK = saved_info;
	drawcount = saved_drawcount;
	if (!video_active) SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, saved_log_priority);
	if (ok != SUCCESS) {
		fprintf(stderr, "selftest: DISPLAY_CAPTURE_MISMATCH: %s\n", SDL_GetError());
	}
	return ok;
}

void scrnmng_draw_sdl_overlays(void) {
	VAEG_VIEWPORT viewport;
	if (scrnmng.native_active || !scrnmng.renderer || scrnmng.sdl_overlays_drawn) return;
	if (scrnmng_calculate_viewport(&viewport) == SUCCESS) {
		scrnmng_draw_video_info_overlay(&viewport);
		scrnmng_draw_framebuffer_info_overlay(&viewport);
	}
	scrnmng.sdl_overlays_drawn = TRUE;
}

void scrnmng_present_end(void) {
	VAEG_VIEWPORT viewport;
	VAEG_FRAME_INPUT frame;
	VAEG_NATIVE_PRESENTER_RESULT native_result;
	int drawable_width;
	int drawable_height;

	if (!scrnmng.enable) {
		return;
	}
	if (scrnmng.native_active) {
		if (scrnmng.rendered_capture_enabled && !scrnmng.native_capture_warning) {
			fprintf(stderr,
			        "Warning: rendered capture is unavailable while Native CRT owns the output; "
			        "use guest-frame capture instead\n");
			scrnmng.native_capture_warning = TRUE;
		}
		if (scrnmng_get_drawable_size(&drawable_width, &drawable_height) != SUCCESS) {
			if (scrnmng.displayed_capture_active)
				scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_GEOMETRY_CHANGED);
			return;
		}
		native_result = vaeg_native_presenter_resize(
		    scrnmng.native_presenter, (UINT32)drawable_width, (UINT32)drawable_height);
		if (native_result == VAEG_NATIVE_PRESENTER_NO_OUTPUT) {
			if (scrnmng.displayed_capture_active)
				scrnmng_displayed_capture_fail(VAEG_RECORDING_DISPLAYED_CAPTURE_READBACK_FAILED);
			return;
		}
		if (native_result == VAEG_NATIVE_PRESENTER_FALLBACK) {
			(void)scrnmng_native_fallback();
		} else {
			if (scrnmng_calculate_viewport(&viewport) == SUCCESS) {
				vaeg_native_presenter_set_output_viewport(scrnmng.native_presenter, viewport.x,
				                                          viewport.y, viewport.width,
				                                          viewport.height);
			}
			if (scrnmng.rotation == VAEG_ROTATE_RIGHT) {
				/* the shaders get the turned picture (height x width) */
				vaeg_frame_input_initialize(
				    &frame, scrnmng_turn_shadow(), scrnmng.height, scrnmng.width,
				    (UINT32)scrnmng.height * 2, VAEG_FRAME_PIXEL_RGB565, VAEG_FRAME_ROWS_TOP_DOWN,
				    scrnmng.aspect ? 3U : (UINT32)scrnmng.height,
				    scrnmng.aspect ? 4U : (UINT32)scrnmng.width, vaeg_nominal_frame_rate, 1U,
				    scrnmng.native_frame_number++, 16666667U);
			} else {
				vaeg_frame_input_initialize(
				    &frame, scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2), scrnmng.width,
				    scrnmng.height, (UINT32)scrnmng.shadow_pitch, VAEG_FRAME_PIXEL_RGB565,
				    VAEG_FRAME_ROWS_TOP_DOWN, scrnmng.aspect ? 4U : (UINT32)scrnmng.width,
				    scrnmng.aspect ? 3U : (UINT32)scrnmng.height, vaeg_nominal_frame_rate, 1U,
				    scrnmng.native_frame_number++, 16666667U);
			}
			{
				VAEG_OUTPUT_CAPTURE capture;
				const BOOL recording_readback =
				    scrnmng_prepare_displayed_recording_frame(&capture);
				SDL_Surface *surface = NULL;
				if (scrnmng.display_capture_ready) {
					surface = SDL_CreateRGBSurfaceWithFormat(
					    0, drawable_width, drawable_height, 32, SDL_PIXELFORMAT_RGBA32);
					if (surface && !recording_readback) {
						capture.pixels = surface->pixels;
						capture.width = surface->w;
						capture.height = surface->h;
						capture.pitch_bytes = surface->pitch;
					}
				}
				/* Exactly one live presentation and one optional readback, even
				 * when a screenshot and recording request the same final image. */
				vaeg_native_presenter_set_output_capture(
				    scrnmng.native_presenter, capture.pixels ? &capture : NULL);
				native_result = vaeg_native_presenter_present(scrnmng.native_presenter, &frame);
				vaeg_native_presenter_set_output_capture(scrnmng.native_presenter, NULL);
				if (scrnmng.display_capture_ready) {
					BOOL copied = capture.complete ? SUCCESS : FAILURE;
					if (surface && recording_readback && capture.complete) {
						copied = SDL_ConvertPixels(
						    surface->w, surface->h, SDL_PIXELFORMAT_RGBA32, capture.pixels,
						    (int)capture.pitch_bytes, SDL_PIXELFORMAT_RGBA32, surface->pixels,
						    surface->pitch) == 0 ? SUCCESS : FAILURE;
					}
					/* Finish the screenshot before a callback may release recording pixels. */
					scrnmng_finish_display_capture(surface, copied);
				}
				if (recording_readback) {
					/* An archive cannot silently skip a guest image when a
					 * drawable is unavailable. Incomplete readback fails closed. */
					(void)scrnmng_finish_displayed_recording_frame(&capture);
				}
			}
			if (native_result == VAEG_NATIVE_PRESENTER_NO_OUTPUT) {
				return;
			}
			if (native_result == VAEG_NATIVE_PRESENTER_FALLBACK) {
				(void)scrnmng_native_fallback();
			}
		}
		if (!scrnmng.native_active && (scrnmng.renderer != NULL)) {
			scrnmng_present_begin();
			scrnmng_present_end();
		}
		return;
	}
	if (scrnmng.renderer == NULL) {
		if (scrnmng.displayed_capture_active) {
			scrnmng_displayed_capture_fail(
			    VAEG_RECORDING_DISPLAYED_CAPTURE_UNAVAILABLE);
		}
		return;
	}
	scrnmng_draw_sdl_overlays();
	if (scrnmng.rendered_capture_enabled) {
		scrnmng_capture_rendered_frame();
	}
	if (scrnmng.display_capture_ready) {
		SDL_Surface *surface = scrnmng_read_sdl_output();
		scrnmng_finish_display_capture(surface, surface ? SUCCESS : FAILURE);
	}
	(void)scrnmng_capture_displayed_recording_frame();
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
	if (scrnmng_path_is_png(path)) {
		if (scrnmng_png_save_surface(scrnmng.rendered_frame, path) != SUCCESS) {
			fprintf(stderr, "scsitrace rendered-screen-save-failed path=%s error=%s\n", path,
			        SDL_GetError());
			return (FAILURE);
		}
		return (SUCCESS);
	}
	if (SDL_SaveBMP(scrnmng.rendered_frame, path) != 0) {
		fprintf(stderr, "scsitrace rendered-screen-save-failed path=%s error=%s\n", path,
		        SDL_GetError());
		return (FAILURE);
	}
	return (SUCCESS);
}

/* Takes ownership. Display readbacks are already oriented; use this only for
 * screenshots composed from the unturned guest framebuffer. */
static SDL_Surface *scrnmng_orient_guest_screenshot(SDL_Surface *source) {
	SDL_Surface *turned;
	int x, y;
	const int bytes = source ? source->format->BytesPerPixel : 0;
	if (!source || scrnmng.rotation != VAEG_ROTATE_RIGHT)
		return source;
	turned = SDL_CreateRGBSurfaceWithFormat(0, source->h, source->w, source->format->BitsPerPixel,
	                                        source->format->format);
	if (!turned) {
		SDL_FreeSurface(source);
		return NULL;
	}
	if (SDL_LockSurface(source) != 0) {
		SDL_FreeSurface(turned);
		SDL_FreeSurface(source);
		return NULL;
	}
	if (SDL_LockSurface(turned) != 0) {
		SDL_UnlockSurface(source);
		SDL_FreeSurface(turned);
		SDL_FreeSurface(source);
		return NULL;
	}
	for (y = 0; y < source->h; ++y) {
		for (x = 0; x < source->w; ++x) {
			memcpy((BYTE *)turned->pixels + (size_t)x * turned->pitch +
			           (size_t)(source->h - 1 - y) * bytes,
			       (const BYTE *)source->pixels + (size_t)y * source->pitch + (size_t)x * bytes,
			       bytes);
		}
	}
	SDL_UnlockSurface(turned);
	SDL_UnlockSurface(source);
	SDL_FreeSurface(source);
	return turned;
}

BOOL scrnmng_save_guest_frame(const char *path) {
	SDL_Surface *surface;
	BOOL result;

	if ((path == NULL) || (path[0] == '\0') || (scrnmng.shadow == NULL)) {
		return (FAILURE);
	}
	surface = SDL_CreateRGBSurfaceWithFormatFrom(scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                                             scrnmng.width, scrnmng.height, 16,
	                                             scrnmng.shadow_pitch, SDL_PIXELFORMAT_RGB565);
	if (surface == NULL) {
		fprintf(stderr, "guest-screen-save-source-failed path=%s error=%s\n", path, SDL_GetError());
		return (FAILURE);
	}
	surface = scrnmng_orient_guest_screenshot(surface);
	if (!surface)
		return FAILURE;
	if (scrnmng_path_is_png(path)) {
		result = scrnmng_png_save_surface(surface, path);
	} else {
		result = (SDL_SaveBMP(surface, path) == 0) ? SUCCESS : FAILURE;
	}
	if (result != SUCCESS) {
		fprintf(stderr, "guest-screen-save-failed path=%s error=%s\n", path, SDL_GetError());
	}
	SDL_FreeSurface(surface);
	return (result);
}

/*
 * M110: a thumbnail of the guest frame (no host effects) for a saved state,
 * `width` pixels wide with the frame's aspect, written as PNG.
 */
/* The state's thumbnail, WIDTH dots wide; turned as the screen is shown
 * (M141: a turned screen's quick saves had sideways thumbnails). */
BOOL scrnmng_save_guest_thumbnail(const char *path, int width) {
	SDL_Surface *source;
	SDL_Surface *full;
	SDL_Surface *thumb;
	SDL_Rect rect;
	BOOL result = FAILURE;
	int w;
	int h;

	if ((path == NULL) || (path[0] == '\0') || (scrnmng.shadow == NULL) || (width <= 0) ||
	    (scrnmng.width <= 0) || (scrnmng.height <= 0)) {
		return (FAILURE);
	}
	if (scrnmng.rotation == VAEG_ROTATE_RIGHT) {
		w = scrnmng.height;
		h = scrnmng.width;
		source = SDL_CreateRGBSurfaceWithFormatFrom((void *)scrnmng_turn_shadow(), w, h, 16, w * 2,
		                                            SDL_PIXELFORMAT_RGB565);
	} else {
		w = scrnmng.width;
		h = scrnmng.height;
		source = SDL_CreateRGBSurfaceWithFormatFrom(
		    scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2), w, h, 16, scrnmng.shadow_pitch,
		    SDL_PIXELFORMAT_RGB565);
	}
	full = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
	rect.x = 0;
	rect.y = 0;
	rect.w = width;
	rect.h = (h * width + w / 2) / w;
	if (rect.h <= 0) {
		rect.h = 1;
	}
	thumb = SDL_CreateRGBSurfaceWithFormat(0, rect.w, rect.h, 32, SDL_PIXELFORMAT_ARGB8888);
	if ((source != NULL) && (full != NULL) && (thumb != NULL) &&
	    (SDL_BlitSurface(source, NULL, full, NULL) == 0) &&
	    (SDL_BlitScaled(full, NULL, thumb, &rect) == 0)) {
		result = scrnmng_png_save_surface(thumb, path);
	}
	if (thumb != NULL) {
		SDL_FreeSurface(thumb);
	}
	if (full != NULL) {
		SDL_FreeSurface(full);
	}
	if (source != NULL) {
		SDL_FreeSurface(source);
	}
	return (result);
}

static UINT32 scrnmng_png_be32(const BYTE *p) {
	return ((UINT32)p[0] << 24) | ((UINT32)p[1] << 16) | ((UINT32)p[2] << 8) | p[3];
}

/*
 * M110: read back a PNG as scrnmng_png_save_surface writes it (8-bit RGBA,
 * stored deflate blocks, filter 0 on every row), for state thumbnails. Any
 * other PNG is refused. *pixels is RGBA32, freed by the caller with free().
 */
BOOL scrnmng_load_stored_png(const char *path, int *width, int *height, BYTE **pixels) {
	FILE *fp;
	long length;
	BYTE *data = NULL;
	BYTE *zdata = NULL;
	BYTE *raw = NULL;
	BYTE *out = NULL;
	size_t zlen = 0;
	size_t pos;
	size_t rawlen = 0;
	size_t row_bytes;
	UINT32 w = 0;
	UINT32 h = 0;
	UINT32 y;
	BOOL result = FAILURE;
	static const BYTE signature[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};

	if ((path == NULL) || (width == NULL) || (height == NULL) || (pixels == NULL)) {
		return (FAILURE);
	}
	fp = scrnmng_fopen(path, FALSE);
	if (fp == NULL) {
		return (FAILURE);
	}
	if ((fseek(fp, 0, SEEK_END) != 0) || ((length = ftell(fp)) < 33) || (length > (16 << 20)) ||
	    (fseek(fp, 0, SEEK_SET) != 0) || ((data = (BYTE *)malloc((size_t)length)) == NULL) ||
	    (fread(data, 1, (size_t)length, fp) != (size_t)length) || memcmp(data, signature, 8)) {
		goto cleanup;
	}
	zdata = (BYTE *)malloc((size_t)length);
	if (zdata == NULL) {
		goto cleanup;
	}
	for (pos = 8; pos + 12 <= (size_t)length;) {
		const UINT32 chunk = scrnmng_png_be32(data + pos);
		const BYTE *type = data + pos + 4;

		if (chunk > (size_t)length - pos - 12) {
			goto cleanup;
		}
		if (!memcmp(type, "IHDR", 4)) {
			if ((chunk != 13) || (data[pos + 16] != 8) || (data[pos + 17] != 6) || data[pos + 18] ||
			    data[pos + 19] || data[pos + 20]) {
				goto cleanup;
			}
			w = scrnmng_png_be32(data + pos + 8);
			h = scrnmng_png_be32(data + pos + 12);
		} else if (!memcmp(type, "IDAT", 4)) {
			memcpy(zdata + zlen, data + pos + 8, chunk);
			zlen += chunk;
		} else if (!memcmp(type, "IEND", 4)) {
			break;
		}
		pos += (size_t)chunk + 12;
	}
	if ((w == 0) || (h == 0) || (w > 4096) || (h > 4096) || (zlen < 2) ||
	    ((zdata[0] & 0x0f) != 8)) {
		goto cleanup;
	}
	row_bytes = (size_t)w * 4 + 1;
	raw = (BYTE *)malloc(row_bytes * h);
	if (raw == NULL) {
		goto cleanup;
	}
	for (pos = 2; pos < zlen;) {
		const BYTE header = zdata[pos];
		size_t block;

		if (((header >> 1) & 3) != 0 || (pos + 5 > zlen)) {
			goto cleanup;
		}
		block = zdata[pos + 1] | ((size_t)zdata[pos + 2] << 8);
		if ((block != (size_t)(~(zdata[pos + 3] | (zdata[pos + 4] << 8)) & 0xffff)) ||
		    (pos + 5 + block > zlen) || (rawlen + block > row_bytes * h)) {
			goto cleanup;
		}
		memcpy(raw + rawlen, zdata + pos + 5, block);
		rawlen += block;
		pos += 5 + block;
		if (header & 1) {
			break;
		}
	}
	if (rawlen != row_bytes * h) {
		goto cleanup;
	}
	out = (BYTE *)malloc((size_t)w * 4 * h);
	if (out == NULL) {
		goto cleanup;
	}
	for (y = 0; y < h; y++) {
		if (raw[y * row_bytes] != 0) {
			goto cleanup;
		}
		memcpy(out + (size_t)y * w * 4, raw + y * row_bytes + 1, (size_t)w * 4);
	}
	*width = (int)w;
	*height = (int)h;
	*pixels = out;
	out = NULL;
	result = SUCCESS;

cleanup:
	fclose(fp);
	free(data);
	free(zdata);
	free(raw);
	free(out);
	return (result);
}

/* M110: a state thumbnail written by scrnmng_png_save_surface reads back
 * pixel for pixel; a PNG with a compressed block is refused. */
BOOL scrnmng_thumbnail_png_selftest(const char *path) {
	SDL_Surface *surface;
	BYTE *pixels = NULL;
	int width = 0;
	int height = 0;
	int x;
	int y;
	BOOL ok;
	FILE *fp;
	static const BYTE fixed_block[] = {
	    0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a, 0, 0, 0, 13, 'I', 'H', 'D', 'R', 0, 0, 0, 1, 0,
	    0, 0, 1, 8, 6, 0, 0, 0, 0, 0, 0, 0,
	    /* a final fixed-Huffman block whose bytes would
	                                    * also pass as a stored block of one row */
	    0, 0, 0, 16, 'I', 'D', 'A', 'T', 0x78, 0x01, 0x03, 0x05, 0x00, 0xfa, 0xff, 0x00, 0x11, 0x22,
	    0x33, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'I', 'E', 'N', 'D', 0, 0, 0, 0};

	surface = SDL_CreateRGBSurfaceWithFormat(0, 37, 5, 32, SDL_PIXELFORMAT_ARGB8888);
	if (surface == NULL) {
		return (FAILURE);
	}
	for (y = 0; y < 5; y++) {
		for (x = 0; x < 37; x++) {
			((Uint32 *)((BYTE *)surface->pixels + y * surface->pitch))[x] =
			    SDL_MapRGBA(surface->format, (Uint8)(x * 7), (Uint8)(y * 50), (Uint8)(x ^ y), 255);
		}
	}
	ok = (scrnmng_png_save_surface(surface, path) == SUCCESS) &&
	     (scrnmng_load_stored_png(path, &width, &height, &pixels) == SUCCESS) && (width == 37) &&
	     (height == 5);
	for (y = 0; ok && (y < 5); y++) {
		for (x = 0; ok && (x < 37); x++) {
			const BYTE *p = pixels + ((size_t)y * 37 + x) * 4;
			ok = (p[0] == (BYTE)(x * 7)) && (p[1] == (BYTE)(y * 50)) && (p[2] == (BYTE)(x ^ y)) &&
			     (p[3] == 255);
		}
	}
	free(pixels);
	pixels = NULL;
	SDL_FreeSurface(surface);
	fp = scrnmng_fopen(path, TRUE);
	if (fp != NULL) {
		fwrite(fixed_block, 1, sizeof(fixed_block), fp);
		fclose(fp);
		if (scrnmng_load_stored_png(path, &width, &height, &pixels) == SUCCESS) {
			free(pixels);
			ok = FALSE;
		}
	} else {
		ok = FALSE;
	}
	(void)remove(path);
	return (ok ? SUCCESS : FAILURE);
}

BOOL scrnmng_save_guest_frame_with_analysis(const char *path) {
	SDL_Surface *source;
	SDL_Surface *surface;
	BOOL result;

	if ((path == NULL) || (path[0] == '\0') || (scrnmng.shadow == NULL)) {
		return (FAILURE);
	}
	source = SDL_CreateRGBSurfaceWithFormatFrom(scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                                            scrnmng.width, scrnmng.height, 16,
	                                            scrnmng.shadow_pitch, SDL_PIXELFORMAT_RGB565);
	if (source == NULL) {
		fprintf(stderr, "guest-screen-analysis-source-failed path=%s error=%s\n", path,
		        SDL_GetError());
		return (FAILURE);
	}
	surface = SDL_CreateRGBSurfaceWithFormat(0, scrnmng.width, scrnmng.height, 32,
	                                         SDL_PIXELFORMAT_ARGB8888);
	if (surface == NULL) {
		fprintf(stderr, "guest-screen-analysis-surface-failed path=%s error=%s\n", path,
		        SDL_GetError());
		SDL_FreeSurface(source);
		return (FAILURE);
	}
	if ((SDL_BlitSurface(source, NULL, surface, NULL) != 0) ||
	    (scrnmng_draw_graphics_analysis(surface) != SUCCESS)) {
		fprintf(stderr, "guest-screen-analysis-render-failed path=%s error=%s\n", path,
		        SDL_GetError());
		SDL_FreeSurface(surface);
		SDL_FreeSurface(source);
		return (FAILURE);
	}
	/* Rotate the complete guest/analysis composition exactly once. */
	surface = scrnmng_orient_guest_screenshot(surface);
	if (!surface) {
		SDL_FreeSurface(source);
		return FAILURE;
	}
	if (scrnmng_path_is_png(path)) {
		result = scrnmng_png_save_surface(surface, path);
	} else {
		result = (SDL_SaveBMP(surface, path) == 0) ? SUCCESS : FAILURE;
	}
	if (result != SUCCESS) {
		fprintf(stderr, "guest-screen-analysis-save-failed path=%s error=%s\n", path,
		        SDL_GetError());
	}
	SDL_FreeSurface(surface);
	SDL_FreeSurface(source);
	return (result);
}

static BOOL scrnmng_read_screenshot_fixture(const char *path, BOOL png, int *width, int *height,
                                            BYTE **pixels) {
	SDL_Surface *bmp, *rgba;
	int y;
	if (png)
		return scrnmng_load_stored_png(path, width, height, pixels);
	bmp = SDL_LoadBMP(path);
	if (!bmp)
		return FAILURE;
	rgba = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_FreeSurface(bmp);
	if (!rgba)
		return FAILURE;
	*width = rgba->w;
	*height = rgba->h;
	*pixels = (BYTE *)malloc((size_t)rgba->w * rgba->h * 4);
	if (*pixels) {
		for (y = 0; y < rgba->h; ++y)
			memcpy(*pixels + (size_t)y * rgba->w * 4,
			       (BYTE *)rgba->pixels + (size_t)y * rgba->pitch, (size_t)rgba->w * 4);
	}
	SDL_FreeSurface(rgba);
	return *pixels ? SUCCESS : FAILURE;
}

BOOL scrnmng_screenshot_rotation_selftest(const char *directory) {
	const SCRNMNG saved = scrnmng;
	const UINT8 saved_info = np2oscfg.DISPCLK;
	const int w = 37, h = 23;
	const int pitch = (SCRNMNG_SURFACE_GUARD_LEFT + 37 + 5) * 2;
	const size_t size = (size_t)pitch * h;
	BYTE *shadow = (BYTE *)malloc(size);
	BYTE *original = (BYTE *)malloc(size);
	BYTE *baseline = NULL, *pixels = NULL;
	SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB565);
	int mode, turn, x, y, width = 0, height = 0;
	BOOL ok = FAILURE;
	if (!shadow || !original || !format)
		goto cleanup;
	memset(shadow, 0xa5, size);
	for (y = 0; y < h; ++y) {
		UINT16 *row = (UINT16 *)(shadow + (size_t)y * pitch) + SCRNMNG_SURFACE_GUARD_LEFT;
		for (x = 0; x < w; ++x)
			row[x] =
			    (UINT16)(((x * 31 / (w - 1)) << 11) | ((y * 63 / (h - 1)) << 5) | ((x ^ y) & 31));
	}
	memcpy(original, shadow, size);
	scrnmng.shadow = shadow;
	scrnmng.shadow_pitch = pitch;
	scrnmng.width = w;
	scrnmng.height = h;
	/* Raw 565 and composed 8888; PNG/BMP; analysis off/on; 0 -> 90 -> 0.
	 * Every output is read back and every pixel checked, including odd widths,
	 * source guards/padding, alpha, orientation and absence of source mutation. */
	for (mode = 0; mode < 6; ++mode) {
		const BOOL png = (mode & 1) == 0;
		np2oscfg.DISPCLK = mode >= 4 ? VAEG_DISPINFO_VIDEO | VAEG_DISPINFO_FRAMEBUFFER : 0;
		for (turn = 0; turn < 3; ++turn) {
			char path[MAX_PATH];
			const int n = snprintf(path, sizeof(path), "%s/screenshot-%d-%d.%s", directory, mode,
			                       turn, png ? "png" : "bmp");
			if (n < 0 || (size_t)n >= sizeof(path))
				goto cleanup;
			scrnmng.rotation = turn == 1 ? VAEG_ROTATE_RIGHT : VAEG_ROTATE_NONE;
			if ((mode < 2 ? scrnmng_save_guest_frame(path)
			              : scrnmng_save_guest_frame_with_analysis(path)) != SUCCESS ||
			    scrnmng_read_screenshot_fixture(path, png, &width, &height, &pixels) != SUCCESS)
				goto cleanup;
			if (width != (turn == 1 ? h : w) || height != (turn == 1 ? w : h))
				goto cleanup;
			if (turn == 0) {
				baseline = pixels;
				pixels = NULL;
				/* The existing SDL 565->8888 blitter has its own expansion
				 * rounding. Check raw 565 against the source, and all composed
				 * rotations bit-exactly against their unturned composition. */
				if (mode < 2) {
					for (y = 0; y < h; ++y) {
						const UINT16 *row = (const UINT16 *)(original + (size_t)y * pitch) +
						                    SCRNMNG_SURFACE_GUARD_LEFT;
						for (x = 0; x < w; ++x) {
							BYTE r, g, b;
							const BYTE *p = baseline + ((size_t)y * w + x) * 4;
							SDL_GetRGB(row[x], format, &r, &g, &b);
							if (p[0] != r || p[1] != g || p[2] != b || p[3] != 255)
								goto cleanup;
						}
					}
				}
			} else {
				for (y = 0; y < h; ++y) {
					for (x = 0; x < w; ++x) {
						const int dx = turn == 1 ? h - 1 - y : x;
						const int dy = turn == 1 ? x : y;
						if (memcmp(pixels + ((size_t)dy * width + dx) * 4,
						           baseline + ((size_t)y * w + x) * 4, 4) != 0)
							goto cleanup;
					}
				}
			}
			free(pixels);
			pixels = NULL;
			if (memcmp(shadow, original, size) != 0)
				goto cleanup;
		}
		free(baseline);
		baseline = NULL;
	}
	/* M141: the state thumbnail turns with the screen (1:1, so exact). */
	for (turn = 0; turn < 2; ++turn) {
		char path[MAX_PATH];
		const int n = snprintf(path, sizeof(path), "%s/thumbnail-%d.png", directory, turn);
		if (n < 0 || (size_t)n >= sizeof(path))
			goto cleanup;
		scrnmng.rotation = turn ? VAEG_ROTATE_RIGHT : VAEG_ROTATE_NONE;
		if (scrnmng_save_guest_thumbnail(path, turn ? h : w) != SUCCESS ||
		    scrnmng_load_stored_png(path, &width, &height, &pixels) != SUCCESS ||
		    width != (turn ? h : w) || height != (turn ? w : h))
			goto cleanup;
		if (turn == 0) {
			baseline = pixels;
			pixels = NULL;
			continue;
		}
		for (y = 0; y < h; ++y) {
			for (x = 0; x < w; ++x) {
				if (memcmp(pixels + ((size_t)x * width + (h - 1 - y)) * 4,
				           baseline + ((size_t)y * w + x) * 4, 4) != 0)
					goto cleanup;
			}
		}
		free(pixels);
		pixels = NULL;
	}
	ok = SUCCESS;
cleanup:
	scrnmng = saved;
	np2oscfg.DISPCLK = saved_info;
	if (format)
		SDL_FreeFormat(format);
	free(shadow);
	free(original);
	free(baseline);
	free(pixels);
	if (ok != SUCCESS)
		fprintf(stderr, "selftest: SCREENSHOT_ROTATION_MISMATCH\n");
	return ok;
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
	/* FPS measurement is independent of the frame-counter visibility switch. */
	(void)vaeg_framedisp_update(&scrnmng.framedisp, tick, draws);
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
