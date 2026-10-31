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
#include "common/sha256.h"
#include <stdio.h>
#include <string.h>

static int matches(VAEG_SHA256 *sha, const char *expected) {
	static const char hex[] = "0123456789abcdef";
	uint8_t digest[32];
	char actual[65];
	unsigned int i;
	vaeg_sha256_finish(sha, digest);
	for (i = 0; i < 32; i++) {
		actual[2 * i] = hex[digest[i] >> 4];
		actual[2 * i + 1] = hex[digest[i] & 15];
	}
	actual[64] = 0;
	return strcmp(actual, expected) == 0;
}

int main(void) {
	static const char *messages[] = {"", "abc",
	                                 "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"};
	static const char *expected[] = {
	    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
	    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
	    "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"};
	static const size_t chunks[] = {1, 7, 64, 65, 1024};
	unsigned int i, j;
	VAEG_SHA256 sha;
	uint8_t block[1000];
	for (i = 0; i < sizeof(messages) / sizeof(messages[0]); i++) {
		for (j = 0; j < sizeof(chunks) / sizeof(chunks[0]); j++) {
			size_t pos, length = strlen(messages[i]);
			vaeg_sha256_initialize(&sha);
			vaeg_sha256_update(&sha, NULL, 0);
			for (pos = 0; pos < length;) {
				size_t count = length - pos;
				if (count > chunks[j])
					count = chunks[j];
				vaeg_sha256_update(&sha, (const uint8_t *)messages[i] + pos, count);
				pos += count;
			}
			if (!matches(&sha, expected[i]))
				return fprintf(stderr, "M142_SHA256_VECTOR\n"), 1;
		}
	}
	memset(block, 'a', sizeof(block));
	vaeg_sha256_initialize(&sha);
	for (i = 0; i < 1000; i++)
		vaeg_sha256_update(&sha, block, sizeof(block));
	if (!matches(&sha, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"))
		return fprintf(stderr, "M142_SHA256_MILLION_A\n"), 1;
	puts("sha256: PASS (16 standard-vector/streaming checks)");
	return 0;
}
