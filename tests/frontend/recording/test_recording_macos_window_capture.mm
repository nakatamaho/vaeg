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
#include <cstdio>
#include <cstring>
#include <cstdlib>
using namespace vaeg::recording;
static void check(bool condition, const char *code) {
	if (!condition) { std::fprintf(stderr, "%s\n", code); std::exit(1); }
}
@interface VaegPublicCaptureView : NSView
@end
@implementation VaegPublicCaptureView
- (void)drawRect:(NSRect)dirtyRect {
	(void)dirtyRect;
	[[NSColor colorWithSRGBRed:1 green:0 blue:0 alpha:1] setFill];
	NSRectFill(self.bounds);
}
@end
int main(int argc, char **argv) {
	@autoreleasepool {
		MacWindowCapture invalid;
		check(invalid.state() == MacCaptureState::Idle, "OSR_MAC_INITIAL");
		invalid.start(0, true);
		if (@available(macOS 14.0, *)) {
			check(invalid.state() == MacCaptureState::WrongWindow, "OSR_MAC_WRONG_WINDOW");
		} else {
			check(invalid.state() == MacCaptureState::RuntimeUnavailable, "OSR_MAC_OLD_RUNTIME");
		}
		const auto failed_state = invalid.state();
		invalid.stop(); invalid.stop();
		check(invalid.state() == failed_state, "OSR_MAC_STOP_PRESERVES_ERROR");
		MacWindowCapture idle;
		idle.stop(); idle.stop();
		check(idle.state() == MacCaptureState::Stopped, "OSR_MAC_IDEMPOTENT_STOP");
		MacWindowCapture hdr;
		hdr.start(0, false);
		check(hdr.state() == MacCaptureState::UnsupportedColor, "OSR_MAC_REJECT_UNVERIFIED_COLOR");
		if (argc > 1 && std::strcmp(argv[1], "--contracts") == 0) {
			std::puts("OSR_MAC_CAPTURE_CONTRACT_PASS"); return 0;
		}
		if (!CGPreflightScreenCaptureAccess()) {
			std::puts("OSR_MAC_CONSENT_REQUIRED: runtime capture NOT_RUN; no permission request made");
			return 77;
		}
		if (@available(macOS 14.0, *)) {
			[NSApplication sharedApplication];
			[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
			NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 100, 641, 401)
			    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
			    backing:NSBackingStoreBuffered defer:NO];
			window.title = @"VAEG public recording capture fixture";
			window.colorSpace = NSColorSpace.sRGBColorSpace;
			window.contentView = [[VaegPublicCaptureView alloc] initWithFrame:window.contentView.bounds];
			[window makeKeyAndOrderFront:nil];
			[window makeMainWindow];
			[NSApp activateIgnoringOtherApps:YES];
			MacWindowCapture capture;
			capture.start(static_cast<std::uint32_t>(window.windowNumber), true);
			MacCapturedImage image;
			NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:10];
			bool obtained = false;
			while (deadline.timeIntervalSinceNow > 0) {
				[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
				if (capture.take_latest(&image)) { obtained = true; break; }
				if (capture.state() != MacCaptureState::Preparing && capture.state() != MacCaptureState::Running) break;
			}
			std::printf("state=%s width=%u height=%u host_ns=%llu\n", mac_capture_state_name(capture.state()),
			            image.width, image.height, static_cast<unsigned long long>(image.display_host_ns));
			capture.stop();
			[window orderOut:nil];
			check(obtained, "OSR_MAC_CAPTURE_NO_FRAME");
			check(image.width > 0 && image.height > 0 && image.display_host_ns > 0 &&
			      image.bgra.size() == static_cast<std::size_t>(image.width) * image.height * 4,
			      "OSR_MAC_CAPTURE_OWNED_EXTENT");
			const auto bottom = ((image.height * 3 / 4) * image.width + image.width / 2) * 4;
			check(image.bgra[bottom + 2] > 240 && image.bgra[bottom] < 10 &&
			      image.bgra[bottom + 1] < 10, "OSR_MAC_CAPTURE_CLIENT_COLOR");
			std::puts("OSR_MAC_CAPTURE_FRAME_PASS; real title/border coverage still requires visual review");
			return 0;
		}
		std::puts("OSR_MAC_RUNTIME_UNAVAILABLE"); return 77;
	}
}
