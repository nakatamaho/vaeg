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
#include <string.h>

void hf_mock_reset(HF_MOCK *m) { memset(m, 0, sizeof(*m)); }

static int log_op(HF_MOCK *m, char kind, unsigned short port, unsigned short value)
{
    if (m->count >= HF_MOCK_OPS)
        return -1;
    m->ops[m->count].kind = kind;
    m->ops[m->count].port = port;
    m->ops[m->count].value = value;
    ++m->count;
    if (kind == 'I') ++m->reads;
    if (kind == 'O') ++m->writes8;
    if (kind == 'W') ++m->writes16;
    return m->count == m->fail_at ? -1 : 0;
}

static int channel(unsigned short port)
{
    if (port == 0xD8) return 0;
    if (port == 0xDA) return 1;
    if (port == 0xDC) return 2;
    return -1;
}

static int mock_in(void *context, unsigned short port, unsigned char *value)
{
    HF_MOCK *m;
    HF_MOCK_READ *entry;
    int c, status;
    unsigned char v;
    m = (HF_MOCK *)context;
    c = channel(port);
    if (c < 0 || value == 0 || m->x >= HF_WIDTH || m->y >= HF_HEIGHT)
        return -1;
    status = 0;
    if (m->script_count != 0U) {
        if (m->script_position >= m->script_count)
            return -1;
        entry = &m->script[m->script_position++];
        if (entry->port != port)
            return -1;
        v = entry->value;
        status = entry->status;
    } else if (m->constant_reads) {
        v = 0xFF;
    } else {
        v = m->pixels[m->alias_coordinates ? 0U : m->y]
                     [m->alias_coordinates ? 0U : m->x][c];
    }
    if (m->count + 1UL == m->corrupt_read_at)
        v ^= 1U;
    if (log_op(m, 'I', port, v) != 0 || status != 0)
        return -1;
    *value = v;
    if (m->advance_after_data)
        m->x = (m->x + 1U) % HF_WIDTH;
    return 0;
}

static int mock_out(void *context, unsigned short port, unsigned char value)
{
    HF_MOCK *m;
    int c;
    m = (HF_MOCK *)context;
    if (log_op(m, 'O', port, value) != 0)
        return -1;
    if (m->count == m->drop_at)
        return 0;
    switch (port) {
    case 0xD7: m->y = (m->y & 255U) | ((unsigned int)value << 8); return 0;
    case 0xD6: m->y = (m->y & 65280U) | value; return 0;
    case 0xD5: m->x = (m->x & 255U) | ((unsigned int)value << 8); return 0;
    case 0xD4: m->x = (m->x & 65280U) | value; return 0;
    case 0xD0: case 0xEE: case 0xEC: return 0;
    default: break;
    }
    c = channel(port);
    if (c < 0 || m->x >= HF_WIDTH || m->y >= HF_HEIGHT)
        return -1;
    m->pixels[m->alias_coordinates ? 0U : m->y]
             [m->alias_coordinates ? 0U : m->x][c] = value;
    if (m->advance_after_data)
        m->x = (m->x + 1U) % HF_WIDTH;
    return 0;
}

static int mock_word(void *context, unsigned short port, unsigned short value)
{
    HF_MOCK *m;
    m = (HF_MOCK *)context;
    if (log_op(m, 'W', port, value) != 0)
        return -1;
    if (m->count == m->drop_at)
        return 0;
    if (port == 0xD6) m->y = value;
    else if (port == 0xD4) m->x = value;
    else return -1;
    return 0;
}

HF_IO hf_mock_backend(HF_MOCK *m)
{
    HF_IO io;
    io.context = m;
    io.in8 = mock_in;
    io.out8 = mock_out;
    io.out16 = mock_word;
    return io;
}
