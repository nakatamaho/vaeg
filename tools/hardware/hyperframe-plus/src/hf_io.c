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
#include "hf_io.h"
#include <string.h>

static int point_valid(int x, int y)
{
    return x >= 0 && x < HF_WIDTH && y >= 0 && y < HF_HEIGHT;
}

static int rgb_valid(const HF_RGB *rgb)
{
    return rgb != 0 && rgb->r <= 255U && rgb->g <= 255U && rgb->b <= 255U;
}

static int rgb_equal(const HF_RGB *a, const HF_RGB *b)
{
    return a->r == b->r && a->g == b->g && a->b == b->b;
}

static int ready(const HF_DEVICE *d, unsigned int required)
{
    if (d == 0 || d->io.in8 == 0 || d->io.out8 == 0)
        return HF_ARGUMENT;
    if ((d->permissions & required) != required)
        return HF_UNSAFE;
    if (d->word_address &&
        (!(d->permissions & HF_ALLOW_WORD) || d->io.out16 == 0))
        return HF_UNSAFE;
    return HF_OK;
}

static int out_byte(HF_DEVICE *d, unsigned short port, unsigned char value)
{
    return d->io.out8(d->io.context, port, value) == 0 ? HF_OK : HF_IO_ERROR;
}

static int address(HF_DEVICE *d, int x, int y)
{
    if (d->word_address) {
        if (d->io.out16(d->io.context, 0xD6, (unsigned short)y) != 0)
            return HF_IO_ERROR;
        if (d->io.out16(d->io.context, 0xD4, (unsigned short)x) != 0)
            return HF_IO_ERROR;
    } else {
        /* Published List 1: Y high, Y low, X high, X low. */
        if (out_byte(d, 0xD7, (unsigned char)((unsigned int)y >> 8)) != HF_OK ||
            out_byte(d, 0xD6, (unsigned char)y) != HF_OK ||
            out_byte(d, 0xD5, (unsigned char)((unsigned int)x >> 8)) != HF_OK ||
            out_byte(d, 0xD4, (unsigned char)x) != HF_OK)
            return HF_IO_ERROR;
    }
    return HF_OK;
}

int hf_device_init(HF_DEVICE *d, const HF_IO *io)
{
    if (d == 0 || io == 0 || io->in8 == 0 || io->out8 == 0)
        return HF_ARGUMENT;
    d->io = *io;
    d->permissions = 0;
    d->word_address = 0;
    return HF_OK;
}

int hf_authorize(HF_DEVICE *d, unsigned int permissions)
{
    if (d == 0 || (permissions & ~HF_PERMISSION_MASK) != 0U)
        return HF_ARGUMENT;
    d->permissions = permissions;
    if (!(permissions & HF_ALLOW_WORD))
        d->word_address = 0;
    return HF_OK;
}

int hf_word_address(HF_DEVICE *d, int enable)
{
    if (d == 0 || (enable != 0 && enable != 1))
        return HF_ARGUMENT;
    if (enable && (!(d->permissions & HF_ALLOW_WORD) || d->io.out16 == 0))
        return HF_UNSAFE;
    d->word_address = enable;
    return HF_OK;
}

int hf_get_pixel(HF_DEVICE *d, int x, int y, HF_RGB *rgb, unsigned int capacity)
{
    static const unsigned short ports[3] = {0xD8, 0xDA, 0xDC};
    unsigned char components[3];
    unsigned int i;
    int status;
    if (!point_valid(x, y) || rgb == 0 || capacity < 1U)
        return HF_ARGUMENT;
    status = ready(d, HF_ALLOW_IO | HF_PIXEL_READY);
    if (status != HF_OK)
        return status;
    /* List 1 selects coordinates before EACH component. Do not assume that
     * a data access leaves the address unchanged. */
    for (i = 0; i < 3U; ++i) {
        status = address(d, x, y);
        if (status != HF_OK)
            return status;
        if (d->io.in8(d->io.context, ports[i], &components[i]) != 0)
            return HF_IO_ERROR;
    }
    rgb->r = components[0];
    rgb->g = components[1];
    rgb->b = components[2];
    return HF_OK;
}

