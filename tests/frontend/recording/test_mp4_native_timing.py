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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Explicit OS/private-backend integration using vaeg's actual FFV1/PCM writer.

Not automatically enabled in CI. Prove the ordinary archive/backend first, then
mutate only one video timestamp per negative case, updating its cluster CRC.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import zlib


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def elements(data, start, end):
    while start < end:
        head = start
        first = data[start]
        width = 1
        while width <= 4 and not first & (0x80 >> (width - 1)):
            width += 1
        assert width <= 4
        tag = int.from_bytes(data[start:start + width], "big")
        start += width
        first = data[start]
        width = 1
        while width <= 8 and not first & (0x80 >> (width - 1)):
            width += 1
        assert width <= 8
        size = int.from_bytes(data[start:start + width], "big") & ((1 << (7 * width)) - 1)
        start += width
        stop = end if size == (1 << (7 * width)) - 1 else start + size
        assert start <= stop <= end
        yield tag, head, start, stop
        start = stop


def change_timestamp(original, position, desired):
    segment = next(x for x in elements(original, 0, len(original)) if x[0] == 0x18538067)
    cluster = next(x for x in elements(original, segment[2], segment[3])
                   if x[0] == 0x1F43B675 and x[2] <= position < x[3])
    children = list(elements(original, cluster[2], cluster[3]))
    time = next(x for x in children if x[0] == 0xE7)
    origin = int.from_bytes(original[time[2]:time[3]], "big")
    crc = next(x for x in children if x[0] == 0xBF)
    assert crc[3] - crc[2] == 4

    def checksum(data):
        return zlib.crc32(data[cluster[2]:crc[1]] + data[crc[3]:cluster[3]])

    assert int.from_bytes(original[crc[2]:crc[3]], "little") == checksum(original)
    assert original[position] == 0x81  # writer's video track 1, one-byte VINT
    data = bytearray(original)
    data[position + 1:position + 3] = (desired - origin).to_bytes(2, "big", signed=True)
    data[crc[2]:crc[3]] = checksum(data).to_bytes(4, "little")
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--writer", required=True, type=Path)
    parser.add_argument("--worker", required=True, type=Path)
    parser.add_argument("--work", required=True, type=Path)
    parser.add_argument("--ffprobe", default="ffprobe")
    parser.add_argument("--ffmpeg", default="ffmpeg")
    args = parser.parse_args()
    args.writer = args.writer.resolve()
    args.worker = args.worker.resolve()
    args.work.mkdir(parents=True, exist_ok=False)
    args.work = args.work.resolve()
    commands = []

    def run(argv):
        commands.append([str(x) for x in argv])
        p = subprocess.run(commands[-1], capture_output=True, timeout=180)
        assert p.returncode == 0, (commands[-1], p.stdout, p.stderr)
        return p.stdout

    source = args.work / "native60.mkv"
    run([args.writer, "--mp4-fixture", source])
    identity = digest(source)
    packets = json.loads(run([args.ffprobe, "-v", "error", "-select_streams", "v",
                              "-show_packets", "-show_entries", "packet=pts,pos,duration",
                              "-of", "json", source]))["packets"]
    assert len(packets) == 60 and [p["pts"] for p in packets[:3]] == [0, 17, 33]
    decoded = json.loads(run([args.ffprobe, "-v", "error", "-select_streams", "v",
                              "-show_frames", "-show_entries", "frame=duration",
                              "-of", "json", source]))["frames"]
    assert len(decoded) == 60 and all(not f.get("duration") for f in decoded[:3])
    run([args.ffmpeg, "-v", "error", "-xerror", "-i", source, "-f", "null", "-"])
    baseline = args.work / "baseline"
    baseline.mkdir()
    movie = baseline / "native60.mkv"
    shutil.copyfile(source, movie)
    log = run([args.worker, "--export", movie])
    (baseline / "worker.log").write_bytes(log)
    assert b"MP4_RESULT state=2 code=" in log and digest(movie) == identity
    output = movie.with_suffix(".mp4")
    run([args.ffmpeg, "-v", "error", "-xerror", "-i", output, "-f", "null", "-"])
    frames = json.loads(run([args.ffprobe, "-v", "error", "-select_streams", "v",
                             "-show_frames", "-show_entries", "frame=best_effort_timestamp_time",
                             "-of", "json", output]))["frames"]
    assert len(frames) == 60
    for packet, frame in zip(packets, frames):
        assert abs(packet["pts"] / 1000 - float(frame["best_effort_timestamp_time"])) <= 0.001
    probe = json.loads(run([args.ffprobe, "-v", "error", "-show_streams", "-show_format",
                            "-of", "json", output]))
    video = next(x for x in probe["streams"] if x["codec_type"] == "video")
    audio = next(x for x in probe["streams"] if x["codec_type"] == "audio")
    assert video["codec_name"] == "h264" and (video["width"], video["height"]) == (640, 400)
    assert audio["codec_name"] == "aac" and audio["sample_rate"] == "48000" and audio["channels"] == 2
    assert abs(float(probe["format"]["duration"]) - 1) < 0.1
    cases = [{"case": "baseline", "exit": 0, "frames": 60}]
    for name, desired in (("duplicate", 17), ("backward", 16)):
        folder = args.work / name
        folder.mkdir()
        movie = folder / "native60.mkv"
        movie.write_bytes(change_timestamp(source.read_bytes(), int(packets[2]["pos"]), desired))
        changed_identity = digest(movie)
        commands.append([str(args.worker), "--export", str(movie)])
        p = subprocess.run(commands[-1], capture_output=True, timeout=180)
        (folder / "worker.log").write_bytes(p.stdout + p.stderr)
        match = re.search(rb"MP4_RESULT state=3 code=([A-Z0-9_]+)", p.stdout)
        assert p.returncode == 1 and match and match[1] == b"MP4_VIDEO_TIMESTAMP", p.stdout + p.stderr
        assert digest(movie) == changed_identity and not movie.with_suffix(".mp4").exists()
        assert not list(folder.glob("*.vaeg-tmp-*"))
        cases.append({"case": name, "exit": p.returncode, "code": match[1].decode()})
    assert digest(source) == identity
    manifest = {"writer_sha256": digest(args.writer), "worker_sha256": digest(args.worker),
                "source_sha256": identity, "output_sha256": digest(output),
                "commands": commands, "cases": cases}
    (args.work / "validation.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"result": "PASS", "cases": len(cases), "frames": 60}))


if __name__ == "__main__":
    main()
