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
"""Developer-only independent verification of the production H.264/PCM fixture.

Not a movie generator or production subprocess boundary. Refuses files without
its public synthetic fixture tag before printing any content digest. Does not
validate private guest assets, VLC playback, real host scheduling or live A/V.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path


def require(condition, code):
    if not condition:
        raise ValueError(code)


def verify(path):
    movie = str(path)
    info = json.loads(subprocess.check_output([
        "ffprobe", "-v", "error", "-show_streams", "-show_format", "-of", "json", movie
    ]))
    tags = {key.upper(): value for key, value in info["format"].get("tags", {}).items()}
    require(tags.get("VAEG_BUILD_ID") == "public-osr-h264-fixture", "OSR_NOT_PUBLIC_FIXTURE")
    require(tags.get("VAEG_ENCODER_PROVIDER") == "h264_videotoolbox", "OSR_ENCODER_PROVIDER")
    require("allow_sw=0;require_sw=0" in tags.get("VAEG_ENCODER_OPTIONS", ""),
            "OSR_ENCODER_HARDWARE_REQUIREMENT")
    require(len(info["streams"]) == 2, "OSR_STREAM_COUNT")
    video, audio = info["streams"]
    require(video["codec_name"] == "h264" and audio["codec_name"] == "pcm_s16le",
            "OSR_STREAM_CODECS")
    require((video["width"], video["height"]) == (640, 400), "OSR_CODED_EXTENT")
    require(video.get("color_range") == "tv" and video.get("color_space") == "bt709"
            and video.get("color_transfer") == "bt709"
            and video.get("color_primaries") == "bt709", "OSR_COLOR_METADATA")
    require(audio["sample_rate"] == "48000" and audio["channels"] == 2, "OSR_PCM_FORMAT")
    require(abs(float(info["format"]["duration"]) - 3) < .001, "OSR_MOVIE_DURATION")
    frames = json.loads(subprocess.check_output([
        "ffprobe", "-v", "error", "-select_streams", "v:0", "-show_frames",
        "-show_entries", "frame=pts_time,key_frame", "-of", "json", movie
    ]))["frames"]
    require(len(frames) == 180, "OSR_VIDEO_FRAME_COUNT")
    # Independent expected clock: frame index / declared recording rate.
    require(all(abs(float(frame["pts_time"]) - index / 60) < .001
                for index, frame in enumerate(frames)), "OSR_VIDEO_CLOCK")
    require(2 <= sum(frame["key_frame"] for frame in frames) < 10, "OSR_INTERFRAME_GOP")
    decoded = subprocess.check_output([
        "ffmpeg", "-v", "error", "-i", movie, "-map", "0:a:0", "-f", "s16le", "-"
    ])
    expected = bytearray()
    # Public PCM markers: first 0.1s of seconds 0, 1 and 2, opposite channels.
    for index in range(144000):
        value = (4000 if index // 55 % 2 else -4000) if index % 48000 < 4800 else 0
        expected.extend(struct.pack("<hh", value, -value))
    require(decoded == expected, "OSR_PCM_MARKERS_EXACT")
    seek = subprocess.check_output([
        "ffmpeg", "-v", "error", "-ss", "2", "-i", movie, "-map", "0:v:0",
        "-frames:v", "1", "-pix_fmt", "rgb24", "-f", "rawvideo", "-"
    ])
    require(len(seek) == 640 * 400 * 3, "OSR_SEEK_FRAME")
    # Lossy H.264: orientation/range bounds, NOT an RGB identity assertion.
    top = seek[(20 * 640 + 20) * 3:(20 * 640 + 20) * 3 + 3]
    bottom = seek[(380 * 640 + 20) * 3:(380 * 640 + 20) * 3 + 3]
    require(max(top) < 50 and min(bottom) > 200, "OSR_ORIENTATION_RANGE")
    print("OSR_PUBLIC_MKV_AUDIO_TIMESTAMP_SEEK_PASS samples=144000 frames=180 duration=3s")
    print("public_movie_sha256=" + hashlib.sha256(path.read_bytes()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("movie", type=Path)
    args = parser.parse_args()
    try:
        verify(args.movie)
    except ValueError as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
