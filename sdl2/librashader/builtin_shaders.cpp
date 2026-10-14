/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "vaeg_builtin_shaders.h"

// M110: the frontend points this at vaeg-cache next to the executable; a
// relative "vaeg-cache" (the current directory) remains the default.
static std::string vaeg_shader_cache_root = "vaeg-cache";

extern "C" void vaeg_builtin_shader_set_cache_root(const char *directory) {
	if (directory != nullptr && directory[0] != '\0')
		vaeg_shader_cache_root = directory;
}

extern "C" const char *vaeg_builtin_shader_preset(void) {
    // Called at preset initialization, never in the per-frame draw path.
    static const std::string preset = []() -> std::string {
        try {
			const auto root = std::filesystem::absolute(
			    std::filesystem::u8path(vaeg_shader_cache_root) / "shaders" / vaeg_shader_digest);
			for (const auto &asset : vaeg_builtin_shaders) {
                const auto path = root / asset.path;
                std::filesystem::create_directories(path.parent_path());
                if (std::filesystem::exists(path)) {
                    std::ifstream input(path, std::ios::binary);
                    const std::string actual((std::istreambuf_iterator<char>(input)), {});
                    if (input.bad() || actual != asset.text)
                        throw std::runtime_error("VAEG_SHADER_CACHE_MISMATCH: remove the shader cache directory and restart");
                    continue;
                }
                auto temporary = path;
                temporary += ".tmp";
                std::ofstream output(temporary, std::ios::binary);
                output << asset.text;
                output.close();
                if (!output)
                    throw std::runtime_error("Cannot write embedded shader cache");
                std::filesystem::rename(temporary, path);
            }
            return (root / "vaeg_crt_default.slangp").u8string();
        } catch (const std::exception &error) {
            std::fprintf(stderr, "Embedded CRT assets: %s\n", error.what());
            return {};
        }
    }();
    return preset.c_str();
}
