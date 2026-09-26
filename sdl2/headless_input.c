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
#include "kbdmap.h"
#include "kbdinject.h"
#include "headless_input.h"
#include "fdd/diskdrv.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	HEADLESS_INPUT_INITIAL_DELAY_FRAMES = 600,
	HEADLESS_INPUT_COMMAND_DELAY_FRAMES = 120,
	HEADLESS_INPUT_KEY_PHASE_FRAMES = 3,
	HEADLESS_INPUT_COMMAND_MAX = 256,
	HEADLESS_INPUT_FILE_MAX = 64 * 1024
};

static void headless_input_release_keys(HEADLESS_INPUT_SCRIPT *script) {
	if (script->key_down) {
		kbdinject_keyup(script->held_key);
	}
	if (script->control_down) {
		kbdinject_keyup(script->held_control);
	}
	script->key_down = FALSE;
	script->control_down = FALSE;
	script->key_phase = 0;
}

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
	headless_input_release_keys(script);
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
	script->commands =
	    (HEADLESS_INPUT_COMMAND *)SDL_calloc(HEADLESS_INPUT_COMMAND_MAX, sizeof(*script->commands));
	return (script->commands != NULL) ? SUCCESS : FAILURE;
}

static BOOL headless_input_script_add_text(HEADLESS_INPUT_SCRIPT *script, const char *line,
                                           UINT length, BOOL append_return) {
	HEADLESS_INPUT_COMMAND *command;
	char *text;
	UINT extra;

	if (script->command_count >= HEADLESS_INPUT_COMMAND_MAX) {
		fprintf(stderr, "Error: --headless-input-script has too many commands; max=%u\n",
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

static BOOL headless_input_script_add_disk_swap(HEADLESS_INPUT_SCRIPT *script, UINT drive,
                                                const char *path, UINT length) {
	HEADLESS_INPUT_COMMAND *command;
	char *text;

	if ((length == 0) || (script->command_count >= HEADLESS_INPUT_COMMAND_MAX)) {
		fprintf(stderr, "Error: headless input disk swap needs a path and a free command slot\n");
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

static BOOL headless_input_script_add_wait(HEADLESS_INPUT_SCRIPT *script, UINT wait_frames) {
	HEADLESS_INPUT_COMMAND *command;

	if (script->command_count >= HEADLESS_INPUT_COMMAND_MAX) {
		fprintf(stderr, "Error: --headless-input-script has too many commands; max=%u\n",
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

static BOOL headless_input_script_add_line(HEADLESS_INPUT_SCRIPT *script, const char *line,
                                           UINT length) {
	char *value;
	char *end;
	unsigned long wait_frames;
	static const struct {
		const char *name;
		KBDMAP_ROLE role;
		BOOL control;
	} keys[] = {{"ctrl-c", KBDROLE_C, TRUE},
	            {"ctrl-z", KBDROLE_Z, TRUE},
	            {"backspace", KBDROLE_BS, FALSE},
	            {"escape", KBDROLE_ESC, FALSE},
	            {"f1", KBDROLE_F1, FALSE}};
	UINT key;
	if (length >= 6 && memcmp(line, "@text ", 6) == 0) {
		return headless_input_script_add_text(script, line + 6, length - 6, FALSE);
	}

	if ((length == 4 && memcmp(line, "@key", 4) == 0) ||
	    (length >= 5 && memcmp(line, "@key ", 5) == 0)) {
		for (key = 0; key < sizeof(keys) / sizeof(keys[0]); key++) {
			if (length == 5 + strlen(keys[key].name) &&
			    memcmp(line + 5, keys[key].name, length - 5) == 0) {
				HEADLESS_INPUT_COMMAND *command;
				if (script->command_count >= HEADLESS_INPUT_COMMAND_MAX ||
				    headless_input_script_reserve(script) != SUCCESS) {
					return FAILURE;
				}
				command = &script->commands[script->command_count++];
				command->key_press = TRUE;
				command->key_role = keys[key].role;
				command->control = keys[key].control;
				return SUCCESS;
			}
		}
		fprintf(stderr,
		        "Error: headless input @key needs ctrl-c, ctrl-z, backspace, escape or f1\n");
		return FAILURE;
	}

	if ((length >= 6) && ((memcmp(line, "@fdd1 ", 6) == 0) || (memcmp(line, "@fdd2 ", 6) == 0))) {
		UINT drive = (line[4] == '1') ? 0 : 1;
		return headless_input_script_add_disk_swap(script, drive, line + 6, length - 6);
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
		if ((errno != 0) || (end == value) || (*end != '\0') || (wait_frames > 0xffffffffUL)) {
			SDL_free(value);
			fprintf(stderr, "Error: --headless-input-script @wait needs a frame count\n");
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

static BOOL headless_input_script_parse(HEADLESS_INPUT_SCRIPT *script, char *buffer, UINT size) {
	UINT position;
	UINT start;
	UINT end;

	position = 0;
	if ((size >= 3) && ((UINT8)buffer[0] == 0xef) && ((UINT8)buffer[1] == 0xbb) &&
	    ((UINT8)buffer[2] == 0xbf)) {
		position = 3;
	}
	while (position < size) {
		while ((position < size) && ((buffer[position] == '\r') || (buffer[position] == '\n'))) {
			position++;
		}
		start = position;
		while ((position < size) && (buffer[position] != '\r') && (buffer[position] != '\n')) {
			position++;
		}
		end = position;
		while ((end > start) && ((buffer[end - 1] == ' ') || (buffer[end - 1] == '\t'))) {
			end--;
		}
		while ((start < end) && ((buffer[start] == ' ') || (buffer[start] == '\t'))) {
			start++;
		}
		if ((start == end) || (buffer[start] == '#')) {
			continue;
		}
		if (headless_input_script_add_line(script, buffer + start, end - start) != SUCCESS) {
			return FAILURE;
		}
	}
	if ((script->command_count == 0) || (script->commands == NULL)) {
		fprintf(stderr, "Error: --headless-input-script contains no commands\n");
		return FAILURE;
	}
	return SUCCESS;
}

BOOL headless_input_script_load(HEADLESS_INPUT_SCRIPT *script, const char *path) {
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
	fprintf(stderr, "headless-input-script loaded commands=%u path=%s\n", script->command_count,
	        path);
	return SUCCESS;
}

void headless_input_script_initialize(HEADLESS_INPUT_SCRIPT *script) {
	if (script == NULL) {
		return;
	}
	headless_input_release_keys(script);
	script->command_index = 0;
	script->next_frame = HEADLESS_INPUT_INITIAL_DELAY_FRAMES;
	script->completed = FALSE;
	fprintf(stderr, "headless-input-script waiting %u frames before first input\n",
	        HEADLESS_INPUT_INITIAL_DELAY_FRAMES);
}

BOOL headless_input_script_after_frame(HEADLESS_INPUT_SCRIPT *script, UINT frames) {
	HEADLESS_INPUT_COMMAND *command;
	UINT command_number;

	if ((script == NULL) || script->completed || (frames < script->next_frame)) {
		return SUCCESS;
	}
	if (script->key_phase) {
		if (script->key_phase == 1) {
			kbdinject_keydown(script->held_key);
			script->key_down = TRUE;
			script->key_phase = 2;
		} else if (script->key_phase == 2) {
			kbdinject_keyup(script->held_key);
			script->key_down = FALSE;
			script->key_phase = script->control_down ? 3 : 0;
		} else {
			headless_input_release_keys(script);
		}
		script->next_frame = frames + (script->key_phase ? HEADLESS_INPUT_KEY_PHASE_FRAMES
		                                                 : HEADLESS_INPUT_COMMAND_DELAY_FRAMES);
		return SUCCESS;
	}
	if (kbdpaste_active()) {
		return SUCCESS;
	}
	if (script->command_index >= script->command_count) {
		script->completed = TRUE;
		fprintf(stderr, "headless-input-script complete\n");
		return SUCCESS;
	}
	command = &script->commands[script->command_index++];
	if (command->key_press) {
		script->held_key = kbdmap_guest_code((KBDMAP_ROLE)command->key_role);
		script->held_control = kbdmap_guest_code(KBDROLE_CTRL);
		if (script->held_key == KBDMAP_NC ||
		    (command->control && script->held_control == KBDMAP_NC)) {
			fprintf(stderr, "Error: requested headless key has no guest mapping\n");
			return FAILURE;
		}
		if (command->control) {
			kbdinject_keydown(script->held_control);
			script->control_down = TRUE;
			script->key_phase = 1;
		} else {
			kbdinject_keydown(script->held_key);
			script->key_down = TRUE;
			script->key_phase = 2;
		}
		script->next_frame = frames + HEADLESS_INPUT_KEY_PHASE_FRAMES;
		fprintf(stderr, "headless-input-script key command=%u frame=%u\n", script->command_index,
		        frames);
		return SUCCESS;
	}
	if (command->wait) {
		script->next_frame = frames + command->wait_frames;
		return SUCCESS;
	}
	if (command->disk_swap) {
		diskdrv_setfdd((REG8)command->disk_drive, command->text, 0);
		fprintf(stderr, "headless-input-script swapped FD%u path=%s frame=%u\n",
		        command->disk_drive + 1, command->text, frames);
		script->next_frame = frames + HEADLESS_INPUT_COMMAND_DELAY_FRAMES;
		return SUCCESS;
	}
	if (!kbdpaste_start_text(command->text)) {
		fprintf(stderr, "Error: headless input script could not inject command %u\n",
		        script->command_index);
		return FAILURE;
	}
	command_number = script->command_index;
	script->next_frame = frames + HEADLESS_INPUT_COMMAND_DELAY_FRAMES;
	fprintf(stderr, "headless-input-script injected command=%u frame=%u\n", command_number, frames);
	return SUCCESS;
}
