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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#include "compiler.h"
#include "gui/gui.h"
#include "diskdrv.h"
#include "dosio.h"
#include "dropmedia.h"
#include "fddfile.h"
#include "keystat.h"
#include "newdisk.h"
#include "np2.h"
#include "np2info.h"
#include "np2ver.h"
#include "sound.h"
#include "adpcm.h"
#include "beep.h"
#include "pccore.h"
#include "sxsi.h"
#include "fdd_mtr.h"
#include "kbdmap.h"
#include "kbdpaste.h"
#include "mousemng.h"
#include "mouseifva.h"
#include "opngen.h"
#include "pcm86.h"
#include "psggen.h"
#include "rhythm.h"
#include "scrndraw.h"
#include "scrnmng.h"
#include "sdlkbd.h"
#include "sgp.h"
#include "soundmng.h"
#include "soundopts.h"
#include "strres.h"
#include "sysmng.h"
#include "taskmng.h"
#include "ymfmbridge.h"
#include "tms3631.h"

extern "C" {
extern _RHYTHM rhythm;
extern _ADPCM adpcm;
extern const unsigned char vaeg_gui_font_ttf[];
extern const unsigned int vaeg_gui_font_ttf_size;
extern const unsigned char vaeg_splash_bmp[];
extern const unsigned int vaeg_splash_bmp_size;
}

