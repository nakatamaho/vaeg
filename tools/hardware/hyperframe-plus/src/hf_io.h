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
#ifndef HF_IO_H
#define HF_IO_H

#include <limits.h>

#if CHAR_BIT != 8 || USHRT_MAX != 65535U || ULONG_MAX < 4294967295UL
#error Unsupported integer representation
#endif

#define HF_WIDTH 640
#define HF_HEIGHT 400
#define HF_BANK_BYTES 131072UL
#define HF_ALLOW_IO 1U
#define HF_PIXEL_READY 2U
#define HF_ALLOW_WORD 4U
#define HF_ALLOW_BANK 8U
#define HF_PERMISSION_MASK 15U

enum hf_status {
    HF_OK = 0, HF_ARGUMENT, HF_UNSAFE, HF_UNSUPPORTED, HF_IO_ERROR,
    HF_MISMATCH, HF_RESTORE_FAILED, HF_ABORTED
};

typedef struct hf_io {
    void *context;
    int (*in8)(void *, unsigned short, unsigned char *);
    int (*out8)(void *, unsigned short, unsigned char);
    int (*out16)(void *, unsigned short, unsigned short);
} HF_IO;

typedef struct hf_device {
    HF_IO io;
    unsigned int permissions;
    int word_address;
} HF_DEVICE;

typedef struct hf_rgb { unsigned short r, g, b; } HF_RGB;
typedef struct hf_point { int x, y; } HF_POINT;

typedef struct hf_readback_report {
    HF_RGB saved[2], observed[2], restored[2];
    unsigned int saved_mask, observed_mask, restore_attempted_mask;
    unsigned int restore_verified_mask;
    int primary_status;
    int restore_write_status[2], restore_read_status[2];
} HF_READBACK_REPORT;

typedef struct hf_span {
    unsigned char bank_value;
    unsigned long bank_offset;
    unsigned int length, buffer_offset;
    unsigned short segment, offset;
} HF_SPAN;

typedef struct hf_pattern_report {
    unsigned long pixels_written;
    unsigned int complete_rows;
} HF_PATTERN_REPORT;

typedef int (*HF_PROGRESS)(void *, unsigned int, unsigned int, unsigned long);

/* Backends return zero on completed operations, nonzero on transport failure.
 * A successful physical OUT cannot prove that hardware accepted the value. */
int hf_device_init(HF_DEVICE *device, const HF_IO *io);
int hf_authorize(HF_DEVICE *device, unsigned int permissions);
int hf_word_address(HF_DEVICE *device, int enable);
/* RGB capacities/counts are numbers of HF_RGB objects, not byte counts. */
int hf_get_pixel(HF_DEVICE *device, int x, int y, HF_RGB *rgb,
                 unsigned int capacity);
int hf_put_pixel(HF_DEVICE *device, int x, int y, const HF_RGB *rgb,
                 unsigned int count);
int hf_readback_two(HF_DEVICE *device, const HF_POINT *points,
                    unsigned int point_count, const HF_RGB *values,
                    unsigned int value_count, HF_READBACK_REPORT *report);
int hf_disable(HF_DEVICE *device);
int hf_pattern(HF_DEVICE *device, int x, int y, unsigned int width,
                unsigned int height, HF_PROGRESS progress, void *context,
                HF_PATTERN_REPORT *report);
/* Pure arithmetic. No memory access or I/O is performed by these planners. */
int hf_bank_value(unsigned int channel, unsigned int half, unsigned char *value);
int hf_plan_line(unsigned int channel, int y, int x, unsigned int length,
                  HF_SPAN *spans, unsigned int capacity, unsigned int *used);
/* Experimental; the real DOS backend deliberately rejects port EC. */
int hf_select_bank(HF_DEVICE *device, unsigned int channel, unsigned int half);
const char *hf_status_name(int status);

#endif
