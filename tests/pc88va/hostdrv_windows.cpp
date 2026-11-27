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
#include "sdl2/hostdrv_namespace.h"

#include <winioctl.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

namespace hostdrv {
struct WindowsRootTest {
    static auto hold(const WindowsRoot &root, std::wstring_view child) {
        return root.lookup(child, nullptr, true, nullptr);
    }
};
}

namespace {
using hostdrv::WindowsRoot;
constexpr DWORD sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
constexpr FILETIME old_time{0x3f000000, 0x01c0abcd};
const wchar_t japanese[] = L"\u8868.txt";
unsigned failures = 0, checks = 0, skipped = 0;

void check(const char *name, bool pass) {
    ++checks;
    if (!pass) ++failures;
    std::printf("%s %s\n", pass ? "PASS" : "FAIL", name);
}

int summary() {
    std::printf("SUMMARY %u passed, %u failed, %u skipped\n", checks - failures, failures, skipped);
    return failures ? 1 : 0;
}

std::string utf8(std::wstring_view name) {
    const int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name.data(), int(name.size()),
                                        nullptr, 0, nullptr, nullptr);
    std::string result(len, '\0');
    if (len) WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name.data(), int(name.size()),
                                 result.data(), len, nullptr, nullptr);
    return result;
}

void diagnostic(const char *stage, std::wstring_view name, DWORD code) {
    std::printf("INFO %s [%s] 0x%08lx (%lu)\n", stage, utf8(name).c_str(),
                static_cast<unsigned long>(code), static_cast<unsigned long>(code));
}

bool empty_dir(const std::wstring &path) {
    WIN32_FIND_DATAW info{};
    const HANDLE files = FindFirstFileW((path + L"\\*").c_str(), &info);
    if (files == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        diagnostic("FindFirstFileW Win32", path, error);
        return error == ERROR_FILE_NOT_FOUND;
    }
    bool empty = true;
    do {
        if (std::wcscmp(info.cFileName, L".") && std::wcscmp(info.cFileName, L"..")) {
            diagnostic("FindFirstFileW/FindNextFileW directory is not empty (entry attributes)",
                       info.cFileName, info.dwFileAttributes);
            empty = false;
            break;
        }
    } while (FindNextFileW(files, &info));
    const DWORD error = empty ? GetLastError() : 0;
    if (empty && error != ERROR_NO_MORE_FILES) diagnostic("FindNextFileW Win32", path, error);
    if (!FindClose(files)) {
        diagnostic("FindClose Win32", path, GetLastError());
        return false;
    }
    return empty && error == ERROR_NO_MORE_FILES;
}

// Track only paths created by this executable. Remove junctions as directory
// entries; never recurse through them during cleanup.
struct Fixtures {
    struct Path { std::wstring name; bool directory; };
    std::vector<Path> paths;
    bool dir(const std::wstring &path) {
        if (!CreateDirectoryW(path.c_str(), nullptr)) {
            diagnostic("CreateDirectoryW Win32", path, GetLastError());
            return false;
        }
        paths.push_back({path, true});
        return true;
    }
    bool file(const std::wstring &path, char byte) {
        const HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, sharing, nullptr,
                                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            diagnostic("CreateFileW fixture Win32", path, GetLastError());
            return false;
        }
        paths.push_back({path, false});
        DWORD n = 0;
        const bool ok = WriteFile(h, &byte, 1, &n, nullptr) && n == 1;
        if (!ok) diagnostic("WriteFile fixture (one byte expected)", path, n);
        return CloseHandle(h) && ok;
    }
    bool cleanup() {
        bool ok = true;
        for (auto it = paths.rbegin(); it != paths.rend(); ++it) {
            if (!(it->directory ? RemoveDirectoryW(it->name.c_str()) : DeleteFileW(it->name.c_str())) &&
                GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) {
                ok = false;
            }
        }
        return ok;
    }
};

bool stamp(const std::wstring &path, FILETIME &value, bool seed) {
    HANDLE h = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES | (seed ? FILE_WRITE_ATTRIBUTES : 0), sharing,
                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    const bool ok = (!seed || SetFileTime(h, nullptr, &old_time, nullptr)) &&
                    GetFileTime(h, nullptr, &value, nullptr);
    return CloseHandle(h) && ok;
}

