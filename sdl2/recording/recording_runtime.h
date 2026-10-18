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
#ifndef VAEG_SDL2_RECORDING_RUNTIME_H
#define VAEG_SDL2_RECORDING_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VAEG_RECORDING_RUNTIME_CONTINUE = 0,
	VAEG_RECORDING_RUNTIME_BATCH_COMPLETE,
	VAEG_RECORDING_RUNTIME_BATCH_FAILED
} VAEG_RECORDING_RUNTIME_RESULT;

typedef enum {
	VAEG_RECORDING_RUNTIME_EVENT_NONE = 0,
	VAEG_RECORDING_RUNTIME_SCREEN_SAVED,
	VAEG_RECORDING_RUNTIME_SCREEN_EMPTY,
	VAEG_RECORDING_RUNTIME_LIMIT_SAVED,
	VAEG_RECORDING_RUNTIME_ERROR_STOPPED
} VAEG_RECORDING_RUNTIME_EVENT;

/* Presentation-thread boundary: interactive errors stop recording, not the guest.
 * CLI/environment batch recording retains its fail-closed exit contract. */
VAEG_RECORDING_RUNTIME_RESULT vaeg_recording_runtime_step(int batch);
/* Notify before applying an interactive screen setting. Opted-in GUI sessions
 * split at the next guest frame; legacy sessions finalize. No-op when idle. */
void vaeg_recording_runtime_screen_changed(void);
VAEG_RECORDING_RUNTIME_EVENT vaeg_recording_runtime_last_event(void);
void vaeg_recording_runtime_clear_event(void);

#ifdef __cplusplus
}
#endif
#endif
