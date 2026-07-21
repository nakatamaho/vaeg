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
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "compiler.h"
#include "hostfat_snapshot.h"
#include "hostfat.h"

namespace fs = std::filesystem;

namespace {

constexpr std::size_t kSectorSize = HOSTFAT_SECTOR_SIZE;
constexpr std::size_t kTotalSectors = HOSTFAT_TOTAL_SECTORS;
constexpr std::size_t kSectorsPerCluster = 2;
constexpr std::size_t kClusterSize = kSectorSize * kSectorsPerCluster;
constexpr std::size_t kFatSectors = 7;
constexpr std::size_t kFatCopies = 2;
constexpr std::size_t kRootEntries = 128;
constexpr std::size_t kRootSectors =
	(kRootEntries * 32 + kSectorSize - 1) / kSectorSize;
constexpr std::size_t kDataStartSector = kFatSectors * kFatCopies + kRootSectors;
constexpr std::size_t kDataClusters =
	(kTotalSectors - kDataStartSector) / kSectorsPerCluster;
constexpr std::size_t kFat12ClusterLimit = 4085;
constexpr std::size_t kFatEntries = 4096;
constexpr std::size_t kFirstReservedFat12Cluster = 0x0ff0;
constexpr unsigned kMaximumDepth = 8;
constexpr unsigned kMaximumEntries = 1024;
static_assert(kDataClusters < kFat12ClusterLimit,
	"HOSTFAT DOS-visible geometry must remain FAT12");
static_assert(HOSTFAT_BACKING_SECTORS >= HOSTFAT_TOTAL_SECTORS,
	"HOSTFAT backing must contain every DOS-visible sector");
constexpr std::array<unsigned char, 11> kVolumeLabel = {
	'H', 'O', 'S', 'T', 'F', 'A', 'T', ' ', ' ', ' ', ' '
};

struct Node {
	fs::path path;
	std::array<unsigned char, 11> dos_name{};
	bool directory = false;
	std::uint32_t size = 0;
	fs::file_time_type modified_time{};
	std::uintmax_t hard_links = 0;
	std::uint16_t first_cluster = 0;
	std::uint16_t cluster_count = 0;
	Node *parent = nullptr;
	std::vector<std::unique_ptr<Node>> children;
};

struct BuildState {
	unsigned entries = 0;
	unsigned files = 0;
	unsigned directories = 0;
	std::uint64_t source_bytes = 0;
	std::vector<fs::path> regular_files;
	std::string error;
};

void set_error(char *destination, UINT size, const std::string &message) {

	if ((destination != nullptr) && (size != 0)) {
		std::snprintf(destination, size, "%s", message.c_str());
	}
}

void store_word(unsigned char *destination, std::uint16_t value) {

	destination[0] = static_cast<unsigned char>(value);
	destination[1] = static_cast<unsigned char>(value >> 8);
}

void store_dword(unsigned char *destination, std::uint32_t value) {

	destination[0] = static_cast<unsigned char>(value);
	destination[1] = static_cast<unsigned char>(value >> 8);
	destination[2] = static_cast<unsigned char>(value >> 16);
	destination[3] = static_cast<unsigned char>(value >> 24);
}

std::uint16_t load_word(const unsigned char *source) {

	return static_cast<std::uint16_t>(source[0] |
		(static_cast<std::uint16_t>(source[1]) << 8));
}

bool dos_character(unsigned char value) {

	return ((value >= 'A') && (value <= 'Z')) ||
		((value >= '0') && (value <= '9')) || value == '_' || value == '-';
}

bool make_dos_name(const std::string &source,
		std::array<unsigned char, 11> &destination) {

	std::size_t dot;
	std::string base;
	std::string extension;

	if (source.empty() || source == "." || source == "..") {
		return false;
	}
	dot = source.find('.');
	if ((dot != std::string::npos) &&
		(source.find('.', dot + 1) != std::string::npos)) {
		return false;
	}
	base = (dot == std::string::npos) ? source : source.substr(0, dot);
	extension = (dot == std::string::npos) ? std::string() : source.substr(dot + 1);
	if (base.empty() || (base.size() > 8) || (extension.size() > 3) ||
		((dot != std::string::npos) && extension.empty())) {
		return false;
	}
	destination.fill(' ');
	for (std::size_t position = 0; position < base.size(); position++) {
		unsigned char value = static_cast<unsigned char>(base[position]);
		if ((value >= 'a') && (value <= 'z')) {
			value = static_cast<unsigned char>(value - 'a' + 'A');
		}
		if (!dos_character(value)) {
			return false;
		}
		destination[position] = value;
	}
	for (std::size_t position = 0; position < extension.size(); position++) {
		if (position >= 3) {
			return false;
		}
		unsigned char value = static_cast<unsigned char>(extension[position]);
		if ((value >= 'a') && (value <= 'z')) {
			value = static_cast<unsigned char>(value - 'a' + 'A');
		}
		if (!dos_character(value)) {
			return false;
		}
		destination[8 + position] = value;
	}
	return true;
}

bool hard_link_count(const fs::path &path, std::uintmax_t &count,
		std::error_code &error) {

#if defined(_WIN32)
	const HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE) {
		error = std::error_code(static_cast<int>(GetLastError()),
			std::system_category());
		return false;
	}
	BY_HANDLE_FILE_INFORMATION information{};
	if (!GetFileInformationByHandle(handle, &information)) {
		error = std::error_code(static_cast<int>(GetLastError()),
			std::system_category());
		CloseHandle(handle);
		return false;
	}
	CloseHandle(handle);
	count = information.nNumberOfLinks;
#else
	count = fs::hard_link_count(path, error);
	if (error) {
		return false;
	}
#endif
	error.clear();
	return true;
}

