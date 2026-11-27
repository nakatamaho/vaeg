/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_HOSTDRV_LIFECYCLE_H
#define VAEG_HOSTDRV_LIFECYCLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t generation;
	uint16_t serial;
} HD_MARKS;

typedef struct {
	int (*guarded)(void *context);
	void (*reset)(void *context, int discarded);
	HD_MARKS (*marks)(void *context);
	void (*load_uninstalled)(void *context, HD_MARKS marks);
} HD_LIFECYCLE;

/* Emulator thread only. Bind before granting authority. Unbind only after
 * revocation and guest image disposal. This does not enable port discovery. */
int hostdrv_bind_lifecycle(const HD_LIFECYCLE *ops, void *context);
int hostdrv_state_guarded(void);
void hostdrv_reset(int discarded);
HD_MARKS hostdrv_get_marks(void);
void hostdrv_load_uninstalled(HD_MARKS marks);

#ifdef __cplusplus
}
#endif
#endif
