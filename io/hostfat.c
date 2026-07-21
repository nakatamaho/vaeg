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

enum {
	HOSTFAT_PACKET_SIZE = 18,
	HOSTFAT_PACKET_UNIT = 1,
	HOSTFAT_PACKET_COMMAND = 2,
	HOSTFAT_PACKET_TRANSFER = 10,
	HOSTFAT_PACKET_COUNT = 14,
	HOSTFAT_PACKET_START = 16,
	HOSTFAT_COMMAND_READ = 4,
	HOSTFAT_MAX_REQUEST_SECTORS = 128
};

typedef struct {
	BYTE *image;
	UINT32 digest;
} HOSTFAT_STATE;

static HOSTFAT_STATE hostfat_state;

static UINT16 load_word(const BYTE *source) {

	return((UINT16)(source[0] | ((UINT16)source[1] << 8)));
}

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

UINT8 hostfat_service_request(UINT32 request_far_pointer) {

	BYTE packet[HOSTFAT_PACKET_SIZE];
	UINT request_offset;
	UINT request_segment;
	UINT transfer_offset;
	UINT transfer_segment;
	UINT16 count;
	UINT16 start;
	UINT32 request_address;
	UINT32 transfer_address;
	UINT32 transfer_size;
	UINT32 image_offset;
	UINT32 position;

	if (hostfat_state.image == NULL) {
		return(HOSTFAT_RESULT_NOT_MOUNTED);
	}
	request_offset = LOW16(request_far_pointer);
	request_segment = (UINT)(request_far_pointer >> 16);
	request_address = ((UINT32)request_segment << 4) + request_offset;
	if ((request_offset > (0x10000U - HOSTFAT_PACKET_SIZE)) ||
		(request_address >= I286_MEMWRITEMAX) ||
		(HOSTFAT_PACKET_SIZE > (I286_MEMWRITEMAX - request_address))) {
		return(HOSTFAT_RESULT_BAD_REQUEST);
	}
	for (position=0; position<sizeof(packet); position++) {
		packet[position] = i286_memoryread(request_address + position);
	}
	if ((packet[0] < HOSTFAT_PACKET_SIZE) ||
		(packet[HOSTFAT_PACKET_UNIT] != 0) ||
		(packet[HOSTFAT_PACKET_COMMAND] != HOSTFAT_COMMAND_READ)) {
		return(HOSTFAT_RESULT_BAD_REQUEST);
	}
	count = load_word(packet + HOSTFAT_PACKET_COUNT);
	start = load_word(packet + HOSTFAT_PACKET_START);
	if (count == 0) {
		return(HOSTFAT_RESULT_OK);
	}
	if ((count > HOSTFAT_MAX_REQUEST_SECTORS) ||
		(start >= HOSTFAT_TOTAL_SECTORS) ||
		((UINT32)count > (HOSTFAT_TOTAL_SECTORS - start))) {
		return(HOSTFAT_RESULT_RANGE);
	}
	transfer_offset = load_word(packet + HOSTFAT_PACKET_TRANSFER);
	transfer_segment = load_word(packet + HOSTFAT_PACKET_TRANSFER + 2);
	transfer_address = ((UINT32)transfer_segment << 4) + transfer_offset;
	transfer_size = (UINT32)count * HOSTFAT_SECTOR_SIZE;
	if ((transfer_address >= I286_MEMWRITEMAX) ||
		(transfer_size > (I286_MEMWRITEMAX - transfer_address))) {
		return(HOSTFAT_RESULT_RANGE);
	}

	image_offset = (UINT32)start * HOSTFAT_SECTOR_SIZE;
	for (position=0; position<transfer_size; position++) {
		i286_memorywrite(transfer_address + position,
				hostfat_state.image[image_offset + position]);
	}
	return(HOSTFAT_RESULT_OK);
}
