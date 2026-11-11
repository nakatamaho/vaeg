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
#ifndef VAEG_PC88_DISPLAY_STATE_H
#define VAEG_PC88_DISPLAY_STATE_H
#include "upd3301.h"
#include "pc88_text_dma.h"
#ifdef __cplusplus
extern "C" {
#endif

enum {
	PC88_DISPLAY_STATE_BYTES = 32,
	PC88_DISPLAY_STATE_VERSION = 1
};
enum {
	PC88_STATE_OK = 0,
	PC88_STATE_ARGUMENT = 1,
	PC88_STATE_SIZE = 2,
	PC88_STATE_VERSION = 3,
	PC88_STATE_FLAGS = 4,
	PC88_STATE_COMMAND = 5,
	PC88_STATE_INTERRUPT = 6,
	PC88_STATE_ATTRIBUTE = 7,
	PC88_STATE_CLOCK = 8,
	PC88_STATE_DMA = 9,
	PC88_STATE_BINDING = 10
};
/* Supplied by the board, never inferred from the untrusted payload. This
 * binds only the clock profile; machine identity/native-VA guards and other
 * board registers must also be checked by the eventual SAV integration. */
typedef struct {
	uint32_t clock_hz;
	uint8_t hires;
} PC88_DISPLAY_CLOCK_BINDING;

/* Independently callable content and clock-binding checks. */
int pc88_display_state_valid(const UPD3301 *crtc, const PC88_TEXT_DMA *dma);
int pc88_display_clock_matches(const UPD3301 *crtc, const PC88_DISPLAY_CLOCK_BINDING *binding);
/* Explicit 32-byte little-endian payload, never a native struct dump.
 * Decode/encode validate before committing output. No machine SAV section,
 * legacy migration, RAM payload or global device instance is registered here. */
int pc88_display_state_encode(const UPD3301 *crtc, const PC88_TEXT_DMA *dma,
                              const PC88_DISPLAY_CLOCK_BINDING *binding, uint8_t *output,
                              size_t size);
int pc88_display_state_decode(UPD3301 *crtc, PC88_TEXT_DMA *dma,
                              const PC88_DISPLAY_CLOCK_BINDING *binding, const uint8_t *input,
                              size_t size);
#ifdef __cplusplus
}
#endif
#endif
