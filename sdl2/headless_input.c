/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "sdlapi.h"
#include "dosio.h"
#include "kbdpaste.h"
#include "headless_input.h"
#include "fdd/diskdrv.h"
#include "cpucva/memoryva.h"
#include "io/tsp.h"
#include "upd9002_trace.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    HEADLESS_INPUT_INITIAL_DELAY_FRAMES = 600,
    HEADLESS_INPUT_COMMAND_DELAY_FRAMES = 120,
    HEADLESS_INPUT_COMMAND_MAX = 256,
    HEADLESS_INPUT_FILE_MAX = 64 * 1024,
    HEADLESS_INPUT_PROMPT_TIMEOUT_FRAMES = 12000
};

static void headless_input_command_clear(HEADLESS_INPUT_COMMAND *command) {

    if (command == NULL) {
        return;
    }
    if (command->text != NULL) {
        SDL_free(command->text);
    }
    ZeroMemory(command, sizeof(*command));
}

void headless_input_script_clear(HEADLESS_INPUT_SCRIPT *script) {

    UINT index;

    if (script == NULL) {
        return;
    }
    if (script->commands != NULL) {
        for (index = 0; index < script->command_count; index++) {
            headless_input_command_clear(&script->commands[index]);
        }
        SDL_free(script->commands);
    }
    ZeroMemory(script, sizeof(*script));
}

static BOOL headless_input_script_reserve(HEADLESS_INPUT_SCRIPT *script) {

    if (script->commands != NULL) {
        return SUCCESS;
    }
    script->commands = (HEADLESS_INPUT_COMMAND *)SDL_calloc(
        HEADLESS_INPUT_COMMAND_MAX, sizeof(*script->commands));
    return (script->commands != NULL) ? SUCCESS : FAILURE;
}

static BOOL headless_input_script_add_text(HEADLESS_INPUT_SCRIPT *script,
                                           const char *line, UINT length,
                                           BOOL append_return) {
    HEADLESS_INPUT_COMMAND *command;
    char *text;
    UINT extra;

    if (script->command_count >= HEADLESS_INPUT_COMMAND_MAX) {
        fprintf(stderr,
            "Error: --headless-input-script has too many commands; max=%u\n",
            HEADLESS_INPUT_COMMAND_MAX);
        return FAILURE;
    }
    if (headless_input_script_reserve(script) != SUCCESS) {
        fprintf(stderr, "Error: could not allocate input-script commands\n");
        return FAILURE;
    }
    extra = append_return ? 1 : 0;
    text = (char *)SDL_malloc((size_t)length + extra + 1);
    if (text == NULL) {
        fprintf(stderr, "Error: could not allocate input-script text\n");
        return FAILURE;
    }
    if (length != 0) {
        CopyMemory(text, line, length);
    }
    if (append_return) {
        text[length] = '\r';
    }
    text[length + extra] = '\0';
    command = &script->commands[script->command_count++];
    command->text = text;
    return SUCCESS;
}

static BOOL headless_input_script_add_disk_swap(
        HEADLESS_INPUT_SCRIPT *script, UINT drive, const char *path,
        UINT length) {
    HEADLESS_INPUT_COMMAND *command;
    char *text;

    if ((length == 0) || (script->command_count >= HEADLESS_INPUT_COMMAND_MAX)) {
        fprintf(stderr,
            "Error: headless input disk swap needs a path and a free command slot\n");
        return FAILURE;
    }
    if (headless_input_script_reserve(script) != SUCCESS) {
        fprintf(stderr, "Error: could not allocate input script commands\n");
        return FAILURE;
    }
    text = (char *)SDL_malloc((size_t)length + 1);
    if (text == NULL) {
        fprintf(stderr, "Error: could not allocate input script disk path\n");
        return FAILURE;
    }
    CopyMemory(text, path, length);
    text[length] = '\0';
    command = &script->commands[script->command_count++];
    command->text = text;
    command->disk_drive = drive;
    command->disk_swap = TRUE;
    return SUCCESS;
}

