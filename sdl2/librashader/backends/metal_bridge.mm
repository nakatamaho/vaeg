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

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <SDL_metal.h>

#include "librashader/frame_conversion.h"
#include "librashader/librashader_loader.h"
#include "librashader/metal_bridge.h"

struct VAEG_METAL_STATE {
	SDL_MetalView view;
	CAMetalLayer *layer;
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;
	id<MTLRenderPipelineState> pipeline;
	id<MTLTexture> source_texture;
	uint8_t *upload_buffer;
	size_t upload_capacity;
	uint32_t source_width;
	uint32_t source_height;
	uint32_t upload_pitch;
	libra_instance_t librashader;
	libra_mtl_filter_chain_t filter_chain;
	bool filter_enabled;
	bool filter_first_frame;
};

static const char vaeg_metal_passthrough_shader[] = R"metal(
#include <metal_stdlib>
using namespace metal;

struct VAEGVertexOut {
    float4 position [[position]];
    float2 texcoord;
};

vertex VAEGVertexOut vaeg_metal_vertex(uint vertex_id [[vertex_id]]) {
    const float2 positions[4] = {
        float2(-1.0, -1.0), float2(1.0, -1.0),
        float2(-1.0, 1.0), float2(1.0, 1.0)
    };
    const float2 texcoords[4] = {
        float2(0.0, 1.0), float2(1.0, 1.0),
        float2(0.0, 0.0), float2(1.0, 0.0)
    };
    VAEGVertexOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.texcoord = texcoords[vertex_id];
    return output;
}

fragment float4 vaeg_metal_fragment(
    VAEGVertexOut input [[stage_in]],
    texture2d<float, access::sample> source [[texture(0)]]) {
    constexpr sampler source_sampler(filter::nearest, address::clamp_to_edge);
    return source.sample(source_sampler, input.texcoord);
}
)metal";

static void vaeg_metal_release_state(VAEG_METAL_STATE *state) {
	if (state == nullptr) {
		return;
	}
	if (state->source_texture != nil) {
		[state->source_texture release];
	}
	if ((state->filter_chain != nullptr) &&
	    (state->librashader.mtl_filter_chain_free != nullptr)) {
		(void)state->librashader.mtl_filter_chain_free(&state->filter_chain);
	}
	if (state->pipeline != nil) {
		[state->pipeline release];
	}
	if (state->queue != nil) {
		[state->queue release];
	}
	if (state->device != nil) {
		[state->device release];
	}
	if (state->view != nullptr) {
		SDL_Metal_DestroyView(state->view);
	}
	free(state->upload_buffer);
	free(state);
}

static void vaeg_metal_report_librashader_error(VAEG_METAL_STATE *state, libra_error_t error,
                                                 const char *operation) {
	if (error == nullptr) {
		return;
	}
	fprintf(stderr, "librashader Metal %s failed\n", operation);
	if (state->librashader.error_print != nullptr) {
		(void)state->librashader.error_print(error);
	}
	if (state->librashader.error_free != nullptr) {
		(void)state->librashader.error_free(&error);
	}
}

static int vaeg_metal_create_filter_chain(VAEG_METAL_STATE *state, const char *preset_path) {
	libra_shader_preset_t preset;
	filter_chain_mtl_opt_t options;
	libra_error_t error;

	if ((preset_path == nullptr) || (preset_path[0] == '\0')) {
		return 0;
	}
	state->librashader = librashader_load_instance();
	if (!state->librashader.instance_loaded) {
		return 0;
	}
	preset = nullptr;
	error = state->librashader.preset_create(preset_path, &preset);
	if ((error != nullptr) || (preset == nullptr)) {
		vaeg_metal_report_librashader_error(state, error, "preset creation");
		return 0;
	}
	memset(&options, 0, sizeof(options));
	options.version = LIBRASHADER_CURRENT_VERSION;
	error = state->librashader.mtl_filter_chain_create(
		&preset, state->queue, &options, &state->filter_chain);
	if ((error != nullptr) || (state->filter_chain == nullptr)) {
		vaeg_metal_report_librashader_error(state, error, "filter-chain creation");
		return 0;
	}
	state->filter_enabled = true;
	state->filter_first_frame = true;
	return 1;
}

static MTLViewport vaeg_metal_viewport(const VAEG_METAL_STATE *state,
                                       const VAEG_FRAME_INPUT *frame) {
	double output_aspect;
	double source_aspect;
	MTLViewport viewport;

	viewport.originX = 0.0;
	viewport.originY = 0.0;
	viewport.width = state->layer.drawableSize.width;
	viewport.height = state->layer.drawableSize.height;
	viewport.znear = 0.0;
	viewport.zfar = 1.0;
	if ((viewport.width <= 0.0) || (viewport.height <= 0.0)) {
		return viewport;
	}
	source_aspect = static_cast<double>(frame->source_aspect_width) /
	                static_cast<double>(frame->source_aspect_height);
	output_aspect = viewport.width / viewport.height;
	if (source_aspect > output_aspect) {
		const double fitted_height = viewport.width / source_aspect;
		viewport.originY = (viewport.height - fitted_height) * 0.5;
		viewport.height = fitted_height;
	} else {
		const double fitted_width = viewport.height * source_aspect;
		viewport.originX = (viewport.width - fitted_width) * 0.5;
		viewport.width = fitted_width;
	}
	return viewport;
}

