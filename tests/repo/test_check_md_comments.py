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

"""check_md_comments.py: content checks and one temporary-repo run."""

import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

CHECKER = Path(__file__).resolve().parents[2] / "tools" / "repo" / "check_md_comments.py"
SPEC = importlib.util.spec_from_file_location("check_md_comments", CHECKER)
check = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check)

# Passes every check: a closed header comment, an inline comment, an
# unclosed inline marker (shown as text), a block comment inside a fence,
# and a one-line block comment.
GOOD = (
    "<!--\n"  # 1
    "Copyright (c) 2026 Nakata Maho\n"
    "-->\n"
    "\n"
    "# Title\n"  # 5
    "\n"
    "Text <!-- an inline note --> and a literal <!-- marker.\n"
    "\n"
    "```html\n"
    "<!-- open in an example\n"  # 10
    "```\n"
    "\n"
    "## Section\n"
    "\n"
    "<!-- a block note -->\n"  # 15
)


class FindUnclosedTest(unittest.TestCase):
    def test_good_fixture_passes(self):
        self.assertEqual(check.find_unclosed(GOOD), [])

    def mutate(self, old, new):
        text = GOOD.replace(old, new, 1)
        self.assertNotEqual(text, GOOD)
        return check.find_unclosed(text)

    def test_header_without_close_hides_the_headings(self):
        # the M118 failure: the header runs on to the next "-->"
        self.assertEqual(self.mutate("Maho\n-->\n", "Maho\n"),
                         [(check.SPANS_HEADING, 1)])

    def test_block_comment_left_open_to_the_end(self):
        self.assertEqual(self.mutate("a block note -->", "a block note"),
                         [(check.UNCLOSED, 15)])

    def test_comment_outside_a_fence_is_a_block_comment(self):
        self.assertEqual(self.mutate("```html\n", ""),
                         [(check.SPANS_HEADING, 9)])


def git(repo, *args):
    subprocess.run(["git", *args], cwd=repo, check=True, capture_output=True)


class CheckerRunTest(unittest.TestCase):
    def test_tracked_files_and_exit_status(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            git(repo, "init", "-q")
            (repo / "good.md").write_bytes(GOOD.encode("utf-8"))
            (repo / "external").mkdir()
            (repo / "external" / "vendored.md").write_bytes(b"<!-- open\n")
            git(repo, "add", "-A")
            run = [sys.executable, str(CHECKER)]
            result = subprocess.run(run, cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            (repo / "bad.md").write_bytes(b"<!--\nheader\n\n# Title\n")
            git(repo, "add", "-A")
            result = subprocess.run(run, cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stdout)
            self.assertIn(f"{check.UNCLOSED} bad.md:1", result.stdout)
            self.assertNotIn("vendored.md", result.stdout)
            result = subprocess.run(run + ["--report"], cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout)


if __name__ == "__main__":
    unittest.main()