static UINT32 headless_input_basic_prompt_signature(UINT32 *count) {
    UINT32 table;
    UINT32 width;
    UINT32 text_start;
    UINT32 rows;
    UINT32 row;
    UINT32 signature;

    *count = 0;
    if (!tsp.dspon || (tsp.lineheight == 0)) {
        return 0;
    }
    table = tsp.texttable;
    if ((table + 0x1c) >= sizeof(textmem)) {
        return 0;
    }
    width = LOADINTELWORD(textmem + table + 0x08);
    text_start = LOADINTELWORD(textmem + table + 0x10);
    if ((width < 2) || (text_start >= sizeof(textmem)) ||
        ((text_start + width) > sizeof(textmem))) {
        return 0;
    }
    signature = 2166136261U;
    rows = tsp.screenlines / tsp.lineheight;
    if (rows > 40) {
        rows = 40;
    }
    for (row = 0; row < rows; row++) {
        UINT32 address = text_start + row * width;
        UINT32 columns = width / 2;
        UINT32 column;

        if ((address + width) > sizeof(textmem)) {
            break;
        }
        if (columns > 80) {
            columns = 80;
        }
        for (column = 0; column + 1 < columns; column++) {
            UINT16 first = LOADINTELWORD(textmem + address + column * 2);
            UINT16 second = LOADINTELWORD(
                textmem + address + (column + 1) * 2);
            if ((first == 0x004f) && (second == 0x006b)) {
                signature ^= row;
                signature *= 16777619U;
                signature ^= column;
                signature *= 16777619U;
                (*count)++;
            }
        }
    }
    return (*count != 0) ? signature : 0;
}

static BOOL headless_input_script_add_prompt_wait(
        HEADLESS_INPUT_SCRIPT *script) {
    HEADLESS_INPUT_COMMAND *command;

    if (script->command_count >= HEADLESS_INPUT_COMMAND_MAX) {
        fprintf(stderr,
            "Error: --headless-input-script has too many commands; max=%u\n",
            HEADLESS_INPUT_COMMAND_MAX);
        return FAILURE;
    }
    if (headless_input_script_reserve(script) != SUCCESS) {
        fprintf(stderr, "Error: could not allocate input-script commands\n");
        return FAILURE;
    }
    command = &script->commands[script->command_count++];
    command->wait_prompt = TRUE;
    return SUCCESS;
}

static BOOL headless_input_script_add_wait(HEADLESS_INPUT_SCRIPT *script,
                                           UINT wait_frames) {
    HEADLESS_INPUT_COMMAND *command;

    if (script->command_count >= HEADLESS_INPUT_COMMAND_MAX) {
        fprintf(stderr,
            "Error: --headless-input-script has too many commands; max=%u\n",
            HEADLESS_INPUT_COMMAND_MAX);
        return FAILURE;
    }
    if (headless_input_script_reserve(script) != SUCCESS) {
        fprintf(stderr, "Error: could not allocate input-script commands\n");
        return FAILURE;
    }
    command = &script->commands[script->command_count++];
    command->wait = TRUE;
    command->wait_frames = wait_frames;
    return SUCCESS;
}

static BOOL headless_input_script_add_exit(HEADLESS_INPUT_SCRIPT *script) {
    HEADLESS_INPUT_COMMAND *command;

    if (script->command_count >= HEADLESS_INPUT_COMMAND_MAX) {
        fprintf(stderr,
            "Error: --headless-input-script has too many commands; max=%u\n",
            HEADLESS_INPUT_COMMAND_MAX);
        return FAILURE;
    }
    if (headless_input_script_reserve(script) != SUCCESS) {
        fprintf(stderr, "Error: could not allocate input-script commands\n");
        return FAILURE;
    }
    command = &script->commands[script->command_count++];
    command->exit = TRUE;
    return SUCCESS;
}

