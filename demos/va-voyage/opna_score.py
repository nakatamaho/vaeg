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
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Original six-FM/three-SSG score and analytical Delta-T samples.

JSON-only preparation is retained. --nasm supplies the separate VOYOPNA.COM
integration; the legacy VOYAGE.COM soundtrack is unchanged. Guest playback
and hardware equivalence require their own validation and listening gate.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import unittest

from generate import FNUM, note

RATE = 11025
FACTORS = (57, 57, 57, 57, 77, 102, 128, 153)


class ScoreError(ValueError):
    def __init__(self, code):
        self.code = code
        super().__init__(code)


def transition(predictor, step, nibble):
    difference = ((2 * (nibble & 7) + 1) * step) // 8
    predictor += -difference if nibble & 8 else difference
    predictor = max(-32768, min(32767, predictor))
    step = max(127, min(24576, step * FACTORS[nibble & 7] // 64))
    return predictor, step


def encode(pcm):
    if len(pcm) % 64:
        raise ScoreError("M107A_PCM_ALIGNMENT")
    predictor, step = 0, 127
    nibbles = []
    for value in pcm:
        if type(value) is not int or not -32768 <= value <= 32767:
            raise ScoreError("M107A_PCM_RANGE")
        choices = [transition(predictor, step, nibble) for nibble in range(16)]
        code = min(range(16), key=lambda n: (abs(choices[n][0] - value), n))
        predictor, step = choices[code]
        nibbles.append(code)
    return bytes((nibbles[i] << 4) | nibbles[i + 1] for i in range(0, len(nibbles), 2))


def decode(data):
    predictor, step = 0, 127
    pcm = []
    for byte in data:
        for code in (byte >> 4, byte & 15):
            predictor, step = transition(predictor, step, code)
            pcm.append(predictor)
    return pcm


def percussion():
    """Analytical kick/snare/sweep, not recorded or imported audio."""
    result = {}
    rng = random.Random(1988)
    for kind, count in (("kick", 2048), ("snare", 2048), ("sweep", 4096)):
        phase = 0.0
        pcm = []
        for index in range(count):
            t = index / RATE
            fade = (1 - index / count) ** 2
            if kind == "kick":
                frequency = 45 + 150 * math.exp(-t * 36)
                phase += 2 * math.pi * frequency / RATE
                signal = math.sin(phase) * math.exp(-t * 18)
            elif kind == "snare":
                signal = (0.72 * rng.uniform(-1, 1) + 0.28 * math.sin(t * 2 * math.pi * 170)) * math.exp(-t * 22)
            else:
                phase += 2 * math.pi * (250 + 2200 * index / count) / RATE
                signal = math.sin(phase) * math.sin(math.pi * index / count)
            pcm.append(round(11000 * fade * signal))
        result[kind] = pcm
    return result


def score():
    fm = [[] for _ in range(6)]
    ssg = [[] for _ in range(3)]
    roots = (0, 8, 5, 7, 0, 8, 5, 7)
    arp = (0, 1, 2, 3, 2, 1, 0, 2, 3, 2, 1, 0, 1, 2, 3, 2)
    drums = []
    for event in range(128):
        root = roots[event // 16]
        chord = (0, 4, 7, 10) if root in (8, 7) else (0, 3, 7, 10)
        # Lower bank: bass / melody / pad; upper bank: chord / echo / shimmer.
        pitches = (note(root, 2), note(root + chord[arp[event % 16]], 4),
                   note(root + 7, 3), note(root + chord[1], 3),
                   note(root + chord[arp[(event - 2) % 16]], 4),
                   note(root + 12, 4))
        for lane, value in zip(fm, pitches):
            lane.append(value)
        # Three independent tone lanes; gated volumes/noise are driver work.
        for channel, octave in enumerate((4, 5, 3)):
            semitone = root + chord[(event + channel) % 4]
            frequency = 261.625565 * 2 ** ((semitone + 12 * (octave - 4)) / 12)
            ssg[channel].append(round(124800 / frequency))
        drums.append("kick" if event % 8 == 0 else "snare" if event % 8 == 4 else "sweep" if event % 32 == 31 else None)
    return {"version": 1, "events": 128, "fm": fm, "ssg": ssg, "adpcm": drums}


def validate_score(data):
    if data.get("version") != 1 or data.get("events") != 128:
        raise ScoreError("M107A_SCHEMA")
    if len(data.get("fm", [])) != 6 or len(data.get("ssg", [])) != 3:
        raise ScoreError("M107A_CHANNEL_COUNT")
    for lane in data["fm"] + data["ssg"]:
        if len(lane) != 128:
            raise ScoreError("M107A_EVENT_COUNT")
    if any(type(n) is not int or not 0 <= n <= 0x3fff or n & 0x7ff not in FNUM
           for lane in data["fm"] for n in lane):
        raise ScoreError("M107A_FM_RANGE")
    if any(type(n) is not int or not 1 <= n <= 4095 for lane in data["ssg"] for n in lane):
        raise ScoreError("M107A_SSG_RANGE")
    if len(data.get("adpcm", [])) != 128 or any(n not in (None, "kick", "snare", "sweep") for n in data["adpcm"]):
        raise ScoreError("M107A_ADPCM_EVENT")


class Tests(unittest.TestCase):
    def test_code_vectors(self):
        self.assertEqual(transition(0, 127, 0), (15, 127))
        self.assertEqual(transition(15, 127, 8), (0, 127))
        self.assertEqual(decode(bytes([0x08])), [15, 0])
        self.assertEqual(encode([0] * 64), bytes([0x08] * 32))

    def test_score(self):
        data = score()
        validate_score(data)
        self.assertEqual(len({tuple(lane) for lane in data["fm"]}), 6)
        self.assertEqual(len({tuple(lane) for lane in data["ssg"]}), 3)

    def test_channel_negative(self):
        data = score()
        validate_score(data)
        data["fm"].pop()
        with self.assertRaises(ScoreError) as caught:
            validate_score(data)
        self.assertEqual(caught.exception.code, "M107A_CHANNEL_COUNT")

    def test_pcm_negative(self):
        encode([0] * 64)
        pcm = [0] * 64
        pcm[0] = 32768
        with self.assertRaises(ScoreError) as caught:
            encode(pcm)
        self.assertEqual(caught.exception.code, "M107A_PCM_RANGE")

    def test_guest_channel_contract(self):
        source = (Path(__file__).parent / "src/voyage_opna.inc").read_text(encoding="utf-8")
        self.assertIn("fm_keys: db 0,1,2,4,5,6", source)
        self.assertIn("mov ax, 8029h", source)
        self.assertIn("cmp bx, 6", source)
        self.assertIn("cmp bx, 3", source)
        self.assertIn("or ah, 38h", source)
        self.assertIn("or ah, 3fh", source)
        self.assertIn("and al, 0c0h", source)
        self.assertNotIn("int 91h", source)
        self.assertNotIn("int 21h", source)

    def test_guest_wait_contract(self):
        source = (Path(__file__).parent / "src/voyage_opna.inc").read_text(encoding="utf-8")
        writer = source.split("opna_bank_write:\n", 1)[1].split("opna_ready:\n", 1)[0]
        self.assertEqual(writer.count("call opna_ready"), 2)
        self.assertIn("je .disabled", writer)
        for routine in ("opna_ready", "adpcm_ready"):
            body = source.split(routine + ":\n", 1)[1]
            self.assertIn("mov cx, 4000h", body)
            self.assertIn("cmp al, 0ffh", body)
            self.assertIn("mov byte [audio_present], 0", body)
        self.assertIn("test al, 8", source)

    def test_guest_sample_contract(self):
        source = (Path(__file__).parent / "src/voyage_opna.inc").read_text(encoding="utf-8")
        self.assertIn("mov ax, 0c201h", source)
        self.assertIn("mov ax, 6000h", source)
        self.assertIn("mov ax, 0a000h", source)
        self.assertIn("mov ax, 7f04h", source)
        self.assertIn("mov ax, 7f0ch", source)
        stop = source.split("audio_stop:\n", 1)[1].split("voice_write:\n", 1)[0]
        self.assertIn("call mute_channels", stop)
        self.assertIn("mov ax, 0100h", stop)
        self.assertIn("mov ax, 000bh", stop)
        self.assertIn("mov ax, 1f10h", stop)
        self.assertIn("mov ax, 3027h", stop)
        self.assertEqual(sum(len(encode(p)) for p in percussion().values()), 4096)

    def test_alignment_negative(self):
        pcm = [0] * 64
        encode(pcm)
        pcm.pop()
        with self.assertRaises(ScoreError) as caught:
            encode(pcm)
        self.assertEqual(caught.exception.code, "M107A_PCM_ALIGNMENT")

    def test_ssg_negative(self):
        data = score()
        validate_score(data)
        data["ssg"][0][0] = 0
        with self.assertRaises(ScoreError) as caught:
            validate_score(data)
        self.assertEqual(caught.exception.code, "M107A_SSG_RANGE")

    def test_samples(self):
        self.assertEqual(percussion(), percussion())
        for pcm in percussion().values():
            coded = encode(pcm)
            self.assertEqual(len(coded) % 32, 0)
            self.assertEqual(len(decode(coded)), len(pcm))
            rms = math.sqrt(sum((a - b) ** 2 for a, b in zip(pcm, decode(coded))) / len(pcm))
            self.assertLess(rms, 1000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--nasm", action="store_true", help="emit guest tables and incbin declarations")
    args = parser.parse_args()
    output = args.output.resolve()
    repo = Path(__file__).resolve().parents[2]
    if output == repo or repo in output.parents:
        raise ScoreError("M107A_OUTPUT_IN_TREE")
    data = score()
    validate_score(data)
    payloads = {name: encode(pcm) for name, pcm in percussion().items()}
    output.mkdir(parents=True, exist_ok=True)
    data["sample_rate_intent"] = RATE
    data["samples"] = {}
    for name, payload in payloads.items():
        (output / (name + ".adpcmb")).write_bytes(payload)
        data["samples"][name] = {"bytes": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}
    if args.nasm:
        lines = ["; Generated original OPNA score; do not edit.",
                 f"%define OPNA_DELTA_N {round(RATE * 65536 * 144 / 7987200)}"]
        for kind in ("fm", "ssg"):
            labels = [f"opna_{kind}_{i}" for i in range(len(data[kind]))]
            lines += [f"opna_{kind}_lanes: dw " + ",".join(labels)]
            for label, lane in zip(labels, data[kind]):
                lines.append(label + ":")
                for start in range(0, 128, 16):
                    lines.append("    dw " + ",".join(str(n) for n in lane[start:start + 16]))
        lines.append("opna_drum_score:")
        drum_ids = {None: 0, "kick": 1, "snare": 2, "sweep": 3}
        for start in range(0, 128, 16):
            lines.append("    db " + ",".join(str(drum_ids[n]) for n in data["adpcm"][start:start + 16]))
        lines.append("opna_sample_units:")
        offset = 0
        for payload in payloads.values():
            lines.append(f"    dw {offset // 32},{(offset + len(payload)) // 32 - 1}")
            offset += len(payload)
        if offset != 4096:
            raise ScoreError("M107B_SAMPLE_SPAN")
        lines.append("opna_samples:")
        for name in payloads:
            lines.append(f'    incbin "{name}.adpcmb"')
        lines += ["opna_samples_end:", "%if opna_samples_end-opna_samples != 4096",
                  '%error "OPNA sample span must be 4096 bytes"', "%endif"]
        (output / "opna_score.inc").write_text("\n".join(lines) + "\n", encoding="utf-8")
    (output / "opna_score.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    prefix = "M107B" if args.nasm else "M107A"
    print(f"{prefix}_SCORE_OK fm=6 ssg=3 events=128 adpcm_samples=3; generated data only")


if __name__ == "__main__":
    main()
