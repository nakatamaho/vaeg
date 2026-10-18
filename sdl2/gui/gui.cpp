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
#include "vaeg_licenses.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "compiler.h"
#include "gui/gui.h"
#include "codecnv.h"
#include "memoryva.h"
#include "maketextva.h"
#include "bkupmemva.h"
#include "extrarom.h"
#include "pcg88.h"
#include "n80rom.h"
#include "cmt.h"
#include "diskdrv.h"
#include "dosio.h"
#include "dropmedia.h"
#include "hostfat_manager.h"
#include "hostfat_path.h"
#include "fddfile.h"
#include "fdd_d88.h"
#include "fontdata.h"
#include "romva.h"
#include "machine/keystat.h"
#include "newdisk.h"
#include "np2.h"
#include "np2info.h"
#include "np2ver.h"
#include "sound.h"
#include "adpcm.h"
#include "tsp.h"
#include "videova.h"
#include "beep.h"
#include "bmsio.h"
#include "emsio.h"
#include "ini.h"
#include "machine/pccore.h"
#include "machine/timing.h"
#include "frontpanel.h"
#include "sxsi.h"
#include "fdd_mtr.h"
#include "kbdmap.h"
#include "kbdpaste.h"
#include "mousemng.h"
#include "mouseifva.h"
#include "opngen.h"
#include "psggen.h"
#include "rhythm.h"
#include "scrndrawva.h"
#include "scrnmng.h"
#include "scrndraw.h"
#include "sdlkbd.h"
#include "sgp.h"
#include "soundmng.h"
#include "soundopts.h"
#include "strres.h"
#include "sysmng.h"
#include "taskmng.h"
#include "timemng.h"
#include "ymfmbridge.h"
#include "librashader/shader_preset.h"

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
constexpr int kScsiImageCount = 6;
constexpr int kSgpPresets[] = {1, 2, 4, 8, 16};
constexpr int kNdp8087Presets[] = {5000000, 8000000, 10000000};
constexpr int kSoundBufferPresets[] = {40, 100, 200, 500, 1000};
constexpr UINT kV98FontRomSize = 0x46800;
constexpr const char kAboutInfoTemplate[] = "CPU: %CPU% %CPUCLK%\n"
                                            "SGP: %SGPCLK%\n"
                                            "FRAME: %FRAME%\n"
                                            "BUILD COMMIT: %COMMIT%\n"
                                            "MODEL: %MODEL%\n"
                                            "SOUND: %SND%\n"
                                            "RHYTHM: %RHYTHM%\n"
                                            "\n"
                                            "[88VA]\n"
                                            "ROM TYPE: %ROMTPVA%\n"
                                            "ROM(Main): %BIOSVA%\n"
                                            "ROM(VupB): %BIOS91%\n"
                                            "ROM(Sub): %BIOSSUB%\n"
                                            "Z80 MODE: %Z80MODE%\n"
                                            "ROM(N80): %BIOSN80%";
namespace fs = std::filesystem;

struct SasiImageChoice {
	const char *label;
	UINT hdd_type;
};

static const SasiImageChoice kSasiImageChoices[kSasiImageCount] = {
    {"5 MB", 0}, {"10 MB", 1}, {"15 MB", 2}, {"20 MB", 4}, {"30 MB", 5}, {"40 MB", 6},
};

struct ScsiImageChoice {
	const char *label;
	UINT size_mb;
};

static const ScsiImageChoice kScsiImageChoices[kScsiImageCount] = {
    {"5 MB", 5}, {"10 MB", 10}, {"20 MB", 20}, {"40 MB", 40}, {"80 MB", 80}, {"160 MB", 160},
};

static void reset_guest(void);
static bool is_directory(const std::string &path);
static void open_hostfat_browser(void);
static void draw_hostfat_browser_popup(void);
static void draw_state_error_dialog(void);
static void state_capture_software(void);
static void history_record_boot(void);
static bool load_tape_path(const std::string &path);
static void draw_hostfat_error_dialog(void);

struct BrowserEntry {
	std::string name;
	std::string path;
	bool is_dir = false;
};

struct FullscreenMenuState {
	bool exclusive = false;
	bool visible = false;
	Uint32 last_activity = 0;
	Uint32 entered_at = 0;
};

static constexpr float kFullscreenRevealHeight = 12.0f;

static bool fullscreen_hint_visible(const FullscreenMenuState &state, Uint32 now) {
	return state.exclusive && static_cast<Uint32>(now - state.entered_at) < 3000U;
}

static bool update_fullscreen_menu(FullscreenMenuState &state, bool exclusive,
                                   bool at_top, bool over_bar, bool interacting, Uint32 now) {
	if (!exclusive) {
		state = FullscreenMenuState{};
		return true;
	}
	if (!state.exclusive) {
		state = FullscreenMenuState{};
		state.exclusive = true;
		state.entered_at = now;
	}
	if (at_top) state.visible = true;
	if (state.visible) {
		if (at_top || over_bar || interacting) state.last_activity = now;
		else if (static_cast<Uint32>(now - state.last_activity) >= 500U) state.visible = false;
	}
	return state.visible;
}

struct GuiState {
	FullscreenMenuState fullscreen_menu;
	bool initialized = false;
	bool text_input_active = false;
	SDL_Renderer *renderer = nullptr;
	SDL_Window *window = nullptr;
	bool native_renderer = false;
	SDL_Texture *about_texture = nullptr;
	ImTextureData *native_about_texture = nullptr;
	int about_texture_width = 0;
	int about_texture_height = 0;
	float menu_font_size = kGuiFontSize;
	ImGuiStyle base_style;
	float ui_scale = 0.0f;
	int fdd_dialog_drive = -1;
	char fdd_path[2][MAX_PATH] = {};
	bool fdd_browser_open = false;
	bool fdd_browser_refresh = false;
	std::string fdd_browser_dir;
	std::vector<BrowserEntry> fdd_entries;
	bool fdd_all_files = false; /* file type: All files (*.*) */
	std::string fdd_status;
	std::string font_status;
	std::string screenshot_status;
	vaeg::librashader::ShaderPreset native_crt_preset;
	std::string native_crt_loaded_path;
	std::string native_crt_status;
	bool native_crt_settings_open = false;
	bool new_fdd_open = false;
	bool new_fdd_refresh = false;
	int new_fdd_format = NEWDISK_FDD_BLANK_2HD_1200;
	int new_fdd_container = NEWDISK_FDD_CONTAINER_D88;
	int new_fdd_drive = 0;
	bool new_fdd_mount_after_create = true;
	char new_fdd_path[MAX_PATH] = {};
	int hdd_dialog_drive = -1;
	char hdd_path[4][MAX_PATH] = {};
	bool hdd_browser_open = false;
	// Cassette tape (vaeg V1/V2 extension).
	bool tape_browser_open = false;
	bool tape_browser_refresh = false;
	std::string tape_browser_dir;
	std::vector<BrowserEntry> tape_entries;
	bool tape_all_files = false; /* file type: All files (*.*) */
	char tape_path[MAX_PATH] = {};
	std::string tape_loaded;
	std::string tape_recording;
	std::string tape_status;
	bool hdd_browser_refresh = false;
	std::string hdd_browser_dir;
	std::vector<BrowserEntry> hdd_entries;
	bool hdd_all_files = false; /* SASI file type: All files (*.*) */
	std::string hdd_status;
	bool new_sasi_open = false;
	bool new_sasi_refresh = false;
	int new_sasi_drive = 0;
	int new_sasi_choice = 3;
	bool new_sasi_open_after_create = true;
	char new_sasi_path[MAX_PATH] = {};
	bool new_scsi_open = false;
	bool new_scsi_refresh = false;
	int new_scsi_drive = 0;
	int new_scsi_choice = 3;
	bool new_scsi_open_after_create = true;
	char new_scsi_path[MAX_PATH] = {};
	std::string state_status;
	std::string osd_text;  /* M125: a short notice at the top right */
	Uint32 osd_until = 0;  /* SDL ticks when it disappears */
	Uint32 quick_last = 0; /* ticks of the last quick save or load */
	bool quick_used = false;
	bool state_error_open = false;
	bool state_error_request = false;
	bool state_force_hostfat_available = false;
	std::string state_force_hostfat_path;
	std::string state_software_media;
	bool state_software_set = false;
	bool state_overview_open = false;
	bool state_save_as_request = false;
	bool state_save_as_open = false;
	char state_save_as_name[128] = {};
	bool state_browser_open = false;
	std::string state_browser_dir;
	std::string keyboard_status;
	bool keyboard_config_open = false;
	int capture_binding = -1;
	SDL_Scancode capture_swallow = SDL_SCANCODE_UNKNOWN;
	bool configure_open = false;
	bool configure_request = false;
	int pending_cpu_multiplier = PCCORE_STANDARD_MULTIPLE;
	int pending_sgp_mode = SGP_SPEED_MODEL_DEFAULT;
	int pending_sgp_multiplier = 1;
	bool pending_ndp8087_enabled = false;
	int pending_ndp8087_clock_hz = static_cast<int>(UPD8087_DEFAULT_CLOCK_HZ);
	bool pending_ndp8087_host_fpu = true;
	int pending_pacing_ms = 0;
	bool pending_hostfat_enabled = false;
	char pending_hostfat_dir[MAX_PATH] = {};
	bool pending_hostfat_rebuild = false;
	bool hostfat_reset_after_build = false;
	std::string hostfat_rebuild_dir;
	bool hostfat_browser_open = false;
	bool hostfat_browser_request = false;
	bool hostfat_browser_refresh = false;
	std::string hostfat_browser_dir;
	std::vector<BrowserEntry> hostfat_entries;
	std::string hostfat_status;
	bool hostfat_error_open = false;
	bool hostfat_error_request = false;
	std::string hostfat_error_message;
	bool bms_config_open = false;
	bool bms_config_request = false;
	bool pending_bms_enabled = false;
	int pending_bms_port = 0;
	int pending_bms_banks = BMSIO_DEFAULT_BANKS;
	bool ems_config_open = false;
	bool ems_config_request = false;
	bool pending_ems_enabled = false;
	int pending_ems_megabytes = EMSIO_DEFAULT_MEGABYTES;
	bool custom_size_open = false;
	bool custom_size_request = false;
	bool custom_size_integer = true;
	int pending_window_scale = 4;
	int pending_window_width = 640;
	int pending_window_height = 422;
	bool sound_buffer_open = false;
	bool sound_buffer_request = false;
	int pending_sound_buffer_ms = VAEG_SOUND_BUFFER_DEFAULT_MS;
	bool about_open = false;
	std::string boot_status; /* M116: why a recent boot did not start */
	bool about_request = false;
	bool about_more = false;
	char about_info[2048] = {};
};

GuiState g_gui;

struct CopyTextFrame {
	UINT16 vw;
	UINT8 mode;
	UINT32 rsa;
	UINT16 rh;
	UINT16 rw;
};

static void copy_append_utf8(std::string *text, UINT32 codepoint) {
	if (codepoint <= 0x7f) {
		text->push_back(static_cast<char>(codepoint));
	} else if (codepoint <= 0x7ff) {
		text->push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
		text->push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
	} else if (codepoint <= 0xffff) {
		text->push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
		text->push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
		text->push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
	} else {
		text->push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
		text->push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
		text->push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
		text->push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
	}
}

static void copy_append_replacement(std::string *text) {
	copy_append_utf8(text, 0xfffd);
}

static void copy_append_hccode(std::string *text, UINT16 hccode) {
	char sjis[3];
	UINT16 utf[4];
	UINT16 jis1;
	UINT16 jis2;
	UINT row;

	if (hccode & 0x8000) {
		return;
	}
	if ((hccode & 0xff00) == 0) {
		if ((hccode == 0) || (hccode == 0x20)) {
			text->push_back(' ');
		} else if ((hccode >= 0x21) && (hccode <= 0x7e)) {
			text->push_back(static_cast<char>(hccode));
		} else if ((hccode >= 0xa1) && (hccode <= 0xdf)) {
			sjis[0] = static_cast<char>(hccode);
			sjis[1] = '\0';
			ZeroMemory(utf, sizeof(utf));
			codecnv_sjis2utf(utf, NELEMENTS(utf), sjis, 2);
			if (utf[0] != 0) {
				copy_append_utf8(text, utf[0]);
			} else {
				copy_append_replacement(text);
			}
		} else {
			copy_append_replacement(text);
		}
		return;
	}
	jis1 = (hccode & 0x7f) + 0x20;
	jis2 = (hccode >> 8) & 0x7f;
	if ((jis1 < 0x21) || (jis1 > 0x7e) || (jis2 < 0x21) || (jis2 > 0x7e)) {
		copy_append_replacement(text);
		return;
	}
	row = jis1 - 0x21;
	/* The JIS row parity selects the Shift-JIS trail-byte formula. */
	sjis[0] = static_cast<char>((row >> 1) + 0x81);
	if ((static_cast<unsigned char>(sjis[0])) >= 0xa0) {
		sjis[0] = static_cast<char>(sjis[0] + 0x40);
	}
	if (row & 1) {
		sjis[1] = static_cast<char>(jis2 + 0x7e);
	} else {
		sjis[1] = static_cast<char>(jis2 + 0x1f);
		if (static_cast<unsigned char>(sjis[1]) >= 0x7f) {
			sjis[1] = static_cast<char>(sjis[1] + 1);
		}
	}
	sjis[2] = '\0';
	ZeroMemory(utf, sizeof(utf));
	codecnv_sjis2utf(utf, NELEMENTS(utf), sjis, 3);
	if (utf[0] != 0) {
		copy_append_utf8(text, utf[0]);
	} else {
		copy_append_replacement(text);
	}
}

/*
 * V1/V2 text under the TSP's uPD3301 emulation: one byte per character in
 * the 8801 format, read as it is displayed (byte-mode addresses, main RAM in
 * standard speed). With port 30h 80CM clear (40 columns) only even byte
 * columns hold characters. Semigraphics cells are copied as their codes.
 */
static void copy_3301_text(std::vector<std::string> *lines) {
	const UINT step = (videova.txtmode8 & 0x01) ? 1 : 2;
	UINT row;

	for (row = 0;; row++) {
		const BYTE *chars = maketextva_3301_text(row);
		std::string line;
		UINT column;

		if (chars == nullptr) {
			if (row >= tsp.emul_rows) {
				break;
			}
			lines->push_back(line);
			continue;
		}
		for (column = 0; column < tsp.emul_chars; column += step) {
			copy_append_hccode(&line, chars[column]);
		}
		while (!line.empty() && (line.back() == ' ')) {
			line.pop_back();
		}
		lines->push_back(line);
	}
}

static BOOL copy_screen_text(void) {
	std::vector<std::string> lines;
	const UINT32 tvram_size = 0x40000;
	const UINT frame_count = 4;
	const UINT lineheight = (tsp.lineheight != 0) ? tsp.lineheight : 1;
	const UINT visible_columns =
	    (videova.txtmode8 & 0x01) ? (SURFACE_WIDTH / 8) : (SURFACE_WIDTH / 16);
	UINT32 raster_used = 0;
	UINT frame_no;

	if (memoryva_88_mode && tsp.emul) {
		copy_3301_text(&lines);
		raster_used = 0x1fe; /* the native frames below are not shown */
	}
	for (frame_no = 0; frame_no < frame_count && raster_used < 0x1fe; frame_no++) {
		BYTE *entry = textmem + tsp.texttable + frame_no * 0x20;
		CopyTextFrame frame;
		UINT raster_count;
		UINT rows;
		UINT columns;
		UINT frame_columns;
		UINT row;

		if ((tsp.texttable + frame_no * 0x20 + 0x1c) >= tvram_size) {
			break;
		}
		frame.vw = LOADINTELWORD(entry + 0x08) & 0x03ff;
		frame.mode = LOADINTELWORD(entry + 0x0a) & 0x07;
		frame.rsa = LOADINTELWORD(entry + 0x10);
		frame.rh = LOADINTELWORD(entry + 0x14) & 0x01fe;
		frame.rw = LOADINTELWORD(entry + 0x16) & 0x03ff;
		if (frame.rh == 0) {
			frame.rh = 0x01fe;
		}
		raster_count = (frame_no == frame_count - 1) ? 0x1fe - raster_used : frame.rh;
		if (raster_count > 0x1fe - raster_used) {
			raster_count = 0x1fe - raster_used;
		}
		if (raster_count == 0) {
			break;
		}
		rows = raster_count / lineheight;
		frame_columns = frame.rw / 8 + 2;
		columns = (frame_columns < visible_columns) ? frame_columns : visible_columns;
		for (row = 0; row < rows; row++) {
			std::string line;
			UINT column;
			UINT16 previous_hccode = 0;
			BOOL have_previous_hccode = FALSE;
			UINT32 address = frame.rsa + frame.vw * row;

			if ((address >= tvram_size) || ((UINT64)address + columns * 2 > tvram_size)) {
				break;
			}
			for (column = 0; column < columns; column++) {
				BYTE *cell = textmem + address + column * 2;
				UINT16 hccode = LOADINTELWORD(cell);
				BYTE attr = 0;

				if ((UINT64)(cell - textmem) + tsp.attroffset < tvram_size) {
					attr = cell[tsp.attroffset];
				}
				if (have_previous_hccode && (hccode == (previous_hccode | 0x8000))) {
					previous_hccode = hccode;
					continue;
				}
				if ((frame.mode == 1) && (attr & 0x01)) {
					line.push_back(' ');
				} else {
					copy_append_hccode(&line, hccode);
				}
				previous_hccode = hccode;
				have_previous_hccode = TRUE;
			}
			while (!line.empty() && (line.back() == ' ')) {
				line.pop_back();
			}
			lines.push_back(line);
		}
		raster_used += raster_count;
	}
	while (!lines.empty() && lines.back().empty()) {
		lines.pop_back();
	}
	std::string clipboard;
	for (frame_no = 0; frame_no < lines.size(); frame_no++) {
		if (frame_no != 0) {
			clipboard.push_back('\n');
		}
		clipboard += lines[frame_no];
	}
	if (SDL_SetClipboardText(clipboard.c_str()) != 0) {
		g_gui.keyboard_status = "Copy failed: ";
		g_gui.keyboard_status += SDL_GetError();
		return (FAILURE);
	}
	g_gui.keyboard_status = "Copy complete.";
	return (SUCCESS);
}

static BOOL save_screenshot(BOOL with_graphics_analysis) {
	_SYSTIME systime;
	char path[MAX_PATH];
	UINT attempt;
	const UINT32 ticks = SDL_GetTicks();

	if (!g_gui.initialized) {
		return (FAILURE);
	}
	if (timemng_gettime(&systime) != SUCCESS) {
		g_gui.screenshot_status = "スクリーンショットの時刻取得に失敗しました。";
		return (FAILURE);
	}
	// M110: screenshots/ next to the executable (one directory for all
	// software), with today's date-and-time names.
	char directory[MAX_PATH];
	(void)file_getappdir(directory, sizeof(directory), "screenshots");
	for (attempt = 0; attempt < 1000; attempt++) {
		char name[96];
		std::snprintf(name, sizeof(name), "vaeg-%04u%02u%02u-%02u%02u%02u-%010u-%03u.png",
		              systime.year, systime.month, systime.day, systime.hour, systime.minute,
		              systime.second, static_cast<unsigned int>(ticks), attempt);
		file_cpyname(path, directory, sizeof(path));
		file_catname(path, name, sizeof(path));
		if (file_attr(path) < 0) {
			break;
		}
	}
	if (attempt == 1000) {
		g_gui.screenshot_status = "スクリーンショット名を確保できません。";
		return (FAILURE);
	}
	if (!with_graphics_analysis) {
		if (scrnmng_request_display_capture(path) != SUCCESS) {
			g_gui.screenshot_status = "Screenshot request failed: ";
			g_gui.screenshot_status += SDL_GetError();
			return FAILURE;
		}
		g_gui.screenshot_status = "Screenshot queued";
		return SUCCESS;
	}
	if (scrnmng_save_guest_frame_with_analysis(path) != SUCCESS) {
		g_gui.screenshot_status = "スクリーンショット保存に失敗しました: ";
		g_gui.screenshot_status += SDL_GetError();
		return (FAILURE);
	}
	g_gui.screenshot_status = "スクリーンショット（加工前）を保存しました: ";
	g_gui.screenshot_status += path;
	std::fprintf(stderr, "screenshot saved path=%s\n", path);
	return (SUCCESS);
}

static void update_text_input_state(void) {
	if (!g_gui.initialized) {
		return;
	}
	ImGuiIO &io = ImGui::GetIO();
	const bool wanted = io.WantTextInput;
	if (wanted == g_gui.text_input_active) {
		return;
	}
	if (wanted) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
	g_gui.text_input_active = wanted;
}

static SDL_Texture *load_about_texture(SDL_Renderer *renderer, int *width, int *height) {
	SDL_RWops *stream = SDL_RWFromConstMem(vaeg_splash_bmp, static_cast<int>(vaeg_splash_bmp_size));
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

static void load_native_about_texture(void) {
	SDL_RWops *stream = SDL_RWFromConstMem(vaeg_splash_bmp, static_cast<int>(vaeg_splash_bmp_size));
	if (!stream)
		return;
	SDL_Surface *source = SDL_LoadBMP_RW(stream, 1);
	if (!source)
		return;
	SDL_Surface *rgba = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_FreeSurface(source);
	if (!rgba)
		return;
	auto *texture = IM_NEW(ImTextureData)();
	texture->Create(ImTextureFormat_RGBA32, rgba->w, rgba->h);
	for (int row = 0; row < rgba->h; ++row) {
		std::memcpy(texture->GetPixelsAt(0, row),
		            static_cast<const unsigned char *>(rgba->pixels) + row * rgba->pitch,
		            texture->GetPitch());
	}
	g_gui.about_texture_width = rgba->w;
	g_gui.about_texture_height = rgba->h;
	SDL_FreeSurface(rgba);
	ImGui::RegisterUserTexture(texture);
	g_gui.native_about_texture = texture;
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

static const char *new_sasi_default_name(void) {
	return "new-sasi-hdd.hdd";
}

static std::string new_scsi_default_name(int drive) {
	std::string name = "new-scsi-hdd_id";

	name += std::to_string(std::clamp(drive, 0, SCSIHDD_MAX - 1));
	name += ".hdi";
	return name;
}

static std::string new_scsi_default_path(const std::string &directory, int drive) {
	std::string name = new_scsi_default_name(drive);

	return join_path(directory, name.c_str());
}

static void open_configure_dialog(void) {
	g_gui.pending_cpu_multiplier = static_cast<int>(np2cfg.multiple);
	g_gui.pending_sgp_mode = static_cast<int>(np2cfg.sgp_speed_mode);
	g_gui.pending_sgp_multiplier = static_cast<int>(np2cfg.sgp_multiplier);
	g_gui.pending_ndp8087_enabled = np2cfg.upd8087_enable != 0;
	g_gui.pending_ndp8087_clock_hz = static_cast<int>(np2cfg.upd8087_clock_hz);
	g_gui.pending_ndp8087_host_fpu = np2cfg.upd8087_host_fpu != 0;
	g_gui.pending_pacing_ms = static_cast<int>(np2oscfg.pacing_ms);
	g_gui.pending_hostfat_enabled = np2oscfg.hostfat_enabled != 0;
	milstr_ncpy(g_gui.pending_hostfat_dir, np2oscfg.hostfat_dir, sizeof(g_gui.pending_hostfat_dir));
	g_gui.pending_hostfat_rebuild = false;
	g_gui.configure_open = true;
	g_gui.configure_request = true;
}

/*
 * M110: dialogs fit the vaeg window. The first size is given in font units
 * and clamped to 95 % of the work area, which also bounds the user's
 * resizing; ImGui keeps a resized window's size for the session. The
 * window is centred each time it appears.
 */
static void prepare_dialog(float width_em, float height_em, ImGuiCond size_cond) {
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float unit = ImGui::GetFontSize();
	const ImVec2 limit(viewport->WorkSize.x * 0.95f, viewport->WorkSize.y * 0.95f);

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
	                               viewport->WorkPos.y + viewport->WorkSize.y * 0.5f),
	                        ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(
	    ImVec2((std::min)(width_em * unit, limit.x), (std::min)(height_em * unit, limit.y)),
	    size_cond);
	ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), limit);
}

/* A file list taking the height the lines below it (and a wrapped status
 * text) leave, so that the path field and the buttons stay visible. */
static float browser_list_height(float footer_lines, const std::string &status) {
	const ImGuiStyle &style = ImGui::GetStyle();
	float footer = footer_lines * ImGui::GetFrameHeightWithSpacing();

	if (!status.empty()) {
		footer +=
		    ImGui::CalcTextSize(status.c_str(), nullptr, false, ImGui::GetContentRegionAvail().x)
		        .y +
		    2.0f * style.ItemSpacing.y + 1.0f;
	}
	return (std::max)(ImGui::GetFrameHeight() * 3.0f, ImGui::GetContentRegionAvail().y - footer);
}

