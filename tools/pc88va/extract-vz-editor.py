#!/usr/bin/env python3
"""Extract the VZ Editor 1.60 PC-88VA files from the release D88.

The input is ``VZ_VA.D88`` of the pinned VZEditor release, a PC-98 2HD
FAT12 image without a system.  The builders install the extracted files on
the PC-88VA utility media.  ``--profile fdd`` extracts the runnable editor,
its two required definition files and the two notices; ``--profile sasi``
also extracts the remaining definition files and the PC-98 manuals.  The
image and every extracted file are checked against their recorded SHA-256.

Both profiles also write ``VZVA.DEF``, the definition file the media
install: ``VZVA.COM`` started from ``\\BIN`` through ``PATH`` reads the
``.DEF`` named after itself in its own directory.  It is ``VZ.DEF`` with one
option changed, ``EM`` (use every free EMS page) to ``EM0`` (no EMS): the
utility media load EMS drivers, and VZ 1.60 for the PC-88VA stops before
drawing its screen when it opens EMS (M106, vaeg VA and VA2).  The release
notes say the PC-88VA build does not use EMS.
"""

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

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path


RELEASE = "https://github.com/nakatamaho/VZEditor/releases/tag/pc88va-v1.60"
IMAGE_SHA256 = "c8435cc34f986f2e663d31aa8a081ebd501dc33d0931a640fa4696a10517da68"

# Name on the release image -> SHA-256.  The FDD profile takes the first
# group; the SASI profile takes both.
RUNTIME_FILES = {
    "VZVA.COM": "984d750d21378d66c54d576f15b6ed9ae7cfa0cd5e27bb802acf38caaaadf1b3",
    "VZ.DEF": "7b11ff97a491e70d566cd73a37524b4d445b924c79be56fb2ef5317e1f219e44",
    "VZFL.DEF": "98aa613aaae9d4314b3548c7d78007961143306b0903940ff46721d97212bbb3",
    "VZVA.DOC": "ddf4eb9fea9f3b875368adef1bf8838b1c247339f7d32292fd1e4f04782a8143",
    "LICENSE.TXT": "8b6459dff2e99ea7983bc533b84898ce3d3915481d0a7defd2a6bbf4a6ca2495",
}
EXTRA_FILES = {
    "BLOCK.DEF": "4c982cbaf8b0d300dcc5463a679aa12a0849e5185d09c6e692c941dab663c4da",
    "CVTKEI.DEF": "0c897f43cc3c09c836e43e70101884d1309c0eb83779dc65ed414fbf9c52b05b",
    "GAME.DEF": "9e0ea77003d4bfbe3f81788aa6850a16ef283f4a4a8f5e7c7120cbf12276b7d8",
    "HELP.DEF": "e7ad663d893ce0c17c77c930d8ac7c4c2499f2175431cae3a354605a331edf83",
    "KEISEN.DEF": "d1cc158d515de395f567a6325a2e727b0954e0371e38813ab8865878ca0d9cb4",
    "KEISEN_J.DEF": "3c972bdc9846cc2dfef1421578adc5f38e6693e76d78a035f08d195eca74e94e",
    "TOOL.DEF": "a6cd07c2632daf36eb32b95e253e12e6dfadef9d6ef832a66ac428f2f24c0463",
    "VZ16.DEF": "e86d02355857955bd72610658c7e46b9d25d522c6c0838188d74e35d19fc03a3",
    "ZENHAN.DEF": "45ad8d61ce02d9b3b0fcf273e6ec932a7fa4ea0a804e1cf41415d95fe0195c20",
    "README.DOC": "61235806f8d6b27f2761ba9559b67258de02be09cd71923bb1a333a644ef9ff4",
    "VZ16.DOC": "53ea981a1f4633c1988d28fb2089d0e6b254b8b083cd155d256a61b687b16d51",
    "MAC16.DOC": "1d83157184d2a3f8eeaaf407b759ce8f6eaae0352acf082a5773efc93583a05d",
}
PROFILES = {"fdd": RUNTIME_FILES, "sasi": {**RUNTIME_FILES, **EXTRA_FILES}}
EMS_OPTION = b"\r\nEM\t\t\t;"
NO_EMS_OPTION = b"\r\nEM0\t\t;"
VZVA_DEF_SHA256 = "c31fbd01222f39bf57af5996bc2fd949ce9803f23156601a12e27f2502091bc3"


class ExtractError(Exception):
    """A deterministic input or layout error with a stable code."""


def read_sectors(image: bytes) -> dict[tuple[int, int, int], bytes]:
    """Map every (cylinder, head, record) of a D88 image to its data."""
    sectors: dict[tuple[int, int, int], bytes] = {}
    if len(image) < 0x2B0:
        raise ExtractError("M106_VZ_GEOMETRY image is too small for a D88 header")
    for track_offset in struct.unpack_from("<164I", image, 0x20):
        if track_offset == 0:
            continue
        if track_offset + 16 > len(image):
            raise ExtractError("M106_VZ_GEOMETRY track offset outside the image")
        position = track_offset
        for _ in range(struct.unpack_from("<H", image, track_offset + 4)[0]):
            if position + 16 > len(image):
                raise ExtractError("M106_VZ_GEOMETRY truncated sector header")
            cylinder, head, record = image[position:position + 3]
            stored = struct.unpack_from("<H", image, position + 14)[0]
            begin = position + 16
            if begin + stored > len(image):
                raise ExtractError("M106_VZ_GEOMETRY truncated sector data")
            sectors[(cylinder, head, record)] = image[begin:begin + stored]
            position = begin + stored
    if not sectors:
        raise ExtractError("M106_VZ_GEOMETRY image holds no sectors")
    return sectors


