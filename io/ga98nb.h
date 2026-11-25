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

/*
 * M166: I-O DATA GA-98NBIV (GA-98NB4; Cirrus Logic CL-GD5434, 4 MiB) in a
 * PC-88VA bus slot, port set y=0. Modelled from the board's PC-98 I/O
 * mapping and the GD543x reference manual; not yet measured on a VA.
 *
 * - I/O: 46E8h (adapter setup/sleep); VGA port p (3C0h-3CFh) at
 *   ((p & 0Fh) << 8) | 40E0h, 3D4h/3D5h at 54E0h/55E0h, 3DAh at 5AE0h.
 *   An odd port xxE1h is the high byte of a 16-bit access to xxE0h (OUT
 *   44E0h,1206h writes SR06 = 12h): vaeg splits a word
 *   access into two byte accesses, so xxE1h stands for VGA port p + 1.
 *   40E1h is the display relay (bit 0) instead.
 * - Enabling: 46E8h <- 18h (setup), 42E0h <- 01h (in setup: the video
 *   enable, the chip's port 102h), 46E8h <- 08h (enabled). Disabled, the
 *   card's ports read FFh, as an empty slot.
 * - Identification: SR06 reads 12h after 12h is written (unlocked), 0Fh
 *   otherwise; CR27 reads A8h (GD5434).
 * - VRAM window: E0000h-E7FFFh while the PC-88VA releases ROM0 to the
 *   bus slot (port 152h RBC03-00 = 8-F on the PC-88VA; A-F assumed on the
 *   PC-88VA2, whose ROM0 has 10 banks), the card is enabled and the Misc
 *   Output enables video memory (bit 1). With GR0B bit 0 (dual page)
 *   E0000h-E3FFFh uses GR09 and E4000h-E7FFFh GR0A, else GR09 for all;
 *   the offset unit is 16 KiB with GR0B bit 5, else 4 KiB. Bytes are
 *   linear (packed-pixel) video memory; the planar VGA modes and the BitBLT
 *   engine are not modelled, nor is the board's video output.
 */

#ifndef VAEG_IO_GA98NB_H
#define VAEG_IO_GA98NB_H

enum {
    GA98NB_VRAM_SIZE = 0x400000,
    GA98NB_WINDOW_BASE = 0x0e0000,
    GA98NB_WINDOW_SIZE = 0x8000
};

/* np2cfg.ga98nb_fault: deliberate faults for testing the probe. */
enum {
    GA98NB_FAULT_NONE = 0,
    GA98NB_FAULT_BYTE_BUS = 1,   /* xxE1h not decoded: word OUTs lose the high byte */
    GA98NB_FAULT_BANK_ALIAS = 2, /* the window ignores the bank registers */
    GA98NB_FAULT_NO_WINDOW = 3   /* the window never decodes */
};

typedef struct {
    UINT8 setup;      /* last 46E8h */
    UINT8 video_on;   /* port 102h bit 0, written through 42E0h in setup */
    UINT8 misc;       /* Misc Output (42E0h write, 4CE0h read) */
    UINT8 relay;      /* 40E1h */
    UINT8 seq_index;
    UINT8 crtc_index;
    UINT8 gr_index;
    UINT8 attr_index;
    UINT8 attr_flip; /* 0 index next, 1 data next */
    UINT8 dac_mask;
    UINT8 hidden_dac;
    UINT8 dac_reads; /* reads of 46E0h since another access */
    UINT8 status;    /* 5AE0h: toggles the retrace bits */
    UINT8 padding[3];
    UINT8 seq[0x20];
    UINT8 crtc[0x40];
    UINT8 gr[0x40];
    UINT8 attr[0x20];
} _GA98NB, *GA98NB;

#ifdef __cplusplus
extern "C" {
#endif

extern _GA98NB ga98nb;
extern UINT8 ga98nb_vram[GA98NB_VRAM_SIZE];

void ga98nb_reset(void);
void ga98nb_bind(void);
BOOL ga98nb_enabled(void);
/* The window for CPU address ADDRESS (E0000h-E7FFFh): TRUE and the video
 * memory offset in *OFFSET if it decodes. The caller checks that ROM0 is
 * released to the bus slot. */
BOOL ga98nb_window(UINT32 address, UINT32 *offset);
REG8 ga98nb_window_read(UINT32 offset);
void ga98nb_window_write(UINT32 offset, REG8 value);

#ifdef __cplusplus
}
#endif

#endif /* VAEG_IO_GA98NB_H */