static BOOL headless_input_script_add_line(HEADLESS_INPUT_SCRIPT *script,
                                           const char *line, UINT length) {
    char *value;
    char *end;
    unsigned long wait_frames;

    if ((length >= 6) &&
        ((memcmp(line, "@fdd1 ", 6) == 0) ||
         (memcmp(line, "@fdd2 ", 6) == 0))) {
        UINT drive = (line[4] == '1') ? 0 : 1;
        return headless_input_script_add_disk_swap(
            script, drive, line + 6, length - 6);
    }
    if (((length == 7) && (memcmp(line, "@prompt", 7) == 0)) ||
        ((length == 12) && (memcmp(line, "@wait-prompt", 12) == 0))) {
        return headless_input_script_add_prompt_wait(script);
    }
    if ((length == 5) && (memcmp(line, "@exit", 5) == 0)) {
        return headless_input_script_add_exit(script);
    }
    if ((length >= 6) && (memcmp(line, "@wait ", 6) == 0)) {
        value = (char *)SDL_malloc((size_t)length - 5);
        if (value == NULL) {
            return FAILURE;
        }
        CopyMemory(value, line + 6, length - 6);
        value[length - 6] = '\0';
        errno = 0;
        end = NULL;
        wait_frames = strtoul(value, &end, 10);
        if ((errno != 0) || (end == value) || (*end != '\0') ||
            (wait_frames > 0xffffffffUL)) {
            SDL_free(value);
            fprintf(stderr,
                "Error: --headless-input-script @wait needs a frame count\n");
            return FAILURE;
        }
        SDL_free(value);
        return headless_input_script_add_wait(script, (UINT)wait_frames);
    }
    if ((length == 6) && (memcmp(line, "@enter", 6) == 0)) {
        return headless_input_script_add_text(script, "\r", 1, FALSE);
    }
    return headless_input_script_add_text(script, line, length, TRUE);
}

static BOOL headless_input_script_parse(HEADLESS_INPUT_SCRIPT *script,
                                        char *buffer, UINT size) {
    UINT position;
    UINT start;
    UINT end;

    position = 0;
    if ((size >= 3) && ((UINT8)buffer[0] == 0xef) &&
        ((UINT8)buffer[1] == 0xbb) && ((UINT8)buffer[2] == 0xbf)) {
        position = 3;
    }
    while (position < size) {
        while ((position < size) &&
            ((buffer[position] == '\r') || (buffer[position] == '\n'))) {
            position++;
        }
        start = position;
        while ((position < size) && (buffer[position] != '\r') &&
            (buffer[position] != '\n')) {
            position++;
        }
        end = position;
        while ((end > start) && ((buffer[end - 1] == ' ') ||
            (buffer[end - 1] == '\t'))) {
            end--;
        }
        while ((start < end) && ((buffer[start] == ' ') ||
            (buffer[start] == '\t'))) {
            start++;
        }
        if ((start == end) || (buffer[start] == '#')) {
            continue;
        }
        if (headless_input_script_add_line(script, buffer + start,
            end - start) != SUCCESS) {
            return FAILURE;
        }
    }
    if ((script->command_count == 0) || (script->commands == NULL)) {
        fprintf(stderr, "Error: --headless-input-script contains no commands\n");
        return FAILURE;
    }
    return SUCCESS;
}

BOOL headless_input_script_load(HEADLESS_INPUT_SCRIPT *script,
                                const char *path) {
    FILEH handle;
    UINT size;
    char *buffer;
    BOOL result;

    if ((script == NULL) || (path == NULL) || (path[0] == '\0')) {
        return FAILURE;
    }
    ZeroMemory(script, sizeof(*script));
    handle = file_open_rb(path);
    if (handle == FILEH_INVALID) {
        fprintf(stderr, "Error: input script not found: %s\n", path);
        return FAILURE;
    }
    size = file_getsize(handle);
    if ((size == 0) || (size > HEADLESS_INPUT_FILE_MAX)) {
        file_close(handle);
        fprintf(stderr, "Error: unsupported input script size: %s\n", path);
        return FAILURE;
    }
    buffer = (char *)SDL_malloc(size);
    if (buffer == NULL) {
        file_close(handle);
        fprintf(stderr, "Error: could not allocate input script buffer\n");
        return FAILURE;
    }
    if (file_read(handle, buffer, size) != size) {
        SDL_free(buffer);
        file_close(handle);
        fprintf(stderr, "Error: could not read input script: %s\n", path);
        return FAILURE;
    }
    file_close(handle);
    result = headless_input_script_parse(script, buffer, size);
    SDL_free(buffer);
    if (result != SUCCESS) {
        headless_input_script_clear(script);
        return FAILURE;
    }
    fprintf(stderr, "headless-input-script loaded commands=%u path=%s\n",
        script->command_count, path);
    return SUCCESS;
}

