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
#include "sdl2/hostdrv_linux_service.h"

extern "C" {
#include "io/hostdrv_wire.h"
}

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

namespace {
namespace fs = std::filesystem;
struct Scratch {
    fs::path path;
    ~Scratch() {
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }
};

void check(bool pass, const char *code) {
    if (!pass) {
        std::cerr << "HD_SERVICE_" << code << '\n';
        std::exit(1);
    }
}

struct Exchange {
    hostdrv::LinuxService service;
    std::array<std::uint8_t, HD_PACKET> packet{};
    std::uint32_t generation = 0;
    std::uint16_t seq = 0;
    int busy = 0;

    unsigned call(unsigned op, std::string_view path = {}, unsigned options = 0,
                  std::uint32_t token = 0, std::uint32_t offset = 0) {
        packet.fill(0);
        memcpy(packet.data(), "HDR1", 4);
        packet[HD_VERSION] = 1;
        packet[HD_OPCODE] = op;
        hostdrv_put16(packet.data() + HD_SEQUENCE, ++seq);
        hostdrv_put32(packet.data() + HD_GENERATION, op == HD_QUERY ? 0 : generation);
        hostdrv_put32(packet.data() + HD_TOKEN, token);
        hostdrv_put32(packet.data() + HD_OFFSET, offset);
        hostdrv_put16(packet.data() + HD_OPTIONS, options);
        hostdrv_put16(packet.data() + HD_PATH, path.size());
        unsigned input = path.size(), capacity = 0;
        if (!path.empty()) {
            memcpy(packet.data() + HD_HEADER, path.data(), path.size());
        }
        if (op == HD_FIND_FIRST) {
            memcpy(packet.data() + HD_HEADER + input, "???????????", 11);
            input += 11;
        }
        if (op == HD_FIND_FIRST || op == HD_FIND_NEXT) {
            capacity = 11;
        } else if (op == HD_SPACE) {
            capacity = 16;
        } else if (op == HD_READ) {
            capacity = 8;
            hostdrv_put16(packet.data() + HD_COUNT, capacity);
        }
        hostdrv_put16(packet.data() + HD_INPUT, input);
        hostdrv_put16(packet.data() + HD_CAPACITY, capacity);
        const auto request = packet;
        const auto result = hostdrv_exchange(&busy, packet.data(), service.dispatch, &service);
        if (result == HD_REPLY_BASE + HD_BUSY) {
            check(packet == request, "BUSY_WROTE_RAM");
            return 0x100 + HD_BUSY;
        }
        check(hostdrv_response_valid(result, request.data(), packet.data()), "RESPONSE_CONTRACT");
        return word(HD_STATUS);
    }
    std::uint16_t word(unsigned offset) const { return hostdrv_get16(packet.data() + offset); }
    std::uint32_t dword(unsigned offset) const { return hostdrv_get32(packet.data() + offset); }
    std::string_view payload() const {
        return {reinterpret_cast<const char *>(packet.data() + HD_HEADER), word(HD_OUTPUT)};
    }
};
}

