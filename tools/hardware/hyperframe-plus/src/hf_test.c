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
#include <stdio.h>
#include <string.h>

#ifndef HF_BUILD_ID
#define HF_BUILD_ID unversioned
#endif
#define HF_STRINGIFY_INNER(value) #value
#define HF_STRINGIFY(value) HF_STRINGIFY_INNER(value)

HF_IO hf_platform_io(void);
int hf_platform_progress(void *, unsigned int, unsigned int, unsigned long);

static void help(void)
{
    puts("HFTEST: experimental HyPER-FRAME+ PC-9801 DOS tests");
    puts("Usage: HFTEST [help|info|off|readback [word]|pattern [full]|init|banktest]");
    puts("Default/help/info: no board I/O. No reliable board auto-detection.");
    puts("off: explicit D0=00, EE=00 writes; not a probe or state restoration.");
    puts("readback: save two pixels, test distinct colors, restore and verify.");
    puts("readback word: experimental word coordinate OUTs; equivalence unproven.");
    puts("pattern: overwrite 32x32 pixels. full: overwrite 640x400 pixels.");
    puts("Patterns are NOT restored. ESC abort is checked at row boundaries.");
    puts("init: BLOCKED until the original binary's conditional flow is verified.");
    puts("banktest: BLOCKED; no aperture memory access or real bank selection.");
    puts("This is NOT a PC-88VA tool. Use only an identified PC-9801-compatible");
    puts("host with the correct installed card and exclusive ownership of its ports.");
    puts("Readback/pattern require an independently initialized pixel I/O mode.");
    puts("No command selects a monitor mode or proves output is visible.");
    puts("Exit: 0 completed; 1 mismatch; 2 blocked/cancelled; 3 usage; 4 I/O/restore.");
}

static int confirm(const char *token)
{
    char line[40];
    size_t n;
    printf("Type %s to continue; ENTER cancels: ", token);
    fflush(stdout);
    if (fgets(line, sizeof(line), stdin) == 0) {
        putchar('\n');
        return 0;
    }
    putchar('\n');
    n = strlen(line);
    if (n == 0U || (line[n - 1U] != '\n' && line[n - 1U] != '\r'))
        return 0;
    while (n != 0U && (line[n - 1U] == '\n' || line[n - 1U] == '\r'))
        line[--n] = '\0';
    return strcmp(line, token) == 0;
}

static int finish(int status)
{
    printf("HF_RESULT=%s\n", hf_status_name(status));
    if (status == HF_OK)
        return 0;
    if (status == HF_MISMATCH)
        return 1;
    if (status == HF_ARGUMENT)
        return 3;
    if (status == HF_IO_ERROR || status == HF_RESTORE_FAILED)
        return 4;
    return 2;
}

static void print_rgb(const HF_RGB *rgb)
{
    printf("%02X/%02X/%02X", (unsigned int)rgb->r,
           (unsigned int)rgb->g, (unsigned int)rgb->b);
}

