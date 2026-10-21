#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
"""Explicit native/private-backend integration, not automatically run in CI.

Generate a public deterministic archive out of tree. Every negative case uses a
proven passing input/backend and one mutation; assert the exact error code.
"""
import argparse
from array import array
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import time


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def wrong_video_codec(data):
    if data.count(b"V_FFV1") == 1:
        return data.replace(b"V_FFV1", b"V_VP80", 1)
    # FFmpeg 6 writes V_MS/VFW/FOURCC with a BITMAPINFOHEADER instead.
    assert b"V_MS/VFW/FOURCC" in data and data.count(b"FFV1") == 1
    offset = data.index(b"FFV1")
    size = int.from_bytes(data[offset - 16:offset - 12], "little")
    # FFmpeg includes codec extradata in biSize (82 for the pinned fixture),
    # not just the fixed 40-byte Windows BITMAPINFOHEADER structure.
    assert 40 <= size <= len(data) - (offset - 16)
    assert int.from_bytes(data[offset - 4:offset - 2], "little") == 1
    assert int.from_bytes(data[offset - 2:offset], "little") in (24, 32)
    return data[:offset] + b"VP80" + data[offset + 4:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--ffmpeg", default="ffmpeg")
    parser.add_argument("--ffprobe", default="ffprobe")
    args = parser.parse_args()
    args.worker = args.worker.resolve()
    args.work.mkdir(parents=True, exist_ok=False)
    args.work = args.work.resolve()
    fixture = args.work / "fixture.mkv"
    commands = []

    def command(argv):
        commands.append(argv)
        result = subprocess.run(argv, capture_output=True, timeout=180)
        if result.returncode:
            raise AssertionError(f"command failed {argv}: {result.stderr.decode(errors='replace')}")
        return result.stdout

    command([args.ffmpeg, "-v", "error", "-f", "lavfi", "-i", "testsrc=size=641x401:rate=60:duration=3",
             "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=44100:duration=3",
             "-vf", "format=bgra", "-c:v", "ffv1", "-level", "3", "-slices", "4",
             "-c:a", "pcm_s16le", "-ac", "2", "-threads:v", "1", "-fflags", "+bitexact",
             "-flags:v", "+bitexact", "-flags:a", "+bitexact", str(fixture)])
    identity = digest(fixture)
    cases = []

    def source(name):
        folder = args.work / name
        folder.mkdir()
        movie = folder / "録画 space ' &.mkv"
        shutil.copyfile(fixture, movie)
        assert digest(movie) == identity
        return movie

    def result(process, movie, expected, code, expected_identity=None):
        stdout, stderr = process.communicate(timeout=180)
        match = re.search(rb"^MP4_RESULT state=(\d+) code=([A-Z0-9_]*)", stdout, re.M)
        assert match, stdout + stderr
        actual = match[2].decode()
        assert process.returncode == expected, stdout + stderr
        assert actual == code, (actual, code, stdout, stderr)
        assert digest(movie) == (expected_identity or identity)
        assert not list(movie.parent.glob("*.vaeg-tmp-*")), "owned temporary output leaked"
        cases.append({"case": movie.parent.name, "exit": process.returncode, "code": actual,
                      "source_sha256": digest(movie)})
        (movie.parent / "worker.log").write_bytes(stdout + stderr)

    def run(movie, action="--export"):
        argv = [str(args.worker), action, str(movie)]
        commands.append(argv)
        return subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    baseline = source("baseline")
    result(run(baseline), baseline, 0, "")
    output = baseline.with_suffix(".mp4")
    probe = json.loads(command([args.ffprobe, "-v", "error", "-count_frames", "-show_streams",
                                "-show_format", "-of", "json", str(output)]))
    video = next(s for s in probe["streams"] if s["codec_type"] == "video")
    audio = next(s for s in probe["streams"] if s["codec_type"] == "audio")
    assert video["codec_name"] == "h264" and video["pix_fmt"] == "yuv420p", video
    assert (video["width"], video["height"]) == (640, 400), video
    assert int(video["nb_read_frames"]) == 180, video
    assert audio["codec_name"] == "aac" and audio["channels"] == 2 and audio["sample_rate"] == "48000", audio
    for stream in (video, audio):
        assert abs(float(stream["start_time"])) <= 0.025, stream
        assert abs(float(stream["duration"]) - 3.0) <= 0.05, stream
    command([args.ffmpeg, "-v", "error", "-xerror", "-err_detect", "explode", "-i", str(output), "-f", "null", "-"])
    command([args.ffmpeg, "-v", "error", "-ss", "2", "-i", str(output), "-t", "0.5", "-f", "null", "-"])
    original_pcm = command([args.ffmpeg, "-v", "error", "-i", str(baseline), "-map", "0:a:0", "-ar", "48000", "-f", "s16le", "-"])
    upload_pcm = command([args.ffmpeg, "-v", "error", "-i", str(output), "-map", "0:a:0", "-f", "s16le", "-"])
    original, upload = array("h"), array("h")
    original.frombytes(original_pcm); upload.frombytes(upload_pcm)
    assert abs(len(original) - len(upload)) <= 2048, (len(original), len(upload))
    n = min(len(original), len(upload))
    signal = sum(x*x for x in original[:n])
    noise = sum((original[i] - upload[i])**2 for i in range(n))
    snr = 10 * math.log10(signal / max(1, noise))
    assert snr > 20, f"audio timing/quality regression: SNR {snr} dB"

    source_rgb = command([args.ffmpeg, "-v", "error", "-i", str(baseline),
                          "-frames:v", "1", "-pix_fmt", "rgb24", "-f", "rawvideo", "-"])
    # Exact documented floor-coordinate nearest-neighbor contract, not swscale's
    # different center-coordinate sampling (which shifts hard pixel-art edges).
    assert len(source_rgb) == 641 * 401 * 3
    expected_rgb = bytearray(640 * 400 * 3)
    for y in range(400):
        for x in range(640):
            source_pixel = ((y * 401 // 400) * 641 + x * 641 // 640) * 3
            dest_pixel = (y * 640 + x) * 3
            expected_rgb[dest_pixel:dest_pixel + 3] = source_rgb[source_pixel:source_pixel + 3]
    actual_rgb = command([args.ffmpeg, "-v", "error", "-i", str(output), "-frames:v", "1",
                          "-pix_fmt", "rgb24", "-f", "rawvideo", "-"])
    assert len(expected_rgb) == len(actual_rgb) == 640 * 400 * 3
    picture_rmse = math.sqrt(sum((a - b)**2 for a, b in zip(expected_rgb, actual_rgb)) / len(actual_rgb))
    assert picture_rmse < 20, f"picture orientation/color regression: RMSE {picture_rmse}"

    # Passing input/backend above; only destination existence changes.
    movie = source("existing")
    target = movie.with_suffix(".mp4"); target.write_bytes(b"existing MP4 sentinel")
    target_identity = digest(target)
    result(run(movie), movie, 1, "MP4_OUTPUT_EXISTS")
    assert digest(target) == target_identity

    movie = source("symlink")
    target = movie.with_suffix(".mp4")
    target.symlink_to(movie.parent / "missing-target")
    result(run(movie), movie, 1, "MP4_OUTPUT_EXISTS")
    assert target.is_symlink() and not target.exists()

    # One mutation: cancel a real conversion after it has made progress.
    movie = source("cancel")
    result(run(movie, "--cancel"), movie, 1, "MP4_CANCELLED")
    assert not movie.with_suffix(".mp4").exists()

    # One mutation: a destination appears AFTER temporary output creation.
    movie = source("publish-race")
    target = movie.with_suffix(".mp4")
    process = run(movie)
    deadline = time.monotonic() + 60
    while not list(movie.parent.glob("*.vaeg-tmp-*")):
        assert process.poll() is None, "publication race did not reach intended branch"
        assert time.monotonic() < deadline
        time.sleep(0.001)
    with target.open("xb") as stream:
        stream.write(b"destination appeared during encoding")
    target_identity = digest(target)
    result(process, movie, 1, "MP4_OUTPUT_EXISTS")
    assert digest(target) == target_identity

    # One controlled metadata mutation: reject a non-FFV1 video codec before
    # decoder probing, independent of whatever other decoders the host installed.
    if os.name != "nt":
        movie = source("relative-protocol-prefix")
        movie = movie.rename(movie.parent / "file:expected.mkv")
        neighbor = movie.parent / "expected.mkv"
        neighbor.write_bytes(b"not the selected recording")
        argv = [str(args.worker), "--export", movie.name]
        commands.append({"argv": argv, "cwd": str(movie.parent)})
        process = subprocess.Popen(argv, cwd=movie.parent, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        result(process, movie, 0, "")
        assert neighbor.read_bytes() == b"not the selected recording"
        command([args.ffmpeg, "-v", "error", "-xerror", "-i", str(movie.with_suffix(".mp4")), "-f", "null", "-"])

    movie = source("wrong-codec")
    data = movie.read_bytes()
    movie.write_bytes(wrong_video_codec(data))
    result(run(movie), movie, 1, "MP4_INPUT_CODEC", digest(movie))

    movie = source("invalid-header")
    data = bytearray(movie.read_bytes()); data[:4] = bytes(4)
    movie.write_bytes(data)
    result(run(movie), movie, 1, "MP4_INPUT_OPEN", digest(movie))

    manifest = {"ffmpeg_version": command([args.ffmpeg, "-version"]).decode().splitlines()[0],
                "worker_sha256": digest(args.worker), "source_sha256": identity,
                "output_sha256": digest(output), "probe": probe, "audio_snr_db": snr,
                "picture_rmse": picture_rmse,
                "cases": cases, "commands": commands}
    (args.work / "validation.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    print(json.dumps({"result": "PASS", "cases": len(cases), "audio_snr_db": snr,
                      "output_sha256": digest(output)}, indent=2))


if __name__ == "__main__":
    main()
