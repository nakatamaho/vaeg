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
class HF:
    """Fake HyPER-FRAME+ on the D0-family ports (M149 task assumptions,
    not measurements). Model: D0 = A5 select byte coordinates, D0 = E1
    select word coordinates; D4/D5/D6/D7 X/Y by byte or word; D8/DA/DC
    pixel bytes into a 640x400x3 framebuffer at 0. /OFF leaves D0=00,
    EE=00 (ignored)."""
import argparse
import sys
from unicorn import (Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UC_HOOK_INSN,
                     UC_MEM_WRITE)
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
                               UC_X86_REG_DX, UC_X86_REG_DS, UC_X86_REG_CS,
                               UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_SP,
                               UC_X86_REG_IP, UC_X86_REG_EFLAGS,
                               UC_X86_INS_IN, UC_X86_INS_OUT)


SEG = 0x1000
BASE = SEG << 4


class HF:
    W, H = 640, 400

    def __init__(self, noinit, word):
        self.noinit, self.word = noinit, word
        self.control = 0x00
        self.x = self.y = 0
        self.ee = 0xff
        self.fb = bytearray(self.W * self.H * 3)
        self.io = []
        self.touched = False

    def touch(self):
        self.touched = True

    def active(self):
        # D0 selects the coordinate width: A5 = byte, E5 = single-plane,
        # E1 = word/normal. Everything that is not the stop value 00h
        # keeps the pixel path enabled (task file: the viewer always
        # leaves D0 non-zero while running; 00h is the stop sequence).
        return self.control != 0x00

    def read(self, port):
        if port == 0x0ee:
            return self.ee
        if port == 0x0d0:
            return self.control if self.touched else None
        if port not in (0x0d3, 0x0d8, 0x0da, 0x0dc):
            return None
        if not self.touched:
            return None
        if not self.active() or port not in (0x0d8, 0x0da, 0x0dc):
            return 0xff
        off = (self.y * self.W + self.x) * 3
        return self.fb[off + {0xd8: 0, 0xda: 1, 0xdc: 2}[port]]

    def write(self, port, v):
        self.touch()
        if port == 0x0ee:
            self.ee = v
            return
        if port == 0x0d3:
            return
        if port == 0x0d0:
            self.control = v
            return
        if port == 0x0d7:
            self.y = (self.y & 0xff) | v << 8
        elif port == 0x0d6:
            self.y = (self.y & 0xff00) | v
        elif port == 0x0d5:
            self.x = (self.x & 0xff) | v << 8
        elif port == 0x0d4:
            self.x = (self.x & 0xff00) | v
        elif port == 0x0d8:
            self.put(0, v)
        elif port == 0x0da:
            self.put(1, v)
        elif port == 0x0dc:
            self.put(2, v)
        self.io.append((port, v))

    # word OUT: unicorn calls io_out per byte for OUT DX,AX; low first.
    def put(self, c, v):
        if not self.active():
            return
        if self.x >= self.W or self.y >= self.H:
            return
        off = (self.y * self.W + self.x) * 3
        self.fb[off + c] = v


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
        self.hf = None
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
        v = self.hf.read(port) if self.hf is not None else None
        return v if v is not None else 0xff

    def io_out(self, mu, port, size, value, _):
        value &= 0xff
        if port in (0x152, 0x153, 0x1d0):
            return
        if self.hf is not None:
            self.hf.write(port, value)

    def run(self):
        self.mu.emu_start(BASE + 0x100, 0, count=30_000_000)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('com')
    ap.add_argument('args', nargs='?', default='')
    ap.add_argument('--machine', choices=('va', 'va2'), default='va')
    ap.add_argument('--noinit', action='store_true')
    ap.add_argument('--word', action='store_true')


    ap.add_argument('--expect', type=lambda s: int(s, 0))
    a = ap.parse_args()
    board = HF(a.noinit, a.word)
    dos = Dos(a.com, a.args, 0xffff if a.machine == 'va' else 0xfffe, board, False)
    dos.hf = board
    dos.run()
    sys.stdout.write(dos.stdout.decode('ascii'))
    problems = []
    if dos.exit_code != 1 and dos.stdout != dos.log:
        problems.append('log differs from the screen')
    if any(b > 0x7e or (b < 0x20 and b not in (13, 10)) for b in dos.stdout):
        problems.append('non-printable output')
    if (dos.p152, dos.p153) != (0x00, 0x41):
        problems.append('152h/153h not restored: %02X %02X' % (dos.p152, dos.p153))
    print('exit=%s' % (dos.exit_code if dos.exit_code is None else '%02X' % dos.exit_code))
    if 'hfv1' in a.com.lower() and dos.exit_code == 0 and not a.noinit and '/off' not in (a.args or '').lower():
        fb = board.fb
        def px(x, y):
            o = (y * 640 + x) * 3
            return tuple(fb[o:o + 3])
        checks = [(40, 10, (255, 255, 255)), (120, 10, (255, 255, 0)),
                  (200, 10, (0, 255, 255)), (280, 10, (0, 255, 0)),
                  (360, 10, (255, 0, 255)), (440, 10, (255, 0, 0)),
                  (520, 10, (0, 0, 255)), (600, 10, (0, 0, 0)),
                  (0, 350, (0, 0, 0)), (320, 350, (127, 127, 127)),
                  (639, 350, (254, 254, 254)), (639, 399, (254, 254, 254))]
        for x, y, want in checks:
            got = px(x, y)
            if got != want:
                print('FB-MISMATCH (%d,%d) got %r want %r' % (x, y, got, want))
    if a.expect is not None and dos.exit_code != a.expect:
        problems.append('exit code %r, expected %02X' % (dos.exit_code, a.expect))
    for p in problems:
        print('SMOKE-FAIL ' + p)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