static int put(HF_DEVICE *d, int x, int y, const HF_RGB *rgb, int best_effort)
{
    static const unsigned short ports[3] = {0xD8, 0xDA, 0xDC};
    unsigned short components[3];
    unsigned int i;
    int status, next;
    components[0] = rgb->r;
    components[1] = rgb->g;
    components[2] = rgb->b;
    status = HF_OK;
    for (i = 0; i < 3U; ++i) {
        next = address(d, x, y);
        /* Never output data after an uncertain coordinate selection. Cleanup
         * can still reselect the coordinates for the next component. */
        if (next == HF_OK)
            next = out_byte(d, ports[i], (unsigned char)components[i]);
        if (next != HF_OK) {
            status = next;
            if (!best_effort)
                return status;
        }
    }
    return status;
}

int hf_put_pixel(HF_DEVICE *d, int x, int y, const HF_RGB *rgb, unsigned int count)
{
    int status;
    if (!point_valid(x, y) || count < 1U || !rgb_valid(rgb))
        return HF_ARGUMENT;
    status = ready(d, HF_ALLOW_IO | HF_PIXEL_READY);
    if (status != HF_OK)
        return status;
    return put(d, x, y, rgb, 0);
}

int hf_readback_two(HF_DEVICE *d, const HF_POINT *points,
                    unsigned int point_count, const HF_RGB *values,
                    unsigned int value_count, HF_READBACK_REPORT *r)
{
    unsigned int i;
    int status, mismatch;
    if (r == 0)
        return HF_ARGUMENT;
    memset(r, 0, sizeof(*r));
    r->primary_status = HF_ARGUMENT;
    for (i = 0; i < 2U; ++i) {
        r->restore_write_status[i] = HF_UNSUPPORTED;
        r->restore_read_status[i] = HF_UNSUPPORTED;
    }
    if (points == 0 || values == 0 || point_count != 2U || value_count != 2U)
        return HF_ARGUMENT;
    for (i = 0; i < 2U; ++i)
        if (!point_valid(points[i].x, points[i].y) || !rgb_valid(&values[i]))
            return HF_ARGUMENT;
    if ((points[0].x == points[1].x && points[0].y == points[1].y) ||
        rgb_equal(&values[0], &values[1]))
        return HF_ARGUMENT;
    status = ready(d, HF_ALLOW_IO | HF_PIXEL_READY);
    r->primary_status = status;
    if (status != HF_OK)
        return status;
    /* Back up BOTH pixels before any pixel data output. Address writes remain. */
    for (i = 0; i < 2U; ++i) {
        status = hf_get_pixel(d, points[i].x, points[i].y, &r->saved[i], 1);
        if (status != HF_OK) {
            r->primary_status = status;
            return status;
        }
        r->saved_mask |= 1U << i;
    }
    for (i = 0; i < 2U; ++i) {
        status = hf_put_pixel(d, points[i].x, points[i].y, &values[i], 1);
        if (status != HF_OK)
            goto restore;
    }
    mismatch = 0;
    for (i = 0; i < 2U; ++i) {
        status = hf_get_pixel(d, points[i].x, points[i].y, &r->observed[i], 1);
        if (status != HF_OK)
            goto restore;
        r->observed_mask |= 1U << i;
        if (!rgb_equal(&values[i], &r->observed[i]))
            mismatch = 1;
    }
    status = mismatch ? HF_MISMATCH : HF_OK;
restore:
    r->primary_status = status;
    /* Continue with the second saved pixel even if the first cleanup fails. */
    for (i = 0; i < 2U; ++i) {
        r->restore_attempted_mask |= 1U << i;
        r->restore_write_status[i] = put(d, points[i].x, points[i].y,
                                          &r->saved[i], 1);
    }
    for (i = 0; i < 2U; ++i) {
        r->restore_read_status[i] = hf_get_pixel(d, points[i].x, points[i].y,
                                                 &r->restored[i], 1);
        if (r->restore_write_status[i] == HF_OK &&
            r->restore_read_status[i] == HF_OK &&
            rgb_equal(&r->saved[i], &r->restored[i]))
            r->restore_verified_mask |= 1U << i;
    }
    if (r->restore_verified_mask != 3U)
        return HF_RESTORE_FAILED;
    return status;
}

int hf_disable(HF_DEVICE *d)
{
    int status, next;
    status = ready(d, HF_ALLOW_IO);
    if (status != HF_OK)
        return status;
    status = out_byte(d, 0xD0, 0);
    /* Best effort for both outputs; this is NOT restoration of prior state. */
    next = out_byte(d, 0xEE, 0);
    return status != HF_OK ? status : next;
}

