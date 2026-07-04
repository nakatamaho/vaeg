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
#include "diskdrv.h"
#include "dosio.h"
#include "fddfile.h"
#include "keystat.h"
#include "np2.h"
#include "sound.h"
#include "adpcm.h"
#include "pccore.h"
#include "opngen.h"
#include "pcm86.h"
#include "psggen.h"
#include "rhythm.h"
#include "scrnmng.h"
#include "sdlkbd.h"
#include "soundmng.h"
#include "sysmng.h"
#include "taskmng.h"

namespace {

constexpr const char *kFontName = "NotoSansJP-Regular.ttf";
constexpr int kStateSlots = 10;

struct GuiState {
	bool initialized = false;
	SDL_Renderer *renderer = nullptr;
	std::string font_path;
	int fdd_dialog_drive = -1;
	char fdd_path[2][MAX_PATH] = {};
	std::string fdd_status;
	std::string state_status;
	UINT sound_sw_saved = 0;
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

static bool file_is_mountable(const char *path, std::string *error) {

	short attr;

	if ((path == nullptr) || (path[0] == '\0')) {
		*error = "FDD image path is empty.";
		return false;
	}
	attr = file_attr(path);
	if (attr == static_cast<short>(-1)) {
		*error = "FDD image not found.";
		return false;
	}
	if ((attr & FILEATTR_DIRECTORY) != 0) {
		*error = "FDD image path is a directory.";
		return false;
	}
	error->clear();
	return true;
}

static void set_fdd_status(int drive, const char *action, const char *path) {

	g_gui.fdd_status = "FDD";
	g_gui.fdd_status += static_cast<char>('1' + drive);
	g_gui.fdd_status += ' ';
	g_gui.fdd_status += action;
	if ((path != nullptr) && (path[0] != '\0')) {
		g_gui.fdd_status += ": ";
		g_gui.fdd_status += path;
	}
}

static void open_fdd_dialog(int drive) {

	const char *current;

	g_gui.fdd_dialog_drive = drive;
	current = fdd_diskname(static_cast<REG8>(drive));
	if ((current != nullptr) && (current[0] != '\0')) {
		milstr_ncpy(g_gui.fdd_path[drive], current,
					sizeof(g_gui.fdd_path[drive]));
	}
	ImGui::OpenPopup("Mount FDD image");
}

static void mount_fdd_from_dialog(void) {

	int drive;
	const char *path;
	std::string error;

	drive = g_gui.fdd_dialog_drive;
	if ((drive < 0) || (drive >= 2)) {
		return;
	}
	path = g_gui.fdd_path[drive];
	if (!file_is_mountable(path, &error)) {
		g_gui.fdd_status = "FDD";
		g_gui.fdd_status += static_cast<char>('1' + drive);
		g_gui.fdd_status += " mount failed: ";
		g_gui.fdd_status += error;
		return;
	}
	diskdrv_setfdd(static_cast<REG8>(drive), path, 0);
	set_fdd_status(drive, "mounted", path);
	ImGui::CloseCurrentPopup();
}

static void eject_fdd(int drive) {

	diskdrv_setfdd(static_cast<REG8>(drive), nullptr, 0);
	set_fdd_status(drive, "ejected", nullptr);
}

static void draw_fdd_dialog(void) {

	int drive;

	if (ImGui::BeginPopupModal("Mount FDD image", nullptr,
							  ImGuiWindowFlags_AlwaysAutoResize)) {
		drive = g_gui.fdd_dialog_drive;
		if ((drive >= 0) && (drive < 2)) {
			ImGui::Text("FDD%d image path", drive + 1);
			ImGui::SetNextItemWidth(560.0f);
			ImGui::InputText("##fdd-path", g_gui.fdd_path[drive],
							 sizeof(g_gui.fdd_path[drive]));
			if (ImGui::Button("Mount")) {
				mount_fdd_from_dialog();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				g_gui.fdd_dialog_drive = -1;
				ImGui::CloseCurrentPopup();
			}
			if (!g_gui.fdd_status.empty()) {
				ImGui::Separator();
				ImGui::TextWrapped("%s", g_gui.fdd_status.c_str());
			}
		}
		ImGui::EndPopup();
	}
}

static void draw_emulate_menu(void) {

	if (ImGui::BeginMenu("Emulate / エミュレート")) {
		if (ImGui::MenuItem("Reset / リセット")) {
			pccore_cfgupdate();
			pccore_reset();
		}
		ImGui::Separator();
		menu_item_not_implemented("Configure... (not implemented)");
		menu_item_not_implemented("NewDisk... (not implemented)");
		menu_item_not_implemented("Font... (not implemented)");
		ImGui::Separator();
		if (ImGui::MenuItem("Exit / 終了")) {
			taskmng_exit();
		}
		ImGui::EndMenu();
	}
}

static void draw_fdd_menu(void) {

	if (ImGui::BeginMenu("FDD")) {
		if (ImGui::MenuItem("FDD1 Open...")) {
			open_fdd_dialog(0);
		}
		if (ImGui::MenuItem("FDD1 Eject")) {
			eject_fdd(0);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("FDD2 Open...")) {
			open_fdd_dialog(1);
		}
		if (ImGui::MenuItem("FDD2 Eject")) {
			eject_fdd(1);
		}
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

static void set_display_scale(int scale) {

	np2oscfg.gui_scale = static_cast<BYTE>(scale);
	scrnmng_set_display(np2oscfg.gui_scale, np2oscfg.gui_aspect);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_display_aspect(bool aspect) {

	np2oscfg.gui_aspect = aspect ? 1 : 0;
	scrnmng_set_display(np2oscfg.gui_scale, np2oscfg.gui_aspect);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_key_mode(BYTE mode) {

	np2cfg.KEY_MODE = mode;
	keystat_resetjoykey();
	sysmng_update(SYS_UPDATECFG);
}

static void set_f12_key(BYTE mode) {

	np2oscfg.F12KEY = mode;
	sdlkbd_resetf12();
	sysmng_update(SYS_UPDATEOSCFG);
}

static void draw_screen_menu(void) {

	if (ImGui::BeginMenu("Screen / 画面")) {
		if (ImGui::MenuItem("Scale x1", nullptr, np2oscfg.gui_scale == 1)) {
			set_display_scale(1);
		}
		if (ImGui::MenuItem("Scale x2", nullptr, np2oscfg.gui_scale == 2)) {
			set_display_scale(2);
		}
		if (ImGui::MenuItem("Scale x3", nullptr, np2oscfg.gui_scale == 3)) {
			set_display_scale(3);
		}
		bool aspect = np2oscfg.gui_aspect != 0;
		if (ImGui::MenuItem("Aspect correction", nullptr, aspect)) {
			set_display_aspect(!aspect);
		}
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
			if (ImGui::MenuItem("Keyboard", nullptr,
								np2cfg.KEY_MODE == 0)) {
				set_key_mode(0);
			}
			if (ImGui::MenuItem("JoyKey-1", nullptr,
								np2cfg.KEY_MODE == 1)) {
				set_key_mode(1);
			}
			if (ImGui::MenuItem("JoyKey-2", nullptr,
								np2cfg.KEY_MODE == 2)) {
				set_key_mode(2);
			}
			if (ImGui::MenuItem("Mouse key", nullptr,
								np2cfg.KEY_MODE == 3)) {
				set_key_mode(3);
			}
			if (ImGui::BeginMenu("F12 binding")) {
				if (ImGui::MenuItem("Mouse", nullptr,
									np2oscfg.F12KEY == 0)) {
					set_f12_key(0);
				}
				if (ImGui::MenuItem("COPY", nullptr,
									np2oscfg.F12KEY == 1)) {
					set_f12_key(1);
				}
				if (ImGui::MenuItem("STOP", nullptr,
									np2oscfg.F12KEY == 2)) {
					set_f12_key(2);
				}
				if (ImGui::MenuItem("Tenkey =", nullptr,
									np2oscfg.F12KEY == 3)) {
					set_f12_key(3);
				}
				if (ImGui::MenuItem("Tenkey ,", nullptr,
									np2oscfg.F12KEY == 4)) {
					set_f12_key(4);
				}
				ImGui::EndMenu();
			}
			menu_item_not_implemented("Mechanical keys (not implemented)");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Sound / 音")) {
			bool enabled = np2cfg.SOUND_SW != 0;
			if (ImGui::MenuItem("Sound on/off", nullptr, enabled)) {
				if (enabled) {
					g_gui.sound_sw_saved = np2cfg.SOUND_SW;
					np2cfg.SOUND_SW = 0;
					soundmng_stop();
				}
				else {
					np2cfg.SOUND_SW = g_gui.sound_sw_saved ?
										g_gui.sound_sw_saved : 4;
					soundmng_play();
				}
				soundrenewal = 1;
				sysmng_update(SYS_UPDATECFG | SYS_UPDATESBOARD);
			}
			int volume = np2cfg.vol_fm;
			if (ImGui::SliderInt("Master volume", &volume, 0, 128)) {
				np2cfg.vol_fm = static_cast<UINT8>(volume);
				np2cfg.vol_ssg = static_cast<UINT8>(volume);
				np2cfg.vol_adpcm = static_cast<UINT8>(volume);
				np2cfg.vol_pcm = static_cast<UINT8>(volume);
				np2cfg.vol_rhythm = static_cast<UINT8>(volume);
				opngen_setvol(np2cfg.vol_fm);
				psggen_setvol(np2cfg.vol_ssg);
				adpcm_setvol(np2cfg.vol_adpcm);
				pcm86gen_setvol(np2cfg.vol_pcm);
				rhythm_setvol(np2cfg.vol_rhythm);
				sysmng_update(SYS_UPDATECFG);
			}
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
		if (ImGui::BeginMenu("Save")) {
			for (int slot = 0; slot < kStateSlots; slot++) {
				char label[32];
				std::snprintf(label, sizeof(label), "Slot %d", slot);
				if (ImGui::MenuItem(label)) {
					char path[MAX_PATH];
					std::snprintf(path, sizeof(path), "np2sdl.S%02d", slot);
					soundmng_stop();
					int ret = statsave_save(path);
					if (ret != STATFLAG_SUCCESS) {
						file_delete(path);
						g_gui.state_status = "State save failed: ";
						g_gui.state_status += path;
					}
					else {
						g_gui.state_status = "State saved: ";
						g_gui.state_status += path;
					}
					soundmng_play();
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Load")) {
			for (int slot = 0; slot < kStateSlots; slot++) {
				char label[32];
				std::snprintf(label, sizeof(label), "Slot %d", slot);
				if (ImGui::MenuItem(label)) {
					char path[MAX_PATH];
					char error[1024];
					std::snprintf(path, sizeof(path), "np2sdl.S%02d", slot);
					error[0] = '\0';
					int ret = statsave_check(path, error, sizeof(error));
					if ((ret & ~STATFLAG_DISKCHG) != 0) {
						g_gui.state_status = "State load failed: ";
						g_gui.state_status += path;
						if (error[0] != '\0') {
							g_gui.state_status += " (";
							g_gui.state_status += error;
							g_gui.state_status += ")";
						}
					}
					else {
						statsave_load(path);
						g_gui.state_status = "State loaded: ";
						g_gui.state_status += path;
						if ((ret & STATFLAG_DISKCHG) != 0) {
							g_gui.state_status += " (disk warning ignored)";
						}
					}
				}
			}
			ImGui::EndMenu();
		}
		if (!g_gui.state_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.state_status.c_str());
		}
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
	draw_fdd_dialog();
}

void gui_render(void) {

	if (!g_gui.initialized) {
		return;
	}
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),
										  g_gui.renderer);
}
