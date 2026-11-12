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
"""Developer smoke run of a B3-E02 probe COM (not a vaeg test).

usage: smoke.py COM [ARGS] [--machine va|va2] [--board|--deadchip]
                [--needreg3 MASK] [--window b|a|both] [--bankmap]
                [--otherbank N] [--float] [--expect N]

Runs the COM under Unicorn with a minimal DOS and a fake board whose
behaviour is the assumption of docs/agents/tasks/M148 (not a hardware
measurement): board control 0FA2h/0FA3h (ID 60h), a chip that decodes
only after 0902h bit 0 is set in setup (0904h=00h), CR27 = 9Ah, SR06
lock. --deadchip has the board control but a chip that never answers;
--needreg3 MASK makes the chip answer only while board reg 03h has
every bit of MASK set. --window gives the board 1MB of VRAM seen through
SMBC=0 at B0000h when reg 01h = 10h (b), at A0000h when GR06 maps
A0000h and Misc bit 1 is set (a), or both; GR09 selects 4KB banks.
Other slot memory at A0000h-BFFFFh reads FFh. Port 1D0h = n > 0 sends
CPU 80000h-9FFFFh to the slot: with --bankmap the slot address is
n * 20000h + offset and meets the board window set by reg 01h; with
--otherbank N another board's RAM answers in bank N; otherwise FFh.
Unconnected ports read FFh (as vaeg), or with --float the code
byte that follows. It checks control flow, output and cleanup only.
"""
import argparse
import sys
from unicorn import (Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UC_HOOK_INSN,
                     UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE)
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
                               UC_X86_REG_DX, UC_X86_REG_DS, UC_X86_REG_CS,
                               UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_SP,
                               UC_X86_REG_IP, UC_X86_REG_EFLAGS,
                               UC_X86_INS_IN, UC_X86_INS_OUT)

SEG = 0x1000
BASE = SEG << 4


class Board:
    def __init__(self, present, chip, needreg3=0):
        self.present, self.chip, self.needreg3 = present, chip, needreg3
        self.idx = 0
        self.regs = {0: 0x60, 1: 0x00, 3: 0x00, 4: 0x10}
        self.setup = 0x00
        self.pos = 0x00
        self.misc = 0x00
        self.sr, self.gr, self.cr = [0] * 256, [0] * 256, [0] * 256
        self.sri = self.gri = self.cri = 0
        self.cr[0x27] = 0x9a
        self.sr[0x0f] = 0x18
        self.window = ''
        self.vram = bytearray(0x100000)

    def vram_addr(self, addr):
        """VRAM offset for a slot address, or None."""
        if not self.on():
            return None
        if 'b' in self.window and self.regs.get(1) == 0x10 and 0xb0000 <= addr < 0xc0000:
            off = addr - 0xb0000
        elif addr >= 0x100000:
            return self.match24(addr)
        elif ('a' in self.window and (self.gr[6] & 0x0c) == 0x04 and self.misc & 2
              and 0xa0000 <= addr < 0xb0000):
            off = addr - 0xa0000
        else:
            return None
        return (self.gr[9] * 4096 + off) & 0xfffff

    WINDOWS = {0x80: 0xf20000, 0xa0: 0xf00000, 0xc0: 0xf40000, 0xe0: 0xf60000,
               0x10: 0x0b0000}

    def match24(self, a24):
        """VRAM offset for a 24-bit slot address (bank path), or None."""
        a24 &= 0xffffff
        base = self.WINDOWS.get(self.regs.get(1, 0))
        if not self.on() or base is None or not base <= a24 < base + 0x10000:
            return None
        return (self.gr[9] * 4096 + a24 - base) & 0xfffff

    def on(self):
        return (self.present and self.chip and (self.pos & 1) and
                (self.regs[3] & self.needreg3) == self.needreg3)

    def crtc(self):
        return 0x0d54 if self.misc & 1 else 0x0b54

    def read(self, port):
        if not self.present:
            return None
        if port == 0x0fa2:
            return self.idx
        if port == 0x0fa3:
            return self.regs.get(self.idx, 0x00)
        if port == 0x0902 and self.setup == 0x00:
            return self.pos
        if not self.on():
            return None
        if port == 0x0c5c:
            return self.misc
        if port == 0x0c54:
            return self.sri
        if port == 0x0c55:
            if self.sri == 6:
                return 0x12 if self.sr[6] == 0x12 else 0x0f
            if self.sri > 6 and self.sr[6] != 0x12:
                return 0xff
            return self.sr[self.sri]
        if port == 0x0c5e:
            return self.gri
        if port == 0x0c5f:
            return self.gr[self.gri]
        if port == self.crtc():
            return self.cri
        if port == self.crtc() + 1:
            return self.cr[self.cri]
        if port in (0x0c52, 0x0c5a, 0x0c57, 0x0b5a, 0x0d5a):
            return 0x00
        return None

    def write(self, port, v):
        if not self.present:
            return
        if port == 0x0fa2:
            self.idx = v & 7
        elif port == 0x0fa3:
            self.regs[self.idx] = v
        elif port == 0x0904:
            self.setup = v
        elif port == 0x0902 and self.setup == 0x00:
            self.pos = v
        elif not self.on():
            return
        elif port == 0x0c52:
            self.misc = v
        elif port == 0x0c54:
            self.sri = v & 0x1f
        elif port == 0x0c55:
            self.sr[self.sri] = v
        elif port == 0x0c5e:
            self.gri = v & 0x3f
        elif port == 0x0c5f:
            self.gr[self.gri] = v
        elif port == self.crtc():
            self.cri = v & 0x3f
        elif port == self.crtc() + 1:
            self.cr[self.cri] = v


