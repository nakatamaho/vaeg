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
#include "compiler.h"
#include "g75_screen.h"

#include "cpucva/memoryva.h"
#include "iova/tsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_u32(FILE *fp, UINT32 value) {
    BYTE bytes[4];

    bytes[0] = (BYTE)(value & 0xff);
    bytes[1] = (BYTE)((value >> 8) & 0xff);
    bytes[2] = (BYTE)((value >> 16) & 0xff);
    bytes[3] = (BYTE)((value >> 24) & 0xff);
    (void)fwrite(bytes, 1, sizeof(bytes), fp);
}

BOOL g75_screen_harness_exit_requested(UINT32 elapsed_ms) {
    const char *value;
    char *end;
    unsigned long limit;

    value = getenv("VAEG_SCREEN_EXIT_MS");
    if ((value == NULL) || (value[0] == '\0')) {
        return FALSE;
    }
    limit = strtoul(value, &end, 10);
    if ((end == value) || (*end != '\0') || (limit > 0xffffffffUL)) {
        return FALSE;
    }
    return (elapsed_ms >= (UINT32)limit) ? TRUE : FALSE;
}

void g75_screen_capture(void) {
    const char *path;
    const char *run_id;
    UINT32 run_id_length;
    FILE *fp;

    path = getenv("VAEG_SCREEN_DUMP");
    if ((path == NULL) || (path[0] == '\0')) {
        return;
    }
    run_id = getenv("VAEG_SCREEN_RUN_ID");
    if (run_id == NULL) {
        run_id = "";
    }
    run_id_length = (UINT32)strlen(run_id);
    if (run_id_length > 4096) {
        run_id_length = 4096;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "scsitrace screen-dump-open-failed path=%s\n", path);
        return;
    }
    (void)fwrite("VAEGSCN1", 1, 8, fp);
    write_u32(fp, 1);
    write_u32(fp, run_id_length);
    (void)fwrite(run_id, 1, run_id_length, fp);
    write_u32(fp, tsp.texttable);
    write_u32(fp, tsp.attroffset);
    write_u32(fp, tsp.lineheight);
    write_u32(fp, tsp.curn);
    write_u32(fp, tsp.sprtable);
    write_u32(fp, tsp.be);
    write_u32(fp, sizeof(textmem));
    (void)fwrite(textmem, 1, sizeof(textmem), fp);
    if (fclose(fp) != 0) {
        fprintf(stderr, "scsitrace screen-dump-close-failed path=%s\n", path);
        return;
    }
    fprintf(stderr, "scsitrace screen-dump path=%s run_id=%.*s\n",
            path, (int)run_id_length, run_id);
}
