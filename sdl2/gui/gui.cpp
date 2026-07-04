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
#include "sdlapi.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "compiler.h"
#include "gui/gui.h"

namespace {

constexpr const char *kFontName = "NotoSansJP-Regular.ttf";

struct GuiState {
	bool initialized = false;
	SDL_Renderer *renderer = nullptr;
	std::string font_path;
};

GuiState g_gui;

static bool file_exists(const std::string &path) {

	std::ifstream f(path.c_str(), std::ios::binary);
	return f.good();
}

static std::string join_path(const std::string &base, const char *leaf) {

	if (base.empty()) {
		return std::string(leaf);
	}
	if ((base.back() == '/') || (base.back() == '\\')) {
		return base + leaf;
	}
	return base + "/" + leaf;
}

static void add_asset_candidate(std::vector<std::string> &paths,
								const std::string &base) {

	paths.push_back(join_path(base, kFontName));
}

static std::string find_font_path(void) {

	std::vector<std::string> paths;
	const char *env = std::getenv("VAEG_ASSET_DIR");
	if ((env != nullptr) && (env[0] != '\0')) {
		add_asset_candidate(paths, env);
	}
	add_asset_candidate(paths, "assets");

	char *base_path = SDL_GetBasePath();
	if (base_path != nullptr) {
		std::string base(base_path);
		add_asset_candidate(paths, join_path(base, "assets"));
		add_asset_candidate(paths, join_path(base, "../assets"));
		add_asset_candidate(paths, join_path(base, "../../assets"));
		add_asset_candidate(paths, join_path(base, "../../../assets"));
		add_asset_candidate(paths, join_path(base, "../share/vaeg/assets"));
		add_asset_candidate(paths, join_path(base, "../share/vaeg"));
		SDL_free(base_path);
	}

	for (const auto &path : paths) {
		if (file_exists(path)) {
			return path;
		}
	}
	return std::string();
}

static void menu_item_not_implemented(const char *label) {

	ImGui::BeginDisabled();
	ImGui::MenuItem(label);
	ImGui::EndDisabled();
}

static void draw_emulate_menu(void) {

	if (ImGui::BeginMenu("Emulate / エミュレート")) {
		menu_item_not_implemented("Reset / リセット (not implemented)");
		ImGui::Separator();
		menu_item_not_implemented("Configure... (not implemented)");
		menu_item_not_implemented("NewDisk... (not implemented)");
		menu_item_not_implemented("Font... (not implemented)");
		ImGui::Separator();
		menu_item_not_implemented("Exit / 終了 (not implemented)");
		ImGui::EndMenu();
	}
}

static void draw_fdd_menu(void) {

	if (ImGui::BeginMenu("FDD")) {
		menu_item_not_implemented("FDD1 Open... (not implemented)");
		menu_item_not_implemented("FDD1 Eject (not implemented)");
		ImGui::Separator();
		menu_item_not_implemented("FDD2 Open... (not implemented)");
		menu_item_not_implemented("FDD2 Eject (not implemented)");
		ImGui::Separator();
		menu_item_not_implemented("FDD3 Open... (not implemented)");
		menu_item_not_implemented("FDD3 Eject (not implemented)");
		menu_item_not_implemented("FDD4 Open... (not implemented)");
		menu_item_not_implemented("FDD4 Eject (not implemented)");
		ImGui::EndMenu();
	}
}

static void draw_harddisk_menu(void) {

	if (ImGui::BeginMenu("HardDisk")) {
		menu_item_not_implemented("SASI-1 Open... (not implemented)");
		menu_item_not_implemented("SASI-1 Remove (not implemented)");
		menu_item_not_implemented("SASI-2 Open... (not implemented)");
		menu_item_not_implemented("SASI-2 Remove (not implemented)");
		ImGui::EndMenu();
	}
}

static void draw_screen_menu(void) {

	if (ImGui::BeginMenu("Screen / 画面")) {
		menu_item_not_implemented("Scale x1 (not implemented)");
		menu_item_not_implemented("Scale x2 (not implemented)");
		menu_item_not_implemented("Scale x3 (not implemented)");
		menu_item_not_implemented("Aspect correction (not implemented)");
		ImGui::Separator();
		menu_item_not_implemented("FullScreen (not implemented)");
		menu_item_not_implemented("Rotate left/right (not implemented)");
		menu_item_not_implemented("Screen option... (not implemented)");
		ImGui::EndMenu();
	}
}

static void draw_device_menu(void) {

	if (ImGui::BeginMenu("Device / デバイス")) {
		if (ImGui::BeginMenu("Keyboard / キーボード")) {
			menu_item_not_implemented("Keyboard (not implemented)");
			menu_item_not_implemented("JoyKey-1 (not implemented)");
			menu_item_not_implemented("JoyKey-2 (not implemented)");
			menu_item_not_implemented("F12 binding (not implemented)");
			menu_item_not_implemented("Mechanical keys (not implemented)");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Sound / 音")) {
			menu_item_not_implemented("Sound on/off (not implemented)");
			menu_item_not_implemented("Volume (not implemented)");
			ImGui::Separator();
			menu_item_not_implemented("Board selection (not implemented)");
			menu_item_not_implemented("Sound option... (not implemented)");
			ImGui::EndMenu();
		}
		menu_item_not_implemented("Memory (not implemented)");
		menu_item_not_implemented("Mouse (not implemented)");
		menu_item_not_implemented("Serial option... (not implemented)");
		menu_item_not_implemented("MIDI option... (not implemented)");
		ImGui::EndMenu();
	}
}

static void draw_other_menu(void) {

	if (ImGui::BeginMenu("Other")) {
		menu_item_not_implemented("BMP Save... (not implemented)");
		menu_item_not_implemented("S98 logging... (not implemented)");
		menu_item_not_implemented("Calendar... (not implemented)");
		menu_item_not_implemented("Clock/Frame display (not implemented)");
		menu_item_not_implemented("Help... (not implemented)");
		menu_item_not_implemented("About... (not implemented)");
		ImGui::EndMenu();
	}
}

static void draw_state_menu(void) {

	if (ImGui::BeginMenu("State / 状態")) {
		menu_item_not_implemented("Save slot 0 (not implemented)");
		menu_item_not_implemented("Load slot 0 (not implemented)");
		ImGui::EndMenu();
	}
}

static void draw_system_menu(void) {

	if (ImGui::BeginMenu("System")) {
		menu_item_not_implemented("Tool window (not implemented)");
		menu_item_not_implemented("Key display (not implemented)");
		menu_item_not_implemented("Soft keyboard (not implemented)");
		menu_item_not_implemented("Debugger utility (not implemented)");
		ImGui::EndMenu();
	}
}

} // namespace

