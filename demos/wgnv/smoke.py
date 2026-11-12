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
"""Developer smoke run of a WGN-V track COM (not a vaeg test).

usage: smoke.py COM [ARGS] [--machine va|va2] [--board] [--n N]
                [--deadchip] [--expect N]

Uses the DOS of ../b3e02/smoke.py with a fake Melco WGN-V built on the
assumptions of docs/agents/tasks/M148 section 9 (not measurements):
id C3h at 51E1h+n, type 04h at 59E1h+n, index/data pairs 53E1h/53E0h,
54E1h/54E0h, 55E1h/55E0h (+n). The E0000h window (CPU E0000h-E7FFFh
with 152h bit 3) is live once 54h-pair register 0Ch = 00h and the data
bit 0 of the current 54h index is set; 54E0h bits 3-2 = 00b show the
VGA/S3 registers (3C2h-3C6h, CRTC at 3D4h/3D5h with the CR38/CR39 lock,
CR2D/CR2E = 88h/11h unless --deadchip), 10b show 32KB of VRAM.
"""
import argparse
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'b3e02'))
import smoke as b3  # noqa: E402
from unicorn import UC_MEM_WRITE  # noqa: E402


class Wgnv:
    def __init__(self, present, n, chip=True):
        self.present, self.n, self.chip = present, n, chip
        self.r53 = {}
        self.r54 = {}
        self.r55 = {}
        self.i53 = self.i54 = self.i55 = 0
        self.cr = [0] * 256
        self.cr[0x2d], self.cr[0x2e], self.cr[0x30] = 0x88, 0x11, 0xe1
        self.cri = 0
        self.vram = bytearray(0x8000)
        self.misc = 0
        self.log = []

    # interface used by ../b3e02/smoke.py hooks
    def on(self):
        return False

    def vram_addr(self, addr):
        return None

    def low_addr(self, addr):
        return None

    def live(self):
        return (self.present and self.r54.get(0x0c, 0xff) == 0x00
                and self.r54.get(self.i54, 0) & 1)

    def mode(self):
        return self.r54.get(self.i54, 0) & 0x0c

    def read(self, port):
        if not self.present or (port & 0xf) & ~1 != self.n:
            return None
        base = port & ~0xf
        lo = port & 1
        if base == 0x51e0 and lo:
            return 0xc3
        if base == 0x59e0 and lo:
            return 0x04
        for b, reg, idx in ((0x53e0, 'r53', 'i53'), (0x54e0, 'r54', 'i54'),
                            (0x55e0, 'r55', 'i55')):
            if base == b:
                if lo:
                    return getattr(self, idx)
                return getattr(self, reg).get(getattr(self, idx), 0)
        return None

    def write(self, port, v):
        if not self.present or (port & 0xf) & ~1 != self.n:
            return
        base = port & ~0xf
        lo = port & 1
        for b, reg, idx in ((0x53e0, 'r53', 'i53'), (0x54e0, 'r54', 'i54'),
                            (0x55e0, 'r55', 'i55')):
            if base == b:
                if lo:
                    setattr(self, idx, v)
                else:
                    getattr(self, reg)[getattr(self, idx)] = v

    def mem(self, off, write, value):
        if not self.live():
            return None
        if self.mode() == 0x08:
            if write:
                self.vram[off] = value
                return 0
            return self.vram[off]
        if self.mode() != 0x00 or not self.chip:
            return None
        if off == 0x3c2:
            if write:
                self.misc = value
            return 0
        if off == 0x3cc:
            return self.misc
        if off == 0x3d4:
            if write:
                self.cri = value
            return self.cri
        if off == 0x3d5:
            unlocked = self.cr[0x38] == 0x48 and self.cr[0x39] == 0xa5
            if write:
                if self.cri < 0x2d or unlocked or self.cri in (0x38, 0x39):
                    self.cr[self.cri] = value
                return 0
            if self.cri >= 0x2d and not unlocked and self.cri not in (0x38, 0x39):
                return 0xff
            return self.cr[self.cri]
        if 0x3c0 <= off < 0x3e0:
            return 0
        return None


class Dos(b3.Dos):
    def low_byte(self, mu, access, addr, value):
        if addr < 0xe0000 or not self.p152 & 0x08:
            return
        r = self.board.mem(addr - 0xe0000, access == UC_MEM_WRITE, value)
        if access != UC_MEM_WRITE:
            mu.mem_write(addr, bytes([0xff if r is None else r]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('com')
    ap.add_argument('args', nargs='?', default='')
    ap.add_argument('--machine', choices=('va', 'va2'), default='va')
    ap.add_argument('--board', action='store_true')
    ap.add_argument('--deadchip', action='store_true')
    ap.add_argument('--n', type=lambda s: int(s, 0), default=0)
    ap.add_argument('--expect', type=lambda s: int(s, 0))
    a = ap.parse_args()
    board = Wgnv(a.board or a.deadchip, a.n, not a.deadchip)
    dos = Dos(a.com, a.args, 0xffff if a.machine == 'va' else 0xfffe, board, False)
    dos.run()
    sys.stdout.write(dos.stdout.decode('ascii'))
    problems = []
    if dos.exit_code != 1 and dos.stdout != dos.log:
        problems.append('log differs from the screen')
    if any(b > 0x7e or (b < 0x20 and b not in (13, 10)) for b in dos.stdout):
        problems.append('non-printable output')
    if (dos.p152, dos.p153) != (0x00, 0x41):
        problems.append('152h/153h not restored')
    print('exit=%s' % (dos.exit_code if dos.exit_code is None else '%02X' % dos.exit_code))
    if a.expect is not None and dos.exit_code != a.expect:
        problems.append('exit code %r, expected %02X' % (dos.exit_code, a.expect))
    for p in problems:
        print('SMOKE-FAIL ' + p)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
