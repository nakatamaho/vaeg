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
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef HF_MOCK_IO_H
#define HF_MOCK_IO_H
#include "hf_io.h"
#define HF_MOCK_OPS 3900000UL
#define HF_MOCK_SCRIPT 64U

typedef struct hf_mock_op {
    char kind;
    unsigned short port, value;
} HF_MOCK_OP;
typedef struct hf_mock_read {
    unsigned short port;
    unsigned char value;
    int status;
} HF_MOCK_READ;
typedef struct hf_mock {
    unsigned char pixels[HF_HEIGHT][HF_WIDTH][3];
    HF_MOCK_OP ops[HF_MOCK_OPS];
    unsigned long count, reads, writes8, writes16;
    unsigned long fail_at, drop_at, corrupt_read_at;
    unsigned int x, y;
    int alias_coordinates, constant_reads, advance_after_data;
    HF_MOCK_READ script[HF_MOCK_SCRIPT];
    unsigned int script_count, script_position;
} HF_MOCK;
void hf_mock_reset(HF_MOCK *mock);
HF_IO hf_mock_backend(HF_MOCK *mock);
#endif
