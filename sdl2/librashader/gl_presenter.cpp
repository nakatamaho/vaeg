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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "librashader/gl_presenter.h"

#include "librashader/gl_bridge.h"

namespace vaeg::librashader {

namespace {

class GLPresenter final : public NativePresenter {
  public:
	GLPresenter() noexcept
	    : state_(PresenterState::Unavailable), error_(PresenterError::None), host_window_(nullptr),
	      drawable_width_(0), drawable_height_(0), backend_(PresenterBackend::OpenGL) {}

	~GLPresenter() override { shutdown(); }

	PresenterState state() const noexcept override { return state_; }
	PresenterError last_error() const noexcept override { return error_; }

	PresenterResult initialize(const NativePresenterCreateInfo &info) noexcept override {
		if ((info.host_window == nullptr) ||
		    ((info.backend != PresenterBackend::Automatic) &&
		     (info.backend != PresenterBackend::OpenGL))) {
			state_ = PresenterState::Unavailable;
			error_ = PresenterError::PlatformUnavailable;
			return PresenterResult::Fallback;
		}
		shutdown();
		host_window_ = info.host_window;
		drawable_width_ = info.drawable_width;
		drawable_height_ = info.drawable_height;
		backend_ = info.backend;
		state_ = PresenterState::Initializing;
		if (!vaeg_gl_bridge_initialize(info.host_window, info.preset_path,
		                               info.enable_filter ? 1 : 0, &bridge_)) {
			state_ = PresenterState::Unavailable;
			error_ = info.enable_filter ? PresenterError::FilterFailure
			                           : PresenterError::DeviceFailure;
			return PresenterResult::Fallback;
		}
		if ((drawable_width_ != 0) && (drawable_height_ != 0) &&
		    (vaeg_gl_bridge_set_drawable_size(&bridge_, drawable_width_, drawable_height_) !=
		     VAEG_GL_BRIDGE_OK)) {
			shutdown();
			state_ = PresenterState::Unavailable;
			error_ = PresenterError::ResourceFailure;
			return PresenterResult::Fallback;
		}
		state_ = info.enable_filter ? PresenterState::Filtered : PresenterState::PassThrough;
		error_ = PresenterError::None;
		return PresenterResult::Recovered;
	}

	PresenterResult present(const VAEG_FRAME_INPUT &frame) noexcept override {
		VAEG_GL_BRIDGE_RESULT result;

		if ((state_ != PresenterState::PassThrough) && (state_ != PresenterState::Filtered)) {
			return PresenterResult::Fallback;
		}
		result = vaeg_gl_bridge_present(&bridge_, &frame);
		if (result == VAEG_GL_BRIDGE_OK) {
			error_ = PresenterError::None;
			return PresenterResult::Presented;
		}
		if (result == VAEG_GL_BRIDGE_INVALID_FRAME) {
			error_ = PresenterError::InvalidFrame;
			return PresenterResult::Failed;
		}
		if (result == VAEG_GL_BRIDGE_NO_OUTPUT) {
			return PresenterResult::Disabled;
		}
		error_ = PresenterError::ResourceFailure;
		state_ = PresenterState::Unavailable;
		return PresenterResult::Fallback;
	}

	PresenterResult set_filter_enabled(bool enabled) noexcept override {
		VAEG_GL_BRIDGE_RESULT result;

		if ((state_ != PresenterState::PassThrough) && (state_ != PresenterState::Filtered)) {
			return PresenterResult::Fallback;
		}
		result = vaeg_gl_bridge_set_filter_enabled(&bridge_, enabled ? 1 : 0);
		if (result == VAEG_GL_BRIDGE_OK) {
			state_ = enabled ? PresenterState::Filtered : PresenterState::PassThrough;
			error_ = PresenterError::None;
			return enabled ? PresenterResult::Recovered : PresenterResult::Disabled;
		}
		error_ = PresenterError::FilterFailure;
		return PresenterResult::Fallback;
	}

	PresenterResult resize(uint32_t drawable_width, uint32_t drawable_height) noexcept override {
		if ((state_ != PresenterState::PassThrough) && (state_ != PresenterState::Filtered)) {
			return PresenterResult::Fallback;
		}
		if ((drawable_width == 0) || (drawable_height == 0)) {
			return PresenterResult::Disabled;
		}
		if (vaeg_gl_bridge_set_drawable_size(&bridge_, drawable_width, drawable_height) !=
		    VAEG_GL_BRIDGE_OK) {
			error_ = PresenterError::ResourceFailure;
			state_ = PresenterState::Unavailable;
			return PresenterResult::Fallback;
		}
		drawable_width_ = drawable_width;
		drawable_height_ = drawable_height;
		return PresenterResult::Recovered;
	}

	PresenterResult recover() noexcept override {
		NativePresenterCreateInfo info;

		if (host_window_ == nullptr) {
			return PresenterResult::Fallback;
		}
		info.host_window = host_window_;
		info.drawable_width = drawable_width_;
		info.drawable_height = drawable_height_;
		info.backend = backend_;
		info.enable_filter = (state_ == PresenterState::Filtered);
		info.preset_path = nullptr;
		return initialize(info);
	}

	void shutdown() noexcept override {
		vaeg_gl_bridge_shutdown(&bridge_);
		state_ = PresenterState::Unavailable;
		error_ = PresenterError::None;
	}

  private:
	VAEG_GL_BRIDGE bridge_{};
	PresenterState state_;
	PresenterError error_;
	void *host_window_;
	uint32_t drawable_width_;
	uint32_t drawable_height_;
	PresenterBackend backend_;
};

} // namespace

std::unique_ptr<NativePresenter> create_gl_presenter() {
	return std::make_unique<GLPresenter>();
}

} // namespace vaeg::librashader
