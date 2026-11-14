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
	if (!memoryva_88_mode) {
		return FALSE;
	}
	/* Independent route for the 8801/8001 family: PC-8801 selections and
	 * the PC-8001mkII/SR models. The plain PC-8001 (model 0) stays on the
	 * firmware path: its TVRAM organisation differs from the mkII's (the
	 * captured boot writes interleaved-looking text) and needs its own
	 * decoder layout before the route can render it. VA/VA2 keep the
	 * firmware/TSP emulation. */
	if (memoryva_pc8801) {
		return TRUE;
	}
	return memoryva_pc8001 && memoryn80.model >= 1 && memoryn80.model <= 2;
}

void n80_crtc_reset(void) {
	upd3301_reset(&n80_crtc);
	upd3301_clock_configure(&n80_crtc, N80_CRTC_CLOCK_HZ, 0);
	pc88_text_dma_reset(&n80_crtc_dma, N80_CRTC_DMA_RESET_ADDRESS);
	ZeroMemory(&n80_crtc_text, sizeof(n80_crtc_text));
	n80_crtc_sync_width();
}

/* The 40/80-column display follows port 30h bit 0 while the CRTC parameters
 * stay at the 80-column geometry, on the PC-8801 as on the PC-8001mkII/SR
 * (M147: the PC-8801's N88-BASIC WIDTH 40 writes 22h to port 30h and keeps
 * 80 CRTC columns, text on every other byte; the boot writes 23h). A
 * sub-80-column parameter geometry is always the doubled byte mode. */
void n80_crtc_sync_width(void) {
	if (!n80_crtc_active()) {
		return;
	}
	n80_crtc.width80 = (uint8_t)(n80_crtc.width80 && upd3301_columns(&n80_crtc) >= 80);
}

static void IOOUTCALL n80_crtc_o_param(UINT port, REG8 dat) {
	(void)port;
	if (n80_crtc_active()) {
		upd3301_parameter_write(&n80_crtc, (uint8_t)dat);
		n80_crtc_sync_width();
	}
}

static void IOOUTCALL n80_crtc_o_command(UINT port, REG8 dat) {
	(void)port;
	if (n80_crtc_active()) {
		const BOOL pending = (n80_crtc.interrupt_request & 1) != 0;
		upd3301_command(&n80_crtc, (uint8_t)dat);
		if (pending && !(n80_crtc.interrupt_request & 1)) {
			/* X88000: RESET/RESET INTERRUPT withdraw the request, so the CPU
			 * must not take a stale vector. The 8214 edge model needs an
			 * explicit withdrawal. */
			pic8214_withdraw(PIC8214_VRTC);
		}
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

void n80_crtc_advance_machine(SINT32 machine_clocks) {
	uint64_t crossed;
	if (!n80_crtc_active() || machine_clocks <= 0 || pccore.realclock == 0) {
		return;
	}
	/* Machine clocks run at pccore.realclock per second; the device counts
	 * cycles at its own crystal. The absolute domain mapping (the 8801/8001
	 * crystal versus the VA-derived machine clock) is pending qualification. */
	if (upd3301_advance(&n80_crtc,
	                    (uint32_t)((UINT64)machine_clocks * N80_CRTC_CLOCK_HZ / pccore.realclock),
	                    &crossed) &&
	    crossed > 0) {
		pic_setirq(2);
	}
}

void n80_crtc_notify_port30(BYTE dat) {
	if (n80_crtc_active()) {
		/* X88000 WriteIO_30 bit 0: the 80/40-column switch, PC-8801 and
		 * PC-8001mkII/SR alike (M147). The port stays owned by the VA
		 * video/cassette handler; the device is notified. */
		n80_crtc.width80 = (uint8_t)(dat & 1);
		n80_crtc_sync_width();
	}
}

void n80_crtc_irq_acknowledge(void) {
	if (n80_crtc_active()) {
		n80_crtc.interrupt_request &= (uint8_t)~1;
	}
}

/* The N80 model maps the guest F000h-FFFFh text window into the TVRAM
 * image (textmem 6000h-6FFFh); the real device DMA reads that memory.
 * Addresses below F000h read the main-RAM backing without side effects. */
static uint8_t n80_crtc_text_peek(void *opaque, uint32_t address) {
	(void)opaque;
	if (address >= 0xf000) {
		return textmem[0x6000 + (address & 0x0fff)];
	}
	return upd9002_mainram_read(0x10000 + address);
}

const PC88_TEXT_MEMORY *n80_crtc_memory(void) {
	n80_crtc_text.main_ram = NULL;
	n80_crtc_text.main_size = 0;
	n80_crtc_text.fast_ram = NULL;
	n80_crtc_text.fast_size = 0;
	n80_crtc_text.fast_active = 0;
	n80_crtc_text.peek = n80_crtc_text_peek;
	n80_crtc_text.peek_opaque = NULL;
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

BOOL n80_crtc_disp_state_save(BYTE *out, UINT size) {
	return n80_crtc_state_encode(out, size) == PC88_STATE_OK;
}

BOOL n80_crtc_disp_state_valid(const BYTE *in, UINT size) {
	UPD3301 candidate;
	PC88_TEXT_DMA dma;
	static const PC88_DISPLAY_CLOCK_BINDING binding = {N80_CRTC_CLOCK_HZ, 0};

	if (!in || size != N80_DISP_STATE_SIZE) {
		return FALSE;
	}
	/* Validate without committing. */
	return pc88_display_state_decode(&candidate, &dma, &binding, in, size) == PC88_STATE_OK;
}

BOOL n80_crtc_disp_state_load(const BYTE *in, UINT size) {
	return n80_crtc_state_decode(in, size) == PC88_STATE_OK;
}
