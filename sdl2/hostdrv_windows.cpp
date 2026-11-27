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
#include "hostdrv_windows.h"

#include <winternl.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <utility>

namespace hostdrv {
namespace {

constexpr DWORD share_all = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
constexpr ULONG dont_reparse = 0x1000; // OBJ_DONT_REPARSE
constexpr DWORD reopen_flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;
constexpr std::size_t max_scan = 4096;
const HANDLE invalid = INVALID_HANDLE_VALUE;

using NtOpen = NTSTATUS (NTAPI *)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
    PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);

NtOpen native_open() {
    static const auto fn = reinterpret_cast<NtOpen>(reinterpret_cast<void (*)()>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtCreateFile")));
    return fn;
}

std::wstring wide(std::string_view text) {
    if (text.empty() || text.size() > 32767) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    if (length && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()),
                                      result.data(), length) != length) {
        return {};
    }
    return result;
}

std::string utf8(std::wstring_view text) {
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), int(text.size()),
                                          nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    if (length && WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), int(text.size()),
                                     result.data(), length, nullptr, nullptr) != length) {
        return {};
    }
    return result;
}

bool component(std::wstring_view name) {
    if (name.empty() || name.size() > 255 || name.back() == L'.' || name.back() == L' ') {
        return false;
    }
    return std::none_of(name.begin(), name.end(), [](wchar_t ch) {
        return ch < 32 || std::wstring_view(L"/\\:*?\"<>|").find(ch) != std::wstring_view::npos;
    });
}

void trace(WindowsRoot::Diagnostic diagnostic, const char *stage, std::wstring_view name, DWORD code = 0) {
    if (diagnostic) diagnostic(stage, name, code);
}

// Verify only the leaf: the parent is a held object. Explicit mount paths
// can use Windows case folding, but never a short-name alias. Guest lookups
// still require the exact spelling returned by filesystem enumeration.
bool match_name(HANDLE handle, std::wstring_view name, bool selecting, WindowsRoot::Diagnostic diagnostic) {
    alignas(FILE_NAME_INFO) std::array<unsigned char, 65536 + sizeof(FILE_NAME_INFO)> buffer;
    if (!GetFileInformationByHandleEx(handle, FileNameInfo, buffer.data(), DWORD(buffer.size()))) {
        trace(diagnostic, "GetFileInformationByHandleEx(FileNameInfo) Win32", name, GetLastError());
        return false;
    }
    const auto *info = reinterpret_cast<const FILE_NAME_INFO *>(buffer.data());
    if (info->FileNameLength % sizeof(wchar_t) ||
        info->FileNameLength > buffer.size() - offsetof(FILE_NAME_INFO, FileName)) {
        trace(diagnostic, "FileNameInfo invalid length", name, info->FileNameLength);
        return false;
    }
    const std::wstring_view full(info->FileName, info->FileNameLength / sizeof(wchar_t));
    const auto actual = full.substr(full.rfind(L'\\') + 1);
    if (actual == name) return true;
    if (selecting) {
        const auto order = CompareStringOrdinal(name.data(), int(name.size()), actual.data(), int(actual.size()), TRUE);
        if (order == CSTR_EQUAL) return true;
        if (!order) trace(diagnostic, "CompareStringOrdinal Win32", name, GetLastError());
    }
    trace(diagnostic, "FileNameInfo requested spelling", name);
    trace(diagnostic, "FileNameInfo actual spelling", actual);
    return false;
}

bool information(HANDLE handle, BY_HANDLE_FILE_INFORMATION &id, FILE_BASIC_INFO &basic,
                 WindowsRoot::Diagnostic diagnostic = nullptr, std::wstring_view name = {}) {
    const DWORD type = GetFileType(handle);
    if (type != FILE_TYPE_DISK) {
        const DWORD error = type == FILE_TYPE_UNKNOWN ? GetLastError() : 0;
        trace(diagnostic, "GetFileType type", name, type);
        if (error) trace(diagnostic, "GetFileType Win32", name, error);
        return false;
    }
    FILE_ATTRIBUTE_TAG_INFO tag{};
    if (!GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &tag, sizeof(tag))) {
        trace(diagnostic, "GetFileInformationByHandleEx(FileAttributeTagInfo) Win32", name, GetLastError());
        return false;
    }
    trace(diagnostic, "FileAttributeTagInfo attributes", name, tag.FileAttributes);
    trace(diagnostic, "FileAttributeTagInfo reparse tag", name, tag.ReparseTag);
    constexpr DWORD denied = FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE;
    if ((tag.FileAttributes & denied) || tag.ReparseTag) return false;
    if (!GetFileInformationByHandle(handle, &id)) {
        trace(diagnostic, "GetFileInformationByHandle Win32", name, GetLastError());
        return false;
    }
    if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &basic, sizeof(basic))) {
        trace(diagnostic, "GetFileInformationByHandleEx(FileBasicInfo) Win32", name, GetLastError());
        return false;
    }
    if (basic.FileAttributes & denied) {
        trace(diagnostic, "FileBasicInfo denied attributes", name, basic.FileAttributes);
        return false;
    }
    return true;
}

