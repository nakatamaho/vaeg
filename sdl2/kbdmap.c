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
#include	"dosio.h"
#include	"np2.h"
#include	"textfile.h"
#include	"kbdinject.h"
#include	"kbdmap.h"
#include	"romankana.h"

#define	E(role, id, label, semantic, code, jis, us, status, evidence) \
	{role, id, label, semantic, code, jis, us, status, evidence}

static const KBDMAP_ENTRY entries[] = {
	E(KBDROLE_STOP, "stop", "STOP", "KEY88_STOP", 0x60, SDL_SCANCODE_PAUSE, SDL_SCANCODE_PAUSE, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:20; keystat.h:102"),
	E(KBDROLE_COPY, "copy", "COPY", "KEY88_COPY", 0x61, SDL_SCANCODE_PRINTSCREEN, SDL_SCANCODE_PRINTSCREEN, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:178; keystat.h:103"),
	E(KBDROLE_F1, "f1", "F1", "KEY88_F1", 0x62, SDL_SCANCODE_F1, SDL_SCANCODE_F1, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:50; keystat.h:104"),
	E(KBDROLE_F2, "f2", "F2", "KEY88_F2", 0x63, SDL_SCANCODE_F2, SDL_SCANCODE_F2, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:50; keystat.h:105"),
	E(KBDROLE_F3, "f3", "F3", "KEY88_F3", 0x64, SDL_SCANCODE_F3, SDL_SCANCODE_F3, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:51; keystat.h:106"),
	E(KBDROLE_F4, "f4", "F4", "KEY88_F4", 0x65, SDL_SCANCODE_F4, SDL_SCANCODE_F4, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:51; keystat.h:107"),
	E(KBDROLE_F5, "f5", "F5", "KEY88_F5", 0x66, SDL_SCANCODE_F5, SDL_SCANCODE_F5, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:51; keystat.h:108"),
	E(KBDROLE_F6, "f6", "F6", "KEY88_F6", 0x67, SDL_SCANCODE_F6, SDL_SCANCODE_F6, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:51; keystat.h:109"),
	E(KBDROLE_F7, "f7", "F7", "KEY88_F7", 0x68, SDL_SCANCODE_F7, SDL_SCANCODE_F7, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:51; keystat.h:111"),
	E(KBDROLE_F8, "f8", "F8", "KEY88_F8", 0x69, SDL_SCANCODE_F8, SDL_SCANCODE_F8, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:51; keystat.h:112"),
	E(KBDROLE_F9, "f9", "F9", "KEY88_F9", 0x6a, SDL_SCANCODE_F9, SDL_SCANCODE_F9, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:52; keystat.h:113"),
	E(KBDROLE_F10, "f10", "F10", "KEY88_F10", 0x6b, SDL_SCANCODE_F10, SDL_SCANCODE_F10, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:52; keystat.h:114"),
	E(KBDROLE_ROLLUP, "rollup", "ROLL UP", "KEY88_ROLLUP", 0x36, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_PAGEUP, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:62; keystat.h:64"),
	E(KBDROLE_ROLLDOWN, "rolldown", "ROLL DOWN", "KEY88_ROLLDOWN", 0x37, SDL_SCANCODE_PAGEDOWN, SDL_SCANCODE_PAGEDOWN, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:63; keystat.h:65"),
	E(KBDROLE_ESC, "esc", "ESC", "KEY88_ESC", 0x00, SDL_SCANCODE_ESCAPE, SDL_SCANCODE_ESCAPE, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:39; keystat.h:4"),
	E(KBDROLE_1, "1", "1", "KEY88_1", 0x01, SDL_SCANCODE_1, SDL_SCANCODE_1, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:39; keystat.h:5"),
	E(KBDROLE_2, "2", "2", "KEY88_2", 0x02, SDL_SCANCODE_2, SDL_SCANCODE_2, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:40; keystat.h:6"),
	E(KBDROLE_3, "3", "3", "KEY88_3", 0x03, SDL_SCANCODE_3, SDL_SCANCODE_3, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:40; keystat.h:7"),
	E(KBDROLE_4, "4", "4", "KEY88_4", 0x04, SDL_SCANCODE_4, SDL_SCANCODE_4, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:41; keystat.h:8"),
	E(KBDROLE_5, "5", "5", "KEY88_5", 0x05, SDL_SCANCODE_5, SDL_SCANCODE_5, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:41; keystat.h:9"),
	E(KBDROLE_6, "6", "6", "KEY88_6", 0x06, SDL_SCANCODE_6, SDL_SCANCODE_6, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:42; keystat.h:10"),
	E(KBDROLE_7, "7", "7", "KEY88_7", 0x07, SDL_SCANCODE_7, SDL_SCANCODE_7, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:42; keystat.h:11"),
	E(KBDROLE_8, "8", "8", "KEY88_8", 0x08, SDL_SCANCODE_8, SDL_SCANCODE_8, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:43; keystat.h:13"),
	E(KBDROLE_9, "9", "9", "KEY88_9", 0x09, SDL_SCANCODE_9, SDL_SCANCODE_9, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:43; keystat.h:14"),
	E(KBDROLE_0, "0", "0", "KEY88_0", 0x0a, SDL_SCANCODE_0, SDL_SCANCODE_0, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:44; keystat.h:15"),
	E(KBDROLE_MINUS, "minus", "-", "KEY88_MINUS", 0x0b, SDL_SCANCODE_MINUS, SDL_SCANCODE_MINUS, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:44; keystat.h:16"),
	E(KBDROLE_CARET, "caret", "^", "KEY88_CARET", 0x0c, SDL_SCANCODE_EQUALS, SDL_SCANCODE_EQUALS, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:45; keystat.h:17"),
	E(KBDROLE_YEN, "yen", "Yen", "KEY88_YEN", 0x0d, SDL_SCANCODE_INTERNATIONAL3, SDL_SCANCODE_BACKSLASH, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:76; keystat.h:18"),
	E(KBDROLE_BS, "backspace", "Backspace", "KEY88_BS", 0x0e, SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_BACKSPACE, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:46; win9x/winkbd.cpp:106; keystat.h:19"),
	E(KBDROLE_TAB, "tab", "TAB", "KEY88_TAB", 0x0f, SDL_SCANCODE_TAB, SDL_SCANCODE_TAB, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:46; keystat.h:20"),
	E(KBDROLE_Q, "q", "Q", "KEY88_q", 0x10, SDL_SCANCODE_Q, SDL_SCANCODE_Q, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:47; keystat.h:22"),
	E(KBDROLE_W, "w", "W", "KEY88_w", 0x11, SDL_SCANCODE_W, SDL_SCANCODE_W, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:47; keystat.h:23"),
	E(KBDROLE_E, "e", "E", "KEY88_e", 0x12, SDL_SCANCODE_E, SDL_SCANCODE_E, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:48; keystat.h:24"),
	E(KBDROLE_R, "r", "R", "KEY88_r", 0x13, SDL_SCANCODE_R, SDL_SCANCODE_R, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:48; keystat.h:25"),
	E(KBDROLE_T, "t", "T", "KEY88_t", 0x14, SDL_SCANCODE_T, SDL_SCANCODE_T, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:49; keystat.h:26"),
	E(KBDROLE_Y, "y", "Y", "KEY88_y", 0x15, SDL_SCANCODE_Y, SDL_SCANCODE_Y, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:49; keystat.h:27"),
	E(KBDROLE_U, "u", "U", "KEY88_u", 0x16, SDL_SCANCODE_U, SDL_SCANCODE_U, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:50; keystat.h:28"),
	E(KBDROLE_I, "i", "I", "KEY88_i", 0x17, SDL_SCANCODE_I, SDL_SCANCODE_I, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:50; keystat.h:29"),
	E(KBDROLE_O, "o", "O", "KEY88_o", 0x18, SDL_SCANCODE_O, SDL_SCANCODE_O, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:51; keystat.h:31"),
	E(KBDROLE_P, "p", "P", "KEY88_p", 0x19, SDL_SCANCODE_P, SDL_SCANCODE_P, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:51; keystat.h:32"),
	E(KBDROLE_AT, "at", "@", "KEY88_AT", 0x1a, SDL_SCANCODE_LEFTBRACKET, SDL_SCANCODE_LEFTBRACKET, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:70; keystat.h:33"),
	E(KBDROLE_BRACKETLEFT, "bracketleft", "[", "KEY88_BRACKETLEFT", 0x1b, SDL_SCANCODE_RIGHTBRACKET, SDL_SCANCODE_RIGHTBRACKET, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:76; keystat.h:34"),
	E(KBDROLE_RETURNL, "returnl", "Return left", "KEY88_RETURNL", 0x1c, SDL_SCANCODE_RETURN, SDL_SCANCODE_RETURN, KBDMAP_STATUS_IMPLEMENTED, "win9x/winkbd.cpp:107-108; keystat.h:35"),
	E(KBDROLE_CTRL, "ctrl", "CTRL/Roman", "KEY88_CTRL", 0x74, SDL_SCANCODE_LCTRL, SDL_SCANCODE_LCTRL, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:84; io/serial.c:38; keystat.h:120"),
	E(KBDROLE_CAPS, "caps", "CAPS", "KEY88_CAPS", 0x71, SDL_SCANCODE_CAPSLOCK, SDL_SCANCODE_CAPSLOCK, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:82; keystat.h:117"),
	E(KBDROLE_A, "a", "A", "KEY88_a", 0x1d, SDL_SCANCODE_A, SDL_SCANCODE_A, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:52; keystat.h:36"),
	E(KBDROLE_S, "s", "S", "KEY88_s", 0x1e, SDL_SCANCODE_S, SDL_SCANCODE_S, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:53; keystat.h:37"),
	E(KBDROLE_D, "d", "D", "KEY88_d", 0x1f, SDL_SCANCODE_D, SDL_SCANCODE_D, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:53; keystat.h:38"),
	E(KBDROLE_F, "f", "F", "KEY88_f", 0x20, SDL_SCANCODE_F, SDL_SCANCODE_F, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:54; keystat.h:40"),
	E(KBDROLE_G, "g", "G", "KEY88_g", 0x21, SDL_SCANCODE_G, SDL_SCANCODE_G, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:54; keystat.h:41"),
	E(KBDROLE_H, "h", "H", "KEY88_h", 0x22, SDL_SCANCODE_H, SDL_SCANCODE_H, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:55; keystat.h:42"),
	E(KBDROLE_J, "j", "J", "KEY88_j", 0x23, SDL_SCANCODE_J, SDL_SCANCODE_J, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:55; keystat.h:43"),
	E(KBDROLE_K, "k", "K", "KEY88_k", 0x24, SDL_SCANCODE_K, SDL_SCANCODE_K, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:56; keystat.h:44"),
	E(KBDROLE_L, "l", "L", "KEY88_l", 0x25, SDL_SCANCODE_L, SDL_SCANCODE_L, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:56; keystat.h:45"),
	E(KBDROLE_SEMICOLON, "semicolon", ";", "KEY88_SEMICOLON", 0x26, SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_SEMICOLON, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:68; keystat.h:46"),
	E(KBDROLE_COLON, "colon", ":", "KEY88_COLON", 0x27, SDL_SCANCODE_APOSTROPHE, SDL_SCANCODE_APOSTROPHE, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:68; keystat.h:47"),
	E(KBDROLE_BRACKETRIGHT, "bracketright", "]", "KEY88_BRACKETRIGHT", 0x28, SDL_SCANCODE_NONUSHASH, SDL_SCANCODE_NONUSHASH, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:76; keystat.h:49"),
	E(KBDROLE_SHIFTL, "shiftl", "Shift left", "KEY88_SHIFTL", 0x70, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_LSHIFT, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:81; io/serial.c:38; keystat.h:116"),
	E(KBDROLE_Z, "z", "Z", "KEY88_z", 0x29, SDL_SCANCODE_Z, SDL_SCANCODE_Z, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:57; keystat.h:50"),
	E(KBDROLE_X, "x", "X", "KEY88_x", 0x2a, SDL_SCANCODE_X, SDL_SCANCODE_X, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:57; keystat.h:51"),
	E(KBDROLE_C, "c", "C", "KEY88_c", 0x2b, SDL_SCANCODE_C, SDL_SCANCODE_C, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:58; keystat.h:52"),
	E(KBDROLE_V, "v", "V", "KEY88_v", 0x2c, SDL_SCANCODE_V, SDL_SCANCODE_V, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:58; keystat.h:53"),
	E(KBDROLE_B, "b", "B", "KEY88_b", 0x2d, SDL_SCANCODE_B, SDL_SCANCODE_B, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:59; keystat.h:54"),
	E(KBDROLE_N, "n", "N", "KEY88_n", 0x2e, SDL_SCANCODE_N, SDL_SCANCODE_N, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:59; keystat.h:55"),
	E(KBDROLE_M, "m", "M", "KEY88_m", 0x2f, SDL_SCANCODE_M, SDL_SCANCODE_M, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:60; keystat.h:56"),
	E(KBDROLE_COMMA, "comma", ",", "KEY88_COMMA", 0x30, SDL_SCANCODE_COMMA, SDL_SCANCODE_COMMA, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:60; keystat.h:58"),
	E(KBDROLE_PERIOD, "period", ".", "KEY88_PERIOD", 0x31, SDL_SCANCODE_PERIOD, SDL_SCANCODE_PERIOD, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:61; keystat.h:59"),
	E(KBDROLE_SLASH, "slash", "/", "KEY88_SLASH", 0x32, SDL_SCANCODE_SLASH, SDL_SCANCODE_SLASH, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:61; keystat.h:60"),
	E(KBDROLE_UNDERSCORE, "underscore", "_/RO", "KEY88_UNDERSCORE", 0x33, SDL_SCANCODE_INTERNATIONAL1, SDL_SCANCODE_GRAVE, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:76-79; keystat.h:61"),
	E(KBDROLE_SHIFTR, "shiftr", "Shift right", "KEY88_SHIFTR", 0x58, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_RSHIFT, KBDMAP_STATUS_MAPPED_UNTESTED, "io/serial.c:38 and :107-109"),
	E(KBDROLE_KANA, "kana", "KANA", "KEY88_KANA", 0x72, SDL_SCANCODE_INTERNATIONAL2, SDL_SCANCODE_RALT, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:82-87; keystat.h:118"),
	E(KBDROLE_GRAPH, "graph", "GRPH", "KEY88_GRAPH", 0x73, SDL_SCANCODE_LALT, SDL_SCANCODE_LALT, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:83; keystat.h:119"),
	E(KBDROLE_KETTEI, "kettei", "NFER/KETTEI", "KEY88_KETTEI", 0x51, SDL_SCANCODE_INTERNATIONAL5, SDL_SCANCODE_F11, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:24-25; io/serial.c:34; keystat.h:95"),
	E(KBDROLE_SPACE, "space", "SPACE", "KEY88_SPACE", 0x34, SDL_SCANCODE_SPACE, SDL_SCANCODE_SPACE, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:62; keystat.h:62"),
	E(KBDROLE_HENKAN, "henkan", "XFER/HENKAN", "KEY88_HENKAN", 0x35, SDL_SCANCODE_INTERNATIONAL4, SDL_SCANCODE_APPLICATION, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:24-25; keystat.h:63"),
	E(KBDROLE_PC, "pc", "PC", "KEY88_PC", 0x5a, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, KBDMAP_STATUS_UNRESOLVED, "win9x/winkbd.cpp:40-42; io/serial.c:38 and :107-109"),
	E(KBDROLE_ZENKAKU, "zenkaku", "ZENKAKU", "KEY88_ZENKAKU", 0x5b, SDL_SCANCODE_LANG5, SDL_SCANCODE_UNKNOWN, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:82-84; io/serial.c:38 and :107-109"),
	E(KBDROLE_INS, "insert", "INSERT", "KEY88_INS", 0x38, SDL_SCANCODE_INSERT, SDL_SCANCODE_INSERT, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:63; keystat.h:67"),
	E(KBDROLE_DEL, "delete", "DELETE", "KEY88_DEL", 0x39, SDL_SCANCODE_DELETE, SDL_SCANCODE_DELETE, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:64; keystat.h:68"),
	E(KBDROLE_UP, "up", "UP", "KEY88_UP", 0x3a, SDL_SCANCODE_UP, SDL_SCANCODE_UP, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:64; keystat.h:69"),
	E(KBDROLE_DOWN, "down", "DOWN", "KEY88_DOWN", 0x3d, SDL_SCANCODE_DOWN, SDL_SCANCODE_DOWN, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:66; keystat.h:72"),
	E(KBDROLE_LEFT, "left", "LEFT", "KEY88_LEFT", 0x3b, SDL_SCANCODE_LEFT, SDL_SCANCODE_LEFT, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:65; keystat.h:70"),
	E(KBDROLE_RIGHT, "right", "RIGHT", "KEY88_RIGHT", 0x3c, SDL_SCANCODE_RIGHT, SDL_SCANCODE_RIGHT, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:65; keystat.h:71"),
	E(KBDROLE_HOME, "home", "HOME CLR", "KEY88_HOME", 0x3e, SDL_SCANCODE_HOME, SDL_SCANCODE_HOME, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:66; keystat.h:73"),
	E(KBDROLE_HELP, "help", "HELP", "KEY88_HELP", 0x3f, SDL_SCANCODE_HELP, SDL_SCANCODE_END, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:67 old fallback; keystat.h:74"),
	E(KBDROLE_KP_SUB, "kp_sub", "keypad -", "KEY88_KP_SUB", 0x40, SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_MINUS, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:67; keystat.h:76"),
	E(KBDROLE_KP_DIVIDE, "kp_divide", "keypad /", "KEY88_KP_DIVIDE", 0x41, SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_DIVIDE, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:68; keystat.h:77"),
	E(KBDROLE_KP_7, "kp_7", "keypad 7", "KEY88_KP_7", 0x42, SDL_SCANCODE_KP_7, SDL_SCANCODE_KP_7, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:68; keystat.h:78"),
	E(KBDROLE_KP_8, "kp_8", "keypad 8", "KEY88_KP_8", 0x43, SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_8, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:69; keystat.h:79"),
	E(KBDROLE_KP_9, "kp_9", "keypad 9", "KEY88_KP_9", 0x44, SDL_SCANCODE_KP_9, SDL_SCANCODE_KP_9, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:69; keystat.h:80"),
	E(KBDROLE_KP_MULTIPLY, "kp_multiply", "keypad *", "KEY88_KP_MULTIPLY", 0x45, SDL_SCANCODE_KP_MULTIPLY, SDL_SCANCODE_KP_MULTIPLY, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:70; keystat.h:81"),
	E(KBDROLE_KP_4, "kp_4", "keypad 4", "KEY88_KP_4", 0x46, SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_4, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:70; keystat.h:82"),
	E(KBDROLE_KP_5, "kp_5", "keypad 5", "KEY88_KP_5", 0x47, SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_5, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:71; keystat.h:83"),
	E(KBDROLE_KP_6, "kp_6", "keypad 6", "KEY88_KP_6", 0x48, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_6, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:71; keystat.h:85"),
	E(KBDROLE_KP_ADD, "kp_add", "keypad +", "KEY88_KP_ADD", 0x49, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_PLUS, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:72; keystat.h:86"),
	E(KBDROLE_KP_1, "kp_1", "keypad 1", "KEY88_KP_1", 0x4a, SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_1, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:72; keystat.h:87"),
	E(KBDROLE_KP_2, "kp_2", "keypad 2", "KEY88_KP_2", 0x4b, SDL_SCANCODE_KP_2, SDL_SCANCODE_KP_2, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:73; keystat.h:88"),
	E(KBDROLE_KP_3, "kp_3", "keypad 3", "KEY88_KP_3", 0x4c, SDL_SCANCODE_KP_3, SDL_SCANCODE_KP_3, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:73; keystat.h:89"),
	E(KBDROLE_KP_EQUAL, "kp_equal", "keypad =", "KEY88_KP_EQUAL", 0x4d, SDL_SCANCODE_KP_EQUALS, SDL_SCANCODE_KP_EQUALS, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:58-59; keystat.h:90"),
	E(KBDROLE_KP_0, "kp_0", "keypad 0", "KEY88_KP_0", 0x4e, SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_0, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:74; keystat.h:91"),
	E(KBDROLE_KP_COMMA, "kp_comma", "keypad ,", "KEY88_KP_COMMA", 0x4f, SDL_SCANCODE_KP_COMMA, SDL_SCANCODE_KP_COMMA, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:46-49; keystat.h:92"),
	E(KBDROLE_KP_PERIOD, "kp_period", "keypad .", "KEY88_KP_PERIOD", 0x50, SDL_SCANCODE_KP_PERIOD, SDL_SCANCODE_KP_PERIOD, KBDMAP_STATUS_IMPLEMENTED, "sdl2/sdlkbd.c:74; keystat.h:94"),
	E(KBDROLE_RETURNR, "returnr", "keypad Enter", "KEY88_RETURNR", 0x59, SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_KP_ENTER, KBDMAP_STATUS_MAPPED_UNTESTED, "win9x/winkbd.cpp:106-108; io/serial.c:38 and :107-109")
};

typedef struct {
	const char	*token;
	BYTE		key;
	BYTE		shift;
	BYTE		mark;
} KANASEQ;

typedef struct {
	const char	*token;
	const char	*base;
	const char	*small_y;
} KANACOMBOSEQ;

typedef struct {
	const char	*token;
	const char	*first;
	const char	*second;
} KANAPAIRSEQ;

enum {
	KANASEQ_MARK_NONE = 0,
	KANASEQ_MARK_DAKUTEN,
	KANASEQ_MARK_HANDAKUTEN
};

static const KANASEQ kana_sequences[] = {
	{"a", 0x03, 0, KANASEQ_MARK_NONE}, {"i", 0x12, 0, KANASEQ_MARK_NONE},
	{"u", 0x04, 0, KANASEQ_MARK_NONE}, {"e", 0x05, 0, KANASEQ_MARK_NONE},
	{"o", 0x06, 0, KANASEQ_MARK_NONE},
	{"xa", 0x03, 1, KANASEQ_MARK_NONE}, {"xi", 0x12, 1, KANASEQ_MARK_NONE},
	{"xu", 0x04, 1, KANASEQ_MARK_NONE}, {"xe", 0x05, 1, KANASEQ_MARK_NONE},
	{"xo", 0x06, 1, KANASEQ_MARK_NONE},
	{"ka", 0x14, 0, KANASEQ_MARK_NONE}, {"ki", 0x21, 0, KANASEQ_MARK_NONE},
	{"ku", 0x22, 0, KANASEQ_MARK_NONE}, {"ke", 0x27, 0, KANASEQ_MARK_NONE},
	{"ko", 0x2d, 0, KANASEQ_MARK_NONE},
	{"sa", 0x2a, 0, KANASEQ_MARK_NONE}, {"shi", 0x1f, 0, KANASEQ_MARK_NONE},
	{"su", 0x13, 0, KANASEQ_MARK_NONE}, {"se", 0x19, 0, KANASEQ_MARK_NONE},
	{"so", 0x2b, 0, KANASEQ_MARK_NONE},
	{"ta", 0x10, 0, KANASEQ_MARK_NONE}, {"chi", 0x1d, 0, KANASEQ_MARK_NONE},
	{"tsu", 0x29, 0, KANASEQ_MARK_NONE}, {"te", 0x11, 0, KANASEQ_MARK_NONE},
	{"to", 0x1e, 0, KANASEQ_MARK_NONE},
	{"na", 0x16, 0, KANASEQ_MARK_NONE}, {"ni", 0x17, 0, KANASEQ_MARK_NONE},
	{"nu", 0x01, 0, KANASEQ_MARK_NONE}, {"ne", 0x30, 0, KANASEQ_MARK_NONE},
	{"no", 0x24, 0, KANASEQ_MARK_NONE},
	{"ha", 0x20, 0, KANASEQ_MARK_NONE}, {"hi", 0x2c, 0, KANASEQ_MARK_NONE},
	{"fu", 0x02, 0, KANASEQ_MARK_NONE}, {"he", 0x0c, 0, KANASEQ_MARK_NONE},
	{"ho", 0x0b, 0, KANASEQ_MARK_NONE},
	{"ma", 0x23, 0, KANASEQ_MARK_NONE}, {"mi", 0x2e, 0, KANASEQ_MARK_NONE},
	{"mu", 0x28, 0, KANASEQ_MARK_NONE}, {"me", 0x32, 0, KANASEQ_MARK_NONE},
	{"mo", 0x2f, 0, KANASEQ_MARK_NONE},
	{"ya", 0x07, 0, KANASEQ_MARK_NONE}, {"yu", 0x08, 0, KANASEQ_MARK_NONE},
	{"yo", 0x09, 0, KANASEQ_MARK_NONE},
	{"xya", 0x07, 1, KANASEQ_MARK_NONE}, {"xyu", 0x08, 1, KANASEQ_MARK_NONE},
	{"xyo", 0x09, 1, KANASEQ_MARK_NONE},
	{"ra", 0x18, 0, KANASEQ_MARK_NONE}, {"ri", 0x25, 0, KANASEQ_MARK_NONE},
	{"ru", 0x31, 0, KANASEQ_MARK_NONE}, {"re", 0x26, 0, KANASEQ_MARK_NONE},
	{"ro", 0x33, 0, KANASEQ_MARK_NONE},
	{"wa", 0x0a, 0, KANASEQ_MARK_NONE}, {"wo", 0x0a, 1, KANASEQ_MARK_NONE},
	{"nn", 0x15, 0, KANASEQ_MARK_NONE}, {"xtsu", 0x29, 1, KANASEQ_MARK_NONE},
	{"ga", 0x14, 0, KANASEQ_MARK_DAKUTEN}, {"gi", 0x21, 0, KANASEQ_MARK_DAKUTEN},
	{"gu", 0x22, 0, KANASEQ_MARK_DAKUTEN}, {"ge", 0x27, 0, KANASEQ_MARK_DAKUTEN},
	{"go", 0x2d, 0, KANASEQ_MARK_DAKUTEN},
	{"za", 0x2a, 0, KANASEQ_MARK_DAKUTEN}, {"ji", 0x1f, 0, KANASEQ_MARK_DAKUTEN},
	{"zu", 0x13, 0, KANASEQ_MARK_DAKUTEN}, {"ze", 0x19, 0, KANASEQ_MARK_DAKUTEN},
	{"zo", 0x2b, 0, KANASEQ_MARK_DAKUTEN},
	{"da", 0x10, 0, KANASEQ_MARK_DAKUTEN}, {"de", 0x11, 0, KANASEQ_MARK_DAKUTEN},
	{"do", 0x1e, 0, KANASEQ_MARK_DAKUTEN},
	{"ba", 0x20, 0, KANASEQ_MARK_DAKUTEN}, {"bi", 0x2c, 0, KANASEQ_MARK_DAKUTEN},
	{"bu", 0x02, 0, KANASEQ_MARK_DAKUTEN}, {"be", 0x0c, 0, KANASEQ_MARK_DAKUTEN},
	{"bo", 0x0b, 0, KANASEQ_MARK_DAKUTEN},
	{"pa", 0x20, 0, KANASEQ_MARK_HANDAKUTEN}, {"pi", 0x2c, 0, KANASEQ_MARK_HANDAKUTEN},
	{"pu", 0x02, 0, KANASEQ_MARK_HANDAKUTEN}, {"pe", 0x0c, 0, KANASEQ_MARK_HANDAKUTEN},
	{"po", 0x0b, 0, KANASEQ_MARK_HANDAKUTEN},
	{"vu", 0x04, 0, KANASEQ_MARK_DAKUTEN}
};

static const KANACOMBOSEQ kana_combo_sequences[] = {
	{"kya", "ki", "xya"}, {"kyu", "ki", "xyu"}, {"kyo", "ki", "xyo"},
	{"sha", "shi", "xya"}, {"shu", "shi", "xyu"}, {"sho", "shi", "xyo"},
	{"cha", "chi", "xya"}, {"chu", "chi", "xyu"}, {"cho", "chi", "xyo"},
	{"nya", "ni", "xya"}, {"nyu", "ni", "xyu"}, {"nyo", "ni", "xyo"},
	{"hya", "hi", "xya"}, {"hyu", "hi", "xyu"}, {"hyo", "hi", "xyo"},
	{"mya", "mi", "xya"}, {"myu", "mi", "xyu"}, {"myo", "mi", "xyo"},
	{"rya", "ri", "xya"}, {"ryu", "ri", "xyu"}, {"ryo", "ri", "xyo"},
	{"gya", "gi", "xya"}, {"gyu", "gi", "xyu"}, {"gyo", "gi", "xyo"},
	{"ja", "ji", "xya"}, {"ju", "ji", "xyu"}, {"jo", "ji", "xyo"},
	{"bya", "bi", "xya"}, {"byu", "bi", "xyu"}, {"byo", "bi", "xyo"},
	{"pya", "pi", "xya"}, {"pyu", "pi", "xyu"}, {"pyo", "pi", "xyo"}
};

static const KANAPAIRSEQ kana_pair_sequences[] = {
	{"va", "vu", "xa"}, {"vi", "vu", "xi"},
	{"ve", "vu", "xe"}, {"vo", "vu", "xo"}
};

static const BYTE f12keys[] = {
	0x61, 0x60, 0x4d, 0x4f
};

#define	KANA_GUEST_CODE	0x72

static BYTE scancode_key[SDL_NUM_SCANCODES];
static int scancode_role[SDL_NUM_SCANCODES];
static SDL_Scancode bindings[KBDROLE_COUNT];
static KBDMAP_STATUS bind_status[KBDROLE_COUNT];
static ROMANKANA_STATE roman_state;
static BOOL roman_scancode_down[SDL_NUM_SCANCODES];
static BOOL kana_mirror;

static const char custom_map_file[] = "keyboard.map";
static const char custom_map_file_prefix[] = "file:";

static int str_equal(const char *a, const char *b) {

	while((*a != '\0') && (*b != '\0')) {
		char ca = *a++;
		char cb = *b++;
		if ((ca >= 'A') && (ca <= 'Z')) {
			ca = (char)(ca - 'A' + 'a');
		}
		if ((cb >= 'A') && (cb <= 'Z')) {
			cb = (char)(cb - 'A' + 'a');
		}
		if (ca != cb) {
			return 0;
		}
	}
	return (*a == '\0') && (*b == '\0');
}

static const char *normal_layout(const char *layout) {

	if (str_equal(layout, "us")) {
		return "us";
	}
	if (str_equal(layout, "custom")) {
		return "custom";
	}
	return "jis";
}

static const char *normal_kana_input(const char *mode) {

	if (str_equal(mode, "roman")) {
		return "roman";
	}
	return "jis-kana";
}

static void update_text_input_state(void) {

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		return;
	}
	SDL_StopTextInput();
}

static int entry_index_from_role(KBDMAP_ROLE role) {

	int	i;

	for (i = 0; i < (int)NELEMENTS(entries); i++) {
		if (entries[i].role == role) {
			return i;
		}
	}
	return -1;
}

static int entry_index_from_id(const char *id) {

	int	i;

	for (i = 0; i < (int)NELEMENTS(entries); i++) {
		if (!strcmp(entries[i].id, id)) {
			return i;
		}
	}
	return -1;
}

static void set_config_string(char *dst, size_t dst_size, const char *src) {

	if (dst_size == 0) {
		return;
	}
	milstr_ncpy(dst, src, (int)dst_size);
}

static void set_layout_defaults(const char *layout) {

	int	i;
	int	use_us;

	use_us = str_equal(layout, "us");
	for (i = 0; i < (int)NELEMENTS(entries); i++) {
		bindings[i] = use_us ? entries[i].us_scancode : entries[i].jis_scancode;
		if (entries[i].status == KBDMAP_STATUS_INTENTIONALLY_UNMAPPED) {
			bind_status[i] = KBDMAP_STATUS_INTENTIONALLY_UNMAPPED;
		}
		else if (bindings[i] == SDL_SCANCODE_UNKNOWN) {
			bind_status[i] = KBDMAP_STATUS_UNRESOLVED;
		}
		else {
			bind_status[i] = entries[i].status;
		}
	}
}

static void rebuild_scancode_table(void) {

	int	i;

	for (i = 0; i < SDL_NUM_SCANCODES; i++) {
		scancode_key[i] = KBDMAP_NC;
		scancode_role[i] = -1;
	}
	for (i = 0; i < (int)NELEMENTS(entries); i++) {
		SDL_Scancode scancode = bindings[i];
		if ((scancode > SDL_SCANCODE_UNKNOWN) &&
			(scancode < SDL_NUM_SCANCODES) &&
			(entries[i].guest_code != KBDMAP_NC)) {
			scancode_key[scancode] = entries[i].guest_code;
			scancode_role[scancode] = i;
		}
	}
}

static int entry_index_from_id_len(const char *id, size_t len) {

	int	i;

	for (i = 0; i < (int)NELEMENTS(entries); i++) {
		if ((strlen(entries[i].id) == len) &&
			(!memcmp(entries[i].id, id, len))) {
			return i;
		}
	}
	return -1;
}

static SDL_Scancode scancode_from_name(const char *name) {

	SDL_Scancode scancode;

	scancode = SDL_GetScancodeFromName(name);
	if (scancode != SDL_SCANCODE_UNKNOWN) {
		return scancode;
	}
	if (!strcmp(name, "-")) {
		return SDL_SCANCODE_MINUS;
	}
	if (!strcmp(name, "=")) {
		return SDL_SCANCODE_EQUALS;
	}
	if (!strcmp(name, "\\")) {
		return SDL_SCANCODE_BACKSLASH;
	}
	if (!strcmp(name, "[")) {
		return SDL_SCANCODE_LEFTBRACKET;
	}
	if (!strcmp(name, "]")) {
		return SDL_SCANCODE_RIGHTBRACKET;
	}
	if (!strcmp(name, ";")) {
		return SDL_SCANCODE_SEMICOLON;
	}
	if (!strcmp(name, "'")) {
		return SDL_SCANCODE_APOSTROPHE;
	}
	if (!strcmp(name, "#")) {
		return SDL_SCANCODE_NONUSHASH;
	}
	if (!strcmp(name, "`")) {
		return SDL_SCANCODE_GRAVE;
	}
	if (!strcmp(name, ",")) {
		return SDL_SCANCODE_COMMA;
	}
	if (!strcmp(name, ".")) {
		return SDL_SCANCODE_PERIOD;
	}
	if (!strcmp(name, "/")) {
		return SDL_SCANCODE_SLASH;
	}
	return SDL_SCANCODE_UNKNOWN;
}

static void set_custom_binding(int index, const char *name) {

	SDL_Scancode scancode;

	if ((index < 0) || (name == NULL)) {
		return;
	}
	scancode = scancode_from_name(name);
	bindings[index] = scancode;
	bind_status[index] = (scancode == SDL_SCANCODE_UNKNOWN) ?
		KBDMAP_STATUS_UNRESOLVED : entries[index].status;
}

static const char *find_next_inline_entry(const char *p) {

	const char	*semi;
	int			i;

	semi = p;
	while((semi = strchr(semi, ';')) != NULL) {
		const char *next = semi + 1;
		for (i = 0; i < (int)NELEMENTS(entries); i++) {
			size_t len = strlen(entries[i].id);
			if ((!strncmp(next, entries[i].id, len)) &&
				(next[len] == '=')) {
				return semi;
			}
		}
		semi++;
	}
	return NULL;
}

static void parse_inline_custom_map(const char *map) {

	const char	*p;

	p = map;
	while((p != NULL) && (*p != '\0')) {
		const char	*eq;
		const char	*end;
		char		name[128];
		size_t		id_len;
		size_t		name_len;
		int			index;

		while(*p == ';') {
			p++;
		}
		eq = strchr(p, '=');
		if (eq == NULL) {
			break;
		}
		id_len = (size_t)(eq - p);
		index = entry_index_from_id_len(p, id_len);
		end = find_next_inline_entry(eq + 1);
		if (end == NULL) {
			end = p + strlen(p);
		}
		name_len = (size_t)(end - (eq + 1));
		if ((*end == '\0') &&
			(name_len > 0) &&
			(eq[1 + name_len - 1] == ';')) {
			name_len--;
		}
		if (name_len >= sizeof(name)) {
			name_len = sizeof(name) - 1;
		}
		memcpy(name, eq + 1, name_len);
		name[name_len] = '\0';
		set_custom_binding(index, name);
		p = (*end == ';') ? end + 1 : end;
	}
}

static void custom_map_path(char *path, int size) {

	file_getstatepath(path, size, custom_map_file);
}

static void parse_sidecar_custom_map(const char *name) {

	char		path[MAX_PATH];
	char		line[256];
	TEXTFILEH	fh;

	(void)name;
	custom_map_path(path, sizeof(path));
	fh = textfile_open(path, 0x800);
	if (fh == NULL) {
		return;
	}
	while(textfile_read(fh, line, sizeof(line)) == SUCCESS) {
		char	*eq;
		int		index;

		eq = strchr(line, '=');
		if (eq == NULL) {
			continue;
		}
		*eq++ = '\0';
		index = entry_index_from_id(line);
		set_custom_binding(index, eq);
	}
	textfile_close(fh);
}

static void parse_custom_map(void) {

	if (!memcmp(np2oscfg.keyboard_custom_map, custom_map_file_prefix,
				NELEMENTS(custom_map_file_prefix) - 1)) {
		parse_sidecar_custom_map(np2oscfg.keyboard_custom_map +
								 NELEMENTS(custom_map_file_prefix) - 1);
	}
	else {
		parse_inline_custom_map(np2oscfg.keyboard_custom_map);
	}
}

static void append_text(char *dst, size_t size, const char *text) {

	size_t len;
	size_t add;

	if (size == 0) {
		return;
	}
	len = strlen(dst);
	if (len + 1 >= size) {
		return;
	}
	add = strlen(text);
	if (add > size - len - 1) {
		add = size - len - 1;
	}
	memcpy(dst + len, text, add);
	dst[len + add] = '\0';
}

static void write_custom_sidecar(void) {

	char	path[MAX_PATH];
	FILEH	fh;
	int		i;

	custom_map_path(path, sizeof(path));
	fh = file_create(path);
	if (fh == FILEH_INVALID) {
		SDL_Log("Keyboard map: cannot write %s", path);
		return;
	}
	for (i = 0; i < (int)NELEMENTS(entries); i++) {
		const char *name = SDL_GetScancodeName(bindings[i]);
		char line[256];
		if ((bindings[i] == SDL_SCANCODE_UNKNOWN) ||
			(name == NULL) || (name[0] == '\0')) {
			continue;
		}
		SPRINTF(line, "%s=%s\n", entries[i].id, name);
		file_write(fh, line, strlen(line));
	}
	file_close(fh);
}

static void write_custom_config(void) {

	write_custom_sidecar();
	set_config_string(np2oscfg.keyboard_custom_map,
					  sizeof(np2oscfg.keyboard_custom_map),
					  "file:keyboard.map");
}

static BYTE getf12key(void) {

	UINT	key;

	key = np2oscfg.F12KEY - 1;
	if (key < NELEMENTS(f12keys)) {
		return f12keys[key];
	}
	return KBDMAP_NC;
}

static BOOL is_roman_scancode(UINT scancode) {

	if ((scancode >= SDL_SCANCODE_A) && (scancode <= SDL_SCANCODE_Z)) {
		return TRUE;
	}
	switch(scancode) {
		case SDL_SCANCODE_APOSTROPHE:
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		case SDL_SCANCODE_CAPSLOCK:
			return TRUE;
		default:
			return FALSE;
	}
}

static char roman_char_from_scancode(UINT scancode) {

	if ((scancode >= SDL_SCANCODE_A) && (scancode <= SDL_SCANCODE_Z)) {
		return (char)('a' + (scancode - SDL_SCANCODE_A));
	}
	if (scancode == SDL_SCANCODE_APOSTROPHE) {
		return '\'';
	}
	return '\0';
}

static BOOL roman_mode(void) {

	return str_equal(normal_kana_input(np2oscfg.keyboard_kana_input), "roman") ?
		TRUE : FALSE;
}

static void roman_emit(const char *token, void *arg);

static void roman_feed_char(char c) {

	char	text[2];

	text[0] = c;
	text[1] = '\0';
	romankana_feed(&roman_state, text, roman_emit, NULL);
}

static BOOL roman_active(void) {

	return (roman_mode() && kana_mirror) ? TRUE : FALSE;
}

static void emit_kana_sequence(const KANASEQ *seq) {

	if (seq->shift) {
		kbdinject_keydown(0x70);
	}
	kbdinject_press(seq->key);
	if (seq->shift) {
		kbdinject_keyup(0x70);
	}
	switch(seq->mark) {
		case KANASEQ_MARK_DAKUTEN:
			kbdinject_press(0x1a);
			break;
		case KANASEQ_MARK_HANDAKUTEN:
			kbdinject_press(0x1b);
			break;
		default:
			break;
	}
}

static const KANASEQ *find_kana_sequence(const char *token) {

	UINT	i;

	for (i = 0; i < NELEMENTS(kana_sequences); i++) {
		if (!strcmp(kana_sequences[i].token, token)) {
			return kana_sequences + i;
		}
	}
	return NULL;
}

static BOOL emit_kana_token(const char *token) {

	const KANASEQ *seq;

	seq = find_kana_sequence(token);
	if (seq == NULL) {
		return FALSE;
	}
	emit_kana_sequence(seq);
	return TRUE;
}

static void roman_emit(const char *token, void *arg) {

	UINT	i;

	(void)arg;
	if (!strcmp(token, "?")) {
		SDL_Log("Roman-Kana: unsupported input flushed");
		return;
	}
	if (emit_kana_token(token)) {
		return;
	}
	for (i = 0; i < NELEMENTS(kana_combo_sequences); i++) {
		if (!strcmp(kana_combo_sequences[i].token, token)) {
			if (emit_kana_token(kana_combo_sequences[i].base) &&
				emit_kana_token(kana_combo_sequences[i].small_y)) {
				return;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(kana_pair_sequences); i++) {
		if (!strcmp(kana_pair_sequences[i].token, token)) {
			if (emit_kana_token(kana_pair_sequences[i].first) &&
				emit_kana_token(kana_pair_sequences[i].second)) {
				return;
			}
			break;
		}
	}
	SDL_Log("Roman-Kana: token has no verified guest sequence: %s", token);
}

void kbdmap_apply_config(void) {

	const char *layout;
	const char *kana_input;

	layout = normal_layout(np2oscfg.keyboard_host_layout);
	kana_input = normal_kana_input(np2oscfg.keyboard_kana_input);
	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), layout);
	set_config_string(np2oscfg.keyboard_kana_input,
					  sizeof(np2oscfg.keyboard_kana_input), kana_input);
	np2oscfg.keyboard_auto_kana_lock =
		np2oscfg.keyboard_auto_kana_lock ? 1 : 0;

	set_layout_defaults(str_equal(layout, "custom") ? "jis" : layout);
	if (str_equal(layout, "custom")) {
		parse_custom_map();
	}
	rebuild_scancode_table();
	update_text_input_state();
}

void kbdmap_initialize(void) {

	romankana_reset(&roman_state);
	ZeroMemory(roman_scancode_down, sizeof(roman_scancode_down));
	kana_mirror = FALSE;
	kbdmap_apply_config();
}

BYTE kbdmap_lookup(UINT scancode) {

	if (scancode == SDL_SCANCODE_F12) {
		return getf12key();
	}
	if (scancode < SDL_NUM_SCANCODES) {
		return scancode_key[scancode];
	}
	return KBDMAP_NC;
}

BOOL kbdmap_keydown(UINT scancode) {

	BYTE	data;
	int	role;
	char	roman_char;

	if (scancode == SDL_SCANCODE_F12) {
		data = getf12key();
		if (data != KBDMAP_NC) {
			kbdinject_keydown(data);
			return TRUE;
		}
		return FALSE;
	}
	role = (scancode < SDL_NUM_SCANCODES) ? scancode_role[scancode] : -1;
	if ((role >= 0) && (entries[role].role == KBDROLE_KANA)) {
		kbdinject_press(entries[role].guest_code);
		kana_mirror = kana_mirror ? FALSE : TRUE;
		romankana_reset(&roman_state);
		return TRUE;
	}
	if (roman_active()) {
		roman_char = roman_char_from_scancode(scancode);
		if (roman_char != '\0') {
			roman_feed_char(roman_char);
			if (scancode < SDL_NUM_SCANCODES) {
				roman_scancode_down[scancode] = TRUE;
			}
			return TRUE;
		}
		if (is_roman_scancode(scancode)) {
			if (scancode < SDL_NUM_SCANCODES) {
				roman_scancode_down[scancode] = TRUE;
			}
			return TRUE;
		}
		romankana_flush(&roman_state, roman_emit, NULL);
	}
	data = kbdmap_lookup(scancode);
	if (data != KBDMAP_NC) {
		kbdinject_keydown(data);
		return TRUE;
	}
	return FALSE;
}

BOOL kbdmap_keyup(UINT scancode) {

	BYTE	data;
	int	role;

	if (scancode == SDL_SCANCODE_F12) {
		data = getf12key();
		if (data != KBDMAP_NC) {
			kbdinject_keyup(data);
			return TRUE;
		}
		return FALSE;
	}
	role = (scancode < SDL_NUM_SCANCODES) ? scancode_role[scancode] : -1;
	if ((role >= 0) && (entries[role].role == KBDROLE_KANA)) {
		return TRUE;
	}
	if ((scancode < SDL_NUM_SCANCODES) && roman_scancode_down[scancode]) {
		roman_scancode_down[scancode] = FALSE;
		return TRUE;
	}
	data = kbdmap_lookup(scancode);
	if (data != KBDMAP_NC) {
		kbdinject_keyup(data);
		return TRUE;
	}
	return FALSE;
}

BOOL kbdmap_textinput(const char *text) {

	(void)text;
	if (!roman_active()) {
		return FALSE;
	}
	return TRUE;
}

void kbdmap_reset_frontend_state(void) {

	kana_mirror = FALSE;
	romankana_reset(&roman_state);
	ZeroMemory(roman_scancode_down, sizeof(roman_scancode_down));
}

void kbdmap_resetf12(void) {

	UINT	i;

	for (i = 0; i < NELEMENTS(f12keys); i++) {
		kbdinject_forcerelease(f12keys[i]);
	}
}

int kbdmap_entry_count(void) {

	return (int)NELEMENTS(entries);
}

const KBDMAP_ENTRY *kbdmap_entry(int index) {

	if ((index < 0) || (index >= (int)NELEMENTS(entries))) {
		return NULL;
	}
	return entries + index;
}

SDL_Scancode kbdmap_binding(int index) {

	if ((index < 0) || (index >= (int)NELEMENTS(entries))) {
		return SDL_SCANCODE_UNKNOWN;
	}
	return bindings[index];
}

KBDMAP_STATUS kbdmap_binding_status(int index) {

	if ((index < 0) || (index >= (int)NELEMENTS(entries))) {
		return KBDMAP_STATUS_UNRESOLVED;
	}
	if (bindings[index] == SDL_SCANCODE_UNKNOWN) {
		return KBDMAP_STATUS_UNRESOLVED;
	}
	return bind_status[index];
}

const char *kbdmap_status_name(KBDMAP_STATUS status) {

	switch(status) {
		case KBDMAP_STATUS_IMPLEMENTED:
			return "implemented";
		case KBDMAP_STATUS_MAPPED_UNTESTED:
			return "mapped-but-untested";
		case KBDMAP_STATUS_INTENTIONALLY_UNMAPPED:
			return "intentionally-unmapped";
		default:
			return "unresolved";
	}
}

const char *kbdmap_layout_name(void) {

	return normal_layout(np2oscfg.keyboard_host_layout);
}

const char *kbdmap_kana_input_name(void) {

	return normal_kana_input(np2oscfg.keyboard_kana_input);
}

void kbdmap_set_layout(const char *layout) {

	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout),
					  normal_layout(layout));
	kbdmap_apply_config();
}

void kbdmap_set_kana_input(const char *mode) {

	const char *new_mode;

	new_mode = normal_kana_input(mode);
	set_config_string(np2oscfg.keyboard_kana_input,
					  sizeof(np2oscfg.keyboard_kana_input), new_mode);
	romankana_reset(&roman_state);
	update_text_input_state();
}

void kbdmap_reset_to_jis(void) {

	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), "jis");
	np2oscfg.keyboard_custom_map[0] = '\0';
	kbdmap_apply_config();
}

void kbdmap_reset_to_us(void) {

	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), "us");
	np2oscfg.keyboard_custom_map[0] = '\0';
	kbdmap_apply_config();
}

BOOL kbdmap_set_binding(int index, SDL_Scancode scancode) {

	if ((index < 0) || (index >= (int)NELEMENTS(entries))) {
		return FAILURE;
	}
	bindings[index] = scancode;
	bind_status[index] = (scancode == SDL_SCANCODE_UNKNOWN) ?
		KBDMAP_STATUS_UNRESOLVED : entries[index].status;
	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), "custom");
	write_custom_config();
	rebuild_scancode_table();
	return SUCCESS;
}

void kbdmap_serialize_custom(char *dst, size_t size) {

	int	i;

	if (size == 0) {
		return;
	}
	dst[0] = '\0';
	for (i = 0; i < (int)NELEMENTS(entries); i++) {
		const char *name = SDL_GetScancodeName(bindings[i]);
		if ((bindings[i] == SDL_SCANCODE_UNKNOWN) ||
			(name == NULL) || (name[0] == '\0')) {
			continue;
		}
		append_text(dst, size, entries[i].id);
		append_text(dst, size, "=");
		append_text(dst, size, name);
		append_text(dst, size, ";");
	}
}

int kbdmap_selftest(void) {

#define	KBDMAP_SELFTEST_FAIL(msg)	\
	do {							\
		fprintf(stderr, "selftest: keyboard-map detail: %s\n", msg); \
		goto err;					\
	} while(0)

	char	saved_layout[sizeof(np2oscfg.keyboard_host_layout)];
	char	saved_mode[sizeof(np2oscfg.keyboard_kana_input)];
	char	saved_custom[sizeof(np2oscfg.keyboard_custom_map)];
	BYTE	saved_auto;
	char	serialized[sizeof(np2oscfg.keyboard_custom_map)];
	int	kana_index;
	int	semicolon_index;
	int	i;

	if (NELEMENTS(entries) != KBDROLE_COUNT) {
		return FAILURE;
	}
	for (i = 0; i < (int)NELEMENTS(entries); i++) {
		if (entries[i].role != (KBDMAP_ROLE)i) {
			return FAILURE;
		}
	}
	for (i = 0; i < (int)NELEMENTS(kana_combo_sequences); i++) {
		if ((find_kana_sequence(kana_combo_sequences[i].base) == NULL) ||
			(find_kana_sequence(kana_combo_sequences[i].small_y) == NULL)) {
			KBDMAP_SELFTEST_FAIL("Roman-Kana yoon sequence table");
		}
	}
	for (i = 0; i < (int)NELEMENTS(kana_pair_sequences); i++) {
		if ((find_kana_sequence(kana_pair_sequences[i].first) == NULL) ||
			(find_kana_sequence(kana_pair_sequences[i].second) == NULL)) {
			KBDMAP_SELFTEST_FAIL("Roman-Kana pair sequence table");
		}
	}

	milstr_ncpy(saved_layout, np2oscfg.keyboard_host_layout, sizeof(saved_layout));
	milstr_ncpy(saved_mode, np2oscfg.keyboard_kana_input, sizeof(saved_mode));
	milstr_ncpy(saved_custom, np2oscfg.keyboard_custom_map, sizeof(saved_custom));
	saved_auto = np2oscfg.keyboard_auto_kana_lock;

	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), "jis");
	set_config_string(np2oscfg.keyboard_kana_input,
					  sizeof(np2oscfg.keyboard_kana_input), "jis-kana");
	np2oscfg.keyboard_custom_map[0] = '\0';
	np2oscfg.keyboard_auto_kana_lock = 0;
	kbdmap_apply_config();
	if (kbdmap_lookup(SDL_SCANCODE_A) != 0x1d) {
		KBDMAP_SELFTEST_FAIL("JIS A lookup");
	}
	if (kbdmap_lookup(SDL_SCANCODE_INTERNATIONAL3) != 0x0d) {
		KBDMAP_SELFTEST_FAIL("JIS yen lookup");
	}
	if ((roman_char_from_scancode(SDL_SCANCODE_A) != 'a') ||
		(roman_char_from_scancode(SDL_SCANCODE_Z) != 'z') ||
		(roman_char_from_scancode(SDL_SCANCODE_APOSTROPHE) != '\'')) {
		KBDMAP_SELFTEST_FAIL("Roman-Kana scancode conversion");
	}
	kana_index = entry_index_from_role(KBDROLE_KANA);
	semicolon_index = entry_index_from_role(KBDROLE_SEMICOLON);
	if ((kana_index < 0) || (semicolon_index < 0)) {
		KBDMAP_SELFTEST_FAIL("role lookup");
	}
	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), "custom");
	set_config_string(np2oscfg.keyboard_custom_map,
					  sizeof(np2oscfg.keyboard_custom_map),
					  "kana=Right Alt;semicolon=;;caret==;");
	kbdmap_apply_config();
	if (kbdmap_binding(kana_index) != SDL_SCANCODE_RALT) {
		KBDMAP_SELFTEST_FAIL("inline Right Alt parse");
	}
	if (kbdmap_binding(semicolon_index) != SDL_SCANCODE_SEMICOLON) {
		KBDMAP_SELFTEST_FAIL("inline semicolon parse");
	}
	kbdmap_serialize_custom(serialized, sizeof(serialized));
	if (strstr(serialized, "kana=") == NULL) {
		KBDMAP_SELFTEST_FAIL("custom serialization");
	}
	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), "custom");
	set_config_string(np2oscfg.keyboard_custom_map,
					  sizeof(np2oscfg.keyboard_custom_map),
					  "kana=Definitely Not A Scancode;");
	kbdmap_apply_config();
	if (kbdmap_binding_status(kana_index) != KBDMAP_STATUS_UNRESOLVED) {
		KBDMAP_SELFTEST_FAIL("unresolved scancode handling");
	}

	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), saved_layout);
	set_config_string(np2oscfg.keyboard_kana_input,
					  sizeof(np2oscfg.keyboard_kana_input), saved_mode);
	set_config_string(np2oscfg.keyboard_custom_map,
					  sizeof(np2oscfg.keyboard_custom_map), saved_custom);
	np2oscfg.keyboard_auto_kana_lock = saved_auto;
	kbdmap_apply_config();
	return SUCCESS;

err:
	set_config_string(np2oscfg.keyboard_host_layout,
					  sizeof(np2oscfg.keyboard_host_layout), saved_layout);
	set_config_string(np2oscfg.keyboard_kana_input,
					  sizeof(np2oscfg.keyboard_kana_input), saved_mode);
	set_config_string(np2oscfg.keyboard_custom_map,
					  sizeof(np2oscfg.keyboard_custom_map), saved_custom);
	np2oscfg.keyboard_auto_kana_lock = saved_auto;
	kbdmap_apply_config();
	return FAILURE;

#undef KBDMAP_SELFTEST_FAIL
}
