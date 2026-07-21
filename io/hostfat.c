/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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
#include "compiler.h"
#include "cpucore.h"
#include "hostfat.h"

typedef struct {
	BYTE *image;
	UINT32 digest;
} HOSTFAT_STATE;

static HOSTFAT_STATE hostfat_state;

static UINT32 digest_image(const BYTE *image, UINT32 size) {

	UINT32 digest;
	UINT32 position;

	digest = 2166136261U;
	for (position=0; position<size; position++) {
		digest ^= image[position];
		digest *= 16777619U;
	}
	return(digest);
}

BOOL hostfat_mount_image(const void *image, UINT32 size) {

	BYTE *replacement;
	BYTE *previous;

	if ((image == NULL) || (size != HOSTFAT_IMAGE_SIZE)) {
		return(FAILURE);
	}
	replacement = (BYTE *)_MALLOC(size, "hostfat-image");
	if (replacement == NULL) {
		return(FAILURE);
	}
	CopyMemory(replacement, image, size);
	previous = hostfat_state.image;
	hostfat_state.image = replacement;
	hostfat_state.digest = digest_image(replacement, size);
	if (previous != NULL) {
		_MFREE(previous);
	}
	return(SUCCESS);
}

void hostfat_unmount(void) {

	if (hostfat_state.image != NULL) {
		_MFREE(hostfat_state.image);
	}
	ZeroMemory(&hostfat_state, sizeof(hostfat_state));
}

BOOL hostfat_is_mounted(void) {

	return(hostfat_state.image != NULL);
}

UINT32 hostfat_image_digest(void) {

	return(hostfat_state.digest);
}

BOOL hostfat_read_sector(UINT32 sector, void *destination) {

	if ((hostfat_state.image == NULL) || (destination == NULL) ||
		(sector >= HOSTFAT_TOTAL_SECTORS)) {
		return(FAILURE);
	}
	CopyMemory(destination,
			hostfat_state.image + (sector * HOSTFAT_SECTOR_SIZE),
			HOSTFAT_SECTOR_SIZE);
	return(SUCCESS);
}
