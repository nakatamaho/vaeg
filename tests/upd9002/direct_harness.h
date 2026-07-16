/*
 * Copyright (c) 2026 Nakata Maho
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
#ifndef VAEG_TESTS_UPD9002_DIRECT_HARNESS_H
#define VAEG_TESTS_UPD9002_DIRECT_HARNESS_H

#include <stdint.h>

#define UPD9002_HARNESS_RAM_CAPACITY 4096
#define UPD9002_HARNESS_PROGRAM_CAPACITY 32

typedef struct {
	uint16_t ax;
	uint16_t bx;
	uint16_t cx;
	uint16_t dx;
	uint16_t si;
	uint16_t di;
	uint16_t bp;
	uint16_t sp;
	uint16_t es;
	uint16_t cs;
	uint16_t ss;
	uint16_t ds;
	uint16_t ip;
	uint16_t flags;
	uint32_t es_base;
	uint32_t cs_base;
	uint32_t ss_base;
	uint32_t ds_base;
	int32_t remain_clock;
	int32_t base_clock;
	uint32_t clock;
} UPD9002_HARNESS_CPU_STATE;

typedef struct {
	UPD9002_HARNESS_CPU_STATE cpu;
	uint32_t ram_address;
	uint32_t ram_size;
	uint8_t ram[UPD9002_HARNESS_RAM_CAPACITY];
	uint32_t program_address;
	uint32_t program_size;
	uint8_t program[UPD9002_HARNESS_PROGRAM_CAPACITY];
	uint32_t step_count;
} UPD9002_HARNESS_INPUT;

typedef struct {
	UPD9002_HARNESS_CPU_STATE cpu;
	uint32_t ram_hash;
	uint32_t executed_steps;
	uint8_t termination;
} UPD9002_HARNESS_RESULT;

int upd9002_harness_run(const UPD9002_HARNESS_INPUT *input,
						UPD9002_HARNESS_RESULT *result);
int upd9002_harness_run_manifest(const char *path);

#endif