BOOL gui_initialize(void *window, void *renderer, const char *argv0) {

	(void)argv0;
	if ((window == nullptr) || (renderer == nullptr)) {
		return FAILURE;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	g_gui.font_path = find_font_path();
	if (g_gui.font_path.empty()) {
		std::fprintf(stderr, "Error: %s not found in GUI asset paths\n",
					 kFontName);
		return FAILURE;
	}
	if (io.Fonts->AddFontFromFileTTF(g_gui.font_path.c_str(), 18.0f,
									 nullptr,
									 io.Fonts->GetGlyphRangesJapanese()) ==
		nullptr) {
		std::fprintf(stderr, "Error: failed to load GUI font: %s\n",
					 g_gui.font_path.c_str());
		return FAILURE;
	}

	ImGui::StyleColorsDark();
	if (!ImGui_ImplSDL2_InitForSDLRenderer(static_cast<SDL_Window *>(window),
										   static_cast<SDL_Renderer *>(renderer))) {
		return FAILURE;
	}
	g_gui.renderer = static_cast<SDL_Renderer *>(renderer);
	if (!ImGui_ImplSDLRenderer2_Init(g_gui.renderer)) {
		ImGui_ImplSDL2_Shutdown();
		return FAILURE;
	}
	g_gui.initialized = true;
	return SUCCESS;
}

void gui_shutdown(void) {

	if (!g_gui.initialized) {
		return;
	}
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	g_gui = GuiState{};
}

BOOL gui_process_event(const void *event) {

	if ((!g_gui.initialized) || (event == nullptr)) {
		return FALSE;
	}
	const SDL_Event *sdl_event = static_cast<const SDL_Event *>(event);
	ImGui_ImplSDL2_ProcessEvent(sdl_event);

	ImGuiIO &io = ImGui::GetIO();
	switch (sdl_event->type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_TEXTINPUT:
			return io.WantCaptureKeyboard ? TRUE : FALSE;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
			return io.WantCaptureMouse ? TRUE : FALSE;

		default:
			return FALSE;
	}
}

void gui_new_frame(void) {

	if (!g_gui.initialized) {
		return;
	}
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void gui_draw(void) {

	if (!g_gui.initialized) {
		return;
	}
	if (ImGui::BeginMainMenuBar()) {
		draw_emulate_menu();
		draw_fdd_menu();
		draw_harddisk_menu();
		draw_screen_menu();
		draw_device_menu();
		draw_other_menu();
		draw_state_menu();
		draw_system_menu();
		ImGui::EndMainMenuBar();
	}
}

void gui_render(void) {

	if (!g_gui.initialized) {
		return;
	}
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),
										  g_gui.renderer);
}
