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

"""Validate generated build identity and print a display or package name.

No Git-history dependencies: content and release policy can be tested with
small fixtures. This helper does not create tags, releases or archives.
"""
import argparse
import json
from pathlib import Path
import re
import sys


class IdentityError(ValueError):
    pass


def require(value, code):
    if not value:
        raise IdentityError(code)


def validate(data, tag=None):
    fields = {"release", "milestone", "commit", "short_commit", "display", "package_stem"}
    require(isinstance(data, dict) and set(data) == fields and
            all(isinstance(value, str) for value in data.values()), "RELEASE_ID_SCHEMA")
    release, milestone, commit = data["release"], data["milestone"], data["commit"]
    require(re.fullmatch(r"[0-9]{8}(?:\.[1-9][0-9]*)?|unknown", release), "RELEASE_ID_DATE")
    require(re.fullmatch(r"M(?:0|[1-9][0-9]*(?:[a-z](?:[1-9][0-9]*)?)?)(?:-dev)?", milestone),
            "RELEASE_ID_MILESTONE")
    require(commit == "unknown" or re.fullmatch(r"[0-9a-f]{40}", commit), "RELEASE_ID_COMMIT")
    short = commit[:8]
    require(data["short_commit"] == short, "RELEASE_ID_SHORT_COMMIT")
    dev = milestone.endswith("-dev")
    display = "Rel." + release + " (" + milestone + (", " + short if dev else "") + ")"
    stem = "vaeg-" + release + "-" + milestone.lower() + ("-g" + short if dev else "")
    require(data["display"] == display and data["package_stem"] == stem, "RELEASE_ID_FORMAT")
    if tag is not None:
        require(not dev, "RELEASE_ID_DEVELOPMENT")
        require(release != "unknown" and commit != "unknown", "RELEASE_ID_UNKNOWN")
        require(tag == "rel-" + release, "RELEASE_ID_TAG")
    return data


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--identity", type=Path, required=True)
    parser.add_argument("--tag", help="official release: must match date; rejects -dev/unknown")
    parser.add_argument("--field", choices=("display", "title", "package_stem", "package"), required=True)
    parser.add_argument("--platform", choices=("windows-x64", "linux-x86_64", "macos-arm64"))
    args = parser.parse_args()
    data = validate(json.loads(args.identity.read_text(encoding="utf-8")), args.tag)
    if args.field == "title":
        print("88VA Eternal Grafx " + data["display"])
    elif args.field == "package":
        require(args.platform, "RELEASE_ID_PLATFORM")
        print(data["package_stem"] + "-" + args.platform)
    else:
        print(data[args.field])


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
