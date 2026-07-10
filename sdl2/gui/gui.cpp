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
#include <fstream>
#include <string>
#include <vector>

#include "compiler.h"
#include "gui/gui.h"
#include "diskdrv.h"
#include "dosio.h"
#include "fddfile.h"
#include "keystat.h"
#include "newdisk.h"
#include "np2.h"
#include "sound.h"
#include "adpcm.h"
#include "beep.h"
#include "pccore.h"
#include "sxsi.h"
#include "fdd_mtr.h"
#include "kbdmap.h"
#include "opngen.h"
#include "pcm86.h"
#include "psggen.h"
#include "rhythm.h"
#include "scrndraw.h"
#include "scrnmng.h"
#include "sdlkbd.h"
#include "soundmng.h"
#include "sysmng.h"
#include "taskmng.h"
#include "tms3631.h"

extern "C" {
extern _RHYTHM rhythm;
extern _ADPCM adpcm;
}

namespace {

constexpr const char *kFontName = "NotoSansJP-Regular.ttf";
constexpr float kGuiFontSize = 16.0f;
constexpr int kStateSlots = 10;
constexpr int kMasterVolumeMax = 128;
constexpr int kSasiImageCount = 6;
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
	std::string font_path;
	float menu_font_size = kGuiFontSize;
	int fdd_dialog_drive = -1;
	char fdd_path[2][MAX_PATH] = {};
	char fdd_mounted_path[2][MAX_PATH] = {};
	bool fdd_browser_open = false;
	bool fdd_browser_refresh = false;
	std::string fdd_browser_dir;
	std::vector<BrowserEntry> fdd_entries;
	std::string fdd_status;
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
	UINT sound_sw_saved = 0;
	bool keyboard_config_open = false;
	int capture_binding = -1;
	SDL_Scancode capture_swallow = SDL_SCANCODE_UNKNOWN;
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
		copy_path(g_gui.fdd_mounted_path[drive],
				  sizeof(g_gui.fdd_mounted_path[drive]), path);
	}
	else {
		g_gui.fdd_mounted_path[drive][0] = '\0';
	}
}

static void capture_reset_fdd_mounts(char paths[2][MAX_PATH]) {

	for (int drive = 0; drive < 2; drive++) {
		const char *current;

		paths[drive][0] = '\0';
		if (g_gui.fdd_mounted_path[drive][0] != '\0') {
			copy_path(paths[drive], MAX_PATH, g_gui.fdd_mounted_path[drive]);
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
	pccore_cfgupdate();
	pccore_reset();
	restore_reset_fdd_mounts(fdd_paths);
	sdlkbd_reset_state();
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

static void draw_emulate_menu(void) {

	if (ImGui::BeginMenu("Emulate / エミュレート")) {
		if (ImGui::MenuItem("Reset / リセット")) {
			reset_guest();
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
		if (ImGui::MenuItem("SASI-1 Open...")) {
			open_hdd_dialog(0);
		}
		if (ImGui::MenuItem("SASI-1 Remove")) {
			remove_hdd(0);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("SASI-2 Open...")) {
			open_hdd_dialog(1);
		}
		if (ImGui::MenuItem("SASI-2 Remove")) {
			remove_hdd(1);
		}
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
			if (ImGui::BeginMenu("OPN backend")) {
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
							statsave_load(path.c_str());
							sdlkbd_reset_state();
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

	g_gui.font_path = find_font_path();
	if (g_gui.font_path.empty()) {
		std::fprintf(stderr, "Error: %s not found in GUI asset paths\n",
					 kFontName);
		return FAILURE;
	}
	ImFont *font = io.Fonts->AddFontFromFileTTF(
		g_gui.font_path.c_str(), kGuiFontSize, nullptr,
		io.Fonts->GetGlyphRangesJapanese());
	if (font == nullptr) {
		std::fprintf(stderr, "Error: failed to load GUI font: %s\n",
					 g_gui.font_path.c_str());
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
		draw_screen_menu();
		draw_device_menu();
		draw_other_menu();
		draw_state_menu();
		draw_system_menu();
		ImGui::EndMainMenuBar();
	}
	draw_fdd_browser();
	draw_hdd_browser();
	draw_new_sasi_dialog();
	draw_keyboard_config();
}

void gui_render(void) {

	if (!g_gui.initialized) {
		return;
	}
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),
										  g_gui.renderer);
}
