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
#include "hostdrv_linux.h"

#include <algorithm>
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <linux/magic.h>
#include <memory>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <utility>

#if __has_include(<linux/openat2.h>)
#include <linux/openat2.h>
#endif

namespace hostdrv {
namespace {

int anchored_open(int base, const char *path, unsigned flags, bool local, bool no_links) {
#if defined(SYS_openat2) && defined(RESOLVE_NO_XDEV)
    struct open_how how {};
    how.flags = flags;
    how.resolve = RESOLVE_NO_MAGICLINKS;
    if (local) {
        how.resolve |= RESOLVE_BENEATH | RESOLVE_NO_XDEV;
    }
    if (no_links) {
        how.resolve |= RESOLVE_NO_SYMLINKS;
    }
    return static_cast<int>(syscall(SYS_openat2, base, path, &how, sizeof(how)));
#else
    (void)base;
    (void)path;
    (void)flags;
    (void)local;
    (void)no_links;
    return -1;
#endif
}

bool valid_relative(std::string_view path) {
    if (path.find('\0') != path.npos || (!path.empty() && path.back() == '/')) {
        return false;
    }
    while (!path.empty()) {
        const auto sep = path.find('/');
        const auto name = path.substr(0, sep);
        if (name.empty() || name == "." || name == "..") {
            return false;
        }
        if (sep == path.npos) {
            break;
        }
        path.remove_prefix(sep + 1);
    }
    return true;
}

bool supported_filesystem(int fd) {
    struct statfs fs {};
    if (fstatfs(fd, &fs) != 0) {
        return false;
    }
    // Positive allowlist: no remote, stacked, userspace or unknown providers.
    return fs.f_type == EXT4_SUPER_MAGIC || fs.f_type == XFS_SUPER_MAGIC || fs.f_type == TMPFS_MAGIC;
}

bool same_object(const struct stat &a, const struct stat &b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino && (a.st_mode & S_IFMT) == (b.st_mode & S_IFMT);
}

} // namespace

LinuxRoot::Handle::~Handle() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

LinuxRoot::Handle::Handle(Handle &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

LinuxRoot::Handle &LinuxRoot::Handle::operator=(Handle &&other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            close(fd_);
        }
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

bool LinuxRoot::Handle::close_checked() noexcept {
    // Linux releases the descriptor even on close errors; do not retry.
    return close(std::exchange(fd_, -1)) == 0;
}

bool LinuxRoot::Handle::metadata(struct stat &st) const {
    return fstat(fd_, &st) == 0;
}

bool LinuxRoot::Handle::same_object(const Handle &other) const {
    struct stat a {}, b {};
    return metadata(a) && other.metadata(b) && hostdrv::same_object(a, b);
}

LinuxRoot::LinuxRoot(Handle root, Handle proc_fds, dev_t device)
    : root_(std::move(root)), proc_fds_(std::move(proc_fds)), device_(device) {}

std::optional<LinuxRoot> LinuxRoot::select(const std::string &path) {
    if (path.empty() || path.find('\0') != path.npos) {
        return std::nullopt;
    }
    constexpr unsigned dir_flags = O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
    Handle root(anchored_open(AT_FDCWD, path.c_str(), dir_flags, false, true));
    struct stat identity {};
    if (!root.metadata(identity) || !supported_filesystem(root.fd_)) {
        return std::nullopt;
    }

    // Anchor in procfs before following its kernel-generated 'self' symlink.
    // NO_XDEV prevents substituted mounts on self/fd or its ancestors.
    Handle proc(anchored_open(AT_FDCWD, "/proc", dir_flags, false, true));
    struct statfs fs {};
    if (fstatfs(proc.fd_, &fs) != 0 || fs.f_type != PROC_SUPER_MAGIC) {
        return std::nullopt;
    }
    Handle refs(anchored_open(proc.fd_, "self/fd", dir_flags, true, false));
    LinuxRoot result(std::move(root), std::move(refs), identity.st_dev);
    const auto check = result.resolve("");
    if (!check || !result.upgrade(*check, true)) {
        return std::nullopt;
    }
    return result;
}

std::optional<LinuxRoot::Handle> LinuxRoot::resolve(std::string_view raw_path, int *error) const {
    const auto reject = [error](int reason) -> std::optional<Handle> {
        if (error) {
            *error = reason;
        }
        return std::nullopt;
    };
    if (!valid_relative(raw_path)) {
        return reject(EINVAL);
    }
    const std::string path(raw_path.empty() ? "." : raw_path);
    Handle pin(anchored_open(root_.fd_, path.c_str(), O_PATH | O_NOFOLLOW | O_CLOEXEC, true, true));
    if (pin.fd_ < 0) {
        return reject(errno);
    }
    struct stat info {};
    if (!pin.metadata(info)) {
        return reject(errno);
    }
    if (info.st_dev != device_ ||
        !(S_ISDIR(info.st_mode) || (S_ISREG(info.st_mode) && info.st_nlink == 1 &&
                                   info.st_size >= 0 && info.st_size <= 0x7fffffff))) {
        return reject(EPERM);
    }
    if (error) {
        *error = 0;
    }
    return pin;
}

std::optional<LinuxRoot::Handle> LinuxRoot::upgrade(const Handle &pin, bool directory) const {
    const auto eligible = [directory](const struct stat &st) {
        return directory ? S_ISDIR(st.st_mode) : S_ISREG(st.st_mode) && st.st_nlink == 1 &&
                                                     st.st_size >= 0 && st.st_size <= 0x7fffffff;
    };
    struct stat before {}, after {};
    if (!pin.metadata(before) || before.st_dev != device_ || !eligible(before)) {
        return std::nullopt;
    }
    const auto number = std::to_string(pin.fd_);
    // The procfs fd magic link refers to the still-open object, not its
    // former path. O_NOATIME is mandatory, including for directory reads.
    Handle data(openat(proc_fds_.fd_, number.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOATIME |
                                                        (directory ? O_DIRECTORY : 0)));
    if (!data.metadata(after) || !same_object(before, after) || !eligible(after)) {
        return std::nullopt;
    }
    return data;
}

std::optional<LinuxRoot::Handle> LinuxRoot::resolve_entry(std::string_view raw_path, const Handle &parent,
                                                         int *error) const {
    if (raw_path.empty() || !valid_relative(raw_path)) {
        if (error) {
            *error = EINVAL;
        }
        return std::nullopt;
    }
    const auto separator = raw_path.rfind('/');
    const auto parent_path = separator == raw_path.npos ? std::string_view{} : raw_path.substr(0, separator);
    const auto parent_matches = [&] {
        const auto current = resolve(parent_path);
        struct stat info {};
        return current && current->metadata(info) && S_ISDIR(info.st_mode) && current->same_object(parent);
    };
    if (!parent_matches()) {
        if (error) {
            *error = EAGAIN;
        }
        return std::nullopt;
    }
    auto pin = resolve(raw_path, error);
    if (!parent_matches()) {
        if (error) {
            *error = EAGAIN;
        }
        return std::nullopt;
    }
    return pin;
}

std::optional<LinuxRoot::Handle> LinuxRoot::open_file(std::string_view raw_path, const Handle &parent,
                                                     const Handle &expected) const {
    const auto pin = resolve_entry(raw_path, parent);
    if (!pin || !pin->same_object(expected)) {
        return std::nullopt;
    }
    return upgrade(*pin, false);
}

bool LinuxRoot::read(const Handle &file, std::uint32_t offset, void *buffer, std::uint16_t count,
                     std::uint16_t &actual) {
    actual = 0;
    if (offset > 0x7fffffff || count == 0 || count > 4096) {
        return false;
    }
    const auto limit = std::min<std::uint32_t>(count, 0x7fffffffu - offset);
    std::uint32_t done = 0;
    // A file that grew past the ABI cap is not a valid file at this point.
    struct stat before {};
    if (!file.metadata(before) || !S_ISREG(before.st_mode) || before.st_size > 0x7fffffff) {
        return false;
    }
    while (done < limit) {
        const auto n = pread(file.fd_, static_cast<char *>(buffer) + done, limit - done, offset + done);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (done == 0) {
                return false;
            }
            // Report bytes already read. The next operation starts at the
            // advanced offset and can report any persistent failure there.
            break;
        }
        if (n == 0) {
            break;
        }
        done += static_cast<std::uint32_t>(n);
    }
    actual = static_cast<std::uint16_t>(done);
    return true;
}

bool LinuxRoot::capacity(std::uint64_t &total) const {
    struct statvfs info {};
    if (fstatvfs(root_.fd_, &info) != 0 || info.f_frsize == 0) {
        return false;
    }
    const auto max = std::numeric_limits<std::uint64_t>::max();
    total = info.f_blocks > max / info.f_frsize ? max : info.f_blocks * std::uint64_t(info.f_frsize);
    return true;
}

bool LinuxRoot::enumerate(std::string_view raw_path, const Handle &expected, std::vector<Entry> &entries) const {
    const auto pin = resolve(raw_path);
    if (!pin || !pin->same_object(expected)) {
        return false;
    }
    auto data = upgrade(*pin, true);
    if (!data) {
        return false;
    }
    DIR *stream = fdopendir(data->fd_);
    if (!stream) {
        return false;
    }
    // fdopendir owns the descriptor on success.
    data->fd_ = -1;
    const auto close_stream = [](DIR *dir) { closedir(dir); };
    const std::unique_ptr<DIR, decltype(close_stream)> owner(stream, close_stream);
    std::vector<Entry> staged;
    while (true) {
        errno = 0;
        const auto *entry = readdir(stream);
        if (!entry) {
            if (errno != 0) {
                return false;
            }
            break;
        }
        const std::string_view name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }
        if (staged.size() == 4096) {
            return false;
        }
        staged.push_back({std::string(name), entry->d_ino});
    }
    const auto current = resolve(raw_path);
    if (!current || !current->same_object(expected)) {
        return false;
    }
    entries = std::move(staged);
    return true;
}

} // namespace hostdrv