bool eligible(HANDLE handle, std::uint64_t volume, bool require_directory,
              WindowsRoot::Diagnostic diagnostic = nullptr, std::wstring_view name = {}) {
    BY_HANDLE_FILE_INFORMATION id{};
    FILE_BASIC_INFO basic{};
    if (!information(handle, id, basic, diagnostic, name)) return false;
    if (id.dwVolumeSerialNumber != volume) {
        trace(diagnostic, "unexpected volume serial", name, id.dwVolumeSerialNumber);
        return false;
    }
    if (basic.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) return true;
    if (require_directory) trace(diagnostic, "directory required", name, basic.FileAttributes);
    else if (id.nNumberOfLinks != 1) trace(diagnostic, "ineligible link count", name, id.nNumberOfLinks);
    return !require_directory && id.nNumberOfLinks == 1;
}

} // namespace

WindowsRoot::Handle::~Handle() { close_checked(); }
WindowsRoot::Handle::Handle(Handle &&other) noexcept : handle_(std::exchange(other.handle_, invalid)) {}
WindowsRoot::Handle &WindowsRoot::Handle::operator=(Handle &&other) noexcept {
    if (this != &other) {
        close_checked();
        handle_ = std::exchange(other.handle_, invalid);
    }
    return *this;
}
bool WindowsRoot::Handle::close_checked() noexcept {
    return handle_ == invalid || CloseHandle(std::exchange(handle_, invalid));
}

