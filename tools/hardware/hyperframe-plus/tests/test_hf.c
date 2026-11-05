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
#include "hf_mock_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HF_MOCK mock;
static HF_DEVICE device;
static unsigned long checks;
#define CHECK(c) do { ++checks; if (!(c)) { \
    fprintf(stderr, "HF_TEST_FAILED line=%d expression=%s\n", __LINE__, #c); \
    exit(1); } } while (0)

static HF_POINT points[2] = {{0, 0}, {1, 0}};
static HF_RGB colors[2] = {{0x12, 0x34, 0x56}, {0xA9, 0xCB, 0xED}};

static void fixture(void)
{
    HF_IO io;
    hf_mock_reset(&mock);
    mock.pixels[0][0][0] = 0x10;
    mock.pixels[0][0][1] = 0x20;
    mock.pixels[0][0][2] = 0x30;
    mock.pixels[0][1][0] = 0x40;
    mock.pixels[0][1][1] = 0x50;
    mock.pixels[0][1][2] = 0x60;
    io = hf_mock_backend(&mock);
    CHECK(hf_device_init(&device, &io) == HF_OK);
    CHECK(hf_authorize(&device, HF_ALLOW_IO | HF_PIXEL_READY) == HF_OK);
}

static void op(unsigned long n, char kind, unsigned short port,
               unsigned short value)
{
    CHECK(n < mock.count);
    CHECK(mock.ops[n].kind == kind);
    CHECK(mock.ops[n].port == port);
    CHECK(mock.ops[n].value == value);
}

static void original_pixels(void)
{
    CHECK(mock.pixels[0][0][0] == 0x10);
    CHECK(mock.pixels[0][0][1] == 0x20);
    CHECK(mock.pixels[0][0][2] == 0x30);
    CHECK(mock.pixels[0][1][0] == 0x40);
    CHECK(mock.pixels[0][1][1] == 0x50);
    CHECK(mock.pixels[0][1][2] == 0x60);
}

static void pixels(void)
{
    HF_RGB read;
    unsigned short expected[3] = {0x12, 0x34, 0x56};
    unsigned int i;
    fixture();
    CHECK(hf_put_pixel(&device, 639, 399, colors, 1) == HF_OK);
    CHECK(mock.count == 15UL);
    CHECK(hf_get_pixel(&device, 639, 399, &read, 1) == HF_OK);
    CHECK(read.r == 0x12 && read.g == 0x34 && read.b == 0x56);
    for (i = 0; i < 6U; ++i) {
        op(i * 5U, 'O', 0xD7, 1); op(i * 5U + 1U, 'O', 0xD6, 0x8F);
        op(i * 5U + 2U, 'O', 0xD5, 2); op(i * 5U + 3U, 'O', 0xD4, 0x7F);
        op(i * 5U + 4U, i < 3U ? 'O' : 'I',
           (unsigned short)(0xD8U + 2U * (i % 3U)), expected[i % 3U]);
    }
    fixture();
    mock.script_count = 3;
    mock.script[0].port = 0xD8; mock.script[0].value = 7;
    mock.script[1].port = 0xDA; mock.script[1].value = 8;
    mock.script[2].port = 0xDC; mock.script[2].value = 9;
    CHECK(hf_get_pixel(&device, 0, 0, &read, 1) == HF_OK);
    CHECK(read.r == 7 && read.g == 8 && read.b == 9);
    CHECK(mock.script_position == 3U);
    puts("PASS byte pixel order, reads and scripted inputs");
}