void headless_input_script_initialize(HEADLESS_INPUT_SCRIPT *script) {
    const char *timeout_value;
    char *timeout_end;
    unsigned long timeout_frames;

    if (script == NULL) {
        return;
    }
    script->command_index = 0;
    script->next_frame = HEADLESS_INPUT_INITIAL_DELAY_FRAMES;
    script->completed = FALSE;
    script->prompt_deadline = 0;
    script->prompt_timeout_frames = HEADLESS_INPUT_PROMPT_TIMEOUT_FRAMES;
    script->max_frames = 0;
    timeout_value = getenv("VAEG_HEADLESS_MAX_FRAMES");
    if ((timeout_value != NULL) && (timeout_value[0] != '\0')) {
        errno = 0;
        timeout_frames = strtoul(timeout_value, &timeout_end, 10);
        if ((errno == 0) && (timeout_end != timeout_value) &&
            (*timeout_end == '\0') && (timeout_frames >= 1) &&
            (timeout_frames <= 0xffffffffUL)) {
            script->max_frames = (UINT32)timeout_frames;
        }
    }
    timeout_value = getenv("VAEG_HEADLESS_PROMPT_TIMEOUT_FRAMES");
    if ((timeout_value != NULL) && (timeout_value[0] != '\0')) {
        errno = 0;
        timeout_frames = strtoul(timeout_value, &timeout_end, 10);
        if ((errno == 0) && (timeout_end != timeout_value) &&
            (*timeout_end == '\0') && (timeout_frames >= 1) &&
            (timeout_frames <= 0xffffffffUL)) {
            script->prompt_timeout_frames = (UINT32)timeout_frames;
        }
        else {
            fprintf(stderr,
                "Warning: invalid VAEG_HEADLESS_PROMPT_TIMEOUT_FRAMES; "
                "using %u\n", HEADLESS_INPUT_PROMPT_TIMEOUT_FRAMES);
        }
    }
    script->prompt_signature = 0;
    script->prompt_count = 0;
    script->prompt_clear_observed = FALSE;
    script->prompt_seen_once = FALSE;
    script->exit_requested = FALSE;
    fprintf(stderr,
        "headless-input-script waiting %u frames before first input; "
        "prompt_timeout_frames=%u max_frames=%u\n",
        HEADLESS_INPUT_INITIAL_DELAY_FRAMES, script->prompt_timeout_frames,
        script->max_frames);
}

