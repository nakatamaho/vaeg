/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#ifndef VAEG_SDL2_LIBRASHADER_NATIVE_PRESENTER_H
#define VAEG_SDL2_LIBRASHADER_NATIVE_PRESENTER_H

#include <cstdint>

#include "librashader/frame_input.h"

namespace vaeg::librashader {

enum class PresenterState : uint8_t {
	Unavailable = 0,
	Initializing,
	PassThrough,
	Filtered
};

enum class PresenterResult : uint8_t {
	Presented = 0,
	Disabled,
	Recovered,
	Fallback,
	Failed
};

enum class PresenterError : uint8_t {
	None = 0,
	InvalidFrame,
	PlatformUnavailable,
	RuntimeMissing,
	AbiMismatch,
	DeviceFailure,
	PresetFailure,
	FilterFailure,
	ResourceFailure
};

enum class PresenterBackend : uint8_t {
	Automatic = 0,
	Metal,
	D3D11,
	OpenGL
};

struct NativePresenterCreateInfo {
	void *host_window;
	uint32_t drawable_width;
	uint32_t drawable_height;
	PresenterBackend backend;
	bool enable_filter;
	const char *preset_path;
};

bool presenter_state_transition_allowed(PresenterState from, PresenterState to) noexcept;
const char *presenter_state_name(PresenterState state) noexcept;
const char *presenter_result_name(PresenterResult result) noexcept;
const char *presenter_error_name(PresenterError error) noexcept;

class NativePresenter {
  public:
	virtual ~NativePresenter() = default;
	virtual PresenterState state() const noexcept = 0;
	virtual PresenterError last_error() const noexcept = 0;
	virtual PresenterResult initialize(const NativePresenterCreateInfo &info) noexcept = 0;
	virtual PresenterResult present(const VAEG_FRAME_INPUT &frame) noexcept = 0;
	virtual PresenterResult set_filter_enabled(bool enabled) noexcept = 0;
	virtual PresenterResult recover() noexcept = 0;
	virtual void shutdown() noexcept = 0;
};

} // namespace vaeg::librashader

#endif
