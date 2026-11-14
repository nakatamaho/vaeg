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
 *
 * Independent uPD3301 command-state adaptation from Manuke's public-domain
 * X88000 1.5.3 PC88Crtc.cpp/.h. See ADR-0016 for the pinned archive and scope.
 */
#ifndef VAEG_UPD3301_H
#define VAEG_UPD3301_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	UPD3301_IDLE = 0,
	UPD3301_RESET = 1,
	UPD3301_LIGHT_PEN = 2,
	UPD3301_CURSOR = 3
};

/* Device-owned state, never an alias of TSP or VA firmware working memory.
 * Not a wire format: the eventual SAV codec must serialize fields explicitly.
 * The clock scheduler, DMA bus and rendering adapters are not connected yet.
 * Callers must initialize with upd3301_reset; these APIs do not decode or
 * validate untrusted saved-state bytes. */
typedef struct {
	uint8_t reset[5];
	uint8_t command, parameter;
	uint8_t cursor_x, cursor_y, cursor_enabled;
	uint8_t display_started, dma_started, reverse, width80;
	uint8_t interrupt_mask, interrupt_request, last_attribute;
	uint16_t frames;
	/* Device-domain ticks scaled by 1,000,000, not host time or CPU batches. */
	uint32_t clock_hz;
	uint64_t clock_remaining;
	uint8_t hires;
} UPD3301;

void upd3301_reset(UPD3301 *crtc);
void upd3301_command(UPD3301 *crtc, uint8_t value);
void upd3301_parameter_write(UPD3301 *crtc, uint8_t value);
uint8_t upd3301_parameter_read(UPD3301 *crtc);
uint8_t upd3301_status(const UPD3301 *crtc);
/* Explicit boundary notification resets the vertical countdown. Normal
 * clocked operation uses upd3301_advance instead; never call both for a frame. */
void upd3301_frame_boundary(UPD3301 *crtc);
/* Source-model vertical timings only: no horizontal scanout or WAIT costs.
 * Configure restarts the countdown and frame counter, retaining IRQ latches.
 * Invalid configuration/advance is atomic (including the frames output). */
int upd3301_clock_configure(UPD3301 *crtc, uint32_t clock_hz, int hires);
/* Numeric clock guard only, not full controller/model/SAV validation. */
int upd3301_clock_valid(const UPD3301 *crtc);
unsigned upd3301_cycle_us(const UPD3301 *crtc);
unsigned upd3301_sync_us(const UPD3301 *crtc);
int upd3301_in_vsync(const UPD3301 *crtc);
int upd3301_advance(UPD3301 *crtc, uint32_t ticks, uint64_t *frames);
unsigned upd3301_columns(const UPD3301 *crtc);
unsigned upd3301_rows(const UPD3301 *crtc);
unsigned upd3301_scanlines(const UPD3301 *crtc);
unsigned upd3301_attribute_pairs(const UPD3301 *crtc);
unsigned upd3301_cursor_period(const UPD3301 *crtc);
int upd3301_display_active(const UPD3301 *crtc);
int upd3301_cursor_phase(const UPD3301 *crtc);
int upd3301_attribute_phase(const UPD3301 *crtc);

#ifdef __cplusplus
}
#endif
#endif