int hf_pattern(HF_DEVICE *d, int x, int y, unsigned int width,
                unsigned int height, HF_PROGRESS progress, void *context,
                HF_PATTERN_REPORT *r)
{
    unsigned int row, col, bar, level;
    HF_RGB color;
    int status;
    if (r == 0)
        return HF_ARGUMENT;
    r->pixels_written = 0;
    r->complete_rows = 0;
    if (!point_valid(x, y) || width == 0U || height == 0U ||
        width > (unsigned int)(HF_WIDTH - x) ||
        height > (unsigned int)(HF_HEIGHT - y))
        return HF_ARGUMENT;
    status = ready(d, HF_ALLOW_IO | HF_PIXEL_READY);
    if (status != HF_OK)
        return status;
    for (row = 0; row < height; ++row) {
        if (progress != 0 && progress(context, row, height, r->pixels_written))
            return HF_ABORTED;
        for (col = 0; col < width; ++col) {
            bar = (unsigned int)((unsigned long)col * 8UL / width);
            level = (((col / 8U) ^ (row / 8U)) & 1U) ? 127U : 255U;
            color.r = (unsigned short)((bar & 4U) ? level : 0U);
            color.g = (unsigned short)((bar & 2U) ? level : 0U);
            color.b = (unsigned short)((bar & 1U) ? level : 0U);
            status = hf_put_pixel(d, x + (int)col, y + (int)row, &color, 1);
            if (status != HF_OK)
                return status;
            ++r->pixels_written;
        }
        r->complete_rows = row + 1U;
    }
    if (progress != 0)
        (void)progress(context, height, height, r->pixels_written);
    return HF_OK;
}

int hf_bank_value(unsigned int channel, unsigned int half, unsigned char *value)
{
    if (channel > 2U || half > 1U || value == 0)
        return HF_ARGUMENT;
    *value = (unsigned char)(0xC0U + 2U * channel + half);
    return HF_OK;
}

int hf_plan_line(unsigned int channel, int y, int x, unsigned int length,
                  HF_SPAN *spans, unsigned int capacity, unsigned int *used)
{
    HF_SPAN temp[2];
    unsigned long linear, offset, available;
    unsigned int count, left, copied, take, i;
    if (channel > 2U || !point_valid(x, y) || length == 0U ||
        length > (unsigned int)(HF_WIDTH - x) || spans == 0 || used == 0)
        return HF_ARGUMENT;
    linear = (unsigned long)y * 640UL + (unsigned long)x;
    left = length;
    count = 0;
    copied = 0;
    while (left != 0U) {
        offset = linear % HF_BANK_BYTES;
        available = HF_BANK_BYTES - offset;
        take = available < (unsigned long)left ? (unsigned int)available : left;
        (void)hf_bank_value(channel, (unsigned int)(linear / HF_BANK_BYTES),
                             &temp[count].bank_value);
        temp[count].bank_offset = offset;
        temp[count].length = take;
        temp[count].buffer_offset = copied;
        temp[count].segment = (unsigned short)(0x8000UL + (offset >> 4));
        temp[count].offset = (unsigned short)(offset & 15UL);
        ++count;
        copied += take;
        left -= take;
        linear += take;
    }
    if (capacity < count)
        return HF_ARGUMENT;
    for (i = 0; i < count; ++i)
        spans[i] = temp[i];
    *used = count;
    return HF_OK;
}

int hf_select_bank(HF_DEVICE *d, unsigned int channel, unsigned int half)
{
    unsigned char value;
    int status;
    status = hf_bank_value(channel, half, &value);
    if (status != HF_OK)
        return status;
    status = ready(d, HF_ALLOW_IO | HF_ALLOW_BANK);
    if (status != HF_OK)
        return status;
    return out_byte(d, 0xEC, value);
}

const char *hf_status_name(int status)
{
    switch (status) {
    case HF_OK: return "HF_OK";
    case HF_ARGUMENT: return "HF_ARGUMENT";
    case HF_UNSAFE: return "HF_UNSAFE";
    case HF_UNSUPPORTED: return "HF_UNSUPPORTED";
    case HF_IO_ERROR: return "HF_IO_ERROR";
    case HF_MISMATCH: return "HF_MISMATCH";
    case HF_RESTORE_FAILED: return "HF_RESTORE_FAILED";
    case HF_ABORTED: return "HF_ABORTED";
    default: return "HF_INVALID_STATUS";
    }
}
