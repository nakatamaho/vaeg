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
#include "recording/recording_contracts.hpp"

#include <cstring>
#include <limits>
#include <new>

namespace vaeg::recording {

OwnedVideoFrame::OwnedVideoFrame(const VAEG_RECORDING_VIDEO_DESCRIPTOR &descriptor,
                                 std::size_t storage_bytes)
    : descriptor_(descriptor), storage_(storage_bytes) {
}

OwnedVideoFrameResult
OwnedVideoFrame::create(const VAEG_RECORDING_VIDEO_DESCRIPTOR &descriptor) noexcept {
	const VAEG_RECORDING_ERROR validation = vaeg_recording_validate_video_descriptor(&descriptor);
	if (validation != VAEG_RECORDING_ERROR_OK) {
		return {validation, nullptr};
	}
	if (descriptor.stride_bytes >
	    static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) / descriptor.height) {
		return {VAEG_RECORDING_ERROR_SIZE_OVERFLOW, nullptr};
	}

	const std::size_t storage_bytes =
	    static_cast<std::size_t>(descriptor.stride_bytes) * descriptor.height;
	try {
		return {VAEG_RECORDING_ERROR_OK,
		        std::unique_ptr<OwnedVideoFrame>(new OwnedVideoFrame(descriptor, storage_bytes))};
	} catch (const std::bad_alloc &) {
		return {VAEG_RECORDING_ERROR_ALLOCATION_FAILURE, nullptr};
	}
}

VAEG_RECORDING_ERROR
OwnedVideoFrame::copy_from(const void *pixels, std::uint64_t bytes) noexcept {
	const VAEG_RECORDING_ERROR validation = vaeg_recording_validate_copy(pixels, bytes);
	if (validation != VAEG_RECORDING_ERROR_OK) {
		return validation;
	}
	if (bytes < storage_.size()) {
		return VAEG_RECORDING_ERROR_BUFFER_TOO_SMALL;
	}
	std::memcpy(storage_.data(), pixels, storage_.size());
	return VAEG_RECORDING_ERROR_OK;
}

const std::vector<std::uint8_t> &OwnedVideoFrame::bytes() const noexcept {
	return storage_;
}

const VAEG_RECORDING_VIDEO_DESCRIPTOR &OwnedVideoFrame::descriptor() const noexcept {
	return descriptor_;
}

RecordingStatusFacade::RecordingStatusFacade() noexcept : capabilities_{}, status_{} {
	vaeg_recording_get_capabilities(&capabilities_);
	vaeg_recording_get_status(&status_);
}

const VAEG_RECORDING_CAPABILITIES &RecordingStatusFacade::capabilities() const noexcept {
	return capabilities_;
}

const VAEG_RECORDING_STATUS &RecordingStatusFacade::status() const noexcept {
	return status_;
}

bool RecordingStatusFacade::backend_available() const noexcept {
	return capabilities_.backend_available != 0u;
}

} // namespace vaeg::recording
