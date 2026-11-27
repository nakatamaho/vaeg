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
#include "sdl2/hostdrv_linux_namespace.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

namespace {
namespace fs = std::filesystem;
using hostdrv::LinuxNamespace;
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
        std::cerr << "HD_NAMESPACE_" << code << '\n'; \
        return 1; \
    } \
} while (false)

bool put(const fs::path &path, std::string_view data) {
    std::ofstream out(path);
    out << data;
    out.close();
    return bool(out);
}
}

int main() {
    char pattern[] = "./hostdrv-namespace-XXXXXX";
    CHECK(mkdtemp(pattern), "SCRATCH");
    const Scratch scratch{pattern};
    const auto root = scratch.path / "root";
    CHECK(fs::create_directories(root / "dir") && put(root / "abc.txt", "lowercase") &&
              put(root / "ABC~1.TXT", "reserved") && put(root / "ABC.TXT", "upper") &&
              put(root / "dir" / u8"表.txt", "japanese"), "FIXTURE");
    auto selected = LinuxRoot::select(root.string());
    CHECK(selected, "SELECT");
    LinuxNamespace names(std::move(*selected));
    LinuxNamespace::Item item;
    std::vector<LinuxNamespace::Item> list;
    std::optional<LinuxRoot::Handle> file;
    CHECK(names.list("", list) == LinuxNamespace::ok && list.size() == 4 && list[0].name == "ABC     TXT" &&
              list[1].name == "ABC~1   TXT" && list[2].name == "ABC~2   TXT" && list[3].name == "DIR        ",
          "INITIAL_RESERVATION");
    CHECK(names.stat("ABC.TXT", item) == LinuxNamespace::ok && item.info.st_size == 5 &&
              names.stat("ABC~1.TXT", item) == LinuxNamespace::ok && item.info.st_size == 8 &&
              names.open("ABC~2.TXT", file, item) == LinuxNamespace::ok && item.info.st_size == 9,
          "NATIVE_COLLISION");
    CHECK(names.stat("DIR\\\x95\x5c.TXT", item) == LinuxNamespace::ok && item.info.st_size == 8,
          "DBCS_LOOKUP");
    fs::rename(root / "abc.txt", root / "abc.TXT");
    CHECK(names.stat("ABC~2.TXT", item) == LinuxNamespace::ok && item.info.st_size == 9, "CASE_ONLY");
    CHECK(fs::remove(root / "abc.TXT") && put(root / "abc.TXT", "new"), "REPLACEMENT_SETUP");
    CHECK(names.stat("ABC~2.TXT", item) == LinuxNamespace::missing &&
              names.stat("ABC~3.TXT", item) == LinuxNamespace::ok && item.info.st_size == 3,
          "REPLACEMENT_TOMBSTONE");
    std::uint16_t count;
    char data[16];
    CHECK(LinuxRoot::read(*file, 0, data, sizeof(data), count) && std::string_view(data, count) == "lowercase",
          "PINNED_READ");
    CHECK(fs::remove(root / "ABC.TXT") && names.stat("ABC.TXT", item) == LinuxNamespace::missing,
          "OBSERVED_REMOVAL");
    CHECK(put(root / "ABC.TXT", "fresh"), "REINSERT_SETUP");
    CHECK(names.stat("ABC.TXT", item) == LinuxNamespace::missing &&
              names.stat("ABC~4.TXT", item) == LinuxNamespace::ok && item.info.st_size == 5,
          "NO_RESURRECTION");
    fs::rename(root / "dir", scratch.path / "moved");
    CHECK(names.stat("DIR\\\x95\x5c.TXT", item) == LinuxNamespace::bad_path, "PARENT_MOVED");
    CHECK(fs::create_directory(root / "dir") && put(root / "dir" / u8"表.txt", "replacement"),
          "PARENT_REPLACEMENT_SETUP");
    CHECK(names.stat("DIR\\\x95\x5c.TXT", item) == LinuxNamespace::bad_path &&
              names.stat("DIR~1\\\x95\x5c.TXT", item) == LinuxNamespace::ok && item.info.st_size == 11,
          "PARENT_REPLACEMENT");
    CHECK(put(root / ".hidden", "hidden") && put(root / "\xff", "invalid"), "HIDDEN_SETUP");
    CHECK(names.list("", list) == LinuxNamespace::ok && list.size() == 5 && list.back().hidden,
          "LIVE_HIDDEN_AND_INVALID_UTF8");
    return 0;
}
