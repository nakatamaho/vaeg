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
#include	"kbdpaste.h"
#include	"pacing.h"
#include	"np2.h"
#include	"scrnmng.h"
#include	"sysmng.h"
#include	"gui/gui.h"
#include	"dropmedia.h"

		BOOL	task_avail;
	static VAEG_PACING_STATE task_pacing;
	static BOOL paste_shortcut_down;

static BOOL taskmng_paste_shortcut(const SDL_Event *event, BOOL captured) {

	UINT16 modifier;

	if (event->type == SDL_KEYUP) {
		if ((event->key.keysym.scancode == SDL_SCANCODE_V) &&
			paste_shortcut_down) {
			paste_shortcut_down = FALSE;
			return TRUE;
		}
		return FALSE;
	}
	if ((event->type != SDL_KEYDOWN) ||
		(event->key.keysym.scancode != SDL_SCANCODE_V)) {
		return FALSE;
	}
	modifier = (UINT16)event->key.keysym.mod;
#if defined(__APPLE__)
	if (!(modifier & KMOD_GUI)) {
#else
	if (!(modifier & KMOD_CTRL)) {
#endif
		return FALSE;
	}
	if (captured) {
		return FALSE;
	}
	paste_shortcut_down = TRUE;
	if (!event->key.repeat) {
		kbdpaste_start_clipboard();
	}
	return TRUE;
}

void taskmng_initialize(void) {

	task_avail = TRUE;
	paste_shortcut_down = FALSE;
	vaeg_pacing_reset(&task_pacing);
	kbdpaste_initialize();
}

void taskmng_exit(void) {

	task_avail = FALSE;
	paste_shortcut_down = FALSE;
	vaeg_pacing_reset(&task_pacing);
	kbdpaste_shutdown();
}

void taskmng_clear_fast_forward(void) {

	vaeg_pacing_reset(&task_pacing);
}

BOOL taskmng_effective_nowait(BOOL configured_nowait) {

	return(vaeg_pacing_effective_nowait(&task_pacing, configured_nowait));
}

UINT taskmng_effective_drawskip(UINT configured_drawskip) {

	return(vaeg_pacing_effective_drawskip(&task_pacing,
											configured_drawskip));
}

void taskmng_rol(void) {

	SDL_Event	e;

	while(task_avail && SDL_PollEvent(&e)) {
		BOOL captured;
		BOOL shortcut;

		if (dropmedia_process_event(&e)) {
			continue;
		}
		shortcut = FALSE;
		if (e.type == SDL_KEYDOWN) {
			shortcut = vaeg_pacing_key(&task_pacing,
							(UINT)e.key.keysym.scancode, TRUE,
							e.key.repeat ? TRUE : FALSE);
		}
		else if (e.type == SDL_KEYUP) {
			shortcut = vaeg_pacing_key(&task_pacing,
							(UINT)e.key.keysym.scancode, FALSE, FALSE);
		}
			else if ((e.type == SDL_WINDOWEVENT) &&
					 (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
				vaeg_pacing_reset(&task_pacing);
				paste_shortcut_down = FALSE;
				kbdpaste_cancel();
			}
			else if ((e.type == SDL_WINDOWEVENT) &&
					 (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
				int width;
				int height;

				if ((scrnmng_capture_window_size(&width, &height) == SUCCESS) &&
					((np2oscfg.gui_window_width != width) ||
					 (np2oscfg.gui_window_height != height))) {
					np2oscfg.gui_window_width = (UINT16)min(width, 65535);
					np2oscfg.gui_window_height = (UINT16)min(height, 65535);
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
		else if (e.type == SDL_QUIT) {
			vaeg_pacing_reset(&task_pacing);
			paste_shortcut_down = FALSE;
			kbdpaste_cancel();
		}
		captured = gui_process_event(&e);
		if (taskmng_paste_shortcut(&e, captured)) {
			shortcut = TRUE;
		}
		switch(e.type) {
			case SDL_QUIT:
				task_avail = FALSE;
				break;

			case SDL_KEYDOWN:
				if (!shortcut) {
					sdlkbd_keydown((UINT)e.key.keysym.scancode,
								   e.key.keysym.sym,
								   (UINT16)e.key.keysym.mod,
								   captured,
								   e.key.repeat ? TRUE : FALSE);
				}
				break;

			case SDL_KEYUP:
				if (!shortcut) {
					sdlkbd_keyup((UINT)e.key.keysym.scancode,
								 e.key.keysym.sym,
								 (UINT16)e.key.keysym.mod,
								 captured);
				}
				break;

			case SDL_TEXTINPUT:
				sdlkbd_textinput(e.text.text, captured);
				break;

			default:
				break;
		}
	}
	if (task_avail) {
		kbdpaste_tick(SDL_GetTicks(), gui_guest_keyboard_blocked());
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