bool WindowsRoot::Handle::metadata(HostMetadata &out, Diagnostic diagnostic) const {
    BY_HANDLE_FILE_INFORMATION id{};
    FILE_BASIC_INFO basic{};
    if (!information(handle_, id, basic, diagnostic)) {
        return false;
    }
    const auto time = basic.LastWriteTime.QuadPart / 10000000 - 11644473600LL;
    out = {id.dwVolumeSerialNumber, (std::uint64_t(id.nFileIndexHigh) << 32) | id.nFileIndexLow,
           (std::uint64_t(id.nFileSizeHigh) << 32) | id.nFileSizeLow,
           std::uint64_t(basic.LastWriteTime.QuadPart), std::uint64_t(basic.ChangeTime.QuadPart),
           // The Windows CRT cannot represent pre-1970 dates in localtime_s.
           std::time_t(std::clamp(time, 0LL, 7500000000LL)),
           (basic.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0,
           basic.FileAttributes & (2u | 4u | 0x10u | 0x20u)};
    return out.id != 0;
}

bool WindowsRoot::Handle::same_object(const Handle &other, Diagnostic diagnostic) const {
    HostMetadata a{}, b{};
    return metadata(a, diagnostic) && other.metadata(b, diagnostic) &&
        a.id == b.id && a.volume == b.volume && a.directory == b.directory;
}

WindowsRoot::WindowsRoot(Handle root, std::wstring path, std::uint64_t volume)
    : root_(std::move(root)), volume_path_(std::move(path)), volume_(volume) {}

std::optional<WindowsRoot> WindowsRoot::select(const std::string &path, Diagnostic diagnostic) {
    if (!native_open()) {
        trace(diagnostic, "NtCreateFile unavailable", {});
        return {};
    }
    auto text = wide(path);
    if (text.empty() && !path.empty()) {
        trace(diagnostic, "invalid or oversized UTF-8 selection", {});
        return {};
    }
    std::replace(text.begin(), text.end(), L'\\', L'/');
    if (text.size() < 3 || text[1] != L':' || text[2] != L'/' ||
        !((text[0] >= L'A' && text[0] <= L'Z') || (text[0] >= L'a' && text[0] <= L'z'))) {
        trace(diagnostic, "absolute drive-letter path required", text);
        return {};
    }
    const std::wstring drive = text.substr(0, 2) + L"\\";
    const auto drive_type = GetDriveTypeW(drive.c_str());
    trace(diagnostic, "GetDriveTypeW type", drive, drive_type);
    if (drive_type != DRIVE_FIXED) return {};
    Handle anchor(CreateFileW((L"\\\\?\\" + drive).c_str(), FILE_READ_ATTRIBUTES, share_all,
                             nullptr, OPEN_EXISTING, reopen_flags, nullptr));
    if (anchor.handle_ == invalid) {
        trace(diagnostic, "CreateFileW(volume root) Win32", drive, GetLastError());
        return {};
    }
    wchar_t filesystem[16]{}, guid[64]{};
    DWORD serial = 0, flags = 0;
    if (!GetVolumeInformationByHandleW(anchor.handle_, nullptr, 0, &serial, nullptr, &flags,
                                       filesystem, DWORD(std::size(filesystem)))) {
        trace(diagnostic, "GetVolumeInformationByHandleW Win32", drive, GetLastError());
        return {};
    }
    trace(diagnostic, "GetVolumeInformationByHandleW filesystem and flags", filesystem, flags);
    trace(diagnostic, "GetVolumeInformationByHandleW serial", drive, serial);
    if (std::wstring_view(filesystem) != L"NTFS" || !eligible(anchor.handle_, serial, true, diagnostic, drive)) {
        return {};
    }
    // GUID volume naming is unavailable on remote providers. Require the
    // actual volume root, not SUBST or a redirected drive-letter subtree.
    const DWORD length = GetFinalPathNameByHandleW(anchor.handle_, guid, DWORD(std::size(guid)), VOLUME_NAME_GUID);
    if (!length) {
        trace(diagnostic, "GetFinalPathNameByHandleW(VOLUME_NAME_GUID) Win32", drive, GetLastError());
        return {};
    }
    if (length != 49 || std::wstring_view(guid, 11) != L"\\\\?\\Volume{" || guid[47] != L'}' || guid[48] != L'\\') {
        trace(diagnostic, "GetFinalPathNameByHandleW not a GUID volume root (length)",
              length < std::size(guid) ? std::wstring_view(guid, length) : std::wstring_view{}, length);
        return {};
    }
    WindowsRoot result(std::move(anchor), std::wstring(guid, length), serial);
    if (text.size() > 3 && text.back() == L'/') {
        text.pop_back();
    }
    if (text.size() > 3) {
        auto selected = result.lookup(std::wstring_view(text).substr(3), nullptr, false, nullptr, true, diagnostic);
        if (!selected) {
            return {};
        }
        result.root_ = std::move(selected->leaf);
    }
    // Reject a mount that cannot obtain a directory handle with suppression.
    const auto probe = result.upgrade(result.root_, true, diagnostic);
    if (!probe) {
        return {};
    }
    return result;
}

std::optional<WindowsRoot::Lookup> WindowsRoot::lookup(std::wstring_view raw, const Handle *parent,
                                                      bool lock_leaf, int *error, bool selecting,
                                                      Diagnostic diagnostic) const {
    int ignored;
    if (!error) {
        error = &ignored;
    }
    *error = EIO;
    Lookup result{{}, Handle(invalid)};
    if (raw.empty()) {
        HANDLE copy = nullptr;
        if (parent || !DuplicateHandle(GetCurrentProcess(), root_.handle_, GetCurrentProcess(), &copy,
                                       0, FALSE, DUPLICATE_SAME_ACCESS)) {
            return {};
        }
        result.leaf = Handle(copy);
        return result;
    }
    const Handle *base = &root_;
    for (;;) {
        if (result.ancestors.size() >= 256) {
            trace(diagnostic, "too many path components", {});
            return {};
        }
        const auto slash = raw.find(L'/');
        const bool leaf = slash == raw.npos;
        const auto name = raw.substr(0, slash);
        if (!component(name)) {
            trace(diagnostic, "invalid path component", name);
            *error = EPERM;
            return {};
        }
        if (leaf && parent && !base->same_object(*parent)) {
            return {};
        }
        UNICODE_STRING string{};
        string.Buffer = const_cast<PWSTR>(name.data());
        string.Length = string.MaximumLength = USHORT(name.size() * sizeof(wchar_t));
        OBJECT_ATTRIBUTES attrs{};
        InitializeObjectAttributes(&attrs, &string, dont_reparse | (selecting ? OBJ_CASE_INSENSITIVE : 0),
                                   base->handle_, nullptr);
        IO_STATUS_BLOCK io{};
        HANDLE handle = nullptr;
        // NTFS share checking ignores attribute-only opens. FILE_TRAVERSE
        // (FILE_EXECUTE) makes the directory handle participate in sharing
        // without granting FILE_LIST_DIRECTORY. Never request it on a leaf.
        const NTSTATUS status = native_open()(&handle, FILE_READ_ATTRIBUTES | SYNCHRONIZE |
                                               (leaf ? 0 : FILE_TRAVERSE),
            &attrs, &io, nullptr, 0, leaf && !lock_leaf ? share_all : share_all & ~FILE_SHARE_DELETE,
            FILE_OPEN, FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT |
                (leaf ? 0 : FILE_DIRECTORY_FILE), nullptr, 0);
        if (status < 0) {
            if (ULONG(status) == 0xc000050bu) { // STATUS_REPARSE_POINT_ENCOUNTERED
                *error = ELOOP;
                trace(diagnostic, "NtCreateFile reparse point encountered (NTSTATUS)", name, ULONG(status));
            } else {
                trace(diagnostic, "NtCreateFile NTSTATUS", name, ULONG(status));
            }
            return {};
        }
        Handle pin(handle);
        if (!eligible(handle, volume_, !leaf || selecting, diagnostic, name)) {
            // Distinguish ineligible objects from metadata inspection errors.
            FILE_ATTRIBUTE_TAG_INFO tag{};
            BY_HANDLE_FILE_INFORMATION info{};
            if (GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &tag, sizeof(tag)) &&
                ((tag.FileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE)) || tag.ReparseTag)) {
                *error = ELOOP;
            } else if (GetFileType(handle) == FILE_TYPE_DISK && GetFileInformationByHandle(handle, &info) &&
                       !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && info.nNumberOfLinks != 1) {
                *error = EPERM;
            }
            return {};
        }
        if (!match_name(handle, name, selecting, diagnostic)) {
            return {};
        }
        if (leaf) {
            result.leaf = std::move(pin);
            return result;
        }
        result.ancestors.push_back(std::move(pin));
        base = &result.ancestors.back();
        raw.remove_prefix(slash + 1);
    }
}

