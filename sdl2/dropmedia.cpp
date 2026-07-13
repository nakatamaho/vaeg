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

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#include <system_error>
#include <vector>

#if defined(VAEG_HAVE_LIBARCHIVE)
#include <archive.h>
#include <archive_entry.h>
#endif

#include "compiler.h"
#include "diskdrv.h"
#include "dosio.h"
#include "dropmedia.h"
#include "np2.h"
#include "sysmng.h"

namespace {

namespace fs = std::filesystem;

constexpr std::uintmax_t kMaxImageBytes = 64u * 1024u * 1024u;
constexpr std::uintmax_t kMaxExtractedBytes = 128u * 1024u * 1024u;
constexpr std::size_t kMaxArchiveEntries = 4096;
constexpr std::size_t kMaxImageCandidates = 256;

struct DiskCandidate {
	std::string path;
	std::string basename;
	bool temporary = false;
};

std::vector<std::string> g_dropped_paths;
std::string g_status;

static std::string ascii_lower(std::string value) {

	std::transform(value.begin(), value.end(), value.begin(),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return value;
}

static std::string path_extension(const std::string &path) {

	return ascii_lower(fs::u8path(path).extension().u8string());
}

static bool image_extension_supported(const std::string &path) {

	static const char *extensions[] = {
		".d88", ".88d", ".d98", ".98d", ".fdi",
		".xdf", ".hdm", ".dup", ".2hd", ".tfd"
	};
	const std::string extension = path_extension(path);

	return std::any_of(std::begin(extensions), std::end(extensions),
		[&extension](const char *candidate) {
			return extension == candidate;
		});
}

static bool archive_extension_supported(const std::string &path) {

	const std::string extension = path_extension(path);

	return (extension == ".zip") || (extension == ".7z") ||
		(extension == ".lzh");
}

static bool candidate_less(const DiskCandidate &left,
						   const DiskCandidate &right) {

	const std::string left_name = ascii_lower(left.basename);
	const std::string right_name = ascii_lower(right.basename);

	if (left_name != right_name) {
		return left_name < right_name;
	}
	return left.path < right.path;
}

static bool archive_path_safe(const char *entry_path) {

	std::string normalized;
	fs::path path;

	if ((entry_path == nullptr) || (entry_path[0] == '\0')) {
		return false;
	}
	normalized = entry_path;
	std::replace(normalized.begin(), normalized.end(), '\\', '/');
	if ((normalized.size() >= 3) &&
		std::isalpha(static_cast<unsigned char>(normalized[0])) &&
		(normalized[1] == ':') && (normalized[2] == '/')) {
		return false;
	}
	path = fs::u8path(normalized);
	if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
		return false;
	}
	for (const fs::path &component : path) {
		const std::string name = component.u8string();
		if ((name == "..") || name.empty()) {
			return false;
		}
	}
	return true;
}

#if defined(VAEG_HAVE_LIBARCHIVE)
static fs::path archive_storage_root(void) {

	char path[MAX_PATH];

	file_getstatepath(path, sizeof(path), "archive-drop");
	return fs::u8path(path);
}

static fs::path create_archive_directory(const fs::path &root,
										 std::string *error) {

	std::error_code ec;
	std::random_device random;

	if ((!fs::create_directories(root, ec)) && ec) {
		*error = "archive storage directory is unavailable";
		return fs::path();
	}
	for (unsigned int attempt = 0; attempt < 64; attempt++) {
		char name[64];
		std::snprintf(name, sizeof(name), "drop-%08x-%02u",
					  random(), attempt);
		fs::path candidate = root / name;
		if (fs::create_directory(candidate, ec)) {
			return candidate;
		}
		if (ec && (ec != std::errc::file_exists)) {
			break;
		}
		ec.clear();
	}
	*error = "could not create an archive storage directory";
	return fs::path();
}

static bool write_test_archive(const fs::path &path, bool seven_zip,
							   const char *entry_path) {

	struct archive *writer = archive_write_new();
	struct archive_entry *entry;
	static const char payload[] = "D88";
	int result;

	if (writer == nullptr) {
		return false;
	}
	result = seven_zip ? archive_write_set_format_7zip(writer) :
		archive_write_set_format_zip(writer);
	if ((!seven_zip) && (result == ARCHIVE_OK)) {
		result = archive_write_set_options(writer, "zip:compression=deflate");
	}
	if (result == ARCHIVE_OK) {
		result = archive_write_open_filename(writer, path.u8string().c_str());
	}
	entry = archive_entry_new();
	if ((result != ARCHIVE_OK) || (entry == nullptr)) {
		if (entry != nullptr) {
			archive_entry_free(entry);
		}
		archive_write_free(writer);
		return false;
	}
	archive_entry_set_pathname(entry, entry_path);
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0600);
	archive_entry_set_size(entry, sizeof(payload) - 1);
	result = archive_write_header(writer, entry);
	if (result == ARCHIVE_OK) {
		result = (archive_write_data(writer, payload, sizeof(payload) - 1) ==
				  static_cast<la_ssize_t>(sizeof(payload) - 1)) ?
			ARCHIVE_OK : ARCHIVE_FATAL;
	}
	archive_entry_free(entry);
	if (archive_write_close(writer) != ARCHIVE_OK) {
		result = ARCHIVE_FATAL;
	}
	archive_write_free(writer);
	return result == ARCHIVE_OK;
}

