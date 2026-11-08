/*
 * Board adapter for the independent uPD3301 (PC-8001mkII/SR route).
 * Port mapping follows Manuke's X88000 1.5.3 PC88Z80Main.cpp
 * ReadIO_50/WriteIO_50/WriteIO_51/WriteIO_68 (public domain), adapted to C.
 *
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
#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "cpu/upd9002/memory.h"
#include "n80_crtc.h"

UPD3301 n80_crtc;
PC88_TEXT_DMA n80_crtc_dma;
static PC88_TEXT_MEMORY n80_crtc_text;

/* X88000's text window starts in the N-BASIC main RAM area; the guest
 * firmware normally rewrites port 64h during startup, so the power-on value
 * is a source-model default, to be qualified against real machines. */
#define N80_CRTC_DMA_RESET_ADDRESS 0xf300
#define N80_CRTC_CLOCK_HZ 3993600u

BOOL n80_crtc_active(void) {
	return memoryva_88_mode && memoryn80.model >= 1 && memoryn80.model <= 2;
}

void n80_crtc_reset(void) {
	upd3301_reset(&n80_crtc);
	upd3301_clock_configure(&n80_crtc, N80_CRTC_CLOCK_HZ, 0);
	pc88_text_dma_reset(&n80_crtc_dma, N80_CRTC_DMA_RESET_ADDRESS);
	ZeroMemory(&n80_crtc_text, sizeof(n80_crtc_text));
}

static void IOOUTCALL n80_crtc_o_param(UINT port, REG8 dat) {
	(void)port;
	if (n80_crtc_active()) {
		upd3301_parameter_write(&n80_crtc, (uint8_t)dat);
	}
}

static void IOOUTCALL n80_crtc_o_command(UINT port, REG8 dat) {
	(void)port;
	if (n80_crtc_active()) {
		upd3301_command(&n80_crtc, (uint8_t)dat);
	}
}

static REG8 IOINPCALL n80_crtc_i_param(UINT port) {
	(void)port;
	return n80_crtc_active() ? upd3301_parameter_read(&n80_crtc) : 0;
}

static REG8 IOINPCALL n80_crtc_i_status(UINT port) {
	(void)port;
	return n80_crtc_active() ? upd3301_status(&n80_crtc) : 0;
}

static void IOOUTCALL n80_crtc_o_dma_address(UINT port, REG8 dat) {
	(void)port;
	if (n80_crtc_active()) {
		pc88_text_dma_write(&n80_crtc_dma, 0x64, (uint8_t)dat);
	}
}

static void IOOUTCALL n80_crtc_o_dma_control(UINT port, REG8 dat) {
	(void)port;
	if (n80_crtc_active()) {
		/* As X88000 WriteIO_68: the control byte also gates the CRTC DMA. */
		pc88_text_dma_write(&n80_crtc_dma, 0x68, (uint8_t)dat);
		n80_crtc.dma_started = (uint8_t)pc88_text_dma_enabled(&n80_crtc_dma);
	}
}

void n80_crtc_bind(void) {
	iocore_attachout(0x50, n80_crtc_o_param);
	iocore_attachout(0x51, n80_crtc_o_command);
	iocore_attachinp(0x50, n80_crtc_i_param);
	iocore_attachinp(0x51, n80_crtc_i_status);
	iocore_attachout(0x64, n80_crtc_o_dma_address);
	iocore_attachout(0x68, n80_crtc_o_dma_control);
}

const PC88_TEXT_MEMORY *n80_crtc_memory(void) {
	n80_crtc_text.main_ram = mem;
	n80_crtc_text.main_size = 0x10000;
	n80_crtc_text.fast_ram = NULL;
	n80_crtc_text.fast_size = 0;
	n80_crtc_text.fast_active = 0;
	return &n80_crtc_text;
}

int n80_crtc_state_encode(uint8_t *output, size_t size) {
	static const PC88_DISPLAY_CLOCK_BINDING binding = {N80_CRTC_CLOCK_HZ, 0};
	return pc88_display_state_encode(&n80_crtc, &n80_crtc_dma, &binding, output, size);
}

int n80_crtc_state_decode(const uint8_t *input, size_t size) {
	static const PC88_DISPLAY_CLOCK_BINDING binding = {N80_CRTC_CLOCK_HZ, 0};
	return pc88_display_state_decode(&n80_crtc, &n80_crtc_dma, &binding, input, size);
}
