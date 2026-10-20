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
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""PE inspector selection is independent of host architecture and Git history."""
import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch

path = Path(__file__).resolve().parents[3] / "tools/release/check-static-windows-imports.py"
spec = importlib.util.spec_from_file_location("static_windows_imports", path)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class InspectorSelection(unittest.TestCase):
    def test_prefers_target_inspector(self):
        with patch.object(audit.shutil, "which", return_value="/cross/bin/pe-objdump") as which:
            self.assertEqual(audit.pe_objdump(), "/cross/bin/pe-objdump")
            which.assert_called_once_with("x86_64-w64-mingw32-objdump")

    def test_native_tool_fallback(self):
        with patch.object(audit.shutil, "which", return_value=None):
            self.assertEqual(audit.pe_objdump(), "objdump")


if __name__ == "__main__":
    unittest.main()
