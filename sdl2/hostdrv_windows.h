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
#ifndef VAEG_HOSTDRV_WINDOWS_H
#define VAEG_HOSTDRV_WINDOWS_H

#include "hostdrv_metadata.h"

#include <windows.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hostdrv {

class WindowsRoot {
public:
    class Handle {
        friend class WindowsRoot;
        explicit Handle(HANDLE handle) : handle_(handle) {}
        HANDLE handle_;

    public:
        ~Handle();
        Handle(Handle &&other) noexcept;
        Handle &operator=(Handle &&other) noexcept;
        Handle(const Handle &) = delete;
        Handle &operator=(const Handle &) = delete;
        bool close_checked() noexcept;
        bool metadata(HostMetadata &info) const;
        bool same_object(const Handle &other) const;
    };

    static std::optional<WindowsRoot> select(const std::string &path);
    std::optional<Handle> resolve(std::string_view raw, int *error = nullptr) const;
    std::optional<Handle> resolve_entry(std::string_view raw, const Handle &parent, int *error = nullptr) const;
    std::optional<Handle> open_file(std::string_view raw, const Handle &parent, const Handle &expected) const;
    static bool read(const Handle &file, std::uint32_t offset, void *buffer,
                     std::uint16_t count, std::uint16_t &actual);
    bool capacity(std::uint64_t &total) const;
    static bool hidden(const HostMetadata &info, std::string_view) { return (info.attributes & 2) != 0; }

    struct Entry {
        std::string raw_name;
        std::uint64_t inode;
    };
    bool enumerate(std::string_view raw, const Handle &expected, std::vector<Entry> &entries) const;

private:
    friend struct WindowsRootTest;
    struct Lookup {
        std::vector<Handle> ancestors;
        Handle leaf;
    };
    WindowsRoot(Handle root, std::wstring volume_path, std::uint64_t volume);
    std::optional<Lookup> lookup(std::wstring_view raw, const Handle *parent, bool lock_leaf, int *error) const;
    std::optional<Handle> upgrade(const Handle &metadata, bool directory) const;
    Handle root_;
    std::wstring volume_path_;
    std::uint64_t volume_;
};
}
#endif
