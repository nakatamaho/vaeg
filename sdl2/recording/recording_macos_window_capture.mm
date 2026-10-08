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
#include "recording/recording_macos_window_capture.hpp"
#import <Cocoa/Cocoa.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <mach/mach_time.h>
#include <unistd.h>

using vaeg::recording::MacCaptureState;
namespace {
struct CaptureStorage {
	std::atomic<MacCaptureState> status{MacCaptureState::Idle};
	std::atomic<bool> canceled{false};
	std::atomic<std::uint64_t> busy{0};
	std::mutex control, pixels_mutex;
	__strong SCStream *stream = nil;
	__strong id output = nil;
	std::vector<std::uint8_t> pixels;
	std::uint32_t width = 0, height = 0;
	std::uint64_t timestamp_ns = 0, overwritten = 0;
	bool pending = false;
	CGSize point_extent{};
	mach_timebase_info_data_t timebase{};
};
bool host_ns(std::uint64_t ticks, const mach_timebase_info_data_t &base,
             std::uint64_t *result) {
	if (!base.denom || !base.numer) return false;
	const auto whole = ticks / base.denom;
	const auto fraction = (ticks % base.denom) * base.numer / base.denom;
	if (whole > (UINT64_MAX - fraction) / base.numer) return false;
	*result = whole * base.numer + fraction;
	return true;
}
void fail(const std::shared_ptr<CaptureStorage> &storage, MacCaptureState status) {
	if (!storage->canceled.load()) storage->status.store(status);
}
}

API_AVAILABLE(macos(14.0))
@interface VaegWindowStreamOutput : NSObject <SCStreamOutput, SCStreamDelegate> {
@public
	std::shared_ptr<CaptureStorage> storage;
}
@end
@implementation VaegWindowStreamOutput
- (void)stream:(SCStream *)stream didStopWithError:(NSError *)error {
	(void)stream; (void)error;
	fail(storage, MacCaptureState::SourceLost);
}
- (void)stream:(SCStream *)stream didOutputSampleBuffer:(CMSampleBufferRef)sample
       ofType:(SCStreamOutputType)type {
	(void)stream;
	if (type != SCStreamOutputTypeScreen || storage->canceled.load() ||
	    storage->status.load() != MacCaptureState::Running) return;
	CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sample, false);
	if (!attachments || CFArrayGetCount(attachments) == 0) {
		fail(storage, MacCaptureState::FrameFailed); return;
	}
	NSDictionary *metadata = (__bridge NSDictionary *)CFArrayGetValueAtIndex(attachments, 0);
	NSNumber *frame_status = metadata[SCStreamFrameInfoStatus];
	if (!frame_status) { fail(storage, MacCaptureState::FrameFailed); return; }
	if (frame_status.integerValue == SCFrameStatusIdle ||
	    frame_status.integerValue == SCFrameStatusStarted) return;
	if (frame_status.integerValue != SCFrameStatusComplete) {
		fail(storage, MacCaptureState::SourceLost); return;
	}
	/* Source size, not just fixed configured output size. Stop instead of
	 * quietly scaling a resized/Retina-changed source into the old movie. */
	CGRect screen_rect{}, content_rect{};
	NSDictionary *screen = metadata[SCStreamFrameInfoScreenRect];
	NSDictionary *content = metadata[SCStreamFrameInfoContentRect];
	NSNumber *display_time = metadata[SCStreamFrameInfoDisplayTime];
	NSNumber *scale = metadata[SCStreamFrameInfoScaleFactor];
	if (!screen || !content || !display_time || !scale ||
	    !CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)screen, &screen_rect) ||
	    !CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)content, &content_rect)) {
		fail(storage, MacCaptureState::FrameFailed); return;
	}
	if (std::fabs(screen_rect.size.width - storage->point_extent.width) > 0.01 ||
	    std::fabs(screen_rect.size.height - storage->point_extent.height) > 0.01 ||
	    std::fabs(screen_rect.size.width * scale.doubleValue - storage->width) > 1.0 ||
	    std::fabs(screen_rect.size.height * scale.doubleValue - storage->height) > 1.0) {
		fail(storage, MacCaptureState::GeometryChanged); return;
	}
	CVPixelBufferRef buffer = CMSampleBufferGetImageBuffer(sample);
	std::uint64_t ns;
	if (!buffer || CVPixelBufferGetPixelFormatType(buffer) != kCVPixelFormatType_32BGRA ||
	    CVPixelBufferGetWidth(buffer) != storage->width ||
	    CVPixelBufferGetHeight(buffer) != storage->height ||
	    !host_ns(display_time.unsignedLongLongValue, storage->timebase, &ns) ||
	    CGRectIsEmpty(content_rect) || content_rect.origin.x < 0 || content_rect.origin.y < 0 ||
	    CGRectGetMaxX(content_rect) > storage->width || CGRectGetMaxY(content_rect) > storage->height) {
		fail(storage, MacCaptureState::FrameFailed); return;
	}
	std::unique_lock<std::mutex> lock(storage->pixels_mutex, std::try_to_lock);
	if (!lock.owns_lock()) { storage->busy.fetch_add(1); return; }
	if (storage->timestamp_ns && ns < storage->timestamp_ns) {
		fail(storage, MacCaptureState::FrameFailed); return;
	}
	if (CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess) {
		fail(storage, MacCaptureState::FrameFailed); return;
	}
	const auto *source = static_cast<const std::uint8_t *>(CVPixelBufferGetBaseAddress(buffer));
	const auto stride = CVPixelBufferGetBytesPerRow(buffer);
	if (!source || stride < storage->width * 4u) {
		CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
		fail(storage, MacCaptureState::FrameFailed); return;
	}
	for (unsigned row = 0; row < storage->height; ++row) {
		std::memcpy(storage->pixels.data() + row * storage->width * 4u,
		            source + row * stride, storage->width * 4u);
	}
	CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
	if (storage->pending) ++storage->overwritten;
	storage->timestamp_ns = ns;
	storage->pending = true;
}
@end