static void negatives(void)
{
    HF_POINT bad[4] = {{-1, 0}, {640, 0}, {0, -1}, {0, 400}};
    HF_RGB color, read;
    unsigned int i;
    fixture();
    CHECK(hf_get_pixel(&device, 0, 0, &read, 1) == HF_OK);
    for (i = 0; i < 4U; ++i) {
        fixture();
        CHECK(hf_get_pixel(&device, bad[i].x, bad[i].y, &read, 1) == HF_ARGUMENT);
        CHECK(mock.count == 0UL);
        CHECK(hf_put_pixel(&device, bad[i].x, bad[i].y, colors, 1) == HF_ARGUMENT);
        CHECK(mock.count == 0UL);
    }
    for (i = 0; i < 3U; ++i) {
        fixture(); color = colors[0];
        if (i == 0U) color.r = 256;
        if (i == 1U) color.g = 256;
        if (i == 2U) color.b = 256;
        CHECK(hf_put_pixel(&device, 0, 0, &color, 1) == HF_ARGUMENT);
        CHECK(mock.count == 0UL);
    }
    fixture();
    CHECK(hf_get_pixel(&device, 0, 0, &read, 0) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    CHECK(hf_get_pixel(&device, 0, 0, 0, 1) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    CHECK(hf_put_pixel(&device, 0, 0, colors, 0) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    CHECK(hf_put_pixel(&device, 0, 0, 0, 1) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    fixture(); device.permissions = 0;
    CHECK(hf_get_pixel(&device, 0, 0, &read, 1) == HF_UNSAFE);
    CHECK(hf_put_pixel(&device, 0, 0, colors, 1) == HF_UNSAFE);
    CHECK(hf_disable(&device) == HF_UNSAFE);
    CHECK(hf_select_bank(&device, 0, 0) == HF_UNSAFE);
    CHECK(mock.count == 0UL);
    fixture(); device.permissions = HF_ALLOW_IO;
    CHECK(hf_put_pixel(&device, 0, 0, colors, 1) == HF_UNSAFE);
    CHECK(mock.count == 0UL);
    fixture();
    CHECK(hf_authorize(&device, HF_PERMISSION_MASK + 1U) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    puts("PASS bounds, component, buffer and permission negatives");
}

static void readback(void)
{
    HF_READBACK_REPORT r;
    HF_POINT p[2];
    HF_RGB c[2];
    unsigned long n;
    unsigned int i;
    int status;
    fixture();
    CHECK(hf_readback_two(&device, points, 2, colors, 2, &r) == HF_OK);
    CHECK(mock.count == 150UL && mock.reads == 18UL && mock.writes8 == 132UL);
    CHECK(r.saved_mask == 3U && r.observed_mask == 3U);
    CHECK(r.restore_attempted_mask == 3U && r.restore_verified_mask == 3U);
    original_pixels();
    fixture(); mock.advance_after_data = 1;
    CHECK(hf_readback_two(&device, points, 2, colors, 2, &r) == HF_OK);
    original_pixels();
    fixture(); mock.corrupt_read_at = 65;
    CHECK(hf_readback_two(&device, points, 2, colors, 2, &r) == HF_MISMATCH);
    CHECK(r.primary_status == HF_MISMATCH && r.restore_verified_mask == 3U);
    original_pixels();
    fixture(); mock.corrupt_read_at = 125;
    CHECK(hf_readback_two(&device, points, 2, colors, 2, &r) == HF_RESTORE_FAILED);
    CHECK(r.primary_status == HF_OK && r.restore_attempted_mask == 3U);
    fixture(); mock.drop_at = 95;
    CHECK(hf_readback_two(&device, points, 2, colors, 2, &r) == HF_RESTORE_FAILED);
    CHECK(r.primary_status == HF_OK && r.restore_verified_mask == 2U);
    fixture(); mock.alias_coordinates = 1;
    CHECK(hf_readback_two(&device, points, 2, colors, 2, &r) == HF_MISMATCH);
    fixture(); mock.constant_reads = 1;
    CHECK(hf_readback_two(&device, points, 2, colors, 2, &r) == HF_MISMATCH);
    /* Each failure is a single mutation of the passing 150-operation fixture. */
    for (n = 1; n <= 150UL; ++n) {
        fixture(); mock.fail_at = n;
        status = hf_readback_two(&device, points, 2, colors, 2, &r);
        if (n <= 30UL) {
            CHECK(status == HF_IO_ERROR && r.restore_attempted_mask == 0U);
            original_pixels();
            for (i = 0; i < (unsigned int)mock.count; ++i)
                CHECK(mock.ops[i].kind != 'O' ||
                      (mock.ops[i].port != 0xD8 && mock.ops[i].port != 0xDA &&
                       mock.ops[i].port != 0xDC));
        } else if (n <= 90UL) {
            CHECK(status == HF_IO_ERROR && r.primary_status == HF_IO_ERROR);
            CHECK(r.restore_attempted_mask == 3U && r.restore_verified_mask == 3U);
            original_pixels();
        } else {
            CHECK(status == HF_RESTORE_FAILED && r.primary_status == HF_OK);
            CHECK(r.restore_attempted_mask == 3U && r.restore_verified_mask != 3U);
        }
    }
    fixture();
    CHECK(hf_readback_two(&device, points, 1, colors, 2, &r) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    CHECK(hf_readback_two(&device, points, 2, colors, 1, &r) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    CHECK(hf_readback_two(&device, 0, 2, colors, 2, &r) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    CHECK(hf_readback_two(&device, points, 2, 0, 2, &r) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    CHECK(hf_readback_two(&device, points, 2, colors, 2, 0) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    p[0] = points[0]; p[1] = points[0];
    CHECK(hf_readback_two(&device, p, 2, colors, 2, &r) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    c[0] = colors[0]; c[1] = colors[0];
    CHECK(hf_readback_two(&device, points, 2, c, 2, &r) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    p[1] = points[1]; p[1].y = 403;
    CHECK(hf_readback_two(&device, p, 2, colors, 2, &r) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    puts("PASS two-pixel cleanup, address advance, alias/FF and all 150 I/O failures");
}

static void words_and_disable(void)
{
    HF_READBACK_REPORT r;
    fixture();
    CHECK(hf_word_address(&device, 1) == HF_UNSAFE);
    CHECK(mock.count == 0UL);
    CHECK(hf_authorize(&device, HF_ALLOW_IO | HF_PIXEL_READY | HF_ALLOW_WORD) == HF_OK);
    CHECK(hf_word_address(&device, 1) == HF_OK);
    CHECK(hf_put_pixel(&device, 639, 399, colors, 1) == HF_OK);
    CHECK(mock.count == 9UL);
    op(0, 'W', 0xD6, 399); op(1, 'W', 0xD4, 639); op(2, 'O', 0xD8, 0x12);
    op(3, 'W', 0xD6, 399); op(4, 'W', 0xD4, 639); op(5, 'O', 0xDA, 0x34);
    op(6, 'W', 0xD6, 399); op(7, 'W', 0xD4, 639); op(8, 'O', 0xDC, 0x56);
    fixture();
    CHECK(hf_authorize(&device, HF_ALLOW_IO | HF_PIXEL_READY | HF_ALLOW_WORD) == HF_OK);
    CHECK(hf_word_address(&device, 1) == HF_OK);
    CHECK(hf_readback_two(&device, points, 2, colors, 2, &r) == HF_OK);
    CHECK(mock.count == 90UL && mock.writes16 == 60UL && mock.writes8 == 12UL);
    original_pixels();
    fixture();
    CHECK(hf_authorize(&device, HF_ALLOW_IO | HF_ALLOW_WORD) == HF_OK);
    device.io.out16 = 0;
    CHECK(hf_word_address(&device, 1) == HF_UNSAFE && mock.count == 0UL);
    fixture();
    CHECK(hf_disable(&device) == HF_OK && mock.count == 2UL);
    op(0, 'O', 0xD0, 0); op(1, 'O', 0xEE, 0);
    fixture(); mock.fail_at = 1;
    CHECK(hf_disable(&device) == HF_IO_ERROR && mock.count == 2UL);
    op(1, 'O', 0xEE, 0);
    puts("PASS word transaction distinction and explicit disable order");
}

static void banks(void)
{
    HF_SPAN spans[2];
    unsigned int channel, half, used, y, x, i;
    unsigned char value;
    unsigned long linear, physical;
    fixture();
    CHECK(hf_authorize(&device, HF_ALLOW_IO | HF_ALLOW_BANK) == HF_OK);
    for (channel = 0; channel < 3U; ++channel)
        for (half = 0; half < 2U; ++half) {
            CHECK(hf_bank_value(channel, half, &value) == HF_OK);
            CHECK(value == 0xC0U + channel * 2U + half);
            CHECK(hf_select_bank(&device, channel, half) == HF_OK);
            op(channel * 2U + half, 'O', 0xEC, value);
        }
    fixture();
    CHECK(hf_select_bank(&device, 0, 0) == HF_UNSAFE && mock.count == 0UL);
    CHECK(hf_bank_value(3, 0, &value) == HF_ARGUMENT);
    CHECK(hf_bank_value(0, 2, &value) == HF_ARGUMENT);
    CHECK(hf_bank_value(0, 0, 0) == HF_ARGUMENT);
    CHECK(hf_plan_line(0, 204, 0, 640, spans, 2, &used) == HF_OK);
    CHECK(used == 2U && spans[0].length == 512U && spans[1].length == 128U);
    CHECK(spans[0].bank_offset == 130560UL && spans[1].bank_offset == 0UL);
    CHECK(spans[0].buffer_offset == 0U && spans[1].buffer_offset == 512U);
    CHECK(spans[0].segment == 0x9FE0 && spans[1].segment == 0x8000);
    CHECK(hf_plan_line(0, 204, 511, 2, spans, 2, &used) == HF_OK);
    CHECK(used == 2U && spans[0].length == 1U && spans[1].length == 1U);
    CHECK(spans[0].bank_offset == 131071UL && spans[1].bank_offset == 0UL);
    CHECK(hf_plan_line(0, 204, 512, 128, spans, 2, &used) == HF_OK);
    CHECK(used == 1U && spans[0].bank_value == 0xC1 && spans[0].bank_offset == 0UL);
    CHECK(hf_plan_line(0, 205, 0, 640, spans, 2, &used) == HF_OK);
    CHECK(used == 1U && spans[0].bank_offset == 128UL && spans[0].segment == 0x8008);
    CHECK(hf_plan_line(2, 399, 639, 1, spans, 2, &used) == HF_OK);
    CHECK(spans[0].bank_value == 0xC5 && spans[0].bank_offset == 124927UL);
    CHECK(spans[0].segment == 0x9E7F && spans[0].offset == 15U);
    /* Exhaust all drawable pixels: pure arithmetic, never mapped memory. */
    for (y = 0; y < 400U; ++y)
        for (x = 0; x < 640U; ++x) {
            CHECK(hf_plan_line(1, (int)y, (int)x, 1, spans, 2, &used) == HF_OK);
            linear = (unsigned long)y * 640UL + x;
            CHECK(used == 1U && spans[0].length == 1U);
            CHECK(spans[0].bank_offset == linear % 131072UL);
            CHECK(spans[0].bank_value == 0xC2U + linear / 131072UL);
            physical = (unsigned long)spans[0].segment * 16UL + spans[0].offset;
            CHECK(physical == 0x80000UL + linear % 131072UL);
        }
    for (i = 0; i < 400U; ++i) {
        CHECK(hf_plan_line(0, (int)i, 0, 640, spans, 2, &used) == HF_OK);
        CHECK(used == (i == 204U ? 2U : 1U));
    }
    CHECK(hf_plan_line(0, 204, 0, 640, spans, 1, &used) == HF_ARGUMENT);
    CHECK(hf_plan_line(0, 0, 0, 0, spans, 2, &used) == HF_ARGUMENT);
    CHECK(hf_plan_line(0, 0, 1, 640, spans, 2, &used) == HF_ARGUMENT);
    CHECK(hf_plan_line(0, 0, 0, 640, 0, 2, &used) == HF_ARGUMENT);
    CHECK(hf_plan_line(0, 0, 0, 640, spans, 2, 0) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    puts("PASS six banks, row-204 split and exhaustive segment arithmetic");
}

static int stop_row(void *context, unsigned int row, unsigned int total,
                     unsigned long pixels_written)
{
    unsigned int *stop;
    stop = (unsigned int *)context;
    CHECK(row <= total);
    CHECK(pixels_written == (unsigned long)row * 4UL);
    return row == *stop;
}

static void patterns(void)
{
    HF_PATTERN_REPORT r;
    unsigned int stop, x, y, bar, level;
    fixture(); stop = 2;
    CHECK(hf_pattern(&device, 0, 0, 4, 4, stop_row, &stop, &r) == HF_ABORTED);
    CHECK(r.complete_rows == 2U && r.pixels_written == 8UL && mock.count == 120UL);
    fixture(); stop = 0;
    CHECK(hf_pattern(&device, 0, 0, 4, 4, stop_row, &stop, &r) == HF_ABORTED);
    CHECK(mock.count == 0UL && r.pixels_written == 0UL);
    fixture();
    CHECK(hf_pattern(&device, 0, 0, 640, 400, 0, 0, &r) == HF_OK);
    CHECK(r.pixels_written == 256000UL && r.complete_rows == 400U);
    CHECK(mock.count == 3840000UL && mock.writes16 == 0UL && mock.reads == 0UL);
    for (y = 0; y < 400U; ++y)
        for (x = 0; x < 640U; ++x) {
            bar = x / 80U;
            level = (((x / 8U) ^ (y / 8U)) & 1U) ? 127U : 255U;
            CHECK(mock.pixels[y][x][0] == ((bar & 4U) ? level : 0U));
            CHECK(mock.pixels[y][x][1] == ((bar & 2U) ? level : 0U));
            CHECK(mock.pixels[y][x][2] == ((bar & 1U) ? level : 0U));
        }
    fixture();
    CHECK(hf_pattern(&device, 0, 0, 0, 1, 0, 0, &r) == HF_ARGUMENT);
    CHECK(hf_pattern(&device, 639, 0, 2, 1, 0, 0, &r) == HF_ARGUMENT);
    CHECK(hf_pattern(&device, 0, 399, 1, 2, 0, 0, &r) == HF_ARGUMENT);
    CHECK(hf_pattern(&device, 0, 0, 640, 400, 0, 0, 0) == HF_ARGUMENT);
    CHECK(mock.count == 0UL);
    fixture(); mock.fail_at = 5;
    CHECK(hf_pattern(&device, 0, 0, 640, 400, 0, 0, &r) == HF_IO_ERROR);
    CHECK(r.pixels_written == 0UL && mock.count == 5UL);
    puts("PASS bounded patterns, full synthetic image, abort and validation");
}

int main(void)
{
    pixels(); negatives(); readback(); words_and_disable(); banks(); patterns();
    printf("PASS %lu assertions; mock contracts only, hardware NOT_RUN\n", checks);
    return 0;
}
