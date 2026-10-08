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
#ifndef VAEG_RECORDING_MACOS_WINDOW_CAPTURE_HPP
#define VAEG_RECORDING_MACOS_WINDOW_CAPTURE_HPP
#include <cstdint>
#include <memory>
#include <vector>
namespace vaeg::recording {
enum class MacCaptureState {
	Idle, Preparing, Running, Stopped, RuntimeUnavailable, ConsentRequired,
	WrongWindow, WindowUnavailable, UnsupportedGeometry, UnsupportedColor, StartFailed,
	SourceLost, FrameFailed, GeometryChanged
};
const char *mac_capture_state_name(MacCaptureState state) noexcept;
struct MacCapturedImage {
	std::uint32_t width = 0, height = 0;
	/* Absolute mach host-clock nanoseconds, NOT callback time or CMTime value.
	 * The real-time coordinator must use the same mach clock epoch. */
	std::uint64_t display_host_ns = 0;
	std::uint64_t overwritten = 0, admission_busy = 0;
	std::vector<std::uint8_t> bgra;
};
/* ScreenCaptureKit independent-window filter; never display/desktop capture.
 * Start only from Cocoa main thread for the actual NSApp main window number.
 * Does not request consent implicitly. Unvalidated title/border coverage must
 * not be advertised as available just because stream startup succeeds.
 * Worker polls take_latest; callbacks own no recycled CVPixelBuffer after return.
 * Feature uses macOS 14 geometry/shadow controls without raising app minimum. */
class MacWindowCapture {
  public:
	MacWindowCapture();
	~MacWindowCapture();
	void start(std::uint32_t main_window_number, bool source_is_verified_sdr) noexcept;
	void stop() noexcept;
	MacCaptureState state() const noexcept;
	bool take_latest(MacCapturedImage *image);
	MacWindowCapture(const MacWindowCapture &) = delete;
	MacWindowCapture &operator=(const MacWindowCapture &) = delete;
  private:
	class Impl;
	std::unique_ptr<Impl> impl_;
};
} // namespace vaeg::recording
#endif