static int vaeg_metal_ensure_source_texture(VAEG_METAL_STATE *state, uint32_t width,
                                             uint32_t height) {
	MTLTextureDescriptor *descriptor;
	const size_t required_capacity = static_cast<size_t>(width) * height * 4U;

	if ((width == 0) || (height == 0) || (required_capacity / 4U != static_cast<size_t>(width) * height)) {
		return 0;
	}
	if ((state->source_texture != nil) && (state->source_width == width) &&
	    (state->source_height == height) && (state->upload_capacity >= required_capacity)) {
		return 1;
	}
	if (state->source_texture != nil) {
		[state->source_texture release];
		state->source_texture = nil;
	}
	if (required_capacity > state->upload_capacity) {
		uint8_t *new_buffer = static_cast<uint8_t *>(realloc(state->upload_buffer, required_capacity));
		if (new_buffer == nullptr) {
			return 0;
		}
		state->upload_buffer = new_buffer;
		state->upload_capacity = required_capacity;
	}
	descriptor = [[MTLTextureDescriptor alloc] init];
	descriptor.textureType = MTLTextureType2D;
	descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
	descriptor.width = width;
	descriptor.height = height;
	descriptor.mipmapLevelCount = 1;
	descriptor.usage = MTLTextureUsageShaderRead;
	descriptor.storageMode = MTLStorageModeShared;
	state->source_texture = [state->device newTextureWithDescriptor:descriptor];
	[descriptor release];
	if (state->source_texture == nil) {
		return 0;
	}
	state->source_width = width;
	state->source_height = height;
	state->upload_pitch = width * 4U;
	return 1;
}

extern "C" int vaeg_metal_bridge_initialize(void *host_window, const char *preset_path,
                                               int enable_filter, VAEG_METAL_BRIDGE *bridge) {
	VAEG_METAL_STATE *state;
	MTLRenderPipelineDescriptor *descriptor;
	id<MTLLibrary> library;
	id<MTLFunction> vertex_function;
	id<MTLFunction> fragment_function;
	NSError *error;

	if ((host_window == nullptr) || (bridge == nullptr)) {
		return 0;
	}
	bridge->state = nullptr;
	state = static_cast<VAEG_METAL_STATE *>(calloc(1, sizeof(*state)));
	if (state == nullptr) {
		return 0;
	}
	state->view = SDL_Metal_CreateView(static_cast<SDL_Window *>(host_window));
	if (state->view == nullptr) {
		vaeg_metal_release_state(state);
		return 0;
	}
	state->layer = (__bridge CAMetalLayer *)SDL_Metal_GetLayer(state->view);
	state->device = MTLCreateSystemDefaultDevice();
	if ((state->layer == nil) || (state->device == nil)) {
		vaeg_metal_release_state(state);
		return 0;
	}
	state->layer.device = state->device;
	state->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	state->layer.framebufferOnly = NO;
	state->queue = [state->device newCommandQueue];
	if (state->queue == nil) {
		vaeg_metal_release_state(state);
		return 0;
	}
	error = nil;
	library = [state->device newLibraryWithSource:[NSString stringWithUTF8String:vaeg_metal_passthrough_shader]
	                                         options:nil error:&error];
	if (library == nil) {
		vaeg_metal_release_state(state);
		return 0;
	}
	vertex_function = [library newFunctionWithName:@"vaeg_metal_vertex"];
	fragment_function = [library newFunctionWithName:@"vaeg_metal_fragment"];
	descriptor = [[MTLRenderPipelineDescriptor alloc] init];
	descriptor.vertexFunction = vertex_function;
	descriptor.fragmentFunction = fragment_function;
	descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
	state->pipeline = [state->device newRenderPipelineStateWithDescriptor:descriptor error:&error];
	[descriptor release];
	[fragment_function release];
	[vertex_function release];
	[library release];
	if (state->pipeline == nil) {
		vaeg_metal_release_state(state);
		return 0;
	}
	if ((enable_filter != 0) && !vaeg_metal_create_filter_chain(state, preset_path)) {
		vaeg_metal_release_state(state);
		return 0;
	}
	bridge->state = state;
	return 1;
}

extern "C" VAEG_METAL_BRIDGE_RESULT vaeg_metal_bridge_set_filter_enabled(
	VAEG_METAL_BRIDGE *bridge, int enabled) {
	VAEG_METAL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return VAEG_METAL_BRIDGE_INVALID_ARGUMENT;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	if ((enabled != 0) && (state->filter_chain == nullptr)) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	if ((enabled != 0) && !state->filter_enabled) {
		state->filter_first_frame = true;
	}
	state->filter_enabled = (enabled != 0);
	return VAEG_METAL_BRIDGE_OK;
}