bool sort_and_check_children(Node &node, BuildState &state) {

	std::sort(node.children.begin(), node.children.end(),
		[](const std::unique_ptr<Node> &left, const std::unique_ptr<Node> &right) {
			return left->dos_name < right->dos_name;
		});
	for (std::size_t position = 1; position < node.children.size(); position++) {
		if (node.children[position - 1]->dos_name ==
			node.children[position]->dos_name) {
			state.error = "case-insensitive 8.3 name collision";
			return false;
		}
	}
	return true;
}

bool scan_directory(Node &node, unsigned depth, BuildState &state) {

	std::error_code error;
	fs::directory_iterator iterator(node.path, error);
	fs::directory_iterator end;

	if (error) {
		state.error = "cannot enumerate host directory: " + error.message();
		return false;
	}
	for (; iterator != end; iterator.increment(error)) {
		if (error) {
			state.error = "cannot continue host-directory scan: " + error.message();
			return false;
		}
		const fs::directory_entry &item = *iterator;
		const fs::file_status status = item.symlink_status(error);
		if (error) {
			state.error = "cannot inspect host entry: " + error.message();
			return false;
		}
		if (fs::is_symlink(status)) {
			state.error = "symbolic links are not supported: " +
				item.path().filename().u8string();
			return false;
		}
		if (!fs::is_directory(status) && !fs::is_regular_file(status)) {
			state.error = "special files are not supported: " +
				item.path().filename().u8string();
			return false;
		}
		if (++state.entries > kMaximumEntries) {
			state.error = "host directory exceeds the M54 entry limit";
			return false;
		}
		auto child = std::make_unique<Node>();
		child->path = item.path();
		child->parent = &node;
		child->directory = fs::is_directory(status);
		const std::string name = item.path().filename().u8string();
		if (!make_dos_name(name, child->dos_name)) {
			state.error = "entry is not an ASCII 8.3 name: " + name;
			return false;
		}
		if (child->directory) {
			if (depth >= kMaximumDepth) {
				state.error = "host directory exceeds the M54 depth limit";
				return false;
			}
			state.directories++;
			if (!scan_directory(*child, depth + 1, state)) {
				return false;
			}
		}
		else {
			const std::uintmax_t size = item.file_size(error);
			if (error ||
				(size > (std::numeric_limits<std::uint32_t>::max)())) {
				state.error = "cannot represent host file size: " + name;
				return false;
			}
			child->size = static_cast<std::uint32_t>(size);
			child->modified_time = item.last_write_time(error);
			if (error) {
				state.error = "cannot inspect host file timestamp: " + name;
				return false;
			}
			if (!hard_link_count(item.path(), child->hard_links, error)) {
				state.error = "cannot inspect host file links: " + name;
				return false;
			}
			if (child->hard_links != 1) {
				state.error = "hard-linked files are not supported: " + name;
				return false;
			}
			for (const fs::path &previous : state.regular_files) {
				const bool same_file = fs::equivalent(previous, item.path(), error);
				if (error) {
					state.error = "cannot compare host file identities: " + name;
					return false;
				}
				if (same_file) {
					state.error = "hard-linked files are not supported: " + name;
					return false;
				}
			}
			state.regular_files.push_back(item.path());
			state.files++;
			state.source_bytes += size;
		}
		node.children.push_back(std::move(child));
	}
	if (error) {
		state.error = "cannot finish host-directory scan: " + error.message();
		return false;
	}
	return sort_and_check_children(node, state);
}

