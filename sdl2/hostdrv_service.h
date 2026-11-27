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
#ifndef VAEG_HOSTDRV_SERVICE_H
#define VAEG_HOSTDRV_SERVICE_H

#include "hostdrv_namespace.h"

#include <array>

namespace hostdrv {

// Emulator-thread service. Does not install the transport or observe guest RAM.
class Service {
public:
    struct Marks {
        std::uint32_t generation = 0;
        std::uint16_t serial = 0;
    };

    // Bind before enable; caller must unbind before destroying this service.
    bool bind_lifecycle();
    bool enable(const std::string &path);
    void revoke();
    // Caller must prove the prior DOS image and its references were discarded.
    void discard_dos_image();
    bool guarded() const { return installation_ != 0; }
    Marks marks() const { return marks_; }
    bool load_uninstalled(Marks saved);
    static unsigned dispatch(void *context, const std::uint8_t *request, std::uint8_t *reply);

private:
    struct File {
        PlatformRoot::Handle handle;
        bool hidden;
        bool denies_read;
    };
    using Result = std::array<std::uint8_t, 21>; // packed metadata (10), DOS name (11)
    using Snapshot = std::vector<Result>;

    unsigned execute(const std::uint8_t *request, std::uint8_t *reply);
    unsigned operate(const std::uint8_t *request, std::uint8_t *reply);
    static void find_result(const Result &result, std::uint8_t *reply);
    std::optional<Namespace> names_;
    std::map<std::uint32_t, File> files_;
    std::map<std::uint32_t, Snapshot> searches_;
    Marks marks_;
    std::uint32_t installation_ = 0;
    std::uint32_t next_token_ = 0;
    bool attached_ = false;
};

} // namespace hostdrv
#endif
