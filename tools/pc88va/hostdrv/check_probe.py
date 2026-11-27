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

"""Build the diagnostic COM with NASM; no guest inputs are required."""

import argparse
import hashlib
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="new output COM path in scratch")
    args = parser.parse_args()
    output = args.output.resolve()
    source = Path(__file__).with_name("hdprobe.asm")
    if output.suffix.lower() != ".com" or output.exists():
        parser.error("output must be a new .com file")
    with TemporaryDirectory(prefix="hdprobe-", dir=output.parent) as scratch:
        binary = Path(scratch) / "hdprobe.com"
        result = subprocess.run(
            ["nasm", "-f", "bin", "-Werror", "-o", str(binary), str(source)],
            capture_output=True, check=False,
        )
        if result.returncode:
            parser.exit(1, f"HDPROBE_ASSEMBLY: {result.stderr.decode(errors='replace')}")
        data = binary.read_bytes()
    if not (0 < len(data) <= 1024) or b"HDPROBE END\r\n" not in data:
        parser.exit(1, "HDPROBE_BUILD_CONTRACT: size or completion marker\n")
    with output.open("xb") as stream:
        stream.write(data)
    print(f"HDPROBE_BUILD PASS: {len(data)} bytes; SHA-256 {hashlib.sha256(data).hexdigest()}")


if __name__ == "__main__":
    main()