bool assign_clusters(Node &node, std::size_t &next_cluster,
		BuildState &state) {

	if (node.parent != nullptr) {
		const std::size_t bytes = node.directory ?
			(node.children.size() + 2) * 32 : node.size;
		std::size_t clusters = (bytes + kClusterSize - 1) / kClusterSize;
		if (node.directory && (clusters == 0)) {
			clusters = 1;
		}
		if ((clusters > UINT16_MAX) ||
			(next_cluster + clusters > 2 + kDataClusters) ||
			(next_cluster + clusters > kFirstReservedFat12Cluster)) {
			state.error = "host directory does not fit the 8 MiB snapshot";
			return false;
		}
		node.cluster_count = static_cast<std::uint16_t>(clusters);
		if (clusters != 0) {
			node.first_cluster = static_cast<std::uint16_t>(next_cluster);
			next_cluster += clusters;
		}
	}
	for (auto &child : node.children) {
		if (!assign_clusters(*child, next_cluster, state)) {
			return false;
		}
	}
	return true;
}

void add_chain(const Node &node, std::vector<std::uint16_t> &fat) {

	for (std::size_t index = 0; index < node.cluster_count; index++) {
		const std::size_t cluster = node.first_cluster + index;
		fat[cluster] = static_cast<std::uint16_t>(
			(index + 1 == node.cluster_count) ? 0x0fff : cluster + 1);
	}
	for (const auto &child : node.children) {
		add_chain(*child, fat);
	}
}

void write_directory_entry(unsigned char *destination,
		const std::array<unsigned char, 11> &name, unsigned char attributes,
		std::uint16_t cluster, std::uint32_t size) {

	std::copy(name.begin(), name.end(), destination);
	destination[11] = attributes;
	store_word(destination + 22, 0);
	store_word(destination + 24, 0x0021);
	store_word(destination + 26, cluster);
	store_dword(destination + 28, size);
}

std::size_t cluster_offset(std::uint16_t cluster) {

	return kDataStartSector * kSectorSize +
		(static_cast<std::size_t>(cluster) - 2) * kClusterSize;
}

bool file_identity_is_unchanged(const Node &node, BuildState &state) {

	std::error_code error;
	const fs::file_status status = fs::symlink_status(node.path, error);
	if (error || fs::is_symlink(status) || !fs::is_regular_file(status)) {
		state.error = "host file type changed while creating snapshot";
		return false;
	}
	const std::uintmax_t size = fs::file_size(node.path, error);
	if (error || (size != node.size)) {
		state.error = "host file size changed while creating snapshot";
		return false;
	}
	const fs::file_time_type modified_time = fs::last_write_time(node.path, error);
	if (error || (modified_time != node.modified_time)) {
		state.error = "host file timestamp changed while creating snapshot";
		return false;
	}
	std::uintmax_t current_hard_links = 0;
	if (!hard_link_count(node.path, current_hard_links, error) ||
		(current_hard_links != node.hard_links) || (current_hard_links != 1)) {
		state.error = "host file link identity changed while creating snapshot";
		return false;
	}
	return true;
}

