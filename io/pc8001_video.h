/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_PC8001_VIDEO_H
#define VAEG_PC8001_VIDEO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pure address/pixel primitives for the independent PC-8001 machine path.
 * No VA register aliases, host pointers, ROM payloads or global state.
 * These are not an alternate machine implementation by themselves. */
enum {
	PC8001_ROM_RAM = 0,
	PC8001_ROM_N80 = 1,
	PC8001_ROM_4TH = 2,
	PC8001_ROM_SR = 3,
	PC8001_ROM_SR_IE = 4,
	PC8001_PLANE_BYTES = 0x4000
};

typedef struct {
	unsigned source;
	unsigned offset;
} PC8001_ROM_ADDRESS;

/* Transparent-text geometry contract derived from X88000, corroborated by
 * M88M; physical-machine agreement is unknown. This packet layer does not
 * implement display timing, interrupts, cursor commands or the VA bridge.
 * Wire bytes: valid, applied R4, next reset parameter (5 = idle), five pending
 * parameters. Idle/unreceived parameter bytes are canonical zeroes. */
enum { PC8001_CRTC_GEOMETRY_SIZE = 8 };
typedef struct {
	uint8_t bytes[PC8001_CRTC_GEOMETRY_SIZE];
} PC8001_CRTC_GEOMETRY;
void pc8001_crtc_geometry_reset(PC8001_CRTC_GEOMETRY *state);
void pc8001_crtc_geometry_write(PC8001_CRTC_GEOMETRY *state, unsigned port, uint8_t data);
/* Returns zero until configured, or for non-transparent modes. Width is the
 * 80-byte character region, including when the displayed text is 40 columns. */
unsigned pc8001_crtc_geometry_pairs(const PC8001_CRTC_GEOMETRY *state);
unsigned pc8001_crtc_geometry_pitch(const PC8001_CRTC_GEOMETRY *state);
int pc8001_crtc_geometry_valid(const uint8_t *data, size_t size);
int pc8001_crtc_geometry_load(PC8001_CRTC_GEOMETRY *state, const uint8_t *data, size_t size);

/* Inputs are the raw N80 ports, not translated VA port 31h.
 * sr is the qualified effective N80SR ROM mode (33h bit 7 on an SR model).
 * A missing selected ROM is handled by the caller as open bus, not fallback. */
PC8001_ROM_ADDRESS pc8001_rom_address(uint16_t address, uint8_t port31,
                                    uint8_t port71, int sr);
/* N80 GVRAM is at 8000h-BFFFh, not the PC-8801 C000h-FFFFh window. */
int pc8001_gvram_window(uint16_t address, unsigned plane, int gvam, int gam);
/* 640-dot output coordinates; a 320-dot pixel occupies two output dots.
 * Return raw colour indices. Palette, monochrome text decoration, text
 * priority, display timing and window composition remain caller-owned.
 * The input is three consecutive 16 KiB B/R/G planes. Invalid coordinates
 * return zero without reading the input. */
unsigned pc8001_mkii_pixel(const uint8_t *vram, unsigned x, unsigned y, int dots320);
unsigned pc8001_sr_pixel_strided(const uint8_t *vram, unsigned stride,
                                unsigned x, unsigned y, int dots320,
                                unsigned front_page, uint8_t port53);
unsigned pc8001_sr_pixel(const uint8_t *vram, unsigned x, unsigned y,
                        int dots320, unsigned front_page, uint8_t port53);

#ifdef __cplusplus
}
#endif
#endif
