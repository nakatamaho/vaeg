#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""Validate the reviewable M75 SCSI controller contract in the source tree."""

from __future__ import annotations

import argparse
import pathlib
import sys


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


def validate(root: pathlib.Path) -> None:
    scsicmd = (root / "cbus" / "scsicmd.c").read_text(encoding="utf-8")
    scsiio = (root / "cbus" / "scsiio.c").read_text(encoding="utf-8")
    scsiio_h = (root / "cbus" / "scsiio.h").read_text(encoding="utf-8")

    for opcode, name in (("0x00", "TEST UNIT READY"),
                         ("0x12", "INQUIRY"),
                         ("0x25", "READ CAPACITY"),
                         ("0x1a", "MODE SENSE")):
        require(scsicmd, f"case {opcode}", f"{name} CDB")
    for phase in ("SCSIPH_COMMAND", "SCSIPH_DATAIN", "SCSIPH_DATAOUT",
                  "SCSIPH_STATUS", "SCSIPH_MSGIN"):
        require(scsicmd, f"case {phase}", f"{phase} transition")
    require(scsicmd, "REG8 scsicmd_transinfo(REG8 id)",
            "phase-aware transfer entry")
    require(scsicmd, "scsicmd_putbe32", "big-endian response encoding")
    require(scsiio_h, "UINT\tcmdpos;", "transfer length state slot")
    require(scsiio_h, "BYTE\treserved[2][0x2000];",
            "serialized board-ROM padding")
    require(scsiio, "case SCSICMD_TRANS_INFO:",
            "controller TRANSFER INFO dispatch")
    require(scsiio, "scsiio.rddatpos >= scsiio.cmdpos",
            "REQ/ACK data completion")
    require(scsiio, "static REG8 scsiio_auxstatus(void)",
            "WD33C93 auxiliary-status composition")
    require(scsiio, "static void scsiio_data_write(REG8 dat)",
            "fixed DATA-window write helper")
    require(scsiio, "static REG8 scsiio_data_read(void)",
            "fixed DATA-window read helper")
    require(scsiio, "scsiio.port != SCSICTR_CMD && scsiio.port != SCSICTR_DATA",
            "COMMAND/DATA fixed-window address behavior")
    require(scsiio, "SCSI_AUX_DBR",
            "DBR auxiliary-status bit")
    require(scsiio, "SCSI_AUX_CIP",
            "CIP auxiliary-status bit")
    require(scsiio, "SCSI_AUX_BSY",
            "BSY auxiliary-status bit")
    require(scsiio, "SCSI_C4_DMER",
            "DMER set/reset definition")
    require(scsiio, "hardware-pending",
            "unsupported NEC register warning")
    require(scsiio, "case SCSICTR_DATA:",
            "AR19 fixed DATA window")
    require(scsiio, "case SCSICTR_STATUS:",
            "AR17 status access")
    require(scsiio, "scsiio.port++;\n\t\t\treturn(scsiio.scsistatus)",
            "AR17 auto-increment to COMMAND")
    require(scsiio, "scsiio.membank & 4",
            "IRE1 IRQ gate")
    require(scsiio, "scsi_command_phase_pending",
            "post-SELECT COMMAND request latch")
    require(scsiio, "scsiintr(0x11)",
            "SELECT completion CSR")
    require(scsiio, "scsiintr(0x8a)",
            "deferred COMMAND-phase CSR")
    require(scsiio, "M75c1 holds Transfer Info at COMMAND phase",
            "M75c1 Transfer Info boundary")
    require(scsiio, "SCSI_AUX_LCI | SCSI_AUX_BSY",
            "LCI and BSY auxiliary-status definitions")
    require(scsiio, "SCSI_AUX_PE | SCSI_AUX_DBR",
            "PE and DBR auxiliary-status definitions")
    require(scsiio, "reserved register range",
            "undefined register warning")
    require(scsiio, "case SCSICTR_PKGID:", "AR32 package-id audit")
    require(scsiio, "case SCSICTR_FIFO_CTRL:", "AR34 FIFO audit")
    require(scsiio, "case SCSICTR_FIFO_STATUS:", "AR35 FIFO audit")
    for field in ("scsi_csr_latched", "scsi_csr_event_active",
                  "scsi_csr_pending", "scsi_csr_pending_status"):
        require(scsiio, field, f"single-depth CSR latch field {field}")
    require(scsiio, "if (!scsi_csr_event_active && !scsi_csr_latched)",
            "CSR event admission")
    require(scsiio, "else if (!scsi_csr_pending)",
            "one-entry CSR pending slot")
    require(scsiio, "if (scsi_csr_latched)",
            "CSR consume on status read")
    require(scsiio, "scsi_csr_latched = FALSE",
            "CSR latch release")
    require(scsiio, "scsiio.resent = (2 << 3)", "VA IRQ6 default")
    require(scsiio, "iocoreva_attachinp(0x0cc6, scsiio_icc6)",
            "VA data-port input mapping")
    require(scsiio, "iocoreva_attachout(0x0cc6, scsiio_occ6)",
            "VA data-port output mapping")
    if "scsiio.bios" in scsiio or "mem + 0xd2000" in scsiio:
        raise AssertionError("SCSI board ROM must remain detached")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path.cwd())
    args = parser.parse_args()
    try:
        validate(args.root.resolve())
    except (AssertionError, OSError) as error:
        print(f"M75_SCSI_CONTROLLER_FAIL: {error}", file=sys.stderr)
        return 1
    print("M75_SCSI_CONTROLLER_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
