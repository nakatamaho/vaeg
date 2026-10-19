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
#include "recording/recording_native_tap.h"

#ifdef __cplusplus
extern "C" {
#endif

/* REC14 exposes the frontend-neutral session controls without FFmpeg types. */
int vaeg_recording_session_start(const char *path, uint32_t source, uint64_t frame_limit);
int vaeg_recording_native_session_start(const char *path, uint64_t frame_limit);
int vaeg_recording_native_session_start_from_environment(void);
/* GUI opt-in only; CLI/environment batches keep descriptor-change failure. */
void vaeg_recording_native_session_enable_auto_split(void);
int vaeg_recording_native_session_auto_split_enabled(void);
void vaeg_recording_native_session_request_split(void);
const char *vaeg_recording_native_session_output_path(void);
/* Called at the guest frame boundary, before its clock advances further. */
VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_session_submit_frame(
    uint64_t sequence, uint64_t end_ticks, uint32_t descriptor_generation,
    const VAEG_RECORDING_NATIVE_FRAME *frame, const VAEG_RECORDING_OSD_SNAPSHOT *osd);
int vaeg_recording_native_session_after_guest_exec(void);
int vaeg_recording_native_session_is_active(void);
int vaeg_recording_session_is_displayed(void);
/* Interactive acquisition changes save complete intervals; batches fail closed. */
int vaeg_recording_session_displayed_capture_changed(void);
int vaeg_recording_native_session_should_stop(void);
int vaeg_recording_native_session_request_stop(void);
int vaeg_recording_native_session_poll_stop(void);
int vaeg_recording_native_session_is_finalizing(void);
uint64_t vaeg_recording_native_session_elapsed_ticks(void);
uint64_t vaeg_recording_native_session_accepted_frames(void);
int vaeg_recording_native_session_stop(void);
void vaeg_recording_native_session_cancel(void);
/* Reset can auto-split; arbitrary state-load/rewind epoch changes still cancel. */
void vaeg_recording_native_session_before_reset(void);
void vaeg_recording_native_session_epoch_change(void);
const char *vaeg_recording_native_session_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