bool junction(Fixtures &fixtures, const std::wstring &path, const std::wstring &destination) {
    if (!fixtures.dir(path)) return false;
    // Mount-point reparse buffer: header, substitute and print names.
    struct Buffer {
        DWORD tag;
        WORD length, reserved, substitute_offset, substitute_length, print_offset, print_length;
        wchar_t names[2048];
    } buffer{};
    const std::wstring target = L"\\??\\" + destination;
    if (target.size() + destination.size() + 2 > std::size(buffer.names)) return false;
    buffer.tag = IO_REPARSE_TAG_MOUNT_POINT;
    buffer.substitute_length = WORD(target.size() * sizeof(wchar_t));
    buffer.print_offset = WORD(buffer.substitute_length + sizeof(wchar_t));
    buffer.print_length = WORD(destination.size() * sizeof(wchar_t));
    std::wmemcpy(buffer.names, target.c_str(), target.size() + 1);
    std::wmemcpy(buffer.names + target.size() + 1, destination.c_str(), destination.size() + 1);
    buffer.length = WORD(8 + buffer.print_offset + buffer.print_length + sizeof(wchar_t));
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD bytes = 0;
    const bool ok = DeviceIoControl(h, FSCTL_SET_REPARSE_POINT, &buffer, buffer.length + 8,
                                    nullptr, 0, &bytes, nullptr);
    return CloseHandle(h) && ok;
}

DWORD rename_directory(const std::wstring &source, const std::wstring &target) {
    HANDLE h = CreateFileW(source.c_str(), DELETE, sharing, nullptr, OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }
    alignas(FILE_RENAME_INFO) unsigned char bytes[32768]{};
    auto *info = reinterpret_cast<FILE_RENAME_INFO *>(bytes);
    const auto size = target.size() * sizeof(wchar_t);
    DWORD error = ERROR_FILENAME_EXCED_RANGE;
    if (size <= sizeof(bytes) - offsetof(FILE_RENAME_INFO, FileName)) {
        info->FileNameLength = DWORD(size);
        std::memcpy(info->FileName, target.data(), size);
        error = SetFileInformationByHandle(h, FileRenameInfo, info, sizeof(bytes)) ? ERROR_SUCCESS : GetLastError();
    }
    CloseHandle(h);
    return error;
}

bool read_byte(const WindowsRoot::Handle &file, char expected) {
    char byte = 0;
    std::uint16_t n = 0;
    return WindowsRoot::read(file, 0, &byte, 1, n) && n == 1 && byte == expected;
}