int main() {
    char pattern[] = "./hostdrv-service-XXXXXX";
    check(mkdtemp(pattern) != nullptr, "SCRATCH");
    const Scratch scratch{pattern};
    const auto root = scratch.path / "root";
    check(fs::create_directories(root / "dir"), "ROOT");
    std::ofstream(root / "A.TXT") << "abcdef";
    std::ofstream(root / "B.TXT") << "before";
    std::ofstream(root / ".hidden") << "hidden";
    check(setenv("TZ", "UTC0", 1) == 0, "TZ");
    const struct timespec timestamp[2]{{946684803, 0}, {946684803, 0}};
    check(utimensat(AT_FDCWD, (root / "A.TXT").c_str(), timestamp, 0) == 0, "TIME_SETUP");

    Exchange x;
    check(x.call(HD_QUERY) == 21 && x.service.enable(root.string()) && x.call(HD_QUERY) == 0, "QUERY");
    x.generation = x.dword(HD_GENERATION);
    check(x.call(HD_STAT, "A.TXT") == 21 && x.call(HD_ATTACH) == 0 && x.service.guarded(), "ATTACH");
    check(x.call(HD_ATTACH) == 0x100 + HD_BUSY && !x.service.load_uninstalled({100, 100}), "RESIDENT_GUARD");
    check(x.call(HD_STAT) == 0 && x.word(HD_ATTR) == 0x11 && x.dword(HD_SIZE) == 0, "ROOT_METADATA");
    check(x.call(HD_STAT, "A.TXT") == 0 && x.word(HD_ATTR) == 0x21 && x.word(HD_DATE) == 0x2821 &&
              x.word(HD_TIME) == 1, "LOCAL_DOS_TIME");
    check(x.call(HD_SPACE) == 0 && x.payload().size() == 16 && x.payload().substr(8) == std::string(8, 0),
          "SPACE_READONLY");
    check(x.call(HD_OPEN, "A.TXT", 1) == 5, "DENY_WRITE");
    check(x.call(HD_OPEN, "A.TXT", 0x10) == 0, "OPEN_EXCLUSIVE");
    const auto file = x.dword(HD_TOKEN);
    const auto serial = x.word(HD_SERIAL);
    check(x.call(HD_OPEN, "A.TXT") == 32, "SHARING");
    check(x.call(HD_READ, {}, 0, file, 2) == 0 && x.payload() == "cdef", "POSITIONED_READ");
    check(x.call(HD_READ, {}, 0, file, 6) == 0 && x.payload().empty(), "EOF");
    fs::rename(root / "A.TXT", scratch.path / "moved");
    check(x.call(HD_STAT, "A.TXT") == 2 && x.call(HD_FSTAT, {}, 0, file) == 0 && x.dword(HD_SIZE) == 6,
          "PINNED_FSTAT");
    std::ofstream(root / "C.TXT") << "new";
    check(x.call(HD_FIND_FIRST, {}, 0x08) == 18, "NO_VOLUME_LABEL");
    check(x.call(HD_FIND_FIRST) == 0 && x.payload() == "B       TXT", "FIRST_FILTER_SORT");
    const auto search = x.dword(HD_TOKEN);
    check(x.call(HD_CLOSE, {}, 0, search) == 6, "TYPED_FILE_TOKEN");
    check(fs::remove(root / "C.TXT"), "REMOVE_AFTER_SNAPSHOT");
    check(x.call(HD_FIND_NEXT, {}, 0, search, 1) == 0 && x.payload() == "C       TXT" &&
              x.call(HD_FIND_NEXT, {}, 0, search, 2) == 18 &&
              x.call(HD_FIND_NEXT, {}, 0, search, 1) == 0 && x.payload() == "C       TXT", "REPLAY_AFTER_EOF");
    for (unsigned i = 0; i < 32; ++i) {
        check(x.call(HD_FIND_FIRST) == 0 && x.payload() == "B       TXT", "NEW_SNAPSHOT");
    }
    check(x.call(HD_FIND_NEXT, {}, 0, search, 1) == 18, "OLDEST_EVICTION");
    check(x.call(HD_DETACH) == 0x100 + HD_BUSY, "DETACH_BUSY");
    check(x.call(HD_CLOSE, {}, 0, file) == 0 && x.call(HD_CLOSE, {}, 0, file) == 6, "RETIRE_FILE");
    x.service.revoke();
    check(x.service.guarded() && x.call(HD_QUERY) == 21 && x.call(HD_FSTAT, {}, 0, file) == 21 &&
              x.call(HD_ATTACH) == 0x100 + HD_BUSY && x.call(HD_UNREGISTER) == 0, "REVOKE_AND_UNREGISTER");
    check(x.service.enable(root.string()) && x.call(HD_QUERY) == 0 && x.dword(HD_GENERATION) > x.generation,
          "FRESH_GENERATION");
    const auto next_generation = x.dword(HD_GENERATION);
    check(x.call(HD_ATTACH) == 21, "STALE_ATTACH");
    x.generation = next_generation;
    check(x.call(HD_ATTACH) == 0, "REATTACH_FRESH");
    std::vector<std::uint32_t> files;
    for (unsigned i = 0; i < 64; ++i) {
        check(x.call(HD_OPEN, "B.TXT", 0x20) == 0 && x.word(HD_SERIAL) > serial, "FRESH_SERIAL");
        files.push_back(x.dword(HD_TOKEN));
    }
    check(x.call(HD_OPEN, "B.TXT") == 4, "HANDLE_CAP");
    for (auto id : files) {
        check(x.call(HD_CLOSE, {}, 0, id) == 0, "DRAIN_HANDLE");
    }
    check(x.call(HD_FIND_FIRST, {}, 0x12) == 0 && x.payload() == "B       TXT", "SEARCH_TO_RELEASE");
    const auto release = x.dword(HD_TOKEN);
    check(x.call(HD_FIND_RELEASE, {}, 0, release) == 0 && x.call(HD_FIND_RELEASE, {}, 0, release) == 18,
          "RELEASE_SEARCH");
    check(x.call(HD_DETACH) == 0 && x.service.guarded() && x.call(HD_UNREGISTER) == 0, "NORMAL_TEARDOWN");
    check(x.service.load_uninstalled({1, 1}) && x.service.marks().generation == x.generation &&
              x.service.marks().serial > serial, "MARKS_NO_ROLLBACK");
    check(x.service.load_uninstalled({0, 65534}) && x.service.enable(root.string()) && x.call(HD_QUERY) == 0,
          "SERIAL_LIMIT_SETUP");
    x.generation = x.dword(HD_GENERATION);
    check(x.call(HD_ATTACH) == 0 && x.call(HD_OPEN, "B.TXT") == 0 && x.word(HD_SERIAL) == 65535,
          "FINAL_NONZERO_SERIAL");
    check(x.call(HD_CLOSE, {}, 0, x.dword(HD_TOKEN)) == 0 && x.call(HD_OPEN, "B.TXT") == 4,
          "SERIAL_EXHAUSTION");
    x.service.discard_dos_image();
    check(!x.service.guarded() && x.call(HD_QUERY) == 21, "DISCARD_IMAGE");
    check(x.service.load_uninstalled({std::numeric_limits<std::uint32_t>::max(), 65535}) &&
              !x.service.enable(root.string()), "GENERATION_EXHAUSTION");
}
