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

"""Tests for the M101 probe output parsers."""

from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "zex"))

import probe_results as pr  # noqa: E402

FLAGPRB = (
    "P1 IN A=FF F=00 BC=0000 DE=0000 HL=0000 OUT A=0F F=04 BC=0000 DE=0000 HL=0000\r\n"
    "P7a IN A=00 F=00 BC=FFFF DE=0000 HL=0000 OUT A=FF F=D7 BC=FFD7 DE=0000 HL=0000\r\n"
    "# P1 AND 0Fh: ZILOG A=0F F=1C; UPD9002 A=0F F=04\r\n"
    "# P7a PUSH BC/POP AF/PUSH AF/POP BC: ZILOG C=FF; UPD9002 C=D7\r\n"
)
ZEX = (
    "Z80doc instruction exerciser\r\n"
    "aluop a,nn....................  ERROR **** crc expected:48799360 found:12967d59\r\n"
    "<daa,cpl,scf,ccf>.............  OK\r\n"
    "Tests complete\r\n"
)


class ParserTest(unittest.TestCase):
    def test_flagprb(self):
        probes, expected = pr.parse_flagprb(FLAGPRB.replace("\r\n", "\n"))
        self.assertEqual([p.ident for p in probes], ["P1", "P7a"])
        self.assertEqual(probes[1].outputs["BC"], "FFD7")
        self.assertEqual(expected["P1"]["upd9002"], {"A": "0F", "F": "04"})
        self.assertEqual(pr.expected_field(probes[1], "C"), "D7")

    def test_flagprb_rejects_unknown_line(self):
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_flagprb(FLAGPRB.replace("\r\n", "\n") + "garbage\n")
        self.assertEqual(context.exception.code, "FLAGPRB_FORMAT")

    def test_zex(self):
        groups = pr.parse_zex(ZEX.replace("\r\n", "\n"))
        self.assertEqual([pr.group_signature(g) for g in groups],
                         ["ERROR 48799360 12967d59", "OK"])
        self.assertEqual(groups[1].name, "<daa,cpl,scf,ccf>")
        self.assertEqual(pr.found_crc(groups[1], "9b4ba675"), "9b4ba675")

    def test_zex_incomplete(self):
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_zex(ZEX.replace("Tests complete", "").replace("\r\n", "\n"))
        self.assertEqual(context.exception.code, "ZEX_INCOMPLETE")

    def test_dump_layout(self):
        data = bytes(range(256)) * 128
        records = pr.dump_records(data)
        self.assertEqual(records[0x0101], (1, 1, 2, 3))
        self.assertEqual(pr.f_in(63), 0xD7)
        self.assertEqual(pr.f_in(0b100001), 0x81)

    def test_dump_size(self):
        with self.assertRaises(pr.ResultError) as context:
            pr.dump_records(b"\0" * 10)
        self.assertEqual(context.exception.code, "DUMP_SIZE")

    def test_daadump_txt_and_crc(self):
        text = "DAA.BIN 32768 940FA288\nCPL.BIN 32768 C4A07F70\n"
        self.assertEqual(pr.parse_daadump_txt(text)["CPL"], (32768, "C4A07F70"))
        self.assertEqual(pr.crc32_hex(b"123456789"), "CBF43926")

    def test_read_text_stops_at_eof_marker(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "flagprb.txt"
            path.write_bytes(FLAGPRB.encode() + b"\x1a\x1a\x1a")
            self.assertEqual(pr.read_text(Path(directory), "FLAGPRB.TXT"),
                             FLAGPRB.replace("\r\n", "\n"))


if __name__ == "__main__":
    unittest.main()