bool write_stable_file(const Node &node, unsigned char *destination,
		BuildState &state) {

	if (!file_identity_is_unchanged(node, state)) {
		return false;
	}
	std::ifstream input(node.path, std::ios::binary);
	if (!input) {
		state.error = "cannot open host file while creating snapshot";
		return false;
	}
	if (node.size != 0) {
		input.read(reinterpret_cast<char *>(destination),
			static_cast<std::streamsize>(node.size));
	}
	if ((static_cast<std::uint32_t>(input.gcount()) != node.size) ||
		(input.peek() != std::char_traits<char>::eof()) ||
		!file_identity_is_unchanged(node, state)) {
		if (state.error.empty()) {
			state.error = "host file changed while creating snapshot";
		}
		return false;
	}

	std::ifstream verification(node.path, std::ios::binary);
	if (!verification) {
		state.error = "cannot reopen host file while verifying snapshot";
		return false;
	}
	std::array<unsigned char, 65536> buffer{};
	std::size_t position = 0;
	while (position < node.size) {
		const std::size_t amount = std::min<std::size_t>(
			buffer.size(), static_cast<std::size_t>(node.size) - position);
		verification.read(reinterpret_cast<char *>(buffer.data()),
			static_cast<std::streamsize>(amount));
		if ((static_cast<std::size_t>(verification.gcount()) != amount) ||
			!std::equal(buffer.begin(), buffer.begin() + amount,
				destination + position)) {
			state.error = "host file content changed while creating snapshot";
			return false;
		}
		position += amount;
	}
	if ((verification.peek() != std::char_traits<char>::eof()) ||
		!file_identity_is_unchanged(node, state)) {
		if (state.error.empty()) {
			state.error = "host file changed while verifying snapshot";
		}
		return false;
	}
	return true;
}

bool write_node(Node &node, std::vector<unsigned char> &image,
		BuildState &state) {

	if (node.parent != nullptr) {
		unsigned char *destination = image.data() + cluster_offset(node.first_cluster);
		if (node.directory) {
			std::array<unsigned char, 11> dot{};
			std::array<unsigned char, 11> dotdot{};
			dot.fill(' ');
			dotdot.fill(' ');
			dot[0] = '.';
			dotdot[0] = '.';
			dotdot[1] = '.';
			write_directory_entry(destination, dot, 0x10,
				node.first_cluster, 0);
			write_directory_entry(destination + 32, dotdot, 0x10,
				(node.parent->parent == nullptr) ? 0 : node.parent->first_cluster, 0);
			for (std::size_t index = 0; index < node.children.size(); index++) {
				const Node &child = *node.children[index];
				write_directory_entry(destination + (index + 2) * 32,
					child.dos_name, child.directory ? 0x10 : 0x21,
					child.first_cluster, child.size);
			}
		}
		else if (!write_stable_file(node, destination, state)) {
			return false;
		}
	}
	for (auto &child : node.children) {
		if (!write_node(*child, image, state)) {
			return false;
		}
	}
	return true;
}

