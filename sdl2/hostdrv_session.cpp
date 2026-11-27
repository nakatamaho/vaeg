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
#include "hostdrv_session.h"

#include "hostdrv_service.h"
#include "io/hostdrv_lifecycle.h"
#include "io/hostdrv_wire.h"

#include <new>

namespace {
hostdrv::Service session;
bool bound;
}

int vaeg_hostdrv_session_start(const char *path) {
    if (bound || !path || !session.bind_lifecycle()) {
        return 0;
    }
    bound = true;
    try {
        if (session.enable(path)) {
            hostdrv_bind_exclusive_transport(hostdrv::Service::dispatch, &session);
            return 1;
        }
    } catch (const std::bad_alloc &) {
        // Fail startup without leaving authority or exceptions at the C boundary.
    }
    vaeg_hostdrv_session_stop();
    return 0;
}

void vaeg_hostdrv_session_stop(void) {
    if (bound) {
        hostdrv_bind_exclusive_transport(nullptr, nullptr);
        session.discard_dos_image();
        hostdrv_bind_lifecycle(nullptr, nullptr);
        bound = false;
    }
}
