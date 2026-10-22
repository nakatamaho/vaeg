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
"""Content-only audit of FFmpeg encoder registrations in a static codec archive.

No history/topology assertions: no approved historical artifacts are introduced.
Normal archive builds allow FFV1/PCM only. Local-only builds additionally require
exact libx264/AAC registrations. This is not legal/distribution approval.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess

BASE = frozenset({"ff_ffv1_encoder", "ff_pcm_s16le_encoder"})
LOCAL = frozenset({"ff_libx264_encoder", "ff_aac_encoder"})


class Invalid(ValueError):
    def __init__(self, code, detail=""):
        self.code = code
        super().__init__(f"{code}: {detail}")


def validate_symbols(symbols, local_only=False):
    # GNU/LLVM/Apple nm -g standard output, including Mach-O leading underscore.
    defined = set()
    for line in symbols.splitlines():
        match = re.search(r"(?:^|\s)([A-Za-z])\s+_?(ff_\w+_encoder)\s*$", line)
        # Registrations are global data, not helpers such as ff_init_range_encoder.
        if match and match[1] in "BDGRSCV":
            defined.add(match[2])
    if not defined:
        raise Invalid("MP4_STATIC_SYMBOLS_UNREADABLE")
    allowed = BASE | (LOCAL if local_only else frozenset())
    forbidden = defined - allowed
    if forbidden:
        raise Invalid("MP4_STATIC_ENCODER_FORBIDDEN", ",".join(sorted(forbidden)))
    missing = allowed - defined
    if missing:
        raise Invalid("MP4_STATIC_ENCODER_REQUIRED", ",".join(sorted(missing)))
    return sorted(defined)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--local-only", action="store_true")
    args = parser.parse_args()
    try:
        result = subprocess.run([args.nm, "-g", str(args.archive)], text=True,
                                capture_output=True, check=False)
        if result.returncode:
            raise Invalid("MP4_STATIC_SYMBOLS_UNREADABLE", result.stderr)
        encoders = validate_symbols(result.stdout, args.local_only)
        digest = hashlib.sha256(args.archive.read_bytes()).hexdigest()
        print(f"MP4_STATIC_ENCODERS_OK sha256={digest} encoders={','.join(encoders)}")
        return 0
    except (OSError, Invalid) as exc:
        print(str(exc) if isinstance(exc, Invalid) else f"MP4_STATIC_AUDIT_IO: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
