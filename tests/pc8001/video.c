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
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "io/pc8001_video.h"
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(condition) do { checks++; if (!(condition)) { \
	fprintf(stderr, "PC8001_VIDEO_CONTRACT line=%d\n", __LINE__); return 1; } } while (0)

static uint8_t pixels[3 * PC8001_PLANE_BYTES];

#define GEOMETRY_REQUIRE(condition, code) do { checks++; if (!(condition)) { \
	fprintf(stderr, "%s line=%d\n", code, __LINE__); return 1; } } while (0)
#define GEOMETRY_CHECK(condition) GEOMETRY_REQUIRE(condition, "M144A_N80_CRTC_GEOMETRY")

static int geometry_contract(void) {
	static const uint8_t parameters[5] = {78, 19, 9, 39, 0x53};
	unsigned pairs, mode, received, i;
	PC8001_CRTC_GEOMETRY state, restored, before;
	uint8_t wire[PC8001_CRTC_GEOMETRY_SIZE];
	for (pairs = 1; pairs <= 32; pairs++) {
		for (mode = 0; mode < 8; mode++) {
			for (received = 0; received <= 5; received++) {
				pc8001_crtc_geometry_reset(&state);
				GEOMETRY_CHECK(pc8001_crtc_geometry_valid(state.bytes, sizeof(state.bytes)));
				GEOMETRY_CHECK(pc8001_crtc_geometry_pitch(&state) == 0);
				pc8001_crtc_geometry_write(&state, 0x51, 0);
				for (i = 0; i < 5; i++)
					pc8001_crtc_geometry_write(&state, 0x50, parameters[i]);
				GEOMETRY_CHECK(pc8001_crtc_geometry_pairs(&state) == 20);
				GEOMETRY_CHECK(pc8001_crtc_geometry_pitch(&state) == 120);
				pc8001_crtc_geometry_write(&state, 0x51, 0);
				for (i = 0; i < received; i++)
					pc8001_crtc_geometry_write(&state, 0x50,
					    i == 4 ? (uint8_t)(mode * 32 + pairs - 1) : parameters[i]);
				if (received < 5)
					GEOMETRY_CHECK(pc8001_crtc_geometry_pairs(&state) == 20);
				memcpy(wire, state.bytes, sizeof(wire));
				pc8001_crtc_geometry_reset(&restored);
				GEOMETRY_CHECK(pc8001_crtc_geometry_load(&restored, wire, sizeof(wire)));
				GEOMETRY_REQUIRE(memcmp(&state, &restored, sizeof(state)) == 0,
				    "M144A_N80_CRTC_CONTINUATION");
				for (i = received; i < 5; i++) {
					const uint8_t value = i == 4 ? (uint8_t)(mode * 32 + pairs - 1) : parameters[i];
					pc8001_crtc_geometry_write(&state, 0x50, value);
					pc8001_crtc_geometry_write(&restored, 0x50, value);
				}
				GEOMETRY_CHECK(memcmp(&state, &restored, sizeof(state)) == 0);
				GEOMETRY_REQUIRE(pc8001_crtc_geometry_pairs(&state) ==
				    ((mode == 0 || mode == 2) ? pairs : 0), "M144A_N80_CRTC_PAIRS");
				GEOMETRY_REQUIRE(pc8001_crtc_geometry_pitch(&state) ==
				    ((mode == 0 || mode == 2) ? 80 + 2 * pairs : 0), "M144A_N80_CRTC_PITCH");
				/* Other ports and data outside a RESET packet have no effect. */
				before = state;
				pc8001_crtc_geometry_write(&state, 0x50, 0xff);
				pc8001_crtc_geometry_write(&state, 0x52, 0);
				pc8001_crtc_geometry_write(&state, 0x150, 0);
				GEOMETRY_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
				/* A new command cancels partial parameters, not applied geometry. */
				pc8001_crtc_geometry_write(&state, 0x51, 0);
				pc8001_crtc_geometry_write(&state, 0x50, 78);
				pc8001_crtc_geometry_write(&state, 0x51, 0x20);
				pc8001_crtc_geometry_write(&state, 0x50, 0xff);
				GEOMETRY_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
				/* Invalid packet load is atomic. */
				memcpy(wire, state.bytes, sizeof(wire));
				wire[2] = 6;
				GEOMETRY_REQUIRE(!pc8001_crtc_geometry_load(&state, wire, sizeof(wire)),
				    "M144A_N80_CRTC_STATE_GUARD");
				GEOMETRY_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
			}
		}
	}
	pc8001_crtc_geometry_reset(&state);
	GEOMETRY_CHECK(pc8001_crtc_geometry_pairs(&state) == 0);
	GEOMETRY_CHECK(!pc8001_crtc_geometry_valid(NULL, sizeof(wire)));
	GEOMETRY_CHECK(!pc8001_crtc_geometry_load(NULL, state.bytes, sizeof(state.bytes)));
	for (i = 0; i <= sizeof(wire) + 1; i++)
		GEOMETRY_CHECK(pc8001_crtc_geometry_valid(state.bytes, i) == (i == sizeof(wire)));
	for (i = 0; i < sizeof(wire); i++) {
		memcpy(wire, state.bytes, sizeof(wire));
		wire[i] = i == 0 ? 2 : i == 2 ? 6 : 1;
		GEOMETRY_CHECK(!pc8001_crtc_geometry_valid(wire, sizeof(wire)));
	}
	/* A future parameter byte cannot appear in a partially received packet. */
	for (received = 0; received < 5; received++) {
		pc8001_crtc_geometry_reset(&state);
		pc8001_crtc_geometry_write(&state, 0x51, 0);
		for (i = 0; i < received; i++)
			pc8001_crtc_geometry_write(&state, 0x50, parameters[i]);
		before = state;
		memcpy(wire, state.bytes, sizeof(wire));
		wire[3 + received] = 1;
		GEOMETRY_CHECK(!pc8001_crtc_geometry_load(&state, wire, sizeof(wire)));
		GEOMETRY_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
	}
	fprintf(stderr, "M144A_N80_CRTC_GEOMETRY: 32 counts x 8 modes x 6 packet boundaries ok\n");
	return 0;
}

