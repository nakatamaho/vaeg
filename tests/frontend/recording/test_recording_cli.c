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
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cliopts.h"

BOOL vaeg_sound_rate_valid(UINT rate) {
	return (rate == 11025u || rate == 22050u || rate == 44100u) ? TRUE : FALSE;
}

BOOL vaeg_sound_buffer_valid(UINT milliseconds) {
	return (milliseconds >= 40u && milliseconds <= 1000u) ? TRUE : FALSE;
}

BOOL pccore_cpu_multiple_valid(UINT multiple) {
	return (multiple >= 1u && multiple <= 32u) ? TRUE : FALSE;
}

BOOL sgp_speed_multiplier_valid(UINT multiple) {
	return (multiple >= 1u && multiple <= 16u) ? TRUE : FALSE;
}

static BOOL parse(int argc, char **argv, VAEG_CLI_OPTIONS *options) {
	char error[256];
	const BOOL result = vaeg_cli_parse(argc, argv, options, error, sizeof(error));
	if (result != SUCCESS) {
		fprintf(stderr, "CLI parse failed: %s\n", error);
	}
	return result == SUCCESS ? TRUE : FALSE;
}

int main(void) {
	VAEG_CLI_OPTIONS options;
	char *valid[] = {"vaeg", "--record", "folder/録画 test.mkv", "--record-source",
	                 "native", "--record-frames", "7", "--model", "va2"};
	char *default_source[] = {"vaeg", "--record", "capture.mkv"};
	char *displayed[] = {"vaeg", "--record", "capture.mkv", "--record-source", "displayed"};
	char *bad_extension[] = {"vaeg", "--record", "capture.avi"};
	char *bad_frames[] = {"vaeg", "--record", "capture.mkv", "--record-frames", "0"};
	char *orphan_frames[] = {"vaeg", "--record-frames", "2"};
	char *orphan_native[] = {"vaeg", "--record-source", "native"};
	char *orphan_source[] = {"vaeg", "--record-source", "displayed"};

	assert(parse((int)(sizeof(valid) / sizeof(valid[0])), valid, &options));
	assert(options.record == TRUE);
	assert(options.record_source == VAEG_RECORDING_SOURCE_NATIVE);
	assert(options.record_frame_limit == 7u);
	assert(strcmp(options.record_path, "folder/録画 test.mkv") == 0);
	assert(options.model == VAEG_CLI_MODEL_VA2);

	assert(parse((int)(sizeof(default_source) / sizeof(default_source[0])), default_source,
	            &options));
	assert(options.record_source == VAEG_RECORDING_SOURCE_NATIVE);
	assert(options.record_frame_limit == 0u);

	assert(parse((int)(sizeof(displayed) / sizeof(displayed[0])), displayed, &options));
	assert(options.record_source == VAEG_RECORDING_SOURCE_DISPLAYED);
	assert(!parse((int)(sizeof(bad_extension) / sizeof(bad_extension[0])), bad_extension,
              &options));
	assert(!parse((int)(sizeof(bad_frames) / sizeof(bad_frames[0])), bad_frames, &options));
	assert(!parse((int)(sizeof(orphan_frames) / sizeof(orphan_frames[0])), orphan_frames,
	              &options));
	assert(!parse((int)(sizeof(orphan_native) / sizeof(orphan_native[0])), orphan_native,
              &options));
	assert(!parse((int)(sizeof(orphan_source) / sizeof(orphan_source[0])), orphan_source,
              &options));
	return 0;
}
