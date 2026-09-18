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
#ifndef VAEG_SDL2_RECORDING_CONTRACTS_HPP
#define VAEG_SDL2_RECORDING_CONTRACTS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "recording/recording.h"

namespace vaeg::recording {

class OwnedVideoFrame;

struct OwnedVideoFrameResult {
	VAEG_RECORDING_ERROR error;
	std::unique_ptr<OwnedVideoFrame> frame;

	explicit operator bool() const noexcept {
		return error == VAEG_RECORDING_ERROR_OK && frame != nullptr;
	}
};

class OwnedVideoFrame {
  public:
	static OwnedVideoFrameResult create(const VAEG_RECORDING_VIDEO_DESCRIPTOR &descriptor) noexcept;

	VAEG_RECORDING_ERROR copy_from(const void *pixels, std::uint64_t bytes) noexcept;
	const std::vector<std::uint8_t> &bytes() const noexcept;
	const VAEG_RECORDING_VIDEO_DESCRIPTOR &descriptor() const noexcept;

  private:
	OwnedVideoFrame(const VAEG_RECORDING_VIDEO_DESCRIPTOR &descriptor, std::size_t storage_bytes);

	VAEG_RECORDING_VIDEO_DESCRIPTOR descriptor_;
	std::vector<std::uint8_t> storage_;
};

class RecordingStatusFacade {
  public:
	RecordingStatusFacade() noexcept;

	const VAEG_RECORDING_CAPABILITIES &capabilities() const noexcept;
	const VAEG_RECORDING_STATUS &status() const noexcept;
	bool backend_available() const noexcept;

  private:
	VAEG_RECORDING_CAPABILITIES capabilities_;
	VAEG_RECORDING_STATUS status_;
};

} // namespace vaeg::recording

#endif