bool build_image(const fs::path &root_path, std::vector<unsigned char> &image,
		BuildState &state) {

	std::error_code error;
	const fs::file_status root_status = fs::symlink_status(root_path, error);
	if (error || fs::is_symlink(root_status)) {
		state.error = "HOSTFAT root must not be a symbolic link";
		return false;
	}
	const fs::path canonical = fs::canonical(root_path, error);
	if (error || !fs::is_directory(canonical, error) || error) {
		state.error = "HOSTFAT root is not a readable directory";
		return false;
	}
	Node root;
	root.path = canonical;
	root.directory = true;
	if (!scan_directory(root, 0, state)) {
		return false;
	}
	if (root.children.size() + 1 > kRootEntries) {
		state.error = "HOSTFAT root exceeds 127 entries";
		return false;
	}
	std::size_t next_cluster = 2;
	if (!assign_clusters(root, next_cluster, state)) {
		return false;
	}
	image.assign(HOSTFAT_IMAGE_SIZE, 0);
	std::vector<std::uint16_t> fat(kFatEntries, 0);
	fat[0] = 0x0ff0;
	fat[1] = 0x0fff;
	add_chain(root, fat);
	std::vector<unsigned char> packed_fat(kFatSectors * kSectorSize, 0);
	for (std::size_t entry = 0; entry < kFatEntries; entry += 2) {
		const std::uint16_t first = fat[entry];
		const std::uint16_t second = fat[entry + 1];
		const std::size_t output = (entry / 2) * 3;
		packed_fat[output] = static_cast<unsigned char>(first);
		packed_fat[output + 1] = static_cast<unsigned char>(
			(first >> 8) | ((second & 0x000f) << 4));
		packed_fat[output + 2] = static_cast<unsigned char>(second >> 4);
	}
	for (std::size_t copy = 0; copy < kFatCopies; copy++) {
		std::copy(packed_fat.begin(), packed_fat.end(),
			image.begin() + copy * packed_fat.size());
	}
	unsigned char *root_directory =
		image.data() + kFatCopies * kFatSectors * kSectorSize;
	write_directory_entry(root_directory, kVolumeLabel, 0x08, 0, 0);
	for (std::size_t index = 0; index < root.children.size(); index++) {
		const Node &child = *root.children[index];
		write_directory_entry(root_directory + (index + 1) * 32,
			child.dos_name, child.directory ? 0x10 : 0x21,
			child.first_cluster, child.size);
	}
	return write_node(root, image, state);
}

bool verify_test_image() {

	std::array<unsigned char, kFatSectors * kSectorSize> first_fat{};
	std::array<unsigned char, kFatSectors * kSectorSize> second_fat{};
	std::array<unsigned char, HOSTFAT_SECTOR_SIZE> root{};
	for (std::size_t sector = 0; sector < kFatSectors; sector++) {
		if ((hostfat_read_sector(static_cast<UINT32>(sector),
				first_fat.data() + sector * kSectorSize) != SUCCESS) ||
			(hostfat_read_sector(static_cast<UINT32>(kFatSectors + sector),
				second_fat.data() + sector * kSectorSize) != SUCCESS)) {
			return false;
		}
	}
	if ((first_fat != second_fat) ||
		(hostfat_read_sector(kFatSectors * kFatCopies, root.data()) != SUCCESS)) {
		return false;
	}
	if (!std::equal(kVolumeLabel.begin(), kVolumeLabel.end(), root.begin()) ||
		(root[11] != 0x08)) {
		return false;
	}
	const unsigned char *docs = root.data() + 32;
	const unsigned char *hello = root.data() + 64;
	const std::array<unsigned char, 11> docs_name = {
		'D', 'O', 'C', 'S', ' ', ' ', ' ', ' ', ' ', ' ', ' '
	};
	const std::array<unsigned char, 11> hello_name = {
		'H', 'E', 'L', 'L', 'O', ' ', ' ', ' ', 'T', 'X', 'T'
	};
	if (!std::equal(docs_name.begin(), docs_name.end(), docs) ||
		!std::equal(hello_name.begin(), hello_name.end(), hello) ||
		(docs[11] != 0x10) || (hello[11] != 0x21)) {
		return false;
	}
	const std::uint16_t docs_cluster = load_word(docs + 26);
	const std::uint16_t hello_cluster = load_word(hello + 26);
	auto fat_entry = [&first_fat](std::uint16_t cluster) {
		const std::size_t offset = cluster + cluster / 2;
		std::uint16_t value = static_cast<std::uint16_t>(first_fat[offset] |
			(static_cast<std::uint16_t>(first_fat[offset + 1]) << 8));
		if ((cluster & 1) != 0) {
			value >>= 4;
		}
		return static_cast<std::uint16_t>(value & 0x0fff);
	};
	if ((docs_cluster < 2) || (hello_cluster < 2) ||
		(fat_entry(docs_cluster) != 0x0fff) ||
		(fat_entry(hello_cluster) != 0x0fff)) {
		return false;
	}
	std::array<unsigned char, HOSTFAT_SECTOR_SIZE> sector{};
	const std::size_t hello_sector = kDataStartSector +
		(static_cast<std::size_t>(hello_cluster) - 2) * kSectorsPerCluster;
	if ((hostfat_read_sector(static_cast<UINT32>(hello_sector), sector.data())
			!= SUCCESS) ||
		!std::equal(sector.begin(), sector.begin() + 6, "hello\n")) {
		return false;
	}
	const std::size_t docs_sector = kDataStartSector +
		(static_cast<std::size_t>(docs_cluster) - 2) * kSectorsPerCluster;
	if (hostfat_read_sector(static_cast<UINT32>(docs_sector), sector.data())
			!= SUCCESS) {
		return false;
	}
	const std::array<unsigned char, 11> readme_name = {
		'R', 'E', 'A', 'D', 'M', 'E', ' ', ' ', 'T', 'X', 'T'
	};
	const std::uint16_t readme_cluster = load_word(sector.data() + 64 + 26);
	return (sector[0] == '.') && (sector[32] == '.') &&
		(sector[33] == '.') &&
		std::equal(readme_name.begin(), readme_name.end(), sector.begin() + 64) &&
		(readme_cluster >= 2) && (fat_entry(readme_cluster) == 0x0fff);
}