class Dos:
    def __init__(self, path, args, machine, board, floating):
        self.mu = Uc(UC_ARCH_X86, UC_MODE_16)
        self.mu.mem_map(0, 0x100000)
        self.mu.mem_write(BASE + 0x100, open(path, 'rb').read())
        tail = (' ' + args if args else '').encode() + b'\r'
        self.mu.mem_write(BASE + 0x80, bytes([len(tail) - 1]) + tail)
        self.mu.mem_write(0xffffe, machine.to_bytes(2, 'little'))
        for r, v in ((UC_X86_REG_CS, SEG), (UC_X86_REG_DS, SEG), (UC_X86_REG_ES, SEG),
                     (UC_X86_REG_SS, SEG), (UC_X86_REG_SP, 0xfffe), (UC_X86_REG_IP, 0x100)):
            self.mu.reg_write(r, v)
        self.mu.hook_add(UC_HOOK_INTR, self.intr)
        self.mu.hook_add(UC_HOOK_INSN, self.io_in, None, 1, 0, UC_X86_INS_IN)
        self.mu.hook_add(UC_HOOK_INSN, self.io_out, None, 1, 0, UC_X86_INS_OUT)
        self.mu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, self.slot_mem, None,
                         0xa0000, 0xbffff)
        self.mu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, self.bank_mem, None,
                         0x80000, 0x9ffff)
        self.p1d0 = 0
        self.bankmap = False
        self.otherbank = None
        self.other_ram = bytearray(0x20000)
        self.board, self.floating = board, floating
        self.p152, self.p153 = 0x00, 0x41
        self.stdout, self.log = bytearray(), bytearray()
        self.exit_code = None
        self.log_open = False

    def flags(self, cf):
        f = self.mu.reg_read(UC_X86_REG_EFLAGS)
        self.mu.reg_write(UC_X86_REG_EFLAGS, (f | 1) if cf else (f & ~1))

    def intr(self, mu, intno, _):
        if intno != 0x21:
            raise RuntimeError('INT %02X' % intno)
        ax = mu.reg_read(UC_X86_REG_AX)
        ah, al = ax >> 8, ax & 0xff
        ds, dx = mu.reg_read(UC_X86_REG_DS), mu.reg_read(UC_X86_REG_DX)
        if ah == 0x2a:
            mu.reg_write(UC_X86_REG_CX, 2026)
            mu.reg_write(UC_X86_REG_DX, 0x0b0c)
        elif ah == 0x2c:
            mu.reg_write(UC_X86_REG_CX, 0x0a05)
            mu.reg_write(UC_X86_REG_DX, 0x0700)
        elif ah == 0x3d:
            mu.reg_write(UC_X86_REG_AX, 2)
            self.flags(True)
        elif ah == 0x3c:
            self.log_open = True
            mu.reg_write(UC_X86_REG_AX, 5)
            self.flags(False)
        elif ah == 0x40:
            bx, cx = mu.reg_read(UC_X86_REG_BX), mu.reg_read(UC_X86_REG_CX)
            data = bytes(mu.mem_read((ds << 4) + dx, cx))
            (self.stdout if bx == 1 else self.log).extend(data)
            mu.reg_write(UC_X86_REG_AX, cx)
            self.flags(False)
        elif ah == 0x42:
            mu.reg_write(UC_X86_REG_AX, 0)
            mu.reg_write(UC_X86_REG_DX, 0)
            self.flags(False)
        elif ah == 0x3e:
            self.flags(False)
        elif ah == 0x4c:
            self.exit_code = al
            mu.emu_stop()
        else:
            raise RuntimeError('INT 21h AH=%02X' % ah)

    def io_in(self, mu, port, size, _):
        if port == 0x152:
            return self.p152
        if port == 0x153:
            return self.p153
        v = self.board.read(port)
        if v is not None:
            return v
        if self.floating:
            ip, cs = mu.reg_read(UC_X86_REG_IP), mu.reg_read(UC_X86_REG_CS)
            return mu.mem_read((cs << 4) + ip + 1, 1)[0]
        return 0xff

    def io_out(self, mu, port, size, value, _):
        value &= 0xff
        if port == 0x1d0:
            self.p1d0 = value
        elif port == 0x152:
            self.p152 = value
        elif port == 0x153:
            self.p153 = value
        else:
            self.board.write(port, value)

    def slot_mem(self, mu, access, addr, size, value, _):
        if self.p153 & 0x0f:
            return
        off = self.board.vram_addr(addr)
        if access == UC_MEM_WRITE:
            if off is not None:
                self.board.vram[off] = value & 0xff
            return
        mu.mem_write(addr, bytes([self.board.vram[off] if off is not None else 0xff]))

    def bank_mem(self, mu, access, addr, size, value, _):
        if self.p1d0 == 0:
            return
        off = addr - 0x80000
        if self.p1d0 == self.otherbank:
            if access == UC_MEM_WRITE:
                self.other_ram[off] = value & 0xff
            else:
                mu.mem_write(addr, bytes([self.other_ram[off]]))
            return
        voff = None
        if self.bankmap:
            # the board sees n * 20000h + offset; a 24-bit address >= 1MB
            a24 = (self.p1d0 << 17) + off
            voff = self.board.match24(a24) if a24 >= 0x100000 else None
            if voff is None and a24 < 0x100000:
                base = self.board.WINDOWS.get(self.board.regs.get(1, 0))
                if self.board.on() and base is not None and base <= a24 < base + 0x10000:
                    voff = (self.board.gr[9] * 4096 + a24 - base) & 0xfffff
        if access == UC_MEM_WRITE:
            if voff is not None:
                self.board.vram[voff] = value & 0xff
            return
        mu.mem_write(addr, bytes([self.board.vram[voff] if voff is not None else 0xff]))

    def run(self):
        self.mu.emu_start(BASE + 0x100, 0, count=2_000_000)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('com')
    ap.add_argument('args', nargs='?', default='')
    ap.add_argument('--machine', choices=('va', 'va2'), default='va')
    ap.add_argument('--board', action='store_true')
    ap.add_argument('--deadchip', action='store_true')
    ap.add_argument('--needreg3', type=lambda s: int(s, 0), default=0)
    ap.add_argument('--window', choices=('b', 'a', 'both'), default='')
    ap.add_argument('--bankmap', action='store_true')
    ap.add_argument('--otherbank', type=lambda s: int(s, 0))
    ap.add_argument('--float', action='store_true')
    ap.add_argument('--expect', type=lambda s: int(s, 0))
    a = ap.parse_args()
    board = Board(a.board or a.deadchip, not a.deadchip, a.needreg3)
    board.window = 'ab' if a.window == 'both' else a.window
    dos = Dos(a.com, a.args, 0xffff if a.machine == 'va' else 0xfffe, board, a.float)
    dos.bankmap, dos.otherbank = a.bankmap, a.otherbank
    if a.otherbank is not None:
        dos.other_ram[:] = bytes(i & 0xff for i in range(0x20000))
    dos.run()
    sys.stdout.write(dos.stdout.decode('ascii'))
    problems = []
    if dos.exit_code != 1 and dos.stdout != dos.log:
        problems.append('log differs from the screen')
    if any(b > 0x7e or (b < 0x20 and b not in (13, 10)) for b in dos.stdout):
        problems.append('non-printable output')
    if dos.p1d0 != 0:
        problems.append('1D0h left %02X' % dos.p1d0)
    if (dos.p152, dos.p153) != (0x00, 0x41):
        problems.append('152h/153h not restored: %02X %02X' % (dos.p152, dos.p153))
    if board.present:
        if board.idx != 0:
            problems.append('board index left %02X' % board.idx)
        if board.pos & 1:
            problems.append('chip left enabled')
        if board.sr[6] == 0x12:
            problems.append('SR06 left unlocked')
        if board.regs[3] != 0x00:
            problems.append('reg03 left %02X' % board.regs[3])
        if board.regs.get(1, 0) != 0 or board.misc != 0:
            problems.append('reg01 or Misc not restored')
        if any(board.sr[i] for i in (0, 2, 4, 7)) or any(board.gr[i] for i in range(16)):
            problems.append('SR/GR set-up not restored')
        if (board.sri, board.gri, board.cri) != (0, 0, 0):
            problems.append('VGA index left')
    print('exit=%s' % (dos.exit_code if dos.exit_code is None else '%02X' % dos.exit_code))
    if a.expect is not None and dos.exit_code != a.expect:
        problems.append('exit code %r, expected %02X' % (dos.exit_code, a.expect))
    for p in problems:
        print('SMOKE-FAIL ' + p)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
