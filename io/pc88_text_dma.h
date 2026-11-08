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
 * Board-side text DMA subset adapted from Manuke's public-domain X88000 1.5.3
 * PC88Z80Main.cpp WriteIO_64/68 and PC88Z80Main.h GetTVRam. See ADR-0016.
 */
#ifndef VAEG_PC88_TEXT_DMA_H
#define VAEG_PC88_TEXT_DMA_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* This is not a complete 8257, a bus scheduler, or part of the uPD3301 LSI.
 * Count/status/other-channel ports are deliberately not claimed. Initialize
 * before use; this struct is not a SAV wire format. The board supplies the
 * reset address rather than assuming one ROM or one BASIC startup mode. */
typedef struct {
	uint16_t address;
	uint8_t high_next, mode;
} PC88_TEXT_DMA;

typedef struct {
	const uint8_t *main_ram;
	size_t main_size;
	const uint8_t *fast_ram;
	size_t fast_size;
	int fast_active;
} PC88_TEXT_MEMORY;

typedef struct {
	uint16_t offset;
	uint8_t fast;
} PC88_TEXT_ADDRESS;

void pc88_text_dma_reset(PC88_TEXT_DMA *dma, uint16_t initial_address);
/* Exact 8-bit port numbers only; return zero for an unimplemented port. */
int pc88_text_dma_write(PC88_TEXT_DMA *dma, unsigned port, uint8_t value);
int pc88_text_dma_enabled(const PC88_TEXT_DMA *dma);
PC88_TEXT_ADDRESS pc88_text_dma_resolve(const PC88_TEXT_DMA *dma, uint32_t offset, int fast_active);
/* A read-only backing-memory view, not a CPU-bus access or DMA transfer.
 * Independent of DMA enable so display-copy/debug consumers can inspect data.
 * Return zero on a missing/short selected source; leave output unchanged.
 * Never silently fall back from selected fast RAM to main RAM. */
int pc88_text_dma_peek(const PC88_TEXT_DMA *dma, const PC88_TEXT_MEMORY *memory, uint32_t offset,
                       uint8_t *value);
#ifdef __cplusplus
}
#endif
#endif
