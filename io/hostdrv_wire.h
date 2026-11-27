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
#ifndef VAEG_HOSTDRV_WIRE_H
#define VAEG_HOSTDRV_WIRE_H

#include <stdint.h>

/* Serialized offsets; this packet never contains native pointers. */
enum {
	HD_VERSION = 4, HD_OPCODE = 5, HD_SEQUENCE = 6, HD_GENERATION = 8,
	HD_TOKEN = 12, HD_OFFSET = 16, HD_COUNT = 20, HD_PATH = 22,
	HD_PATH2 = 24, HD_INPUT = 26, HD_CAPACITY = 28, HD_OPTIONS = 30,
	HD_STATUS = 32, HD_ACTUAL = 34, HD_OUTPUT = 36, HD_CAPS = 38,
	HD_SIZE = 40, HD_ATTR = 44, HD_TIME = 46, HD_DATE = 48, HD_SERIAL = 50,
	HD_HEADER = 64, HD_DATA = 4096, HD_PACKET = HD_HEADER + HD_DATA
};

enum {
	HD_QUERY, HD_STAT, HD_SPACE, HD_OPEN, HD_READ, HD_CLOSE, HD_FIND_FIRST,
	HD_FIND_NEXT, HD_FSTAT, HD_FIND_RELEASE, HD_ATTACH, HD_DETACH, HD_UNREGISTER
};

enum {
	HD_COMPLETED, HD_BAD_RANGE, HD_BAD_PACKET, HD_DISABLED, HD_BUSY
};

#define HD_REPLY_BASE UINT32_C(0x48520000)

uint16_t hostdrv_get16(const uint8_t *p);
uint32_t hostdrv_get32(const uint8_t *p);
void hostdrv_put16(uint8_t *p, uint16_t value);
void hostdrv_put32(uint8_t *p, uint32_t value);
int hostdrv_request_valid(const uint8_t packet[HD_PACKET]);
void hostdrv_response_init(uint8_t response[HD_PACKET], const uint8_t request[HD_PACKET],
                          uint16_t dos_status);
int hostdrv_response_valid(uint32_t port, const uint8_t request[HD_PACKET],
                           const uint8_t response[HD_PACKET]);

/* Arithmetic for the native VA RAM window only. Callers must establish the
 * native execution profile before interpreting the returned address as RAM.
 */
int hostdrv_bounce_address(uint32_t far_pointer, uint32_t mainram_limit, uint32_t *address);

/* The adapter supplies only a checked, linear writable RAM span, or NULL.
 * A null dispatcher means the execution profile does not enable the port.
 * Runs synchronously on the emulator thread; dispatch returns an HD_* transport
 * code and must not call the guest. Request and response must not overlap.
 * The caller publishes the returned dword only after this function returns.
 */
typedef unsigned (*hostdrv_dispatch)(void *context, const uint8_t request[HD_PACKET],
                                     uint8_t response[HD_PACKET]);
uint32_t hostdrv_exchange(int *busy, uint8_t *ram, hostdrv_dispatch dispatch, void *context);

/* Emulator-thread registration at a safe point, only after the caller has
 * established the exclusive execution profile. NULL disables dispatch.
 * Changing the adapter does not implement mount or DOS residency policy.
 */
void hostdrv_bind_exclusive_transport(hostdrv_dispatch dispatch, void *context);

#endif