static void draw_multiplier_input(const char *label, int *value, const int *presets,
                                  int preset_count) {
	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	ImGui::SameLine(145.0f);
	ImGui::SetNextItemWidth(80.0f);
	ImGui::InputInt("##value", value, 1, 0);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(110.0f);
	if (ImGui::BeginCombo("##presets", "Preset")) {
		for (int i = 0; i < preset_count; i++) {
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

static void draw_ndp8087_clock_input(int *value) {
	ImGui::PushID("NDP8087 clock");
	const float unit = ImGui::GetFontSize();
	ImGui::TextUnformatted("Clock (Hz)");
	ImGui::SameLine(unit * 6.0f);
	ImGui::SetNextItemWidth(unit * 11.0f);
	ImGui::InputInt("##value", value, 100000, 1000000);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(unit * 7.0f);
	if (ImGui::BeginCombo("##presets", "Preset")) {
		for (int i = 0; i < static_cast<int>(std::size(kNdp8087Presets)); i++) {
			char item[32];
			std::snprintf(item, sizeof(item), "%d MHz", kNdp8087Presets[i] / 1000000);
			if (ImGui::Selectable(item, *value == kNdp8087Presets[i])) {
				*value = kNdp8087Presets[i];
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();
}

static void apply_configure_dialog(void) {
	const bool clock_changed =
	    (np2cfg.baseclock != PCBASECLOCK40) ||
	    (np2cfg.multiple != static_cast<UINT>(g_gui.pending_cpu_multiplier)) ||
	    (np2cfg.sgp_speed_mode != static_cast<UINT8>(g_gui.pending_sgp_mode)) ||
	    (np2cfg.sgp_multiplier != static_cast<UINT8>(g_gui.pending_sgp_multiplier)) ||
	    (np2oscfg.pacing_ms != static_cast<UINT16>(g_gui.pending_pacing_ms));
	const bool ndp8087_changed =
	    ((np2cfg.upd8087_enable != 0) != g_gui.pending_ndp8087_enabled) ||
	    (np2cfg.upd8087_clock_hz != static_cast<UINT32>(g_gui.pending_ndp8087_clock_hz));
	const bool hostfat_changed =
	    ((np2oscfg.hostfat_enabled != 0) != g_gui.pending_hostfat_enabled) ||
	    (std::strcmp(np2oscfg.hostfat_dir, g_gui.pending_hostfat_dir) != 0);
	bool reset_done = false;

	if (clock_changed) {
		np2cfg.baseclock = PCBASECLOCK40;
		np2cfg.multiple = static_cast<UINT>(g_gui.pending_cpu_multiplier);
		np2cfg.sgp_speed_mode = static_cast<UINT8>(g_gui.pending_sgp_mode);
		np2cfg.sgp_multiplier = static_cast<UINT8>(g_gui.pending_sgp_multiplier);
		np2oscfg.pacing_ms = static_cast<UINT16>(g_gui.pending_pacing_ms);
		sysmng_update(SYS_UPDATECFG | SYS_UPDATEOSCFG | SYS_UPDATECLOCK);
		reset_guest();
		reset_done = true;
	}
	// M123: takes effect at once; no reset
	if ((np2cfg.upd8087_host_fpu != 0) != g_gui.pending_ndp8087_host_fpu) {
		np2cfg.upd8087_host_fpu = g_gui.pending_ndp8087_host_fpu ? 1 : 0;
		upd8087_set_host_fpu(np2cfg.upd8087_host_fpu != 0);
		sysmng_update(SYS_UPDATECFG);
	}
	if (ndp8087_changed) {
		np2cfg.upd8087_enable = g_gui.pending_ndp8087_enabled ? 1 : 0;
		np2cfg.upd8087_clock_hz = static_cast<UINT32>(g_gui.pending_ndp8087_clock_hz);
		sysmng_update(SYS_UPDATECFG | SYS_UPDATECLOCK);
		if (!reset_done) {
			reset_guest();
			reset_done = true;
		}
	}
	if (hostfat_changed || g_gui.pending_hostfat_rebuild) {
		char error[256]{};
		if (g_gui.pending_hostfat_enabled) {
			const std::string hostfat_dir = vaeg_hostfat::normalize_path(g_gui.pending_hostfat_dir);
			if (hostfat_manager_rebuild_async(hostfat_dir.c_str(), error, sizeof(error)) ==
			    SUCCESS) {
				g_gui.hostfat_rebuild_dir = hostfat_dir;
				g_gui.hostfat_status = "Building immutable HOSTFAT snapshot...";
				g_gui.hostfat_reset_after_build = true;
			} else {
				g_gui.hostfat_status = "HOSTFAT rebuild failed to start: ";
				g_gui.hostfat_status += error;
				g_gui.hostfat_error_message = g_gui.hostfat_status;
				g_gui.hostfat_error_open = true;
				g_gui.hostfat_error_request = true;
			}
		} else if (hostfat_manager_unmount(error, sizeof(error)) == SUCCESS) {
			np2oscfg.hostfat_enabled = 0;
			np2oscfg.hostfat_dir[0] = '\0';
			sysmng_update(SYS_UPDATEOSCFG);
			g_gui.hostfat_status = "HOSTFAT unmounted.";
			if (!reset_done) {
				reset_guest();
			}
		} else {
			g_gui.hostfat_status = "HOSTFAT unmount failed: ";
			g_gui.hostfat_status += error;
			g_gui.hostfat_error_message = g_gui.hostfat_status;
			g_gui.hostfat_error_open = true;
			g_gui.hostfat_error_request = true;
		}
	}
	g_gui.pending_hostfat_rebuild = false;
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
	/*
	 * The dialog takes the height of its sections (font units, no fixed
	 * pixel sizes), up to 95 % of the window; beyond that the sections
	 * scroll and the message line and the buttons stay visible.
	 */
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float unit = ImGui::GetFontSize();
	const ImVec2 limit(viewport->WorkSize.x * 0.95f, viewport->WorkSize.y * 0.95f);
	const float width = (std::min)(unit * 36.0f, limit.x);
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
	                               viewport->WorkPos.y + viewport->WorkSize.y * 0.5f),
	                        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, limit.y));
	if (ImGui::BeginPopupModal("Configure##clock-config", &g_gui.configure_open,
	                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
		const bool cpu_valid =
		    pccore_cpu_multiple_valid(static_cast<UINT>(g_gui.pending_cpu_multiplier));
		const bool sgp_mode_valid = sgp_speed_mode_valid(static_cast<UINT>(g_gui.pending_sgp_mode));
		const bool sgp_multiplier_valid =
		    (g_gui.pending_sgp_mode != SGP_SPEED_CUSTOM) ||
		    sgp_speed_multiplier_valid(static_cast<UINT>(g_gui.pending_sgp_multiplier));
		const bool ndp8087_clock_valid =
		    (g_gui.pending_ndp8087_clock_hz >= static_cast<int>(UPD8087_MIN_CLOCK_HZ)) &&
		    (g_gui.pending_ndp8087_clock_hz <= static_cast<int>(UPD8087_MAX_CLOCK_HZ)) &&
		    upd8087_clock_valid(static_cast<UINT32>(g_gui.pending_ndp8087_clock_hz));
		const bool hostfat_valid =
		    !g_gui.pending_hostfat_enabled || is_directory(g_gui.pending_hostfat_dir);
		HOSTFAT_MANAGER_STATUS hostfat_manager_status{};
		hostfat_manager_get_status(&hostfat_manager_status);
		const bool hostfat_idle = hostfat_manager_status.state != HOSTFAT_MANAGER_BUILDING;
		const bool valid =
		    cpu_valid && sgp_mode_valid && sgp_multiplier_valid && ndp8087_clock_valid &&
		    hostfat_valid && hostfat_idle;
		const bool hostfat_action_error =
		    (g_gui.hostfat_status.rfind("HOSTFAT rebuild failed", 0) == 0) ||
		    (g_gui.hostfat_status.rfind("HOSTFAT unmount failed", 0) == 0);

		// The sections scroll when the window is too low; the message line
		// and the buttons stay below.
		const ImGuiStyle &style = ImGui::GetStyle();
		const float footer = 2.0f * ImGui::GetFrameHeightWithSpacing() + ImGui::GetFrameHeight() +
		                     2.0f * style.WindowPadding.y;
		const ImGuiChildFlags section = ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY;
		ImGui::SetNextWindowSizeConstraints(
		    ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, (std::max)(limit.y - footer, unit * 6.0f)));
		ImGui::BeginChild("configure-body", ImVec2(width - 2.0f * style.WindowPadding.x, 0.0f),
		                  ImGuiChildFlags_AutoResizeY);
		if (ImGui::BeginChild("ndp8087-config", ImVec2(0.0f, 0.0f), section,
		                      ImGuiWindowFlags_NoScrollbar)) {
			const bool requested_changed =
			    ((np2cfg.upd8087_enable != 0) != g_gui.pending_ndp8087_enabled) ||
			    (np2cfg.upd8087_clock_hz != static_cast<UINT32>(g_gui.pending_ndp8087_clock_hz));
			ImGui::TextUnformatted("Intel 8087 (NDP)");
			ImGui::Separator();
			ImGui::Checkbox("Enable 8087", &g_gui.pending_ndp8087_enabled);
			draw_ndp8087_clock_input(&g_gui.pending_ndp8087_clock_hz);
			if (upd8087_host_fpu_available()) {
				ImGui::Checkbox("FPTAN/FPATAN/F2XM1/FYL2X/FYL2XP1 on the host x87",
				                &g_gui.pending_ndp8087_host_fpu);
				ImGui::TextDisabled("Faster and closer to exact; results may differ from "
				                    "other hosts in the last bits.");
			} else {
				ImGui::TextDisabled("Transcendentals: SoftFloat series (no host x87).");
			}
			ImGui::Text("Active: %s, %u Hz", upd8087.enabled ? "enabled" : "disabled",
			            static_cast<unsigned int>(upd8087.clock_hz));
			if (requested_changed) {
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
				                   "Requested values apply on guest reset.");
			} else {
				ImGui::TextDisabled("Requested values are active.");
			}
			ImGui::TextDisabled("One 8087 maximum; FPO2 is never connected.");
		}
		ImGui::EndChild();

		if (ImGui::BeginChild("sgp-config", ImVec2(0.0f, 0.0f), section,
		                      ImGuiWindowFlags_NoScrollbar)) {
			static const char *modes[] = {"Model default", "Follow CPU", "Custom"};
			const UINT model = (milstr_cmp(np2cfg.model, str_VA1) == 0) ? PCMODEL_VA1 : PCMODEL_VA2;
			const double model_clock_mhz = static_cast<double>(sgp_model_clock(model)) / 1000000.0;
			double effective_scale = 1.0;
			ImGui::TextUnformatted("SGP");
			ImGui::Separator();
			ImGui::Combo("Speed", &g_gui.pending_sgp_mode, modes,
			             static_cast<int>(std::size(modes)));
			if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				draw_multiplier_input("Custom multiplier", &g_gui.pending_sgp_multiplier,
				                      kSgpPresets, static_cast<int>(std::size(kSgpPresets)));
			}
			if (g_gui.pending_sgp_mode == SGP_SPEED_MODEL_DEFAULT) {
				ImGui::TextUnformatted("Effective SGP scale: Model default (x1.0000)");
			} else if (g_gui.pending_sgp_mode == SGP_SPEED_FOLLOW_CPU) {
				ImGui::Text("Effective SGP scale: x%.4f relative to Model default",
				            static_cast<double>(g_gui.pending_cpu_multiplier) /
				                PCCORE_STANDARD_MULTIPLE);
			} else if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				ImGui::Text("Effective SGP scale: x%d relative to Model default",
				            g_gui.pending_sgp_multiplier);
			}
			if (g_gui.pending_sgp_mode == SGP_SPEED_FOLLOW_CPU) {
				effective_scale =
				    static_cast<double>(g_gui.pending_cpu_multiplier) / PCCORE_STANDARD_MULTIPLE;
			} else if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				effective_scale = g_gui.pending_sgp_multiplier;
			}
			ImGui::Text("Effective SGP clock: %.4f MHz", model_clock_mhz * effective_scale);
		}
		ImGui::EndChild();
		if (ImGui::BeginChild("pacing-config", ImVec2(0.0f, 0.0f), section,
		                      ImGuiWindowFlags_NoScrollbar)) {
			// M116: host pacing (PacingMs): the shortest time between two
			// guest frames; 0 leaves the speed to the normal timing.
			ImGui::TextUnformatted("ホストの待ち時間");
			ImGui::Separator();
			ImGui::TextUnformatted("1 フレームごとの最小間隔 (ms)");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(unit * 9.0f);
			ImGui::InputInt("##pacing-ms", &g_gui.pending_pacing_ms, 1, 8);
			if (g_gui.pending_pacing_ms < 0)
				g_gui.pending_pacing_ms = 0;
			if (g_gui.pending_pacing_ms > VAEG_PACING_MS_MAX) {
				g_gui.pending_pacing_ms = VAEG_PACING_MS_MAX;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::TextWrapped("0 で無効（ふつうは 0）。大きくするとホストの負荷は下がり、"
			                   "エミュレーションは遅くなります。");
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();

		if (ImGui::BeginChild("hostfat-config", ImVec2(0.0f, 0.0f), section,
		                      ImGuiWindowFlags_NoScrollbar)) {
			ImGui::TextUnformatted("HOSTFAT read-only host folder");
			ImGui::Separator();
			ImGui::Checkbox("Enable HOSTFAT", &g_gui.pending_hostfat_enabled);
			const float browse = unit * 6.0f;
			ImGui::SetNextItemWidth(-(browse + style.ItemSpacing.x));
			ImGui::InputText("##hostfat-dir", g_gui.pending_hostfat_dir,
			                 sizeof(g_gui.pending_hostfat_dir));
			ImGui::SameLine();
			if (ImGui::Button("Browse...", ImVec2(browse, 0.0f))) {
				open_hostfat_browser();
			}
			if (ImGui::Button("Rebuild + reset on OK")) {
				g_gui.pending_hostfat_rebuild = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("FAT12 max: 63.72 MiB usable");
			if (hostfat_action_error) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
				ImGui::TextWrapped("%s", g_gui.hostfat_status.c_str());
				ImGui::PopStyleColor();
			}
			if (hostfat_manager_status.state == HOSTFAT_MANAGER_BUILDING) {
				const float fraction =
				    (hostfat_manager_status.total != 0)
				        ? static_cast<float>(static_cast<double>(hostfat_manager_status.completed) /
				                             static_cast<double>(hostfat_manager_status.total))
				        : 0.0f;
				ImGui::ProgressBar(std::clamp(fraction, 0.0f, 1.0f), ImVec2(-1.0f, 0.0f),
				                   hostfat_manager_status.phase);
			} else if (!hostfat_action_error && hostfat_manager_status.message[0] != '\0') {
				if (hostfat_manager_status.state == HOSTFAT_MANAGER_ERROR) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
					ImGui::TextWrapped("Error: %s", hostfat_manager_status.message);
					ImGui::PopStyleColor();
				} else {
					ImGui::TextWrapped("%s", hostfat_manager_status.message);
				}
			} else {
				ImGui::TextDisabled("%s", hostfat_manager_status.phase);
			}
		}
		ImGui::EndChild();
		ImGui::EndChild();

		if (!cpu_valid) {
			ImGui::TextUnformatted("Multiplier must be between 1 and 32.");
		} else if (!sgp_mode_valid) {
			ImGui::TextUnformatted("Select a valid SGP speed mode.");
		} else if (!sgp_multiplier_valid) {
			ImGui::TextUnformatted("SGP multiplier must be between 1 and 16.");
		} else if (!ndp8087_clock_valid) {
			ImGui::TextUnformatted("8087 clock must be between 1000000 and 20000000 Hz.");
		} else if (!hostfat_valid) {
			ImGui::TextUnformatted("Select an existing HOSTFAT directory.");
		} else if (!hostfat_idle) {
			ImGui::TextUnformatted("Wait for the current HOSTFAT build to finish.");
		}

		const float button_width = unit * 5.5f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x -
		                     (button_width * 2.0f + style.ItemSpacing.x));
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
		draw_hostfat_browser_popup();
		ImGui::EndPopup();
	}
}

static void open_bms_config_dialog(void) {
	g_gui.pending_bms_enabled = bmsiocfg.enabled != FALSE;
	g_gui.pending_bms_port = (bmsiocfg.port == BMSIO_PORT_COMPAT) ? 1 : 0;
	g_gui.pending_bms_banks = bmsiocfg.numbanks;
	g_gui.bms_config_open = true;
	g_gui.bms_config_request = true;
}

static void apply_bms_config_dialog(void) {
	const UINT16 port = (g_gui.pending_bms_port == 1) ? BMSIO_PORT_COMPAT : BMSIO_PORT_DEFAULT;
	const BOOL enabled = g_gui.pending_bms_enabled ? TRUE : FALSE;
	const UINT8 banks = static_cast<UINT8>(g_gui.pending_bms_banks);
	const bool changed = (bmsiocfg.enabled != enabled) || (bmsiocfg.port != port) ||
	                     (bmsiocfg.numbanks != banks) || (bmsiocfg.portmask != BMSIO_PORT_MASK);

	if (changed) {
		bmsiocfg.enabled = enabled;
		bmsiocfg.port = port;
		bmsiocfg.portmask = BMSIO_PORT_MASK;
		bmsiocfg.numbanks = banks;
		sysmng_update(SYS_UPDATECFG);
		reset_guest();
	}
	g_gui.bms_config_open = false;
	ImGui::CloseCurrentPopup();
}