std::optional<WindowsRoot::Handle> WindowsRoot::resolve(std::string_view raw, int *error) const {
    const auto path = wide(raw);
    if (!raw.empty() && path.empty()) {
        if (error) *error = EPERM;
        return {};
    }
    auto result = lookup(path, nullptr, false, error);
    return result ? std::optional<Handle>(std::move(result->leaf)) : std::nullopt;
}

std::optional<WindowsRoot::Handle> WindowsRoot::resolve_entry(std::string_view raw, const Handle &parent,
                                                             int *error) const {
    const auto path = wide(raw);
    if (path.empty()) {
        if (error) *error = EPERM;
        return {};
    }
    auto result = lookup(path, &parent, false, error);
    return result ? std::optional<Handle>(std::move(result->leaf)) : std::nullopt;
}

std::optional<WindowsRoot::Handle> WindowsRoot::upgrade(const Handle &metadata, bool directory,
                                                       Diagnostic diagnostic) const {
    HostMetadata before{};
    if (!eligible(metadata.handle_, volume_, directory, diagnostic, L"before reopen")) return {};
    if (!metadata.metadata(before, diagnostic) || before.directory != directory) {
        trace(diagnostic, "metadata identity or type check before reopen failed", {});
        return {};
    }
    Handle data(ReOpenFile(metadata.handle_, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                           share_all, reopen_flags));
    // -1 changes per-handle NTFS update behavior, not an on-disk timestamp.
    // Only the reopened handle can perform data operations; fail on refusal.
    const FILETIME suppress{0xffffffffu, 0xffffffffu};
    if (data.handle_ == invalid) {
        trace(diagnostic, "ReOpenFile Win32", {}, GetLastError());
        return {};
    }
    if (!SetFileTime(data.handle_, nullptr, &suppress, nullptr)) {
        trace(diagnostic, "SetFileTime(suppress last access) Win32", {}, GetLastError());
        return {};
    }
    if (!eligible(data.handle_, volume_, directory, diagnostic, L"after reopen")) return {};
    if (!metadata.same_object(data, diagnostic)) {
        trace(diagnostic, "reopened object identity or type mismatch", {});
        return {};
    }
    return data;
}

