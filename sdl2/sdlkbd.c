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
#include	"np2.h"
#include	"sdlkbd.h"
#include	"keystat.h"

typedef struct {
	SDL_Scancode	scancode;
	UINT16			keycode;
} SDLKCNV;

#define		NC		0xff

static const SDLKCNV sdlcnv101[] = {
			{SDL_SCANCODE_ESCAPE,		0x00},	{SDL_SCANCODE_1,			0x01},
			{SDL_SCANCODE_2,			0x02},	{SDL_SCANCODE_3,			0x03},
			{SDL_SCANCODE_4,			0x04},	{SDL_SCANCODE_5,			0x05},
			{SDL_SCANCODE_6,			0x06},	{SDL_SCANCODE_7,			0x07},
			{SDL_SCANCODE_8,			0x08},	{SDL_SCANCODE_9,			0x09},
			{SDL_SCANCODE_0,			0x0a},	{SDL_SCANCODE_MINUS,		0x0b},
			{SDL_SCANCODE_EQUALS,		0x0c},	{SDL_SCANCODE_BACKSLASH,	0x0d},
			{SDL_SCANCODE_BACKSPACE,	0x0e},	{SDL_SCANCODE_TAB,			0x0f},
			{SDL_SCANCODE_Q,			0x10},	{SDL_SCANCODE_W,			0x11},
			{SDL_SCANCODE_E,			0x12},	{SDL_SCANCODE_R,			0x13},
			{SDL_SCANCODE_T,			0x14},	{SDL_SCANCODE_Y,			0x15},
			{SDL_SCANCODE_U,			0x16},	{SDL_SCANCODE_I,			0x17},
			{SDL_SCANCODE_O,			0x18},	{SDL_SCANCODE_P,			0x19},
			{SDL_SCANCODE_RETURN,		0x1c},	{SDL_SCANCODE_A,			0x1d},
			{SDL_SCANCODE_S,			0x1e},	{SDL_SCANCODE_D,			0x1f},
			{SDL_SCANCODE_F,			0x20},	{SDL_SCANCODE_G,			0x21},
			{SDL_SCANCODE_H,			0x22},	{SDL_SCANCODE_J,			0x23},
			{SDL_SCANCODE_K,			0x24},	{SDL_SCANCODE_L,			0x25},
			{SDL_SCANCODE_Z,			0x29},	{SDL_SCANCODE_X,			0x2a},
			{SDL_SCANCODE_C,			0x2b},	{SDL_SCANCODE_V,			0x2c},
			{SDL_SCANCODE_B,			0x2d},	{SDL_SCANCODE_N,			0x2e},
			{SDL_SCANCODE_M,			0x2f},	{SDL_SCANCODE_COMMA,		0x30},
			{SDL_SCANCODE_PERIOD,		0x31},	{SDL_SCANCODE_SLASH,		0x32},
			{SDL_SCANCODE_SPACE,		0x34},	{SDL_SCANCODE_PAGEUP,		0x36},
			{SDL_SCANCODE_PAGEDOWN,		0x37},	{SDL_SCANCODE_INSERT,		0x38},
			{SDL_SCANCODE_DELETE,		0x39},	{SDL_SCANCODE_UP,			0x3a},
			{SDL_SCANCODE_LEFT,			0x3b},	{SDL_SCANCODE_RIGHT,		0x3c},
			{SDL_SCANCODE_DOWN,			0x3d},	{SDL_SCANCODE_HOME,			0x3e},
			{SDL_SCANCODE_END,			0x3f},	{SDL_SCANCODE_KP_MINUS,		0x40},
			{SDL_SCANCODE_KP_DIVIDE,	0x41},	{SDL_SCANCODE_KP_7,			0x42},
			{SDL_SCANCODE_KP_8,			0x43},	{SDL_SCANCODE_KP_9,			0x44},
			{SDL_SCANCODE_KP_MULTIPLY,	0x45},	{SDL_SCANCODE_KP_4,			0x46},
			{SDL_SCANCODE_KP_5,			0x47},	{SDL_SCANCODE_KP_6,			0x48},
			{SDL_SCANCODE_KP_PLUS,		0x49},	{SDL_SCANCODE_KP_1,			0x4a},
			{SDL_SCANCODE_KP_2,			0x4b},	{SDL_SCANCODE_KP_3,			0x4c},
			{SDL_SCANCODE_KP_0,			0x4e},	{SDL_SCANCODE_KP_PERIOD,	0x50},
			{SDL_SCANCODE_PAUSE,		0x60},	{SDL_SCANCODE_PRINTSCREEN,	0x61},
			{SDL_SCANCODE_F1,			0x62},	{SDL_SCANCODE_F2,			0x63},
			{SDL_SCANCODE_F3,			0x64},	{SDL_SCANCODE_F4,			0x65},
			{SDL_SCANCODE_F5,			0x66},	{SDL_SCANCODE_F6,			0x67},
			{SDL_SCANCODE_F7,			0x68},	{SDL_SCANCODE_F8,			0x69},
			{SDL_SCANCODE_F9,			0x6a},	{SDL_SCANCODE_F10,			0x6b},
			{SDL_SCANCODE_RSHIFT,		0x70},	{SDL_SCANCODE_LSHIFT,		0x70},
			{SDL_SCANCODE_CAPSLOCK,		0x71},
			{SDL_SCANCODE_RALT,			0x73},	{SDL_SCANCODE_LALT,			0x73},
			{SDL_SCANCODE_RCTRL,		0x74},	{SDL_SCANCODE_LCTRL,		0x74}};

static	BYTE	keytbl[SDL_NUM_SCANCODES];

static const BYTE f12keys[] = {
			0x61, 0x60, 0x4d, 0x4f};

void sdlkbd_initialize(void) {

	int			i;
const SDLKCNV	*key;
const SDLKCNV	*keyterm;

	for (i=0; i<SDL_NUM_SCANCODES; i++) {
		keytbl[i] = NC;
	}
	key = sdlcnv101;
	keyterm = key + (sizeof(sdlcnv101)/sizeof(SDLKCNV));
	while(key < keyterm) {
		keytbl[key->scancode] = (BYTE)key->keycode;
		key++;
	}
}

static BYTE getf12key(void) {

	UINT	key;

	key = np2oscfg.F12KEY - 1;
	if (key < (sizeof(f12keys)/sizeof(BYTE))) {
		return(f12keys[key]);
	}
	return(NC);
}

static BYTE scancode_to_key(UINT scancode) {

	if (scancode == SDL_SCANCODE_F12) {
		return(getf12key());
	}
	if (scancode < SDL_NUM_SCANCODES) {
		return(keytbl[scancode]);
	}
	return(NC);
}

void sdlkbd_keydown(UINT scancode) {

	BYTE	data;

	data = scancode_to_key(scancode);
	if (data != NC) {
		keystat_senddata(data);
	}
}

void sdlkbd_keyup(UINT scancode) {

	BYTE	data;

	data = scancode_to_key(scancode);
	if (data != NC) {
		keystat_senddata((BYTE)(data | 0x80));
	}
}

void sdlkbd_resetf12(void) {

	UINT	i;

	for (i=0; i<(sizeof(f12keys)/sizeof(BYTE)); i++) {
		keystat_forcerelease(f12keys[i]);
	}
}