int main(int argc, char **argv)
{
    HF_DEVICE device;
    HF_IO io;
    HF_POINT points[2];
    HF_RGB values[2];
    HF_READBACK_REPORT readback;
    HF_PATTERN_REPORT pattern;
    unsigned int permissions, i;
    int command, word, full, status;
    printf("HFTEST build %s; hardware status: UNVERIFIED\n", HF_STRINGIFY(HF_BUILD_ID));
    command = 0;
    word = 0;
    full = 0;
    if (argc == 1 || (argc == 2 &&
        (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "info") == 0))) {
        help();
        return finish(HF_OK);
    }
    if (argc == 2 && (strcmp(argv[1], "init") == 0 ||
                      strcmp(argv[1], "banktest") == 0)) {
        puts("BLOCKED: unqualified initialization/aperture. No board I/O performed.");
        return finish(HF_UNSUPPORTED);
    }
    if (argc == 2 && strcmp(argv[1], "off") == 0)
        command = 1;
    if ((argc == 2 || argc == 3) && strcmp(argv[1], "readback") == 0) {
        command = 2;
        if (argc == 3) {
            if (strcmp(argv[2], "word") != 0)
                return finish(HF_ARGUMENT);
            word = 1;
        }
    }
    if ((argc == 2 || argc == 3) && strcmp(argv[1], "pattern") == 0) {
        command = 3;
        if (argc == 3) {
            if (strcmp(argv[2], "full") != 0)
                return finish(HF_ARGUMENT);
            full = 1;
        }
    }
    if (command == 0) {
        help();
        return finish(HF_ARGUMENT);
    }
    puts("WARNING: this writes physical I/O ports. No card-presence check exists.");
    puts("A wrong card, conflicting port, unknown mode or bad hardware may cause");
    puts("unrecoverable display/state changes. Power/reset loss prevents cleanup.");
    puts("Confirm PC-9801-compatible host, correct card, safe cabling and no conflicts.");
    if (command == 1)
        puts("Disable sequence is task-supplied binary evidence, not yet locally reverified.");
    if (!confirm("PC98-HF"))
        return finish(HF_ABORTED);
    permissions = HF_ALLOW_IO;
    if (command != 1) {
        puts("Confirm pixel I/O mode was independently initialized. Unknown means STOP.");
        if (!confirm("PIXEL-READY"))
            return finish(HF_UNSAFE);
        permissions |= HF_PIXEL_READY;
    }
    if (word) {
        puts("Word OUT is NOT proven equivalent to byte address writes on this hardware.");
        if (!confirm("WORD-OUT"))
            return finish(HF_ABORTED);
        permissions |= HF_ALLOW_WORD;
    }
    if (command == 3) {
        puts("WARNING: the selected pattern rectangle will be overwritten, NOT restored.");
        if (!confirm(full ? "FULL-FRAME" : "DRAW-32"))
            return finish(HF_ABORTED);
    }
    io = hf_platform_io();
    status = hf_device_init(&device, &io);
    if (status != HF_OK)
        return finish(status);
    (void)hf_authorize(&device, permissions);
    if (word) {
        status = hf_word_address(&device, 1);
        if (status != HF_OK)
            return finish(status);
    }
    if (command == 1) {
        status = hf_disable(&device);
        puts("Writes attempted. Prior register state was not saved or restored.");
        return finish(status);
    }
    if (command == 2) {
        points[0].x = 0; points[0].y = 0;
        points[1].x = 1; points[1].y = 0;
        values[0].r = 0x12; values[0].g = 0x34; values[0].b = 0x56;
        values[1].r = 0xA9; values[1].g = 0xCB; values[1].b = 0xED;
        status = hf_readback_two(&device, points, 2, values, 2, &readback);
        for (i = 0; i < 2U; ++i) {
            printf("PIXEL %d,%d expected=", points[i].x, points[i].y);
            print_rgb(&values[i]);
            printf(" observed=");
            if (readback.observed_mask & (1U << i))
                print_rgb(&readback.observed[i]);
            else
                printf("NOT_READ");
            printf(" saved=");
            if (readback.saved_mask & (1U << i))
                print_rgb(&readback.saved[i]);
            else
                printf("NOT_SAVED");
            printf(" restore=%s\n", (readback.restore_verified_mask & (1U << i)) ?
                   "READBACK_MATCH" : "NOT_VERIFIED");
        }
        printf("Primary=%s restore_attempted=%u restore_verified=%u\n",
               hf_status_name(readback.primary_status),
               readback.restore_attempted_mask, readback.restore_verified_mask);
        puts("This checks selected pixel bytes only, not complete state or video output.");
        puts("Failure does NOT diagnose a defective board. FF can also be valid pixel data.");
        return finish(status);
    }
    status = hf_pattern(&device, 0, 0, full ? 640U : 32U, full ? 400U : 32U,
                        hf_platform_progress, 0, &pattern);
    printf("Rows=%u pixels=%lu; previous pixels NOT restored; display NOT_VERIFIED\n",
           pattern.complete_rows, pattern.pixels_written);
    return finish(status);
}