static bool extract_archive_images(const std::string &archive_path,
								   std::vector<DiskCandidate> *images,
								   std::string *error,
								   const fs::path &storage_root) {

	struct archive *reader;
	struct archive_entry *entry;
	fs::path temp_dir;
	std::uintmax_t total_bytes;
	std::size_t entry_count;
	std::size_t image_count;
	const std::size_t initial_image_count = images->size();
	int result;

	temp_dir = create_archive_directory(storage_root, error);
	if (temp_dir.empty()) {
		return false;
	}
	reader = archive_read_new();
	if (reader == nullptr) {
		fs::remove_all(temp_dir);
		*error = "libarchive initialization failed";
		return false;
	}
	archive_read_support_filter_all(reader);
	archive_read_support_format_all(reader);
#if defined(_WIN32)
	result = archive_read_open_filename_w(reader,
		fs::u8path(archive_path).c_str(), 10240);
#else
	result = archive_read_open_filename(reader, archive_path.c_str(), 10240);
#endif
	if (result != ARCHIVE_OK) {
		*error = archive_error_string(reader) ? archive_error_string(reader) :
			"archive open failed";
		archive_read_free(reader);
		fs::remove_all(temp_dir);
		return false;
	}
	total_bytes = 0;
	entry_count = 0;
	image_count = 0;
	while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK) {
		const char *entry_path = archive_entry_pathname_utf8(entry);
		if (entry_path == nullptr) {
			entry_path = archive_entry_pathname(entry);
		}
		entry_count++;
		if (entry_count > kMaxArchiveEntries) {
			*error = "archive contains too many entries";
			goto extract_error;
		}
		if (!archive_path_safe(entry_path)) {
			*error = "archive contains an unsafe path";
			goto extract_error;
		}
		if ((archive_entry_symlink(entry) != nullptr) ||
			(archive_entry_hardlink(entry) != nullptr)) {
			*error = "archive links are not allowed";
			goto extract_error;
		}
		if ((archive_entry_filetype(entry) != AE_IFREG) ||
			(!image_extension_supported(entry_path))) {
			archive_read_data_skip(reader);
			continue;
		}
		if (image_count >= kMaxImageCandidates) {
			*error = "archive contains too many disk images";
			goto extract_error;
		}
		const la_int64_t declared_size = archive_entry_size(entry);
		if ((declared_size < 0) ||
			(static_cast<std::uintmax_t>(declared_size) > kMaxImageBytes) ||
			(static_cast<std::uintmax_t>(declared_size) >
			 (kMaxExtractedBytes - total_bytes))) {
			*error = "archive disk image exceeds the extraction limit";
			goto extract_error;
		}
		fs::path basename = fs::u8path(entry_path).filename();
		char subdir_name[16];
		std::snprintf(subdir_name, sizeof(subdir_name), "%04zu", image_count);
		fs::path output_dir = temp_dir / subdir_name;
		fs::path output_path = output_dir / basename;
		std::error_code ec;
		if ((!fs::create_directory(output_dir, ec)) || ec) {
			*error = "could not create an archive extraction directory";
			goto extract_error;
		}
		std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
		if (!output) {
			*error = "could not create an extracted disk image";
			goto extract_error;
		}
		std::uintmax_t image_bytes = 0;
		char buffer[16384];
		for (;;) {
			la_ssize_t count = archive_read_data(reader, buffer, sizeof(buffer));
			if (count == 0) {
				break;
			}
			if (count < 0) {
				*error = archive_error_string(reader) ? archive_error_string(reader) :
					"archive extraction failed";
				goto extract_error;
			}
			if ((static_cast<std::uintmax_t>(count) >
				 (kMaxImageBytes - image_bytes)) ||
				(static_cast<std::uintmax_t>(count) >
				 (kMaxExtractedBytes - total_bytes))) {
				*error = "archive extraction limit exceeded";
				goto extract_error;
			}
			output.write(buffer, count);
			if (!output) {
				*error = "could not write an extracted disk image";
				goto extract_error;
			}
			image_bytes += static_cast<std::uintmax_t>(count);
			total_bytes += static_cast<std::uintmax_t>(count);
		}
		output.close();
		if (!output) {
			*error = "could not finish an extracted disk image";
			goto extract_error;
		}
		if (output_path.u8string().size() >= MAX_PATH) {
			*error = "extracted disk image path is too long";
			goto extract_error;
		}
		images->push_back({output_path.u8string(), basename.u8string(), true});
		image_count++;
	}
	if (result != ARCHIVE_EOF) {
		*error = archive_error_string(reader) ? archive_error_string(reader) :
			"archive read failed";
		goto extract_error;
	}
	archive_read_free(reader);
	if (image_count == 0) {
		fs::remove_all(temp_dir);
		*error = "archive contains no supported disk image";
		return false;
	}
	return true;