namespace vaeg::recording {
const char *mac_capture_state_name(MacCaptureState state) noexcept {
	switch (state) {
	case MacCaptureState::Idle: return "OSR_MAC_IDLE";
	case MacCaptureState::Preparing: return "OSR_MAC_PREPARING";
	case MacCaptureState::Running: return "OSR_MAC_RUNNING";
	case MacCaptureState::Stopped: return "OSR_MAC_STOPPED";
	case MacCaptureState::RuntimeUnavailable: return "OSR_MAC_RUNTIME_UNAVAILABLE";
	case MacCaptureState::ConsentRequired: return "OSR_MAC_CONSENT_REQUIRED";
	case MacCaptureState::WrongWindow: return "OSR_MAC_WRONG_WINDOW";
	case MacCaptureState::WindowUnavailable: return "OSR_MAC_WINDOW_UNAVAILABLE";
	case MacCaptureState::UnsupportedGeometry: return "OSR_MAC_UNSUPPORTED_GEOMETRY";
	case MacCaptureState::UnsupportedColor: return "OSR_MAC_UNSUPPORTED_COLOR";
	case MacCaptureState::StartFailed: return "OSR_MAC_START_FAILED";
	case MacCaptureState::SourceLost: return "OSR_MAC_SOURCE_LOST";
	case MacCaptureState::FrameFailed: return "OSR_MAC_FRAME_FAILED";
	case MacCaptureState::GeometryChanged: return "OSR_MAC_GEOMETRY_CHANGED";
	}
	return "OSR_MAC_UNKNOWN";
}
class MacWindowCapture::Impl {
  public:
	std::shared_ptr<CaptureStorage> storage = std::make_shared<CaptureStorage>();
};
MacWindowCapture::MacWindowCapture() : impl_(std::make_unique<Impl>()) {}
MacWindowCapture::~MacWindowCapture() { stop(); }
MacCaptureState MacWindowCapture::state() const noexcept { return impl_->storage->status.load(); }
void MacWindowCapture::start(std::uint32_t number, bool source_is_verified_sdr) noexcept {
	const auto storage = impl_->storage;
	if (storage->status.load() != MacCaptureState::Idle) return;
	if (!source_is_verified_sdr) {
		storage->status.store(MacCaptureState::UnsupportedColor); return;
	}
	if (@available(macOS 14.0, *)) {
		if (![NSThread isMainThread] || !number || NSApp.mainWindow.windowNumber != number) {
			storage->status.store(MacCaptureState::WrongWindow); return;
		}
		if (!CGPreflightScreenCaptureAccess()) {
			storage->status.store(MacCaptureState::ConsentRequired); return;
		}
		storage->status.store(MacCaptureState::Preparing);
		mach_timebase_info(&storage->timebase);
		[SCShareableContent getShareableContentExcludingDesktopWindows:YES onScreenWindowsOnly:YES
		    completionHandler:^(SCShareableContent *shareable, NSError *error) {
			if (storage->canceled.load()) return;
			if (error || !shareable) { fail(storage, MacCaptureState::WindowUnavailable); return; }
			SCWindow *target = nil;
			for (SCWindow *window in shareable.windows) {
				if (window.windowID == number && window.owningApplication.processID == getpid()) {
					target = window; break;
				}
			}
			if (!target) { fail(storage, MacCaptureState::WindowUnavailable); return; }
			SCContentFilter *filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:target];
			const auto extent = filter.contentRect.size;
			const double width = std::ceil(extent.width * filter.pointPixelScale);
			const double height = std::ceil(extent.height * filter.pointPixelScale);
			if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0 ||
			    width > 8192 || height > 8192 || width * height * 4 > 256 * 1024 * 1024) {
				fail(storage, MacCaptureState::UnsupportedGeometry); return;
			}
			try {
				std::lock_guard<std::mutex> lock(storage->pixels_mutex);
				storage->width = static_cast<unsigned>(width);
				storage->height = static_cast<unsigned>(height);
				storage->point_extent = extent;
				storage->pixels.resize(storage->width * storage->height * 4u);
			} catch (const std::bad_alloc &) { fail(storage, MacCaptureState::StartFailed); return; }
			SCStreamConfiguration *config = [[SCStreamConfiguration alloc] init];
			config.width = storage->width; config.height = storage->height;
			config.pixelFormat = kCVPixelFormatType_32BGRA;
			config.minimumFrameInterval = CMTimeMake(1, 60);
			config.queueDepth = 3;
			config.showsCursor = NO;
			config.capturesAudio = NO;
			config.scalesToFit = NO;
			config.ignoreShadowsSingleWindow = YES;
			CGColorRef black = CGColorCreateGenericRGB(0, 0, 0, 1);
			config.backgroundColor = black;
			CGColorRelease(black);
			VaegWindowStreamOutput *output = [[VaegWindowStreamOutput alloc] init];
			output->storage = storage;
			SCStream *stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:output];
			NSError *add_error = nil;
			if (![stream addStreamOutput:output type:SCStreamOutputTypeScreen
			    sampleHandlerQueue:dispatch_queue_create("vaeg.recording.window", DISPATCH_QUEUE_SERIAL)
			    error:&add_error]) { fail(storage, MacCaptureState::StartFailed); return; }
			{
				std::lock_guard<std::mutex> lock(storage->control);
				if (storage->canceled.load()) return;
				storage->stream = stream; storage->output = output;
			}
			[stream startCaptureWithCompletionHandler:^(NSError *start_error) {
				if (storage->canceled.load()) {
					[stream stopCaptureWithCompletionHandler:^(NSError *ignored) { (void)ignored; }];
					return;
				}
				storage->status.store(start_error ? MacCaptureState::StartFailed : MacCaptureState::Running);
			}];
		}];
	} else {
		storage->status.store(MacCaptureState::RuntimeUnavailable);
	}
}
void MacWindowCapture::stop() noexcept {
	const auto storage = impl_->storage;
	storage->canceled.store(true);
	if (@available(macOS 14.0, *)) {
		SCStream *stream;
		{
			std::lock_guard<std::mutex> lock(storage->control);
			stream = storage->stream;
			storage->stream = nil; storage->output = nil;
		}
		if (stream) [stream stopCaptureWithCompletionHandler:^(NSError *ignored) { (void)ignored; }];
	}
	storage->status.store(MacCaptureState::Stopped);
}
bool MacWindowCapture::take_latest(MacCapturedImage *image) {
	if (!image || state() != MacCaptureState::Running) return false;
	const auto storage = impl_->storage;
	std::lock_guard<std::mutex> lock(storage->pixels_mutex);
	if (!storage->pending) return false;
	image->width = storage->width; image->height = storage->height;
	image->display_host_ns = storage->timestamp_ns;
	image->overwritten = storage->overwritten; image->admission_busy = storage->busy.load();
	image->bgra = storage->pixels;
	storage->pending = false;
	return true;
}
} // namespace vaeg::recording
