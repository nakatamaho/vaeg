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
#include "hostdrv_linux_service.h"

extern "C" {
#include "io/hostdrv_wire.h"
}

#include <algorithm>
#include <cstring>
#include <ctime>
#include <limits>
#include <new>
#include <utility>

namespace hostdrv {
namespace {

constexpr auto token_max = std::numeric_limits<std::uint32_t>::max();
constexpr auto serial_max = std::numeric_limits<std::uint16_t>::max();
constexpr std::size_t max_files = 64, max_searches = 32;

unsigned attributes(const struct stat &st, bool hidden) {
    return 1u | (hidden ? 2u : 0u) | (S_ISDIR(st.st_mode) ? 0x10u : 0x20u);
}

bool dos_metadata(std::uint8_t *out, const struct stat &st, bool hidden) {
    const bool directory = S_ISDIR(st.st_mode);
    if (!directory && (!S_ISREG(st.st_mode) || st.st_size < 0 || st.st_size > 0x7fffffff)) {
        return false;
    }
    std::tm time {};
    if (!localtime_r(&st.st_mtim.tv_sec, &time)) {
        return false;
    }
    unsigned date, clock;
    if (time.tm_year < 80) {
        date = 0x21; // 1980-01-01 00:00:00
        clock = 0;
    } else if (time.tm_year > 207) {
        date = (127u << 9) | (12u << 5) | 31u;
        clock = (23u << 11) | (59u << 5) | 29u;
    } else {
        date = ((time.tm_year - 80) << 9) | ((time.tm_mon + 1) << 5) | time.tm_mday;
        clock = (time.tm_hour << 11) | (time.tm_min << 5) | (std::min(time.tm_sec, 59) / 2);
    }
    hostdrv_put32(out, directory ? 0 : static_cast<std::uint32_t>(st.st_size));
    hostdrv_put16(out + 4, attributes(st, hidden));
    hostdrv_put16(out + 6, clock);
    hostdrv_put16(out + 8, date);
    return true;
}

} // namespace

bool LinuxService::enable(const std::string &path) {
    if (guarded() || names_ || marks_.generation == token_max) {
        return false;
    }
    auto root = LinuxRoot::select(path);
    if (!root) {
        return false;
    }
    names_.emplace(std::move(*root));
    ++marks_.generation;
    return true;
}

void LinuxService::revoke() {
    files_.clear();
    searches_.clear();
    names_.reset();
    attached_ = false;
    next_token_ = 0;
    // No enabled generation survives. The next enable must advance the mark.
}

void LinuxService::discard_dos_image() {
    revoke();
    installation_ = 0;
}

bool LinuxService::load_uninstalled(Marks saved) {
    if (guarded()) {
        return false;
    }
    revoke();
    marks_.generation = std::max(marks_.generation, saved.generation);
    marks_.serial = std::max(marks_.serial, saved.serial);
    return true;
}

unsigned LinuxService::dispatch(void *context, const std::uint8_t *req, std::uint8_t *reply) {
    auto &service = *static_cast<LinuxService *>(context);
    if (!hostdrv_request_valid(req)) {
        return HD_BAD_PACKET;
    }
    hostdrv_response_init(reply, req, 0);
    try {
        return service.execute(req, reply);
    } catch (const std::bad_alloc &) {
        hostdrv_response_init(reply, req, 31);
        return HD_COMPLETED;
    }
}

unsigned LinuxService::execute(const std::uint8_t *req, std::uint8_t *reply) {
    const auto generation = hostdrv_get32(req + HD_GENERATION);
    unsigned status = 21;
    switch (req[HD_OPCODE]) {
    case HD_QUERY:
        if (names_) {
            hostdrv_put32(reply + HD_GENERATION, marks_.generation);
            hostdrv_put16(reply + HD_CAPS, 1);
            status = 0;
        }
        break;
    case HD_ATTACH:
        if (guarded()) {
            return HD_BUSY;
        }
        if (names_ && generation == marks_.generation) {
            installation_ = generation;
            attached_ = true;
            status = 0;
        }
        break;
    case HD_UNREGISTER:
        if (!attached_ && guarded() && generation == installation_) {
            installation_ = 0;
            status = 0;
        }
        break;
    case HD_DETACH:
        if (attached_ && generation == installation_) {
            if (!files_.empty() || !searches_.empty()) {
                return HD_BUSY;
            }
            revoke();
            status = 0;
        }
        break;
    default:
        if (attached_ && generation == installation_) {
            status = operate(req, reply);
        }
        break;
    }
    if (status) {
        hostdrv_response_init(reply, req, status);
    }
    return HD_COMPLETED;
}

void LinuxService::find_result(const Result &result, std::uint8_t *reply) {
    std::memcpy(reply + HD_SIZE, result.data(), 10);
    std::memcpy(reply + HD_HEADER, result.data() + 10, 11);
    hostdrv_put16(reply + HD_OUTPUT, 11);
}

unsigned LinuxService::operate(const std::uint8_t *req, std::uint8_t *reply) {
    const auto token = hostdrv_get32(req + HD_TOKEN);
    const auto offset = hostdrv_get32(req + HD_OFFSET);
    const auto options = hostdrv_get16(req + HD_OPTIONS);
    const auto *payload = reinterpret_cast<const char *>(req + HD_HEADER);
    const std::string_view path(payload, hostdrv_get16(req + HD_PATH));
    LinuxNamespace::Item item;

    switch (req[HD_OPCODE]) {
    case HD_STAT: {
        const auto status = names_->stat(path, item);
        if (status) {
            return status;
        }
        return dos_metadata(reply + HD_SIZE, item.info, item.hidden) ? 0 : 31;
    }
    case HD_SPACE: {
        std::uint64_t bytes;
        if (!names_->capacity(bytes)) {
            return 31;
        }
        hostdrv_put32(reply + HD_HEADER, static_cast<std::uint32_t>(bytes));
        hostdrv_put32(reply + HD_HEADER + 4, static_cast<std::uint32_t>(bytes >> 32));
        hostdrv_put16(reply + HD_OUTPUT, 16);
        return 0;
    }
    case HD_OPEN: {
        if (options & 7) {
            return 5;
        }
        if (files_.size() >= max_files || marks_.serial == serial_max || next_token_ == token_max) {
            return 4;
        }
        std::optional<LinuxRoot::Handle> handle;
        const auto status = names_->open(path, handle, item);
        if (status) {
            return status;
        }
        // Compatibility read and deny-write permit other readers. Deny-all
        // and deny-read conflict in either direction, including via aliases.
        const auto share = options >> 4;
        const bool denies_read = share == 1 || share == 3;
        for (const auto &active : files_) {
            if (!denies_read && !active.second.denies_read) {
                continue;
            }
            struct stat st {};
            if (!active.second.handle.metadata(st)) {
                return 31;
            }
            if (st.st_dev == item.info.st_dev && st.st_ino == item.info.st_ino) {
                return 32;
            }
        }
        if (!dos_metadata(reply + HD_SIZE, item.info, item.hidden)) {
            return 31;
        }
        files_.emplace(next_token_ + 1, File{std::move(*handle), item.hidden, denies_read});
        hostdrv_put32(reply + HD_TOKEN, ++next_token_);
        hostdrv_put16(reply + HD_SERIAL, ++marks_.serial);
        return 0;
    }
    case HD_CLOSE:
    case HD_READ:
    case HD_FSTAT: {
        const auto file = files_.find(token);
        if (file == files_.end()) {
            return 6;
        }
        if (req[HD_OPCODE] == HD_CLOSE) {
            const bool closed = file->second.handle.close_checked();
            files_.erase(file);
            return closed ? 0 : 31;
        }
        if (req[HD_OPCODE] == HD_FSTAT) {
            struct stat st {};
            return file->second.handle.metadata(st) && dos_metadata(reply + HD_SIZE, st, file->second.hidden)
                       ? 0 : 31;
        }
        std::uint16_t actual;
        if (!LinuxRoot::read(file->second.handle, offset, reply + HD_HEADER,
                             hostdrv_get16(req + HD_COUNT), actual)) {
            return 31;
        }
        hostdrv_put16(reply + HD_ACTUAL, actual);
        hostdrv_put16(reply + HD_OUTPUT, actual);
        return 0;
    }
    case HD_FIND_FIRST: {
        const std::string_view pattern(payload + path.size(), 11);
        if (!valid_template(pattern)) {
            return 31;
        }
        std::vector<LinuxNamespace::Item> items;
        const auto status = names_->list(path, items);
        if (status) {
            return status;
        }
        Snapshot found;
        for (const auto &entry : items) {
            const auto attr = attributes(entry.info, entry.hidden);
            if ((options & 0x1e) == 8 || (attr & 0x16 & ~options) || !matches(entry.name, pattern)) {
                continue;
            }
            Result result;
            if (!dos_metadata(result.data(), entry.info, entry.hidden)) {
                return 31;
            }
            std::memcpy(result.data() + 10, entry.name.data(), 11);
            found.push_back(result);
        }
        if (found.empty()) {
            return 18;
        }
        if (next_token_ == token_max) {
            return 31;
        }
        // Commit first (the only allocating operation below), then evict.
        const auto added = searches_.emplace(next_token_ + 1, std::move(found)).first;
        if (searches_.size() > max_searches) {
            searches_.erase(searches_.begin());
        }
        hostdrv_put32(reply + HD_TOKEN, ++next_token_);
        find_result(added->second.front(), reply);
        return 0;
    }
    case HD_FIND_NEXT:
    case HD_FIND_RELEASE: {
        const auto search = searches_.find(token);
        if (search == searches_.end()) {
            return 18;
        }
        if (req[HD_OPCODE] == HD_FIND_RELEASE) {
            searches_.erase(search);
        } else {
            if (offset >= search->second.size()) {
                return 18;
            }
            find_result(search->second[offset], reply);
        }
        return 0;
    }
    default:
        return 1;
    }
}

} // namespace hostdrv
