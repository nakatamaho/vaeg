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
#include "hostdrv_lifecycle.h"

#include <stddef.h>

static const HD_LIFECYCLE *lifecycle;
static void *service;
/* Preserve imported marks even on a host with no backing implementation. */
static HD_MARKS high_water;

static HD_MARKS merge(HD_MARKS lhs, HD_MARKS rhs) {
	if (lhs.generation < rhs.generation)
		lhs.generation = rhs.generation;
	if (lhs.serial < rhs.serial)
		lhs.serial = rhs.serial;
	return lhs;
}

int hostdrv_bind_lifecycle(const HD_LIFECYCLE *ops, void *context) {
	if (hostdrv_state_guarded() || (ops && (lifecycle || !ops->guarded || !ops->reset ||
	                                       !ops->marks || !ops->load_uninstalled)))
		return 0;
	if (lifecycle) {
		high_water = hostdrv_get_marks();
		hostdrv_reset(0);
	}
	lifecycle = ops;
	service = ops ? context : NULL;
	if (ops)
		hostdrv_load_uninstalled(high_water);
	return 1;
}

int hostdrv_state_guarded(void) {
	return lifecycle && lifecycle->guarded(service);
}

void hostdrv_reset(int discarded) {
	if (lifecycle)
		lifecycle->reset(service, discarded);
}

HD_MARKS hostdrv_get_marks(void) {
	return lifecycle ? merge(high_water, lifecycle->marks(service)) : high_water;
}

void hostdrv_load_uninstalled(HD_MARKS marks) {
	if (!hostdrv_state_guarded()) {
		high_water = merge(hostdrv_get_marks(), marks);
		if (lifecycle)
			lifecycle->load_uninstalled(service, high_water);
	}
}
