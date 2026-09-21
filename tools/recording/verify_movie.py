#!/usr/bin/env python3
"""Verify a recording-v1 Matroska fixture with FFmpeg command-line tools."""

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
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys


def fail(message: str) -> "NoReturn":
    raise SystemExit(f"verify_movie: {message}")


def run_json(tool: str, *arguments: str) -> dict:
    command = [tool, *arguments]
    try:
        completed = subprocess.run(command, check=True, capture_output=True)
    except (OSError, subprocess.CalledProcessError) as error:
        fail(f"command failed: {' '.join(command)} ({error})")
    try:
        return json.loads(completed.stdout.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        fail(f"invalid JSON from {' '.join(command)}: {error}")


def run_bytes(tool: str, *arguments: str) -> bytes:
    command = [tool, *arguments]
    try:
        completed = subprocess.run(command, check=True, capture_output=True)
    except (OSError, subprocess.CalledProcessError) as error:
        fail(f"command failed: {' '.join(command)} ({error})")
    return completed.stdout


def integer_field(value: object, name: str) -> int:
    if not isinstance(value, int) or value < 0:
        fail(f"manifest field {name} is not a non-negative integer")
    return value


def verify_packets(probe: dict) -> None:
    packets = probe.get("packets")
    if not isinstance(packets, list) or not packets:
        fail("packet listing is empty")
    last_pts: dict[int, int] = {}
    stream_seen: set[int] = set()
    for packet in packets:
        try:
            stream_index = int(packet["stream_index"])
            pts = int(packet["pts"])
            dts = int(packet["dts"])
        except (KeyError, TypeError, ValueError) as error:
            fail(f"packet is missing a numeric timestamp field: {error}")
        duration = packet.get("duration")
        if duration is not None:
            try:
                duration = int(duration)
            except (TypeError, ValueError) as error:
                fail(f"packet duration is not numeric: {error}")
            if duration <= 0:
                fail("packet has a non-positive duration")
        elif packet.get("codec_type") == "audio":
            fail("audio packet is missing its duration")
        if pts < 0 or dts < 0:
            fail("packet has a negative timestamp")
        if stream_index in last_pts and pts < last_pts[stream_index]:
            fail(f"stream {stream_index} packet timestamps regress")
        last_pts[stream_index] = pts
        stream_seen.add(stream_index)
    if stream_seen != {0, 1}:
        fail(f"expected packet streams 0 and 1, got {sorted(stream_seen)}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--movie", type=pathlib.Path, required=True)
    parser.add_argument("--manifest", type=pathlib.Path, required=True)
    parser.add_argument("--expected-rgb", type=pathlib.Path, required=True)
    parser.add_argument("--expected-pcm", type=pathlib.Path, required=True)
    parser.add_argument("--ffprobe", default="ffprobe")
    parser.add_argument("--ffmpeg", default="ffmpeg")
    args = parser.parse_args()

    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"cannot read manifest: {error}")
    if manifest.get("format") != "recording-v1":
        fail("manifest format is not recording-v1")
    width = integer_field(manifest.get("video_width"), "video_width")
    height = integer_field(manifest.get("video_height"), "video_height")
    frame_count = integer_field(manifest.get("video_frames"), "video_frames")
    sample_rate = integer_field(manifest.get("audio_sample_rate_hz"), "audio_sample_rate_hz")
    sample_frames = integer_field(manifest.get("audio_sample_frames"), "audio_sample_frames")

    streams = run_json(args.ffprobe, "-v", "error", "-show_format", "-show_streams", "-of", "json", str(args.movie))
    format_info = streams.get("format", {})
    if "matroska" not in str(format_info.get("format_name", "")):
        fail("container is not Matroska")
    stream_list = streams.get("streams", [])
    if len(stream_list) != 2:
        fail(f"expected exactly two streams, got {len(stream_list)}")
    video = next((item for item in stream_list if item.get("codec_type") == "video"), None)
    audio = next((item for item in stream_list if item.get("codec_type") == "audio"), None)
    if video is None or audio is None:
        fail("expected one video and one audio stream")
    if video.get("codec_name") != "ffv1" or video.get("width") != width or video.get("height") != height:
        fail("video codec or dimensions do not match the manifest")
    if audio.get("codec_name") != "pcm_s16le" or audio.get("sample_rate") != str(sample_rate):
        fail("audio codec or sample rate does not match the manifest")
    if audio.get("channels") != 2 or audio.get("bits_per_sample") != 16:
        fail("audio is not stereo 16-bit PCM")
    tags = format_info.get("tags", {})
    if tags.get("VAEG_RECORDING_FORMAT") != "recording-v1" or tags.get("VAEG_RECORDING_OSD") != "included":
        fail("required public recording tags are missing")

    packet_probe = run_json(args.ffprobe, "-v", "error", "-show_packets", "-of", "json", str(args.movie))
    verify_packets(packet_probe)

    decoded_rgb = run_bytes(
        args.ffmpeg,
        "-v",
        "error",
        "-xerror",
        "-i",
        str(args.movie),
        "-map",
        "0:v:0",
        "-an",
        "-fps_mode",
        "passthrough",
        "-pix_fmt",
        "rgb24",
        "-f",
        "rawvideo",
        "pipe:1",
    )
    expected_rgb = args.expected_rgb.read_bytes()
    expected_pcm = args.expected_pcm.read_bytes()
    if len(decoded_rgb) != width * height * 3 * frame_count:
        fail(f"decoded RGB size does not contain {frame_count} complete frames")
    if decoded_rgb != expected_rgb:
        fail("decoded RGB bytes differ from the independent expected file")

    decoded_pcm = run_bytes(
        args.ffmpeg,
        "-v",
        "error",
        "-xerror",
        "-i",
        str(args.movie),
        "-map",
        "0:a:0",
        "-vn",
        "-c:a",
        "pcm_s16le",
        "-f",
        "s16le",
        "pipe:1",
    )
    if len(decoded_pcm) != sample_frames * 2 * 2:
        fail(f"decoded PCM size does not contain {sample_frames} stereo frames")
    if decoded_pcm != expected_pcm:
        fail("decoded PCM bytes differ from the independent expected file")
    print(f"PASS: {args.movie} ({frame_count} video frames, {sample_frames} audio frames)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
