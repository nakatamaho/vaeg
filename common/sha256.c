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
#include "sha256.h"
#include <string.h>

static const uint32_t constants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
    0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
    0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
    0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
    0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
    0xc67178f2U};

static uint32_t rotate(uint32_t value, unsigned int count) {
	return (value >> count) | (value << (32 - count));
}

static void transform(VAEG_SHA256 *sha, const uint8_t *block) {
	uint32_t words[64], a, b, c, d, e, f, g, h, first, second;
	unsigned int i;
	for (i = 0; i < 16; i++) {
		words[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) |
		           ((uint32_t)block[i * 4 + 2] << 8) | block[i * 4 + 3];
	}
	for (i = 16; i < 64; i++) {
		first = rotate(words[i - 15], 7) ^ rotate(words[i - 15], 18) ^ (words[i - 15] >> 3);
		second = rotate(words[i - 2], 17) ^ rotate(words[i - 2], 19) ^ (words[i - 2] >> 10);
		words[i] = words[i - 16] + first + words[i - 7] + second;
	}
	a = sha->state[0];
	b = sha->state[1];
	c = sha->state[2];
	d = sha->state[3];
	e = sha->state[4];
	f = sha->state[5];
	g = sha->state[6];
	h = sha->state[7];
	for (i = 0; i < 64; i++) {
		first = h + (rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25)) + ((e & f) ^ ((~e) & g)) +
		        constants[i] + words[i];
		second = (rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
		h = g;
		g = f;
		f = e;
		e = d + first;
		d = c;
		c = b;
		b = a;
		a = first + second;
	}
	sha->state[0] += a;
	sha->state[1] += b;
	sha->state[2] += c;
	sha->state[3] += d;
	sha->state[4] += e;
	sha->state[5] += f;
	sha->state[6] += g;
	sha->state[7] += h;
}

void vaeg_sha256_initialize(VAEG_SHA256 *sha) {
	static const uint32_t initial[8] = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
	                                    0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
	memset(sha, 0, sizeof(*sha));
	memcpy(sha->state, initial, sizeof(initial));
}

void vaeg_sha256_update(VAEG_SHA256 *sha, const uint8_t *data, size_t size) {
	size_t amount;
	sha->bit_count += (uint64_t)size * 8;
	while (size != 0) {
		amount = 64 - sha->block_size;
		if (amount > size)
			amount = size;
		memcpy(sha->block + sha->block_size, data, amount);
		sha->block_size += (unsigned int)amount;
		data += amount;
		size -= amount;
		if (sha->block_size == 64) {
			transform(sha, sha->block);
			sha->block_size = 0;
		}
	}
}

void vaeg_sha256_finish(VAEG_SHA256 *sha, uint8_t identity[32]) {
	uint64_t bits = sha->bit_count;
	unsigned int i;
	sha->block[sha->block_size++] = 0x80;
	if (sha->block_size > 56) {
		memset(sha->block + sha->block_size, 0, 64 - sha->block_size);
		transform(sha, sha->block);
		sha->block_size = 0;
	}
	memset(sha->block + sha->block_size, 0, 56 - sha->block_size);
	for (i = 0; i < 8; i++)
		sha->block[63 - i] = (uint8_t)(bits >> (i * 8));
	transform(sha, sha->block);
	for (i = 0; i < 8; i++) {
		identity[i * 4] = (uint8_t)(sha->state[i] >> 24);
		identity[i * 4 + 1] = (uint8_t)(sha->state[i] >> 16);
		identity[i * 4 + 2] = (uint8_t)(sha->state[i] >> 8);
		identity[i * 4 + 3] = (uint8_t)sha->state[i];
	}
}
