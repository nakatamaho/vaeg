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
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <SDL_metal.h>

#include "librashader/librashader_loader.h"
#include "librashader/metal_bridge.h"

extern "C" int vaeg_metal_bridge_initialize(void *host_window, VAEG_METAL_BRIDGE *bridge) {
	SDL_MetalView view;
	CAMetalLayer *layer;
	id<MTLDevice> device;

	if ((host_window == nullptr) || (bridge == nullptr)) {
		return 0;
	}
	bridge->view = nullptr;
	bridge->layer = nullptr;
	bridge->device = nullptr;
	view = SDL_Metal_CreateView(static_cast<SDL_Window *>(host_window));
	if (view == nullptr) {
		return 0;
	}
	layer = (__bridge CAMetalLayer *)SDL_Metal_GetLayer(view);
	device = MTLCreateSystemDefaultDevice();
	if ((layer == nil) || (device == nil)) {
		SDL_Metal_DestroyView(view);
		return 0;
	}
	layer.device = device;
	layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	layer.framebufferOnly = NO;
	bridge->view = view;
	bridge->layer = (__bridge void *)layer;
	bridge->device = (__bridge void *)device;
	return 1;
}

extern "C" void vaeg_metal_bridge_set_drawable_size(const VAEG_METAL_BRIDGE *bridge,
                                                      uint32_t width, uint32_t height) {
	CAMetalLayer *layer;

	if ((bridge == nullptr) || (bridge->layer == nullptr)) {
		return;
	}
	layer = (__bridge CAMetalLayer *)bridge->layer;
	layer.drawableSize = CGSizeMake(static_cast<CGFloat>(width), static_cast<CGFloat>(height));
}

extern "C" void vaeg_metal_bridge_shutdown(VAEG_METAL_BRIDGE *bridge) {
	if ((bridge == nullptr) || (bridge->view == nullptr)) {
		return;
	}
	SDL_Metal_DestroyView(static_cast<SDL_MetalView>(bridge->view));
	bridge->view = nullptr;
	bridge->layer = nullptr;
	bridge->device = nullptr;
}
