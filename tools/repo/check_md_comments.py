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

"""Find HTML block comments that hide Markdown content.

Usage:
  python tools/repo/check_md_comments.py --report   # list findings, exit 0
  python tools/repo/check_md_comments.py            # enforce, exit 1 on findings

GitHub renders a line that starts with "<!--" as an HTML block comment that
lasts until the first line containing "-->". A header comment that lost its
"-->" therefore hides the rest of the file, or everything up to a later
"-->" (M118: task files and the Rel.20261017 release notes rendered blank).
Findings, as "CODE path:line" with the line of the opening "<!--":

  MD_COMMENT_UNCLOSED        the comment never closes
  MD_COMMENT_SPANS_HEADING   the comment closes, but hides a heading line

An unclosed "<!--" inside a line is shown as text and is not reported;
fenced code blocks are skipped. Files under external/ are skipped.
"""
import argparse
import re
import subprocess
import sys

UNCLOSED = "MD_COMMENT_UNCLOSED"
SPANS_HEADING = "MD_COMMENT_SPANS_HEADING"
SKIP_PREFIXES = ("external/",)
FENCE_RE = re.compile(r"^ {0,3}(`{3,}|~{3,})")
BLOCK_COMMENT_RE = re.compile(r"^ {0,3}<!--")
HEADING_RE = re.compile(r"^ {0,3}#{1,6}(?:[ \t]|$)")


def find_unclosed(text):
    """Return [(code, line)] for block comments that hide content."""
    findings = []
    fence = None
    open_line = None
    hides_heading = False
    for number, line in enumerate(text.split("\n"), start=1):
        if open_line is not None:
            if HEADING_RE.match(line):
                hides_heading = True
            if "-->" in line:
                if hides_heading:
                    findings.append((SPANS_HEADING, open_line))
                open_line = None
            continue
        match = FENCE_RE.match(line)
        if fence is not None:
            if match and match.group(1)[0] == fence[0] and \
                    len(match.group(1)) >= len(fence):
                fence = None
            continue
        if match:
            fence = match.group(1)
            continue
        if BLOCK_COMMENT_RE.match(line) and "-->" not in line[line.index("<!--") + 4:]:
            open_line = number
            hides_heading = False
    if open_line is not None:
        findings.append((UNCLOSED, open_line))
    return findings


def tracked_markdown():
    out = subprocess.run(["git", "ls-files", "-z", "--", "*.md"],
                         capture_output=True, check=True)
    return [p for p in out.stdout.decode("utf-8").split("\0")
            if p and not p.startswith(SKIP_PREFIXES)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", action="store_true")
    args = ap.parse_args()
    count = 0
    for path in tracked_markdown():
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
        for code, line in find_unclosed(text):
            print(f"{code} {path}:{line}")
            count += 1
    print(f"{count} finding(s)")
    return 0 if (args.report or count == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
