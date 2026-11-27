/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
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
#include "sdl2/hostdrv_linux.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;
using hostdrv::LinuxRoot;

struct Scratch {
    fs::path path;
    ~Scratch() {
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }
};

#define CHECK(condition, code) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "HOSTDRV_LINUX_%s at line %d\n", code, __LINE__); \
        return 1; \
    } \
} while (false)

bool same_timespec(timespec a, timespec b) {
    return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
}

bool same_times(const struct stat &before, const struct stat &after) {
    return same_timespec(before.st_atim, after.st_atim) && same_timespec(before.st_mtim, after.st_mtim) &&
           same_timespec(before.st_ctim, after.st_ctim);
}
} // namespace

int main() {
    char pattern[] = "./hostdrv-linux-XXXXXX";
    CHECK(mkdtemp(pattern), "SCRATCH");
    const Scratch scratch{pattern};
    const auto root = scratch.path / "root";
    const auto child = root / "dir";
    const auto file = child / "file";
    const auto moved = scratch.path / "moved";
    CHECK(fs::create_directories(child), "FIXTURE");
    CHECK(bool(std::ofstream(file) << "abcdef"), "FIXTURE");
    constexpr timespec times[] = {{946684800, 123}, {946684801, 456}};
    CHECK(utimensat(AT_FDCWD, file.c_str(), times, 0) == 0 &&
          utimensat(AT_FDCWD, child.c_str(), times, 0) == 0, "FIXTURE_TIME");

    const auto mount = LinuxRoot::select(root.string());
    CHECK(mount, "SELECT");
    const auto dir = mount->resolve("dir");
    CHECK(dir, "DIRECTORY");
    const auto pin = mount->resolve_entry("dir/file", *dir);
    CHECK(pin, "RESOLVE");
    struct stat file_before {}, file_after {}, dir_before {}, dir_after {};
    CHECK(pin->metadata(file_before) && dir->metadata(dir_before), "FSTAT");
    std::vector<LinuxRoot::Entry> items;
    CHECK(mount->enumerate("dir", *dir, items) && items.size() == 1 &&
          items.front().raw_name == "file" && items.front().inode == file_before.st_ino, "LIST");
    const auto data = mount->open_file("dir/file", *dir, *pin);
    CHECK(data, "OPEN");
    char buffer[8]{};
    std::uint16_t actual = 0;
    CHECK(LinuxRoot::read(*data, 3, buffer, sizeof(buffer), actual) && actual == 3 &&
          std::string_view(buffer, actual) == "def", "POSITIONED_READ");
    CHECK(LinuxRoot::read(*data, 6, buffer, sizeof(buffer), actual) && actual == 0, "EOF");
    CHECK(pin->metadata(file_after) && dir->metadata(dir_after) &&
          same_times(file_before, file_after) && same_times(dir_before, dir_after), "TIMES_CHANGED");
    std::uint64_t capacity = 0;
    CHECK(mount->capacity(capacity) && capacity != 0, "CAPACITY");

    CHECK(!mount->resolve("dir/../dir/file") && !mount->resolve("/dir/file") &&
          !mount->resolve("dir//file") && !mount->resolve("dir/"), "INVALID_PATH");
    fs::create_symlink("file", child / "link");
    CHECK(!mount->resolve("dir/link"), "SYMLINK");
    fs::create_directory_symlink("dir", root / "link");
    CHECK(!mount->resolve("link/file"), "SYMLINK_PREFIX");
    fs::create_hard_link(file, child / "hardlink");
    CHECK(!mount->resolve("dir/file") && !mount->open_file("dir/file", *dir, *pin), "HARDLINK");
    CHECK(fs::remove(child / "hardlink"), "FIXTURE_CLEANUP");
    CHECK(mkfifo((child / "pipe").c_str(), 0600) == 0, "FIXTURE_FIFO");
    CHECK(!mount->resolve("dir/pipe"), "FIFO");
    CHECK(!mount->open_file("dir", *mount->resolve(""), *dir), "NOT_REGULAR");
    const auto large = child / "large";
    CHECK(bool(std::ofstream(large)), "FIXTURE_LARGE");
    fs::resize_file(large, 0x80000000ull);
    CHECK(!mount->resolve("dir/large"), "LARGE_FILE");
    fs::create_directory_symlink("root", scratch.path / "root-link");
    CHECK(!LinuxRoot::select((scratch.path / "root-link").string()), "ROOT_LINK");

    fs::rename(child, moved);
    CHECK(!mount->open_file("dir/file", *dir, *pin) && !mount->enumerate("dir", *dir, items),
          "PARENT_MOVED");
    CHECK(LinuxRoot::read(*data, 0, buffer, 3, actual) && actual == 3 &&
          std::string_view(buffer, actual) == "abc", "OPEN_SURVIVES_MOVE");
    CHECK(fs::create_directory(child), "FIXTURE_PARENT_REPLACEMENT");
    fs::rename(moved / "file", file);
    const auto moved_pin = mount->resolve("dir/file");
    CHECK(moved_pin && moved_pin->same_object(*pin) && !mount->open_file("dir/file", *dir, *pin),
          "STALE_PARENT_PIN");
    const auto new_dir = mount->resolve("dir");
    CHECK(new_dir && mount->open_file("dir/file", *new_dir, *pin), "OPEN_IN_NEW_PARENT");
    fs::rename(file, moved / "file");
    CHECK(bool(std::ofstream(file) << "replacement"), "FIXTURE_REPLACEMENT");
    const auto replacement = mount->resolve("dir/file");
    CHECK(replacement && !replacement->same_object(*pin) && !mount->open_file("dir/file", *new_dir, *pin),
          "STALE_FILE_PIN");

    const auto renamed_root = scratch.path / "renamed-root";
    fs::rename(root, renamed_root);
    CHECK(fs::create_directory(root), "FIXTURE_ROOT_REPLACEMENT");
    const auto after_rename = mount->resolve("dir/file");
    CHECK(after_rename && after_rename->same_object(*replacement), "ROOT_RETARGETED");

    // Exactly 4096 raw entries pass, even though none has been published in
    // the DOS namespace. One additional entry fails with no partial output.
    const auto crowded = renamed_root / "crowded";
    CHECK(fs::create_directory(crowded), "FIXTURE_CROWDED");
    for (int i = 0; i < 4096; ++i) {
        CHECK(bool(std::ofstream(crowded / std::to_string(i))), "FIXTURE_ENTRY");
    }
    const auto crowded_pin = mount->resolve("crowded");
    CHECK(crowded_pin && mount->enumerate("crowded", *crowded_pin, items) && items.size() == 4096,
          "QUOTA_BASELINE");
    CHECK(bool(std::ofstream(crowded / "overflow")), "FIXTURE_OVERFLOW");
    items.clear();
    CHECK(!mount->enumerate("crowded", *crowded_pin, items) && items.empty(), "QUOTA_OVERFLOW");
    return 0;
}
