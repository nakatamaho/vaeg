/* Copyright (c) 2026 Nakata Maho
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
#ifndef VAEG_IO_VAB_H
#define VAEG_IO_VAB_H

/* M150: NEC PC-8801-17 Video Art Board, after X88000 1.5.3 (PDS source,
 * PC88Z80Main.cpp and X88ScreenDrawer.cpp):
 *   - 128 KiB of RAM appears as extended-RAM card 1: port E3h bits 3-2 =
 *     01b select it, bits 1-0 the 32 KiB bank; port E2h bit 0 maps it for
 *     reads and bit 4 for writes at 0000h-7FFFh (the usual extended-RAM
 *     switching);
 *   - port B4h (mode 1) and B5h (mode 2) are latches; they read back only
 *     while card 1 is selected in E3h, otherwise FFh;
 *   - when mode 1 has bits 6 and 0 set ((B4h & 41h) == 41h) the screen is
 *     the board's 320x200 picture instead of the PC-8801 screen: one
 *     little-endian word per pixel, row-major from offset 0, green in bits
 *     15-10, red in bits 9-5, blue in bits 4-0.
 * A board on the PC-8801 bus does not fit a PC-88VA; vaeg models it in
 * every 88-compatible session (the PC-8801 models of M113 and V1/V2 mode),
 * default off. */

enum {
    VAB_W = 320,
    VAB_H = 200,
    VAB_RAM_SIZE = 0x20000,
    VAB_CARD = 1
};

typedef struct {
    UINT8 mode1; /* last OUT B4h */
    UINT8 mode2; /* last OUT B5h */
    UINT16 padding;
} _VAB, *VAB;

#ifdef __cplusplus
extern "C" {
#endif

extern _VAB vab;
extern UINT8 vab_ram[VAB_RAM_SIZE];

void vab_reset(void);
void vab_bind(void);
/* TRUE when the extended-RAM bank register (port E3h, 4 bits) selects the
 * board and the board is enabled. */
BOOL vab_card_selected(UINT8 eram_bank);
/* The board RAM offset of an extended-RAM access at CPU address 0000h-
 * 7FFFh with the given E3h value. */
UINT32 vab_ram_offset(UINT8 eram_bank, UINT32 address);
/* TRUE while the board's picture replaces the screen. */
BOOL vab_display_active(void);
/* Draw the 320x200 picture into an RGB565 surface (dst at pixel (0,0),
 * pitch bytes per row), scaled to width x height, nearest neighbour. */
void vab_compose_rgb565(UINT8 *dst, int pitch, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