namespace {

constexpr float kGuiFontSize = 16.0f;
constexpr int kStateSlots = 10;
constexpr int kMasterVolumeMax = 128;
constexpr int kSasiImageCount = 6;
constexpr int kCpuPresets[] = {1, 2, 4, 5, 6, 8, 10, 12, 16, 20};
constexpr int kSgpPresets[] = {1, 2, 4, 8, 16};
constexpr int kSoundBufferPresets[] = {40, 100, 200, 500, 1000};
constexpr const char kAboutInfoTemplate[] =
	"CPU: %CPU% %CLOCK%\n"
	"MODEL: %MODEL%\n"
	"SOUND: %EXSND%\n"
	"RHYTHM: %RHYTHM%\n"
	"\n"
	"SCREEN: %DISP%\n"
	"\n"
	"[88VA]\n"
	"ROM TYPE: %ROMTPVA%\n"
	"ROM(Main): %BIOSVA%\n"
	"ROM(VupB): %BIOS91%\n"
	"ROM(Sub): %BIOSSUB%\n"
	"\n"
	"[98x1]\n"
	"MEM: %MEM1%\n"
	"GDC: %GDC%\n"
	"     %GDC2%\n"
	"TEXT: %TEXT%\n"
	"GRPH: %GRPH%\n"
	"\n"
	"BIOS: %BIOS%";
namespace fs = std::filesystem;

struct SasiImageChoice {
	const char *label;
	UINT hdd_type;
};

static const SasiImageChoice kSasiImageChoices[kSasiImageCount] = {
	{"5 MB", 0},
	{"10 MB", 1},
	{"15 MB", 2},
	{"20 MB", 4},
	{"30 MB", 5},
	{"40 MB", 6},
};

static void reset_guest(void);

static std::string state_slot_path(int slot) {

	char	name[32];
	char	path[MAX_PATH];

	std::snprintf(name, sizeof(name), "np2sdl.S%02d", slot);
	file_getstatepath(path, sizeof(path), name);
	return std::string(path);
}

struct BrowserEntry {
	std::string name;
	std::string path;
	bool is_dir = false;
};

struct GuiState {
	bool initialized = false;
	SDL_Renderer *renderer = nullptr;
	SDL_Texture *about_texture = nullptr;
	int about_texture_width = 0;
	int about_texture_height = 0;
	float menu_font_size = kGuiFontSize;
	int fdd_dialog_drive = -1;
	char fdd_path[2][MAX_PATH] = {};
	bool fdd_browser_open = false;
	bool fdd_browser_refresh = false;
	std::string fdd_browser_dir;
	std::vector<BrowserEntry> fdd_entries;
	std::string fdd_status;
	bool new_fdd_open = false;
	bool new_fdd_refresh = false;
	int new_fdd_format = NEWDISK_FDD_MSDOS_2HD;
	int new_fdd_container = NEWDISK_FDD_CONTAINER_D88;
	int new_fdd_drive = 0;
	bool new_fdd_mount_after_create = true;
	char new_fdd_path[MAX_PATH] = {};
	int hdd_dialog_drive = -1;
	char hdd_path[2][MAX_PATH] = {};
	bool hdd_browser_open = false;
	bool hdd_browser_refresh = false;
	std::string hdd_browser_dir;
	std::vector<BrowserEntry> hdd_entries;
	std::string hdd_status;
	bool new_sasi_open = false;
	bool new_sasi_refresh = false;
	int new_sasi_drive = 0;
	int new_sasi_choice = 3;
	bool new_sasi_open_after_create = true;
	char new_sasi_path[MAX_PATH] = {};
	std::string state_status;
	std::string keyboard_status;
	bool keyboard_config_open = false;
	int capture_binding = -1;
	SDL_Scancode capture_swallow = SDL_SCANCODE_UNKNOWN;
	bool configure_open = false;
	bool configure_request = false;
	int pending_cpu_multiplier = PCCORE_STANDARD_MULTIPLE;
	int pending_sgp_mode = SGP_SPEED_MODEL_DEFAULT;
	int pending_sgp_multiplier = 1;
	bool custom_size_open = false;
	bool custom_size_request = false;
	int pending_window_width = 640;
	int pending_window_height = 422;
	bool sound_buffer_open = false;
	bool sound_buffer_request = false;
	int pending_sound_buffer_ms = VAEG_SOUND_BUFFER_DEFAULT_MS;
	bool about_open = false;
	bool about_request = false;
	bool about_more = false;
	char about_info[2048] = {};
};

GuiState g_gui;

static SDL_Texture *load_about_texture(SDL_Renderer *renderer,
										int *width, int *height) {

	SDL_RWops *stream = SDL_RWFromConstMem(vaeg_splash_bmp,
										static_cast<int>(vaeg_splash_bmp_size));
	if (stream == nullptr) {
		return nullptr;
	}
	SDL_Surface *surface = SDL_LoadBMP_RW(stream, 1);
	if (surface == nullptr) {
		return nullptr;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture != nullptr) {
		*width = surface->w;
		*height = surface->h;
		SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
	}
	SDL_FreeSurface(surface);
	return texture;
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

static void menu_item_not_implemented(const char *label) {

	ImGui::BeginDisabled();
	ImGui::MenuItem(label);
	ImGui::EndDisabled();
}

static void open_configure_dialog(void) {

	g_gui.pending_cpu_multiplier = static_cast<int>(np2cfg.multiple);
	g_gui.pending_sgp_mode = static_cast<int>(np2cfg.sgp_speed_mode);
	g_gui.pending_sgp_multiplier = static_cast<int>(np2cfg.sgp_multiplier);
	g_gui.configure_open = true;
	g_gui.configure_request = true;
}

static void draw_multiplier_input(const char *label, int *value,
								  const int *presets, int preset_count) {

	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	ImGui::SameLine(145.0f);
	ImGui::SetNextItemWidth(80.0f);
	ImGui::InputInt("##value", value, 1, 0);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(110.0f);
	if (ImGui::BeginCombo("##presets", "Preset")) {
		for (int i=0; i<preset_count; i++) {
			char item[16];

			std::snprintf(item, sizeof(item), "x%d", presets[i]);
			if (ImGui::Selectable(item, *value == presets[i])) {
				*value = presets[i];
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();
}

static void apply_configure_dialog(void) {

	const bool changed =
		(np2cfg.baseclock != PCBASECLOCK40) ||
		(np2cfg.multiple != static_cast<UINT>(g_gui.pending_cpu_multiplier)) ||
		(np2cfg.sgp_speed_mode != static_cast<UINT8>(g_gui.pending_sgp_mode)) ||
		(np2cfg.sgp_multiplier !=
							static_cast<UINT8>(g_gui.pending_sgp_multiplier));

	if (changed) {
		np2cfg.baseclock = PCBASECLOCK40;
		np2cfg.multiple = static_cast<UINT>(g_gui.pending_cpu_multiplier);
		np2cfg.sgp_speed_mode = static_cast<UINT8>(g_gui.pending_sgp_mode);
		np2cfg.sgp_multiplier =
							static_cast<UINT8>(g_gui.pending_sgp_multiplier);
		sysmng_update(SYS_UPDATECFG | SYS_UPDATECLOCK);
		reset_guest();
	}
	g_gui.configure_open = false;
	ImGui::CloseCurrentPopup();
}

static void draw_configure_dialog(void) {

	if (g_gui.configure_request) {
		ImGui::OpenPopup("Configure##clock-config");
		g_gui.configure_request = false;
	}
	if (!g_gui.configure_open) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing,
												ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(420.0f, 415.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Configure##clock-config",
										&g_gui.configure_open,
										ImGuiWindowFlags_NoResize |
										ImGuiWindowFlags_NoCollapse)) {
		const bool cpu_valid = pccore_cpu_multiple_valid(
								static_cast<UINT>(g_gui.pending_cpu_multiplier));
		const bool sgp_mode_valid = sgp_speed_mode_valid(
								static_cast<UINT>(g_gui.pending_sgp_mode));
		const bool sgp_multiplier_valid =
			(g_gui.pending_sgp_mode != SGP_SPEED_CUSTOM) ||
			sgp_speed_multiplier_valid(
								static_cast<UINT>(g_gui.pending_sgp_multiplier));
		const bool valid = cpu_valid && sgp_mode_valid && sgp_multiplier_valid;

		if (ImGui::BeginChild("cpu-config", ImVec2(0.0f, 145.0f), true,
												ImGuiWindowFlags_NoScrollbar)) {
			ImGui::TextUnformatted("CPU");
			ImGui::Separator();
			ImGui::BeginDisabled();
			if (ImGui::BeginCombo("Base clock", "3.9936 MHz")) {
				ImGui::Selectable("3.9936 MHz", true);
				ImGui::EndCombo();
			}
			ImGui::EndDisabled();
			draw_multiplier_input("Multiplier", &g_gui.pending_cpu_multiplier,
									kCpuPresets, static_cast<int>(std::size(kCpuPresets)));
			ImGui::Text("Effective CPU clock: %.4f MHz",
							3.9936 * static_cast<double>(g_gui.pending_cpu_multiplier));
			ImGui::TextUnformatted("Standard setting: x2 (7.9872 MHz)");
		}
		ImGui::EndChild();

		if (ImGui::BeginChild("sgp-config", ImVec2(0.0f, 165.0f), true,
												ImGuiWindowFlags_NoScrollbar)) {
			static const char *modes[] = {"Model default", "Follow CPU", "Custom"};
			const UINT model = (milstr_cmp(np2cfg.model, str_VA1) == 0) ?
											PCMODEL_VA1 : PCMODEL_VA2;
			const double model_clock_mhz =
						static_cast<double>(sgp_model_clock(model)) / 1000000.0;
			double effective_scale = 1.0;
			ImGui::TextUnformatted("SGP");
			ImGui::Separator();
			ImGui::Combo("Speed", &g_gui.pending_sgp_mode, modes,
										static_cast<int>(std::size(modes)));
			if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				draw_multiplier_input("Custom multiplier",
									 &g_gui.pending_sgp_multiplier, kSgpPresets,
									 static_cast<int>(std::size(kSgpPresets)));
			}
			if (g_gui.pending_sgp_mode == SGP_SPEED_MODEL_DEFAULT) {
				ImGui::TextUnformatted(
							"Effective SGP scale: Model default (x1.0000)");
			}
			else if (g_gui.pending_sgp_mode == SGP_SPEED_FOLLOW_CPU) {
				ImGui::Text("Effective SGP scale: x%.4f relative to Model default",
						static_cast<double>(g_gui.pending_cpu_multiplier) /
											PCCORE_STANDARD_MULTIPLE);
			}
			else if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				ImGui::Text("Effective SGP scale: x%d relative to Model default",
											g_gui.pending_sgp_multiplier);
			}
			if (g_gui.pending_sgp_mode == SGP_SPEED_FOLLOW_CPU) {
				effective_scale =
					static_cast<double>(g_gui.pending_cpu_multiplier) /
											PCCORE_STANDARD_MULTIPLE;
			}
			else if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				effective_scale = g_gui.pending_sgp_multiplier;
			}
			ImGui::Text("Effective SGP clock: %.4f MHz",
									model_clock_mhz * effective_scale);
		}
		ImGui::EndChild();

		if (!cpu_valid) {
			ImGui::TextUnformatted("Multiplier must be between 1 and 32.");
		}
		else if (!sgp_mode_valid) {
			ImGui::TextUnformatted("Select a valid SGP speed mode.");
		}
		else if (!sgp_multiplier_valid) {
			ImGui::TextUnformatted("SGP multiplier must be between 1 and 16.");
		}

		const float button_width = 88.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x -
												(button_width * 2.0f + 8.0f));
		ImGui::BeginDisabled(!valid);
		if (ImGui::Button("OK", ImVec2(button_width, 0.0f))) {
			apply_configure_dialog();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f)) ||
			ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.configure_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static UINT8 scale_master_volume(int volume, int max_value) {

	volume = std::clamp(volume, 0, kMasterVolumeMax);
	return static_cast<UINT8>((volume * max_value + kMasterVolumeMax / 2) /
							  kMasterVolumeMax);
}

static void apply_master_volume(int volume) {

	const UINT8 mixer_volume = scale_master_volume(volume, 128);
	const UINT8 beep_volume = scale_master_volume(volume, 3);
	const UINT8 snd14_volume = scale_master_volume(volume, 15);
	const UINT8 motor_volume = scale_master_volume(volume, 100);

	np2cfg.vol_fm = mixer_volume;
	np2cfg.vol_ssg = mixer_volume;
	np2cfg.vol_adpcm = mixer_volume;
	np2cfg.vol_pcm = mixer_volume;
	np2cfg.vol_rhythm = mixer_volume;
	opngen_setvol(np2cfg.vol_fm);
	psggen_setvol(np2cfg.vol_ssg);
	rhythm_setvol(np2cfg.vol_rhythm);
	rhythm_update(&rhythm);
	adpcm_setvol(np2cfg.vol_adpcm);
	adpcm_update(&adpcm);
	pcm86gen_setvol(np2cfg.vol_pcm);
	pcm86gen_update();

	np2cfg.BEEP_VOL = beep_volume;
	beep_setvol(np2cfg.BEEP_VOL);
	for (BYTE &volume14 : np2cfg.vol14) {
		volume14 = snd14_volume;
	}
	tms3631_setvol(np2cfg.vol14);

	np2cfg.MOTORVOL = motor_volume;
	fddmtrsnd_volume(np2cfg.MOTORVOL);
	sysmng_update(SYS_UPDATECFG);
}

static void select_opn_backend(UINT backend) {

	if (opngen_getbackend() == backend) {
		return;
	}
	opngen_setbackend(backend);
	milstr_ncpy(np2oscfg.opn_backend, opngen_backendname(backend),
										sizeof(np2oscfg.opn_backend));
	soundrenewal = 1;
	sysmng_update(SYS_UPDATEOSCFG);
	reset_guest();
}

static void select_ymfm_fidelity(UINT fidelity) {

	if ((fidelity >= YMFMBRIDGE_FIDELITY_COUNT) ||
		(ymfm_opn_getfidelity() == fidelity)) {
		return;
	}
	soundmng_stop();
	ymfm_opn_setfidelity(fidelity);
	milstr_ncpy(np2oscfg.ymfm_fidelity,
				ymfm_opn_fidelityname(fidelity),
				sizeof(np2oscfg.ymfm_fidelity));
	soundrenewal = 1;
	sysmng_update(SYS_UPDATEOSCFG);
	reset_guest();
}

static void select_sampling_rate(UINT rate) {

	if (!vaeg_sound_rate_valid(rate) || (np2cfg.samplingrate == rate)) {
		return;
	}
	np2cfg.samplingrate = static_cast<UINT16>(rate);
	soundrenewal = 1;
	sysmng_update(SYS_UPDATECFG | SYS_UPDATERATE);
	reset_guest();
}

static void select_sound_buffer(UINT delayms) {

	if (!vaeg_sound_buffer_valid(delayms) || (np2cfg.delayms == delayms)) {
		return;
	}
	np2cfg.delayms = static_cast<UINT16>(delayms);
	soundrenewal = 1;
	sysmng_update(SYS_UPDATECFG | SYS_UPDATESBUF);
	reset_guest();
}

static void select_boot_model(const char *model) {

	const UINT16 old_sound = np2cfg.SOUND_SW;

	np2_select_boot_model(model);
	if (np2cfg.SOUND_SW != old_sound) {
		soundrenewal = 1;
	}
	sysmng_update(SYS_UPDATECFG);
	reset_guest();
}

static void select_sound_hardware(UINT16 sound) {

	if (np2cfg.SOUND_SW == sound) {
		return;
	}
	np2cfg.SOUND_SW = sound;
	soundrenewal = 1;
	sysmng_update(SYS_UPDATECFG | SYS_UPDATESBOARD);
	reset_guest();
}

static int menu_bar_height(void) {

	const ImGuiStyle &style = ImGui::GetStyle();
	return static_cast<int>(
		std::ceil(g_gui.menu_font_size + (style.FramePadding.y * 2.0f)));
}

static std::string home_dir(void) {

	const char *home;

	home = std::getenv("HOME");
	if ((home != nullptr) && (home[0] != '\0')) {
		return home;
	}
	return ".";
}

static bool is_directory(const std::string &path) {

	std::error_code ec;
	return fs::is_directory(fs::u8path(path), ec);
}

static std::string absolute_path(const std::string &path) {

	std::error_code ec;
	fs::path abs = fs::absolute(fs::u8path(path), ec);
	if (ec) {
		return path;
	}
	return abs.u8string();
}

static std::string parent_dir(const std::string &path) {

	std::error_code ec;
	fs::path p = fs::absolute(fs::u8path(path), ec);
	if (ec) {
		p = fs::u8path(path);
	}
	p = p.parent_path();
	if (p.empty()) {
		return home_dir();
	}
	return p.u8string();
}

static void copy_path(char *dst, size_t dst_size, const std::string &src) {

	milstr_ncpy(dst, src.c_str(), static_cast<int>(dst_size));
}

static bool browser_entry_less(const BrowserEntry &a, const BrowserEntry &b) {

	if (a.is_dir != b.is_dir) {
		return a.is_dir > b.is_dir;
	}
	return a.name < b.name;
}

static void refresh_fdd_browser(void) {

	std::error_code ec;

	g_gui.fdd_entries.clear();
	if (!is_directory(g_gui.fdd_browser_dir)) {
		g_gui.fdd_browser_dir = home_dir();
	}
	for (const auto &entry :
		 fs::directory_iterator(fs::u8path(g_gui.fdd_browser_dir), ec)) {
		BrowserEntry item;
		std::error_code st_ec;

		if (ec) {
			break;
		}
		item.is_dir = entry.is_directory(st_ec);
		if ((!item.is_dir) && (!entry.is_regular_file(st_ec))) {
			continue;
		}
		item.name = entry.path().filename().u8string();
		item.path = entry.path().u8string();
		if (item.name.empty() || (item.name[0] == '.')) {
			continue;
		}
		g_gui.fdd_entries.push_back(item);
	}
	std::sort(g_gui.fdd_entries.begin(), g_gui.fdd_entries.end(),
			  browser_entry_less);
	g_gui.fdd_browser_refresh = false;
}

static void refresh_hdd_browser(void) {

	std::error_code ec;

	g_gui.hdd_entries.clear();
	if (!is_directory(g_gui.hdd_browser_dir)) {
		g_gui.hdd_browser_dir = home_dir();
	}
	for (const auto &entry :
		 fs::directory_iterator(fs::u8path(g_gui.hdd_browser_dir), ec)) {
		BrowserEntry item;
		std::error_code st_ec;

		if (ec) {
			break;
		}
		item.is_dir = entry.is_directory(st_ec);
		if ((!item.is_dir) && (!entry.is_regular_file(st_ec))) {
			continue;
		}
		item.name = entry.path().filename().u8string();
		item.path = entry.path().u8string();
		if (item.name.empty() || (item.name[0] == '.')) {
			continue;
		}
		g_gui.hdd_entries.push_back(item);
	}
	std::sort(g_gui.hdd_entries.begin(), g_gui.hdd_entries.end(),
			  browser_entry_less);
	g_gui.hdd_browser_refresh = false;
}

static void persist_fdd_dir(const std::string &dir) {

	if (!is_directory(dir)) {
		return;
	}
	copy_path(np2oscfg.gui_fdd_dir, sizeof(np2oscfg.gui_fdd_dir), dir);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void persist_hdd_dir(const std::string &dir) {

	if (!is_directory(dir)) {
		return;
	}
	copy_path(np2oscfg.gui_hdd_dir, sizeof(np2oscfg.gui_hdd_dir), dir);
	sysmng_update(SYS_UPDATEOSCFG);
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

static bool hdd_file_is_mountable(const char *path, std::string *error) {

	short attr;

	if ((path == nullptr) || (path[0] == '\0')) {
		*error = "SASI HDD image path is empty.";
		return false;
	}
	attr = file_attr(path);
	if (attr == static_cast<short>(-1)) {
		*error = "SASI HDD image not found.";
		return false;
	}
	if ((attr & FILEATTR_DIRECTORY) != 0) {
		*error = "SASI HDD image path is a directory.";
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

static void set_hdd_status(int drive, const char *action, const char *path) {

	g_gui.hdd_status = "SASI-";
	g_gui.hdd_status += static_cast<char>('1' + drive);
	g_gui.hdd_status += ' ';
	g_gui.hdd_status += action;
	if ((path != nullptr) && (path[0] != '\0')) {
		g_gui.hdd_status += ": ";
		g_gui.hdd_status += path;
	}
}

static void remember_fdd_mount(int drive, const char *path) {

	if ((drive < 0) || (drive >= 2)) {
		return;
	}
	if ((path != nullptr) && (path[0] != '\0')) {
		copy_path(np2oscfg.fdd_image[drive],
				  sizeof(np2oscfg.fdd_image[drive]), path);
	}
	else {
		np2oscfg.fdd_image[drive][0] = '\0';
	}
	sysmng_update(SYS_UPDATEOSCFG);
}

static void capture_reset_fdd_mounts(char paths[2][MAX_PATH]) {

	for (int drive = 0; drive < 2; drive++) {
		const char *current;

		paths[drive][0] = '\0';
		if (np2oscfg.fdd_image[drive][0] != '\0') {
			copy_path(paths[drive], MAX_PATH, np2oscfg.fdd_image[drive]);
			continue;
		}
		current = fdd_diskname(static_cast<REG8>(drive));
		if ((current != nullptr) && (current[0] != '\0')) {
			copy_path(paths[drive], MAX_PATH, current);
		}
	}
}

static void restore_reset_fdd_mounts(char paths[2][MAX_PATH]) {

	for (int drive = 0; drive < 2; drive++) {
		if (paths[drive][0] == '\0') {
			continue;
		}
		diskdrv_setfdd(static_cast<REG8>(drive), paths[drive], 0);
		remember_fdd_mount(drive, paths[drive]);
	}
}

static void reset_guest(void) {

	char fdd_paths[2][MAX_PATH];

	capture_reset_fdd_mounts(fdd_paths);
	taskmng_clear_fast_forward();
	pccore_cfgupdate();
	pccore_reset();
	restore_reset_fdd_mounts(fdd_paths);
	sdlkbd_reset_state();
	mousemng_reset();
	scrndraw_redraw();
}

static void open_fdd_dialog(int drive) {

	const char *current;
	std::string start_dir;

	g_gui.fdd_dialog_drive = drive;
	current = fdd_diskname(static_cast<REG8>(drive));
	if ((current != nullptr) && (current[0] != '\0')) {
		milstr_ncpy(g_gui.fdd_path[drive], current,
					sizeof(g_gui.fdd_path[drive]));
		start_dir = parent_dir(current);
	}
	if (start_dir.empty() &&
		(np2oscfg.gui_fdd_dir[0] != '\0') &&
		is_directory(np2oscfg.gui_fdd_dir)) {
		start_dir = np2oscfg.gui_fdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.fdd_browser_dir = absolute_path(start_dir);
	g_gui.fdd_browser_open = true;
	g_gui.fdd_browser_refresh = true;
}

static void open_hdd_dialog(int drive) {

	const char *current;
	std::string start_dir;

	g_gui.hdd_dialog_drive = drive;
	current = np2cfg.sasihdd[drive];
	if ((current != nullptr) && (current[0] != '\0')) {
		milstr_ncpy(g_gui.hdd_path[drive], current,
					sizeof(g_gui.hdd_path[drive]));
		start_dir = parent_dir(current);
	}
	else {
		g_gui.hdd_path[drive][0] = '\0';
	}
	if (start_dir.empty() &&
		(np2oscfg.gui_hdd_dir[0] != '\0') &&
		is_directory(np2oscfg.gui_hdd_dir)) {
		start_dir = np2oscfg.gui_hdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.hdd_browser_dir = absolute_path(start_dir);
	g_gui.hdd_browser_open = true;
	g_gui.hdd_browser_refresh = true;
}

static void open_new_sasi_dialog(int drive) {

	std::string start_dir;
	std::string path;

	g_gui.new_sasi_drive = std::clamp(drive, 0, 1);
	if ((np2oscfg.gui_hdd_dir[0] != '\0') &&
		is_directory(np2oscfg.gui_hdd_dir)) {
		start_dir = np2oscfg.gui_hdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.hdd_browser_dir = absolute_path(start_dir);
	path = join_path(g_gui.hdd_browser_dir, "newdisk.hdi");
	copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path), path);
	g_gui.new_sasi_choice = 3;
	g_gui.new_sasi_open_after_create = true;
	g_gui.new_sasi_open = true;
	g_gui.new_sasi_refresh = true;
	g_gui.hdd_browser_open = false;
}

static const char *new_fdd_default_name(int format, int container) {

	if (container == NEWDISK_FDD_CONTAINER_RAW) {
		if (format == NEWDISK_FDD_MSDOS_2DD) {
			return "newdisk-2dd.img";
		}
		return "newdisk-2hd.img";
	}
	if (format == NEWDISK_FDD_MSDOS_2DD) {
		return "newdisk-2dd.d88";
	}
	return "newdisk-2hd.d88";
}

static void open_new_fdd_dialog(int format) {

	std::string start_dir;
	std::string path;

	g_gui.new_fdd_format = std::clamp(format, 0,
									NEWDISK_FDD_MSDOS_COUNT - 1);
	g_gui.new_fdd_container = NEWDISK_FDD_CONTAINER_D88;
	if ((np2oscfg.gui_fdd_dir[0] != '\0') &&
		is_directory(np2oscfg.gui_fdd_dir)) {
		start_dir = np2oscfg.gui_fdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.fdd_browser_dir = absolute_path(start_dir);
	path = join_path(g_gui.fdd_browser_dir,
					new_fdd_default_name(g_gui.new_fdd_format,
									 g_gui.new_fdd_container));
	copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path), path);
	g_gui.new_fdd_drive = 0;
	g_gui.new_fdd_mount_after_create = true;
	g_gui.new_fdd_open = true;
	g_gui.new_fdd_refresh = true;
	g_gui.fdd_browser_open = false;
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
	remember_fdd_mount(drive, path);
	dropmedia_prune_storage();
	persist_fdd_dir(parent_dir(path));
	set_fdd_status(drive, "mounted", path);
	g_gui.fdd_browser_open = false;
}

static void mount_hdd_from_dialog(void) {

	int drive;
	const char *path;
	std::string error;

	drive = g_gui.hdd_dialog_drive;
	if ((drive < 0) || (drive >= 2)) {
		return;
	}
	path = g_gui.hdd_path[drive];
	if (!hdd_file_is_mountable(path, &error)) {
		g_gui.hdd_status = "SASI-";
		g_gui.hdd_status += static_cast<char>('1' + drive);
		g_gui.hdd_status += " open failed: ";
		g_gui.hdd_status += error;
		return;
	}
	diskdrv_sethdd(static_cast<REG8>(drive), path);
	persist_hdd_dir(parent_dir(path));
	set_hdd_status(drive, "configured; reset to apply", path);
	g_gui.hdd_browser_open = false;
}

static std::string hdi_path(const char *path) {

	std::string result;

	if (path != nullptr) {
		result = path;
	}
	if (result.empty()) {
		return result;
	}
	fs::path p = fs::u8path(result);
	if (p.extension().empty()) {
		p += ".hdi";
		result = p.u8string();
	}
	return result;
}

static std::string fdd_image_path(const char *path, int container) {

	std::string result;
	std::string extension;

	if (path != nullptr) {
		result = path;
	}
	if (result.empty()) {
		return result;
	}
	fs::path p = fs::u8path(result);
	extension = p.extension().u8string();
	std::transform(extension.begin(), extension.end(), extension.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	const char *wanted = (container == NEWDISK_FDD_CONTAINER_RAW) ?
							".img" : ".d88";
	if (extension.empty()) {
		p += wanted;
	}
	else if ((extension == ".d88") || (extension == ".img")) {
		p.replace_extension(wanted);
	}
	else if (extension != wanted) {
		p += wanted;
	}
	return p.u8string();
}

static void create_new_fdd_image(void) {

	int drive;
	int format;
	int container;
	std::string path;
	short attr;

	drive = std::clamp(g_gui.new_fdd_drive, 0, 1);
	format = std::clamp(g_gui.new_fdd_format, 0,
									NEWDISK_FDD_MSDOS_COUNT - 1);
	container = std::clamp(g_gui.new_fdd_container, 0,
									NEWDISK_FDD_CONTAINER_COUNT - 1);
	path = fdd_image_path(g_gui.new_fdd_path, container);
	if (path.empty()) {
		g_gui.fdd_status = "New FDD image failed: path is empty.";
		return;
	}
	path = absolute_path(path);
	attr = file_attr(path.c_str());
	if (attr != static_cast<short>(-1)) {
		g_gui.fdd_status = "New FDD image failed: file already exists.";
		return;
	}
	if (!is_directory(parent_dir(path))) {
		g_gui.fdd_status =
				"New FDD image failed: parent directory not found.";
		return;
	}
	if (newdisk_fdd_msdos_ex(path.c_str(), static_cast<UINT>(format),
								 static_cast<UINT>(container)) != SUCCESS) {
		g_gui.fdd_status = "New FDD image failed: create failed.";
		return;
	}
	copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path), path);
	persist_fdd_dir(parent_dir(path));
	if (g_gui.new_fdd_mount_after_create) {
		diskdrv_setfdd(static_cast<REG8>(drive), path.c_str(), 0);
		remember_fdd_mount(drive, path.c_str());
		dropmedia_prune_storage();
		set_fdd_status(drive, "created and mounted", path.c_str());
	}
	else {
		g_gui.fdd_status = "New FDD image created: ";
		g_gui.fdd_status += path;
	}
	g_gui.new_fdd_open = false;
}

static void create_new_sasi_image(void) {

	int drive;
	int choice;
	std::string path;
	short attr;

	drive = std::clamp(g_gui.new_sasi_drive, 0, 1);
	choice = std::clamp(g_gui.new_sasi_choice, 0, kSasiImageCount - 1);
	path = hdi_path(g_gui.new_sasi_path);
	if (path.empty()) {
		g_gui.hdd_status = "New SASI image failed: path is empty.";
		return;
	}
	attr = file_attr(path.c_str());
	if (attr != static_cast<short>(-1)) {
		g_gui.hdd_status = "New SASI image failed: file already exists.";
		return;
	}
	if (!is_directory(parent_dir(path))) {
		g_gui.hdd_status = "New SASI image failed: parent directory not found.";
		return;
	}
	newdisk_hdi(path.c_str(), kSasiImageChoices[choice].hdd_type);
	attr = file_attr(path.c_str());
	if ((attr == static_cast<short>(-1)) ||
		((attr & FILEATTR_DIRECTORY) != 0)) {
		g_gui.hdd_status = "New SASI image failed: create failed.";
		return;
	}
	copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path), path);
	persist_hdd_dir(parent_dir(path));
	if (g_gui.new_sasi_open_after_create) {
		diskdrv_sethdd(static_cast<REG8>(drive), path.c_str());
		set_hdd_status(drive, "created and configured; reset to apply",
					   path.c_str());
	}
	else {
		g_gui.hdd_status = "New SASI image created: ";
		g_gui.hdd_status += path;
	}
	g_gui.new_sasi_open = false;
}

static void eject_fdd(int drive) {

	diskdrv_setfdd(static_cast<REG8>(drive), nullptr, 0);
	remember_fdd_mount(drive, nullptr);
	dropmedia_prune_storage();
	set_fdd_status(drive, "ejected", nullptr);
}

static void remove_hdd(int drive) {

	diskdrv_sethdd(static_cast<REG8>(drive), nullptr);
	sxsi_open();
	if (!sxsi_issasi()) {
		pccore.hddif &= ~PCHDD_SASI;
	}
	set_hdd_status(drive, "removed; reset to apply", nullptr);
}

static void draw_fdd_browser(void) {

	int drive;

	if (!g_gui.fdd_browser_open) {
		return;
	}
	drive = g_gui.fdd_dialog_drive;
	if ((drive < 0) || (drive >= 2)) {
		g_gui.fdd_browser_open = false;
		return;
	}
	if (g_gui.fdd_browser_refresh) {
		refresh_fdd_browser();
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f),
							 ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Mount FDD image", &g_gui.fdd_browser_open)) {
		ImGui::Text("FDD%d directory", drive + 1);
		ImGui::TextWrapped("%s", g_gui.fdd_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.fdd_browser_dir = home_dir();
			g_gui.fdd_browser_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.fdd_browser_dir = parent_dir(g_gui.fdd_browser_dir);
			g_gui.fdd_browser_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("fdd-browser-list", ImVec2(0, 230.0f),
							  ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.fdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.fdd_browser_dir = entry.path;
						g_gui.fdd_browser_refresh = true;
					}
					else {
						copy_path(g_gui.fdd_path[drive],
								  sizeof(g_gui.fdd_path[drive]),
								  entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##fdd-path", g_gui.fdd_path[drive],
						 sizeof(g_gui.fdd_path[drive]));
		if (ImGui::Button("Mount")) {
			mount_fdd_from_dialog();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.fdd_browser_open = false;
		}
		if (!g_gui.fdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.fdd_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_hdd_browser(void) {

	int drive;

	if (!g_gui.hdd_browser_open) {
		return;
	}
	drive = g_gui.hdd_dialog_drive;
	if ((drive < 0) || (drive >= 2)) {
		g_gui.hdd_browser_open = false;
		return;
	}
	if (g_gui.hdd_browser_refresh) {
		refresh_hdd_browser();
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f),
							 ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Open SASI HDD image", &g_gui.hdd_browser_open)) {
		ImGui::Text("SASI-%d directory", drive + 1);
		ImGui::TextWrapped("%s", g_gui.hdd_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.hdd_browser_dir = home_dir();
			g_gui.hdd_browser_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.hdd_browser_dir = parent_dir(g_gui.hdd_browser_dir);
			g_gui.hdd_browser_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("hdd-browser-list", ImVec2(0, 230.0f),
							  ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.hdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.hdd_browser_dir = entry.path;
						g_gui.hdd_browser_refresh = true;
					}
					else {
						copy_path(g_gui.hdd_path[drive],
								  sizeof(g_gui.hdd_path[drive]),
								  entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##hdd-path", g_gui.hdd_path[drive],
						 sizeof(g_gui.hdd_path[drive]));
		if (ImGui::Button("Open")) {
			mount_hdd_from_dialog();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.hdd_browser_open = false;
		}
		if (!g_gui.hdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.hdd_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_new_sasi_dialog(void) {

	if (!g_gui.new_sasi_open) {
		return;
	}
	if (g_gui.new_sasi_refresh) {
		refresh_hdd_browser();
		g_gui.new_sasi_refresh = false;
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 500.0f),
							 ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Create SASI HDD image", &g_gui.new_sasi_open)) {
		ImGui::Text("Target directory");
		ImGui::TextWrapped("%s", g_gui.hdd_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.hdd_browser_dir = home_dir();
			g_gui.new_sasi_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.hdd_browser_dir = parent_dir(g_gui.hdd_browser_dir);
			g_gui.new_sasi_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("new-sasi-browser-list",
							  ImVec2(0, 170.0f), ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.hdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.hdd_browser_dir = entry.path;
						copy_path(g_gui.new_sasi_path,
								  sizeof(g_gui.new_sasi_path),
								  join_path(entry.path, "newdisk.hdi"));
						g_gui.new_sasi_refresh = true;
					}
					else {
						copy_path(g_gui.new_sasi_path,
								  sizeof(g_gui.new_sasi_path),
								  entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##new-sasi-path", g_gui.new_sasi_path,
						 sizeof(g_gui.new_sasi_path));
		ImGui::Text("Image size");
		for (int i = 0; i < kSasiImageCount; i++) {
			if (i > 0) {
				ImGui::SameLine();
			}
			ImGui::RadioButton(kSasiImageChoices[i].label,
							   &g_gui.new_sasi_choice, i);
		}
		ImGui::Text("Configure after create");
		ImGui::RadioButton("SASI-1", &g_gui.new_sasi_drive, 0);
		ImGui::SameLine();
		ImGui::RadioButton("SASI-2", &g_gui.new_sasi_drive, 1);
		ImGui::Checkbox("Set HDD file after create",
						&g_gui.new_sasi_open_after_create);
		if (ImGui::Button("Create")) {
			create_new_sasi_image();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.new_sasi_open = false;
		}
		if (!g_gui.hdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.hdd_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_new_fdd_dialog(void) {

	if (!g_gui.new_fdd_open) {
		return;
	}
	if (g_gui.new_fdd_refresh) {
		refresh_fdd_browser();
		g_gui.new_fdd_refresh = false;
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 500.0f),
							 ImGuiCond_FirstUseEver);
	if (ImGui::Begin("New FDD image", &g_gui.new_fdd_open)) {
		ImGui::Text("Target directory");
		ImGui::TextWrapped("%s", g_gui.fdd_browser_dir.c_str());
		if (ImGui::Button("Home##new-fdd")) {
			g_gui.fdd_browser_dir = home_dir();
			copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
				join_path(g_gui.fdd_browser_dir,
						new_fdd_default_name(g_gui.new_fdd_format,
										 g_gui.new_fdd_container)));
			g_gui.new_fdd_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up##new-fdd")) {
			g_gui.fdd_browser_dir = parent_dir(g_gui.fdd_browser_dir);
			copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
				join_path(g_gui.fdd_browser_dir,
						new_fdd_default_name(g_gui.new_fdd_format,
										 g_gui.new_fdd_container)));
			g_gui.new_fdd_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("new-fdd-browser-list",
							  ImVec2(0, 170.0f), ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.fdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.fdd_browser_dir = entry.path;
						copy_path(g_gui.new_fdd_path,
								  sizeof(g_gui.new_fdd_path),
							join_path(entry.path, new_fdd_default_name(
											g_gui.new_fdd_format,
											g_gui.new_fdd_container)));
						g_gui.new_fdd_refresh = true;
					}
					else {
						copy_path(g_gui.new_fdd_path,
								  sizeof(g_gui.new_fdd_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##new-fdd-path", g_gui.new_fdd_path,
						 sizeof(g_gui.new_fdd_path));
		ImGui::Text("Image format");
		if (ImGui::RadioButton("D88", &g_gui.new_fdd_container,
							NEWDISK_FDD_CONTAINER_D88)) {
			copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
				fdd_image_path(g_gui.new_fdd_path,
								NEWDISK_FDD_CONTAINER_D88));
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("IMG (raw)", &g_gui.new_fdd_container,
							NEWDISK_FDD_CONTAINER_RAW)) {
			copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
				fdd_image_path(g_gui.new_fdd_path,
								NEWDISK_FDD_CONTAINER_RAW));
		}
		ImGui::Text("Disk format");
		ImGui::RadioButton("2HD (1.2 MB)", &g_gui.new_fdd_format,
						   NEWDISK_FDD_MSDOS_2HD);
		ImGui::SameLine();
		ImGui::RadioButton("2DD (640 KB)", &g_gui.new_fdd_format,
						   NEWDISK_FDD_MSDOS_2DD);
		ImGui::Text("Mount after create");
		ImGui::RadioButton("FDD1##new-fdd", &g_gui.new_fdd_drive, 0);
		ImGui::SameLine();
		ImGui::RadioButton("FDD2##new-fdd", &g_gui.new_fdd_drive, 1);
		ImGui::Checkbox("Mount image after create",
						&g_gui.new_fdd_mount_after_create);
		if (ImGui::Button("Create##new-fdd")) {
			create_new_fdd_image();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel##new-fdd")) {
			g_gui.new_fdd_open = false;
		}
		if (!g_gui.fdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.fdd_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_emulate_menu(void) {

	if (ImGui::BeginMenu("Emulate / エミュレート")) {
		if (ImGui::MenuItem("Reset / リセット")) {
			reset_guest();
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("Boot model / 起動機種")) {
			if (ImGui::MenuItem("VA", nullptr,
								milstr_cmp(np2cfg.model, str_VA1) == 0)) {
				select_boot_model(str_VA1);
			}
			if (ImGui::MenuItem("VA2/VA3", nullptr,
								milstr_cmp(np2cfg.model, str_VA2) == 0)) {
				select_boot_model(str_VA2);
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
			if (ImGui::MenuItem("Configure...")) {
				open_configure_dialog();
			}
		menu_item_not_implemented("Font... (not implemented)");
		ImGui::Separator();
		if (ImGui::MenuItem("Exit / 終了")) {
			taskmng_exit();
		}
		ImGui::EndMenu();
	}
}

static void draw_edit_menu(void) {

	if (ImGui::BeginMenu("Edit / 編集")) {
#if defined(__APPLE__)
		const char *shortcut = "Cmd+V";
#else
		const char *shortcut = "Ctrl+V";
#endif
		if (ImGui::MenuItem("Paste / 貼り付け", shortcut)) {
			kbdpaste_start_clipboard();
		}
		if (ImGui::MenuItem("Cancel Paste", nullptr, false,
						kbdpaste_active() ? true : false)) {
			kbdpaste_cancel();
		}
		if (kbdpaste_status()[0] != '\0') {
			ImGui::Separator();
			ImGui::TextWrapped("%s", kbdpaste_status());
		}
		ImGui::EndMenu();
	}
}

static void draw_fdd_mount_state(int drive) {

	const char *path;
	bool inserting;

	path = fdd_diskname(static_cast<REG8>(drive));
	inserting = false;
	if ((path == nullptr) || (path[0] == '\0')) {
		path = diskdrv_fname[drive];
		inserting = path[0] != '\0';
	}
	if ((path == nullptr) || (path[0] == '\0')) {
		ImGui::TextDisabled("FDD%d: Empty", drive + 1);
		return;
	}
	const std::string name = fs::u8path(path).filename().u8string();
	if (inserting) {
		ImGui::Text("FDD%d: %s (inserting)", drive + 1,
						name.c_str());
	}
	else {
		ImGui::Text("FDD%d: %s", drive + 1, name.c_str());
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", path);
	}
}

static void draw_hdd_mount_state(int drive) {

	const char *path;

	path = np2cfg.sasihdd[drive];
	if ((path == nullptr) || (path[0] == '\0')) {
		ImGui::TextDisabled("SASI-%d: Empty", drive + 1);
		return;
	}
	const std::string name = fs::u8path(path).filename().u8string();
	ImGui::Text("SASI-%d: %s", drive + 1, name.c_str());
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", path);
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
		draw_fdd_mount_state(0);
		ImGui::Separator();
		if (ImGui::MenuItem("FDD2 Open...")) {
			open_fdd_dialog(1);
		}
		if (ImGui::MenuItem("FDD2 Eject")) {
			eject_fdd(1);
		}
		draw_fdd_mount_state(1);
		ImGui::Separator();
		menu_item_not_implemented("FDD3 Open... (not implemented)");
		menu_item_not_implemented("FDD3 Eject (not implemented)");
		menu_item_not_implemented("FDD4 Open... (not implemented)");
		menu_item_not_implemented("FDD4 Eject (not implemented)");
		if (dropmedia_status()[0] != '\0') {
			ImGui::Separator();
			ImGui::TextWrapped("%s", dropmedia_status());
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("New FDD image")) {
			if (ImGui::MenuItem("2HD (1.2 MB)...")) {
				open_new_fdd_dialog(NEWDISK_FDD_MSDOS_2HD);
			}
			if (ImGui::MenuItem("2DD (640 KB)...")) {
				open_new_fdd_dialog(NEWDISK_FDD_MSDOS_2DD);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
}

static void draw_harddisk_menu(void) {

	if (ImGui::BeginMenu("HardDisk")) {
		if (ImGui::MenuItem("SASI-1 Open...")) {
			open_hdd_dialog(0);
		}
		if (ImGui::MenuItem("SASI-1 Remove")) {
			remove_hdd(0);
		}
		draw_hdd_mount_state(0);
		ImGui::Separator();
		if (ImGui::MenuItem("SASI-2 Open...")) {
			open_hdd_dialog(1);
		}
		if (ImGui::MenuItem("SASI-2 Remove")) {
			remove_hdd(1);
		}
		draw_hdd_mount_state(1);
		ImGui::Separator();
		if (ImGui::MenuItem("New SASI image...")) {
			open_new_sasi_dialog(0);
		}
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

static void set_display_scaling(int scaling) {

	np2oscfg.gui_scaling = static_cast<BYTE>(scaling);
	scrnmng_set_scaling(np2oscfg.gui_scaling);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_display_effect(int effect) {

	np2oscfg.gui_effect = static_cast<BYTE>(effect);
	scrnmng_set_effect(np2oscfg.gui_effect);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_display_mode(int mode) {

	UINT width = np2oscfg.fscrn_cx;
	UINT height = np2oscfg.fscrn_cy;
	UINT refresh = np2oscfg.gui_fullscreen_refresh;
	UINT8 fscrnmod = np2oscfg.fscrnmod;

	if (scrnmng_get_display_mode() == mode) {
		return;
	}
	if (mode == VAEG_DISPLAY_EXCLUSIVE) {
		width = 0;
		height = 0;
		refresh = 0;
		fscrnmod = static_cast<UINT8>((fscrnmod & 3) | 4);
	}
	if (scrnmng_set_display_mode(mode, np2oscfg.gui_monitor,
							width, height, refresh, fscrnmod) == SUCCESS) {
		np2oscfg.gui_display_mode = static_cast<BYTE>(mode);
		if (mode == VAEG_DISPLAY_EXCLUSIVE) {
			np2oscfg.fscrn_cx = 0;
			np2oscfg.fscrn_cy = 0;
			np2oscfg.gui_fullscreen_refresh = 0;
			np2oscfg.fscrnmod = fscrnmod;
		}
	}
	else {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
				"Display mode change failed: %s", SDL_GetError());
		np2oscfg.gui_display_mode = VAEG_DISPLAY_WINDOWED;
	}
	sysmng_update(SYS_UPDATEOSCFG);
}

static void open_custom_size_dialog(void) {

	g_gui.pending_window_width = np2oscfg.gui_window_width;
	g_gui.pending_window_height = np2oscfg.gui_window_height;
	g_gui.custom_size_request = true;
}

static void draw_custom_size_dialog(void) {

	if (g_gui.custom_size_request) {
		g_gui.custom_size_request = false;
		g_gui.custom_size_open = true;
		ImGui::OpenPopup("Custom window size##display");
	}
	if (!g_gui.custom_size_open) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(340.0f, 150.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Custom window size##display",
									&g_gui.custom_size_open,
									ImGuiWindowFlags_NoResize)) {
		ImGui::InputInt("Logical width", &g_gui.pending_window_width);
		ImGui::InputInt("Logical height", &g_gui.pending_window_height);
		const bool valid = (g_gui.pending_window_width >= 320) &&
			(g_gui.pending_window_width <= 7680) &&
			(g_gui.pending_window_height >= 240) &&
			(g_gui.pending_window_height <= 4320);
		if (!valid) {
			ImGui::TextUnformatted("Size must be between 320x240 and 7680x4320.");
		}
		ImGui::BeginDisabled(!valid ||
			(scrnmng_get_display_mode() != VAEG_DISPLAY_WINDOWED));
		if (ImGui::Button("Apply")) {
			SDL_SetWindowSize(static_cast<SDL_Window *>(scrnmng_get_window()),
					g_gui.pending_window_width, g_gui.pending_window_height);
			np2oscfg.gui_window_width =
					static_cast<UINT16>(g_gui.pending_window_width);
			np2oscfg.gui_window_height =
					static_cast<UINT16>(g_gui.pending_window_height);
			np2oscfg.gui_scale = 0;
			sysmng_update(SYS_UPDATEOSCFG);
			g_gui.custom_size_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.custom_size_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void open_sound_buffer_dialog(void) {

	g_gui.pending_sound_buffer_ms = np2cfg.delayms;
	g_gui.sound_buffer_request = true;
}

static void draw_sound_buffer_dialog(void) {

	if (g_gui.sound_buffer_request) {
		g_gui.sound_buffer_request = false;
		g_gui.sound_buffer_open = true;
		ImGui::OpenPopup("Custom sound buffer##sound");
	}
	if (!g_gui.sound_buffer_open) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(360.0f, 145.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Custom sound buffer##sound",
									&g_gui.sound_buffer_open,
									ImGuiWindowFlags_NoResize)) {
		ImGui::InputInt("Buffer length (ms)",
								&g_gui.pending_sound_buffer_ms);
		const bool valid = vaeg_sound_buffer_valid(
					static_cast<UINT>(g_gui.pending_sound_buffer_ms)) != FALSE;
		if (!valid) {
			ImGui::TextUnformatted("Buffer length must be between 40 and 1000 ms.");
		}
		ImGui::BeginDisabled(!valid);
		if (ImGui::Button("Apply")) {
			const UINT delayms =
					static_cast<UINT>(g_gui.pending_sound_buffer_ms);

			g_gui.sound_buffer_open = false;
			ImGui::CloseCurrentPopup();
			select_sound_buffer(delayms);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.sound_buffer_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
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

static void set_mouse_capture(bool capture) {

	np2oscfg.MOUSE_SW = capture ? 1 : 0;
	mousemng_setcapture(capture ? TRUE : FALSE);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_mouse_device(UINT8 device) {

	if ((device != MOUSEIFVA_JOYPAD) && (device != MOUSEIFVA_MOUSE)) {
		return;
	}
	mouseifvacfg.device = device;
	sysmng_update(SYS_UPDATECFG);
}

static void set_mouse_rapid(bool rapid) {

	np2cfg.MOUSERAPID = rapid ? 1 : 0;
	sysmng_update(SYS_UPDATECFG);
}

static void set_keyboard_layout(const char *layout) {

	kbdmap_set_layout(layout);
	g_gui.keyboard_status = "Keyboard layout: ";
	g_gui.keyboard_status += kbdmap_layout_name();
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_kana_input(const char *mode) {

	kbdmap_set_kana_input(mode);
	g_gui.keyboard_status = "Kana input: ";
	g_gui.keyboard_status += kbdmap_kana_input_name();
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_tenkey_overlay(bool enabled) {

	kbdmap_set_tenkey_overlay(enabled ? TRUE : FALSE);
	g_gui.keyboard_status = enabled ?
		"Tenkey overlay enabled." : "Tenkey overlay disabled.";
	sysmng_update(SYS_UPDATEOSCFG);
}

static const char *binding_name(SDL_Scancode scancode) {

	const char *name;

	if (scancode == SDL_SCANCODE_UNKNOWN) {
		return "(unassigned)";
	}
	name = SDL_GetScancodeName(scancode);
	if ((name == nullptr) || (name[0] == '\0')) {
		return "(unknown)";
	}
	return name;
}

static void reset_keyboard_to_jis(void) {

	kbdmap_reset_to_jis();
	g_gui.keyboard_status = "Keyboard map reset to JIS.";
	sysmng_update(SYS_UPDATEOSCFG);
}

static void reset_keyboard_to_us(void) {

	kbdmap_reset_to_us();
	g_gui.keyboard_status = "Keyboard map reset to US.";
	sysmng_update(SYS_UPDATEOSCFG);
}

static void begin_key_capture(int index) {

	g_gui.capture_binding = index;
	g_gui.capture_swallow = SDL_SCANCODE_UNKNOWN;
	const KBDMAP_ENTRY *entry = kbdmap_entry(index);
	g_gui.keyboard_status = "Capture: ";
	g_gui.keyboard_status += (entry != nullptr) ? entry->label : "(unknown)";
}

static void clear_key_binding(int index) {

	if (kbdmap_set_binding(index, SDL_SCANCODE_UNKNOWN) == SUCCESS) {
		g_gui.keyboard_status = "Binding cleared.";
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

static void draw_keyboard_config(void) {

	if (!g_gui.keyboard_config_open) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Keyboard mapping", &g_gui.keyboard_config_open)) {
		if (ImGui::Button("Reset to JIS defaults")) {
			reset_keyboard_to_jis();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset to US defaults")) {
			reset_keyboard_to_us();
		}
		if (!g_gui.keyboard_status.empty()) {
			ImGui::TextWrapped("%s", g_gui.keyboard_status.c_str());
		}
		ImGui::Separator();
		if (ImGui::BeginTable("keyboard-map-table", 6,
							  ImGuiTableFlags_Borders |
							  ImGuiTableFlags_RowBg |
							  ImGuiTableFlags_ScrollY,
							  ImVec2(0.0f, 390.0f))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Role");
			ImGui::TableSetupColumn("Guest");
			ImGui::TableSetupColumn("Binding");
			ImGui::TableSetupColumn("Status");
			ImGui::TableSetupColumn("Capture");
			ImGui::TableSetupColumn("Clear");
			ImGui::TableHeadersRow();
			for (int i = 0; i < kbdmap_entry_count(); i++) {
				const KBDMAP_ENTRY *entry = kbdmap_entry(i);
				char guest[16];
				if (entry == nullptr) {
					continue;
				}
				std::snprintf(guest, sizeof(guest), "0x%02x",
							  static_cast<unsigned int>(entry->guest_code));
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(entry->label);
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(guest);
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(binding_name(kbdmap_binding(i)));
				ImGui::TableSetColumnIndex(3);
				ImGui::TextUnformatted(
					kbdmap_status_name(kbdmap_binding_status(i)));
				ImGui::TableSetColumnIndex(4);
				ImGui::PushID(i);
				if (g_gui.capture_binding == i) {
					ImGui::Button("...");
				}
				else if (ImGui::Button("Set")) {
					begin_key_capture(i);
				}
				ImGui::TableSetColumnIndex(5);
				if (ImGui::Button("Clear")) {
					clear_key_binding(i);
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

static void draw_screen_menu(void) {

	if (ImGui::BeginMenu("Screen / 画面")) {
		if (ImGui::BeginMenu("Effect")) {
			static const char *labels[] = {
				"Unfiltered", "Linear", "Scanline", "CRT Lite"
			};
			for (int value=0; value<VAEG_EFFECT_COUNT; value++) {
				if (ImGui::MenuItem(labels[value], nullptr,
								np2oscfg.gui_effect == value)) {
					set_display_effect(value);
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Scaling")) {
			static const char *labels[] = {
				"Native", "Fit", "Fit 8-dot", "Integer", "Stretch"
			};
			for (int value=0; value<VAEG_SCALING_COUNT; value++) {
				if (ImGui::MenuItem(labels[value], nullptr,
								np2oscfg.gui_scaling == value)) {
					set_display_scaling(value);
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window size")) {
			if (ImGui::MenuItem("Native", nullptr, np2oscfg.gui_scale == 1)) {
				set_display_scale(1);
			}
			if (ImGui::MenuItem("x2", nullptr, np2oscfg.gui_scale == 2)) {
				set_display_scale(2);
			}
			if (ImGui::MenuItem("x3", nullptr, np2oscfg.gui_scale == 3)) {
				set_display_scale(3);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Custom...", nullptr, false,
					scrnmng_get_display_mode() == VAEG_DISPLAY_WINDOWED)) {
				open_custom_size_dialog();
			}
			ImGui::EndMenu();
		}
		bool aspect = np2oscfg.gui_aspect != 0;
		if (ImGui::MenuItem("Aspect correction", nullptr, aspect)) {
			set_display_aspect(!aspect);
		}
		if (ImGui::MenuItem("Windowed", nullptr,
				scrnmng_get_display_mode() == VAEG_DISPLAY_WINDOWED)) {
			set_display_mode(VAEG_DISPLAY_WINDOWED);
		}
		if (ImGui::MenuItem("Exclusive fullscreen", nullptr,
				scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE)) {
			set_display_mode(VAEG_DISPLAY_EXCLUSIVE);
		}
		ImGui::Separator();
		bool nowait = np2oscfg.NOWAIT != 0;
		if (ImGui::MenuItem("No Wait", nullptr, nowait)) {
			np2oscfg.NOWAIT = nowait ? 0 : 1;
			sysmng_update(SYS_UPDATEOSCFG);
		}
		if (ImGui::BeginMenu("Frame skip")) {
			static const char *labels[] = {
				"Auto", "Full frame", "1/2 frame", "1/3 frame", "1/4 frame"
			};
			for (int value=0; value<static_cast<int>(std::size(labels)); value++) {
				if (ImGui::MenuItem(labels[value], nullptr,
								np2oscfg.DRAW_SKIP == value)) {
					np2oscfg.DRAW_SKIP = static_cast<BYTE>(value);
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
			ImGui::EndMenu();
		}
		bool framedisp = (np2oscfg.DISPCLK & 2) != 0;
		if (ImGui::MenuItem("Frame display", nullptr, framedisp)) {
			np2oscfg.DISPCLK ^= 2;
			scrnmng_set_framedisp((np2oscfg.DISPCLK & 2) ? TRUE : FALSE);
			sysmng_update(SYS_UPDATEOSCFG);
		}
		ImGui::Separator();
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
			if (ImGui::BeginMenu("Host layout")) {
				if (ImGui::MenuItem("JIS physical", nullptr,
									std::string(kbdmap_layout_name()) == "jis")) {
					set_keyboard_layout("jis");
				}
				if (ImGui::MenuItem("US keytop", nullptr,
									std::string(kbdmap_layout_name()) == "us")) {
					set_keyboard_layout("us");
				}
				if (ImGui::MenuItem("Custom", nullptr,
									std::string(kbdmap_layout_name()) == "custom")) {
					set_keyboard_layout("custom");
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Kana input")) {
				if (ImGui::MenuItem("JIS Kana", nullptr,
									std::string(kbdmap_kana_input_name()) == "jis-kana")) {
					set_kana_input("jis-kana");
				}
				if (ImGui::MenuItem("Roman Kana", nullptr,
									std::string(kbdmap_kana_input_name()) == "roman")) {
					set_kana_input("roman");
				}
				ImGui::EndMenu();
			}
			{
				bool tenkey_overlay = kbdmap_tenkey_overlay_enabled() ? true : false;
				if (ImGui::MenuItem("Tenkey overlay (YUI/HJK/NM,.)",
									nullptr, tenkey_overlay)) {
					set_tenkey_overlay(!tenkey_overlay);
				}
			}
			if (ImGui::MenuItem("Key bindings...")) {
				g_gui.keyboard_config_open = true;
			}
			menu_item_not_implemented("Mechanical keys (not implemented)");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Sound / 音")) {
			if (ImGui::BeginMenu("FM sound OPN/OPNA")) {
				const bool va1 = milstr_cmp(np2cfg.model, str_VA1) == 0;
				ImGui::BeginDisabled(!va1);
				if (ImGui::MenuItem("OPN (VA)", nullptr,
								 np2cfg.SOUND_SW == FMBOARD_VA_OPN)) {
					select_sound_hardware(FMBOARD_VA_OPN);
				}
				ImGui::EndDisabled();
				if (ImGui::MenuItem(
						"OPNA (VA2/VA3, VA + Sound Board II)",
						nullptr, np2cfg.SOUND_SW == FMBOARD_VA_OPNA)) {
					select_sound_hardware(FMBOARD_VA_OPNA);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("FM sound backend")) {
				const UINT backend = opngen_getbackend();
				if (ImGui::MenuItem("NP2", nullptr,
								backend == OPN_BACKEND_NP2)) {
					select_opn_backend(OPN_BACKEND_NP2);
				}
				if (ImGui::MenuItem("ymfm", nullptr,
								backend == OPN_BACKEND_YMFM)) {
					select_opn_backend(OPN_BACKEND_YMFM);
				}
				ImGui::EndMenu();
			}
			ImGui::BeginDisabled(opngen_getbackend() != OPN_BACKEND_YMFM);
			if (ImGui::BeginMenu("ymfm fidelity")) {
				const UINT fidelity = ymfm_opn_getfidelity();
				if (ImGui::MenuItem("Minimum (~166 kHz native, default)", nullptr,
							fidelity == YMFMBRIDGE_FIDELITY_MINIMUM)) {
					select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MINIMUM);
				}
				if (ImGui::MenuItem("Medium (~333 kHz native)", nullptr,
							fidelity == YMFMBRIDGE_FIDELITY_MEDIUM)) {
					select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MEDIUM);
				}
				if (ImGui::MenuItem("Maximum (~998 kHz native, high CPU)", nullptr,
							fidelity == YMFMBRIDGE_FIDELITY_MAXIMUM)) {
					select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MAXIMUM);
				}
				ImGui::EndMenu();
			}
			ImGui::EndDisabled();
			if (ImGui::BeginMenu("Sampling rate")) {
				if (ImGui::MenuItem("11.025 kHz", nullptr,
								np2cfg.samplingrate == 11025)) {
					select_sampling_rate(11025);
				}
				if (ImGui::MenuItem("22.05 kHz", nullptr,
								np2cfg.samplingrate == 22050)) {
					select_sampling_rate(22050);
				}
				if (ImGui::MenuItem("44.1 kHz (Recommended)", nullptr,
								np2cfg.samplingrate == 44100)) {
					select_sampling_rate(44100);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Sound buffer")) {
				bool preset_selected = false;
				for (const int preset : kSoundBufferPresets) {
					char label[32];
					const bool selected = np2cfg.delayms == preset;

					std::snprintf(label, sizeof(label), "%d ms", preset);
					preset_selected = preset_selected || selected;
					if (ImGui::MenuItem(label, nullptr, selected)) {
						select_sound_buffer(static_cast<UINT>(preset));
					}
				}
				char custom_label[48];
				std::snprintf(custom_label, sizeof(custom_label),
							"Custom... (%u ms)", np2cfg.delayms);
				if (ImGui::MenuItem(custom_label, nullptr, !preset_selected)) {
					open_sound_buffer_dialog();
				}
				ImGui::EndMenu();
			}
			bool enabled = soundmng_isenabled() ? true : false;
			if (ImGui::MenuItem("Sound on/off", nullptr, enabled)) {
				np2oscfg.sound_enabled = enabled ? 0 : 1;
				soundmng_setenabled(np2oscfg.sound_enabled ? TRUE : FALSE);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			bool motor = np2cfg.MOTOR != 0;
			if (ImGui::MenuItem("Seek/motor sound", nullptr, motor)) {
				np2cfg.MOTOR = motor ? 0 : 1;
				if (np2cfg.MOTOR == 0) {
					fddmtrsnd_stop();
				}
				sysmng_update(SYS_UPDATECFG);
			}
			int volume = np2cfg.vol_fm;
			if (ImGui::SliderInt("Master volume", &volume, 0, 128)) {
				apply_master_volume(volume);
			}
			ImGui::EndMenu();
		}
		menu_item_not_implemented("Memory (not implemented)");
		if (ImGui::BeginMenu("Mouse")) {
			bool capture = np2oscfg.MOUSE_SW != 0;
			const char *capture_shortcut = (np2oscfg.F12KEY == 0) ?
								"F12 / middle click" : "Middle click";
			if (ImGui::MenuItem("Capture mouse", capture_shortcut, capture)) {
				set_mouse_capture(!capture);
			}
			if (ImGui::BeginMenu("VA controller port")) {
				if (ImGui::MenuItem("Joystick", nullptr,
						mouseifvacfg.device == MOUSEIFVA_JOYPAD)) {
					set_mouse_device(MOUSEIFVA_JOYPAD);
				}
				if (ImGui::MenuItem("Mouse", nullptr,
						mouseifvacfg.device == MOUSEIFVA_MOUSE)) {
					set_mouse_device(MOUSEIFVA_MOUSE);
				}
				ImGui::EndMenu();
			}
			bool rapid = np2cfg.MOUSERAPID != 0;
			if (ImGui::MenuItem("Rapid buttons", nullptr, rapid)) {
				set_mouse_rapid(!rapid);
			}
			if (mousemng_status()[0] != '\0') {
				ImGui::Separator();
				ImGui::BeginDisabled();
				ImGui::MenuItem(mousemng_status());
				ImGui::EndDisabled();
			}
			ImGui::EndMenu();
		}
		menu_item_not_implemented("Serial option... (not implemented)");
		menu_item_not_implemented("MIDI option... (not implemented)");
		ImGui::EndMenu();
	}
}

static void draw_about_menu(void) {

	if (ImGui::MenuItem("About")) {
		g_gui.about_open = true;
		g_gui.about_request = true;
		g_gui.about_more = false;
		g_gui.about_info[0] = '\0';
	}
}

static void draw_about_dialog(void) {

	if (g_gui.about_request) {
		g_gui.about_request = false;
		ImGui::OpenPopup("About...##vaeg");
	}
	if (!g_gui.about_open) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float width = (std::min)(g_gui.about_more ? 620.0f : 360.0f,
									viewport->WorkSize.x * 0.9f);
	const float height = g_gui.about_more ?
		(std::min)(700.0f, viewport->WorkSize.y * 0.9f) : 310.0f;
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing,
											ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
	if (ImGui::BeginPopupModal("About...##vaeg", &g_gui.about_open,
								ImGuiWindowFlags_NoResize |
								ImGuiWindowFlags_NoCollapse)) {
		if (g_gui.about_texture != nullptr) {
			const float available = ImGui::GetContentRegionAvail().x;
			const float image_width = static_cast<float>(g_gui.about_texture_width);
			const float image_height = static_cast<float>(g_gui.about_texture_height);
			const float scale = (std::min)(1.0f, available / image_width);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
										(available - image_width * scale) * 0.5f);
			ImGui::Image(ImTextureRef(g_gui.about_texture),
								ImVec2(image_width * scale, image_height * scale));
		}
		else {
			ImGui::TextUnformatted("88VA Eternal Grafx");
		}

		bool close_about = false;
		if (ImGui::BeginTable("about-footer", 2,
								ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("text", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("buttons", ImGuiTableColumnFlags_WidthFixed,
														88.0f);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("88VA Eternal Grafx  %s", VAEGREL_CORE);
			ImGui::TableNextColumn();
			close_about = ImGui::Button("OK", ImVec2(-1.0f, 0.0f));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Based on Neko Project II  %s", NP2VER_CORE);
			ImGui::TableNextColumn();
			ImGui::BeginDisabled(g_gui.about_more);
			if (ImGui::Button("More >>", ImVec2(-1.0f, 0.0f))) {
				np2info(g_gui.about_info, kAboutInfoTemplate,
											sizeof(g_gui.about_info), nullptr);
				g_gui.about_more = true;
			}
			ImGui::EndDisabled();
			ImGui::EndTable();
		}

		if (g_gui.about_more) {
			ImGui::SeparatorText("Running VM configuration");
			ImGui::InputTextMultiline("##runtime-info", g_gui.about_info,
									sizeof(g_gui.about_info), ImVec2(-1.0f, -1.0f),
									ImGuiInputTextFlags_ReadOnly);
		}

		if (close_about ||
			ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.about_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void draw_state_menu(void) {

	if (ImGui::BeginMenu("State / 状態")) {
		if (ImGui::BeginMenu("Save")) {
			for (int slot = 0; slot < kStateSlots; slot++) {
				char label[32];
				std::snprintf(label, sizeof(label), "Slot %d", slot);
				if (ImGui::MenuItem(label)) {
					std::string path = state_slot_path(slot);
					soundmng_stop();
					int ret = statsave_save(path.c_str());
					if (ret != STATFLAG_SUCCESS) {
						file_delete(path.c_str());
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
					std::string path = state_slot_path(slot);
					char error[1024];
					error[0] = '\0';
					int ret = statsave_check(path.c_str(), error,
											 sizeof(error));
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
							taskmng_clear_fast_forward();
							statsave_load(path.c_str());
							sdlkbd_reset_state();
							mousemng_reset();
							scrndraw_redraw();
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

	ImFontConfig font_config;
	font_config.FontDataOwnedByAtlas = false;
	ImFont *font = io.Fonts->AddFontFromMemoryTTF(
		const_cast<unsigned char *>(vaeg_gui_font_ttf),
		static_cast<int>(vaeg_gui_font_ttf_size), kGuiFontSize,
		&font_config, io.Fonts->GetGlyphRangesJapanese());
	if (font == nullptr) {
		std::fprintf(stderr, "Error: failed to load embedded GUI font\n");
		return FAILURE;
	}

	ImGui::StyleColorsDark();
	scrnmng_set_menu_height(menu_bar_height());
	if (!ImGui_ImplSDL2_InitForSDLRenderer(static_cast<SDL_Window *>(window),
										   static_cast<SDL_Renderer *>(renderer))) {
		return FAILURE;
	}
	g_gui.renderer = static_cast<SDL_Renderer *>(renderer);
	if (!ImGui_ImplSDLRenderer2_Init(g_gui.renderer)) {
		ImGui_ImplSDL2_Shutdown();
		return FAILURE;
	}
	g_gui.about_texture = load_about_texture(g_gui.renderer,
									&g_gui.about_texture_width,
									&g_gui.about_texture_height);
	if (g_gui.about_texture == nullptr) {
		std::fprintf(stderr, "Warning: failed to load embedded About image: %s\n",
						SDL_GetError());
	}
	g_gui.initialized = true;
	return SUCCESS;
}

void gui_shutdown(void) {

	if (!g_gui.initialized) {
		return;
	}
	mousemng_setguiblocked(TRUE);
	if (g_gui.about_texture != nullptr) {
		SDL_DestroyTexture(g_gui.about_texture);
		g_gui.about_texture = nullptr;
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

	if (g_gui.capture_binding >= 0) {
		if ((sdl_event->type == SDL_KEYDOWN) && (!sdl_event->key.repeat)) {
			SDL_Scancode scancode = sdl_event->key.keysym.scancode;
			if (kbdmap_set_binding(g_gui.capture_binding, scancode) == SUCCESS) {
				const KBDMAP_ENTRY *entry = kbdmap_entry(g_gui.capture_binding);
				g_gui.keyboard_status = "Bound ";
				g_gui.keyboard_status += (entry != nullptr) ? entry->label : "(unknown)";
				g_gui.keyboard_status += " to ";
				g_gui.keyboard_status += binding_name(scancode);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			g_gui.capture_swallow = scancode;
			g_gui.capture_binding = -1;
			return TRUE;
		}
		if (sdl_event->type == SDL_KEYUP) {
			return TRUE;
		}
	}
	if ((g_gui.capture_swallow != SDL_SCANCODE_UNKNOWN) &&
		(sdl_event->type == SDL_KEYUP) &&
		(sdl_event->key.keysym.scancode == g_gui.capture_swallow)) {
		g_gui.capture_swallow = SDL_SCANCODE_UNKNOWN;
		return TRUE;
	}

	ImGuiIO &io = ImGui::GetIO();
	switch (sdl_event->type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_TEXTINPUT:
			return (io.WantCaptureKeyboard || io.WantTextInput) ? TRUE : FALSE;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
			return io.WantCaptureMouse ? TRUE : FALSE;

		default:
			return FALSE;
	}
}

BOOL gui_guest_keyboard_blocked(void) {

	if (!g_gui.initialized) {
		return FALSE;
	}
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureKeyboard || io.WantTextInput ||
		(g_gui.capture_binding >= 0)) {
		return TRUE;
	}
	return (g_gui.fdd_browser_open || g_gui.hdd_browser_open ||
			g_gui.new_fdd_open || g_gui.new_sasi_open ||
			g_gui.keyboard_config_open || g_gui.configure_open ||
			g_gui.custom_size_open || g_gui.about_open) ? TRUE : FALSE;
}

BOOL gui_guest_mouse_blocked(void) {

	if (!g_gui.initialized) {
		return FALSE;
	}
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse || (g_gui.capture_binding >= 0)) {
		return TRUE;
	}
	return (g_gui.fdd_browser_open || g_gui.hdd_browser_open ||
			g_gui.new_fdd_open || g_gui.new_sasi_open ||
			g_gui.keyboard_config_open || g_gui.configure_open ||
			g_gui.custom_size_open || g_gui.about_open) ? TRUE : FALSE;
}

void gui_new_frame(void) {

	if (!g_gui.initialized) {
		return;
	}
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	scrnmng_set_menu_height(
		static_cast<int>(std::ceil(ImGui::GetFrameHeight())));
}

void gui_draw(void) {

	if (!g_gui.initialized) {
		return;
	}
	if (ImGui::BeginMainMenuBar()) {
		draw_emulate_menu();
		draw_fdd_menu();
		draw_harddisk_menu();
		draw_edit_menu();
		draw_screen_menu();
		draw_device_menu();
		draw_state_menu();
		draw_system_menu();
		draw_about_menu();
		ImGui::EndMainMenuBar();
	}
	draw_fdd_browser();
	draw_hdd_browser();
	draw_new_fdd_dialog();
	draw_new_sasi_dialog();
	draw_keyboard_config();
	draw_configure_dialog();
	draw_custom_size_dialog();
	draw_sound_buffer_dialog();
	draw_about_dialog();
}

void gui_render(void) {

	if (!g_gui.initialized) {
		return;
	}
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),
										  g_gui.renderer);
}
