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
#include "hostdrv_names.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

namespace hostdrv {
namespace {

constexpr std::uint32_t mappings[] = {
#include "hostdrv_cp932.inc"
};

bool lead(unsigned char c) {
    return (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc);
}

char upper(char c) {
    // VA FILE_UCASE_TAB leaves all high single bytes unchanged.
    return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
}

unsigned decode932(std::string_view text, std::size_t &i) {
    unsigned key = static_cast<unsigned char>(text[i++]);
    if (key < 0x80) {
        return key;
    }
    if (lead(key)) {
        if (i == text.size()) {
            return 0xffffffffu;
        }
        key = (key << 8) | static_cast<unsigned char>(text[i++]);
    }
    const auto *found = std::lower_bound(std::begin(mappings), std::end(mappings), key << 16);
    return found != std::end(mappings) && (*found >> 16) == key ? *found & 0xffffu : 0xffffffffu;
}

unsigned encode932(unsigned scalar) {
    if (scalar < 0x80) {
        return scalar;
    }
    if (scalar > 0xffff) {
        return 0;
    }
    // Use the lowest encoded spelling when the table has duplicate mappings.
    static const auto reverse = [] {
        std::array<std::uint16_t, 65536> keys{};
        for (auto pair : mappings) {
            auto &key = keys[pair & 0xffffu];
            if (!key) {
                key = pair >> 16;
            }
        }
        return keys;
    }();
    return reverse[scalar];
}

bool utf8_next(std::string_view raw, std::size_t &i, unsigned &scalar) {
    const auto c = static_cast<unsigned char>(raw[i++]);
    if (c < 0x80) {
        scalar = c;
        return c != 0;
    }
    const unsigned tails = c >= 0xf0 && c <= 0xf4 ? 3 : c >= 0xe0 && c < 0xf0 ? 2 :
                           c >= 0xc2 && c < 0xe0 ? 1 : 0;
    if (!tails || raw.size() - i < tails) {
        return false;
    }
    scalar = c & (0x7fu >> (tails + 1));
    for (unsigned n = 0; n < tails; ++n) {
        const auto b = static_cast<unsigned char>(raw[i++]);
        if ((b & 0xc0) != 0x80) {
            return false;
        }
        scalar = (scalar << 6) | (b & 0x3f);
    }
    constexpr unsigned minimum[] = {0, 0x80, 0x800, 0x10000};
    return scalar >= minimum[tails] && scalar <= 0x10ffff &&
           !(scalar >= 0xd800 && scalar <= 0xdfff);
}

bool legal_single(unsigned c) {
    return c > 0x20 && c != 0x7f &&
           (c >= 0x80 || std::string_view("\"*+,./:;<=>?[\\]|").find(c) == std::string_view::npos);
}

bool reserved(std::string_view base) {
    return base == "CON" || base == "PRN" || base == "AUX" || base == "NUL" ||
           base == "CLOCK$" || base == "CONIN$" || base == "CONOUT$" ||
           (base.size() == 4 && (base.substr(0, 3) == "COM" || base.substr(0, 3) == "LPT") &&
            base[3] >= '1' && base[3] <= '9');
}

DosName padded(std::string_view base, std::string_view ext) {
    DosName result(base);
    result.resize(8, ' ');
    result.append(ext);
    result.resize(11, ' ');
    return result;
}

bool template_field(std::string_view field) {
    bool padding = false;
    for (std::size_t i = 0; i < field.size();) {
        const auto start = i;
        const auto c = decode932(field, i);
        if (c == ' ') {
            padding = true;
        } else if (padding || c == 0xffffffffu ||
                   (i == start + 1 && c != '?' && !legal_single(c))) {
            return false;
        }
    }
    return true;
}

std::string sanitized(std::string_view text, std::size_t length) {
    std::string result;
    for (std::size_t i = 0; i < text.size() && result.size() < length;) {
        unsigned scalar;
        if (!utf8_next(text, i, scalar)) {
            break;
        }
        const auto key = encode932(scalar);
        if (key > 0xff) {
            if (length - result.size() < 2) {
                break;
            }
            result += static_cast<char>(key >> 8);
            result += static_cast<char>(key);
        } else if (legal_single(key)) {
            result += upper(static_cast<char>(key));
        }
    }
    return result;
}

} // namespace

std::optional<DosName> dos_name(std::string_view component) {
    std::string base, ext;
    bool dot = false;
    for (std::size_t i = 0; i < component.size();) {
        const auto start = i;
        const auto c = decode932(component, i);
        if (c == 0xffffffffu) {
            return std::nullopt;
        }
        if (i == start + 1 && c == '.' && !dot) {
            dot = true;
            continue;
        }
        auto &field = dot ? ext : base;
        if (i == start + 2) {
            field.append(component.substr(start, 2));
        } else if (legal_single(c)) {
            field += upper(component[start]);
        } else {
            return std::nullopt;
        }
        if (field.size() > (dot ? 3u : 8u)) {
            return std::nullopt;
        }
    }
    if (base.empty() || (dot && ext.empty()) || reserved(base)) {
        return std::nullopt;
    }
    return padded(base, ext);
}

std::optional<std::vector<DosName>> dos_path(std::string_view path) {
    if (path.size() > 127 || (!path.empty() && path.front() == '\\')) {
        return std::nullopt;
    }
    std::vector<DosName> result;
    for (std::size_t start = 0; start < path.size();) {
        auto end = start;
        while (end < path.size() && path[end] != '\\') {
            if (decode932(path, end) == 0xffffffffu) {
                return std::nullopt;
            }
        }
        auto name = dos_name(path.substr(start, end - start));
        if (!name || (end < path.size() && end + 1 == path.size())) {
            return std::nullopt;
        }
        result.push_back(std::move(*name));
        start = end + 1;
    }
    return result;
}

std::optional<HostName> host_name(std::string_view raw) {
    if (raw.empty() || raw == "." || raw == "..") {
        return std::nullopt;
    }
    bool roundtrip = true;
    std::string encoded;
    for (std::size_t i = 0; i < raw.size();) {
        unsigned scalar;
        if (!utf8_next(raw, i, scalar)) {
            return std::nullopt;
        }
        const auto key = encode932(scalar);
        if (!key) {
            roundtrip = false;
        } else if (key > 0xff) {
            encoded += static_cast<char>(key >> 8);
            encoded += static_cast<char>(key);
        } else {
            encoded += upper(static_cast<char>(key));
        }
    }
    const auto dot = raw.rfind('.');
    const bool has_ext = dot != raw.npos && dot != 0;
    HostName result;
    result.prefix = sanitized(raw.substr(0, has_ext ? dot : raw.size()), 8);
    if (result.prefix.empty()) {
        result.prefix = "FILE";
    }
    if (has_ext) {
        result.extension = sanitized(raw.substr(dot + 1), 3);
    }
    if (roundtrip) {
        result.native = dos_name(encoded);
        result.folded = std::move(encoded);
    }
    return result;
}

std::optional<DosName> alias_name(const HostName &name, unsigned number) {
    if (!number || number > 999999) {
        return std::nullopt;
    }
    const auto digits = std::to_string(number);
    const auto limit = 7 - digits.size();
    std::size_t length = 0;
    while (length < name.prefix.size()) {
        const std::size_t width = lead(static_cast<unsigned char>(name.prefix[length])) ? 2 : 1;
        if (length + width > limit) {
            break;
        }
        length += width;
    }
    const auto prefix = length ? name.prefix.substr(0, length) : std::string("FILE").substr(0, limit);
    return padded(prefix + '~' + digits, name.extension);
}

bool valid_template(std::string_view pattern) {
    return pattern.size() == 11 && pattern.front() != ' ' &&
           template_field(pattern.substr(0, 8)) && template_field(pattern.substr(8));
}

bool matches(std::string_view name, std::string_view pattern) {
    if (name.size() != 11 || pattern.size() != 11) {
        return false;
    }
    for (std::size_t i = 0; i < 11;) {
        const std::size_t width = lead(static_cast<unsigned char>(name[i])) ? 2 : 1;
        const auto literal = name.substr(i, width);
        const auto expected = pattern.substr(i, width);
        if (width != literal.size() || (expected != literal && expected != (width == 2 ? "??" : "?"))) {
            return false;
        }
        i += width;
    }
    return true;
}

} // namespace hostdrv
