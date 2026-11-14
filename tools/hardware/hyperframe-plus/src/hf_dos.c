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
#if !defined(__WATCOMC__) || !defined(__DOS__) || defined(__386__)
#error This backend requires 16-bit real-mode Open Watcom DOS
#endif

#include "hf_io.h"
#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

typedef char hf_require_16_bit_int[sizeof(int) == 2 ? 1 : -1];

static int dos_in8(void *context, unsigned short port, unsigned char *value)
{
    (void)context;
    if (value == 0 || (port != 0xD8 && port != 0xDA && port != 0xDC))
        return -1;
    *value = (unsigned char)inp(port);
    return 0;
}

static int dos_out8(void *context, unsigned short port, unsigned char value)
{
    (void)context;
    /* EC is deliberately absent: memory bank/aperture tests are blocked. */
    switch (port) {
    case 0xD0: case 0xEE:
        if (value != 0)
            return -1;
        break;
    case 0xD4: case 0xD5: case 0xD6: case 0xD7:
    case 0xD8: case 0xDA: case 0xDC:
        break;
    default:
        return -1;
    }
    (void)outp(port, value);
    return 0;
}

static int dos_out16(void *context, unsigned short port, unsigned short value)
{
    (void)context;
    if ((port == 0xD4 && value < 640U) || (port == 0xD6 && value < 400U)) {
        (void)outpw(port, value);
        return 0;
    }
    return -1;
}

HF_IO hf_platform_io(void)
{
    HF_IO io;
    io.context = 0;
    io.in8 = dos_in8;
    io.out8 = dos_out8;
    io.out16 = dos_out16;
    return io;
}

int hf_platform_progress(void *context, unsigned int row,
                         unsigned int total, unsigned long pixels)
{
    union REGS input, output;
    (void)context;
    if ((row % 8U) == 0U || row == total)
        printf("Progress %u/%u rows, %lu pixels\n", row, total, pixels);
    if (row == total)
        return 0;
    /* DOS 3+ nonblocking console input. AL=0 also covers no key or an extended
     * key prefix. No blocking read, IBM INT 16h, or VGA BIOS is used here. */
    memset(&input, 0, sizeof(input));
    input.h.ah = 0x06;
    input.h.dl = 0xFF;
    (void)int86(0x21, &input, &output);
    return output.h.al == 27 || output.h.al == 3;
}
