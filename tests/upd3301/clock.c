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
#include "io/upd3301.h"
#include <stdio.h>
#include <string.h>
#define CHECK(condition, code)                                                                     \
	do {                                                                                           \
		if (!(condition)) {                                                                        \
			fprintf(stderr, "%s line=%d\n", code, __LINE__);                                       \
			return 1;                                                                              \
		}                                                                                          \
	} while (0)

int main(void) {
	static const uint32_t rates[] = {1, 3993600, 4000000, 8000000, UINT32_MAX};
	static const uint32_t batches[] = {0, 1, 2, 17, 999, 65535, 1000000, UINT32_MAX};
	static const unsigned periods[2][2] = {{16270, 16020}, {17720, 18050}};
	static const unsigned syncs[2][2] = {{3755, 3505}, {1611, 1933}};
	UPD3301 a, b, saved;
	uint64_t frames, total, remaining;
	unsigned high, lines, rate, batch, i;
	for (high = 0; high < 2; high++)
		for (lines = 1; lines <= 32; lines++)
			for (rate = 0; rate < sizeof(rates) / sizeof(rates[0]); rate++) {
				const unsigned height25 = lines == 8 || lines == 16 || lines == 24 || lines == 32;
				const unsigned period = periods[high][height25];
				const uint64_t cycle = (uint64_t)rates[rate] * period;
				upd3301_reset(&a);
				a.reset[2] = (uint8_t)(lines - 1);
				CHECK(upd3301_clock_configure(&a, rates[rate], (int)high) &&
				          upd3301_clock_valid(&a) && a.clock_remaining == cycle &&
				          upd3301_cycle_us(&a) == period &&
				          upd3301_sync_us(&a) == syncs[high][height25] && upd3301_in_vsync(&a),
				      "UPD3301_CLOCK_PROFILE");
				a.interrupt_request = 2;
				saved = a;
				for (batch = 0; batch <= sizeof(batches) / sizeof(batches[0]); batch++) {
					const uint32_t ticks = batch < sizeof(batches) / sizeof(batches[0])
					                           ? batches[batch]
					                           : (uint32_t)((cycle + 999999) / 1000000);
					const uint64_t elapsed = (uint64_t)ticks * 1000000;
					const uint64_t expected = elapsed / cycle;
					a = saved;
					CHECK(upd3301_advance(&a, ticks, &frames) && frames == expected &&
					          a.frames == (uint16_t)expected &&
					          a.clock_remaining == cycle - elapsed % cycle &&
					          a.interrupt_request == (expected ? 3 : 2) && upd3301_clock_valid(&a),
					      "UPD3301_CLOCK_ADVANCE");
					CHECK(upd3301_in_vsync(&a) ==
					          (elapsed % cycle < (uint64_t)rates[rate] * syncs[high][height25]),
					      "UPD3301_CLOCK_VSYNC");
				}
				/* Same elapsed time in one batch or 65 irregular chunks; no floating
		 * rounding, frame-loop limit, or dependence on CPU dispatch batches. */
				a = saved;
				b = saved;
				CHECK(upd3301_advance(&a, UINT32_MAX, &total), "UPD3301_CLOCK_BATCH_FIXTURE");
				remaining = UINT32_MAX;
				frames = 0;
				for (i = 0; i < 64; i++) {
					uint64_t crossed;
					const uint32_t ticks = (i * 7919 + 17) % 65536;
					CHECK(upd3301_advance(&b, ticks, &crossed), "UPD3301_CLOCK_PARTITION");
					frames += crossed;
					remaining -= ticks;
				}
				{
					uint64_t crossed;
					CHECK(upd3301_advance(&b, (uint32_t)remaining, &crossed),
					      "UPD3301_CLOCK_PARTITION");
					frames += crossed;
				}
				CHECK(frames == total && !memcmp(&a, &b, sizeof(a)), "UPD3301_CLOCK_PARTITION");
			}
	/* Exact source-clock edges: sync ends at the threshold; the next frame
	 * starts on (not after) the cycle boundary. */
	for (high = 0; high < 2; high++)
		for (lines = 0; lines < 2; lines++) {
			const uint32_t sync_ticks = syncs[high][lines] * 4;
			const uint32_t cycle_ticks = periods[high][lines] * 4;
			upd3301_reset(&a);
			a.reset[2] = lines ? 15 : 9;
			CHECK(upd3301_clock_configure(&a, 4000000, (int)high), "UPD3301_CLOCK_EDGE_FIXTURE");
			CHECK(upd3301_advance(&a, sync_ticks - 1, &frames) && !frames && upd3301_in_vsync(&a),
			      "UPD3301_CLOCK_VSYNC_EDGE");
			CHECK(upd3301_advance(&a, 1, &frames) && !frames && !upd3301_in_vsync(&a),
			      "UPD3301_CLOCK_VSYNC_EDGE");
			CHECK(upd3301_advance(&a, cycle_ticks - sync_ticks - 1, &frames) && !frames,
			      "UPD3301_CLOCK_FRAME_EDGE");
			CHECK(upd3301_advance(&a, 1, &frames) && frames == 1 && a.frames == 1 &&
			          upd3301_in_vsync(&a),
			      "UPD3301_CLOCK_FRAME_EDGE");
		}
	upd3301_reset(&a);
	CHECK(upd3301_advance(&a, 17, &frames), "UPD3301_CLOCK_COMMAND_FIXTURE");
	remaining = a.clock_remaining;
	upd3301_command(&a, 0);
	upd3301_parameter_write(&a, 78);
	upd3301_parameter_write(&a, 24);
	upd3301_parameter_write(&a, 9);
	CHECK(a.clock_remaining == remaining && upd3301_cycle_us(&a) == 17720 &&
	          upd3301_clock_valid(&a),
	      "UPD3301_CLOCK_R2_CONTINUITY");
	b = a; /* Copied pending RESET and countdown, not a machine-SAV round trip. */
	CHECK(upd3301_advance(&a, 72182, &frames) && !frames && upd3301_advance(&a, 1, &frames) &&
	          frames == 1 && a.clock_remaining == UINT64_C(70880000000),
	      "UPD3301_CLOCK_R2_CONTINUITY");
	CHECK(upd3301_advance(&b, 72183, &frames) && !memcmp(&a, &b, sizeof(a)),
	      "UPD3301_CLOCK_COPY_CONTINUATION");
	CHECK(upd3301_advance(&a, 123, &frames), "UPD3301_CLOCK_COMMAND_FIXTURE");
	remaining = a.clock_remaining;
	upd3301_command(&a, 0xa0);
	CHECK(a.clock_remaining == remaining && a.frames == 1 && !a.interrupt_request,
	      "UPD3301_CLOCK_IRQ_ACK");
	for (i = 0; i < 2; i++) {
		CHECK(upd3301_advance(&a, 17, &frames), "UPD3301_CLOCK_COMMAND_FIXTURE");
		upd3301_command(&a, i ? 0xc0 : 0x20);
		CHECK(a.clock_remaining == UINT64_C(70880000000) && !a.frames && !a.interrupt_request,
		      "UPD3301_CLOCK_COMMAND_RESET");
	}
	saved = a;
	CHECK(!upd3301_clock_configure(&a, 0, 1) && !memcmp(&a, &saved, sizeof(a)),
	      "UPD3301_CLOCK_CONFIG_ATOMIC");
	CHECK(!upd3301_clock_configure(&a, 4000000, 2) && !memcmp(&a, &saved, sizeof(a)),
	      "UPD3301_CLOCK_CONFIG_ATOMIC");
	for (i = 0; i < 4; i++) {
		a = saved;
		CHECK(upd3301_clock_valid(&a), "UPD3301_CLOCK_INVALID_FIXTURE");
		if (i == 0)
			a.clock_hz = 0;
		if (i == 1)
			a.clock_remaining = 0;
		if (i == 2)
			a.clock_remaining = UINT64_MAX;
		if (i == 3)
			a.hires = 2;
		b = a;
		frames = 123;
		CHECK(!upd3301_advance(&a, 1, &frames) && frames == 123 && !memcmp(&a, &b, sizeof(a)),
		      "UPD3301_CLOCK_INVALID_ATOMIC");
	}
	puts(
	    "upd3301 clock: 320 profiles, 2880 advances, 320 partition checks, edges/commands/guards PASS");
	return 0;
}
