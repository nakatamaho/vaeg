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
#ifndef VAEG_SDL2_NP2_H
#define VAEG_SDL2_NP2_H

enum {
	NP2OSCFG_KEYBOARD_NAME_SIZE = 16,
	NP2OSCFG_KEYBOARD_CUSTOM_MAP_SIZE = 8192,
	NP2OSCFG_OPN_BACKEND_NAME_SIZE = 8
};

typedef struct {
	BYTE	NOWAIT;
	BYTE	DRAW_SKIP;
	BYTE	F12KEY;
	BYTE	resume;
	BYTE	jastsnd;
	BYTE	gui_scale;
	BYTE	gui_aspect;
	char	gui_fdd_dir[MAX_PATH];
	char	gui_hdd_dir[MAX_PATH];
	char	fdd_image[2][MAX_PATH];
	char	keyboard_host_layout[NP2OSCFG_KEYBOARD_NAME_SIZE];
	char	keyboard_kana_input[NP2OSCFG_KEYBOARD_NAME_SIZE];
	BYTE	keyboard_auto_kana_lock;
	BYTE	keyboard_tenkey_overlay;
	char	keyboard_custom_map[NP2OSCFG_KEYBOARD_CUSTOM_MAP_SIZE];
	char	opn_backend[NP2OSCFG_OPN_BACKEND_NAME_SIZE];
	BYTE	sound_enabled;
} NP2OSCFG;

#if defined(SIZE_QVGA)
enum {
	FULLSCREEN_WIDTH	= 320,
	FULLSCREEN_HEIGHT	= 240
};
#else
enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 400
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern	NP2OSCFG	np2oscfg;
UINT16 np2_default_sound_for_model(const char *model);
BOOL np2_sound_hardware_valid(const char *model, UINT16 sound);
BOOL np2_select_boot_model(const char *model);

#ifdef __cplusplus
}
#endif

#endif
