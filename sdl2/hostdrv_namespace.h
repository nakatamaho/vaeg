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
#ifndef VAEG_HOSTDRV_NAMESPACE_H
#define VAEG_HOSTDRV_NAMESPACE_H

#ifdef _WIN32
#include "hostdrv_windows.h"
#else
#include "hostdrv_linux.h"
#endif
#include "hostdrv_names.h"

#include <list>
#include <map>

namespace hostdrv {

#ifdef _WIN32
using PlatformRoot = WindowsRoot;
#else
using PlatformRoot = LinuxRoot;
#endif

// A session-local DOS namespace over the selected root. No method accepts
// a guest-supplied raw host pathname. Returned Handles are anchored pins.
class Namespace {
public:
    explicit Namespace(PlatformRoot root);

    enum Error : unsigned { ok = 0, missing = 2, bad_path = 3, denied = 5, io = 31 };
    struct Item {
        DosName name;
        HostMetadata info;
        bool hidden;
    };
    Error stat(std::string_view path, Item &item);
    Error list(std::string_view path, std::vector<Item> &items);
    Error open(std::string_view path, std::optional<PlatformRoot::Handle> &file, Item &item);
    bool capacity(std::uint64_t &total) const { return root_.capacity(total); }

private:
    struct Entry {
        std::string raw;
        std::optional<std::string> folded;
        PlatformRoot::Handle pin;
        HostMetadata info;
    };
    struct Directory {
        PlatformRoot::Handle pin;
        // nullopt is a permanently reserved spelling in this directory.
        std::map<DosName, std::optional<Entry>> entries;
    };
    struct Location {
        Directory *parent = nullptr;
        std::string raw;
        DosName name;
        const PlatformRoot::Handle *expected = nullptr;
        std::optional<PlatformRoot::Handle> pin;
    };

    Error locate(std::string_view path, Location &location);
    Directory *directory(const PlatformRoot::Handle &pin, std::string_view raw);
    Error refresh(Directory &directory, std::string_view raw);
    Error describe(const Location &location, Item &item) const;
    PlatformRoot root_;
    std::list<Directory> directories_;
    std::size_t issued_ = 0;
};

} // namespace hostdrv
#endif
