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
#include "librashader/native_presenter.h"

namespace vaeg::librashader {

bool presenter_state_transition_allowed(PresenterState from, PresenterState to) noexcept {
	if (from == to) {
		return true;
	}
	switch (from) {
	case PresenterState::Unavailable:
		return to == PresenterState::Initializing;
	case PresenterState::Initializing:
		return (to == PresenterState::Unavailable) || (to == PresenterState::PassThrough);
	case PresenterState::PassThrough:
		return (to == PresenterState::Filtered) || (to == PresenterState::Initializing) ||
		       (to == PresenterState::Unavailable);
	case PresenterState::Filtered:
		return (to == PresenterState::PassThrough) || (to == PresenterState::Initializing) ||
		       (to == PresenterState::Unavailable);
	default:
		return false;
	}
}

const char *presenter_state_name(PresenterState state) noexcept {
	switch (state) {
	case PresenterState::Unavailable:
		return "unavailable";
	case PresenterState::Initializing:
		return "initializing";
	case PresenterState::PassThrough:
		return "pass-through";
	case PresenterState::Filtered:
		return "filtered";
	default:
		return "unknown";
	}
}

const char *presenter_result_name(PresenterResult result) noexcept {
	switch (result) {
	case PresenterResult::Presented:
		return "presented";
	case PresenterResult::Disabled:
		return "disabled";
	case PresenterResult::Recovered:
		return "recovered";
	case PresenterResult::Fallback:
		return "fallback";
	case PresenterResult::Failed:
		return "failed";
	default:
		return "unknown";
	}
}

const char *presenter_error_name(PresenterError error) noexcept {
	switch (error) {
	case PresenterError::None:
		return "none";
	case PresenterError::InvalidFrame:
		return "invalid_frame";
	case PresenterError::PlatformUnavailable:
		return "platform_unavailable";
	case PresenterError::RuntimeMissing:
		return "runtime_missing";
	case PresenterError::AbiMismatch:
		return "abi_mismatch";
	case PresenterError::DeviceFailure:
		return "device_failure";
	case PresenterError::PresetFailure:
		return "preset_failure";
	case PresenterError::FilterFailure:
		return "filter_failure";
	case PresenterError::ResourceFailure:
		return "resource_failure";
	default:
		return "unknown";
	}
}

} // namespace vaeg::librashader