std::optional<WindowsRoot::Handle> WindowsRoot::open_file(std::string_view raw, const Handle &parent,
                                                         const Handle &expected) const {
    const auto path = wide(raw);
    auto result = path.empty() ? std::nullopt : lookup(path, &parent, true, nullptr);
    if (!result || !result->leaf.same_object(expected)) {
        return {};
    }
    // Keep the entire walk's no-delete handles alive until data authorization.
    return upgrade(result->leaf, false);
}

bool WindowsRoot::read(const Handle &file, std::uint32_t offset, void *buffer,
                       std::uint16_t count, std::uint16_t &actual) {
    HostMetadata info{};
    if (offset > 0x7fffffffu || !count || count > 4096 || !file.metadata(info) ||
        info.directory || info.size > 0x7fffffffu) {
        return false;
    }
    LARGE_INTEGER position{};
    position.QuadPart = offset;
    DWORD done = 0;
    if (!SetFilePointerEx(file.handle_, position, nullptr, FILE_BEGIN) ||
        !ReadFile(file.handle_, buffer, std::min<DWORD>(count, 0x7fffffffu - offset), &done, nullptr)) {
        return false;
    }
    actual = std::uint16_t(done);
    return true;
}

bool WindowsRoot::capacity(std::uint64_t &total) const {
    ULARGE_INTEGER bytes{};
    if (!GetDiskFreeSpaceExW(volume_path_.c_str(), nullptr, &bytes, nullptr)) {
        return false;
    }
    total = bytes.QuadPart;
    return true;
}

bool WindowsRoot::enumerate(std::string_view raw, const Handle &expected, std::vector<Entry> &entries) const {
    const auto path = wide(raw);
    if (!raw.empty() && path.empty()) {
        return false;
    }
    auto locked = lookup(path, nullptr, true, nullptr);
    if (!locked || !locked->leaf.same_object(expected)) {
        return false;
    }
    const auto listing = upgrade(locked->leaf, true);
    if (!listing) {
        return false;
    }
    locked.reset();
    std::vector<Entry> staged;
    auto mode = FileIdBothDirectoryRestartInfo;
    alignas(FILE_ID_BOTH_DIR_INFO) std::array<unsigned char, 16384> buffer;
    while (GetFileInformationByHandleEx(listing->handle_, mode, buffer.data(), DWORD(buffer.size()))) {
        mode = FileIdBothDirectoryInfo;
        std::size_t offset = 0;
        for (;;) {
            constexpr auto fixed = offsetof(FILE_ID_BOTH_DIR_INFO, FileName);
            if (buffer.size() - offset < fixed) {
                return false;
            }
            const auto *info = reinterpret_cast<const FILE_ID_BOTH_DIR_INFO *>(buffer.data() + offset);
            const auto next = info->NextEntryOffset;
            if ((next && (next < fixed || next > buffer.size() - offset || next % alignof(FILE_ID_BOTH_DIR_INFO))) ||
                info->FileNameLength % sizeof(wchar_t) ||
                info->FileNameLength > (next ? next : buffer.size() - offset) - fixed) {
                return false;
            }
            const std::wstring_view name(info->FileName, info->FileNameLength / sizeof(wchar_t));
            if (component(name)) {
                auto spelling = utf8(name);
                if (!spelling.empty()) {
                    if (staged.size() == max_scan) {
                        return false;
                    }
                    staged.push_back({std::move(spelling), std::uint64_t(info->FileId.QuadPart)});
                }
            }
            if (!next) break;
            offset += next;
        }
    }
    if (GetLastError() != ERROR_NO_MORE_FILES) {
        return false;
    }
    const auto current = resolve(raw);
    if (!current || !current->same_object(expected)) {
        return false;
    }
    entries.swap(staged);
    return true;
}

} // namespace hostdrv