static void draw_bms_config_dialog(void) {
	if (g_gui.bms_config_request) {
		ImGui::OpenPopup("I/O Bank Memory##bms-config");
		g_gui.bms_config_request = false;
	}
	if (!g_gui.bms_config_open) {
		return;
	}
	prepare_dialog(26.875f, 17.8125f, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("I/O Bank Memory##bms-config", &g_gui.bms_config_open,
	                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		static const char *ports[] = {"01D0H (PC-88VA-01/02 mode)", "00ECH (PC-9801 mode)"};
		const bool banks_valid =
		    (g_gui.pending_bms_banks >= 1) && (g_gui.pending_bms_banks <= BMSIO_MAX_BANKS);
		const bool port_valid = (g_gui.pending_bms_port >= 0) &&
		                        (g_gui.pending_bms_port < static_cast<int>(std::size(ports)));

		ImGui::Checkbox("Use I/O Bank Memory", &g_gui.pending_bms_enabled);
		ImGui::SetNextItemWidth(220.0f);
		ImGui::Combo("I/O port", &g_gui.pending_bms_port, ports,
		             static_cast<int>(std::size(ports)));
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputInt("128KB banks", &g_gui.pending_bms_banks, 1, 16);
		if (banks_valid) {
			ImGui::Text("Capacity: %dKB (%.3fMiB)",
			            g_gui.pending_bms_banks * (BMSIO_BANK_BYTES / 1024),
			            static_cast<double>(g_gui.pending_bms_banks) / 8.0);
		} else {
			ImGui::TextUnformatted("Bank count must be between 1 and 255.");
		}
		ImGui::Separator();
		ImGui::TextWrapped("Selector 0 restores main RAM at 80000H-9FFFFH; "
		                   "selectors 1 through the configured bank count choose BMS storage.");
		ImGui::TextWrapped("Applying a change resets the guest. Disabling BMS or "
		                   "changing its bank count discards current BMS contents.");

		const float button_width = 88.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - (button_width * 2.0f + 8.0f));
		ImGui::BeginDisabled(!banks_valid || !port_valid);
		if (ImGui::Button("OK", ImVec2(button_width, 0.0f))) {
			apply_bms_config_dialog();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.bms_config_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

/* With this on, the BIOS boot-disk setting (B1FC4h bit 7) follows whether a
 * hard disk is attached, at the next reset; a floppy in drive 1 still boots
 * first. */
static void save_boot_fixed_disk(bool enabled) {
	const UINT8 previous = np2cfg.boot_fixed_disk;
	np2cfg.boot_fixed_disk = enabled ? 1 : 0;
	if (initsave_checked() != SUCCESS) {
		np2cfg.boot_fixed_disk = previous;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Configuration not saved",
		                         "Could not save the active configuration file. Check write "
		                         "permissions and whether --no-cfg is enabled. Settings were not "
		                         "changed.",
		                         nullptr);
		return;
	}
	sysmng_update(SYS_UPDATECFG);
}

static void save_main_ram_settings(UINT16 capacity, bool automatic) {
	const UINT16 previous_capacity = np2cfg.main_ram;
	const UINT8 previous_auto = np2cfg.main_ram_auto;
	np2cfg.main_ram = capacity;
	np2cfg.main_ram_auto = automatic ? 1 : 0;
	if (initsave_checked() != SUCCESS) {
		np2cfg.main_ram = previous_capacity;
		np2cfg.main_ram_auto = previous_auto;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Configuration not saved",
		                        "Could not save the active configuration file. Check write "
		                        "permissions and whether --no-cfg is enabled. Settings were not changed.",
		                        nullptr);
		return;
	}
	sysmng_update(SYS_UPDATECFG);
}

static void open_ems_config_dialog(void) {
	g_gui.pending_ems_enabled = np2cfg.EXTMEM != 0;
	g_gui.pending_ems_megabytes = (np2cfg.EXTMEM != 0) ? np2cfg.EXTMEM : EMSIO_DEFAULT_MEGABYTES;
	g_gui.ems_config_open = true;
	g_gui.ems_config_request = true;
}

static void apply_ems_config_dialog(void) {
	const UINT8 megabytes =
	    g_gui.pending_ems_enabled ? static_cast<UINT8>(g_gui.pending_ems_megabytes) : 0;
	const bool changed = np2cfg.EXTMEM != megabytes;

	if (changed) {
		np2cfg.EXTMEM = megabytes;
		sysmng_update(SYS_UPDATECFG);
		reset_guest();
	}
	g_gui.ems_config_open = false;
	ImGui::CloseCurrentPopup();
}

static void draw_ems_config_dialog(void) {
	if (g_gui.ems_config_request) {
		ImGui::OpenPopup("EMS Board##ems-config");
		g_gui.ems_config_request = false;
	}
	if (!g_gui.ems_config_open) {
		return;
	}
	prepare_dialog(26.875f, 15.625f, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("EMS Board##ems-config", &g_gui.ems_config_open,
	                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		const bool capacity_valid =
		    !g_gui.pending_ems_enabled || ((g_gui.pending_ems_megabytes >= EMSIO_MIN_MEGABYTES) &&
		                                   (g_gui.pending_ems_megabytes <= EMSIO_MAX_MEGABYTES));

		ImGui::Checkbox("Use EMS Board", &g_gui.pending_ems_enabled);
		ImGui::BeginDisabled(!g_gui.pending_ems_enabled);
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputInt("Installed memory (MB)", &g_gui.pending_ems_megabytes, 1, 1);
		ImGui::EndDisabled();
		if (capacity_valid && g_gui.pending_ems_enabled) {
			ImGui::Text("Capacity: %dMB", g_gui.pending_ems_megabytes);
		} else if (!capacity_valid) {
			ImGui::Text("Capacity must be between %dMB and %dMB.", EMSIO_MIN_MEGABYTES,
			            EMSIO_MAX_MEGABYTES);
		} else {
			ImGui::TextUnformatted("Capacity: disabled");
		}
		ImGui::Separator();
		ImGui::TextWrapped("Applying a change resets the guest and discards "
		                   "current EMS contents. EMMVA also requires a compatible guest EMM "
		                   "manager.");

		const float button_width = 88.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - (button_width * 2.0f + 8.0f));
		ImGui::BeginDisabled(!capacity_valid);
		if (ImGui::Button("OK", ImVec2(button_width, 0.0f))) {
			apply_ems_config_dialog();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.ems_config_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static UINT8 scale_master_volume(int volume, int max_value) {
	volume = std::clamp(volume, 0, kMasterVolumeMax);
	return static_cast<UINT8>((volume * max_value + kMasterVolumeMax / 2) / kMasterVolumeMax);
}

/* 個別の音量 (M110). 音源 sets FM, SSG, ADPCM and rhythm together: their
 * ratio is fixed by the chip and the board on hardware. */
static void apply_source_volume(int volume) {
	const UINT8 mixer_volume = scale_master_volume(volume, 128);

	np2cfg.vol_fm = mixer_volume;
	np2cfg.vol_ssg = mixer_volume;
	np2cfg.vol_adpcm = mixer_volume;
	np2cfg.vol_rhythm = mixer_volume;
	opngen_setvol(np2cfg.vol_fm);
	psggen_setvol(np2cfg.vol_ssg);
	rhythm_setvol(np2cfg.vol_rhythm);
	rhythm_update(&rhythm);
	adpcm_setvol(np2cfg.vol_adpcm);
	adpcm_update(&adpcm);
	sysmng_update(SYS_UPDATECFG);
}

static int beep_volume(void) {
	if (np2cfg.beep_level <= kMasterVolumeMax) {
		return np2cfg.beep_level;
	}
	return (std::min)(np2cfg.BEEP_VOL * 43, kMasterVolumeMax);
}

static void apply_beep_volume(int volume) {
	// BEEP_vol (0-3) keeps a nonzero value unless the level is 0, for
	// older builds.
	np2cfg.beep_level = static_cast<UINT8>(std::clamp(volume, 0, kMasterVolumeMax));
	np2cfg.BEEP_VOL =
	    static_cast<UINT8>((np2cfg.beep_level * 3 + kMasterVolumeMax - 1) / kMasterVolumeMax);
	beep_setlevel(np2cfg.beep_level);
	sysmng_update(SYS_UPDATECFG);
}

static void draw_source_volumes(void) {
	const float width = ImGui::GetFontSize() * 10.0f;
	int value;

	value = np2cfg.vol_fm;
	ImGui::SetNextItemWidth(width);
	if (ImGui::SliderInt("音源 (FM/SSG/ADPCM/リズム)", &value, 0, kMasterVolumeMax)) {
		apply_source_volume(value);
	}
	value = beep_volume();
	ImGui::SetNextItemWidth(width);
	if (ImGui::SliderInt("BEEP", &value, 0, kMasterVolumeMax)) {
		apply_beep_volume(value);
	}
	value = np2cfg.cmt_vol;
	ImGui::SetNextItemWidth(width);
	if (ImGui::SliderInt("テープ", &value, 0, 128)) {
		np2cfg.cmt_vol = static_cast<UINT8>(value);
		cmt_setvol(np2cfg.cmt_vol);
		sysmng_update(SYS_UPDATECFG);
	}
	value = np2cfg.pcg_vol;
	ImGui::SetNextItemWidth(width);
	if (ImGui::SliderInt("PCG", &value, 0, 128)) {
		np2cfg.pcg_vol = static_cast<UINT8>(value);
		pcg88_setvol(np2cfg.pcg_vol);
		sysmng_update(SYS_UPDATECFG);
	}
	value = np2cfg.MOTORVOL;
	ImGui::SetNextItemWidth(width);
	if (ImGui::SliderInt("シーク音", &value, 0, 100)) {
		np2cfg.MOTORVOL = static_cast<UINT8>(value);
		fddmtrsnd_volume(np2cfg.MOTORVOL);
		sysmng_update(SYS_UPDATECFG);
	}
}

static void select_opn_backend(UINT backend) {
	if (opngen_getbackend() == backend) {
		return;
	}
	opngen_setbackend(backend);
	milstr_ncpy(np2oscfg.opn_backend, opngen_backendname(backend), sizeof(np2oscfg.opn_backend));
	soundrenewal = 1;
	sysmng_update(SYS_UPDATEOSCFG);
	reset_guest();
}

static void select_ymfm_fidelity(UINT fidelity) {
	if ((fidelity >= YMFMBRIDGE_FIDELITY_COUNT) || (ymfm_opn_getfidelity() == fidelity)) {
		return;
	}
	soundmng_stop();
	ymfm_opn_setfidelity(fidelity);
	milstr_ncpy(np2oscfg.ymfm_fidelity, ymfm_opn_fidelityname(fidelity),
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

/*
 * 模擬 > 起動: select the machine and its boot mode in one step and reset
 * (M111). 自動 lets the ROM decide from the boot disk (a V3 disk boots V3,
 * otherwise V1/V2 as last chosen); V2/V1 H/S skip the V3 search and start
 * V1/V2; N starts N-BASIC. V1/V2 is the VA's memory switch B1FC5h bit 0, as
 * its setup menu stores it; H/S and N are vaeg extensions (memctrlva_i031).
 */
static void select_boot(const char *model, int boot_mode, bool v1, bool standard,
                        const char *n80rom, const char *extra_machine = nullptr) {
	const UINT16 old_sound = np2cfg.SOUND_SW;

	np2_select_boot_model(model);
	if (np2cfg.SOUND_SW != old_sound) {
		soundrenewal = 1;
	}
	milstr_ncpy(np2cfg.extra_machine, (extra_machine != nullptr) ? extra_machine : "",
	            sizeof(np2cfg.extra_machine));
	np2cfg.boot_mode = static_cast<UINT8>(boot_mode);
	if (boot_mode == BOOTMODE_N) {
		if (n80rom != nullptr) {
			milstr_ncpy(np2cfg.v1v2_n80rom, n80rom, sizeof(np2cfg.v1v2_n80rom));
		}
	} else if (boot_mode == BOOTMODE_V1V2) {
		bkupmemva_set_88v1(v1 ? TRUE : FALSE);
		np2cfg.v1v2_standard = standard ? 1 : 0;
	}
	sysmng_update(SYS_UPDATECFG);
	reset_guest();
}

/* M113: start a PC-8801 model from extra/<name>/ in the VA's V1/V2 mode
 * (V2/V1 with H or S, or N), on the VA model its profile names. */
static void select_extra_machine(const char *name, int boot_mode, bool v1, bool standard) {
	EXTRAROM_PROFILE profile;

	if (!extrarom_profile(name, &profile) || !profile.startable) {
		return;
	}
	select_boot(profile.base_model, boot_mode, v1, standard, nullptr, name);
}

static void select_monitor(bool khz15) {
	if ((np2cfg.monitor_15khz != 0) == khz15) {
		return;
	}
	np2cfg.monitor_15khz = khz15 ? 1 : 0;
	sysmng_update(SYS_UPDATECFG);
	reset_guest();
}

/* N-BASIC ROM files found when the 起動 menu was last opened. */
static N80ROMENTRY g_n80_entries[ROMVA_N80_NAMES];
static bool g_n80_scanned = false;

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
	return static_cast<int>(std::ceil(g_gui.menu_font_size + (style.FramePadding.y * 2.0f)));
}

static std::string home_dir(void) {
	const char *home;

#if defined(WIN32)
	home = std::getenv("USERPROFILE");
	if ((home == nullptr) || (home[0] == '\0')) {
		home = std::getenv("HOME");
	}
	if ((home == nullptr) || (home[0] == '\0')) {
		const char *drive = std::getenv("HOMEDRIVE");
		const char *path = std::getenv("HOMEPATH");
		if ((drive != nullptr) && (drive[0] != '\0') && (path != nullptr) && (path[0] != '\0')) {
			return std::string(drive) + path;
		}
	}
#else
	home = std::getenv("HOME");
#endif
	if ((home != nullptr) && (home[0] != '\0')) {
		return home;
	}
	return ".";
}

static bool is_directory(const std::string &path) {
	std::error_code ec;
	return fs::is_directory(vaeg_hostfat::path_from_utf8(path), ec);
}

static std::string absolute_path(const std::string &path) {
	std::error_code ec;
	fs::path abs = fs::absolute(vaeg_hostfat::path_from_utf8(path), ec);
	if (ec) {
		return path;
	}
	return abs.u8string();
}

static std::string parent_dir(const std::string &path) {
	std::error_code ec;
	fs::path p = fs::absolute(vaeg_hostfat::path_from_utf8(path), ec);
	if (ec) {
		p = vaeg_hostfat::path_from_utf8(path);
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

static std::vector<std::string> host_drive_roots(void) {
	std::vector<std::string> roots;

#if defined(_WIN32)
	for (char letter = 'A'; letter <= 'Z'; letter++) {
		std::string root;

		root += letter;
		root += ":\\";
		if (is_directory(root)) {
			roots.push_back(root);
		}
	}
#endif
	return roots;
}

static bool drive_root_matches(const std::string &path, const std::string &root) {
	if ((path.size() < 2) || (root.size() < 2) || (path[1] != ':') || (root[1] != ':')) {
		return false;
	}
	return static_cast<char>(std::toupper(static_cast<unsigned char>(path[0]))) ==
	       static_cast<char>(std::toupper(static_cast<unsigned char>(root[0])));
}

static bool draw_host_drive_selector(std::string &directory, bool &refresh, const char *id) {
	const std::vector<std::string> roots = host_drive_roots();
	std::string preview = "Current";
	bool changed = false;

	if (roots.empty()) {
		return false;
	}
	for (const std::string &root : roots) {
		if (drive_root_matches(directory, root)) {
			preview = root.substr(0, 2);
			break;
		}
	}
	std::string combo_id = "Drive##";
	combo_id += id;
	ImGui::SetNextItemWidth(110.0f);
	if (ImGui::BeginCombo(combo_id.c_str(), preview.c_str())) {
		for (const std::string &root : roots) {
			const std::string label = root.substr(0, 2);
			const bool selected = (label == preview);

			if (ImGui::Selectable(label.c_str(), selected)) {
				directory = absolute_path(root);
				refresh = true;
				changed = true;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Select a host drive");
	}
	return changed;
}

static bool browser_entry_less(const BrowserEntry &a, const BrowserEntry &b) {
	if (a.is_dir != b.is_dir) {
		return a.is_dir > b.is_dir;
	}
	return a.name < b.name;
}

/* File types of the FDD and tape browsers (M106, maintainer request): the
 * usual image and archive types by default, or every file, as in a Windows
 * "Files of type" list. */
static const char *const kFddFileTypes[] = {".d88", ".88d", ".zip", ".7z", ".lzh"};
static const char *const kTapeFileTypes[] = {".cmt", ".t88", ".zip", ".7z", ".lzh"};
static const char kFddFileTypeLabel[] = "Disk images (*.d88;*.88d;*.zip;*.7z;*.lzh)";
static const char kTapeFileTypeLabel[] = "Tape images (*.cmt;*.t88;*.zip;*.7z;*.lzh)";
/* The SASI browser lists .hdi images by default (maintainer request); the
 * SCSI browser keeps listing every file. */
static const char *const kSasiFileTypes[] = {".hdi"};
static const char kSasiFileTypeLabel[] = "SASI HDD images (*.hdi)";

static bool file_type_listed(const std::string &name, const char *const *types, size_t count) {
	std::string extension = fs::u8path(name).extension().u8string();

	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	for (size_t i = 0; i < count; i++) {
		if (extension == types[i]) {
			return true;
		}
	}
	return false;
}

/* A "Files of type" combo; returns true when the choice changed. */
static bool draw_file_type_combo(const char *id, const char *label, bool *all_files) {
	bool changed = false;

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo(id, *all_files ? "All files (*.*)" : label)) {
		if (ImGui::Selectable(label, !*all_files) && *all_files) {
			*all_files = false;
			changed = true;
		}
		if (ImGui::Selectable("All files (*.*)", *all_files) && !*all_files) {
			*all_files = true;
			changed = true;
		}
		ImGui::EndCombo();
	}
	return changed;
}

static void refresh_fdd_browser(void) {
	std::error_code ec;

	g_gui.fdd_entries.clear();
	if (!is_directory(g_gui.fdd_browser_dir)) {
		g_gui.fdd_browser_dir = home_dir();
	}
	for (const auto &entry : fs::directory_iterator(fs::u8path(g_gui.fdd_browser_dir), ec)) {
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
		if (!item.is_dir && !g_gui.fdd_all_files &&
		    !file_type_listed(item.name, kFddFileTypes, std::size(kFddFileTypes))) {
			continue;
		}
		g_gui.fdd_entries.push_back(item);
	}
	std::sort(g_gui.fdd_entries.begin(), g_gui.fdd_entries.end(), browser_entry_less);
	g_gui.fdd_browser_refresh = false;
}

static bool hdd_is_scsi(int drive);

static void refresh_hdd_browser(void) {
	std::error_code ec;

	g_gui.hdd_entries.clear();
	if (!is_directory(g_gui.hdd_browser_dir)) {
		g_gui.hdd_browser_dir = home_dir();
	}
	for (const auto &entry : fs::directory_iterator(fs::u8path(g_gui.hdd_browser_dir), ec)) {
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
		if (!item.is_dir && !g_gui.hdd_all_files && !hdd_is_scsi(g_gui.hdd_dialog_drive) &&
		    !file_type_listed(item.name, kSasiFileTypes, std::size(kSasiFileTypes))) {
			continue;
		}
		g_gui.hdd_entries.push_back(item);
	}
	std::sort(g_gui.hdd_entries.begin(), g_gui.hdd_entries.end(), browser_entry_less);
	g_gui.hdd_browser_refresh = false;
}

static void refresh_hostfat_browser(void) {
	std::error_code ec;
	g_gui.hostfat_entries.clear();
	if (!is_directory(g_gui.hostfat_browser_dir)) {
		g_gui.hostfat_browser_dir = home_dir();
	}
	for (const auto &entry :
	     fs::directory_iterator(vaeg_hostfat::path_from_utf8(g_gui.hostfat_browser_dir), ec)) {
		if (ec) {
			break;
		}
		std::error_code status_error;
		const bool is_dir = entry.is_directory(status_error);
		if (status_error || !is_dir) {
			continue;
		}
		BrowserEntry item;
		item.is_dir = true;
		item.name = entry.path().filename().u8string();
		item.path = entry.path().u8string();
		if (item.name.empty() || (item.name[0] == '.')) {
			continue;
		}
		g_gui.hostfat_entries.push_back(std::move(item));
	}
	std::sort(g_gui.hostfat_entries.begin(), g_gui.hostfat_entries.end(), browser_entry_less);
	g_gui.hostfat_browser_refresh = false;
}

static void open_hostfat_browser(void) {
	std::string start = g_gui.pending_hostfat_dir;
	if (!is_directory(start)) {
		start = home_dir();
	}
	g_gui.hostfat_browser_dir = absolute_path(start);
	g_gui.hostfat_browser_open = true;
	g_gui.hostfat_browser_request = true;
	g_gui.hostfat_browser_refresh = true;
}

static void draw_hostfat_browser_popup(void) {
	if (g_gui.hostfat_browser_request) {
		ImGui::OpenPopup("Select HOSTFAT folder##hostfat-browser");
		g_gui.hostfat_browser_request = false;
	}
	if (!g_gui.hostfat_browser_open) {
		return;
	}
	prepare_dialog(38.75f, 30.0f, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Select HOSTFAT folder##hostfat-browser",
	                           &g_gui.hostfat_browser_open, ImGuiWindowFlags_NoCollapse)) {
		if (g_gui.hostfat_browser_refresh) {
			refresh_hostfat_browser();
		}
		draw_host_drive_selector(g_gui.hostfat_browser_dir, g_gui.hostfat_browser_refresh,
		                         "hostfat");
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.hostfat_browser_dir.c_str());
		if (ImGui::Button("Up")) {
			g_gui.hostfat_browser_dir = parent_dir(g_gui.hostfat_browser_dir);
			g_gui.hostfat_browser_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Refresh")) {
			g_gui.hostfat_browser_refresh = true;
		}
		if (ImGui::BeginChild("hostfat-directory-list", ImVec2(0, browser_list_height(1.5f, "")),
		                      ImGuiChildFlags_Borders)) {
			for (const BrowserEntry &entry : g_gui.hostfat_entries) {
				const std::string label = "[D] " + entry.name;
				if (ImGui::Selectable(label.c_str())) {
					g_gui.hostfat_browser_dir = entry.path;
					g_gui.hostfat_browser_refresh = true;
				}
			}
		}
		ImGui::EndChild();
		if (ImGui::Button("Select this folder")) {
			copy_path(g_gui.pending_hostfat_dir, sizeof(g_gui.pending_hostfat_dir),
			          g_gui.hostfat_browser_dir);
			g_gui.hostfat_browser_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel##hostfat-folder") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.hostfat_browser_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
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
		*error = "HDD image path is empty.";
		return false;
	}
	attr = file_attr(path);
	if (attr == static_cast<short>(-1)) {
		*error = "HDD image not found.";
		return false;
	}
	if ((attr & FILEATTR_DIRECTORY) != 0) {
		*error = "HDD image path is a directory.";
		return false;
	}
	error->clear();
	return true;
}

static bool hdd_is_scsi(int drive) {
	return ((drive & 0x20) != 0);
}

static int hdd_slot(int drive) {
	return (drive & 0x0f);
}

static const char *hdd_config_path(int drive) {
	int slot;

	slot = hdd_slot(drive);
	return (hdd_is_scsi(drive) ? np2cfg.scsihdd[slot] : np2cfg.sasihdd[slot]);
}

static const char *hdd_interface_name(int drive) {
	return (hdd_is_scsi(drive) ? "SCSI ID " : "SASI-");
}

static std::string display_file_name(const char *path) {
	if ((path == nullptr) || (path[0] == '\0')) {
		return std::string();
	}
	std::string name = fs::u8path(path).filename().u8string();
	const std::size_t separator = name.find_last_of("/\\");
	if (separator != std::string::npos) {
		name.erase(0, separator + 1);
	}
	return name;
}

// ---- History (M110): media per drive, recent boots, recent folders.
// Kept in vaeg-history.txt in the user directory, one "key<TAB>value" per line.
// Only media inserted through the GUI (menus, drag and drop) are recorded.

enum HistoryKind {
	HIST_FDD1 = GUI_HISTORY_FDD1,
	HIST_FDD2 = GUI_HISTORY_FDD2,
	HIST_SASI1,
	HIST_SASI2,
	HIST_SCSI0,
	HIST_TAPE = HIST_SCSI0 + SCSIHDD_MAX,
	/* M122: recent folders of the FDD, HDD and tape file windows */
	HIST_DIR_FDD,
	HIST_DIR_HDD,
	HIST_DIR_TAPE,
	HIST_COUNT
};

struct BootRecord {
	std::string model;
	bool v1 = false;
	bool standard = false;
	int mode = BOOTMODE_AUTO;
	std::string n80rom;
	std::string fdd[2];
	std::string sasi[2];
	std::string tape;
	std::string extra; /* M113: PC-8801 model directory, empty for the VA */
	UINT fdd_image[2] = {0, 0}; /* M120: disk of a multi-image D88 */
	/* M124: the monitor (1 = 15 kHz) and screen rotation in use; -1 for a
	 * record written before M124, which leaves them as they are */
	int monitor_15khz = -1;
	int rotate = -1;
	/* the display at the end of the session (updated when it ends): 描画方式
	 * (0 SDL, 1 CRT, 2 librashader unfiltered), 拡大方法, window scale and
	 * window size without the front panel; -1 when unknown */
	int renderer = -1;
	int scaling = -1;
	int scale = -1;
	int window_width = -1;
	int window_height = -1;

	bool operator==(const BootRecord &other) const {
		return (model == other.model) && (v1 == other.v1) && (standard == other.standard) &&
		       (mode == other.mode) && (n80rom == other.n80rom) && (fdd[0] == other.fdd[0]) &&
		       (fdd[1] == other.fdd[1]) && (sasi[0] == other.sasi[0]) &&
		       (sasi[1] == other.sasi[1]) && (tape == other.tape) && (extra == other.extra) &&
		       (fdd_image[0] == other.fdd_image[0]) && (fdd_image[1] == other.fdd_image[1]) &&
		       (monitor_15khz == other.monitor_15khz);
	}
};

constexpr std::size_t kRecentBoots = 10;

static std::vector<std::string> g_history[HIST_COUNT];
static std::vector<BootRecord> g_recent_boots;
static bool g_history_loaded = false;
/* The path the user chose for each FDD (an archive, not its extracted
 * image), for recent boots. */
static std::string g_fdd_origin[2];

static std::size_t history_limit(int kind) {
	if ((kind == HIST_FDD1) || (kind == HIST_FDD2) || (kind == HIST_TAPE)) {
		return 10;
	}
	return 5;
}

static std::string history_key(int kind) {
	if (kind == HIST_FDD1) {
		return "fdd1";
	}
	if (kind == HIST_FDD2) {
		return "fdd2";
	}
	if ((kind == HIST_SASI1) || (kind == HIST_SASI2)) {
		return "sasi" + std::to_string(kind - HIST_SASI1 + 1);
	}
	if ((kind >= HIST_SCSI0) && (kind < HIST_TAPE)) {
		return "scsi" + std::to_string(kind - HIST_SCSI0);
	}
	if (kind == HIST_TAPE) {
		return "tape";
	}
	if (kind == HIST_DIR_FDD) {
		return "dirfdd";
	}
	return (kind == HIST_DIR_HDD) ? "dirhdd" : "dirtape";
}

static std::string history_file(void) {
	char path[MAX_PATH];

	file_getstatepath(path, sizeof(path), "vaeg-history.txt");
	return path;
}

static std::vector<std::string> history_split(const std::string &line) {
	std::vector<std::string> fields;
	std::size_t start = 0;

	for (;;) {
		const std::size_t tab = line.find('\t', start);
		fields.push_back(line.substr(start, tab - start));
		if (tab == std::string::npos) {
			return fields;
		}
		start = tab + 1;
	}
}

static void history_save(void) {
	std::ofstream output(fs::u8path(history_file()), std::ios::binary | std::ios::trunc);

	for (int kind = 0; kind < HIST_COUNT; kind++) {
		for (const auto &path : g_history[kind]) {
			output << history_key(kind) << '\t' << path << '\n';
		}
	}
	for (const auto &boot : g_recent_boots) {
		output << "boot\t" << boot.model << '\t' << (boot.v1 ? 1 : 0) << '\t'
		       << (boot.standard ? 1 : 0) << '\t' << boot.mode << '\t' << boot.n80rom << '\t'
		       << boot.fdd[0] << '\t' << boot.fdd[1] << '\t' << boot.sasi[0] << '\t' << boot.sasi[1]
		       << '\t' << boot.tape << '\t' << boot.extra << '\t' << boot.fdd_image[0] << ','
		       << boot.fdd_image[1] << '\t' << boot.monitor_15khz << ',' << boot.rotate << ','
		       << boot.renderer << ',' << boot.scaling << ',' << boot.scale << ','
		       << boot.window_width << ',' << boot.window_height << '\n';
	}
}

/* M122: the folders of the shared "dir" list (M110-M120) go to the FDD,
 * HDD and tape lists whose files were in them; a folder of none of them
 * goes to the FDD list. Written back in the new form at the next save. */
static void history_split_legacy_dirs(const std::vector<std::string> &dirs) {
	static const struct {
		int first;
		int last;
		int dir_kind;
	} groups[] = {{HIST_FDD1, HIST_FDD2, HIST_DIR_FDD},
	              {HIST_SASI1, HIST_TAPE - 1, HIST_DIR_HDD},
	              {HIST_TAPE, HIST_TAPE, HIST_DIR_TAPE}};

	for (const std::string &dir : dirs) {
		bool placed = false;
		for (const auto &group : groups) {
			bool used = false;
			for (int kind = group.first; !used && (kind <= group.last); kind++) {
				for (const std::string &file : g_history[kind]) {
					if (parent_dir(file) == dir) {
						used = true;
						break;
					}
				}
			}
			std::vector<std::string> &list = g_history[group.dir_kind];
			if (used && (list.size() < history_limit(group.dir_kind)) &&
			    (std::find(list.begin(), list.end(), dir) == list.end())) {
				list.push_back(dir);
			}
			placed = placed || used;
		}
		std::vector<std::string> &fdd = g_history[HIST_DIR_FDD];
		if (!placed && (fdd.size() < history_limit(HIST_DIR_FDD)) &&
		    (std::find(fdd.begin(), fdd.end(), dir) == fdd.end())) {
			fdd.push_back(dir);
		}
	}
}

static void history_load(void) {
	if (g_history_loaded) {
		return;
	}
	g_history_loaded = true;
	std::ifstream input(fs::u8path(history_file()), std::ios::binary);
	std::string line;
	std::vector<std::string> legacy_dirs;
	while (std::getline(input, line)) {
		if (!line.empty() && (line.back() == '\r')) {
			line.pop_back();
		}
		const std::vector<std::string> fields = history_split(line);
		if ((fields.size() >= 11) && (fields.size() <= 14) && (fields[0] == "boot")) {
			BootRecord boot;
			boot.model = fields[1];
			boot.v1 = fields[2] == "1";
			boot.standard = fields[3] == "1";
			boot.mode = std::atoi(fields[4].c_str());
			if ((boot.mode < BOOTMODE_AUTO) || (boot.mode > BOOTMODE_N)) {
				boot.mode = BOOTMODE_AUTO;
			}
			boot.n80rom = fields[5];
			boot.fdd[0] = fields[6];
			boot.fdd[1] = fields[7];
			boot.sasi[0] = fields[8];
			boot.sasi[1] = fields[9];
			boot.tape = fields[10];
			if (fields.size() >= 12) {
				boot.extra = fields[11];
			}
			if (fields.size() >= 13) {
				// M120: "first,second" disks of multi-image D88 files
				unsigned first = 0;
				unsigned second = 0;
				if (std::sscanf(fields[12].c_str(), "%u,%u", &first, &second) == 2) {
					boot.fdd_image[0] = (first < 255u) ? first : 0u;
					boot.fdd_image[1] = (second < 255u) ? second : 0u;
				}
			}
			if (fields.size() == 14) {
				// M124: "monitor,rotation[,renderer,scaling,scale,width,height]"
				int v[7] = {-1, -1, -1, -1, -1, -1, -1};
				const int n = std::sscanf(fields[13].c_str(), "%d,%d,%d,%d,%d,%d,%d", &v[0], &v[1],
				                          &v[2], &v[3], &v[4], &v[5], &v[6]);
				if ((n >= 2) && (v[0] >= 0) && (v[0] <= 1) && (v[1] >= 0) &&
				    (v[1] < VAEG_ROTATE_COUNT)) {
					boot.monitor_15khz = v[0];
					boot.rotate = v[1];
				}
				if ((n == 7) && (v[2] >= 0) && (v[2] <= 2) && (v[3] >= 0) &&
				    (v[3] < VAEG_SCALING_COUNT) && (v[4] >= 0) && (v[4] <= 16) && (v[5] >= 320) &&
				    (v[5] <= 7680) && (v[6] >= 240) && (v[6] <= 4320)) {
					boot.renderer = v[2];
					boot.scaling = v[3];
					boot.scale = v[4];
					boot.window_width = v[5];
					boot.window_height = v[6];
				}
			}
			if (g_recent_boots.size() < kRecentBoots) {
				g_recent_boots.push_back(boot);
			}
			continue;
		}
		if ((fields.size() != 2) || fields[1].empty()) {
			continue;
		}
		if (fields[0] == "dir") {
			legacy_dirs.push_back(fields[1]);
			continue;
		}
		for (int kind = 0; kind < HIST_COUNT; kind++) {
			if ((fields[0] == history_key(kind)) &&
			    (g_history[kind].size() < history_limit(kind))) {
				g_history[kind].push_back(fields[1]);
			}
		}
	}
	history_split_legacy_dirs(legacy_dirs);
}

static void history_add(int kind, const std::string &path) {
	if ((kind < 0) || (kind >= HIST_COUNT) || path.empty() ||
	    (path.find_first_of("\t\r\n") != std::string::npos)) {
		return;
	}
	history_load();
	std::vector<std::string> &list = g_history[kind];
	list.erase(std::remove(list.begin(), list.end(), path), list.end());
	list.insert(list.begin(), path);
	if (list.size() > history_limit(kind)) {
		list.resize(history_limit(kind));
	}
	history_save();
}

/* Record the folder of `path` in a recent-folder list (HIST_DIR_*). */
static void history_add_dir(int dir_kind, const std::string &path) {
	const std::string dir = parent_dir(path);

	if (is_directory(dir)) {
		history_add(dir_kind, dir);
	}
}

/*
 * M122: the folder a file window opens in: the folder of the medium in use
 * (`current`), else the newest existing folder of its own list, else of the
 * FDD, HDD and tape lists in that order, else the home directory.
 */
static std::string history_start_dir(int dir_kind, const std::string &current) {
	static const int order[] = {HIST_DIR_FDD, HIST_DIR_HDD, HIST_DIR_TAPE};

	if (!current.empty() && is_directory(current)) {
		return current;
	}
	history_load();
	for (int pass = 0; pass < 2; pass++) {
		for (int kind : order) {
			if ((pass == 0) != (kind == dir_kind)) {
				continue;
			}
			for (const std::string &dir : g_history[kind]) {
				if (is_directory(dir)) {
					return dir;
				}
			}
		}
	}
	return home_dir();
}

/* Drag and drop (dropmedia.cpp) reports the file the user dropped. */
static void history_add_dropped(int kind, const char *path) {
	if ((path == nullptr) || (path[0] == '\0')) {
		return;
	}
	if ((kind == HIST_FDD1) || (kind == HIST_FDD2)) {
		g_fdd_origin[kind] = path;
	}
	history_add(kind, path);
	history_add_dir(HIST_DIR_FDD, path);
}

static int history_hdd_kind(int drive) {
	return hdd_is_scsi(drive) ? (HIST_SCSI0 + hdd_slot(drive)) : (HIST_SASI1 + hdd_slot(drive));
}

/* M116: the files of a recorded boot that no longer exist, one per line. */
static std::string history_missing_paths(const BootRecord &boot) {
	std::string missing;
	const std::string *paths[] = {&boot.fdd[0], &boot.fdd[1], &boot.sasi[0], &boot.sasi[1],
	                              &boot.tape};
	for (const std::string *path : paths) {
		std::error_code ec;
		if (!path->empty() && !fs::exists(fs::u8path(*path), ec)) {
			missing += (missing.empty() ? "" : "\n") + *path;
		}
	}
	return missing;
}

/* 最近の起動: the media, machine and mode at a reset from the GUI. */
/* M124: whether g_recent_boots[0] is the record of the running session. */
static bool g_session_recorded = false;

static void history_capture_display(BootRecord *boot) {
	boot->rotate = np2oscfg.gui_rotate;
	boot->renderer = !np2oscfg.gui_native_crt ? 0 : (np2oscfg.gui_native_filter ? 1 : 2);
	boot->scaling = np2oscfg.gui_scaling;
	boot->scale = np2oscfg.gui_scale;
	boot->window_width = np2oscfg.gui_window_width;
	boot->window_height = np2oscfg.gui_window_height;
}

/* M124: the display at the end of a session goes into its recent boot (at
 * the next reset, before a recent boot is restored, and at exit). */
static void history_note_session_display(void) {
	if (!g_session_recorded || g_recent_boots.empty()) {
		return;
	}
	BootRecord &boot = g_recent_boots.front();
	const BootRecord before = boot;
	history_capture_display(&boot);
	if ((boot.rotate != before.rotate) || (boot.renderer != before.renderer) ||
	    (boot.scaling != before.scaling) || (boot.scale != before.scale) ||
	    (boot.window_width != before.window_width) ||
	    (boot.window_height != before.window_height)) {
		history_save();
	}
}

static void history_record_boot(void) {
	BootRecord boot;

	history_load();
	history_note_session_display();
	g_session_recorded = false;
	boot.model = np2cfg.model;
	boot.v1 = bkupmemva_get_88v1() != FALSE;
	boot.standard = np2cfg.v1v2_standard != 0;
	boot.mode = np2cfg.boot_mode;
	boot.n80rom = (boot.mode == BOOTMODE_N) ? np2cfg.v1v2_n80rom : "";
	boot.extra = np2cfg.extra_machine;
	for (int drive = 0; drive < 2; drive++) {
		const char *mounted = fdd_diskname(static_cast<REG8>(drive));
		if ((mounted == nullptr) || (mounted[0] == '\0')) {
			mounted = diskdrv_fname[drive];
		}
		if ((mounted != nullptr) && (mounted[0] != '\0')) {
			char archive[MAX_PATH];
			// M116: an image extracted from an archive (also one restored
			// from the configuration at start-up) is recorded as the
			// archive, since the extracted copy is removed later.
			if (!g_fdd_origin[drive].empty()) {
				boot.fdd[drive] = g_fdd_origin[drive];
			} else if (dropmedia_source_archive(mounted, archive, sizeof(archive))) {
				boot.fdd[drive] = archive;
			} else {
				boot.fdd[drive] = mounted;
			}
			boot.fdd_image[drive] = (fdd_diskname(static_cast<REG8>(drive))[0] != '\0')
			                            ? fdd_diskimage(static_cast<REG8>(drive))
			                            : diskdrv_pendingimage(static_cast<REG8>(drive));
		}
		boot.sasi[drive] = np2cfg.sasihdd[drive];
	}
	boot.tape = cmt_inserted() ? g_gui.tape_loaded : "";
	boot.monitor_15khz = (np2cfg.monitor_15khz != 0) ? 1 : 0;
	history_capture_display(&boot);
	if (boot.fdd[0].empty() && boot.fdd[1].empty() && boot.sasi[0].empty() &&
	    boot.sasi[1].empty() && boot.tape.empty()) {
		return;
	}
	g_recent_boots.erase(std::remove(g_recent_boots.begin(), g_recent_boots.end(), boot),
	                     g_recent_boots.end());
	g_recent_boots.insert(g_recent_boots.begin(), boot);
	if (g_recent_boots.size() > kRecentBoots) {
		g_recent_boots.resize(kRecentBoots);
	}
	g_session_recorded = true;
	history_save();
}

static std::string history_boot_label(const BootRecord &boot) {
	std::string name;
	const std::string *media[] = {&boot.fdd[0], &boot.sasi[0], &boot.tape, &boot.fdd[1],
	                              &boot.sasi[1]};
	for (const std::string *path : media) {
		if (!path->empty()) {
			name = display_file_name(path->c_str());
			// M120: the disk of a multi-image D88 file in a drive
			if ((path == &boot.fdd[0]) && (boot.fdd_image[0] != 0)) {
				name += " #" + std::to_string(boot.fdd_image[0] + 1);
			} else if ((path == &boot.fdd[1]) && (boot.fdd_image[1] != 0)) {
				name += " #" + std::to_string(boot.fdd_image[1] + 1);
			}
			break;
		}
	}
	std::string mode = (boot.mode == BOOTMODE_N) ? "N" : (boot.v1 ? "V1" : "V2");
	if (boot.mode != BOOTMODE_N) {
		mode += boot.standard ? " S" : " H";
	}
	if (boot.mode == BOOTMODE_AUTO) {
		mode = "自動";
	}
	const std::string machine =
	    !boot.extra.empty() ? boot.extra
	                        : ((milstr_cmp(boot.model.c_str(), str_VA1) == 0) ? "VA" : "VA2/VA3");
	std::string display;
	if (boot.monitor_15khz >= 0) {
		display = (boot.monitor_15khz != 0) ? ", 15 kHz" : ", 24 kHz";
	}
	if (boot.rotate == VAEG_ROTATE_RIGHT) {
		display += ", 右 90°";
	}
	return name + "  (" + machine + ", " + mode + display + ")";
}

/* A list of recent files for one drive; `select` is called with the path. */
template <typename Select>
static void draw_history_menu(const char *label, int kind, Select select) {
	if (!ImGui::BeginMenu(label)) {
		return;
	}
	history_load();
	std::vector<std::string> &list = g_history[kind];
	if (list.empty()) {
		ImGui::TextDisabled("なし");
	}
	for (std::size_t index = 0; index < list.size(); index++) {
		const std::string path = list[index];
		std::error_code ec;
		const bool exists = fs::exists(fs::u8path(path), ec);
		const std::string item = display_file_name(path.c_str()) + "##" + std::to_string(index);
		if (ImGui::MenuItem(item.c_str(), nullptr, false, exists)) {
			select(path);
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("%s", path.c_str());
		}
	}
	ImGui::Separator();
	if (ImGui::MenuItem("履歴を消す", nullptr, false, !list.empty())) {
		list.clear();
		history_save();
	}
	ImGui::EndMenu();
}

/* A "最近のフォルダ" combo for the file-selection windows. */
static bool draw_recent_dirs(int dir_kind, std::string &dir) {
	bool changed = false;

	history_load();
	if (g_history[dir_kind].empty()) {
		return false;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
	if (ImGui::BeginCombo("##recent-dirs", "最近のフォルダ", ImGuiComboFlags_HeightLarge)) {
		for (const auto &path : g_history[dir_kind]) {
			if (ImGui::Selectable(path.c_str())) {
				dir = path;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}

static void set_fdd_status(int drive, const char *action, const char *path) {
	g_gui.fdd_status = "FDD";
	g_gui.fdd_status += static_cast<char>('1' + drive);
	g_gui.fdd_status += ' ';
	g_gui.fdd_status += action;
	const std::string name = display_file_name(path);
	if (!name.empty()) {
		g_gui.fdd_status += ": ";
		g_gui.fdd_status += name;
	}
}

static void set_hdd_status(int drive, const char *action, const char *path) {
	g_gui.hdd_status = hdd_interface_name(drive);
	if (hdd_is_scsi(drive)) {
		g_gui.hdd_status += std::to_string(hdd_slot(drive));
	} else {
		g_gui.hdd_status += static_cast<char>('1' + hdd_slot(drive));
	}
	g_gui.hdd_status += ' ';
	g_gui.hdd_status += action;
	const std::string name = display_file_name(path);
	if (!name.empty()) {
		g_gui.hdd_status += ": ";
		g_gui.hdd_status += name;
	}
}

/* `image`: the disk of a multi-image D88 (M120), 0 = first. */
static void remember_fdd_mount(int drive, const char *path, UINT image = 0) {
	if ((drive < 0) || (drive >= 2)) {
		return;
	}
	if ((path != nullptr) && (path[0] != '\0')) {
		copy_path(np2oscfg.fdd_image[drive], sizeof(np2oscfg.fdd_image[drive]), path);
		np2oscfg.fdd_image_index[drive] = static_cast<BYTE>(image);
	} else {
		np2oscfg.fdd_image[drive][0] = '\0';
		np2oscfg.fdd_image_index[drive] = 0;
	}
	sysmng_update(SYS_UPDATEOSCFG);
}

static void capture_reset_fdd_mounts(char paths[2][MAX_PATH], UINT images[2]) {
	for (int drive = 0; drive < 2; drive++) {
		const char *current;

		paths[drive][0] = '\0';
		images[drive] = 0;
		if (np2oscfg.fdd_image[drive][0] != '\0') {
			copy_path(paths[drive], MAX_PATH, np2oscfg.fdd_image[drive]);
			images[drive] = np2oscfg.fdd_image_index[drive];
			continue;
		}
		current = fdd_diskname(static_cast<REG8>(drive));
		if ((current != nullptr) && (current[0] != '\0')) {
			copy_path(paths[drive], MAX_PATH, current);
			images[drive] = fdd_diskimage(static_cast<REG8>(drive));
		}
	}
}

static void restore_reset_fdd_mounts(char paths[2][MAX_PATH], const UINT images[2]) {
	for (int drive = 0; drive < 2; drive++) {
		if (paths[drive][0] == '\0') {
			continue;
		}
		diskdrv_setfddex(static_cast<REG8>(drive), paths[drive], 0, images[drive]);
		remember_fdd_mount(drive, paths[drive], images[drive]);
	}
}

static void reset_guest(void) {
	char fdd_paths[2][MAX_PATH];
	UINT fdd_images[2];

	capture_reset_fdd_mounts(fdd_paths, fdd_images);
	taskmng_clear_fast_forward();
	pccore_cfgupdate();
	pccore_reset();
	restore_reset_fdd_mounts(fdd_paths, fdd_images);
	sdlkbd_reset_state();
	mousemng_reset();
	scrndrawva_redraw();
	state_capture_software();
	history_record_boot();
}

static void open_fdd_dialog(int drive) {
	const char *current;
	char archive_source_dir[MAX_PATH];
	std::string start_dir;

	g_gui.fdd_dialog_drive = drive;
	current = fdd_diskname(static_cast<REG8>(drive));
	if ((current == nullptr) || (current[0] == '\0')) {
		current = diskdrv_fname[drive];
	}
	if ((current != nullptr) && (current[0] != '\0')) {
		milstr_ncpy(g_gui.fdd_path[drive], current, sizeof(g_gui.fdd_path[drive]));
		if (dropmedia_fdd_source_directory(static_cast<UINT>(drive), current, archive_source_dir,
		                                   sizeof(archive_source_dir))) {
			start_dir = archive_source_dir;
		} else {
			start_dir = parent_dir(current);
		}
	}
	start_dir = history_start_dir(HIST_DIR_FDD, start_dir);
	g_gui.fdd_browser_dir = absolute_path(start_dir);
	g_gui.fdd_browser_open = true;
	g_gui.fdd_browser_refresh = true;
}

static void open_hdd_dialog(int drive) {
	const char *current;
	std::string start_dir;

	g_gui.hdd_dialog_drive = drive;
	current = hdd_config_path(drive);
	if ((current != nullptr) && (current[0] != '\0')) {
		milstr_ncpy(g_gui.hdd_path[hdd_slot(drive)], current,
		            sizeof(g_gui.hdd_path[hdd_slot(drive)]));
		start_dir = parent_dir(current);
	} else {
		g_gui.hdd_path[hdd_slot(drive)][0] = '\0';
	}
	start_dir = history_start_dir(HIST_DIR_HDD, start_dir);
	g_gui.hdd_browser_dir = absolute_path(start_dir);
	g_gui.hdd_browser_open = true;
	g_gui.hdd_browser_refresh = true;
}

static void open_new_sasi_dialog(int drive) {
	std::string start_dir;
	std::string path;

	g_gui.new_sasi_drive = std::clamp(drive, 0, 1);
	start_dir = history_start_dir(HIST_DIR_HDD, "");
	g_gui.hdd_browser_dir = absolute_path(start_dir);
	path = join_path(g_gui.hdd_browser_dir, new_sasi_default_name());
	copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path), path);
	g_gui.new_sasi_choice = 3;
	g_gui.new_sasi_open_after_create = true;
	g_gui.new_sasi_open = true;
	g_gui.new_sasi_refresh = true;
	g_gui.hdd_browser_open = false;
}

static void open_new_scsi_dialog(int drive) {
	std::string start_dir;
	std::string path;

	g_gui.new_scsi_drive = std::clamp(drive, 0, SCSIHDD_MAX - 1);
	start_dir = history_start_dir(HIST_DIR_HDD, "");
	g_gui.hdd_browser_dir = absolute_path(start_dir);
	path = new_scsi_default_path(g_gui.hdd_browser_dir, g_gui.new_scsi_drive);
	copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path), path);
	g_gui.new_scsi_choice = 3;
	g_gui.new_scsi_open_after_create = true;
	g_gui.new_scsi_open = true;
	g_gui.new_scsi_refresh = true;
	g_gui.hdd_browser_open = false;
}

static const char *new_fdd_default_name(int format, int container) {
	switch (format) {
	case NEWDISK_FDD_BLANK_2DD_720:
		return "newdisk-2dd.d88";
	case NEWDISK_FDD_BLANK_2D_320:
		return "newdisk-2d.d88";
	case NEWDISK_FDD_BLANK_2D_360:
		return "newdisk-2d360.d88";
	case NEWDISK_FDD_BLANK_2HD_1200:
		return "newdisk-2hd.d88";
	case NEWDISK_FDD_BLANK_2DD_640:
		return "newdisk-2dd.d88";
	default:
		break;
	}
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

	g_gui.new_fdd_format = std::clamp(format, 0, NEWDISK_FDD_MSDOS_COUNT - 1);
	g_gui.new_fdd_container = NEWDISK_FDD_CONTAINER_D88;
	start_dir = history_start_dir(HIST_DIR_FDD, "");
	g_gui.fdd_browser_dir = absolute_path(start_dir);
	path = join_path(g_gui.fdd_browser_dir,
	                 new_fdd_default_name(g_gui.new_fdd_format, g_gui.new_fdd_container));
	copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path), path);
	g_gui.new_fdd_drive = 0;
	g_gui.new_fdd_mount_after_create = true;
	g_gui.new_fdd_open = true;
	g_gui.new_fdd_refresh = true;
	g_gui.fdd_browser_open = false;
}

/* Mount an image or an archive (its images from `drive` on) chosen by the
 * user, and record it in the drive's history; `image` picks a disk of a
 * multi-image D88 (M120). */
static bool mount_fdd_path(int drive, const std::string &path_string, UINT image = 0) {
	const char *path = path_string.c_str();
	std::string error;

	if ((drive < 0) || (drive >= 2)) {
		return false;
	}
	if (!file_is_mountable(path, &error)) {
		g_gui.fdd_status = "FDD";
		g_gui.fdd_status += static_cast<char>('1' + drive);
		g_gui.fdd_status += " mount failed: ";
		g_gui.fdd_status += error;
		return false;
	}
	if (dropmedia_path_is_archive(path)) {
		if (!dropmedia_mount_archive(path, static_cast<UINT>(drive))) {
			g_gui.fdd_status = dropmedia_status();
			return false;
		}
		g_gui.fdd_status.clear();
	} else {
		diskdrv_setfddex(static_cast<REG8>(drive), path, 0, image);
		remember_fdd_mount(drive, path, image);
		dropmedia_prune_storage();
		set_fdd_status(drive, "mounted", path);
		// M120: the first disk of a multi-image D88 in FDD1 brings its
		// second disk into FDD2, as PC-8801 software expects.
		if ((drive == 0) && (image == 0) && (fddd88_images(path, nullptr, 0) >= 2)) {
			diskdrv_setfddex(1, path, 0, 1);
			remember_fdd_mount(1, path, 1);
			g_fdd_origin[1] = path_string;
			g_gui.fdd_status += " (FDD2: #2)";
		}
	}
	g_fdd_origin[drive] = path_string;
	history_add(drive, path_string);
	history_add_dir(HIST_DIR_FDD, path_string);
	return true;
}

static void mount_fdd_from_dialog(void) {
	const int drive = g_gui.fdd_dialog_drive;

	if ((drive >= 0) && (drive < 2) && mount_fdd_path(drive, g_gui.fdd_path[drive])) {
		g_gui.fdd_browser_open = false;
	}
}

static bool mount_hdd_path(int drive, const std::string &path_string) {
	const char *path = path_string.c_str();
	const int slot = hdd_slot(drive);
	std::string error;

	if (!hdd_file_is_mountable(path, &error)) {
		g_gui.hdd_status = hdd_interface_name(drive);
		g_gui.hdd_status += static_cast<char>('1' + slot);
		g_gui.hdd_status += " open failed: ";
		g_gui.hdd_status += error;
		return false;
	}
	diskdrv_sethdd(static_cast<REG8>(drive), path);

	set_hdd_status(drive, "configured; reset to apply", path);
	history_add(history_hdd_kind(drive), path_string);
	history_add_dir(HIST_DIR_HDD, path_string);
	return true;
}

static void mount_hdd_from_dialog(void) {
	const int drive = g_gui.hdd_dialog_drive;
	const int slot = hdd_slot(drive);

	if ((drive < 0) || (slot < 0) || (slot >= 4)) {
		return;
	}
	if (mount_hdd_path(drive, g_gui.hdd_path[slot])) {
		g_gui.hdd_browser_open = false;
	}
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

static std::string scsi_image_path(const char *path) {
	std::string result;

	if (path != nullptr) {
		result = path;
	}
	if (result.empty()) {
		return result;
	}
	fs::path p = fs::u8path(result);
	if (p.extension().empty()) {
		p += ".hdd";
	} else if ((p.extension() != ".hdd") && (p.extension() != ".hdi")) {
		p.replace_extension(".hdd");
	}
	return p.u8string();
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
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	const char *wanted = (container == NEWDISK_FDD_CONTAINER_RAW) ? ".img" : ".d88";
	if (extension.empty()) {
		p += wanted;
	} else if ((extension == ".d88") || (extension == ".img")) {
		p.replace_extension(wanted);
	} else if (extension != wanted) {
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
	format = std::clamp(g_gui.new_fdd_format, 0, NEWDISK_FDD_MSDOS_COUNT - 1);
	container = std::clamp(g_gui.new_fdd_container, 0, NEWDISK_FDD_CONTAINER_COUNT - 1);
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
		g_gui.fdd_status = "New FDD image failed: parent directory not found.";
		return;
	}
	if (newdisk_fdd_msdos_ex(path.c_str(), static_cast<UINT>(format),
	                         static_cast<UINT>(container)) != SUCCESS) {
		g_gui.fdd_status = "New FDD image failed: create failed.";
		return;
	}
	copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path), path);
	history_add_dir(HIST_DIR_FDD, path);
	if (g_gui.new_fdd_mount_after_create) {
		diskdrv_setfdd(static_cast<REG8>(drive), path.c_str(), 0);
		remember_fdd_mount(drive, path.c_str());
		dropmedia_prune_storage();
		set_fdd_status(drive, "created and mounted", path.c_str());
	} else {
		g_gui.fdd_status = "New FDD image created: ";
		g_gui.fdd_status += display_file_name(path.c_str());
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
	if ((attr == static_cast<short>(-1)) || ((attr & FILEATTR_DIRECTORY) != 0)) {
		g_gui.hdd_status = "New SASI image failed: create failed.";
		return;
	}
	copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path), path);
	history_add_dir(HIST_DIR_HDD, path);
	if (g_gui.new_sasi_open_after_create) {
		diskdrv_sethdd(static_cast<REG8>(drive), path.c_str());
		set_hdd_status(drive, "created and configured; reset to apply", path.c_str());
	} else {
		g_gui.hdd_status = "New SASI image created: ";
		g_gui.hdd_status += display_file_name(path.c_str());
	}
	g_gui.new_sasi_open = false;
}

static void create_new_scsi_image(void) {
	int drive;
	int choice;
	std::string path;
	short attr;

	drive = std::clamp(g_gui.new_scsi_drive, 0, SCSIHDD_MAX - 1);
	choice = std::clamp(g_gui.new_scsi_choice, 0, kScsiImageCount - 1);
	path = scsi_image_path(g_gui.new_scsi_path);
	if (path.empty()) {
		g_gui.hdd_status = "New SCSI image failed: path is empty.";
		return;
	}
	path = absolute_path(path);
	attr = file_attr(path.c_str());
	if (attr != static_cast<short>(-1)) {
		g_gui.hdd_status = "New SCSI image failed: file already exists.";
		return;
	}
	if (!is_directory(parent_dir(path))) {
		g_gui.hdd_status = "New SCSI image failed: parent directory not found.";
		return;
	}
	newdisk_vhd(path.c_str(), kScsiImageChoices[choice].size_mb);
	attr = file_attr(path.c_str());
	if ((attr == static_cast<short>(-1)) || ((attr & FILEATTR_DIRECTORY) != 0)) {
		g_gui.hdd_status = "New SCSI image failed: create failed.";
		return;
	}
	copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path), path);
	history_add_dir(HIST_DIR_HDD, path);
	if (g_gui.new_scsi_open_after_create) {
		diskdrv_sethdd(static_cast<REG8>(0x20 | drive), path.c_str());
		set_hdd_status(0x20 | drive, "created and configured; reset to apply", path.c_str());
	} else {
		g_gui.hdd_status = "New SCSI image created: ";
		g_gui.hdd_status += display_file_name(path.c_str());
	}
	g_gui.new_scsi_open = false;
}

static void eject_fdd(int drive) {
	g_fdd_origin[drive].clear();
	diskdrv_setfdd(static_cast<REG8>(drive), nullptr, 0);
	remember_fdd_mount(drive, nullptr);
	dropmedia_prune_storage();
	set_fdd_status(drive, "ejected", nullptr);
}

static void remove_hdd(int drive) {
	diskdrv_sethdd(static_cast<REG8>(drive), nullptr);
	sxsi_open();
	if (hdd_is_scsi(drive)) {
		if (!sxsi_isscsi()) {
			pccore.hddif &= ~PCHDD_SCSI;
		}
	} else if (!sxsi_issasi()) {
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
	prepare_dialog(38.75f, 26.25f, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Mount FDD image or archive", &g_gui.fdd_browser_open)) {
		if (draw_host_drive_selector(g_gui.fdd_browser_dir, g_gui.fdd_browser_refresh,
		                             "fdd-open")) {
			g_gui.fdd_path[drive][0] = '\0';
		}
		ImGui::Text("Target Dir");
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
		if (draw_recent_dirs(HIST_DIR_FDD, g_gui.fdd_browser_dir)) {
			g_gui.fdd_browser_refresh = true;
		}
		ImGui::Separator();
		const ImGuiStyle &style = ImGui::GetStyle();
		float footer_height = 3.0f * ImGui::GetFrameHeightWithSpacing();
		if (!g_gui.fdd_status.empty()) {
			footer_height += ImGui::CalcTextSize(g_gui.fdd_status.c_str(), nullptr, false,
			                                     ImGui::GetContentRegionAvail().x)
			                     .y +
			                 2.0f * style.ItemSpacing.y + 1.0f;
		}
		const float list_height = (std::max)(ImGui::GetFrameHeight() * 3.0f,
		                                     ImGui::GetContentRegionAvail().y - footer_height);
		if (ImGui::BeginChild("fdd-browser-list", ImVec2(0, list_height),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.fdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.fdd_browser_dir = entry.path;
						g_gui.fdd_browser_refresh = true;
					} else {
						copy_path(g_gui.fdd_path[drive], sizeof(g_gui.fdd_path[drive]), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##fdd-path", g_gui.fdd_path[drive], sizeof(g_gui.fdd_path[drive]));
		if (draw_file_type_combo("##fdd-file-type", kFddFileTypeLabel, &g_gui.fdd_all_files)) {
			g_gui.fdd_browser_refresh = true;
		}
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
	int slot;

	if (!g_gui.hdd_browser_open) {
		return;
	}
	drive = g_gui.hdd_dialog_drive;
	slot = hdd_slot(drive);
	if ((drive < 0) || (slot < 0) || (slot >= 4)) {
		g_gui.hdd_browser_open = false;
		return;
	}
	if (g_gui.hdd_browser_refresh) {
		refresh_hdd_browser();
	}
	prepare_dialog(38.75f, 28.125f, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Open HDD image", &g_gui.hdd_browser_open)) {
		if (draw_host_drive_selector(g_gui.hdd_browser_dir, g_gui.hdd_browser_refresh,
		                             "hdd-open")) {
			g_gui.hdd_path[slot][0] = '\0';
		}
		ImGui::Text("Target Dir");
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
		if (draw_recent_dirs(HIST_DIR_HDD, g_gui.hdd_browser_dir)) {
			g_gui.hdd_browser_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("hdd-browser-list",
		                      ImVec2(0, browser_list_height(3.0f, g_gui.hdd_status)),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.hdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.hdd_browser_dir = entry.path;
						g_gui.hdd_browser_refresh = true;
					} else {
						copy_path(g_gui.hdd_path[slot], sizeof(g_gui.hdd_path[slot]), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##hdd-path", g_gui.hdd_path[slot], sizeof(g_gui.hdd_path[slot]));
		if (!hdd_is_scsi(drive) &&
		    draw_file_type_combo("##hdd-file-type", kSasiFileTypeLabel, &g_gui.hdd_all_files)) {
			g_gui.hdd_browser_refresh = true;
		}
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
	prepare_dialog(38.75f, 31.25f, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Create SASI HDD image", &g_gui.new_sasi_open)) {
		if (draw_host_drive_selector(g_gui.hdd_browser_dir, g_gui.new_sasi_refresh, "new-sasi")) {
			copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path),
			          join_path(g_gui.hdd_browser_dir, new_sasi_default_name()));
		}
		ImGui::Text("Target Dir");
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
		                      ImVec2(0, browser_list_height(8.0f, g_gui.hdd_status)),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.hdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.hdd_browser_dir = entry.path;
						copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path),
						          join_path(entry.path, new_sasi_default_name()));
						g_gui.new_sasi_refresh = true;
					} else {
						copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##new-sasi-path", g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path));
		ImGui::Text("Image size");
		for (int i = 0; i < kSasiImageCount; i++) {
			if (i > 0) {
				ImGui::SameLine();
			}
			ImGui::RadioButton(kSasiImageChoices[i].label, &g_gui.new_sasi_choice, i);
		}
		ImGui::Text("Configure after create");
		ImGui::RadioButton("SASI-1", &g_gui.new_sasi_drive, 0);
		ImGui::SameLine();
		ImGui::RadioButton("SASI-2", &g_gui.new_sasi_drive, 1);
		ImGui::Checkbox("Set HDD file after create", &g_gui.new_sasi_open_after_create);
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

static void draw_new_scsi_dialog(void) {
	if (!g_gui.new_scsi_open) {
		return;
	}
	if (g_gui.new_scsi_refresh) {
		refresh_hdd_browser();
		g_gui.new_scsi_refresh = false;
	}
	prepare_dialog(38.75f, 31.25f, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Create SCSI HDD image", &g_gui.new_scsi_open)) {
		if (draw_host_drive_selector(g_gui.hdd_browser_dir, g_gui.new_scsi_refresh, "new-scsi")) {
			copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path),
			          new_scsi_default_path(g_gui.hdd_browser_dir, g_gui.new_scsi_drive));
		}
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.hdd_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.hdd_browser_dir = home_dir();
			g_gui.new_scsi_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.hdd_browser_dir = parent_dir(g_gui.hdd_browser_dir);
			g_gui.new_scsi_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("new-scsi-browser-list",
		                      ImVec2(0, browser_list_height(9.0f, g_gui.hdd_status)),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.hdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.hdd_browser_dir = entry.path;
						copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path),
						          new_scsi_default_path(entry.path, g_gui.new_scsi_drive));
						g_gui.new_scsi_refresh = true;
					} else {
						copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##new-scsi-path", g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path));
		ImGui::Text("Image size");
		for (int i = 0; i < kScsiImageCount; i++) {
			if (i > 0) {
				ImGui::SameLine();
			}
			ImGui::RadioButton(kScsiImageChoices[i].label, &g_gui.new_scsi_choice, i);
		}
		ImGui::Text("Configure after create");
		for (int drive = 0; drive < SCSIHDD_MAX; drive++) {
			int previous_drive = g_gui.new_scsi_drive;

			if (drive > 0) {
				ImGui::SameLine();
			}
			std::string label = "SCSI ID ";
			label += std::to_string(drive);
			if (ImGui::RadioButton(label.c_str(), &g_gui.new_scsi_drive, drive) &&
			    (std::string(g_gui.new_scsi_path) ==
			     new_scsi_default_path(g_gui.hdd_browser_dir, previous_drive))) {
				copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path),
				          new_scsi_default_path(g_gui.hdd_browser_dir, drive));
			}
		}
		ImGui::Checkbox("Set HDD file after create", &g_gui.new_scsi_open_after_create);
		if (ImGui::Button("Create")) {
			create_new_scsi_image();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.new_scsi_open = false;
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
	prepare_dialog(38.75f, 31.25f, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("New FDD image", &g_gui.new_fdd_open)) {
		if (draw_host_drive_selector(g_gui.fdd_browser_dir, g_gui.new_fdd_refresh, "new-fdd")) {
			copy_path(
			    g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			    join_path(g_gui.fdd_browser_dir,
			              new_fdd_default_name(g_gui.new_fdd_format, g_gui.new_fdd_container)));
		}
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.fdd_browser_dir.c_str());
		if (ImGui::Button("Home##new-fdd")) {
			g_gui.fdd_browser_dir = home_dir();
			copy_path(
			    g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			    join_path(g_gui.fdd_browser_dir,
			              new_fdd_default_name(g_gui.new_fdd_format, g_gui.new_fdd_container)));
			g_gui.new_fdd_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up##new-fdd")) {
			g_gui.fdd_browser_dir = parent_dir(g_gui.fdd_browser_dir);
			copy_path(
			    g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			    join_path(g_gui.fdd_browser_dir,
			              new_fdd_default_name(g_gui.new_fdd_format, g_gui.new_fdd_container)));
			g_gui.new_fdd_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("new-fdd-browser-list",
		                      ImVec2(0, browser_list_height(11.0f, g_gui.fdd_status)),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.fdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.fdd_browser_dir = entry.path;
						copy_path(
						    g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
						    join_path(entry.path, new_fdd_default_name(g_gui.new_fdd_format,
						                                               g_gui.new_fdd_container)));
						g_gui.new_fdd_refresh = true;
					} else {
						copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##new-fdd-path", g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path));
		// M113: new disks are unformatted D88 images (blank E5h sectors);
		// the guest's FORMAT rewrites every track with its own sector layout,
		// so one initial layout per media type is offered.
		ImGui::Text("未フォーマット (D88)");
		ImGui::RadioButton("2HD", &g_gui.new_fdd_format, NEWDISK_FDD_BLANK_2HD_1200);
		ImGui::SameLine();
		ImGui::RadioButton("2DD", &g_gui.new_fdd_format, NEWDISK_FDD_BLANK_2DD_720);
		ImGui::SameLine();
		ImGui::RadioButton("2D", &g_gui.new_fdd_format, NEWDISK_FDD_BLANK_2D_320);
		if (!newdisk_fdd_has_filesystem(static_cast<UINT>(g_gui.new_fdd_format)) &&
		    (g_gui.new_fdd_container != NEWDISK_FDD_CONTAINER_D88)) {
			g_gui.new_fdd_container = NEWDISK_FDD_CONTAINER_D88;
			copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			          fdd_image_path(g_gui.new_fdd_path, NEWDISK_FDD_CONTAINER_D88));
		}
		ImGui::Text("Mount after create");
		ImGui::RadioButton("FDD1##new-fdd", &g_gui.new_fdd_drive, 0);
		ImGui::SameLine();
		ImGui::RadioButton("FDD2##new-fdd", &g_gui.new_fdd_drive, 1);
		ImGui::Checkbox("Mount image after create", &g_gui.new_fdd_mount_after_create);
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

/* Emulation speed: 10-400 % of real time in 10 % steps, and No Wait (full
 * speed) at the right end of the slider. */
/* 実行速度: 10-400 % of real time in 10 % steps, and No Wait (full speed) at
 * the right end of the slider; frames, timers and sound follow. */
static void draw_speed_control(void) {
	enum {
		SPEED_MIN = 10,
		SPEED_MAX = 400,
		SPEED_FULL = SPEED_MAX + 10
	};
	const int speed = (np2oscfg.speed_percent == 0) ? 100 : np2oscfg.speed_percent;
	int value = np2oscfg.NOWAIT ? SPEED_FULL : speed;

	ImGui::TextUnformatted("実行速度");
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
	if (ImGui::SliderInt("##speed", &value, SPEED_MIN, SPEED_FULL,
	                     np2oscfg.NOWAIT ? "最大 (No Wait)" : "%d%%",
	                     ImGuiSliderFlags_AlwaysClamp)) {
		value = ((value + 5) / 10) * 10;
		if (value > SPEED_MAX) {
			np2oscfg.NOWAIT = 1;
		} else {
			np2oscfg.NOWAIT = 0;
			np2oscfg.speed_percent = static_cast<UINT16>(value);
			timing_setspeed(np2oscfg.speed_percent);
		}
		sysmng_update(SYS_UPDATEOSCFG);
	}
	if (ImGui::MenuItem("実行速度を 100% に戻す", nullptr, false,
	                    (np2oscfg.NOWAIT != 0) || (speed != 100))) {
		np2oscfg.NOWAIT = 0;
		np2oscfg.speed_percent = 100;
		timing_setspeed(100);
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

/*
 * CPU クロック (M110): the CPU alone; frames, timers and sound keep real
 * time. A preset is a multiplier of the 3.9936 MHz base and a 10-100 %
 * speed; both apply at once, without a reset. On the VA's V1/V2 mode the
 * PC-8801 presets are approximations to be calibrated on hardware.
 */
struct CpuClockPreset {
	const char *label;
	UINT multiple;
	UINT speed;
};

static const CpuClockPreset kCpuClockPresets[] = {
    {"機種の既定", PCCORE_STANDARD_MULTIPLE, 100},
    {"PC-8801 標準速度相当", PCCORE_STANDARD_MULTIPLE, 50},
    {"PC-8801 高速相当", PCCORE_STANDARD_MULTIPLE, 100},
    {nullptr, 0, 0},
    {"2倍", PCCORE_STANDARD_MULTIPLE * 2, 100},
    {"4倍", PCCORE_STANDARD_MULTIPLE * 4, 100},
    {"8倍", PCCORE_STANDARD_MULTIPLE * 8, 100},
};

static void set_cpu_clock(UINT multiple, UINT speed) {
	if (!pccore_cpu_multiple_valid(multiple)) {
		return;
	}
	np2cfg.multiple = multiple;
	np2cfg.cpu_speed = static_cast<UINT8>(std::clamp(speed, 10u, 100u));
	pccore_clockrestore();
	sgp_configure_speed();
	sysmng_update(SYS_UPDATECFG);
}

static void draw_cpu_clock_menu(void) {
	char heading[64];
	const UINT multiple = pccore_cpu_multiple();
	const UINT speed = pccore_cpu_speed();
	const double mhz = 3.9936 * multiple * speed / 100.0;

	std::snprintf(heading, sizeof(heading), "CPU クロック (%.2f MHz)###cpu-clock", mhz);
	if (!ImGui::BeginMenu(heading)) {
		return;
	}
	bool matched = false;
	for (const auto &preset : kCpuClockPresets) {
		if (preset.label == nullptr) {
			ImGui::Separator();
			continue;
		}
		// Only the first equal preset is checked (高速相当 equals 既定 on the VA).
		const bool selected = !matched && (multiple == preset.multiple) && (speed == preset.speed);
		matched = matched || selected;
		if (ImGui::MenuItem(preset.label, nullptr, selected)) {
			set_cpu_clock(preset.multiple, preset.speed);
		}
	}
	if (ImGui::BeginMenu("任意", true)) {
		int value = static_cast<int>(multiple);
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
		if (ImGui::SliderInt("倍率", &value, 1, PCCORE_CPU_MULTIPLE_MAX, "x%d",
		                     ImGuiSliderFlags_AlwaysClamp)) {
			set_cpu_clock(static_cast<UINT>(value), speed);
		}
		value = static_cast<int>(speed);
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
		if (ImGui::SliderInt("速度", &value, 10, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
			set_cpu_clock(multiple, static_cast<UINT>(((value + 2) / 5) * 5));
		}
		ImGui::TextDisabled("3.9936 MHz × 倍率 × 速度");
		ImGui::EndMenu();
	}
	ImGui::EndMenu();
}

/* M112: ROMs of the PC-8801/8001 families under extra/, scanned (SHA-1)
 * once each time the 起動 menu opens. */
static std::vector<EXTRAROM_MACHINE> g_extra_machines;

static void scan_extra_roms(void) {
	g_extra_machines.resize(EXTRAROM_MAX_MACHINES);
	const UINT count = extrarom_scan(g_extra_machines.data(), EXTRAROM_MAX_MACHINES);
	g_extra_machines.resize(count);
}

/* A MAME match is not shown; a ROM MAME does not list, or marks BAD_DUMP, is. */
static std::string extra_rom_item(const EXTRAROM_FILE &file) {
	std::string text = file.label;
	if (file.mame[0] == '\0') {
		text += "  [MAME と不一致]";
	} else if (file.mame_bad_dump) {
		text += "  [BAD_DUMP]";
	}
	return text;
}

static void extra_rom_tooltip(const EXTRAROM_FILE &file) {
	if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		return;
	}
	ImGui::BeginTooltip();
	ImGui::Text("%s  (%u バイト)", file.file, static_cast<unsigned>(file.size));
	ImGui::Text("SHA-1 %s", file.sha1);
	if (file.banner[0] != '\0') {
		ImGui::Text("版表示: %s", file.banner);
	}
	if (file.mame[0] != '\0') {
		ImGui::Text("MAME: %s%s%s%s", file.mame, (file.mame_bios[0] != '\0') ? " (" : "",
		            file.mame_bios, (file.mame_bios[0] != '\0') ? ")" : "");
	}
	ImGui::EndTooltip();
}

/* M113: the start entries of a model, labelled as for the VA: V2/V1 (H
 * where the model runs at 8 MHz, S) and N when it has N-BASIC. */
static void draw_extra_machine_start(const EXTRAROM_MACHINE &machine) {
	EXTRAROM_PROFILE profile;

	if (!extrarom_profile(machine.name, &profile) || !profile.startable) {
		ImGui::TextDisabled("起動はまだできません");
		return;
	}
	const bool ready = extrarom_startable(&machine);
	const bool current = milstr_cmp(np2cfg.extra_machine, machine.name) == 0;
	const bool v1 = bkupmemva_get_88v1() != FALSE;
	const bool standard = np2cfg.v1v2_standard != 0;
	// M114: the PC-8001 family starts in N mode only, from its own N-BASIC.
	const UINT n_role = profile.n_role ? profile.n_role : static_cast<UINT>(EXTRAROM_ROLE_N80);
	const bool has_n80 = extrarom_choice(&machine, n_role) != nullptr;
	static const struct {
		const char *label;
		bool v1;
		bool standard;
	} modes[] = {
	    {"V2 H", false, false}, {"V2 S", false, true}, {"V1 H", true, false}, {"V1 S", true, true}};
	for (const auto &mode : modes) {
		if ((!mode.v1 && !profile.v2) || (!mode.standard && !profile.high) || profile.n_role) {
			continue;
		}
		const bool selected = current && (np2cfg.boot_mode == BOOTMODE_V1V2) && (v1 == mode.v1) &&
		                      (standard == mode.standard);
		if (ImGui::MenuItem(mode.label, nullptr, selected, ready)) {
			select_extra_machine(machine.name, BOOTMODE_V1V2, mode.v1, mode.standard);
		}
	}
	if (has_n80 &&
	    ImGui::MenuItem("N", nullptr, current && (np2cfg.boot_mode == BOOTMODE_N), ready)) {
		select_extra_machine(machine.name, BOOTMODE_N, v1, profile.high ? standard : true);
	}
	if (!ready) {
		ImGui::TextDisabled(profile.n_role ? "BASIC の ROM がありません" : "n88.rom がありません");
	}
}

/* The ROMs of a model, one line per role; a role with several files is a
 * submenu of its revisions, remembered per model and role. */
static void draw_extra_rom_files(const EXTRAROM_MACHINE &machine) {
	static const char *const reasons[] = {"名前が役割に合わない", "大きさが違う", "中身が空",
	                                      "読めない"};
	if (!ImGui::BeginMenu("ROM")) {
		return;
	}
	if (machine.missing[0] != '\0') {
		ImGui::TextDisabled("不足: %s", machine.missing);
	}
	for (UINT role = 0; role < EXTRAROM_ROLES; role++) {
		const EXTRAROM_FILE *files[EXTRAROM_MAX_FILES];
		const UINT n = extrarom_revisions(&machine, role, files, EXTRAROM_MAX_FILES);
		const EXTRAROM_FILE *chosen = extrarom_choice(&machine, role);
		if ((n == 0) || (chosen == nullptr)) {
			continue;
		}
		const std::string head = std::string(extrarom_role_names[role]) + ": ";
		if (n == 1) {
			ImGui::MenuItem((head + extra_rom_item(*chosen)).c_str(), nullptr, false, false);
			extra_rom_tooltip(*chosen);
			continue;
		}
		const std::string menu = head + chosen->label + "##" + extrarom_role_names[role];
		if (!ImGui::BeginMenu(menu.c_str())) {
			continue;
		}
		for (UINT i = 0; i < n && i < EXTRAROM_MAX_FILES; i++) {
			const std::string item =
			    extra_rom_item(*files[i]) + "  " + files[i]->file + "##" + std::to_string(i);
			if (ImGui::MenuItem(item.c_str(), nullptr, files[i] == chosen)) {
				extrarom_select(machine.name, role, files[i]->revision);
				sysmng_update(SYS_UPDATECFG);
			}
			extra_rom_tooltip(*files[i]);
		}
		ImGui::EndMenu();
	}
	for (UINT i = 0; (i < machine.ignored) && (i < EXTRAROM_MAX_IGNORED); i++) {
		ImGui::TextDisabled("使わない: %s (%s)", machine.skipped[i].file,
		                    reasons[machine.skipped[i].reason]);
	}
	ImGui::EndMenu();
}

static void draw_extra_rom_machine(const EXTRAROM_MACHINE &machine) {
	std::string title = machine.name;
	if (!machine.known) {
		title += " (機種不明)";
	} else if (machine.missing[0] != '\0') {
		title += " (不足あり)";
	}
	if (!ImGui::BeginMenu(title.c_str())) {
		return;
	}
	draw_extra_machine_start(machine);
	ImGui::Separator();
	draw_extra_rom_files(machine);
	ImGui::EndMenu();
}

/* 起動 > PC-8801/8001: the models found under extra/, started in the VA's
 * V1/V2 mode with their own ROMs. The directory is shown on hover only
 * when nothing was found, so that a long path does not widen the menu. */
static void draw_extra_rom_menu(void) {
	if (!ImGui::BeginMenu("PC-8801/8001")) {
		return;
	}
	char dir[MAX_PATH];
	extrarom_dir(dir, sizeof(dir));
	if (g_extra_machines.empty()) {
		ImGui::TextDisabled("ROM がありません");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("%s/<機種名>/<役割>.rom", dir);
		}
	}
	for (const auto &machine : g_extra_machines) {
		draw_extra_rom_machine(machine);
	}
	ImGui::EndMenu();
}

static void draw_boot_menu(void) {
	static const struct {
		const char *label;
		bool v1;
		bool standard;
	} modes[] = {
	    {"V2 H", false, false}, {"V2 S", false, true}, {"V1 H", true, false}, {"V1 S", true, true}};
	static const struct {
		const char *label;
		const char *model;
	} machines[] = {{"PC-88VA2/VA3", str_VA2}, {"PC-88VA", str_VA1}};

	if (!ImGui::BeginMenu("起動")) {
		g_n80_scanned = false;
		return;
	}
	// The ROM directory is scanned (SHA-1) once each time the menu opens.
	if (!g_n80_scanned) {
		n80rom_scan(g_n80_entries, ROMVA_N80_NAMES);
		scan_extra_roms();
		g_n80_scanned = true;
	}
	const bool v1 = bkupmemva_get_88v1() != FALSE;
	const bool standard = np2cfg.v1v2_standard != 0;
	const int boot_mode = np2cfg.boot_mode;
	for (const auto &machine : machines) {
		const bool current =
		    (milstr_cmp(np2cfg.model, machine.model) == 0) && (np2cfg.extra_machine[0] == '\0');
		if (!ImGui::BeginMenu(machine.label)) {
			continue;
		}
		// 自動: a disk without a V3 IPL starts the V1/V2 mode last chosen.
		if (ImGui::MenuItem("自動", nullptr, current && (boot_mode == BOOTMODE_AUTO))) {
			select_boot(machine.model, BOOTMODE_AUTO, v1, standard, nullptr);
		}
		for (const auto &mode : modes) {
			if (ImGui::MenuItem(mode.label, nullptr,
			                    current && (boot_mode == BOOTMODE_V1V2) && (v1 == mode.v1) &&
			                        (standard == mode.standard))) {
				select_boot(machine.model, BOOTMODE_V1V2, mode.v1, mode.standard, nullptr);
			}
		}
		// vaeg extension: N-BASIC from a user-supplied ROM.
		for (const auto &entry : g_n80_entries) {
			if (!entry.present) {
				continue;
			}
			const std::string label = std::string("N (") + entry.label + ")##" + entry.name;
			const bool loaded = current && (boot_mode == BOOTMODE_N) && memoryva_n80_exist &&
			                    (strcmp(memoryva_n80_file, entry.name) == 0);
			if (ImGui::MenuItem(label.c_str(), nullptr, loaded)) {
				select_boot(machine.model, BOOTMODE_N, v1, standard, entry.name);
			}
		}
		ImGui::EndMenu();
	}
	ImGui::Separator();
	draw_extra_rom_menu();
	ImGui::EndMenu();
}

static void set_display_rotation(int rotation);
static void set_display_scaling(int scaling);
static void set_display_scale(int scale);
static void set_native_crt_mode(bool filtered);

/* M124: put back the display a recent boot ended with. */
static void restore_boot_display(const BootRecord &boot) {
	if (boot.rotate >= 0) {
		set_display_rotation(boot.rotate);
	}
	if (boot.renderer >= 0) {
		const int current = !np2oscfg.gui_native_crt ? 0 : (np2oscfg.gui_native_filter ? 1 : 2);
		if (boot.renderer != current) {
			if (boot.renderer == 0) {
				np2oscfg.gui_native_filter = 1;
				scrnmng_request_native_crt(FALSE, TRUE);
				sysmng_update(SYS_UPDATEOSCFG);
			} else {
				set_native_crt_mode(boot.renderer == 1);
			}
		}
	}
	if (boot.scaling >= 0) {
		set_display_scaling(boot.scaling);
	}
	if ((boot.window_width > 0) && (scrnmng_get_display_mode() == VAEG_DISPLAY_WINDOWED)) {
		if (boot.scale >= 1) {
			set_display_scale(boot.scale);
		} else {
			np2oscfg.gui_scale = 0;
		}
		SDL_SetWindowSize(static_cast<SDL_Window *>(scrnmng_get_window()), boot.window_width,
		                  boot.window_height + scrnmng_front_panel_applied());
		np2oscfg.gui_window_width = static_cast<UINT16>(boot.window_width);
		np2oscfg.gui_window_height = static_cast<UINT16>(boot.window_height);
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

/* 最近の起動: put a recorded set of media back, select its machine and
 * mode, and reset (which moves the set to the top). */
static void boot_from_history(const BootRecord &chosen) {
	// M124: the running session ends here; record its display (this only
	// updates the first record in place), then copy the chosen one, whose
	// display is applied below
	history_note_session_display();
	g_session_recorded = false;
	const BootRecord boot = chosen;
	g_gui.boot_status.clear();
	for (int drive = 0; drive < 2; drive++) {
		if (boot.fdd[drive].empty()) {
			eject_fdd(drive);
		} else if (!mount_fdd_path(drive, boot.fdd[drive], boot.fdd_image[drive])) {
			// M116: do not reset into the previous disk; say why instead.
			g_gui.boot_status = "起動できませんでした: " + g_gui.fdd_status;
			fprintf(stderr, "最近の起動: %s\n", g_gui.fdd_status.c_str());
			return;
		}
		if (boot.sasi[drive].empty()) {
			diskdrv_sethdd(static_cast<REG8>(drive), nullptr);
		} else {
			diskdrv_sethdd(static_cast<REG8>(drive), boot.sasi[drive].c_str());
		}
	}
	if (boot.tape.empty()) {
		if (cmt_inserted()) {
			cmt_eject();
		}
		g_gui.tape_loaded.clear();
	} else {
		(void)load_tape_path(boot.tape);
	}
	// M124: the monitor applies at the reset below; the rotation at once
	if (boot.monitor_15khz >= 0) {
		np2cfg.monitor_15khz = static_cast<UINT8>(boot.monitor_15khz);
		sysmng_update(SYS_UPDATECFG);
	}
	restore_boot_display(boot);
	if (!boot.extra.empty()) {
		select_extra_machine(boot.extra.c_str(), boot.mode, boot.v1, boot.standard);
		return;
	}
	select_boot(boot.model.c_str(), boot.mode, boot.v1, boot.standard,
	            (boot.mode == BOOTMODE_N) ? boot.n80rom.c_str() : nullptr);
}

static void draw_recent_boots_menu(void) {
	if (!ImGui::BeginMenu("最近の起動")) {
		return;
	}
	history_load();
	if (!g_gui.boot_status.empty()) {
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "%s", g_gui.boot_status.c_str());
		ImGui::Separator();
	}
	if (g_recent_boots.empty()) {
		ImGui::TextDisabled("なし");
	}
	for (std::size_t index = 0; index < g_recent_boots.size(); index++) {
		const BootRecord boot = g_recent_boots[index];
		const std::string label = history_boot_label(boot) + "##boot" + std::to_string(index);
		const std::string missing = history_missing_paths(boot);
		if (ImGui::MenuItem(label.c_str(), nullptr, false, missing.empty())) {
			boot_from_history(boot);
		}
		// M116: a greyed entry says which file is gone.
		if (!missing.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("見つからないファイル:\n%s", missing.c_str());
		}
	}
	ImGui::Separator();
	if (ImGui::MenuItem("履歴を消す", nullptr, false, !g_recent_boots.empty())) {
		g_recent_boots.clear();
		history_save();
	}
	ImGui::EndMenu();
}

static void draw_machine_config_menu(void) {
	if (!ImGui::BeginMenu("機種の構成")) {
		return;
	}
	if (ImGui::BeginMenu("メインメモリ容量")) {
		const UINT16 capacities[] = {256, 384, 512, 640};
		for (UINT16 capacity : capacities) {
			char label[16];
			std::snprintf(label, sizeof(label), "%u KB", capacity);
			if (ImGui::MenuItem(label, nullptr, np2cfg.main_ram == capacity)) {
				save_main_ram_settings(capacity, np2cfg.main_ram_auto != 0);
			}
		}
		ImGui::TextDisabled("変更は次のリセットで反映");
		ImGui::TextDisabled("現在の実装容量: %u KB", pccore_mainram_limit() / 1024);
		ImGui::EndMenu();
	}
	if (ImGui::MenuItem("バックアップメモリへ自動反映", nullptr, np2cfg.main_ram_auto != 0)) {
		save_main_ram_settings(np2cfg.main_ram, np2cfg.main_ram_auto == 0);
	}
	if (ImGui::MenuItem("HDD から起動", nullptr, np2cfg.boot_fixed_disk != 0)) {
		save_boot_fixed_disk(np2cfg.boot_fixed_disk == 0);
	}
	// M115: PCG-8100/8800 (character generator and three tones) in V1/V2
	// mode; ports 00h-03h and 0Ch-0Fh, no reset needed.
	if (ImGui::MenuItem("PCG-8100/8800 (V1/V2)", nullptr, np2cfg.pcg_enable != 0)) {
		np2cfg.pcg_enable = np2cfg.pcg_enable ? 0 : 1;
		sysmng_update(SYS_UPDATECFG);
		pccore_redraw();
	}
	if (ImGui::MenuItem("I/O バンクメモリ…", nullptr, bmsiocfg.enabled != FALSE)) {
		open_bms_config_dialog();
	}
	if (ImGui::MenuItem("EMS ボード…", nullptr, np2cfg.EXTMEM != 0)) {
		open_ems_config_dialog();
	}
	ImGui::EndMenu();
}

static void draw_emulate_menu(void) {
	if (!ImGui::BeginMenu("模擬")) {
		g_n80_scanned = false;
		return;
	}
	if (ImGui::MenuItem("リセット")) {
		reset_guest();
	}
	draw_boot_menu();
	draw_recent_boots_menu();
	ImGui::Separator();
	draw_speed_control();
	draw_cpu_clock_menu();
	ImGui::Separator();
	draw_machine_config_menu();
	if (ImGui::MenuItem("詳細設定…")) {
		open_configure_dialog();
	}
	ImGui::Separator();
	if (ImGui::MenuItem("終了")) {
		taskmng_exit();
	}
	ImGui::EndMenu();
}

static void draw_edit_menu(void) {
	if (ImGui::BeginMenu("編集")) {
#if defined(__APPLE__)
		const char *copy_shortcut = "Cmd+C";
		const char *shortcut = "Cmd+V";
#else
		const char *copy_shortcut = "Ctrl+Shift+C";
		const char *shortcut = "Ctrl+V";
#endif
		if (ImGui::MenuItem("画面の文字をコピー", copy_shortcut)) {
			copy_screen_text();
		}
		if (ImGui::MenuItem("貼り付け", shortcut)) {
			kbdpaste_start_clipboard();
		}
		if (ImGui::MenuItem("貼り付けの取り消し", nullptr, false,
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

/* M120: the disk names of a D88 file (UTF-8, empty when unnamed), read
 * again only when the file or its size changes; empty for other files. */
static const std::vector<std::string> &d88_disk_names(const std::string &path) {
	struct Cached {
		long long size = -1;
		std::vector<std::string> names;
	};
	static std::map<std::string, Cached> cache;
	std::error_code error;
	const long long size = static_cast<long long>(fs::file_size(fs::u8path(path), error));
	Cached &entry = cache[path];
	if (!error && (entry.size == size)) {
		return entry.names;
	}
	entry.size = error ? -1 : size;
	entry.names.clear();
	// a plain array: libc++ rejects a std::vector of char arrays
	static char raw[FDDD88_MAX_IMAGES][FDDD88_NAME_SIZE];
	const UINT count = fddd88_images(path.c_str(), raw, FDDD88_MAX_IMAGES);
	for (UINT i = 0; i < count; i++) {
		UINT16 utf[FDDD88_NAME_SIZE];
		std::string name;
		ZeroMemory(utf, sizeof(utf));
		codecnv_sjis2utf(utf, NELEMENTS(utf), raw[i], FDDD88_NAME_SIZE);
		for (UINT k = 0; (k < NELEMENTS(utf)) && (utf[k] != 0); k++) {
			copy_append_utf8(&name, utf[k]);
		}
		while (!name.empty() && (name.back() == ' ')) {
			name.pop_back();
		}
		entry.names.push_back(name);
	}
	return entry.names;
}

/* The file in a drive or about to be inserted, and its disk (M120). */
static std::string fdd_current_file(int drive, UINT *image, bool *inserting) {
	const char *path = fdd_diskname(static_cast<REG8>(drive));
	*inserting = false;
	*image = fdd_diskimage(static_cast<REG8>(drive));
	if ((path == nullptr) || (path[0] == '\0')) {
		path = diskdrv_fname[drive];
		*inserting = path[0] != '\0';
		*image = diskdrv_pendingimage(static_cast<REG8>(drive));
	}
	return ((path != nullptr) ? path : "");
}

static std::string d88_disk_label(UINT image, const std::vector<std::string> &names) {
	std::string label = std::to_string(image + 1) + "/" + std::to_string(names.size());
	if ((image < names.size()) && !names[image].empty()) {
		label += " " + names[image];
	}
	return label;
}

/* Put disk `image` of `path` in `drive`, keeping where the file came from
 * (an archive) as `origin` (M120). */
static void switch_fdd_disk(int drive, const std::string &path, UINT image,
                            const std::string &origin) {
	diskdrv_setfddex(static_cast<REG8>(drive), path.c_str(), 0, image);
	remember_fdd_mount(drive, path.c_str(), image);
	g_fdd_origin[drive] = origin;
	set_fdd_status(drive, "mounted", path.c_str());
	g_gui.fdd_status += " #" + std::to_string(image + 1);
}

/* FDDn ディスク: the disks of the file in this drive and, when it holds
 * several, of the file in the other drive (M120). */
static void draw_fdd_disk_menu(int drive) {
	struct Source {
		int drive;
		std::string path;
		UINT image;
	};
	std::vector<Source> sources;
	for (int i = 0; i < 2; i++) {
		const int source_drive = (i == 0) ? drive : (1 - drive);
		bool inserting;
		UINT image;
		const std::string path = fdd_current_file(source_drive, &image, &inserting);
		if (path.empty() || (d88_disk_names(path).size() < 2) ||
		    ((i == 1) && !sources.empty() && (sources[0].path == path))) {
			continue;
		}
		sources.push_back({source_drive, path, image});
	}
	char label[32];
	std::snprintf(label, sizeof(label), "FDD%d ディスク", drive + 1);
	if (sources.empty() || !ImGui::BeginMenu(label)) {
		return;
	}
	bool current_image_set = false;
	UINT current_image = 0;
	bool inserting;
	const std::string current = fdd_current_file(drive, &current_image, &inserting);
	current_image_set = !current.empty();
	for (const Source &source : sources) {
		const std::vector<std::string> names = d88_disk_names(source.path);
		if (source.drive != drive) {
			ImGui::Separator();
			ImGui::TextDisabled("FDD%d のファイル: %s", source.drive + 1,
			                    display_file_name(source.path.c_str()).c_str());
		}
		for (UINT k = 0; k < names.size(); k++) {
			std::string item = std::to_string(k + 1) + ": ";
			item += names[k].empty() ? "(名前なし)" : names[k];
			item += "##" + std::to_string(source.drive) + "-" + std::to_string(k);
			const bool selected =
			    current_image_set && (current == source.path) && (current_image == k);
			if (ImGui::MenuItem(item.c_str(), nullptr, selected)) {
				switch_fdd_disk(drive, source.path, k, g_fdd_origin[source.drive]);
			}
		}
	}
	ImGui::EndMenu();
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
		ImGui::TextDisabled("FDD%d: なし", drive + 1);
		return;
	}
	std::string name = display_file_name(path);
	// M120: which disk of a multi-image D88 file
	const std::vector<std::string> &names = d88_disk_names(path);
	if (names.size() > 1) {
		UINT image;
		bool pending;
		(void)fdd_current_file(drive, &image, &pending);
		name += " [" + d88_disk_label(image, names) + "]";
	}
	if (inserting) {
		ImGui::Text("FDD%d: %s (挿入中)", drive + 1, name.c_str());
	} else {
		ImGui::Text("FDD%d: %s", drive + 1, name.c_str());
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", name.c_str());
	}
}

static void draw_hdd_mount_state(int drive) {
	std::string label;
	const char *path;

	path = hdd_config_path(drive);
	label = hdd_interface_name(drive);
	if (hdd_is_scsi(drive)) {
		label += std::to_string(hdd_slot(drive));
	} else {
		label += static_cast<char>('1' + hdd_slot(drive));
	}
	if ((path == nullptr) || (path[0] == '\0')) {
		ImGui::TextDisabled("%s: なし", label.c_str());
		return;
	}
	const std::string name = display_file_name(path);
	ImGui::Text("%s: %s", label.c_str(), name.c_str());
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", name.c_str());
	}
}

static void draw_fdd_menu(void) {
	if (ImGui::BeginMenu("FDD")) {
		if (ImGui::MenuItem("FDD1 開く…")) {
			open_fdd_dialog(0);
		}
		draw_history_menu("FDD1 履歴", HIST_FDD1, [](const std::string &path) {
			mount_fdd_path(0, path);
		});
		if (ImGui::MenuItem("FDD1 取り出し")) {
			eject_fdd(0);
		}
		draw_fdd_disk_menu(0);
		draw_fdd_mount_state(0);
		ImGui::Separator();
		if (ImGui::MenuItem("FDD2 開く…")) {
			open_fdd_dialog(1);
		}
		draw_history_menu("FDD2 履歴", HIST_FDD2, [](const std::string &path) {
			mount_fdd_path(1, path);
		});
		if (ImGui::MenuItem("FDD2 取り出し")) {
			eject_fdd(1);
		}
		draw_fdd_disk_menu(1);
		draw_fdd_mount_state(1);
		if (dropmedia_status()[0] != '\0') {
			ImGui::Separator();
			ImGui::TextWrapped("%s", dropmedia_status());
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("新しい FDD イメージ")) {
			// Every new disk is unformatted; the guest formats it with its
			// own sector layout (a 2DD becomes 640 or 720 KB, a 2D 320 or
			// 360 KB), so one entry per media type is enough.
			ImGui::TextDisabled("未フォーマット");
			if (ImGui::MenuItem("2HD…")) {
				open_new_fdd_dialog(NEWDISK_FDD_BLANK_2HD_1200);
			}
			if (ImGui::MenuItem("2DD…")) {
				open_new_fdd_dialog(NEWDISK_FDD_BLANK_2DD_720);
			}
			if (ImGui::MenuItem("2D…")) {
				open_new_fdd_dialog(NEWDISK_FDD_BLANK_2D_320);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
}

// ---- Cassette tape (vaeg extension for V1/V2 mode, M103j)

static void refresh_tape_browser(void) {
	std::error_code ec;

	g_gui.tape_entries.clear();
	if (!is_directory(g_gui.tape_browser_dir)) {
		g_gui.tape_browser_dir = home_dir();
	}
	for (const auto &entry : fs::directory_iterator(fs::u8path(g_gui.tape_browser_dir), ec)) {
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
		if (!item.is_dir && !g_gui.tape_all_files &&
		    !file_type_listed(item.name, kTapeFileTypes, std::size(kTapeFileTypes))) {
			continue;
		}
		g_gui.tape_entries.push_back(item);
	}
	std::sort(g_gui.tape_entries.begin(), g_gui.tape_entries.end(), browser_entry_less);
	g_gui.tape_browser_refresh = false;
}

static void open_tape_dialog(void) {
	std::string start_dir;

	if (!g_gui.tape_loaded.empty()) {
		start_dir = parent_dir(g_gui.tape_loaded);
	} else if (!g_gui.tape_recording.empty()) {
		start_dir = parent_dir(g_gui.tape_recording);
	}
	start_dir = history_start_dir(HIST_DIR_TAPE, start_dir);
	g_gui.tape_browser_dir = absolute_path(start_dir);
	g_gui.tape_status.clear();
	g_gui.tape_browser_open = true;
	g_gui.tape_browser_refresh = true;
}

/* Load a tape image, or the first .cmt/.t88 of a zip/7z/lzh archive, and
 * record it in the tape history. */
static bool load_tape_path(const std::string &path) {
	char extracted[MAX_PATH];
	const char *source = path.c_str();

	if (dropmedia_path_is_tape_archive(path.c_str())) {
		source = dropmedia_extract_tape(path.c_str(), extracted, sizeof(extracted)) ? extracted
		                                                                            : nullptr;
	}
	if (source == nullptr) {
		g_gui.tape_status = dropmedia_status();
		return false;
	}
	if (cmt_open(source) != SUCCESS) {
		g_gui.tape_status = "Not a usable tape image.";
		return false;
	}
	g_gui.tape_loaded = path;
	g_gui.tape_status = "Tape loaded.";
	history_add(HIST_TAPE, path);
	history_add_dir(HIST_DIR_TAPE, path);
	return true;
}

static void stop_tape_recording(void) {
	if (cmt_saving()) {
		g_gui.tape_status =
		    (cmt_save_end() == SUCCESS) ? "Recording saved: " : "Recording could not be written: ";
		g_gui.tape_status += g_gui.tape_recording;
	}
	g_gui.tape_recording.clear();
}

static void draw_tape_browser(void) {
	if (!g_gui.tape_browser_open) {
		return;
	}
	if (g_gui.tape_browser_refresh) {
		refresh_tape_browser();
	}
	prepare_dialog(38.75f, 26.25f, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Cassette tape image", &g_gui.tape_browser_open)) {
		draw_host_drive_selector(g_gui.tape_browser_dir, g_gui.tape_browser_refresh, "tape-open");
		ImGui::TextWrapped("%s", g_gui.tape_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.tape_browser_dir = home_dir();
			g_gui.tape_browser_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.tape_browser_dir = parent_dir(g_gui.tape_browser_dir);
			g_gui.tape_browser_refresh = true;
		}
		if (draw_recent_dirs(HIST_DIR_TAPE, g_gui.tape_browser_dir)) {
			g_gui.tape_browser_refresh = true;
		}
		ImGui::Separator();
		const float list_height = (std::max)(ImGui::GetFrameHeight() * 3.0f,
		                                     ImGui::GetContentRegionAvail().y -
		                                         4.0f * ImGui::GetFrameHeightWithSpacing());
		if (ImGui::BeginChild("tape-browser-list", ImVec2(0, list_height),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.tape_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.tape_browser_dir = entry.path;
						g_gui.tape_browser_refresh = true;
					} else {
						copy_path(g_gui.tape_path, sizeof(g_gui.tape_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##tape-path", g_gui.tape_path, sizeof(g_gui.tape_path));
		if (draw_file_type_combo("##tape-file-type", kTapeFileTypeLabel, &g_gui.tape_all_files)) {
			g_gui.tape_browser_refresh = true;
		}
		if (ImGui::Button("Load tape") && load_tape_path(g_gui.tape_path)) {
			g_gui.tape_browser_open = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Record to this file")) {
			stop_tape_recording();
			if ((g_gui.tape_path[0] != '\0') && (cmt_save_begin(g_gui.tape_path) == SUCCESS)) {
				g_gui.tape_recording = g_gui.tape_path;
				history_add_dir(HIST_DIR_TAPE, g_gui.tape_recording);
				g_gui.tape_status = "Recording; written when stopped.";
				g_gui.tape_browser_open = false;
			} else {
				g_gui.tape_status = "Enter a file name to record to.";
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.tape_browser_open = false;
		}
		if (!g_gui.tape_status.empty()) {
			ImGui::TextWrapped("%s", g_gui.tape_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_tape_menu(void) {
	if (ImGui::BeginMenu("Tape")) {
		if (ImGui::MenuItem("読み込み・録音…")) {
			open_tape_dialog();
		}
		draw_history_menu("履歴", HIST_TAPE, [](const std::string &path) {
			(void)load_tape_path(path);
		});
		if (ImGui::MenuItem("巻き戻し", nullptr, false, cmt_inserted() != FALSE)) {
			cmt_rewind();
		}
		if (ImGui::MenuItem("取り出し", nullptr, false, cmt_inserted() != FALSE)) {
			cmt_eject();
			g_gui.tape_loaded.clear();
		}
		if (ImGui::MenuItem("録音を止める", nullptr, false, cmt_saving() != FALSE)) {
			stop_tape_recording();
		}
		bool fast = np2cfg.cmt_fast != 0;
		if (ImGui::MenuItem("Fast load", nullptr, fast)) {
			np2cfg.cmt_fast = fast ? 0 : 1;
			sysmng_update(SYS_UPDATECFG);
		}
		ImGui::Separator();
		if (cmt_inserted()) {
			const std::string name = display_file_name(g_gui.tape_loaded.c_str());
			ImGui::Text("テープ: %s", name.c_str());
			ImGui::Text("%u / %u バイト", static_cast<unsigned>(cmt_position()),
			            static_cast<unsigned>(cmt_length()));
		} else {
			ImGui::TextDisabled("テープなし");
		}
		if (cmt_saving()) {
			const std::string name = display_file_name(g_gui.tape_recording.c_str());
			ImGui::Text("録音中: %s (%u バイト)", name.c_str(),
			            static_cast<unsigned>(cmt_saved_bytes()));
		}
		if (!g_gui.tape_status.empty()) {
			ImGui::TextWrapped("%s", g_gui.tape_status.c_str());
		}
		ImGui::TextDisabled("V1/V2 BASIC 用 (vaeg 独自)");
		ImGui::EndMenu();
	}
}

static void draw_harddisk_menu(void) {
	if (!ImGui::BeginMenu("HDD")) {
		return;
	}
	for (int drive = 0; drive < 2; drive++) {
		const std::string name = std::string("SASI-") + static_cast<char>('1' + drive);
		if (ImGui::MenuItem((name + " 開く…").c_str())) {
			open_hdd_dialog(drive);
		}
		draw_history_menu((name + " 履歴").c_str(), HIST_SASI1 + drive,
		                  [drive](const std::string &path) {
			                  mount_hdd_path(drive, path);
		                  });
		if (ImGui::MenuItem((name + " 外す").c_str())) {
			remove_hdd(drive);
		}
		draw_hdd_mount_state(drive);
		ImGui::Separator();
	}
	int in_use = 0;
	for (int drive = 0; drive < SCSIHDD_MAX; drive++) {
		const char *path = hdd_config_path(0x20 | drive);
		if ((path != nullptr) && (path[0] != '\0')) {
			in_use++;
		}
	}
	char heading[48];
	std::snprintf(heading, sizeof(heading), "SCSI (使用中 %d 台)###scsi", in_use);
	if (ImGui::BeginMenu(heading)) {
		for (int drive = 0; drive < SCSIHDD_MAX; drive++) {
			const int encoded = 0x20 | drive;
			const std::string id = std::string("ID ") + std::to_string(drive);
			if (ImGui::BeginMenu(id.c_str())) {
				if (ImGui::MenuItem("開く…")) {
					open_hdd_dialog(encoded);
				}
				draw_history_menu("履歴", HIST_SCSI0 + drive, [encoded](const std::string &path) {
					mount_hdd_path(encoded, path);
				});
				if (ImGui::MenuItem("外す")) {
					remove_hdd(encoded);
				}
				draw_hdd_mount_state(encoded);
				ImGui::EndMenu();
			}
		}
		ImGui::EndMenu();
	}
	ImGui::Separator();
	if (ImGui::MenuItem("新しい SASI イメージ…")) {
		open_new_sasi_dialog(0);
	}
	if (ImGui::MenuItem("新しい SCSI イメージ…")) {
		open_new_scsi_dialog(0);
	}
	ImGui::EndMenu();
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

static void set_display_rotation(int rotation) {
	np2oscfg.gui_rotate = static_cast<BYTE>(rotation);
	scrnmng_set_rotation(np2oscfg.gui_rotate);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_display_effect(int effect) {
	np2oscfg.gui_effect = static_cast<BYTE>(effect);
	scrnmng_set_effect(np2oscfg.gui_effect);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void load_native_crt_preset(void) {
	std::string error;

	if (np2oscfg.gui_shader_preset[0] == '\0') {
		milstr_ncpy(np2oscfg.gui_shader_preset, VAEG_DEFAULT_SHADER_PRESET,
		            sizeof(np2oscfg.gui_shader_preset));
	}
	if (!g_gui.native_crt_preset.load(scrnmng_native_preset_path(), &error)) {
		g_gui.native_crt_loaded_path.clear();
		g_gui.native_crt_status = "Preset load failed: ";
		g_gui.native_crt_status += error;
		return;
	}
	g_gui.native_crt_loaded_path = np2oscfg.gui_shader_preset;
	if (!g_gui.native_crt_preset.parameters().load_config()) {
		g_gui.native_crt_status = "Preset loaded; parameter state is invalid";
	} else {
		g_gui.native_crt_status = "Preset loaded";
	}
}

static void set_native_crt_mode(bool filtered) {
	if (scrnmng_native_active()) {
		if (!scrnmng_set_native_filter(filtered ? TRUE : FALSE)) {
			g_gui.native_crt_status = filtered ? "CRT unavailable; reload preset to retry"
			                                   : "Unfiltered librashader mode unavailable";
			return;
		}
		np2oscfg.gui_native_crt = 1;
		np2oscfg.gui_native_filter = filtered ? 1 : 0;
		g_gui.native_crt_settings_open = false;
		g_gui.native_crt_status = filtered ? "CRT enabled" : "Librashader unfiltered";
	} else {
		np2oscfg.gui_native_crt = 1;
		np2oscfg.gui_native_filter = filtered ? 1 : 0;
		scrnmng_request_native_crt(TRUE, TRUE);
#if defined(_WIN32) && defined(VAEG_ENABLE_LIBRASHADER)
		g_gui.native_crt_status =
		    filtered ? "Applying CRT selection" : "Applying unfiltered selection";
#else
		g_gui.native_crt_status = filtered ? "librashader selected; restart to apply"
		                                   : "unfiltered librashader selected; restart to apply";
#endif
	}
	sysmng_update(SYS_UPDATEOSCFG);
}

static void draw_renderer_selection(void) {
	bool enabled = scrnmng_native_active() != FALSE;
	bool filtered = enabled && scrnmng_native_filter_enabled() != FALSE;
	if (ImGui::BeginMenu("描画方式")) {
		if (ImGui::MenuItem("標準（SDL）", nullptr, !enabled)) {
			np2oscfg.gui_native_filter = 1;
			scrnmng_request_native_crt(FALSE, TRUE);
			sysmng_update(SYS_UPDATEOSCFG);
			g_gui.native_crt_status = "SDL selected";
		}
		if (ImGui::MenuItem("CRT効果（librashader）", nullptr, filtered)) {
			set_native_crt_mode(true);
		}
		if (ImGui::MenuItem("加工なし（librashader）", nullptr, enabled && !filtered)) {
			set_native_crt_mode(false);
		}
		ImGui::EndMenu();
	}
	const char *renderer_status = scrnmng_native_status();
	if (std::strcmp(renderer_status, "SDL") != 0 &&
	    std::strcmp(renderer_status, "Native CRT ON") != 0) {
		ImGui::TextWrapped("描画状態: %s", renderer_status);
	}
#if !defined(_WIN32) || !defined(VAEG_ENABLE_LIBRASHADER)
	if (np2oscfg.gui_native_crt && !enabled) {
		ImGui::TextWrapped("librashader requested; restart required (if supported by this build)");
	}
#endif
	if (enabled && filtered) {
		if (ImGui::MenuItem("CRT設定…")) {
			g_gui.native_crt_settings_open = true;
		}
	}
}

static void draw_native_crt_settings(void) {
	if (!g_gui.native_crt_settings_open || !scrnmng_native_filter_enabled()) {
		return;
	}
	prepare_dialog(35.0f, 26.25f, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("CRT設定###Shader settings", &g_gui.native_crt_settings_open)) {
		ImGui::End();
		return;
	}
	ImGui::TextWrapped("Active: %s", scrnmng_native_status());
	ImGui::Separator();
	if (ImGui::InputText("Preset path", np2oscfg.gui_shader_preset,
	                     sizeof(np2oscfg.gui_shader_preset))) {
		g_gui.native_crt_status = "Preset path changed; press Reload";
		sysmng_update(SYS_UPDATEOSCFG);
	}
	if (ImGui::Button("Reload preset")) {
		load_native_crt_preset();
		if (scrnmng_native_active())
			scrnmng_request_native_crt(TRUE, TRUE);
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear status")) {
		g_gui.native_crt_status.clear();
	}
	if (g_gui.native_crt_loaded_path != np2oscfg.gui_shader_preset) {
		ImGui::TextDisabled("Preset is not loaded");
	} else {
		vaeg::librashader::ShaderParameterSet &parameters =
		    g_gui.native_crt_preset.parameters();
		vaeg::librashader::ShaderParameterInfo *mask_parameter = nullptr;
		for (std::size_t i = 0; i < parameters.size(); ++i) {
			vaeg::librashader::ShaderParameterInfo *parameter = parameters.at(i);
			if ((parameter != nullptr) && (parameter->name == "MASK_INTENSITY")) {
				mask_parameter = parameter;
				break;
			}
		}
		if (mask_parameter != nullptr) {
			if (np2oscfg.gui_native_mask_auto) {
				mask_parameter->value = scrnmng_native_mask_intensity();
			}
			bool auto_mask = np2oscfg.gui_native_mask_auto != 0;
			if (ImGui::Checkbox("RGB mask: Auto by window scale", &auto_mask)) {
				np2oscfg.gui_native_mask_auto = auto_mask ? 1 : 0;
				if (auto_mask) {
					mask_parameter->value = scrnmng_native_mask_intensity();
				}
				scrnmng_apply_native_mask_profile();
				if (!parameters.save_config()) {
					g_gui.native_crt_status = "RGB mask mode save failed";
				} else {
					g_gui.native_crt_status = auto_mask ? "RGB mask automatic"
					                                  : "RGB mask manual";
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
		}
		for (std::size_t i = 0; i < parameters.size(); ++i) {
			vaeg::librashader::ShaderParameterInfo *parameter = parameters.at(i);
			if (parameter == nullptr) {
				continue;
			}
			float value = parameter->value;
			ImGui::PushID(static_cast<int>(i));
			const char *format = parameter->name == "SCREEN_SIZE" ? "%.2f" : "%.3f";
			if (ImGui::SliderFloat(parameter->name.c_str(), &value, parameter->minimum,
			                       parameter->maximum, format)) {
				(void)parameters.set_value_at(i, value, nullptr);
				if (parameter->name == "MASK_INTENSITY") {
					np2oscfg.gui_native_mask_auto = 0;
				}
				if (scrnmng_native_active() &&
				    scrnmng_native_set_parameter(parameter->name.c_str(), parameter->value) !=
				        SUCCESS) {
					g_gui.native_crt_status = "Live parameter update failed";
				} else if (!parameters.save_config()) {
					g_gui.native_crt_status = "Parameter save failed";
				} else {
					g_gui.native_crt_status = "Parameter updated";
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
			if (!parameter->description.empty()) {
				ImGui::TextDisabled("%s", parameter->description.c_str());
			}
			ImGui::PopID();
		}
		if (ImGui::Button("Reset parameters")) {
			np2oscfg.gui_native_mask_auto = 1;
			parameters.reset();
			if (mask_parameter != nullptr) {
				mask_parameter->value = scrnmng_native_mask_intensity();
			}
			if (scrnmng_native_active()) {
				for (std::size_t i = 0; i < parameters.size(); ++i) {
					const auto *parameter = parameters.at(i);
					(void)scrnmng_native_set_parameter(parameter->name.c_str(), parameter->value);
				}
			}
			if (!parameters.save_config()) {
				g_gui.native_crt_status = "Parameter reset save failed";
			} else {
				g_gui.native_crt_status = "Parameters reset";
				sysmng_update(SYS_UPDATEOSCFG);
			}
		}
	}
	if (!g_gui.native_crt_status.empty()) {
		ImGui::TextDisabled("%s", g_gui.native_crt_status.c_str());
	}
	ImGui::End();
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
	if (scrnmng_set_display_mode(mode, np2oscfg.gui_monitor, width, height, refresh, fscrnmod) ==
	    SUCCESS) {
		np2oscfg.gui_display_mode = static_cast<BYTE>(mode);
		if (mode == VAEG_DISPLAY_EXCLUSIVE) {
			np2oscfg.fscrn_cx = 0;
			np2oscfg.fscrn_cy = 0;
			np2oscfg.gui_fullscreen_refresh = 0;
			np2oscfg.fscrnmod = fscrnmod;
		}
	} else {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Display mode change failed: %s", SDL_GetError());
		np2oscfg.gui_display_mode = VAEG_DISPLAY_WINDOWED;
	}
	scrnmng_set_menu_height(scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE ? 0 : menu_bar_height());
	sysmng_update(SYS_UPDATEOSCFG);
}

static void open_custom_size_dialog(void) {
	g_gui.custom_size_integer = true;
	g_gui.pending_window_scale = np2oscfg.gui_scale ? np2oscfg.gui_scale : 1;
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
	prepare_dialog(23.75f, 12.5f, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Custom window size##display", &g_gui.custom_size_open,
	                           ImGuiWindowFlags_NoResize)) {
		if (ImGui::RadioButton("Integer multiplier", g_gui.custom_size_integer)) {
			g_gui.custom_size_integer = true;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Pixels", !g_gui.custom_size_integer)) {
			g_gui.custom_size_integer = false;
		}
		if (g_gui.custom_size_integer) {
			ImGui::InputInt("Multiplier (x)", &g_gui.pending_window_scale);
		} else {
			ImGui::InputInt("Logical width", &g_gui.pending_window_width);
			ImGui::InputInt("Logical height", &g_gui.pending_window_height);
		}
		const bool valid =
		    g_gui.custom_size_integer
		        ? (g_gui.pending_window_scale >= 1 &&
		           g_gui.pending_window_scale <= VAEG_WINDOW_SCALE_MAX)
		        : ((g_gui.pending_window_width >= 320) && (g_gui.pending_window_width <= 7680) &&
		           (g_gui.pending_window_height >= 240) && (g_gui.pending_window_height <= 4320));
		if (!valid) {
			if (g_gui.custom_size_integer) {
				ImGui::Text("Multiplier must be 1-%d.", VAEG_WINDOW_SCALE_MAX);
			} else {
				ImGui::TextUnformatted("Size must be between 320x240 and 7680x4320.");
			}
		}
		ImGui::BeginDisabled(!valid || (scrnmng_get_display_mode() != VAEG_DISPLAY_WINDOWED));
		if (ImGui::Button("Apply")) {
			if (g_gui.custom_size_integer) {
				set_display_scale(g_gui.pending_window_scale);
			} else {
				// The size excludes the front panel, which is added below it.
				SDL_SetWindowSize(static_cast<SDL_Window *>(scrnmng_get_window()),
				                  g_gui.pending_window_width,
				                  g_gui.pending_window_height + scrnmng_front_panel_applied());
				np2oscfg.gui_window_width = static_cast<UINT16>(g_gui.pending_window_width);
				np2oscfg.gui_window_height = static_cast<UINT16>(g_gui.pending_window_height);
				np2oscfg.gui_scale = 0;
			}
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
	prepare_dialog(22.5f, 9.0625f, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Custom sound buffer##sound", &g_gui.sound_buffer_open,
	                           ImGuiWindowFlags_NoResize)) {
		ImGui::InputInt("Buffer length (ms)", &g_gui.pending_sound_buffer_ms);
		const bool valid =
		    vaeg_sound_buffer_valid(static_cast<UINT>(g_gui.pending_sound_buffer_ms)) != FALSE;
		if (!valid) {
			ImGui::TextUnformatted("Buffer length must be between 40 and 1000 ms.");
		}
		ImGui::BeginDisabled(!valid);
		if (ImGui::Button("Apply")) {
			const UINT delayms = static_cast<UINT>(g_gui.pending_sound_buffer_ms);

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
	prepare_dialog(47.5f, 32.5f, ImGuiCond_FirstUseEver);
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
		                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		                          ImGuiTableFlags_ScrollY,
		                      ImVec2(0.0f, 390.0f))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Role");
			ImGui::TableSetupColumn("Guest / action");
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
				if (entry->guest_code == KBDMAP_NC) {
					std::snprintf(guest, sizeof(guest), "host");
				} else {
					std::snprintf(guest, sizeof(guest), "0x%02x",
					              static_cast<unsigned int>(entry->guest_code));
				}
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(entry->label);
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(guest);
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(binding_name(kbdmap_binding(i)));
				ImGui::TableSetColumnIndex(3);
				ImGui::TextUnformatted(kbdmap_status_name(kbdmap_binding_status(i)));
				ImGui::TableSetColumnIndex(4);
				ImGui::PushID(i);
				if (g_gui.capture_binding == i) {
					ImGui::Button("...");
				} else if (ImGui::Button("Set")) {
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

static bool font_preset_path(const char *filename, char *path, size_t path_size) {
	short attr;

	getbiospath(path, filename, static_cast<int>(path_size));
	attr = file_attr(path);
	return (attr != static_cast<short>(-1)) && ((attr & FILEATTR_DIRECTORY) == 0);
}

static void load_font_preset(const char *filename) {
	char path[MAX_PATH];
	FILEH fh;
	UINT size;

	if (!font_preset_path(filename, path, sizeof(path))) {
		g_gui.font_status = "Font not found: ";
		g_gui.font_status += filename;
		return;
	}
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		g_gui.font_status = "Font open failed: ";
		g_gui.font_status += filename;
		return;
	}
	size = file_getsize(fh);
	file_close(fh);
	if (size != kV98FontRomSize) {
		g_gui.font_status = "Font size mismatch: ";
		g_gui.font_status += filename;
		return;
	}
	if (!romva_load_pc98_font(path)) {
		g_gui.font_status = "Font load failed: ";
		g_gui.font_status += filename;
		return;
	}
	file_cpyname(np2cfg.fontfile, filename, sizeof(np2cfg.fontfile));
	pccore_redraw();
	sysmng_update(SYS_UPDATECFG);
	g_gui.font_status = "Font loaded: ";
	g_gui.font_status += filename;
}

static void load_default_va_font(void) {
	const char *filename = romva_default_font_filename();

	if (!romva_load_default_font()) {
		g_gui.font_status = "VA font load failed: ";
		g_gui.font_status += filename;
		return;
	}
	np2cfg.fontfile[0] = '\0';
	pccore_redraw();
	sysmng_update(SYS_UPDATECFG);
	g_gui.font_status = "Font loaded: ";
	g_gui.font_status += filename;
}

static void draw_screen_menu(void) {
	if (ImGui::BeginMenu("画面")) {
		const char *screenshot_shortcut =
		    (np2oscfg.F12KEY == KBDMAP_F12_SCREENSHOT) ? "PrintScreen / F12" : "PrintScreen";
		if (ImGui::MenuItem("スクリーンショットを保存", screenshot_shortcut)) {
			save_screenshot(FALSE);
		}
		if (ImGui::MenuItem("スクリーンショットを保存（加工前）")) {
			save_screenshot(TRUE);
		}
		if (!g_gui.screenshot_status.empty()) {
			ImGui::TextDisabled("%s", g_gui.screenshot_status.c_str());
		}
		ImGui::Separator();
		draw_renderer_selection();
		if (!scrnmng_native_active() && ImGui::BeginMenu("エフェクト")) {
			static const char *labels[] = {"加工なし", "線形補間", "走査線", "CRT 簡易"};
			for (int value = 0; value < VAEG_EFFECT_COUNT; value++) {
				if (ImGui::MenuItem(labels[value], nullptr, np2oscfg.gui_effect == value)) {
					set_display_effect(value);
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("拡大方法")) {
			static const char *labels[] = {"等倍", "合わせる", "合わせる (8 ドット単位)", "整数倍",
			                               "引き伸ばし"};
			for (int value = 0; value < VAEG_SCALING_COUNT; value++) {
				if (ImGui::MenuItem(labels[value], nullptr, np2oscfg.gui_scaling == value)) {
					set_display_scaling(value);
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("ウィンドウサイズ")) {
			if (ImGui::MenuItem("等倍", nullptr, np2oscfg.gui_scale == 1)) {
				set_display_scale(1);
			}
			if (ImGui::MenuItem("2倍", nullptr, np2oscfg.gui_scale == 2)) {
				set_display_scale(2);
			}
			if (ImGui::MenuItem("3倍", nullptr, np2oscfg.gui_scale == 3)) {
				set_display_scale(3);
			}
			if (ImGui::MenuItem("4倍", nullptr, np2oscfg.gui_scale == 4)) {
				set_display_scale(4);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("任意…", nullptr, np2oscfg.gui_scale == 0 || np2oscfg.gui_scale > 4,
			                    scrnmng_get_display_mode() == VAEG_DISPLAY_WINDOWED)) {
				open_custom_size_dialog();
			}
			ImGui::EndMenu();
		}
		bool aspect = np2oscfg.gui_aspect != 0;
		if (ImGui::MenuItem("縦横比の補正", nullptr, aspect)) {
			set_display_aspect(!aspect);
		}
		const bool rotated = np2oscfg.gui_rotate == VAEG_ROTATE_RIGHT;
		if (ImGui::MenuItem("右に 90° 回転", nullptr, rotated)) {
			set_display_rotation(rotated ? VAEG_ROTATE_NONE : VAEG_ROTATE_RIGHT);
		}
		const bool fullscreen = scrnmng_get_display_mode() != VAEG_DISPLAY_WINDOWED;
		if (ImGui::MenuItem("全画面表示", nullptr, fullscreen)) {
			set_display_mode(fullscreen ? VAEG_DISPLAY_WINDOWED : VAEG_DISPLAY_EXCLUSIVE);
		}
		ImGui::Separator();
		// The monitor's horizontal frequency (DIP switch SW1, read by the ROM
		// at reset) and how its picture is shown.
		if (ImGui::BeginMenu(np2cfg.monitor_15khz ? "モニタ: 15 kHz" : "モニタ: 24 kHz")) {
			if (ImGui::MenuItem("24 kHz (リセット)", nullptr, np2cfg.monitor_15khz == 0)) {
				select_monitor(false);
			}
			if (ImGui::MenuItem("15 kHz (リセット)", nullptr, np2cfg.monitor_15khz != 0)) {
				select_monitor(true);
			}
			ImGui::Separator();
			// Analog or digital RGB output; display only, no reset.
			if (ImGui::MenuItem("デジタル RGB (8色)", nullptr, np2cfg.monitor_digital != 0)) {
				np2cfg.monitor_digital = np2cfg.monitor_digital ? 0 : 1;
				sysmng_update(SYS_UPDATECFG);
				scrndrawva_redraw();
			}
			ImGui::Separator();
			// 15 kHz scanlines: gaps as on a CRT, or filled as on an LCD.
			if (ImGui::MenuItem("15 kHz 走査線: 隙間あり (CRT)", nullptr,
			                    np2cfg.monitor_15khz_fill == 0)) {
				np2cfg.monitor_15khz_fill = 0;
				sysmng_update(SYS_UPDATECFG);
				pccore_redraw();
			}
			if (ImGui::MenuItem("15 kHz 走査線: 埋める (液晶)", nullptr,
			                    np2cfg.monitor_15khz_fill != 0)) {
				np2cfg.monitor_15khz_fill = 1;
				sysmng_update(SYS_UPDATECFG);
				pccore_redraw();
			}
			ImGui::EndMenu();
		}
		// V1/V2 at 24 kHz: the VA's own picture or a PC-8801-style one (8x8
		// text doubled, 200-line graphics with scanline gaps).
		if (ImGui::BeginMenu("V1/V2 表示")) {
			if (ImGui::MenuItem("VA 標準", nullptr, np2cfg.v1v2_8801_display == 0)) {
				np2cfg.v1v2_8801_display = 0;
				sysmng_update(SYS_UPDATECFG);
				pccore_redraw();
			}
			if (ImGui::MenuItem("PC-8801 風", nullptr, np2cfg.v1v2_8801_display != 0)) {
				np2cfg.v1v2_8801_display = 1;
				sysmng_update(SYS_UPDATECFG);
				pccore_redraw();
			}
			ImGui::EndMenu();
		}
		// Front panel below the screen: FDD access and V1/V2/V3 mode lamps.
		if (ImGui::MenuItem("前面パネル (ランプ)", nullptr,
		                    np2oscfg.front_panel != FRONTPANEL_OFF)) {
			np2oscfg.front_panel =
			    (np2oscfg.front_panel != FRONTPANEL_OFF) ? FRONTPANEL_OFF : FRONTPANEL_SIMPLE;
			sysmng_update(SYS_UPDATEOSCFG);
			scrnmng_front_panel_changed();
		}
		if (ImGui::BeginMenu("フレームスキップ")) {
			static const char *labels[] = {"自動", "全フレーム", "1/2 フレーム", "1/3 フレーム",
			                               "1/4 フレーム"};
			for (int value = 0; value < static_cast<int>(std::size(labels)); value++) {
				if (ImGui::MenuItem(labels[value], nullptr, np2oscfg.DRAW_SKIP == value)) {
					np2oscfg.DRAW_SKIP = static_cast<BYTE>(value);
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("フォント")) {
			char path[MAX_PATH];
			const char *va_filename = romva_default_font_filename();
			const bool va_available = font_preset_path(va_filename, path, sizeof(path));
			const bool va_selected = np2cfg.fontfile[0] == '\0';
			if (ImGui::MenuItem("VA 標準", nullptr, va_selected, va_available)) {
				load_default_va_font();
			}
			if (!va_available) {
				ImGui::TextDisabled("ROM ディレクトリに %s がありません", va_filename);
			}
			const bool pc98_available = font_preset_path(pc98fontromname, path, sizeof(path));
			const bool pc98_selected =
			    np2cfg.fontfile[0] && !file_cmpname(file_getname(np2cfg.fontfile), pc98fontromname);
			if (ImGui::MenuItem("98font 互換", nullptr, pc98_selected, pc98_available)) {
				load_font_preset(pc98fontromname);
			}
			if (!pc98_available) {
				ImGui::TextDisabled("ROM ディレクトリに 98font.rom がありません");
			}
			ImGui::EndMenu();
		}
		if (!g_gui.font_status.empty()) {
			ImGui::TextDisabled("%s", g_gui.font_status.c_str());
		}
		ImGui::EndMenu();
	}
}

/* カーソル割り当て: what the host cursor keys send. The tenkey overlay and
 * the joystick and mouse key modes all take the cursor keys, so they are one
 * choice. */
enum {
	CURSOR_NORMAL = 0,
	CURSOR_TENKEY,
	CURSOR_JOYSTICK,
	CURSOR_MOUSE
};

static int cursor_assignment(void) {
	if (np2cfg.KEY_MODE == 1) {
		return CURSOR_JOYSTICK;
	}
	if (np2cfg.KEY_MODE == 3) {
		return CURSOR_MOUSE;
	}
	return kbdmap_tenkey_overlay_enabled() ? CURSOR_TENKEY : CURSOR_NORMAL;
}

static void set_cursor_assignment(int assignment) {
	set_key_mode((assignment == CURSOR_JOYSTICK) ? 1 : (assignment == CURSOR_MOUSE) ? 3 : 0);
	kbdmap_set_tenkey_overlay((assignment == CURSOR_TENKEY) ? TRUE : FALSE);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void draw_key_menu(void) {
	if (!ImGui::BeginMenu("Key")) {
		return;
	}
	if (ImGui::BeginMenu("カーソル割り当て")) {
		static const char *labels[] = {"通常", "テンキー 8/4/6/2", "ジョイスティック", "マウス"};
		const int current = cursor_assignment();
		for (int i = 0; i < static_cast<int>(std::size(labels)); i++) {
			if (ImGui::MenuItem(labels[i], nullptr, current == i)) {
				set_cursor_assignment(i);
			}
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("ホストの配列")) {
		if (ImGui::MenuItem("JIS", nullptr, std::string(kbdmap_layout_name()) == "jis")) {
			set_keyboard_layout("jis");
		}
		if (ImGui::MenuItem("US", nullptr, std::string(kbdmap_layout_name()) == "us")) {
			set_keyboard_layout("us");
		}
		if (ImGui::MenuItem("カスタム", nullptr, std::string(kbdmap_layout_name()) == "custom")) {
			set_keyboard_layout("custom");
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("かな入力")) {
		if (ImGui::MenuItem("JIS かな", nullptr,
		                    std::string(kbdmap_kana_input_name()) == "jis-kana")) {
			set_kana_input("jis-kana");
		}
		if (ImGui::MenuItem("ローマ字かな", nullptr,
		                    std::string(kbdmap_kana_input_name()) == "roman")) {
			set_kana_input("roman");
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("F12 の割り当て")) {
		if (ImGui::MenuItem("マウス", nullptr, np2oscfg.F12KEY == 0)) {
			set_f12_key(0);
		}
		if (ImGui::MenuItem("COPY", nullptr, np2oscfg.F12KEY == 1)) {
			set_f12_key(1);
		}
		if (ImGui::MenuItem("STOP", nullptr, np2oscfg.F12KEY == 2)) {
			set_f12_key(2);
		}
		if (ImGui::MenuItem("テンキー =", nullptr, np2oscfg.F12KEY == 3)) {
			set_f12_key(3);
		}
		if (ImGui::MenuItem("テンキー ,", nullptr, np2oscfg.F12KEY == 4)) {
			set_f12_key(4);
		}
		if (ImGui::MenuItem("PC キー", nullptr, np2oscfg.F12KEY == 5)) {
			set_f12_key(5);
		}
		if (ImGui::MenuItem("最大速度", nullptr, np2oscfg.F12KEY == KBDMAP_F12_FULL_SPEED)) {
			set_f12_key(KBDMAP_F12_FULL_SPEED);
		}
		if (ImGui::MenuItem("スクリーンショット", nullptr,
		                    np2oscfg.F12KEY == KBDMAP_F12_SCREENSHOT)) {
			set_f12_key(KBDMAP_F12_SCREENSHOT);
		}
		if (ImGui::MenuItem("一時停止", nullptr, np2oscfg.F12KEY == KBDMAP_F12_PAUSE)) {
			set_f12_key(KBDMAP_F12_PAUSE);
		}
		if (ImGui::MenuItem("クイックセーブ (Shift+F12 で読み込み)", nullptr,
		                    np2oscfg.F12KEY == KBDMAP_F12_QUICKSAVE)) {
			set_f12_key(KBDMAP_F12_QUICKSAVE);
		}
		ImGui::EndMenu();
	}
	if (ImGui::MenuItem("キーの割り当て…")) {
		g_gui.keyboard_config_open = true;
	}
	ImGui::Separator();
	bool capture = np2oscfg.MOUSE_SW != 0;
	const char *capture_shortcut = (np2oscfg.F12KEY == 0) ? "F12 / 中ボタン" : "中ボタン";
	if (ImGui::MenuItem("マウスを捕捉", capture_shortcut, capture)) {
		set_mouse_capture(!capture);
	}
	if (ImGui::BeginMenu("コントローラポート")) {
		if (ImGui::MenuItem("ジョイスティック", nullptr, mouseifvacfg.device == MOUSEIFVA_JOYPAD)) {
			set_mouse_device(MOUSEIFVA_JOYPAD);
		}
		if (ImGui::MenuItem("マウス", nullptr, mouseifvacfg.device == MOUSEIFVA_MOUSE)) {
			set_mouse_device(MOUSEIFVA_MOUSE);
		}
		ImGui::EndMenu();
	}
	bool rapid = np2cfg.MOUSERAPID != 0;
	if (ImGui::MenuItem("連射ボタン", nullptr, rapid)) {
		set_mouse_rapid(!rapid);
	}
	if (mousemng_status()[0] != '\0') {
		ImGui::TextWrapped("%s", mousemng_status());
	}

	ImGui::EndMenu();
}

static void draw_sound_menu(void) {
	if (!ImGui::BeginMenu("音")) {
		return;
	}
	// 全体の音量: the final mix (M110); the sources' balance is below.
	int master = np2oscfg.master_volume;
	ImGui::TextUnformatted("全体の音量");
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
	if (ImGui::SliderInt("##master-volume", &master, 0, 100, "%d%%",
	                     ImGuiSliderFlags_AlwaysClamp)) {
		np2oscfg.master_volume = static_cast<BYTE>(master);
		soundmng_setmastervolume(np2oscfg.master_volume);
		sysmng_update(SYS_UPDATEOSCFG);
	}
	const bool muted = soundmng_isenabled() ? false : true;
	if (ImGui::MenuItem("ミュート", nullptr, muted)) {
		np2oscfg.sound_enabled = muted ? 1 : 0;
		soundmng_setenabled(np2oscfg.sound_enabled ? TRUE : FALSE);
		sysmng_update(SYS_UPDATEOSCFG);
	}
	if (ImGui::BeginMenu("個別の音量")) {
		draw_source_volumes();
		ImGui::EndMenu();
	}
	ImGui::Separator();
	if (ImGui::BeginMenu("FM 音源ボード")) {
		const bool va1 = milstr_cmp(np2cfg.model, str_VA1) == 0;
		if (va1 && ImGui::MenuItem("OPN", nullptr, np2cfg.SOUND_SW == FMBOARD_VA_OPN)) {
			select_sound_hardware(FMBOARD_VA_OPN);
		}
		if (ImGui::MenuItem("OPNA (VA2/VA3、VA + サウンドボード II)", nullptr,
		                    np2cfg.SOUND_SW == FMBOARD_VA_OPNA)) {
			select_sound_hardware(FMBOARD_VA_OPNA);
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("FM の方式")) {
		const UINT backend = opngen_getbackend();
		if (ImGui::MenuItem("ymfm", nullptr, backend == OPN_BACKEND_YMFM)) {
			select_opn_backend(OPN_BACKEND_YMFM);
		}
		if (ImGui::MenuItem("NP2", nullptr, backend == OPN_BACKEND_NP2)) {
			select_opn_backend(OPN_BACKEND_NP2);
		}
		ImGui::EndMenu();
	}
	if ((opngen_getbackend() == OPN_BACKEND_YMFM) && ImGui::BeginMenu("ymfm の精度")) {
		const UINT fidelity = ymfm_opn_getfidelity();
		if (ImGui::MenuItem("最小 (約 166 kHz、既定)", nullptr,
		                    fidelity == YMFMBRIDGE_FIDELITY_MINIMUM)) {
			select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MINIMUM);
		}
		if (ImGui::MenuItem("中 (約 333 kHz)", nullptr, fidelity == YMFMBRIDGE_FIDELITY_MEDIUM)) {
			select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MEDIUM);
		}
		if (ImGui::MenuItem("最大 (約 998 kHz、高負荷)", nullptr,
		                    fidelity == YMFMBRIDGE_FIDELITY_MAXIMUM)) {
			select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MAXIMUM);
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("サンプリング周波数")) {
		if (ImGui::MenuItem("11.025 kHz", nullptr, np2cfg.samplingrate == 11025)) {
			select_sampling_rate(11025);
		}
		if (ImGui::MenuItem("22.05 kHz", nullptr, np2cfg.samplingrate == 22050)) {
			select_sampling_rate(22050);
		}
		if (ImGui::MenuItem("44.1 kHz (推奨)", nullptr, np2cfg.samplingrate == 44100)) {
			select_sampling_rate(44100);
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("バッファ")) {
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
		std::snprintf(custom_label, sizeof(custom_label), "任意… (%u ms)", np2cfg.delayms);
		if (ImGui::MenuItem(custom_label, nullptr, !preset_selected)) {
			open_sound_buffer_dialog();
		}
		ImGui::EndMenu();
	}
	bool motor = np2cfg.MOTOR != 0;
	if (ImGui::MenuItem("シーク音", nullptr, motor)) {
		np2cfg.MOTOR = motor ? 0 : 1;
		if (np2cfg.MOTOR == 0) {
			fddmtrsnd_stop();
		}
		sysmng_update(SYS_UPDATECFG);
	}
	ImGui::EndMenu();
}

static void open_about_dialog(void) {
	g_gui.about_open = true;
	g_gui.about_request = true;
	g_gui.about_more = false;
	g_gui.about_info[0] = '\0';
}

static void set_info_layer(UINT layer, bool enabled) {
	scrndrawva_set_layer_enabled(layer, enabled ? TRUE : FALSE);
	pccore_redraw();
}

/* While paused (F12 assigned to 一時停止), say so and offer to resume, as
 * the menu bar no longer has a Pause button (M110). */
static void draw_pause_overlay(void) {
	if (!taskmng_ispaused()) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowBgAlpha(0.85f);
	if (ImGui::Begin("##paused", nullptr,
	                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
	                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
	                     ImGuiWindowFlags_NoNav)) {
		ImGui::TextUnformatted("一時停止中");
		if (ImGui::Button("再開")) {
			taskmng_toggle_pause();
		}
	}
	ImGui::End();
}

static void draw_info_menu(void) {
	// 情報 alone at the right end of the menu bar, or right after the other
	// menus when the window is too narrow.
	const ImGuiStyle &style = ImGui::GetStyle();
	const float width = ImGui::CalcTextSize("情報").x + style.ItemSpacing.x * 2.0f;
	const float right = ImGui::GetWindowContentRegionMax().x - width;
	if (right > ImGui::GetCursorPosX()) {
		ImGui::SetCursorPosX(right);
	}
	if (ImGui::BeginMenu("情報")) {
		bool show_fdd = (np2oscfg.DISPCLK & VAEG_DISPINFO_FDD) != 0;
		if (ImGui::MenuItem("FDD を表示", nullptr, show_fdd)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_FDD;
			scrnmng_refresh_title();
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_fps = (np2oscfg.DISPCLK & VAEG_DISPINFO_FPS) != 0;
		if (ImGui::MenuItem("FPS を表示", nullptr, show_fps)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_FPS;
			scrnmng_refresh_title();
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_cpu_clock = (np2oscfg.DISPCLK & VAEG_DISPINFO_CPU_CLOCK) != 0;
		if (ImGui::MenuItem("CPU クロックを表示", nullptr, show_cpu_clock)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_CPU_CLOCK;
			scrnmng_refresh_title();
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_sgp_clock = (np2oscfg.DISPCLK & VAEG_DISPINFO_SGP_CLOCK) != 0;
		if (ImGui::MenuItem("SGP クロックを表示", nullptr, show_sgp_clock)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_SGP_CLOCK;
			scrnmng_refresh_title();
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_frame = (np2oscfg.DISPCLK & VAEG_DISPINFO_FRAME) != 0;
		if (ImGui::MenuItem("フレームを表示", nullptr, show_frame)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_FRAME;
			scrnmng_set_framedisp((np2oscfg.DISPCLK & VAEG_DISPINFO_FRAME) ? TRUE : FALSE);
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_video = (np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO) != 0;
		if (ImGui::MenuItem("ビデオ情報を表示", nullptr, show_video)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_VIDEO;
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_framebuffer = (np2oscfg.DISPCLK & VAEG_DISPINFO_FRAMEBUFFER) != 0;
		if (ImGui::MenuItem("FB 情報を表示", nullptr, show_framebuffer)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_FRAMEBUFFER;
			sysmng_update(SYS_UPDATEOSCFG);
		}
		ImGui::Separator();
		bool show_text = scrndrawva_layer_enabled(VAEG_VA_LAYER_TEXT) != FALSE;
		if (ImGui::MenuItem("テキストを表示", nullptr, show_text)) {
			set_info_layer(VAEG_VA_LAYER_TEXT, !show_text);
		}
		bool show_sprite = scrndrawva_layer_enabled(VAEG_VA_LAYER_SPRITE) != FALSE;
		if (ImGui::MenuItem("スプライトを表示", nullptr, show_sprite)) {
			set_info_layer(VAEG_VA_LAYER_SPRITE, !show_sprite);
		}
		bool show_graphics0 = scrndrawva_layer_enabled(VAEG_VA_LAYER_GRAPHICS0) != FALSE;
		if (ImGui::MenuItem("グラフィック 0 を表示", nullptr, show_graphics0)) {
			set_info_layer(VAEG_VA_LAYER_GRAPHICS0, !show_graphics0);
		}
		bool show_graphics1 = scrndrawva_layer_enabled(VAEG_VA_LAYER_GRAPHICS1) != FALSE;
		if (ImGui::MenuItem("グラフィック 1 を表示", nullptr, show_graphics1)) {
			set_info_layer(VAEG_VA_LAYER_GRAPHICS1, !show_graphics1);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("About")) {
			open_about_dialog();
		}
		ImGui::EndMenu();
	}
}

static void draw_third_party_licenses(void) {
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float unit = ImGui::GetFontSize();
	ImGui::SetNextWindowSize(ImVec2((std::min)(54.0f * unit, viewport->WorkSize.x * 0.95f),
	                                (std::min)(36.0f * unit, viewport->WorkSize.y * 0.95f)),
	                         ImGuiCond_Appearing);
	bool open = true;
	if (ImGui::BeginPopupModal("Third-party licenses", &open)) {
		static int selected = 0;
		ImGui::TextWrapped("Licenses and source references are embedded in this executable. "
		                   "Optional runtime notices do not indicate that a runtime is loaded.");
		ImGui::Text("Dear ImGui %s; SDL headers %d.%d.%d", IMGUI_VERSION, SDL_MAJOR_VERSION,
		            SDL_MINOR_VERSION, SDL_PATCHLEVEL);
		if (ImGui::BeginCombo("Component", vaeg_licenses[selected].name)) {
			for (int i = 0; i < IM_ARRAYSIZE(vaeg_licenses); ++i) {
				if (ImGui::Selectable(vaeg_licenses[i].name, selected == i))
					selected = i;
			}
			ImGui::EndCombo();
		}
		if (ImGui::Button("Copy text"))
			ImGui::SetClipboardText(vaeg_licenses[selected].text);
		ImGui::SameLine();
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
		const bool long_notice = std::strlen(vaeg_licenses[selected].text) > 65536;
		if (ImGui::BeginChild("license-text", ImVec2(0, 0), ImGuiChildFlags_Borders,
		                      long_notice ? ImGuiWindowFlags_HorizontalScrollbar : 0)) {
			if (!long_notice)
				ImGui::PushTextWrapPos(0.0f);
			ImGui::TextUnformatted(vaeg_licenses[selected].text);
			if (!long_notice)
				ImGui::PopTextWrapPos();
		}
		ImGui::EndChild();
		ImGui::EndPopup();
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
	const float ui_scale = ImGui::GetFontSize() / 16.0f;
	const float width =
	    (std::min)((g_gui.about_more ? 620.0f : 360.0f) * ui_scale, viewport->WorkSize.x * 0.9f);
	const float height =
	    (std::min)((g_gui.about_more ? 740.0f : 395.0f) * ui_scale, viewport->WorkSize.y * 0.9f);
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
	if (ImGui::BeginPopupModal("About...##vaeg", &g_gui.about_open,
	                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		if (g_gui.about_texture != nullptr || g_gui.native_about_texture != nullptr) {
			const float available = ImGui::GetContentRegionAvail().x;
			const float image_width = static_cast<float>(g_gui.about_texture_width);
			const float image_height = static_cast<float>(g_gui.about_texture_height);
			const float scale = (std::min)(1.0f, available / image_width);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (available - image_width * scale) * 0.5f);
			ImGui::Image(g_gui.native_about_texture ? g_gui.native_about_texture->GetTexRef()
			                                        : ImTextureRef(g_gui.about_texture),
			             ImVec2(image_width * scale, image_height * scale));
		} else {
			ImGui::TextUnformatted("88VA Eternal Grafx");
		}

		bool close_about = false;
		if (ImGui::BeginTable("about-footer", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("text", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("buttons", ImGuiTableColumnFlags_WidthFixed, 88.0f);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("88VA Eternal Grafx  %s", VAEGREL_CORE);
			ImGui::TableNextColumn();
			close_about = ImGui::Button("OK", ImVec2(-1.0f, 0.0f));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("PC-88VA emulator core");
			ImGui::TableNextColumn();
			ImGui::BeginDisabled(g_gui.about_more);
			if (ImGui::Button("More >>", ImVec2(-1.0f, 0.0f))) {
				np2info(g_gui.about_info, kAboutInfoTemplate, sizeof(g_gui.about_info), nullptr);
				if (memoryva_n80_exist) {
					const char *id = n80rom_label(memoryva_n80_file);
					milstr_ncat(g_gui.about_info, " (", sizeof(g_gui.about_info));
					milstr_ncat(g_gui.about_info, (id != nullptr) ? id : "unreadable",
					            sizeof(g_gui.about_info));
					milstr_ncat(g_gui.about_info, ")", sizeof(g_gui.about_info));
				}
				if (np2cfg.extra_machine[0] != '\0') {
					milstr_ncat(g_gui.about_info, "\nPC-8801 machine: ", sizeof(g_gui.about_info));
					milstr_ncat(g_gui.about_info,
					            (extrarom_last_error[0] != '\0') ? extrarom_last_error
					                                             : np2cfg.extra_machine,
					            sizeof(g_gui.about_info));
				}
				g_gui.about_more = true;
			}
			ImGui::EndDisabled();
			ImGui::EndTable();
		}

		// PC-8801/8001 and PCG emulation references (M113-M115a).
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::TextWrapped("PC-8801/8001 and PCG-8100/8800: written with reference to X88000 "
		                   "(Manuke, public domain) and QUASI88 (showtime).");
		ImGui::PopStyleColor();
		if (ImGui::Button("Third-party licenses..."))
			ImGui::OpenPopup("Third-party licenses");
		draw_third_party_licenses();

		if (g_gui.about_more) {
			ImGui::SeparatorText("Running VA configuration");
			ImGui::InputTextMultiline("##runtime-info", g_gui.about_info, sizeof(g_gui.about_info),
			                          ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
		}

		if (close_about || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape))) {
			g_gui.about_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// ---- Saved states (M110): states/<software>/slotN.sav with slotN.png

constexpr int kStateThumbnailWidth = 320;

struct StateThumbnail {
	long long stamp = -1;
	SDL_Texture *texture = nullptr;
	ImTextureData *native = nullptr;
	int width = 0;
	int height = 0;
};

static std::map<std::string, StateThumbnail> g_state_thumbnails;

/* The medium that names the software: FDD1, else SASI-1, else the tape. */
static std::string state_media_path(void) {
	const char *path = fdd_diskname(0);

	if ((path == nullptr) || (path[0] == '\0')) {
		path = diskdrv_fname[0];
	}
	if ((path != nullptr) && (path[0] != '\0')) {
		return path;
	}
	path = hdd_config_path(0);
	if ((path != nullptr) && (path[0] != '\0')) {
		return path;
	}
	return g_gui.tape_loaded;
}

/* The software is the one booted: its medium is taken at each reset (and on
 * first use), so a disk swap during play keeps the slots together. */
static void state_capture_software(void) {
	g_gui.state_software_media = state_media_path();
	g_gui.state_software_set = true;
}

/* states/<file name without extension>-<hash of the full path>/, or
 * states/common/ with no medium. The name is kept as written (Japanese
 * included); characters host file systems forbid become '_'. */
static std::string state_directory_name(const std::string &media) {
	if (media.empty()) {
		return "common";
	}
	std::string stem = fs::u8path(media).stem().u8string();
	for (char &c : stem) {
		if ((static_cast<unsigned char>(c) < 0x20) || (std::strchr("<>:\"/\\|?*", c) != nullptr)) {
			c = '_';
		}
	}
	while (!stem.empty() && ((stem.back() == '.') || (stem.back() == ' '))) {
		stem.pop_back();
	}
	if (stem.empty()) {
		stem = "media";
	}
	UINT32 hash = 2166136261u;
	for (unsigned char c : media) {
		hash = (hash ^ c) * 16777619u;
	}
	char suffix[16];
	std::snprintf(suffix, sizeof(suffix), "-%06x", static_cast<unsigned>(hash & 0xffffff));
	return stem + suffix;
}

static std::string state_root(void) {
	char root[MAX_PATH];

	(void)file_getappdir(root, sizeof(root), "states");
	return root;
}

static std::string state_software_dir(void) {
	if (!g_gui.state_software_set) {
		state_capture_software();
	}
	std::string dir =
	    join_path(state_root(), state_directory_name(g_gui.state_software_media).c_str());
	(void)file_dircreate(dir.c_str());
	return dir;
}

static std::string state_slot_file(int slot) {
	char name[32];

	std::snprintf(name, sizeof(name), "slot%d.sav", slot);
	return join_path(state_software_dir(), name);
}

static std::string state_thumbnail_path(const std::string &state) {
	return fs::u8path(state).replace_extension(".png").u8string();
}

/* "MM-DD HH:MM" of a file, or an empty string when it does not exist. */
static std::string state_file_time(const std::string &path) {
	std::error_code ec;
	const fs::path file = fs::u8path(path);

	if (!fs::is_regular_file(file, ec)) {
		return std::string();
	}
	const auto written = fs::last_write_time(file, ec);
	if (ec) {
		return std::string();
	}
	const auto system = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
	    written - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
	const std::time_t seconds = std::chrono::system_clock::to_time_t(system);
	std::tm local{};
#if defined(_WIN32)
	localtime_s(&local, &seconds);
#else
	localtime_r(&seconds, &local);
#endif
	char text[32];
	std::snprintf(text, sizeof(text), "%02d-%02d %02d:%02d", local.tm_mon + 1, local.tm_mday,
	              local.tm_hour, local.tm_min);
	return text;
}

static long long state_file_stamp(const std::string &path) {
	std::error_code ec;
	const fs::path file = fs::u8path(path);
	const auto size = fs::file_size(file, ec);
	if (ec) {
		return -1;
	}
	const auto written = fs::last_write_time(file, ec);
	if (ec) {
		return -1;
	}
	// Nonnegative, so that -1 alone means "no file".
	return (static_cast<long long>(written.time_since_epoch().count()) ^
	        (static_cast<long long>(size) << 20)) &
	       0x7fffffffffffffffLL;
}

static void state_release_thumbnail(StateThumbnail &thumbnail) {
	if (thumbnail.texture != nullptr) {
		SDL_DestroyTexture(thumbnail.texture);
		thumbnail.texture = nullptr;
	}
	if (thumbnail.native != nullptr) {
		ImGui::UnregisterUserTexture(thumbnail.native);
		IM_DELETE(thumbnail.native);
		thumbnail.native = nullptr;
	}
	thumbnail.width = 0;
	thumbnail.height = 0;
}

static void state_release_thumbnails(void) {
	for (auto &entry : g_state_thumbnails) {
		state_release_thumbnail(entry.second);
	}
	g_state_thumbnails.clear();
}

/* The thumbnail of a state as a texture, reloaded when the file changes. */
static const StateThumbnail *state_thumbnail(const std::string &state) {
	const std::string path = state_thumbnail_path(state);
	const long long stamp = state_file_stamp(path);
	StateThumbnail &thumbnail = g_state_thumbnails[path];

	if (thumbnail.stamp == stamp) {
		return ((thumbnail.texture != nullptr) || (thumbnail.native != nullptr)) ? &thumbnail
		                                                                         : nullptr;
	}
	state_release_thumbnail(thumbnail);
	thumbnail.stamp = stamp;
	int width = 0;
	int height = 0;
	BYTE *pixels = nullptr;
	if ((stamp < 0) ||
	    (scrnmng_load_stored_png(path.c_str(), &width, &height, &pixels) != SUCCESS)) {
		return nullptr;
	}
	if (g_gui.native_renderer) {
		auto *texture = IM_NEW(ImTextureData)();
		texture->Create(ImTextureFormat_RGBA32, width, height);
		for (int row = 0; row < height; row++) {
			std::memcpy(texture->GetPixelsAt(0, row), pixels + static_cast<size_t>(row) * width * 4,
			            static_cast<size_t>(width) * 4);
		}
		ImGui::RegisterUserTexture(texture);
		thumbnail.native = texture;
	} else if (g_gui.renderer != nullptr) {
		thumbnail.texture = SDL_CreateTexture(g_gui.renderer, SDL_PIXELFORMAT_RGBA32,
		                                      SDL_TEXTUREACCESS_STATIC, width, height);
		if (thumbnail.texture != nullptr) {
			SDL_UpdateTexture(thumbnail.texture, nullptr, pixels, width * 4);
		}
	}
	std::free(pixels);
	thumbnail.width = width;
	thumbnail.height = height;
	return ((thumbnail.texture != nullptr) || (thumbnail.native != nullptr)) ? &thumbnail : nullptr;
}

static void state_draw_thumbnail(const std::string &state, float width) {
	const StateThumbnail *thumbnail = state_thumbnail(state);
	const float height = width * 0.625f;

	if (thumbnail == nullptr) {
		ImGui::Dummy(ImVec2(width, height));
		return;
	}
	const float scaled = width * static_cast<float>(thumbnail->height) / thumbnail->width;
	ImGui::Image(thumbnail->native ? thumbnail->native->GetTexRef()
	                               : ImTextureRef(thumbnail->texture),
	             ImVec2(width, scaled));
}

static void state_thumbnail_tooltip(const std::string &state) {
	if (ImGui::IsItemHovered() && (state_thumbnail(state) != nullptr)) {
		ImGui::BeginTooltip();
		state_draw_thumbnail(state, ImGui::GetFontSize() * 15.0f);
		ImGui::EndTooltip();
	}
}

/* Write a state and its thumbnail; a failed state is removed. */
static bool state_write(const std::string &path) {
	soundmng_stop();
	const bool ok = statsave_save(path.c_str()) == STATFLAG_SUCCESS;
	if (!ok) {
		file_delete(path.c_str());
	} else {
		const std::string thumbnail = state_thumbnail_path(path);
		(void)scrnmng_save_guest_thumbnail(thumbnail.c_str(), kStateThumbnailWidth);
	}
	soundmng_play();
	return ok;
}

static void state_save(const std::string &path) {
	g_gui.state_status = state_write(path) ? "保存しました: " : "保存できませんでした: ";
	g_gui.state_status += display_file_name(path.c_str());
}

static void state_load(const std::string &path) {
	char error[1024];
	char override_error[1024];

	error[0] = '\0';
	override_error[0] = '\0';
	g_gui.state_force_hostfat_available = false;
	g_gui.state_force_hostfat_path.clear();
	const int ret = statsave_check(path.c_str(), error, sizeof(error));
	if ((ret & ~STATFLAG_DISKCHG) != 0) {
		const int override_ret =
		    statsave_check_hostfat_override(path.c_str(), override_error, sizeof(override_error));
		g_gui.state_status = "読み込めませんでした: ";
		g_gui.state_status += path;
		if (error[0] != '\0') {
			g_gui.state_status += " (";
			g_gui.state_status += error;
			g_gui.state_status += ")";
		}
		if ((override_ret & ~STATFLAG_DISKCHG) == 0) {
			g_gui.state_force_hostfat_available = true;
			g_gui.state_force_hostfat_path = path;
		}
		g_gui.state_error_request = true;
		return;
	}
	taskmng_clear_fast_forward();
	statsave_load(path.c_str());
	sdlkbd_reset_state();
	mousemng_reset();
	scrndrawva_redraw();
	g_gui.state_status = "読み込みました: ";
	g_gui.state_status += display_file_name(path.c_str());
	if ((ret & STATFLAG_DISKCHG) != 0) {
		g_gui.state_status += " (ディスクの違いを無視)";
	}
}

// ---- M125: quick saves, quick1 (newest) to quick5, and the state before
// the last quick load (quickundo), next to the slots of the software.

constexpr int kQuickSlots = 5;
constexpr Uint32 kQuickIntervalMs = 1000;

static std::string quick_file(const std::string &dir, int index) {
	char name[32];

	std::snprintf(name, sizeof(name), "quick%d.sav", index);
	return join_path(dir, name);
}

static std::string quick_undo_file(const std::string &dir) {
	return join_path(dir, "quickundo.sav");
}

static void quick_rename(const std::string &from, const std::string &to) {
	std::error_code ec;
	fs::remove(fs::u8path(to), ec);
	if (fs::exists(fs::u8path(from), ec)) {
		fs::rename(fs::u8path(from), fs::u8path(to), ec);
	}
}

/*
 * Make the freshly written `fresh` (and its .png) quick1: quick5 is
 * dropped, quick1-4 move to quick2-5. Called only after `fresh` was written
 * completely, so a failed save leaves the five quick saves as they were.
 */
static bool quick_rotate(const std::string &dir, const std::string &fresh) {
	std::error_code ec;

	if (!fs::is_regular_file(fs::u8path(fresh), ec)) {
		return false;
	}
	for (int index = kQuickSlots - 1; index >= 1; index--) {
		const std::string from = quick_file(dir, index);
		const std::string to = quick_file(dir, index + 1);
		quick_rename(from, to);
		quick_rename(state_thumbnail_path(from), state_thumbnail_path(to));
	}
	quick_rename(fresh, quick_file(dir, 1));
	quick_rename(state_thumbnail_path(fresh), state_thumbnail_path(quick_file(dir, 1)));
	return fs::is_regular_file(fs::u8path(quick_file(dir, 1)), ec);
}

static void show_osd(const std::string &text) {
	g_gui.osd_text = text;
	g_gui.osd_until = SDL_GetTicks() + 1500;
}

/* One quick save or load per second, so a hurried double press does not
 * push two saves or load twice. */
static bool quick_allowed(void) {
	const Uint32 now = SDL_GetTicks();
	if (g_gui.quick_used && ((now - g_gui.quick_last) < kQuickIntervalMs)) {
		return false;
	}
	g_gui.quick_used = true;
	g_gui.quick_last = now;
	return true;
}

static void quick_save(void) {
	if (!quick_allowed()) {
		return;
	}
	const std::string dir = state_software_dir();
	const std::string fresh = join_path(dir, "quick-new.sav");
	const Uint32 start = SDL_GetTicks();
	const bool ok = state_write(fresh) && quick_rotate(dir, fresh);
	fprintf(stderr, "quick save: %s in %u ms\n", ok ? "done" : "failed",
	        static_cast<unsigned>(SDL_GetTicks() - start));
	g_gui.state_status = ok ? "クイックセーブしました" : "クイックセーブできませんでした";
	show_osd(g_gui.state_status);
}

static void state_load(const std::string &path);

/* Load quick save `index`; the state before it is kept as quickundo. */
static void quick_load(int index) {
	const std::string dir = state_software_dir();
	const std::string path = quick_file(dir, index);
	std::error_code ec;

	if (!fs::is_regular_file(fs::u8path(path), ec)) {
		g_gui.state_status = "クイックセーブがありません";
		show_osd(g_gui.state_status);
		return;
	}
	(void)state_write(quick_undo_file(dir));
	state_load(path);
	show_osd((g_gui.state_status.rfind("読み込みました", 0) == 0)
	             ? "クイックセーブ " + std::to_string(index) + " を読み込みました"
	             : g_gui.state_status);
}

static void quick_undo(void) {
	const std::string dir = state_software_dir();
	const std::string undo = quick_undo_file(dir);
	const std::string held = join_path(dir, "quickundo-held.sav");
	std::error_code ec;

	if (!fs::is_regular_file(fs::u8path(undo), ec)) {
		return;
	}
	// swap: the state now becomes the undo, so a second undo returns
	quick_rename(undo, held);
	quick_rename(state_thumbnail_path(undo), state_thumbnail_path(held));
	(void)state_write(undo);
	state_load(held);
	file_delete(held.c_str());
	file_delete(state_thumbnail_path(held).c_str());
	show_osd("クイックロードの前に戻しました");
}

static void draw_osd(void) {
	if (g_gui.osd_text.empty() || (static_cast<Sint32>(SDL_GetTicks() - g_gui.osd_until) >= 0)) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(
	    ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 8.0f, viewport->WorkPos.y + 8.0f),
	    ImGuiCond_Always, ImVec2(1.0f, 0.0f));
	ImGui::SetNextWindowBgAlpha(0.75f);
	if (ImGui::Begin("##osd", nullptr,
	                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
	                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
	                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs)) {
		ImGui::TextUnformatted(g_gui.osd_text.c_str());
	}
	ImGui::End();
}

static std::string state_software_label(void) {
	if (!g_gui.state_software_set) {
		state_capture_software();
	}
	if (g_gui.state_software_media.empty()) {
		return "共通 (メディアなし)";
	}
	return display_file_name(g_gui.state_software_media.c_str());
}

static void draw_state_slots(bool save) {
	ImGui::TextDisabled("%s", state_software_label().c_str());
	for (int slot = 0; slot < kStateSlots; slot++) {
		const std::string path = state_slot_file(slot);
		const std::string when = state_file_time(path);
		char label[64];
		std::snprintf(label, sizeof(label), "スロット %d", slot);
		if (ImGui::MenuItem(label, when.empty() ? "空き" : when.c_str(), false,
		                    save || !when.empty())) {
			if (save) {
				state_save(path);
			} else {
				state_load(path);
			}
		}
		state_thumbnail_tooltip(path);
	}
}

static void draw_state_menu(void) {
	if (!ImGui::BeginMenu("状態")) {
		return;
	}
	if (ImGui::BeginMenu("保存")) {
		draw_state_slots(true);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("読み込み")) {
		draw_state_slots(false);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("クイックセーブ")) {
		const bool on_f12 = np2oscfg.F12KEY == KBDMAP_F12_QUICKSAVE;
		const std::string dir = state_software_dir();
		if (ImGui::MenuItem("今すぐ保存", on_f12 ? "F12" : nullptr)) {
			g_gui.quick_used = false;
			quick_save();
		}
		ImGui::Separator();
		ImGui::TextDisabled("読み込み (新しい順)");
		for (int index = 1; index <= kQuickSlots; index++) {
			const std::string path = quick_file(dir, index);
			const std::string when = state_file_time(path);
			char label[32];
			std::snprintf(label, sizeof(label), "%d", index);
			const char *shortcut = when.empty() ? "空き" : when.c_str();
			if (ImGui::MenuItem(label, shortcut, false, !when.empty())) {
				quick_load(index);
			}
			state_thumbnail_tooltip(path);
			if ((index == 1) && on_f12 && ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Shift+F12");
			}
		}
		ImGui::Separator();
		const bool undo = !state_file_time(quick_undo_file(dir)).empty();
		if (ImGui::MenuItem("読み込む前の状態に戻す", nullptr, false, undo)) {
			quick_undo();
		}
		state_thumbnail_tooltip(quick_undo_file(dir));
		ImGui::EndMenu();
	}
	if (ImGui::MenuItem("一覧…")) {
		g_gui.state_overview_open = true;
	}
	ImGui::Separator();
	if (ImGui::MenuItem("名前を付けて保存…")) {
		g_gui.state_save_as_name[0] = '\0';
		g_gui.state_save_as_request = true;
	}
	if (ImGui::MenuItem("ファイルから読み込み…")) {
		g_gui.state_browser_dir = state_software_dir();
		g_gui.state_browser_open = true;
	}
	if (!g_gui.state_status.empty()) {
		ImGui::Separator();
		ImGui::TextWrapped("%s", g_gui.state_status.c_str());
	}
	ImGui::EndMenu();
}

/* 一覧: the ten slots of the current software with their thumbnails. */
static void draw_state_overview(void) {
	if (!g_gui.state_overview_open) {
		return;
	}
	prepare_dialog(56.0f, 32.0f, ImGuiCond_Appearing);
	if (ImGui::Begin("状態の一覧###state-overview", &g_gui.state_overview_open)) {
		ImGui::TextUnformatted(state_software_label().c_str());
		ImGui::Separator();
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float width = (std::max)(ImGui::GetFontSize() * 4.0f,
		                               (ImGui::GetContentRegionAvail().x - spacing * 4.0f) / 5.0f);
		for (int slot = 0; slot < kStateSlots; slot++) {
			const std::string path = state_slot_file(slot);
			const std::string when = state_file_time(path);
			ImGui::PushID(slot);
			ImGui::BeginGroup();
			state_draw_thumbnail(path, width);
			ImGui::Text("スロット %d", slot);
			ImGui::TextDisabled("%s", when.empty() ? "空き" : when.c_str());
			if (ImGui::SmallButton("保存")) {
				state_save(path);
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(when.empty());
			if (ImGui::SmallButton("読み込み")) {
				state_load(path);
			}
			ImGui::EndDisabled();
			ImGui::EndGroup();
			ImGui::PopID();
			if ((slot % 5) != 4) {
				ImGui::SameLine();
			}
		}
		if (!g_gui.state_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.state_status.c_str());
		}
	}
	ImGui::End();
}

/* 名前を付けて保存: states/<software>/<name>.sav (and .png). */
static void draw_state_save_as(void) {
	if (g_gui.state_save_as_request) {
		g_gui.state_save_as_request = false;
		g_gui.state_save_as_open = true;
		ImGui::OpenPopup("名前を付けて保存###state-save-as");
	}
	if (!g_gui.state_save_as_open) {
		return;
	}
	prepare_dialog(28.0f, 10.0f, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("名前を付けて保存###state-save-as", &g_gui.state_save_as_open)) {
		ImGui::TextUnformatted(state_software_label().c_str());
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		const bool entered = ImGui::InputText("##state-name", g_gui.state_save_as_name,
		                                      sizeof(g_gui.state_save_as_name),
		                                      ImGuiInputTextFlags_EnterReturnsTrue);
		std::string name = g_gui.state_save_as_name;
		for (char &c : name) {
			if ((static_cast<unsigned char>(c) < 0x20) ||
			    (std::strchr("<>:\"/\\|?*", c) != nullptr)) {
				c = '_';
			}
		}
		ImGui::TextDisabled("同じ名前のときは上書きします");
		ImGui::BeginDisabled(name.empty());
		if ((ImGui::Button("保存") || entered) && !name.empty()) {
			state_save(join_path(state_software_dir(), (name + ".sav").c_str()));
			g_gui.state_save_as_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("キャンセル")) {
			g_gui.state_save_as_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

/* ファイルから読み込み: any state below states/, for another software's
 * slots or a named state. */
static void draw_state_browser(void) {
	if (!g_gui.state_browser_open) {
		return;
	}
	prepare_dialog(44.0f, 30.0f, ImGuiCond_Appearing);
	if (ImGui::Begin("ファイルから読み込み###state-browser", &g_gui.state_browser_open)) {
		const std::string root = state_root();
		std::error_code ec;
		std::vector<std::string> dirs;
		for (const auto &entry : fs::directory_iterator(fs::u8path(root), ec)) {
			std::error_code st_ec;
			if (entry.is_directory(st_ec)) {
				dirs.push_back(entry.path().filename().u8string());
			}
		}
		std::sort(dirs.begin(), dirs.end());
		const float list_height = browser_list_height(1.5f, g_gui.state_status);
		ImGui::BeginChild("state-dirs",
		                  ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, list_height),
		                  ImGuiChildFlags_Borders);
		for (const auto &dir : dirs) {
			const std::string path = join_path(root, dir.c_str());
			const bool selected =
			    fs::u8path(path) == fs::u8path(g_gui.state_browser_dir).parent_path() ||
			    fs::u8path(path) == fs::u8path(g_gui.state_browser_dir);
			if (ImGui::Selectable(dir.c_str(), selected)) {
				g_gui.state_browser_dir = path;
			}
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("state-files", ImVec2(0.0f, list_height), ImGuiChildFlags_Borders);
		std::vector<std::string> files;
		for (const auto &entry : fs::directory_iterator(fs::u8path(g_gui.state_browser_dir), ec)) {
			std::error_code st_ec;
			if (entry.is_regular_file(st_ec) && (entry.path().extension() == ".sav")) {
				files.push_back(entry.path().u8string());
			}
		}
		std::sort(files.begin(), files.end());
		for (const auto &file : files) {
			const std::string label =
			    display_file_name(file.c_str()) + "  " + state_file_time(file);
			if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) &&
			    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				state_load(file);
			}
			state_thumbnail_tooltip(file);
		}
		ImGui::EndChild();
		ImGui::TextDisabled("ダブルクリックで読み込み");
		if (!g_gui.state_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.state_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_hostfat_error_dialog(void) {
	if (g_gui.hostfat_error_request) {
		g_gui.hostfat_error_request = false;
		ImGui::OpenPopup("HOSTFAT error##hostfat-error");
	}
	{
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowSizeConstraints(
		    ImVec2(0.0f, 0.0f), ImVec2(viewport->WorkSize.x * 0.95f, viewport->WorkSize.y * 0.95f));
	}
	if (ImGui::BeginPopupModal("HOSTFAT error##hostfat-error", &g_gui.hostfat_error_open,
	                           ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
		ImGui::TextUnformatted("HOSTFAT rebuild/reset failed.");
		ImGui::PopStyleColor();
		ImGui::Separator();
		ImGui::TextWrapped("%s", g_gui.hostfat_error_message.c_str());
		ImGui::Spacing();
		if (ImGui::Button("OK##hostfat-error", ImVec2(120.0f, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.hostfat_error_open = false;
			g_gui.hostfat_error_message.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void draw_state_error_dialog(void) {
	bool force_load = false;
	std::string force_path;

	if (g_gui.state_error_request) {
		g_gui.state_error_request = false;
		g_gui.state_error_open = true;
		ImGui::OpenPopup("State load rejected##state-load-error");
	}
	{
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowSizeConstraints(
		    ImVec2(0.0f, 0.0f), ImVec2(viewport->WorkSize.x * 0.95f, viewport->WorkSize.y * 0.95f));
	}
	if (ImGui::BeginPopupModal("State load rejected##state-load-error", &g_gui.state_error_open,
	                           ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextWrapped("%s", g_gui.state_status.c_str());
		if (g_gui.state_force_hostfat_available) {
			ImGui::Spacing();
			ImGui::TextWrapped("This save state references a different HOSTFAT snapshot. "
			                   "Force load keeps the current HOSTFAT mount state and "
			                   "read-only snapshot. Guest-cached FAT, directory, open-file, "
			                   "or file data may no longer match.");
		}
		ImGui::Separator();
		const char *cancel_label = g_gui.state_force_hostfat_available ? "Cancel" : "OK";
		if (ImGui::Button(cancel_label, ImVec2(120.0f, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.state_error_open = false;
			g_gui.state_force_hostfat_available = false;
			g_gui.state_force_hostfat_path.clear();
			ImGui::CloseCurrentPopup();
		}
		if (g_gui.state_force_hostfat_available) {
			ImGui::SameLine();
			if (ImGui::Button("Force load", ImVec2(120.0f, 0.0f))) {
				force_load = true;
				force_path = g_gui.state_force_hostfat_path;
				g_gui.state_error_open = false;
				g_gui.state_force_hostfat_available = false;
				g_gui.state_force_hostfat_path.clear();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndPopup();
	}
	if (force_load) {
		taskmng_clear_fast_forward();
		int ret = statsave_load_hostfat_override(force_path.c_str());
		if (ret == STATFLAG_FAILURE) {
			g_gui.state_status = "Forced state load failed: ";
			g_gui.state_status += force_path;
			g_gui.state_error_request = true;
		} else {
			sdlkbd_reset_state();
			mousemng_reset();
			scrndrawva_redraw();
			g_gui.state_status = "State loaded with HOSTFAT override: ";
			g_gui.state_status += force_path;
			if ((ret & STATFLAG_DISKCHG) != 0) {
				g_gui.state_status += " (disk warning ignored)";
			}
		}
	}
}

} // namespace

extern "C" void gui_session_end(void) {
	history_note_session_display();
}

extern "C" void gui_history_add(int kind, const char *path) {
	history_add_dropped(kind, path);
}
extern "C" BOOL gui_copy_screen_text(void) {
	return copy_screen_text();
}

extern "C" void gui_quick_save(void) {
	quick_save();
}

extern "C" void gui_quick_load(void) {
	if (quick_allowed()) {
		quick_load(1);
	}
}

extern "C" BOOL gui_save_screenshot(void) {
	return save_screenshot(FALSE);
}

BOOL gui_initialize(void *window, void *renderer, const char *argv0) {
	(void)argv0;
	if ((window == nullptr) || ((renderer == nullptr) && !scrnmng_native_active())) {
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
	    const_cast<unsigned char *>(vaeg_gui_font_ttf), static_cast<int>(vaeg_gui_font_ttf_size),
	    kGuiFontSize, &font_config, io.Fonts->GetGlyphRangesJapanese());
	if (font == nullptr) {
		std::fprintf(stderr, "Error: failed to load embedded GUI font\n");
		ImGui::DestroyContext();
		return FAILURE;
	}

	ImGui::StyleColorsDark();
	g_gui.base_style = ImGui::GetStyle();
	g_gui.ui_scale = 0.0f;
	scrnmng_set_menu_height(scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE ? 0 : menu_bar_height());
	g_gui.window = static_cast<SDL_Window *>(window);
	g_gui.native_renderer = renderer == nullptr;
	if (g_gui.native_renderer) {
		if (!ImGui_ImplSDL2_InitForOther(g_gui.window)) {
			ImGui::DestroyContext();
			return FAILURE;
		}
		if (scrnmng_native_gui_prepare() != SUCCESS) {
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
			return FAILURE;
		}
		g_gui.initialized = true;
		load_native_about_texture();
		load_native_crt_preset();
		SDL_Log("GUI renderer: native; base font=16 pixels, DPI-scaled");
		return SUCCESS;
	}
	if (!ImGui_ImplSDL2_InitForSDLRenderer(static_cast<SDL_Window *>(window),
	                                       static_cast<SDL_Renderer *>(renderer))) {
		return FAILURE;
	}
	g_gui.renderer = static_cast<SDL_Renderer *>(renderer);
	if (!ImGui_ImplSDLRenderer2_Init(g_gui.renderer)) {
		ImGui_ImplSDL2_Shutdown();
		return FAILURE;
	}
	g_gui.about_texture =
	    load_about_texture(g_gui.renderer, &g_gui.about_texture_width, &g_gui.about_texture_height);
	if (g_gui.about_texture == nullptr) {
		std::fprintf(stderr, "Warning: failed to load embedded About image: %s\n", SDL_GetError());
	}
	g_gui.initialized = true;
	return SUCCESS;
}

void gui_shutdown(void) {
	if (!g_gui.initialized) {
		return;
	}
	mousemng_setguiblocked(TRUE);
	/* Renderer changes can tear down a GUI while its popup owns capture. */
	ImGui_ImplSDL2_SetMouseCaptureMode(ImGui_ImplSDL2_MouseCaptureMode_Disabled);
	SDL_CaptureMouse(SDL_FALSE);
	if (g_gui.text_input_active) {
		SDL_StopTextInput();
		g_gui.text_input_active = false;
	}
	state_release_thumbnails();
	if (g_gui.about_texture != nullptr) {
		SDL_DestroyTexture(g_gui.about_texture);
		g_gui.about_texture = nullptr;
	}
	if (g_gui.native_renderer)
		scrnmng_native_gui_shutdown();
	else
		ImGui_ImplSDLRenderer2_Shutdown();
	if (g_gui.native_about_texture) {
		ImGui::UnregisterUserTexture(g_gui.native_about_texture);
		IM_DELETE(g_gui.native_about_texture);
	}
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	g_gui = GuiState{};
}

static bool fullscreen_menu_mouse_zone(void) {
	if (scrnmng_get_display_mode() != VAEG_DISPLAY_EXCLUSIVE ||
	    SDL_GetRelativeMouseMode() || SDL_GetMouseFocus() != g_gui.window) return false;
	int x, y, width, height;
	SDL_GetMouseState(&x, &y);
	SDL_GetWindowSize(g_gui.window, &width, &height);
	const float limit = g_gui.fullscreen_menu.visible ?
	    static_cast<float>(menu_bar_height()) : kFullscreenRevealHeight * g_gui.ui_scale;
	return x >= 0 && x < width && y >= 0 && y < limit;
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
	if ((g_gui.capture_swallow != SDL_SCANCODE_UNKNOWN) && (sdl_event->type == SDL_KEYUP) &&
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
		return (io.WantCaptureMouse || fullscreen_menu_mouse_zone()) ? TRUE : FALSE;

	default:
		return FALSE;
	}
}

BOOL gui_guest_keyboard_blocked(void) {
	if (!g_gui.initialized) {
		return FALSE;
	}
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureKeyboard || io.WantTextInput || (g_gui.capture_binding >= 0)) {
		return TRUE;
	}
	return (g_gui.fdd_browser_open || g_gui.hdd_browser_open || g_gui.hostfat_browser_open ||
	        g_gui.tape_browser_open || g_gui.new_fdd_open || g_gui.new_sasi_open ||
	        g_gui.new_scsi_open || g_gui.keyboard_config_open || g_gui.configure_open ||
	        g_gui.bms_config_open || g_gui.custom_size_open || g_gui.state_error_open ||
	        g_gui.hostfat_error_open || g_gui.about_open)
	           ? TRUE
	           : FALSE;
}

BOOL gui_guest_mouse_blocked(void) {
	if (!g_gui.initialized) {
		return FALSE;
	}
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse || fullscreen_menu_mouse_zone() || (g_gui.capture_binding >= 0)) {
		return TRUE;
	}
	return (g_gui.fdd_browser_open || g_gui.hdd_browser_open || g_gui.hostfat_browser_open ||
	        g_gui.tape_browser_open || g_gui.new_fdd_open || g_gui.new_sasi_open ||
	        g_gui.new_scsi_open || g_gui.keyboard_config_open || g_gui.configure_open ||
	        g_gui.bms_config_open || g_gui.custom_size_open || g_gui.state_error_open ||
	        g_gui.hostfat_error_open || g_gui.about_open)
	           ? TRUE
	           : FALSE;
}

void gui_new_frame(void) {
	if (!g_gui.initialized) {
		return;
	}
	if (g_gui.native_renderer) {
		if (scrnmng_native_gui_prepare() != SUCCESS) {
			if (scrnmng_fallback_to_sdl() != SUCCESS ||
			    gui_initialize(scrnmng_get_window(), scrnmng_get_renderer(), nullptr) != SUCCESS) {
				taskmng_exit();
				return;
			}
			(void)scrnmng_take_native_fallback();
		}
	} else {
		ImGui_ImplSDLRenderer2_NewFrame();
	}
	ImGui_ImplSDL2_NewFrame();
	if (g_gui.native_renderer) {
		int width, height, pixels_width, pixels_height;
		SDL_GetWindowSize(g_gui.window, &width, &height);
		SDL_GetWindowSizeInPixels(g_gui.window, &pixels_width, &pixels_height);
		if (width > 0 && height > 0) {
			ImGui::GetIO().DisplayFramebufferScale =
			    ImVec2(static_cast<float>(pixels_width) / width,
			           static_cast<float>(pixels_height) / height);
		}
	}
	float ui_scale = 1.0f;
#if defined(_WIN32)
	const float pixel_scale = ImGui::GetIO().DisplayFramebufferScale.x;
	ui_scale = ImGui_ImplSDL2_GetContentScaleForWindow(g_gui.window) /
	           (pixel_scale > 0.0f ? pixel_scale : 1.0f);
#endif
	if (np2oscfg.gui_ui_scale >= 100 && np2oscfg.gui_ui_scale <= 300) {
		ui_scale = np2oscfg.gui_ui_scale / 100.0f;
	}
	ui_scale = (std::max)(1.0f, (std::min)(4.0f, ui_scale));
	if (std::fabs(ui_scale - g_gui.ui_scale) > 0.001f) {
		ImGuiStyle &style = ImGui::GetStyle();
		style = g_gui.base_style;
		style.ScaleAllSizes(ui_scale);
		style.FontScaleDpi = ui_scale;
		g_gui.menu_font_size = kGuiFontSize * ui_scale;
		g_gui.ui_scale = ui_scale;
		SDL_Log("GUI scale: %.2f; font=%.1f; mode=%s", ui_scale, g_gui.menu_font_size,
		        np2oscfg.gui_ui_scale == 0 ? "automatic DPI" : "manual");
	}
	ImGui::NewFrame();
	scrnmng_set_menu_height(scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE
	                            ? 0 : static_cast<int>(std::ceil(ImGui::GetFrameHeight())));
}

void gui_draw(void) {
	if (!g_gui.initialized) {
		return;
	}
	// Capture on a fresh frame without menus/dialogs; information overlays
	// are still emitted by gui_render/scrnmng_present_end.
	if (scrnmng_prepare_display_capture()) return;
	const UINT hostfat_event = hostfat_manager_poll();
	if (hostfat_event == HOSTFAT_MANAGER_EVENT_MOUNTED) {
		HOSTFAT_MANAGER_STATUS status{};
		hostfat_manager_get_status(&status);
		g_gui.hostfat_status = status.message;
		if (g_gui.hostfat_reset_after_build) {
			np2oscfg.hostfat_enabled = 1;
			milstr_ncpy(np2oscfg.hostfat_dir, g_gui.hostfat_rebuild_dir.c_str(),
			            sizeof(np2oscfg.hostfat_dir));
			sysmng_update(SYS_UPDATEOSCFG);
			g_gui.hostfat_rebuild_dir.clear();
			g_gui.hostfat_reset_after_build = false;
			reset_guest();
		}
	} else if (hostfat_event == HOSTFAT_MANAGER_EVENT_FAILED) {
		HOSTFAT_MANAGER_STATUS status{};
		hostfat_manager_get_status(&status);
		g_gui.hostfat_status = "HOSTFAT rebuild failed: ";
		g_gui.hostfat_status += status.message;
		g_gui.hostfat_rebuild_dir.clear();
		g_gui.hostfat_reset_after_build = false;
		g_gui.hostfat_error_message = g_gui.hostfat_status;
		g_gui.hostfat_error_open = true;
		g_gui.hostfat_error_request = true;
		g_gui.configure_open = true;
		g_gui.configure_request = true;
	}
	const ImGuiIO &io = ImGui::GetIO();
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float mouse_y = io.MousePos.y - viewport->Pos.y;
	const bool mouse_in_window = SDL_GetMouseFocus() == g_gui.window &&
	    !SDL_GetRelativeMouseMode() && io.MousePos.x >= viewport->Pos.x &&
	    io.MousePos.x < viewport->Pos.x + viewport->Size.x && mouse_y >= 0;
	const bool show_menu = update_fullscreen_menu(
	    g_gui.fullscreen_menu, scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE,
	    mouse_in_window && mouse_y < kFullscreenRevealHeight * g_gui.ui_scale,
	    mouse_in_window && mouse_y < ImGui::GetFrameHeight(),
	    ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) ||
	        ImGui::IsAnyItemActive(), SDL_GetTicks());
	if (show_menu && ImGui::BeginMainMenuBar()) {
		draw_emulate_menu();
		draw_fdd_menu();
		draw_harddisk_menu();
		draw_tape_menu();
		draw_edit_menu();
		draw_screen_menu();
		draw_key_menu();
		draw_sound_menu();
		draw_state_menu();
		draw_info_menu();
		ImGui::EndMainMenuBar();
	}
	if (!show_menu && fullscreen_hint_visible(g_gui.fullscreen_menu, SDL_GetTicks())) {
		ImGui::SetNextWindowPos(
		    ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f,
		           viewport->Pos.y + ImGui::GetFrameHeight() + 8.0f * g_gui.ui_scale),
		    ImGuiCond_Always, ImVec2(0.5f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.85f);
		if (ImGui::Begin("##fullscreen_hint", nullptr,
		                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
		                 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings |
		                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
			ImGui::TextUnformatted("画面上端でメニュー表示");
		}
		ImGui::End();
	}
	draw_pause_overlay();
	draw_osd();
	draw_state_error_dialog();
	draw_state_overview();
	draw_state_save_as();
	draw_state_browser();
	draw_fdd_browser();
	draw_hdd_browser();
	draw_tape_browser();
	draw_new_fdd_dialog();
	draw_new_sasi_dialog();
	draw_new_scsi_dialog();
	draw_keyboard_config();
	draw_configure_dialog();
	draw_hostfat_error_dialog();
	draw_bms_config_dialog();
	draw_ems_config_dialog();
	draw_custom_size_dialog();
	draw_native_crt_settings();
	draw_sound_buffer_dialog();
	draw_about_dialog();
}

void gui_render(void) {
	if (!g_gui.initialized) {
		return;
	}
	if (g_gui.native_renderer) scrnmng_draw_native_overlays();
	ImGui::Render();
	update_text_input_state();
	if (!g_gui.native_renderer) {
		scrnmng_draw_sdl_overlays();
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_gui.renderer);
	}
}

void gui_display_capture_result(const char *path, BOOL success) {
	g_gui.screenshot_status = success == SUCCESS ? "Screenshot saved: " : "Screenshot failed: ";
	g_gui.screenshot_status += path;
	if (success != SUCCESS) {
		g_gui.screenshot_status += " / ";
		g_gui.screenshot_status += SDL_GetError();
	}
	SDL_Log("%s", g_gui.screenshot_status.c_str());
}

static void draw_overlay_rect(ImDrawList *list, ImVec2 scale, ImVec2 origin,
                              int x, int y, int width, int height, ImU32 color) {
	if (scale.x <= 0 || scale.y <= 0) return;
	list->AddRectFilled(
	    ImVec2(origin.x + x / scale.x, origin.y + y / scale.y),
	    ImVec2(origin.x + (x + width) / scale.x, origin.y + (y + height) / scale.y),
	    color);
}

void gui_overlay_rect(int x, int y, int width, int height,
                      unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	if (!g_gui.initialized || !g_gui.native_renderer) return;
	draw_overlay_rect(ImGui::GetBackgroundDrawList(), ImGui::GetIO().DisplayFramebufferScale,
	                  ImGui::GetMainViewport()->Pos, x, y, width, height, IM_COL32(r, g, b, a));
}

BOOL gui_quick_rotate_selftest(const char *root, char *problem, size_t size) {
	const std::string dir = absolute_path(root);
	const char *failed = nullptr;
	auto write = [](const std::string &path, const std::string &text) {
		std::ofstream(fs::u8path(path), std::ios::binary) << text;
	};
	auto read = [](const std::string &path) {
		std::ifstream input(fs::u8path(path), std::ios::binary);
		std::string text;
		std::getline(input, text);
		return text;
	};
	const std::string fresh = join_path(dir, "quick-new.sav");

	for (int round = 1; round <= 7; round++) {
		write(fresh, "s" + std::to_string(round));
		write(state_thumbnail_path(fresh), "t" + std::to_string(round));
		if (!quick_rotate(dir, fresh)) {
			failed = "a written save was not rotated in";
			break;
		}
	}
	/* seven saves: quick1..5 hold s7..s3, the thumbnails follow */
	for (int index = 1; !failed && (index <= kQuickSlots); index++) {
		const std::string expect = std::to_string(8 - index);
		if ((read(quick_file(dir, index)) != "s" + expect) ||
		    (read(state_thumbnail_path(quick_file(dir, index))) != "t" + expect)) {
			failed = "quick1 is the newest and older saves move down";
		}
	}
	if (!failed && (quick_rotate(dir, fresh) || (read(quick_file(dir, 1)) != "s7") ||
	                (read(quick_file(dir, 5)) != "s3"))) {
		failed = "a failed save (no file) leaves the five saves as they were";
	}
	if (!failed && fs::exists(fs::u8path(quick_file(dir, 6)))) {
		failed = "no sixth save is kept";
	}
	for (int index = 1; index <= kQuickSlots + 1; index++) {
		file_delete(quick_file(dir, index).c_str());
		file_delete(state_thumbnail_path(quick_file(dir, index)).c_str());
	}
	if (failed != nullptr) {
		std::snprintf(problem, size, "%s", failed);
		return FAILURE;
	}
	return SUCCESS;
}

BOOL gui_recent_dirs_selftest(const char *root, char *problem, size_t size) {
	std::vector<std::string> saved[HIST_COUNT];
	const bool saved_loaded = g_history_loaded;
	const std::string base = absolute_path(root); /* lists hold absolute folders */
	const std::string fdd = join_path(base, "fdd");
	const std::string hdd = join_path(base, "hdd");
	const std::string tape = join_path(base, "tape");
	const std::string other = join_path(base, "other");
	const std::string media = join_path(base, "media");
	const char *failed = nullptr;

	for (const std::string &dir : {fdd, hdd, tape, other, media}) {
		(void)file_dircreate(dir.c_str());
	}
	for (int kind = 0; kind < HIST_COUNT; kind++) {
		saved[kind].swap(g_history[kind]);
	}
	g_history_loaded = true; /* no history file is read or written */

	if (history_start_dir(HIST_DIR_TAPE, "") != home_dir()) {
		failed = "no folders: home";
	}
	g_history[HIST_DIR_HDD] = {hdd};
	g_history[HIST_DIR_TAPE] = {join_path(base, "missing"), tape};
	if (!failed && (history_start_dir(HIST_DIR_FDD, "") != hdd)) {
		failed = "FDD without its own folder: the HDD folder";
	}
	if (!failed && (history_start_dir(HIST_DIR_TAPE, "") != tape)) {
		failed = "tape: its own newest existing folder";
	}
	g_history[HIST_DIR_FDD] = {fdd};
	if (!failed && (history_start_dir(HIST_DIR_HDD, "") != hdd)) {
		failed = "HDD: its own folder";
	}
	g_history[HIST_DIR_HDD].clear();
	g_history[HIST_DIR_TAPE].clear();
	if (!failed && ((history_start_dir(HIST_DIR_HDD, "") != fdd) ||
	                (history_start_dir(HIST_DIR_TAPE, "") != fdd))) {
		failed = "HDD and tape without their own: the FDD folder";
	}
	if (!failed && (history_start_dir(HIST_DIR_FDD, media) != media)) {
		failed = "the medium in use first";
	}
	for (int kind = 0; kind < HIST_COUNT; kind++) {
		g_history[kind].clear();
	}
	g_history[HIST_FDD2] = {join_path(fdd, "a.d88")};
	g_history[HIST_SASI1] = {join_path(hdd, "a.hdi")};
	g_history[HIST_TAPE] = {join_path(tape, "a.cmt"), join_path(fdd, "b.cmt")};
	history_split_legacy_dirs({tape, other, fdd, hdd});
	if (!failed && ((g_history[HIST_DIR_FDD] != std::vector<std::string>{other, fdd}) ||
	                (g_history[HIST_DIR_HDD] != std::vector<std::string>{hdd}) ||
	                (g_history[HIST_DIR_TAPE] != std::vector<std::string>{tape, fdd}))) {
		failed = "the shared list split by the files used in each folder";
	}

	for (int kind = 0; kind < HIST_COUNT; kind++) {
		g_history[kind].swap(saved[kind]);
	}
	g_history_loaded = saved_loaded;
	for (const std::string &dir : {fdd, hdd, tape, other, media}) {
		std::error_code error;
		fs::remove(fs::u8path(dir), error);
	}
	if (failed != nullptr) {
		std::snprintf(problem, size, "%s", failed);
		return FAILURE;
	}
	return SUCCESS;
}

BOOL gui_overlay_selftest(void) {
	FullscreenMenuState menu;
	update_fullscreen_menu(menu, true, false, false, false, 100);
	if (!fullscreen_hint_visible(menu, 3099) || fullscreen_hint_visible(menu, 3100)) {
		std::fprintf(stderr, "selftest: FULLSCREEN_HINT_DURATION\n");
		return FAILURE;
	}
	update_fullscreen_menu(menu, false, false, false, false, 200);
	if (fullscreen_hint_visible(menu, 201)) return FAILURE;
	if (!update_fullscreen_menu(menu, false, false, false, false, 0) ||
	    update_fullscreen_menu(menu, true, false, false, false, 1) ||
	    !update_fullscreen_menu(menu, true, true, false, false, 10) ||
	    !update_fullscreen_menu(menu, true, false, false, false, 509) ||
	    update_fullscreen_menu(menu, true, false, false, false, 510) ||
	    !update_fullscreen_menu(menu, true, true, false, false, 600) ||
	    !update_fullscreen_menu(menu, true, false, false, true, 2000) ||
	    !update_fullscreen_menu(menu, true, false, true, false, 3000) ||
	    update_fullscreen_menu(menu, true, false, false, false, 3500) ||
	    !update_fullscreen_menu(menu, false, false, false, false, 3501) ||
	    update_fullscreen_menu(menu, true, false, false, false, 3502)) {
		std::fprintf(stderr, "selftest: FULLSCREEN_MENU_POLICY_MISMATCH\n");
		return FAILURE;
	}
	update_fullscreen_menu(menu, true, true, false, false, 0xffffff00U);
	if (update_fullscreen_menu(menu, true, false, false, false, 0x00000100U)) {
		std::fprintf(stderr, "selftest: FULLSCREEN_MENU_TIMER_WRAP\n");
		return FAILURE;
	}
	ImGuiContext *saved = ImGui::GetCurrentContext();
	ImGuiContext *context = ImGui::CreateContext();
	ImGui::SetCurrentContext(context);
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = ImVec2(640, 400);
	io.Fonts->Build();
	ImGui::NewFrame();
	ImDrawList *list = ImGui::GetBackgroundDrawList();
	const ImU32 color = IM_COL32(255, 255, 192, 255);
	BOOL result = SUCCESS;
	for (int scale = 1; scale <= 2; ++scale) {
		const int first = list->VtxBuffer.Size;
		draw_overlay_rect(list, ImVec2(scale, scale), ImVec2(3, 5), 20, 40, 8, 8, color);
		if (list->VtxBuffer.Size != first + 4 ||
		    list->VtxBuffer[first].pos.x != 3 + 20.0f / scale ||
		    list->VtxBuffer[first].pos.y != 5 + 40.0f / scale ||
		    list->VtxBuffer[first + 2].pos.x != 3 + 28.0f / scale ||
		    list->VtxBuffer[first + 2].pos.y != 5 + 48.0f / scale ||
		    list->VtxBuffer[first].col != color) result = FAILURE;
	}
	ImGui::EndFrame();
	ImGui::DestroyContext(context);
	ImGui::SetCurrentContext(saved);
	if (result != SUCCESS) std::fprintf(stderr, "selftest: OVERLAY_GEOMETRY_MISMATCH\n");
	return result;
}
