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

static HF_MOCK mock;

static void summary(void)
{
    unsigned long n;
    printf("MOCK_COUNTS I=%lu O=%lu W=%lu\n", mock.reads, mock.writes8, mock.writes16);
    for (n = 0; n < mock.count && n < 2UL; ++n)
        printf("MOCK_OP %c %04X %04X\n", mock.ops[n].kind,
               (unsigned int)mock.ops[n].port, (unsigned int)mock.ops[n].value);
}

HF_IO hf_platform_io(void)
{
    return hf_mock_backend(&mock);
}

int hf_platform_progress(void *context, unsigned int row,
                         unsigned int total, unsigned long pixels)
{
    const char *abort_at;
    (void)context;
    printf("MOCK_PROGRESS row=%u total=%u pixels=%lu\n", row, total, pixels);
    abort_at = getenv("HF_MOCK_ABORT_ROW");
    return abort_at != 0 && row == (unsigned int)strtoul(abort_at, 0, 10);
}

int hf_cli_main(int argc, char **argv);

int main(int argc, char **argv)
{
    hf_mock_reset(&mock);
    if (atexit(summary) != 0)
        return 99;
    return hf_cli_main(argc, argv);
}
