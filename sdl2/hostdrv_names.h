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
#ifndef VAEG_HOSTDRV_NAMES_H
#define VAEG_HOSTDRV_NAMES_H

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hostdrv {

// Names are folded, padded 8+3 CP932 bytes; 0xe5 is not escaped here.
using DosName = std::string;

// Reject malformed encoding, illegal names and byte lengths over 8+3.
std::optional<DosName> dos_name(std::string_view component);
std::optional<std::vector<DosName>> dos_path(std::string_view path);

struct HostName {
    std::optional<std::string> folded; // full raw basename; for case-only continuity
    std::optional<DosName> native;
    std::string prefix;
    std::string extension;
};

// Input is actual raw spelling as UTF-8; returns nothing on invalid UTF-8.
std::optional<HostName> host_name(std::string_view raw);
std::optional<DosName> alias_name(const HostName &name, unsigned number);

bool valid_template(std::string_view pattern);
bool matches(std::string_view name, std::string_view pattern);

} // namespace hostdrv
#endif
