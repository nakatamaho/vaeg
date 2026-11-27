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
#include "hostdrv_namespace.h"

#include <algorithm>
#include <cerrno>
#include <set>
#include <utility>

namespace hostdrv {
namespace {

constexpr std::size_t max_issued = 8192;
constexpr std::size_t max_directories = 4096;

std::string append(std::string_view directory, std::string_view basename) {
    std::string path(directory);
    if (!path.empty()) {
        path += '/';
    }
    path.append(basename);
    return path;
}

} // namespace

Namespace::Namespace(PlatformRoot root) : root_(std::move(root)) {}

Namespace::Directory *Namespace::directory(const PlatformRoot::Handle &pin, std::string_view raw) {
    for (auto &dir : directories_) {
        if (pin.same_object(dir.pin)) {
            return &dir;
        }
    }
    if (directories_.size() >= max_directories) {
        return nullptr;
    }
    auto current = root_.resolve(raw);
    if (!current || !current->same_object(pin)) {
        return nullptr;
    }
    directories_.push_back({std::move(*current), {}});
    return &directories_.back();
}

Namespace::Error Namespace::refresh(Directory &dir, std::string_view raw) {
    struct Candidate {
        HostName name;
        Entry entry;
        DosName issued;
    };
    HostMetadata before {};
    std::vector<PlatformRoot::Entry> scan;
    if (!dir.pin.metadata(before) || !root_.enumerate(raw, dir.pin, scan)) {
        return io;
    }
    std::sort(scan.begin(), scan.end(), [](const auto &a, const auto &b) { return a.raw_name < b.raw_name; });
    std::vector<Candidate> staged;
    std::map<std::uint64_t, std::vector<std::size_t>> by_inode;
    for (std::size_t i = 0; i < scan.size(); ++i) {
        auto &entry = scan[i];
        if (i && entry.raw_name == scan[i - 1].raw_name) {
            return io;
        }
        auto name = host_name(entry.raw_name);
        if (!name) {
            continue;
        }
        int reason;
        auto pin = root_.resolve_entry(append(raw, entry.raw_name), dir.pin, &reason);
        if (!pin) {
            if (reason == EPERM || reason == EXDEV || reason == ELOOP) {
                continue;
            }
            return io;
        }
        HostMetadata info {};
        if (!pin->metadata(info) || info.id != entry.inode) {
            return io;
        }
        // The file can grow between authorization and the metadata sample.
        if (!info.directory && info.size > 0x7fffffffu) {
            continue;
        }
        by_inode[info.id].push_back(staged.size());
        staged.push_back({std::move(*name), {entry.raw_name, {}, std::move(*pin), info}, {}});
    }
    const auto current = root_.resolve(raw);
    HostMetadata after {};
    // Detect visible mutations during enumeration. This is not an atomic
    // snapshot or an authorization check; each child was resolved separately.
    if (!current || !current->same_object(dir.pin) || !current->metadata(after) ||
        before.modified != after.modified || before.changed != after.changed) {
        return io;
    }

    // Exact raw identity first, otherwise a unique case-only association.
    // Directory entries with ambiguous inode associations get no rename carry-over.
    std::map<std::uint64_t, unsigned> prior_count;
    for (const auto &item : dir.entries) {
        if (item.second) {
            ++prior_count[item.second->info.id];
        }
    }
    for (const auto &item : dir.entries) {
        const auto &old = item.second;
        if (!old) {
            continue;
        }
        const auto found = by_inode.find(old->info.id);
        if (found == by_inode.end()) {
            continue;
        }
        for (auto index : found->second) {
            auto &next = staged[index];
            if (next.issued.empty() &&
                (next.entry.raw == old->raw ||
                 (found->second.size() == 1 && prior_count[found->first] == 1 && old->folded &&
                  next.name.folded == old->folded)) && next.entry.pin.same_object(old->pin)) {
                next.issued = item.first;
                break;
            }
        }
    }

    std::set<DosName> added;
    // Reserve all native names in raw byte order before allocating aliases.
    for (auto &next : staged) {
        if (next.issued.empty() && next.name.native && !dir.entries.count(*next.name.native) &&
            added.insert(*next.name.native).second) {
            next.issued = *next.name.native;
        }
    }
    std::map<std::pair<std::string, std::string>, unsigned> next_number;
    for (auto &next : staged) {
        if (!next.issued.empty()) {
            continue;
        }
        auto &number = next_number[{next.name.prefix, next.name.extension}];
        do {
            const auto alias = alias_name(next.name, ++number);
            if (!alias) {
                return io;
            }
            if (!dir.entries.count(*alias) && added.insert(*alias).second) {
                next.issued = *alias;
            }
        } while (next.issued.empty());
    }
    if (added.size() > max_issued - issued_) {
        return io;
    }

    // Allocate the replacement map before mutating the existing ledger.
    // Keep tombstones even when an old native name is available again.
    std::map<DosName, std::optional<Entry>> replacement;
    for (auto &next : staged) {
        next.entry.folded = std::move(next.name.folded);
        replacement.emplace(std::move(next.issued), std::move(next.entry));
    }
    for (const auto &old : dir.entries) {
        replacement.try_emplace(old.first, std::nullopt);
    }
    dir.entries.swap(replacement);
    issued_ += added.size();
    return ok;
}

Namespace::Error Namespace::locate(std::string_view path, Location &result) {
    const auto parts = dos_path(path);
    if (!parts) {
        return bad_path;
    }
    Location loc;
    loc.pin = root_.resolve("");
    if (!loc.pin) {
        return io;
    }
    for (std::size_t i = 0; i < parts->size(); ++i) {
        const auto &name = (*parts)[i];
        HostMetadata info {};
        if (!loc.pin->metadata(info)) {
            return io;
        }
        if (!info.directory) {
            return bad_path;
        }
        auto *dir = directory(*loc.pin, loc.raw);
        if (!dir) {
            return io;
        }
        const auto status = refresh(*dir, loc.raw);
        if (status != ok) {
            return status;
        }
        const auto found = dir->entries.find(name);
        const auto absent = i + 1 == parts->size() ? missing : bad_path;
        if (found == dir->entries.end() || !found->second) {
            return absent;
        }
        const auto &entry = *found->second;
        loc.raw = append(loc.raw, entry.raw);
        loc.pin = root_.resolve_entry(loc.raw, dir->pin);
        if (!loc.pin || !loc.pin->same_object(entry.pin)) {
            return absent;
        }
        loc.parent = dir;
        loc.expected = &entry.pin;
        loc.name = name;
    }
    result = std::move(loc);
    return ok;
}

Namespace::Error Namespace::describe(const Location &location, Item &item) const {
    // Earlier namespace scans never authorize a later pathname access.
    const auto current = location.parent ? root_.resolve_entry(location.raw, location.parent->pin) : root_.resolve("");
    HostMetadata info {};
    if (!current || !current->same_object(*location.pin) || !current->metadata(info)) {
        return io;
    }
    const auto sep = location.raw.rfind('/');
    const auto basename = std::string_view(location.raw).substr(sep == std::string::npos ? 0 : sep + 1);
    item = {location.name, info, PlatformRoot::hidden(info, basename)};
    return ok;
}

Namespace::Error Namespace::stat(std::string_view path, Item &item) {
    Location loc;
    const auto status = locate(path, loc);
    return status == ok ? describe(loc, item) : status;
}

Namespace::Error Namespace::list(std::string_view path, std::vector<Item> &items) {
    Location loc;
    const auto status = locate(path, loc);
    if (status != ok) {
        return status == missing ? bad_path : status;
    }
    HostMetadata info {};
    if (!loc.pin->metadata(info)) {
        return io;
    }
    if (!info.directory) {
        return bad_path;
    }
    auto *dir = directory(*loc.pin, loc.raw);
    if (!dir) {
        return io;
    }
    const auto scanned = refresh(*dir, loc.raw);
    if (scanned != ok) {
        return scanned;
    }
    std::vector<Item> result;
    for (const auto &item : dir->entries) {
        if (item.second) {
            result.push_back({item.first, item.second->info, PlatformRoot::hidden(item.second->info, item.second->raw)});
        }
    }
    items.swap(result);
    return ok;
}

Namespace::Error Namespace::open(std::string_view path, std::optional<PlatformRoot::Handle> &file, Item &item) {
    Location loc;
    const auto status = locate(path, loc);
    if (status != ok) {
        return status;
    }
    if (!loc.parent) {
        return denied;
    }
    auto opened = root_.open_file(loc.raw, loc.parent->pin, *loc.expected);
    HostMetadata info {};
    if (!opened || !opened->metadata(info)) {
        return denied;
    }
    const auto sep = loc.raw.rfind('/');
    const auto basename = std::string_view(loc.raw).substr(sep == std::string::npos ? 0 : sep + 1);
    item = {std::move(loc.name), info, PlatformRoot::hidden(info, basename)};
    file = std::move(opened);
    return ok;
}

} // namespace hostdrv
