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
"""Exercise the real CLI with a mock-only link; never load the DOS backend."""
import os
import re
import subprocess
import sys
from pathlib import Path

binary = Path(sys.argv[1]).resolve()
cases = 0


def run(args, answer, code, result, counts, abort=None):
    global cases
    env = os.environ.copy()
    env.pop("HF_MOCK_ABORT_ROW", None)
    if abort is not None:
        env["HF_MOCK_ABORT_ROW"] = str(abort)
    p = subprocess.run([str(binary), *args], input=answer, text=True,
                       capture_output=True, timeout=20, env=env)
    assert p.returncode == code, (args, p.returncode, p.stdout, p.stderr)
    assert re.findall(r"^HF_RESULT=(\w+)$", p.stdout, re.M) == [result], p.stdout
    actual = re.findall(r"^MOCK_COUNTS I=(\d+) O=(\d+) W=(\d+)$", p.stdout, re.M)
    assert actual == [tuple(map(str, counts))], p.stdout
    cases += 1
    return p.stdout


for args in ([], ["help"], ["info"]):
    run(args, "", 0, "HF_OK", (0, 0, 0))
# Establish each passing operation before its single-mutation negatives.
out = run(["off"], "PC98-HF\n", 0, "HF_OK", (0, 2, 0))
assert re.findall(r"^MOCK_OP .+$", out, re.M) == [
    "MOCK_OP O 00D0 0000", "MOCK_OP O 00EE 0000"]
run(["off"], "PC98-HX\n", 2, "HF_ABORTED", (0, 0, 0))
run(["off"], "", 2, "HF_ABORTED", (0, 0, 0))
run(["off"], "PC98-HF", 2, "HF_ABORTED", (0, 0, 0))
run(["off"], "PC98-HF\x00\n", 2, "HF_ABORTED", (0, 0, 0))
run(["off", "extra"], "PC98-HF\n", 3, "HF_ARGUMENT", (0, 0, 0))
run(["offf"], "PC98-HF\n", 3, "HF_ARGUMENT", (0, 0, 0))
run(["readback"], "PC98-HF\nPIXEL-READY\n", 0, "HF_OK", (18, 132, 0))
run(["readback"], "PC98-HF\nPIXEL-NO\n", 2, "HF_UNSAFE", (0, 0, 0))
run(["readback", "word"], "PC98-HF\nPIXEL-READY\nWORD-OUT\n",
    0, "HF_OK", (18, 12, 60))
run(["readback", "word"], "PC98-HF\nPIXEL-READY\nWORD-NO\n",
    2, "HF_ABORTED", (0, 0, 0))
run(["readback", "wordd"], "PC98-HF\nPIXEL-READY\nWORD-OUT\n",
    3, "HF_ARGUMENT", (0, 0, 0))
run(["pattern"], "PC98-HF\nPIXEL-READY\nDRAW-32\n", 0, "HF_OK", (0, 15360, 0))
run(["pattern"], "PC98-HF\nPIXEL-READY\nDRAW-NO\n", 2, "HF_ABORTED", (0, 0, 0))
run(["pattern", "full"], "PC98-HF\nPIXEL-READY\nFULL-FRAME\n",
    0, "HF_OK", (0, 3840000, 0))
run(["pattern", "full"], "PC98-HF\nPIXEL-READY\nFULL-NO\n",
    2, "HF_ABORTED", (0, 0, 0))
run(["pattern", "full"], "PC98-HF\nPIXEL-READY\nFULL-FRAME\n",
    2, "HF_ABORTED", (0, 0, 0), abort=0)
for args in (["init"], ["banktest"]):
    run(args, "PC98-HF\nPIXEL-READY\nYES\n", 2, "HF_UNSUPPORTED", (0, 0, 0))
print(f"PASS {cases} CLI cases; no host hardware backend linked")
