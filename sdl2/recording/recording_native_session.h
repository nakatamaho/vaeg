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
#ifndef VAEG_SDL2_RECORDING_NATIVE_SESSION_H
#define VAEG_SDL2_RECORDING_NATIVE_SESSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* REC13's controller is private until the public recording UI/CLI milestone. */
int vaeg_recording_native_session_start(const char *path, uint64_t frame_limit);
int vaeg_recording_native_session_start_from_environment(void);
int vaeg_recording_native_session_after_guest_exec(void);
int vaeg_recording_native_session_is_active(void);
int vaeg_recording_native_session_should_stop(void);
int vaeg_recording_native_session_stop(void);
void vaeg_recording_native_session_cancel(void);
void vaeg_recording_native_session_epoch_change(void);
const char *vaeg_recording_native_session_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