extract_error:
	archive_read_free(reader);
	fs::remove_all(temp_dir);
	images->resize(initial_image_count);
	return false;
}
#endif

static void append_status_line(const std::string &line) {

	if (!g_status.empty()) {
		g_status += '\n';
	}
	g_status += line;
}

static void mount_candidates(std::vector<DiskCandidate> *images) {

	std::sort(images->begin(), images->end(), candidate_less);
	for (std::size_t index = 0; index < images->size() && index < 2; index++) {
		const DiskCandidate &image = (*images)[index];
		diskdrv_setfdd(static_cast<REG8>(index), image.path.c_str(), 0);
		file_cpyname(np2oscfg.fdd_image[index], image.path.c_str(),
					 sizeof(np2oscfg.fdd_image[index]));
		sysmng_update(SYS_UPDATEOSCFG);
	}
	if (images->size() > 2) {
		append_status_line("Ignored " + std::to_string(images->size() - 2) +
			" additional disk image(s).");
	}
	dropmedia_prune_storage();
}

static void process_drop_batch(void) {

	std::vector<DiskCandidate> images;

	g_status.clear();
	for (const std::string &path : g_dropped_paths) {
		std::error_code ec;
		const fs::path fs_path = fs::u8path(path);
		if ((!fs::is_regular_file(fs_path, ec)) || ec) {
			append_status_line("Drop ignored: file not found: " + path);
			continue;
		}
		if (image_extension_supported(path)) {
			if (path.size() >= MAX_PATH) {
				append_status_line("Drop ignored: path is too long: " + path);
				continue;
			}
			images.push_back({path, fs_path.filename().u8string(), false});
			continue;
		}
		if (archive_extension_supported(path)) {
#if defined(VAEG_HAVE_LIBARCHIVE)
			std::string error;
			if (!extract_archive_images(path, &images, &error,
									archive_storage_root())) {
				append_status_line("Archive drop failed: " + error + ": " +
					fs_path.filename().u8string());
			}
#else
			append_status_line(
				"Archive drop unavailable: this build has no LibArchive support.");
#endif
			continue;
		}
		append_status_line("Unsupported dropped file: " +
			fs_path.filename().u8string());
	}
	if (!images.empty()) {
		mount_candidates(&images);
	}
	else if (g_status.empty()) {
		g_status = "Drop contained no supported disk image.";
	}
	g_dropped_paths.clear();
}

} // namespace

