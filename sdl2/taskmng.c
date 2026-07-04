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
#include	"taskmng.h"
#include	"sdlkbd.h"

	BOOL	task_avail;

void taskmng_initialize(void) {

	task_avail = TRUE;
}

void taskmng_exit(void) {

	task_avail = FALSE;
}

void taskmng_rol(void) {

	SDL_Event	e;

	while(task_avail && SDL_PollEvent(&e)) {
		switch(e.type) {
			case SDL_QUIT:
				task_avail = FALSE;
				break;

			case SDL_KEYDOWN:
				if (!e.key.repeat) {
					sdlkbd_keydown((UINT)e.key.keysym.scancode);
				}
				break;

			case SDL_KEYUP:
				sdlkbd_keyup((UINT)e.key.keysym.scancode);
				break;

			default:
				break;
		}
	}
}

BOOL taskmng_sleep(UINT32 tick) {

	Uint64	base;
	Uint64	freq;
	Uint64	elapsed;

	base = SDL_GetPerformanceCounter();
	freq = SDL_GetPerformanceFrequency();
	do {
		taskmng_rol();
		if (!task_avail) {
			break;
		}
		SDL_Delay(1);
		elapsed = ((SDL_GetPerformanceCounter() - base) * 1000) / freq;
	} while(elapsed < tick);
	return(task_avail);
}
