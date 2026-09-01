#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Focused M98z host contract tests."""

from __future__ import annotations

from pathlib import Path
import sys
import unittest

TOOLS = Path(__file__).resolve().parent
ASM = TOOLS.parent / "256" / "zundamon_orbit_256.asm"
STATUS = TOOLS.parent / "256" / "zundamon_status_table.inc"
sys.path.insert(0, str(TOOLS))
import zundamon_orbit_controls as controls  # noqa: E402


class OrbitControlTests(unittest.TestCase):
    def test_key_directions_and_case_equivalence(self) -> None:
        for key, field, expected in (
                ("A", "requested_speed_index", 4),
                ("Z", "requested_speed_index", 2),
                ("Q", "requested_distance_bias", 1),
                ("E", "requested_distance_bias", -1),
                ("W", "requested_look_level", 1),
                ("S", "requested_look_level", -1),
                ("O", "requested_radius_index", 5),
                ("P", "requested_radius_index", 3)):
            state = controls.OrbitControlState()
            self.assertTrue(state.press(key))
            self.assertEqual(getattr(state, field), expected)

    def test_bounds_saturate_without_wrap(self) -> None:
        state = controls.OrbitControlState()
        for _ in range(20):
            state.press("A")
            state.press("Q")
            state.press("W")
            state.press("O")
        self.assertEqual((state.requested_speed_index,
                          state.requested_distance_bias,
                          state.requested_look_level,
                          state.requested_radius_index), (7, 4, 4, 8))
        for _ in range(20):
            state.press("Z")
            state.press("E")
            state.press("S")
            state.press("P")
        self.assertEqual((state.requested_speed_index,
                          state.requested_distance_bias,
                          state.requested_look_level,
                          state.requested_radius_index), (0, -4, -4, 0))
        self.assertGreater(state.saturated_key_requests, 0)

    def test_snapshot_is_immutable_until_publish(self) -> None:
        state = controls.OrbitControlState()
        state.begin_snapshot()
        self.assertTrue(state.press("Q"))
        self.assertEqual(state.active_distance_bias, 0)
        with self.assertRaisesRegex(controls.ControlError, "M98Z_SNAPSHOT_PENDING"):
            state.begin_snapshot()
        state.publish()
        state.begin_snapshot()
        self.assertEqual(state.active_distance_bias, 1)

    def test_speed_accumulator_and_misses(self) -> None:
        for index, increment in enumerate(controls.SPEED_INCREMENTS_Q8):
            state = controls.OrbitControlState(requested_speed_index=index)
            state.begin_snapshot()
            before = state.lookup_phase()
            state.publish()
            self.assertEqual(state.phase_accumulator, increment % (64 * 256))
            self.assertIn(state.lookup_phase(), range(64))
            self.assertNotEqual((before, state.lookup_phase()), (99, 99))

    def test_distance_look_radius_projection(self) -> None:
        state = controls.OrbitControlState(active_distance_bias=0,
                                           active_look_level=0,
                                           active_radius_index=4)
        self.assertEqual(state.projection(-96, 48, 16), (-96, 48, 16))
        state.active_distance_bias = 4
        state.active_look_level = 4
        state.active_radius_index = 8
        self.assertEqual(state.projection(-96, 48, 28), (-144, 88, 30))

    def test_radius_round_trip_is_exact(self) -> None:
        for value in range(-96, 97):
            # The default factor is an identity operation.  Radius changes
            # always derive from the immutable base radii, so returning to
            # index 4 never accumulates a prior rounded value.
            self.assertEqual(
                controls.symmetric_round_q8(value, controls.RADIUS_FACTORS_Q8[4]),
                value)
        self.assertEqual(controls.RADIUS_FACTORS_Q8[4], 256)
        self.assertLess(
            controls.symmetric_round_q8(96, controls.RADIUS_FACTORS_Q8[0]),
            controls.symmetric_round_q8(96, controls.RADIUS_FACTORS_Q8[4]))
        self.assertLess(
            controls.symmetric_round_q8(96, controls.RADIUS_FACTORS_Q8[4]),
            controls.symmetric_round_q8(96, controls.RADIUS_FACTORS_Q8[8]))

    def test_scale_clamps_and_hud_active_state(self) -> None:
        self.assertEqual(controls.effective_scale(1, -4), 1)
        self.assertEqual(controls.effective_scale(30, 4), 30)
        state = controls.OrbitControlState()
        state.begin_snapshot()
        state.publish()
        lines = controls.format_status("IDA", 1, 4, state)
        self.assertEqual(lines[1], "IDA CNT:  4")
        self.assertIn("SPD:1.00X", lines[0])
        self.assertIn("DIST:+0 LOOK:+0 RAD:1.00X", lines[2])

    def test_guest_contract_contains_all_bounded_controls(self) -> None:
        source = ASM.read_text(encoding="utf-8")
        for token in (
                "KEY_SCAN_A", "KEY_SCAN_Z", "KEY_SCAN_Q", "KEY_SCAN_E",
                "KEY_SCAN_W", "KEY_SCAN_S", "KEY_SCAN_O", "KEY_SCAN_P",
                "apply_requested_projection_state", "scale_signed_q8",
                "active_speed_increment", "active_radius_factor",
                "screen_y_bias", "draw_hud_status", "cmp byte [paused], 0"):
            self.assertIn(token, source)
        status = STATUS.read_text(encoding="ascii")
        for token in (
                "%define HUD_STATUS_SPEED_COUNT 8",
                "%define HUD_STATUS_DISTANCE_COUNT 9",
                "%define HUD_STATUS_LOOK_COUNT 9",
                "%define HUD_STATUS_RADIUS_COUNT 9",
                "hud_status_speed_tile_pointers:",
                "hud_status_distance_tile_pointers:",
                "hud_status_look_tile_pointers:",
                "hud_status_radius_tile_pointers:"):
            self.assertIn(token, status)


if __name__ == "__main__":
    unittest.main()
