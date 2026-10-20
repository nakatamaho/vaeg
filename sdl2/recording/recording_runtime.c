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
#include <string.h>

static VAEG_RECORDING_RUNTIME_EVENT last_event;

void vaeg_recording_runtime_screen_changed(void) {
	if (!vaeg_recording_native_session_is_active())
		return;
	if (vaeg_recording_native_session_auto_split_enabled()) {
		vaeg_recording_native_session_request_split();
		return;
	}
	if (vaeg_recording_native_session_stop())
		last_event = VAEG_RECORDING_RUNTIME_SCREEN_SAVED;
	else {
		const char *error = vaeg_recording_native_session_last_error();
		last_event =
		    (error != NULL && (!strcmp(error, "none") || !strcmp(error, "recording_no_intervals")))
		        ? VAEG_RECORDING_RUNTIME_SCREEN_EMPTY
		        : VAEG_RECORDING_RUNTIME_ERROR_STOPPED;
	}
	fprintf(stderr,
	        "Recording stopped after screen configuration change; guest continues (event=%d)\n",
	        (int)last_event);
}

VAEG_RECORDING_RUNTIME_RESULT vaeg_recording_runtime_step(int batch) {
	if (!vaeg_recording_native_session_is_active())
		return VAEG_RECORDING_RUNTIME_CONTINUE;
	/* A rejected descriptor never enters the old stream. Save only already
	 * accepted, complete intervals before the generic error path cancels them. */
	if (!batch && (vaeg_recording_session_displayed_capture_changed() ||
	               (!vaeg_recording_session_is_displayed() &&
	                !vaeg_recording_native_tap_is_active() &&
	                vaeg_recording_native_tap_last_error() ==
	                    VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED))) {
		vaeg_recording_runtime_screen_changed();
		return VAEG_RECORDING_RUNTIME_CONTINUE;
	}
	if (!vaeg_recording_native_session_after_guest_exec()) {
		last_event = VAEG_RECORDING_RUNTIME_ERROR_STOPPED;
		if (!batch) {
			fprintf(stderr, "Recording failed; guest continues: %s\n",
			        vaeg_recording_native_session_last_error());
		}
		return batch ? VAEG_RECORDING_RUNTIME_BATCH_FAILED : VAEG_RECORDING_RUNTIME_CONTINUE;
	}
	if (vaeg_recording_native_session_should_stop()) {
		if (batch)
			return VAEG_RECORDING_RUNTIME_BATCH_COMPLETE;
		last_event = vaeg_recording_native_session_stop() ? VAEG_RECORDING_RUNTIME_LIMIT_SAVED
		                                                  : VAEG_RECORDING_RUNTIME_ERROR_STOPPED;
	}
	return VAEG_RECORDING_RUNTIME_CONTINUE;
}

VAEG_RECORDING_RUNTIME_EVENT vaeg_recording_runtime_last_event(void) {
	return last_event;
}
void vaeg_recording_runtime_clear_event(void) {
	last_event = VAEG_RECORDING_RUNTIME_EVENT_NONE;
}