int main(void) {
	unsigned address, p31, p71, sr, x, y, front, mask, a, b, plane;
	PC8001_ROM_ADDRESS mapped;
	if (geometry_contract())
		return 1;
	for (address = 0; address < 65536; address++) {
		for (sr = 0; sr < 2; sr++) {
			for (p31 = 0; p31 < 4; p31++) {
				for (p71 = 0; p71 < 2; p71++) {
					unsigned source = PC8001_ROM_RAM;
					unsigned offset = address;
					if (address < 32768 && !(p31 & 2)) {
						source = sr ? PC8001_ROM_SR : PC8001_ROM_N80;
						if (address >= 24576 && (sr ? !p71 : !(p31 & 1))) {
							source = sr ? PC8001_ROM_SR_IE : PC8001_ROM_4TH;
							offset -= 24576;
						}
					}
					mapped = pc8001_rom_address((uint16_t)address, (uint8_t)p31,
					                              (uint8_t)p71, (int)sr);
					CHECK(mapped.source == source && mapped.offset == offset);
				}
			}
		}
		for (plane = 0; plane < 4; plane++) {
			const int in_window = address >= 32768 && address <= 49151;
			CHECK(pc8001_gvram_window((uint16_t)address, plane, 0, 0) == (in_window && plane < 3));
			CHECK(pc8001_gvram_window((uint16_t)address, plane, 1, 0) == 0);
			CHECK(pc8001_gvram_window((uint16_t)address, plane, 1, 1) == in_window);
		}
	}
	/* Every packed byte pattern, every output dot, both horizontal edges. */
	for (a = 0; a < 256; a++) {
		memset(pixels, (int)a, sizeof(pixels));
		for (x = 0; x < 640; x++) {
			CHECK(pc8001_mkii_pixel(pixels, x, 199, 0) == ((a & (128U >> (x % 8))) != 0));
			CHECK(pc8001_mkii_pixel(pixels, x, 199, 1) ==
			      (((a & (128U >> (x % 8 & ~1U))) != 0) * 2U +
			       ((a & (64U >> (x % 8 & ~1U))) != 0)));
		}
	}
	/* Independent solid-colour pages cover every priority/disable/zero case. */
	for (a = 0; a < 8; a++) {
		for (b = 0; b < 8; b++) {
			for (plane = 0; plane < 3; plane++) {
				memset(pixels + plane * PC8001_PLANE_BYTES, (a & (1U << plane)) ? 255 : 0, 8192);
				memset(pixels + plane * PC8001_PLANE_BYTES + 8192, (b & (1U << plane)) ? 255 : 0, 8192);
			}
			for (front = 0; front < 2; front++) {
				for (mask = 0; mask < 4; mask++) {
					unsigned c0 = mask & 1 ? 0 : a;
					unsigned c1 = mask & 2 ? 0 : b;
					unsigned expected = front ? (c1 ? c1 : c0) : (c0 ? c0 : c1);
					for (y = 0; y < 200; y++) {
						for (x = 0; x < 640; x += 639) {
							CHECK(pc8001_sr_pixel(pixels, x, y, 1, front, (uint8_t)(mask * 2)) == expected);
						}
					}
				}
			}
		}
	}
	/* Nonuniform pages detect row stride, page offset, bit/plane order and
	 * 320-dot doubling mistakes that solid colours cannot detect. */
	memset(pixels, 0, sizeof(pixels));
	for (front = 0; front < 2; front++) {
		for (y = 0; y < 200; y++) {
			for (x = 0; x < 320; x++) {
				const unsigned colour = (x + 3 * y + 5 * front) % 8;
				for (plane = 0; plane < 3; plane++) {
					if (colour & (1U << plane)) {
						pixels[plane * 16384 + front * 8192 + y * 40 + x / 8] |=
						    (uint8_t)(128U >> (x % 8));
					}
				}
			}
		}
	}
	for (y = 0; y < 200; y++) {
		for (x = 0; x < 640; x++) {
			const unsigned c0 = (x / 2 + 3 * y) % 8;
			const unsigned c1 = (x / 2 + 3 * y + 5) % 8;
			CHECK(pc8001_sr_pixel(pixels, x, y, 1, 0, 0) == (c0 ? c0 : c1));
			CHECK(pc8001_sr_pixel(pixels, x, y, 1, 1, 0) == (c1 ? c1 : c0));
		}
	}
	memset(pixels, 0, sizeof(pixels));
	for (y = 0; y < 200; y++) {
		for (x = 0; x < 640; x++) {
			const unsigned colour = (x + y) % 8;
			for (plane = 0; plane < 3; plane++) {
				if (colour & (1U << plane)) {
					pixels[plane * 16384 + y * 80 + x / 8] |= (uint8_t)(128U >> (x % 8));
				}
			}
		}
	}
	for (y = 0; y < 200; y++) {
		for (x = 0; x < 640; x++) {
			CHECK(pc8001_sr_pixel(pixels, x, y, 0, 0, 0) == (x + y) % 8);
			CHECK(pc8001_sr_pixel(pixels, x, y, 0, 0, 2) == 0);
		}
	}
	CHECK(pc8001_sr_pixel(NULL, 0, 0, 1, 0, 0) == 0);
	CHECK(pc8001_sr_pixel(pixels, 640, 0, 1, 0, 0) == 0);
	CHECK(pc8001_sr_pixel(pixels, 0, 200, 1, 0, 0) == 0);
	CHECK(pc8001_sr_pixel(pixels, 0, 0, 1, 2, 0) == 0);
	CHECK(pc8001_mkii_pixel(NULL, 0, 0, 0) == 0);
	CHECK(pc8001_mkii_pixel(pixels, 640, 0, 1) == 0);
	CHECK(pc8001_mkii_pixel(pixels, 0, 200, 0) == 0);
	printf("PC8001_VIDEO_PRIMITIVES_PASS checks=%u; machine integration NOT_RUN\n", checks);
	return 0;
}