BOOL headless_input_script_after_frame(HEADLESS_INPUT_SCRIPT *script,
                                       UINT frames) {
    HEADLESS_INPUT_COMMAND *command;
    UINT command_number;
    UINT32 prompt_count;
    UINT32 prompt_signature;

    if ((script == NULL) || script->completed) {
        return SUCCESS;
    }
    if ((script->max_frames != 0) && (frames >= script->max_frames)) {
        fprintf(stderr, "Error: deterministic frame bound reached frame=%u max_frames=%u\n", frames, script->max_frames);
        script->completed = TRUE;
        return FAILURE;
    }
    if ((frames < script->next_frame) || kbdpaste_active()) {
        return SUCCESS;
    }
    if (script->command_index >= script->command_count) {
        script->completed = TRUE;
        fprintf(stderr, "headless-input-script complete\n");
        return SUCCESS;
    }
    command = &script->commands[script->command_index++];
    if (command->wait_prompt) {
        if (script->prompt_deadline == 0) {
            script->prompt_deadline =
                (script->prompt_timeout_frames > (0xffffffffU - frames)) ?
                0xffffffffU : frames + script->prompt_timeout_frames;
            fprintf(stderr,
                "headless-input-script waiting for BASIC Ok prompt "
                "until frame=%u\n", script->prompt_deadline);
        }
        prompt_signature = headless_input_basic_prompt_signature(
            &prompt_count);
        if (!script->prompt_seen_once) {
            if (prompt_signature != 0) {
                fprintf(stderr,
                    "headless-input-script observed BASIC Ok prompt "
                    "frame=%u signature=%08x count=%u\n", frames,
                    prompt_signature, prompt_count);
                script->prompt_seen_once = TRUE;
                script->prompt_signature = prompt_signature;
                script->prompt_count = prompt_count;
                script->prompt_deadline = 0;
                script->prompt_clear_observed = FALSE;
                upd9002_m74_trace_lifecycle("headless-prompt-observed");
                script->next_frame = frames +
                    HEADLESS_INPUT_COMMAND_DELAY_FRAMES;
                return SUCCESS;
            }
        }
        else if (!script->prompt_clear_observed) {
            if (prompt_signature == 0) {
                script->prompt_clear_observed = TRUE;
                fprintf(stderr,
                    "headless-input-script observed BASIC Ok prompt "
                    "cleared frame=%u\n", frames);
            }
            else if ((prompt_signature != script->prompt_signature) ||
                (prompt_count != script->prompt_count)) {
                fprintf(stderr,
                    "headless-input-script observed new BASIC Ok prompt "
                    "frame=%u signature=%08x count=%u\n", frames,
                    prompt_signature, prompt_count);
                script->prompt_signature = prompt_signature;
                script->prompt_count = prompt_count;
                script->prompt_deadline = 0;
                script->prompt_clear_observed = FALSE;
                upd9002_m74_trace_lifecycle("headless-prompt-observed");
                script->next_frame = frames +
                    HEADLESS_INPUT_COMMAND_DELAY_FRAMES;
                return SUCCESS;
            }
        }
        else if (prompt_signature != 0) {
            fprintf(stderr,
                "headless-input-script observed BASIC Ok prompt "
                "frame=%u signature=%08x count=%u\n", frames,
                prompt_signature, prompt_count);
            script->prompt_signature = prompt_signature;
            script->prompt_count = prompt_count;
            script->prompt_deadline = 0;
            script->prompt_seen_once = TRUE;
            script->prompt_clear_observed = FALSE;
            upd9002_m74_trace_lifecycle("headless-prompt-observed");
            script->next_frame = frames +
                HEADLESS_INPUT_COMMAND_DELAY_FRAMES;
            return SUCCESS;
        }
        if (frames >= script->prompt_deadline) {
            fprintf(stderr,
                "Error: --headless-input-script BASIC Ok prompt "
                "timeout at frame=%u\n", frames);
            return FAILURE;
        }
        script->command_index--;
        script->next_frame = frames + 1;
        return SUCCESS;
    }
    if (command->exit) {
        script->completed = TRUE;
        script->exit_requested = TRUE;
        upd9002_m74_trace_lifecycle("headless-exit");
        fprintf(stderr, "headless-input-script requested exit frame=%u\n",
            frames);
        return SUCCESS;
    }
    if (command->wait) {
        upd9002_m74_trace_lifecycle("headless-before-wait");
        script->next_frame = frames + command->wait_frames;
        return SUCCESS;
    }
    if (command->disk_swap) {
        diskdrv_setfdd((REG8)command->disk_drive, command->text, 0);
        fprintf(stderr,
            "headless-input-script swapped FD%u path=%s frame=%u\n",
            command->disk_drive + 1, command->text, frames);
        script->next_frame = frames + HEADLESS_INPUT_COMMAND_DELAY_FRAMES;
        return SUCCESS;
    }
    command_number = script->command_index;
    {
        char label[64];

        snprintf(label, sizeof(label), "headless-before-command-%u",
            command_number);
        upd9002_m74_trace_lifecycle(label);
    }
    script->prompt_clear_observed = FALSE;
    upd9002_m74_trace_arm(command_number);
    if (!kbdpaste_start_text(command->text)) {
        fprintf(stderr,
            "Error: headless input script could not inject command %u\n",
            script->command_index);
        return FAILURE;
    }
    script->next_frame = frames + HEADLESS_INPUT_COMMAND_DELAY_FRAMES;
    fprintf(stderr,
        "headless-input-script injected command=%u frame=%u\n",
        command_number, frames);
    return SUCCESS;
}

BOOL headless_input_script_exit_requested(
        const HEADLESS_INPUT_SCRIPT *script) {

    return ((script != NULL) && script->exit_requested) ? TRUE : FALSE;
}