extern "C" BOOL dropmedia_process_event(const void *event) {

	const SDL_Event *sdl_event;

	if (event == nullptr) {
		return FALSE;
	}
	sdl_event = static_cast<const SDL_Event *>(event);
	switch (sdl_event->type) {
		case SDL_DROPBEGIN:
			g_dropped_paths.clear();
			return TRUE;

		case SDL_DROPFILE:
			if (sdl_event->drop.file != nullptr) {
				g_dropped_paths.emplace_back(sdl_event->drop.file);
				SDL_free(sdl_event->drop.file);
			}
			return TRUE;

		case SDL_DROPCOMPLETE:
			process_drop_batch();
			return TRUE;

		default:
			return FALSE;
	}
}

extern "C" const char *dropmedia_status(void) {

	return g_status.c_str();
}

#if defined(VAEG_HAVE_LIBARCHIVE)
static bool path_is_within(const fs::path &path, const fs::path &directory) {

	const fs::path normalized_path = path.lexically_normal();
	const fs::path normalized_directory = directory.lexically_normal();
	auto path_part = normalized_path.begin();
	auto directory_part = normalized_directory.begin();

	for (; directory_part != normalized_directory.end();
		 directory_part++, path_part++) {
		if ((path_part == normalized_path.end()) ||
			(*path_part != *directory_part)) {
			return false;
		}
	}
	return true;
}

static bool storage_directory_referenced(const fs::path &directory,
										 const char *references[2]) {

	for (unsigned int drive = 0; drive < 2; drive++) {
		if ((references[drive] != nullptr) &&
			(references[drive][0] != '\0') &&
			path_is_within(fs::u8path(references[drive]), directory)) {
			return true;
		}
	}
	return false;
}

static void prune_storage_root(const fs::path &root,
							  const char *references[2]) {

	std::error_code ec;

	if ((!fs::exists(root, ec)) || ec) {
		return;
	}
	for (const fs::directory_entry &batch : fs::directory_iterator(root, ec)) {
		if (ec) {
			return;
		}
		const std::string batch_name = batch.path().filename().u8string();
		if ((batch_name.rfind("drop-", 0) != 0) ||
			(!batch.is_directory(ec)) || ec) {
			ec.clear();
			continue;
		}
		for (const fs::directory_entry &image :
			 fs::directory_iterator(batch.path(), ec)) {
			if (ec) {
				return;
			}
			const std::string image_name = image.path().filename().u8string();
			if ((image_name.size() != 4) ||
				(!std::all_of(image_name.begin(), image_name.end(),
					[](unsigned char ch) { return std::isdigit(ch) != 0; }))) {
				continue;
			}
			if (!storage_directory_referenced(image.path(), references)) {
				fs::remove_all(image.path(), ec);
				ec.clear();
			}
		}
		if (fs::is_empty(batch.path(), ec) && !ec) {
			fs::remove(batch.path(), ec);
		}
		ec.clear();
	}
}
#endif

extern "C" void dropmedia_prune_storage(void) {

#if defined(VAEG_HAVE_LIBARCHIVE)
	const char *references[2] = {
		np2oscfg.fdd_image[0], np2oscfg.fdd_image[1]
	};

	prune_storage_root(archive_storage_root(), references);
#endif
}

extern "C" void dropmedia_initialize(void) {

	dropmedia_prune_storage();
}

extern "C" void dropmedia_shutdown(void) {

	g_dropped_paths.clear();
	g_status.clear();
}

