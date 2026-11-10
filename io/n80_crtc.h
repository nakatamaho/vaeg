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
#ifndef VAEG_N80_CRTC_H
#define VAEG_N80_CRTC_H
#include "compiler.h"
#include "io/pc88_display_state.h"
#include "io/pc88_text_dma.h"
#include "io/upd3301.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Board adapter for the independent uPD3301 on the PC-8801 and
 * PC-8001mkII/SR selections. It owns no VA state: the VA/VA2 firmware/TSP
 * path is separate. Ports (X88000 1.5.3 PC88Z80Main.cpp ReadIO_50/WriteIO_50/51/68):
 *   IN  50h  CRTC parameter read      OUT 50h  CRTC parameter write
 *   IN  51h  CRTC status read         OUT 51h  CRTC command write
 *   OUT 64h  text DMA address byte    OUT 68h  text DMA control
 * Handlers are attached unconditionally and gate on the live model, so the
 * binding order versus model selection does not matter. */
extern UPD3301 n80_crtc;
extern PC88_TEXT_DMA n80_crtc_dma;

/* TRUE on the independent 8801/8001-family route in 88-mode guest execution. */
BOOL n80_crtc_active(void);
void n80_crtc_reset(void);
void n80_crtc_bind(void);
/* Advance the device by machine-clock-domain time (pccore.realclock units)
 * and raise the VRTC interrupt (pic_setirq(2)) on each device frame boundary,
 * per the source model's unconditional per-frame request. */
void n80_crtc_advance_machine(SINT32 machine_clocks);
/* The compat CPU acknowledged the VRTC level: clear the device's vsync
 * request (X88000 DoInterrupt case 0x02). No effect on VA machines. */
void n80_crtc_irq_acknowledge(void);
/* Port 30h (VA video/cassette handler owns the port) bit 0: 80/40 switch. */
void n80_crtc_notify_port30(BYTE dat);
/* Derive the displayed width for the PC-8801 role from the device params. */
void n80_crtc_sync_width(void);
/* The independent machine's text-source view for rendering/state code. */
const PC88_TEXT_MEMORY *n80_crtc_memory(void);
/* Component codec gate; machine-identity guards stay with the SAV layer. */
#define N80_DISP_STATE_SIZE ((UINT)PC88_DISPLAY_STATE_BYTES)
int n80_crtc_state_encode(uint8_t *output, size_t size);
int n80_crtc_state_decode(const uint8_t *input, size_t size);
/* SAV section helpers: encode/validate/commit without publishing the codec. */
BOOL n80_crtc_disp_state_save(BYTE *out, UINT size);
BOOL n80_crtc_disp_state_valid(const BYTE *in, UINT size);
BOOL n80_crtc_disp_state_load(const BYTE *in, UINT size);
#ifdef __cplusplus
}
#endif
#endif