extern "C" void vaeg_metal_bridge_set_drawable_size(const VAEG_METAL_BRIDGE *bridge,
                                                      uint32_t width, uint32_t height) {
	VAEG_METAL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	if ((width != 0) && (height != 0)) {
		state->layer.drawableSize = CGSizeMake(static_cast<CGFloat>(width), static_cast<CGFloat>(height));
	}
}

extern "C" VAEG_METAL_BRIDGE_RESULT vaeg_metal_bridge_present(
	VAEG_METAL_BRIDGE *bridge, const VAEG_FRAME_INPUT *frame) {
	VAEG_METAL_STATE *state;
	MTLRenderPassDescriptor *pass_descriptor;
	id<CAMetalDrawable> drawable;
	id<MTLCommandBuffer> command_buffer;
	id<MTLRenderCommandEncoder> encoder;
	MTLViewport viewport;
	libra_viewport_t libra_viewport;
	frame_mtl_opt_t filter_options;
	libra_error_t error;

	if ((bridge == nullptr) || (bridge->state == nullptr) || (frame == nullptr)) {
		return VAEG_METAL_BRIDGE_INVALID_ARGUMENT;
	}
	if (vaeg_frame_input_validate(frame) != VAEG_FRAME_INPUT_OK) {
		return VAEG_METAL_BRIDGE_INVALID_FRAME;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	if (!vaeg_metal_ensure_source_texture(state, frame->width, frame->height)) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	if (vaeg_frame_convert_rgba8888(frame, state->upload_buffer, state->upload_pitch,
	                                state->upload_capacity) != VAEG_FRAME_CONVERSION_OK) {
		return VAEG_METAL_BRIDGE_INVALID_FRAME;
	}
	[state->source_texture replaceRegion:MTLRegionMake2D(0, 0, state->source_width, state->source_height)
	                       mipmapLevel:0
	                         withBytes:state->upload_buffer
	                       bytesPerRow:state->upload_pitch];
	drawable = [state->layer nextDrawable];
	if (drawable == nil) {
		return VAEG_METAL_BRIDGE_NO_DRAWABLE;
	}
	command_buffer = [state->queue commandBuffer];
	if (command_buffer == nil) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	viewport = vaeg_metal_viewport(state, frame);
	if ((viewport.width <= 0.0) || (viewport.height <= 0.0)) {
		return VAEG_METAL_BRIDGE_NO_DRAWABLE;
	}
	if (state->filter_enabled) {
		libra_viewport.x = static_cast<float>(viewport.originX);
		libra_viewport.y = static_cast<float>(viewport.originY);
		libra_viewport.width = static_cast<uint32_t>(viewport.width);
		libra_viewport.height = static_cast<uint32_t>(viewport.height);
		memset(&filter_options, 0, sizeof(filter_options));
		filter_options.version = LIBRASHADER_CURRENT_VERSION;
		filter_options.clear_history = state->filter_first_frame;
		filter_options.frame_direction = 1;
		filter_options.rotation = 0;
		filter_options.total_subframes = 1;
		filter_options.current_subframe = 1;
		filter_options.aspect_ratio = static_cast<float>(frame->source_aspect_width) /
		                              static_cast<float>(frame->source_aspect_height);
		filter_options.frames_per_second =
			static_cast<float>(frame->source_frame_rate_numerator) /
			static_cast<float>(frame->source_frame_rate_denominator);
		filter_options.frametime_delta = static_cast<uint32_t>(frame->frame_time_delta_ns / 1000000U);
		error = state->librashader.mtl_filter_chain_frame(
			&state->filter_chain, command_buffer, 1, state->source_texture, drawable.texture,
			&libra_viewport, nullptr, &filter_options);
		if (error != nullptr) {
			vaeg_metal_report_librashader_error(state, error, "frame rendering");
			return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
		}
		state->filter_first_frame = false;
		[command_buffer presentDrawable:drawable];
		[command_buffer commit];
		return VAEG_METAL_BRIDGE_OK;
	}
	pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
	pass_descriptor.colorAttachments[0].texture = drawable.texture;
	pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
	pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
	pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
	encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_descriptor];
	if (encoder == nil) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	[encoder setViewport:viewport];
	[encoder setRenderPipelineState:state->pipeline];
	[encoder setFragmentTexture:state->source_texture atIndex:0];
	[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
	[encoder endEncoding];
	[command_buffer presentDrawable:drawable];
	[command_buffer commit];
	return VAEG_METAL_BRIDGE_OK;
}

extern "C" void vaeg_metal_bridge_shutdown(VAEG_METAL_BRIDGE *bridge) {
	VAEG_METAL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	bridge->state = nullptr;
	vaeg_metal_release_state(state);
}
