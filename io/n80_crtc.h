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

/* Board adapter for the independent uPD3301 on the PC-8001mkII/SR models.
 * It owns no VA state: the VA/VA2 firmware/TSP path is separate. Ports
 * (X88000 1.5.3 PC88Z80Main.cpp ReadIO_50/WriteIO_50/51/68):
 *   IN  50h  CRTC parameter read      OUT 50h  CRTC parameter write
 *   IN  51h  CRTC status read         OUT 51h  CRTC command write
 *   OUT 64h  text DMA address byte    OUT 68h  text DMA control
 * Handlers are attached unconditionally and gate on the live model, so the
 * binding order versus model selection does not matter. */
extern UPD3301 n80_crtc;
extern PC88_TEXT_DMA n80_crtc_dma;

/* TRUE on the independent mkII/SR route in 88-mode guest execution. */
BOOL n80_crtc_active(void);
void n80_crtc_reset(void);
void n80_crtc_bind(void);
/* The independent machine's text-source view for rendering/state code. */
const PC88_TEXT_MEMORY *n80_crtc_memory(void);
/* Component codec gate; machine-identity guards stay with the SAV layer. */
int n80_crtc_state_encode(uint8_t *output, size_t size);
int n80_crtc_state_decode(const uint8_t *input, size_t size);
#ifdef __cplusplus
}
#endif
#endif