extern "C" BOOL dropmedia_selftest(void) {

	std::vector<DiskCandidate> candidates = {
		{"/tmp/b.D88", "b.D88", false},
		{"/tmp/A.d88", "A.d88", false},
		{"/tmp/c.xdf", "c.xdf", false}
	};

	if ((!image_extension_supported("disk.D88")) ||
		(!image_extension_supported("disk.2HD")) ||
		image_extension_supported("disk.zip") ||
		(!archive_extension_supported("set.7Z")) ||
		(!archive_extension_supported("set.LZH"))) {
		return FAILURE;
	}
	if ((!archive_path_safe("folder/disk.d88")) ||
		archive_path_safe("../disk.d88") ||
		archive_path_safe("folder/../../disk.d88") ||
		archive_path_safe("/absolute/disk.d88") ||
		archive_path_safe("C:\\absolute\\disk.d88")) {
		return FAILURE;
	}
	std::sort(candidates.begin(), candidates.end(), candidate_less);
	if ((candidates[0].basename != "A.d88") ||
		(candidates[1].basename != "b.D88") ||
		(candidates[2].basename != "c.xdf")) {
		return FAILURE;
	}
#if defined(VAEG_HAVE_LIBARCHIVE)
	std::string error;
	std::error_code ec;
	fs::path test_dir = fs::temp_directory_path(ec) / "vaeg-dropmedia-selftest";
	fs::remove_all(test_dir, ec);
	if (ec || (!fs::create_directory(test_dir, ec)) || ec) {
		return FAILURE;
	}
	for (bool seven_zip : {false, true}) {
		fs::path archive_path = test_dir /
			(seven_zip ? "dropmedia-test.7z" : "dropmedia-test.zip");
		std::vector<DiskCandidate> extracted;
		if ((!write_test_archive(archive_path, seven_zip, "nested/test.d88")) ||
			(!extract_archive_images(archive_path.u8string(), &extracted, &error,
									 test_dir / "storage")) ||
			(extracted.size() != 1) || (extracted[0].basename != "test.d88")) {
			fs::remove_all(test_dir);
			dropmedia_shutdown();
			return FAILURE;
		}
	}
	fs::path unsafe_path = test_dir / "dropmedia-unsafe.zip";
	std::vector<DiskCandidate> unsafe_images;
	if ((!write_test_archive(unsafe_path, false, "../escape.d88")) ||
		extract_archive_images(unsafe_path.u8string(), &unsafe_images, &error,
								 test_dir / "storage")) {
		fs::remove_all(test_dir);
		dropmedia_shutdown();
		return FAILURE;
	}
	fs::path prune_root = test_dir / "prune";
	fs::path keep_first = prune_root / "drop-one" / "0000" / "first.d88";
	fs::path remove_image = prune_root / "drop-one" / "0001" / "unused.d88";
	fs::path keep_second = prune_root / "drop-two" / "0000" / "second.d88";
	fs::create_directories(keep_first.parent_path(), ec);
	fs::create_directories(remove_image.parent_path(), ec);
	fs::create_directories(keep_second.parent_path(), ec);
	std::ofstream(keep_first).put('\0');
	std::ofstream(remove_image).put('\0');
	std::ofstream(keep_second).put('\0');
	const std::string first_reference = keep_first.u8string();
	const std::string second_reference = keep_second.u8string();
	const char *references[2] = {
		first_reference.c_str(), second_reference.c_str()
	};
	prune_storage_root(prune_root, references);
	if ((!fs::exists(keep_first)) || fs::exists(remove_image) ||
		(!fs::exists(keep_second))) {
		fs::remove_all(test_dir);
		dropmedia_shutdown();
		return FAILURE;
	}
	references[0] = "";
	prune_storage_root(prune_root, references);
	if (fs::exists(keep_first) || (!fs::exists(keep_second))) {
		fs::remove_all(test_dir);
		dropmedia_shutdown();
		return FAILURE;
	}
	fs::remove_all(test_dir);
	dropmedia_shutdown();
#endif
	return SUCCESS;
}
