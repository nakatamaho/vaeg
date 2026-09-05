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
#include "compiler.h"
#include "sdlapi.h"
#include "scrnmng.h"
#include "splash.h"

extern const unsigned char vaeg_splash_bmp[];
extern const unsigned int vaeg_splash_bmp_size;

static BOOL splash_fit_rect(int image_width, int image_height, int output_width,
                            int output_height, SDL_Rect *dst) {
	if (!dst || image_width <= 0 || image_height <= 0 || output_width <= 0 || output_height <= 0)
		return FAILURE;
	if ((SINT64)output_width * image_height <= (SINT64)output_height * image_width) {
		dst->w = output_width;
		dst->h = max(1, (int)((SINT64)image_height * output_width / image_width));
	} else {
		dst->h = output_height;
		dst->w = max(1, (int)((SINT64)image_width * output_height / image_height));
	}
	dst->x = (output_width - dst->w) / 2;
	dst->y = (output_height - dst->h) / 2;
	return SUCCESS;
}

BOOL splash_selftest(void) {
	SDL_Rect dst;
	const int cases[][6] = {
	    {640, 422, 0, 11, 640, 400},
	    {1280, 844, 0, 22, 1280, 800},
	    {1920, 1080, 96, 0, 1728, 1080},
	    {300, 200, 0, 6, 300, 187},
	};
	for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		if (splash_fit_rect(640, 400, cases[i][0], cases[i][1], &dst) != SUCCESS ||
		    dst.x != cases[i][2] || dst.y != cases[i][3] ||
		    dst.w != cases[i][4] || dst.h != cases[i][5]) return FAILURE;
	}
	return splash_fit_rect(640, 400, 0, 400, &dst) == FAILURE ? SUCCESS : FAILURE;
}

BOOL splash_show(void) {
	SDL_Renderer *renderer;
	SDL_RWops *stream;
	SDL_Surface *surface;
	SDL_Texture *texture;
	SDL_Rect dst;
	int output_width;
	int output_height;

	renderer = (SDL_Renderer *)scrnmng_get_renderer();
	if (renderer == NULL) {
		return (FAILURE);
	}
	stream = SDL_RWFromConstMem(vaeg_splash_bmp, (int)vaeg_splash_bmp_size);
	if (stream == NULL) {
		fprintf(stderr, "Warning: startup splash stream failed: %s\n", SDL_GetError());
		return (FAILURE);
	}
	surface = SDL_LoadBMP_RW(stream, 1);
	if (surface == NULL) {
		fprintf(stderr, "Warning: embedded startup splash failed: %s\n", SDL_GetError());
		return (FAILURE);
	}
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture == NULL) {
		fprintf(stderr, "Warning: startup splash texture failed: %s\n", SDL_GetError());
		SDL_FreeSurface(surface);
		return (FAILURE);
	}
	SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
	output_width = 0;
	output_height = 0;
	if (SDL_GetRendererOutputSize(renderer, &output_width, &output_height) != 0 ||
	    splash_fit_rect(surface->w, surface->h, output_width, output_height, &dst) != SUCCESS) {
		SDL_DestroyTexture(texture);
		SDL_FreeSurface(surface);
		return FAILURE;
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, &dst);
	SDL_RenderPresent(renderer);
	SDL_Log("Startup splash: embedded assets/vaeg.bmp; output=%dx%d image=%dx%d",
	        output_width, output_height, dst.w, dst.h);
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
	return (SUCCESS);
}