void run(Fixtures &fixtures, const std::wstring &base) {
    const auto path = base + L"\\root", parent = path + L"\\PARENT", renamed = base + L"\\moved";
    const auto outside = base + L"\\outside";
    const auto data = parent + L"\\DATA.TXT", jp = path + L"\\" + japanese;
    const bool seeded = fixtures.dir(path) && fixtures.dir(parent) && fixtures.dir(outside) &&
                        fixtures.file(data, 'I') &&
                        fixtures.file(outside + L"\\DATA.TXT", 'O') && fixtures.file(jp, 'J');
    check("FIXTURES", seeded);
    if (!seeded) return;
    auto root = WindowsRoot::select(utf8(path), diagnostic);
    check("LOCAL_NTFS", bool(root));
    if (!root) return;
    {
        const auto original = base + L"\\selected", moved_root = base + L"\\selected-moved";
        const auto selection = fixtures.dir(original) ? WindowsRoot::select(utf8(original), diagnostic)
                                                       : std::nullopt;
        const auto expected = selection ? selection->resolve("") : std::nullopt;
        const bool renamed_root = expected && MoveFileExW(original.c_str(), moved_root.c_str(), 0);
        bool anchored = false;
        if (renamed_root) {
            fixtures.paths.back().name = moved_root;
            std::vector<WindowsRoot::Entry> listing;
            anchored = fixtures.dir(original) && fixtures.file(original + L"\\OTHER.TXT", 'O') &&
                fixtures.file(moved_root + L"\\KEPT.TXT", 'K') && selection->enumerate("", *expected, listing) &&
                listing.size() == 1 && listing.front().raw_name == "KEPT.TXT";
        }
        check("RENAMED_ROOT_LISTS_ORIGINAL_OBJECT", anchored);
    }
    {
        const auto mixed = WindowsRoot::select(utf8(base + L"\\ROOT"), diagnostic);
        const auto exact = root->resolve("");
        const auto folded = mixed ? mixed->resolve("") : std::nullopt;
        check("MOUNT_PATH_CASE_FOLD_ONLY", exact && folded && exact->same_object(*folded) &&
              root->resolve("PARENT") && !root->resolve("parent"));
    }

    HKEY key = nullptr;
    DWORD setting = 0, type = 0, len = sizeof(setting);
    const bool queried = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\FileSystem",
                                      0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS &&
                         RegQueryValueExW(key, L"NtfsDisableLastAccessUpdate", nullptr, &type,
                            reinterpret_cast<BYTE *>(&setting), &len) == ERROR_SUCCESS &&
                         type == REG_DWORD && len == sizeof(setting);
    if (key) RegCloseKey(key);
    if (queried) {
        std::printf("INFO NTFS last-access setting: 0x%08lx (%s)\n", static_cast<unsigned long>(setting),
                    (setting & 1) ? "updates disabled" : "updates enabled");
    } else {
        std::puts("INFO NTFS last-access setting: unavailable; run fsutil behavior query disablelastaccess");
    }
    check("LAST_ACCESS_SETTING", queried);

    FILETIME file_before{}, dir_before{}, file_after{}, dir_after{};
    const bool times = stamp(data, file_before, true) && stamp(parent, dir_before, true);
    auto parent_pin = root->resolve("PARENT");
    auto file_pin = root->resolve("PARENT/DATA.TXT");
    auto handle = parent_pin && file_pin ? root->open_file("PARENT/DATA.TXT", *parent_pin, *file_pin) : std::nullopt;
    std::vector<WindowsRoot::Entry> entries;
    const bool listed = parent_pin && root->enumerate("PARENT", *parent_pin, entries) && entries.size() == 1 &&
                        entries.front().raw_name == "DATA.TXT";
    const bool read = handle && read_byte(*handle, 'I');
    check("CONTAINED_READ_AND_LIST", listed && read);
    check("LAST_ACCESS_UNCHANGED", times && listed && read && handle->close_checked() &&
          stamp(data, file_after, false) && stamp(parent, dir_after, false) &&
          CompareFileTime(&file_before, &old_time) == 0 && CompareFileTime(&dir_before, &old_time) == 0 &&
          CompareFileTime(&file_before, &file_after) == 0 && CompareFileTime(&dir_before, &dir_after) == 0);
    check("PATH_ESCAPE", !root->resolve("../outside/DATA.TXT") && !root->resolve("/PARENT/DATA.TXT") &&
          !root->resolve("PARENT\\DATA.TXT") && !root->resolve("PARENT//DATA.TXT"));
    std::uint64_t total = 0;
    check("CAPACITY", root->capacity(total) && total > 0);

    {
        const auto target = path + L"\\LOCKED", away = base + L"\\locked-away";
        const auto replacement = base + L"\\replacement";
        if (fixtures.dir(target) && fixtures.file(target + L"\\LEAF.TXT", 'L') && fixtures.dir(replacement)) {
            fixtures.paths.push_back({away, true});
            fixtures.paths.push_back({away + L"\\LEAF.TXT", false});
            const auto replace = [&]() {
                const DWORD error = rename_directory(target, away);
                return error == ERROR_SUCCESS ? rename_directory(replacement, target) : error;
            };
            const DWORD unlocked = replace();
            const bool restored = unlocked == ERROR_SUCCESS &&
                                  rename_directory(target, replacement) == ERROR_SUCCESS &&
                                  rename_directory(away, target) == ERROR_SUCCESS;
            check("UNLOCKED_ANCESTOR_REPLACE_CONTROL", restored);
            if (restored) {
                auto held = hostdrv::WindowsRootTest::hold(*root, L"LOCKED/LEAF.TXT");
                const DWORD blocked = held ? replace() : ERROR_INVALID_HANDLE;
                if (blocked != ERROR_SHARING_VIOLATION) diagnostic("held replacement Win32", target, blocked);
                check("HELD_ANCESTOR_REPLACE", blocked == ERROR_SHARING_VIOLATION);
            }
        } else {
            check("ANCESTOR_REPLACE_FIXTURES", false);
        }
    }
    HANDLE deleter = CreateFileW(parent.c_str(), DELETE, sharing, nullptr, OPEN_EXISTING,
                                 FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    check("LOCK_CONFLICT_FAILS_CLOSED", deleter != INVALID_HANDLE_VALUE && !root->resolve("PARENT/DATA.TXT"));
    if (deleter != INVALID_HANDLE_VALUE) CloseHandle(deleter);
    constexpr auto guest_path = "PARENT/DATA.TXT";
    handle.reset();
    {
        auto retained = parent_pin && file_pin ? root->open_file(guest_path, *parent_pin, *file_pin) : std::nullopt;
        const auto displaced = outside + L"\\DISPLACED.TXT";
        const bool moved = MoveFileExW(data.c_str(), displaced.c_str(), 0);
        if (moved) fixtures.paths.push_back({displaced, false});
        else diagnostic("MoveFileExW leaf Win32", data, GetLastError());
        const bool seeded_leaf = moved && fixtures.file(data, 'N');
        if (moved && !seeded_leaf) std::puts("INFO could not seed replacement leaf");
        const auto fresh = seeded_leaf ? root->resolve(guest_path) : std::nullopt;
        const bool old_read = retained && read_byte(*retained, 'I');
        const bool stale_denied = parent_pin && file_pin && !root->open_file(guest_path, *parent_pin, *file_pin);
        const auto opened = parent_pin && fresh ? root->open_file(guest_path, *parent_pin, *fresh) : std::nullopt;
        const bool fresh_read = opened && read_byte(*opened, 'N');
        const bool distinct = fresh && file_pin && !fresh->same_object(*file_pin);
        if (!old_read || !stale_denied || !fresh_read || !distinct) {
            std::printf("INFO leaf replacement: retained=%d old_read=%d stale_denied=%d fresh_read=%d distinct=%d\n",
                        bool(retained), old_read, stale_denied, fresh_read, distinct);
        }
        check("LEAF_REPLACEMENT_PRESERVES_OPEN_HANDLE", distinct && old_read && stale_denied && fresh_read);
    }
    // NTFS can refuse an ancestor directory rename while a descendant file
    // is open even with delete sharing. Test ancestor identity separately,
    // without live descendant handles, and keep the old directory pin.
    file_pin.reset();
    entries.clear();
    const bool moved = MoveFileExW(parent.c_str(), renamed.c_str(), 0);
    if (moved) {
        fixtures.paths.push_back({renamed, true});
        fixtures.paths.push_back({renamed + L"\\DATA.TXT", false});
    } else {
        diagnostic("MoveFileExW ancestor Win32", parent, GetLastError());
    }
    const bool seeded_parent = moved && fixtures.dir(parent) && fixtures.file(data, 'R');
    if (moved && !seeded_parent) std::puts("INFO could not seed replacement ancestor and leaf");
    const auto fresh_parent = seeded_parent ? root->resolve("PARENT") : std::nullopt;
    const auto fresh_leaf = seeded_parent ? root->resolve(guest_path) : std::nullopt;
    const auto replacement_file = fresh_parent && fresh_leaf ? root->open_file(guest_path, *fresh_parent, *fresh_leaf)
                                                             : std::nullopt;
    const bool distinct = parent_pin && fresh_parent && !fresh_parent->same_object(*parent_pin);
    const bool stale_denied = parent_pin && fresh_leaf && !root->open_file(guest_path, *parent_pin, *fresh_leaf);
    const bool fresh_read = replacement_file && read_byte(*replacement_file, 'R');
    if (!distinct || !stale_denied || !fresh_read) {
        std::printf("INFO ancestor replacement: distinct=%d stale_denied=%d fresh_read=%d\n",
                    distinct, stale_denied, fresh_read);
    }
    check("UNLOCKED_REPLACEMENT_IS_NEW_OBJECT", distinct && stale_denied && fresh_read);

    const auto mountpoint = path + L"\\JUNCTION";
    check("JUNCTION_REJECTED", junction(fixtures, mountpoint, outside) &&
          !root->resolve("JUNCTION") && !root->resolve("JUNCTION/DATA.TXT") &&
          !WindowsRoot::select(utf8(mountpoint)));
    const auto link = path + L"\\SYMLINK";
    // 2 permits non-elevated symlink creation when Developer Mode is on.
    const auto target = outside + L"\\DATA.TXT";
    bool linked = CreateSymbolicLinkW(link.c_str(), target.c_str(), 2);
    if (!linked && GetLastError() == ERROR_INVALID_PARAMETER) {
        linked = CreateSymbolicLinkW(link.c_str(), target.c_str(), 0);
    }
    if (linked) {
        fixtures.paths.push_back({link, false});
        check("SYMLINK_REJECTED", !root->resolve("SYMLINK"));
    } else {
        const DWORD error = GetLastError();
        if (error == ERROR_PRIVILEGE_NOT_HELD) {
            ++skipped;
            std::puts("SKIP SYMLINK_REJECTED (fixture not creatable: 1314)");
        } else {
            diagnostic("CreateSymbolicLinkW Win32", link, error);
            check("SYMLINK_REJECTED", false);
        }
    }

    const auto alias = path + L"\\HARDLINK";
    const bool hardlink = CreateHardLinkW(alias.c_str(), (outside + L"\\DATA.TXT").c_str(), nullptr);
    if (hardlink) fixtures.paths.push_back({alias, false});
    check("HARDLINK_REJECTED", hardlink && !root->resolve("HARDLINK"));
    const auto stream = data + L":extra";
    const bool stream_created = fixtures.file(stream, 'S');
    HANDLE pipe = CreateNamedPipeW(L"\\\\.\\pipe\\vaeg-hostdrv-ntfs-test", PIPE_ACCESS_DUPLEX,
                                   PIPE_TYPE_BYTE, 1, 128, 128, 0, nullptr);
    check("STREAM_AND_DEVICE_REJECTED", stream_created && pipe != INVALID_HANDLE_VALUE &&
          !root->resolve("PARENT/DATA.TXT:extra") && !root->resolve("\\\\.\\pipe\\vaeg-hostdrv-ntfs-test") &&
          !WindowsRoot::select("\\\\.\\pipe\\vaeg-hostdrv-ntfs-test") &&
          !WindowsRoot::select("\\\\localhost\\C$"));
    if (pipe != INVALID_HANDLE_VALUE) CloseHandle(pipe);

    const auto long_jp = path + L"\\\u8868\u8868\u8868\u8868\u8868.txt";
    const bool long_created = fixtures.file(long_jp, 'A');
    const auto many = path + L"\\MANY";
    bool seeded_scan = fixtures.dir(many);
    auto many_pin = seeded_scan ? root->resolve("MANY") : std::nullopt;
    std::vector<WindowsRoot::Entry> bounded;
    check("RAW_SCAN_CONTROL", many_pin && root->enumerate("MANY", *many_pin, bounded) && bounded.empty());
    for (unsigned i = 0; seeded_scan && i < 4097; ++i) {
        seeded_scan = fixtures.file(many + L"\\" + std::to_wstring(i), 'S');
    }
    check("RAW_SCAN_CAP", seeded_scan && many_pin && !root->enumerate("MANY", *many_pin, bounded) && bounded.empty());

    const auto big = parent + L"\\BIG.BIN";
    bool oversized = fixtures.file(big, 'B');
    if (oversized) {
        const HANDLE h = CreateFileW(big.c_str(), GENERIC_WRITE, sharing, nullptr, OPEN_EXISTING, 0, nullptr);
        oversized = h != INVALID_HANDLE_VALUE;
        if (oversized) {
            LARGE_INTEGER length{};
            length.QuadPart = 0x80000000LL;
            DWORD unused = 0;
            oversized = DeviceIoControl(h, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &unused, nullptr) &&
                        SetFilePointerEx(h, length, nullptr, FILE_BEGIN) && SetEndOfFile(h);
            oversized = CloseHandle(h) && oversized;
        }
    }
    hostdrv::Namespace names(std::move(*root));
    std::vector<hostdrv::Namespace::Item> found;
    check("OVERSIZED_FILE_HIDDEN_FROM_LIST", oversized && names.list("PARENT", found) == hostdrv::Namespace::ok &&
          found.size() == 1 && found.front().info.size == 1);
    hostdrv::Namespace::Item item{};
    std::optional<WindowsRoot::Handle> opened;
    check("JAPANESE_CP932_AND_ALIAS", long_created &&
          names.open("\x95\x5c.TXT", opened, item) == hostdrv::Namespace::ok && opened && read_byte(*opened, 'J') &&
          names.open("\x95\x5c\x95\x5c\x95\x5c~1.TXT", opened, item) == hostdrv::Namespace::ok &&
          opened && read_byte(*opened, 'A'));
}
} // namespace

int wmain(int argc, wchar_t **argv) {
    if (argc != 2) {
        check("USAGE: hostdrv_wintest.exe <empty local NTFS directory>", false);
        return summary();
    }
    std::wstring base(argv[1]);
    if (base.size() > 3 && (base.back() == L'\\' || base.back() == L'/')) base.pop_back();
    for (auto &ch : base) if (ch == L'/') ch = L'\\';
    const bool selected = bool(WindowsRoot::select(utf8(base), diagnostic));
    check("SELECT_LOCAL_NTFS_DIRECTORY", selected);
    if (!selected) return summary();
    const bool empty = empty_dir(base);
    check("EMPTY_LOCAL_NTFS_DIRECTORY", empty);
    if (!empty) return summary();
    Fixtures fixtures;
    const auto work = base + L"\\vaeg-hostdrv-wintest";
    if (fixtures.dir(work)) run(fixtures, work);
    else check("WORK_DIRECTORY", false);
    check("CLEANUP", fixtures.cleanup() && empty_dir(base));
    return summary();
}
