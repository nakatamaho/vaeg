/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
#include <cassert>
#include <cstdint>
#include <vector>

#include "recording/recording_contracts.hpp"

int main() {
	const VAEG_RECORDING_VIDEO_DESCRIPTOR descriptor = {4u, 2u, 12u, VAEG_RECORDING_PIXEL_RGB24};
	auto result = vaeg::recording::OwnedVideoFrame::create(descriptor);
	assert(result);
	assert(result.frame->descriptor().width == 4u);
	assert(result.frame->bytes().size() == 24u);

	std::vector<std::uint8_t> source(24u, 0x5au);
	assert(result.frame->copy_from(source.data(), source.size()) == VAEG_RECORDING_ERROR_OK);
	source[0] = 0xa5u;
	assert(result.frame->bytes()[0] == 0x5au);
	assert(result.frame->copy_from(source.data(), 23u) == VAEG_RECORDING_ERROR_BUFFER_TOO_SMALL);

	VAEG_RECORDING_VIDEO_DESCRIPTOR short_stride = descriptor;
	short_stride.stride_bytes = 11u;
	auto invalid = vaeg::recording::OwnedVideoFrame::create(short_stride);
	assert(!invalid);
	assert(invalid.error == VAEG_RECORDING_ERROR_INVALID_STRIDE);

	VAEG_RECORDING_VIDEO_DESCRIPTOR overflow = descriptor;
	overflow.width = UINT32_MAX;
	overflow.height = UINT32_MAX;
	overflow.stride_bytes = UINT64_MAX;
	auto oversized = vaeg::recording::OwnedVideoFrame::create(overflow);
	assert(!oversized);
	assert(oversized.error == VAEG_RECORDING_ERROR_SIZE_OVERFLOW);

	vaeg::recording::RecordingStatusFacade facade;
	assert(facade.capabilities().source_count == 2u);
	assert(!facade.backend_available());
	assert(facade.status().state == VAEG_RECORDING_STATE_DISABLED);
	return 0;
}
