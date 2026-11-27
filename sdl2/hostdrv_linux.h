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
#ifndef VAEG_HOSTDRV_LINUX_H
#define VAEG_HOSTDRV_LINUX_H

#include "hostdrv_metadata.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

namespace hostdrv {

// Internal platform operations. Paths here use raw directory-entry bytes,
// never guest path bytes. The DOS namespace must mediate every guest lookup.
class LinuxRoot {
public:
    class Handle {
        friend class LinuxRoot;
        explicit Handle(int fd) : fd_(fd) {}
        int fd_;

    public:
        ~Handle();
        Handle(Handle &&other) noexcept;
        Handle &operator=(Handle &&other) noexcept;
        Handle(const Handle &) = delete;
        Handle &operator=(const Handle &) = delete;
        bool close_checked() noexcept;
        bool metadata(struct stat &st) const;
        bool metadata(HostMetadata &info) const;
        bool same_object(const Handle &other) const;
    };

    static std::optional<LinuxRoot> select(const std::string &path);
    // Empty path is the selected root. Every other path is resolved in one
    // openat2 call from that root, not from any previously discovered child.
    std::optional<Handle> resolve(std::string_view raw_path, int *error = nullptr) const;
    // Resolve an entry while revalidating its containing directory. The
    // optional errno output separates ineligible/absent entries from failures
    // that must abort namespace reconciliation (e.g. descriptor exhaustion).
    std::optional<Handle> resolve_entry(std::string_view raw_path, const Handle &parent,
                                        int *error = nullptr) const;
    // Authorize by fresh raw-path resolution and match both namespace pins.
    std::optional<Handle> open_file(std::string_view raw_path, const Handle &parent,
                                    const Handle &expected) const;
    static bool read(const Handle &file, std::uint32_t offset, void *buffer,
                     std::uint16_t count, std::uint16_t &actual);
    bool capacity(std::uint64_t &total) const;
    static bool hidden(const HostMetadata &, std::string_view basename) {
        return !basename.empty() && basename.front() == '.';
    }

    struct Entry {
        std::string raw_name;
        ino_t inode; // enumeration hint only, not authorization
    };
    // Stages raw names only. Each publishable entry still requires a fresh
    // root-relative authorization and namespace identity verification.
    bool enumerate(std::string_view raw_path, const Handle &expected,
                   std::vector<Entry> &entries) const;

private:
    LinuxRoot(Handle root, Handle proc_fds, dev_t device);
    std::optional<Handle> upgrade(const Handle &metadata, bool directory) const;
    Handle root_;
    Handle proc_fds_;
    dev_t device_;
};

} // namespace hostdrv
#endif
