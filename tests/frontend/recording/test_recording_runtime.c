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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "recording/recording_runtime.h"
#include "recording/recording_native_session.h"
#include "recording/recording_native_tap.h"
#include <stdio.h>
#include <stdlib.h>

static int active, after_ok, stop_ok, limit, after_calls, stop_calls;
static VAEG_RECORDING_NATIVE_TAP_ERROR tap_error;
static const char *error;
static void check(int valid, const char *code) {
	if (!valid) {
		fprintf(stderr, "%s\n", code);
		exit(1);
	}
}
static void fixture(void) {
	active = after_ok = stop_ok = 1;
	limit = after_calls = stop_calls = 0;
	tap_error = VAEG_RECORDING_NATIVE_TAP_OK;
	error = "none";
	vaeg_recording_runtime_clear_event();
}
int vaeg_recording_native_session_auto_split_enabled(void) {
	return 0;
}
void vaeg_recording_native_session_request_split(void) {
}
int vaeg_recording_native_session_is_active(void) {
	return active;
}
int vaeg_recording_native_tap_is_active(void) {
	return tap_error == VAEG_RECORDING_NATIVE_TAP_OK;
}
VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_tap_last_error(void) {
	return tap_error;
}
int vaeg_recording_native_session_after_guest_exec(void) {
	++after_calls;
	if (!after_ok || tap_error != VAEG_RECORDING_NATIVE_TAP_OK) {
		active = 0;
		error = "native_tap_stopped";
		return 0;
	}
	return 1;
}
int vaeg_recording_native_session_should_stop(void) {
	return limit;
}
int vaeg_recording_native_session_stop(void) {
	++stop_calls;
	active = 0;
	return stop_ok;
}
const char *vaeg_recording_native_session_last_error(void) {
	return error;
}

int main(void) {
	int batch;
	for (batch = 0; batch < 2; ++batch) {
		fixture();
		check(vaeg_recording_runtime_step(batch) == VAEG_RECORDING_RUNTIME_CONTINUE &&
		          after_calls == 1 && stop_calls == 0 && active,
		      "RUNTIME_VALID_FIXTURE");
		fixture();
		active = 0;
		check(vaeg_recording_runtime_step(batch) == VAEG_RECORDING_RUNTIME_CONTINUE &&
		          after_calls == 0 && stop_calls == 0,
		      "RUNTIME_IDLE_NO_SIDE_EFFECTS");
	}
	fixture();
	tap_error = VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED;
	check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE && !active &&
	          stop_calls == 1 && after_calls == 0 &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_SCREEN_SAVED,
	      "RUNTIME_GUI_GEOMETRY_FINALIZE_BEFORE_ABORT");
	fixture();
	tap_error = VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED;
	stop_ok = 0;
	error = "recording_no_intervals";
	check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE && !active &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_SCREEN_EMPTY,
	      "RUNTIME_GUI_GEOMETRY_EMPTY");
	fixture();
	tap_error = VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED;
	stop_ok = 0;
	error = "write_failed";
	check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE && !active &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_ERROR_STOPPED,
	      "RUNTIME_GUI_GEOMETRY_WRITE_FAILURE");
	fixture();
	tap_error = VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED;
	check(vaeg_recording_runtime_step(1) == VAEG_RECORDING_RUNTIME_BATCH_FAILED && !active &&
	          after_calls == 1 && stop_calls == 0,
	      "RUNTIME_BATCH_GEOMETRY_FAIL_CLOSED");
	fixture();
	after_ok = 0;
	check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE && !active &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_ERROR_STOPPED,
	      "RUNTIME_GUI_ERROR_CONTAINED");
	fixture();
	after_ok = 0;
	check(vaeg_recording_runtime_step(1) == VAEG_RECORDING_RUNTIME_BATCH_FAILED && !active,
	      "RUNTIME_BATCH_ERROR_FAIL_CLOSED");
	fixture();
	limit = 1;
	check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE && !active &&
	          stop_calls == 1 &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_LIMIT_SAVED,
	      "RUNTIME_GUI_LIMIT_SAVED_WITHOUT_GUEST_EXIT");
	fixture();
	limit = 1;
	check(vaeg_recording_runtime_step(1) == VAEG_RECORDING_RUNTIME_BATCH_COMPLETE && active &&
	          stop_calls == 0,
	      "RUNTIME_BATCH_LIMIT_DEFERRED_CLEANUP");
	fixture();
	vaeg_recording_runtime_screen_changed();
	check(!active && stop_calls == 1 && after_calls == 0 &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_SCREEN_SAVED,
	      "RUNTIME_EXPLICIT_SCREEN_CHANGE");
	fixture();
	active = 0;
	vaeg_recording_runtime_screen_changed();
	check(stop_calls == 0 &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_EVENT_NONE,
	      "RUNTIME_IDLE_SCREEN_CHANGE");
	vaeg_recording_runtime_clear_event();
	check(vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_EVENT_NONE,
	      "RUNTIME_EVENT_CLEAR");
	puts("recording runtime tests passed");
	return 0;
}