bool verify_internal_limits() {

	std::array<unsigned char, 11> lowercase{};
	std::array<unsigned char, 11> uppercase{};
	if (!make_dos_name("case.txt", lowercase) ||
		!make_dos_name("CASE.TXT", uppercase) || (lowercase != uppercase) ||
		make_dos_name("bad name.txt", uppercase)) {
		return false;
	}
	Node collision_root;
	for (const char *name : {"case.txt", "CASE.TXT"}) {
		auto child = std::make_unique<Node>();
		if (!make_dos_name(name, child->dos_name)) {
			return false;
		}
		collision_root.children.push_back(std::move(child));
	}
	BuildState collision_state;
	if (sort_and_check_children(collision_root, collision_state)) {
		return false;
	}
	Node root;
	auto oversized = std::make_unique<Node>();
	oversized->parent = &root;
	oversized->size = HOSTFAT_IMAGE_SIZE;
	root.children.push_back(std::move(oversized));
	std::size_t next_cluster = 2;
	BuildState state;
	if (assign_clusters(root, next_cluster, state)) {
		return false;
	}
	state = BuildState{};
	state.entries = kMaximumEntries;
	if (++state.entries <= kMaximumEntries) {
		return false;
	}
	return true;
}

}  // namespace

extern "C" BOOL hostfat_snapshot_mount_directory(const char *path,
		HOSTFAT_SNAPSHOT_INFO *info, char *error, UINT error_size) {

	if ((error != nullptr) && (error_size != 0)) {
		error[0] = '\0';
	}
	if ((path == nullptr) || (path[0] == '\0')) {
		set_error(error, error_size, "HOSTFAT directory path is empty");
		return FAILURE;
	}
	try {
		BuildState state;
		std::vector<unsigned char> image;
		if (!build_image(fs::u8path(path), image, state)) {
			set_error(error, error_size, state.error);
			return FAILURE;
		}
		if (hostfat_mount_image(image.data(), static_cast<UINT32>(image.size()))
				!= SUCCESS) {
			set_error(error, error_size, "cannot allocate HOSTFAT snapshot");
			return FAILURE;
		}
		if (info != nullptr) {
			info->files = state.files;
			info->directories = state.directories;
			info->source_bytes = state.source_bytes;
			info->digest = hostfat_image_digest();
		}
		return SUCCESS;
	}
	catch (const std::exception &exception) {
		set_error(error, error_size,
			std::string("HOSTFAT snapshot failed: ") + exception.what());
		return FAILURE;
	}
}