def extract(image: bytes, names: dict[str, str]) -> dict[str, bytes]:
    """Return the named root-directory files of a FAT12 D88 image."""
    sectors = read_sectors(image)
    per_track = max(record for _, _, record in sectors)

    def logical(index: int) -> bytes:
        cylinder, remainder = divmod(index, 2 * per_track)
        head, record = divmod(remainder, per_track)
        try:
            return sectors[(cylinder, head, record + 1)]
        except KeyError:
            raise ExtractError(
                f"M106_VZ_GEOMETRY missing sector for logical {index}") from None

    boot = logical(0)
    (bytes_per_sector, per_cluster, reserved, fat_count, root_entries,
     _total, _media, per_fat) = struct.unpack_from("<HBHBHHBH", boot, 11)
    if bytes_per_sector != len(boot) or per_cluster < 1 or fat_count < 1:
        raise ExtractError("M106_VZ_GEOMETRY unexpected boot parameter block")
    fat = b"".join(logical(reserved + index) for index in range(per_fat))
    root_start = reserved + fat_count * per_fat
    root_sectors = (root_entries * 32 + bytes_per_sector - 1) // bytes_per_sector
    root = b"".join(logical(root_start + index) for index in range(root_sectors))
    data_start = root_start + root_sectors

    def following(cluster: int) -> int:
        offset = cluster * 3 // 2
        word = struct.unpack_from("<H", fat, offset)[0]
        return (word >> 4) if cluster & 1 else (word & 0x0FFF)

    def contents(cluster: int, size: int) -> bytes:
        collected = bytearray()
        seen = set()
        while 2 <= cluster < 0x0FF8:
            if cluster in seen:
                raise ExtractError("M106_VZ_GEOMETRY loop in the FAT12 chain")
            seen.add(cluster)
            first = data_start + (cluster - 2) * per_cluster
            for index in range(per_cluster):
                collected += logical(first + index)
            cluster = following(cluster)
        if len(collected) < size:
            raise ExtractError("M106_VZ_GEOMETRY FAT12 chain is shorter than the file")
        return bytes(collected[:size])

    found: dict[str, bytes] = {}
    for offset in range(0, len(root), 32):
        entry = root[offset:offset + 32]
        if entry[0] == 0:
            break
        if entry[0] == 0xE5 or entry[11] & 0x18:
            continue
        base = entry[:8].decode("ascii", "replace").rstrip()
        extension = entry[8:11].decode("ascii", "replace").rstrip()
        name = f"{base}.{extension}" if extension else base
        if name not in names:
            continue
        found[name] = contents(struct.unpack_from("<H", entry, 26)[0],
                               struct.unpack_from("<I", entry, 28)[0])
    for name, digest in names.items():
        if name not in found:
            raise ExtractError(f"M106_VZ_MISSING_FILE {name}")
        observed = hashlib.sha256(found[name]).hexdigest()
        if observed != digest:
            raise ExtractError(f"M106_VZ_FILE_SHA256 {name} ({observed})")
    return found


def vzva_def(vz_def: bytes) -> bytes:
    """Return VZ.DEF with the EMS option line set to EM0 (see above)."""
    if vz_def.count(EMS_OPTION) != 1:
        raise ExtractError("M106_VZ_EMS_OPTION VZ.DEF has no single EM option line")
    patched = vz_def.replace(EMS_OPTION, NO_EMS_OPTION)
    observed = hashlib.sha256(patched).hexdigest()
    if observed != VZVA_DEF_SHA256:
        raise ExtractError(f"M106_VZ_FILE_SHA256 VZVA.DEF ({observed})")
    return patched


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--image", required=True, type=Path,
                        help=f"VZ_VA.D88 of {RELEASE}")
    parser.add_argument("--output", required=True, type=Path,
                        help="directory to write the extracted files into")
    parser.add_argument("--profile", choices=sorted(PROFILES), default="fdd",
                        help="which files to extract (default fdd)")
    args = parser.parse_args(argv)
    try:
        image = args.image.read_bytes()
        observed = hashlib.sha256(image).hexdigest()
        if observed != IMAGE_SHA256:
            raise ExtractError(
                f"M106_VZ_IMAGE_SHA256 {args.image} is not VZ_VA.D88 of "
                f"{RELEASE} ({observed})")
        files = extract(image, PROFILES[args.profile])
        files["VZVA.DEF"] = vzva_def(files["VZ.DEF"])
        args.output.mkdir(parents=True, exist_ok=True)
        for name, data in sorted(files.items()):
            (args.output / name).write_bytes(data)
    except (ExtractError, OSError) as error:
        parser.exit(1, f"error: {error}\n")
    print(f"Extracted {len(files)} VZ Editor files into {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
