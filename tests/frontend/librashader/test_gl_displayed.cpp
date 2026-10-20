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
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include "imgui.h"
#include "librashader/gl_bridge.h"

/* Real OpenGL/ImGui readback under X11/Wayland (Xvfb + Mesa is supported).
 * No commercial ROM, shader runtime or guest image is required. */
int main() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_Window *window = SDL_CreateWindow("Displayed test", 0, 0, 64, 48,
	                                     SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
	VAEG_GL_BRIDGE bridge{};
	if (!window || !vaeg_gl_bridge_initialize(window, nullptr, 0, &bridge)) {
		std::fprintf(stderr, "GL initialization failed: %s\n", SDL_GetError());
		return 1;
	}
	ImGui::CreateContext();
	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = ImVec2(64, 48);
	io.DeltaTime = 1.0f / 60.0f;
	bool ok = vaeg_gl_bridge_gui_prepare(&bridge) != 0;
	ImGui::NewFrame();
	ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(12, 8),
	                                            IM_COL32(255, 0, 0, 255));
	ImGui::GetForegroundDrawList()->AddText(ImVec2(16, 0), IM_COL32_WHITE, "VA");
	ImGui::Render();
	const uint32_t source[] = {0xff00ff00u, 0xff00ff00u, 0xff00ff00u, 0xff00ff00u};
	VAEG_FRAME_INPUT frame{};
	frame.pixels = source;
	frame.width = 2;
	frame.height = 2;
	frame.pitch_bytes = 8;
	frame.pixel_format = VAEG_FRAME_PIXEL_ARGB8888;
	frame.frame_time_delta_ns = 16666667u;
	frame.source_aspect_width = 1;
	frame.source_aspect_height = 1;
	frame.source_frame_rate_numerator = 60;
	frame.source_frame_rate_denominator = 1;
	std::vector<uint8_t> pixels(64 * 48 * 4, 0x5a);
	VAEG_OUTPUT_CAPTURE capture{};
	capture.pixels = pixels.data();
	capture.width = 64;
	capture.height = 48;
	capture.pitch_bytes = 64 * 4;
	bridge.capture = &capture;
	auto pixel_store = reinterpret_cast<void (*)(GLenum, GLint)>(SDL_GL_GetProcAddress("glPixelStorei"));
	auto get_integer = reinterpret_cast<void (*)(GLenum, GLint *)>(SDL_GL_GetProcAddress("glGetIntegerv"));
	pixel_store(GL_PACK_ALIGNMENT, 8);
	pixel_store(GL_PACK_ROW_LENGTH, 80);
	pixel_store(GL_PACK_SKIP_ROWS, 1);
	pixel_store(GL_PACK_SKIP_PIXELS, 1);
	ok = ok && vaeg_gl_bridge_present(&bridge, &frame) == VAEG_GL_BRIDGE_OK && capture.complete;
	GLint alignment, row_length, skip_rows, skip_pixels;
	get_integer(GL_PACK_ALIGNMENT, &alignment);
	get_integer(GL_PACK_ROW_LENGTH, &row_length);
	get_integer(GL_PACK_SKIP_ROWS, &skip_rows);
	get_integer(GL_PACK_SKIP_PIXELS, &skip_pixels);
	ok = ok && alignment == 8 && row_length == 80 && skip_rows == 1 && skip_pixels == 1;
	auto color = [&](unsigned x, unsigned y, unsigned r, unsigned g, unsigned b) {
		const auto *p = pixels.data() + (y * 64 + x) * 4;
		return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
	};
	ok = ok && color(2, 2, 255, 0, 0) && color(32, 24, 0, 255, 0) &&
	     color(2, 45, 0, 0, 0);
	const auto first = pixels;
	bridge.capture = nullptr;
	ok = ok && vaeg_gl_bridge_present(&bridge, &frame) == VAEG_GL_BRIDGE_OK && pixels == first;
	bridge.capture = &capture;
	capture.width = 63;
	capture.complete = 1;
	ok = ok && vaeg_gl_bridge_present(&bridge, &frame) == VAEG_GL_BRIDGE_OK && !capture.complete && pixels == first;
	capture.width = 64;
	capture.complete = 1;
	frame.width = 0;
	ok = ok && vaeg_gl_bridge_present(&bridge, &frame) == VAEG_GL_BRIDGE_INVALID_FRAME && !capture.complete;
	vaeg_gl_bridge_gui_shutdown(&bridge);
	ImGui::DestroyContext();
	vaeg_gl_bridge_shutdown(&bridge);
	SDL_DestroyWindow(window);
	SDL_Quit();
	std::fprintf(stderr, "GL final-client GUI/RGBA/top-down/off/invalid geometry: %s\n", ok ? "PASS" : "FAIL");
	return ok ? 0 : 1;
}