extern "C" void hostfat_snapshot_unmount(void) {

	hostfat_unmount();
}

extern "C" BOOL hostfat_snapshot_selftest(void) {

	if (!verify_internal_limits()) {
		return FAILURE;
	}
	std::error_code error;
	const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
	const fs::path root = fs::temp_directory_path(error) /
		("vaeg-hostfat-selftest-" + std::to_string(nonce));
	if (error || !fs::create_directories(root / "docs", error) || error) {
		return FAILURE;
	}
	bool result = false;
	try {
		{
			std::ofstream hello(root / "hello.txt", std::ios::binary);
			hello << "hello\n";
		}
		{
			std::ofstream readme(root / "docs" / "readme.txt", std::ios::binary);
			readme << "nested\n";
		}
		HOSTFAT_SNAPSHOT_INFO first{};
		HOSTFAT_SNAPSHOT_INFO second{};
		char message[256];
		if (hostfat_snapshot_mount_directory(root.u8string().c_str(), &first,
				message, sizeof(message)) != SUCCESS) {
			throw std::runtime_error(std::string("valid snapshot mount failed: ") +
				message);
		}
		if ((first.files != 2) || (first.directories != 1) ||
			(first.source_bytes != 13) || !verify_test_image()) {
			throw std::runtime_error("valid snapshot check failed");
		}
		if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), &second,
				message, sizeof(message)) != SUCCESS) ||
			(first.digest != second.digest)) {
			throw std::runtime_error("snapshot regeneration was not deterministic");
		}
		const UINT32 accepted_digest = second.digest;
		{
			std::ofstream invalid(root / "bad name.txt", std::ios::binary);
			invalid << "invalid\n";
		}
		if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), nullptr,
				message, sizeof(message)) == SUCCESS) ||
			(hostfat_image_digest() != accepted_digest)) {
			throw std::runtime_error("invalid name did not fail transactionally");
		}
		fs::remove(root / "bad name.txt", error);
		if (error) {
			throw std::runtime_error("temporary invalid-name cleanup failed");
		}
		error.clear();
		fs::create_symlink(root / "hello.txt", root / "link.txt", error);
		if (!error) {
			if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), nullptr,
					message, sizeof(message)) == SUCCESS) ||
				(hostfat_image_digest() != accepted_digest)) {
				throw std::runtime_error("symbolic link was accepted");
			}
			fs::remove(root / "link.txt", error);
			if (error) {
				throw std::runtime_error("temporary symbolic-link cleanup failed");
			}
		}
		error.clear();
		fs::create_hard_link(root / "hello.txt", root / "hard.txt", error);
		if (!error) {
			if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), nullptr,
					message, sizeof(message)) == SUCCESS) ||
				(hostfat_image_digest() != accepted_digest)) {
				throw std::runtime_error("hard link was accepted");
			}
			fs::remove(root / "hard.txt", error);
			if (error) {
				throw std::runtime_error("temporary hard-link cleanup failed");
			}
		}
		error.clear();
		fs::path nested = root;
		for (unsigned depth = 0; depth <= kMaximumDepth; depth++) {
			nested /= "deep" + std::to_string(depth);
			if (!fs::create_directory(nested, error) || error) {
				throw std::runtime_error("temporary depth-tree creation failed");
			}
		}
		if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), nullptr,
				message, sizeof(message)) == SUCCESS) ||
			(hostfat_image_digest() != accepted_digest)) {
			throw std::runtime_error("excessive directory depth was accepted");
		}
		result = true;
	}
	catch (const std::exception &exception) {
		std::fprintf(stderr, "HOSTFAT selftest detail: %s\n", exception.what());
		result = false;
	}
	catch (...) {
		std::fprintf(stderr, "HOSTFAT selftest detail: unknown exception\n");
		result = false;
	}
	hostfat_snapshot_unmount();
	error.clear();
	fs::remove_all(root, error);
	return result ? SUCCESS : FAILURE;
}
