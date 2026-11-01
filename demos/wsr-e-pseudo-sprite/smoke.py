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
"""Developer smoke run of a WSR-E track COM (not a vaeg test).

Runs the COM under Unicorn with a minimal DOS, a VRTC interrupt every N
instructions, and an optional fake WSR-E (GD542x register files with the
SR06 lock and word OUT semantics).

usage: smoke.py COM [--board] [--float self|next] [--n N] [--io]

Needs the unicorn module (pip install unicorn). An 8086-family CPU
model with a minimal DOS (INT 21h 25h 35h 3Ch 3Dh 3Eh 40h 42h 4Ch),
F000:FFFE = FFFEh, an IRQ2 every N instructions, port 152h, and:
  (default)      unconnected ports read FFh, as in vaeg
  --float self   unconnected ports return the IN instruction's own word
  --float next   unconnected ports return the word after the IN
  --board        a fake GD542x at 4nE0h (SR06 lock, CR27 = 98h assumed,
                 word OUT to 44E0h/4EE0h/54E0h = index + data)
It checks control flow, CSV output and cleanup only; timings mean nothing.
"""
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UC_HOOK_INSN, UcError
from unicorn.x86_const import *

SEG = 0x1000
BASE = SEG << 4


class Dos:
    def __init__(self, path, board, floating, n):
        self.mu = Uc(UC_ARCH_X86, UC_MODE_16)
        self.mu.mem_map(0, 0x100000)
        code = open(path, 'rb').read()
        self.mu.mem_write(BASE + 0x100, code)
        self.mu.mem_write(BASE + 2, (0xa000).to_bytes(2, 'little'))
        self.mu.mem_write(0xffffe, (0xfffe).to_bytes(2, 'little'))  # VA2/3
        self.mu.mem_write(0x500, b'\xcf')  # dummy IRET
        for v in range(256):
            self.mu.mem_write(v * 4, (0x500).to_bytes(2, 'little') + (0).to_bytes(2, 'little'))
        for r, v in ((UC_X86_REG_CS, SEG), (UC_X86_REG_DS, SEG), (UC_X86_REG_ES, SEG),
                     (UC_X86_REG_SS, SEG), (UC_X86_REG_SP, 0xfffe), (UC_X86_REG_IP, 0x100)):
            self.mu.reg_write(r, v)
        self.mu.hook_add(UC_HOOK_INTR, self.intr)
        self.mu.hook_add(UC_HOOK_INSN, self.io_in, None, 1, 0, UC_X86_INS_IN)
        self.mu.hook_add(UC_HOOK_INSN, self.io_out, None, 1, 0, UC_X86_INS_OUT)
        self.board, self.floating, self.n = board, floating, n
        self.mask = 0xbf
        self.rom_bank = 0x0000
        self.stdout, self.log = bytearray(), bytearray()
        self.log_exists = False
        self.exit_code = None
        self.ticks = 0
        self.io_log = []
        # fake GD542x
        self.sr, self.gr, self.cr = [0] * 256, [0] * 256, [0] * 256
        self.sr_i = self.gr_i = self.cr_i = 0
        self.cr[0x27] = 0x98
        self.setup = 0x18

    # --- DOS ---------------------------------------------------------------
    def flags(self, cf):
        f = self.mu.reg_read(UC_X86_REG_EFLAGS)
        self.mu.reg_write(UC_X86_REG_EFLAGS, (f | 1) if cf else (f & ~1))

    def intr(self, mu, intno, _):
        if intno != 0x21:
            raise RuntimeError('INT %02X' % intno)
        ax = mu.reg_read(UC_X86_REG_AX)
        ah, al = ax >> 8, ax & 0xff
        ds = mu.reg_read(UC_X86_REG_DS)
        dx = mu.reg_read(UC_X86_REG_DX)
        if ah == 0x35:
            v = mu.mem_read(al * 4, 4)
            mu.reg_write(UC_X86_REG_BX, v[0] | v[1] << 8)
            mu.reg_write(UC_X86_REG_ES, v[2] | v[3] << 8)
        elif ah == 0x25:
            mu.mem_write(al * 4, dx.to_bytes(2, 'little') + ds.to_bytes(2, 'little'))
        elif ah == 0x3d:
            if self.log_exists:
                mu.reg_write(UC_X86_REG_AX, 5); self.flags(False)
            else:
                mu.reg_write(UC_X86_REG_AX, 2); self.flags(True)
        elif ah == 0x3c:
            self.log_exists = True
            mu.reg_write(UC_X86_REG_AX, 5); self.flags(False)
        elif ah == 0x40:
            bx = mu.reg_read(UC_X86_REG_BX); cx = mu.reg_read(UC_X86_REG_CX)
            data = bytes(mu.mem_read((ds << 4) + dx, cx))
            (self.stdout if bx == 1 else self.log).extend(data)
            mu.reg_write(UC_X86_REG_AX, cx); self.flags(False)
        elif ah == 0x42:
            mu.reg_write(UC_X86_REG_AX, 0); mu.reg_write(UC_X86_REG_DX, 0); self.flags(False)
        elif ah == 0x3e:
            self.flags(False)
        elif ah == 0x4c:
            self.exit_code = al
            mu.emu_stop()
        else:
            raise RuntimeError('INT 21h AH=%02X' % ah)

    # --- ports -------------------------------------------------------------
    def float_value(self, port):
        ip = self.mu.reg_read(UC_X86_REG_IP)
        cs = self.mu.reg_read(UC_X86_REG_CS)
        word = (ip & ~1) if self.floating == 'self' else ((ip + 2) & ~1)
        b = self.mu.mem_read((cs << 4) + word, 2)
        return b[port & 1]

    def io_in(self, mu, port, size, _):
        v = self.read_port(port)
        if size == 2:
            v |= self.read_port(port + 1) << 8
        return v

    def read_port(self, port):
        if port == 0x18a:
            return self.mask
        if port == 0x1cb:
            return 0x00
        if port == 0x152:
            return self.rom_bank & 0xff
        if port == 0x153:
            return 0x00
        if self.board:
            if port == 0x45e0:
                v = self.sr[self.sr_i]
                if self.sr_i == 6:
                    return 0x12 if v == 0x12 else 0x0f
                return v
            if port == 0x4fe0:
                return self.gr[self.gr_i]
            if port == 0x55e0:
                return self.cr[self.cr_i]
            if port == 0x46e8:
                return self.setup
        return self.float_value(port) if self.floating else 0xff

    def io_out(self, mu, port, size, value, _ud=None):
        self.io_log.append((port, size, value))
        if size == 2 and port in (0x44e0, 0x4ee0, 0x54e0):
            self.write_port(port, value & 0xff)
            self.write_port(port + 0x100, value >> 8)
            return
        if size == 2:
            self.write_port(port, value & 0xff)
            self.write_port(port + 1, value >> 8)
            return
        self.write_port(port, value)

    def write_port(self, port, v):
        if port == 0x18a:
            self.mask = v
        elif port == 0x152:
            self.rom_bank = v
        elif self.board:
            if port == 0x44e0: self.sr_i = v
            elif port == 0x45e0: self.sr[self.sr_i] = v
            elif port == 0x4ee0: self.gr_i = v
            elif port == 0x4fe0: self.gr[self.gr_i] = v
            elif port == 0x54e0: self.cr_i = v
            elif port == 0x55e0:
                if self.cr_i != 0x27: self.cr[self.cr_i] = v
            elif port == 0x46e8: self.setup = v

    # --- run ---------------------------------------------------------------
    def irq2(self):
        mu = self.mu
        f = mu.reg_read(UC_X86_REG_EFLAGS)
        if not (f & 0x200) or (self.mask & 0x04):
            return
        v = mu.mem_read(0x0a * 4, 4)
        off, seg = v[0] | v[1] << 8, v[2] | v[3] << 8
        cs = mu.reg_read(UC_X86_REG_CS)
        if seg != cs:
            return
        ss, sp, ip = mu.reg_read(UC_X86_REG_SS), mu.reg_read(UC_X86_REG_SP), mu.reg_read(UC_X86_REG_IP)
        sp = (sp - 6) & 0xffff
        mu.mem_write((ss << 4) + sp, ip.to_bytes(2, 'little') + cs.to_bytes(2, 'little') + (f & 0xffff).to_bytes(2, 'little'))
        mu.reg_write(UC_X86_REG_SP, sp)
        mu.reg_write(UC_X86_REG_EFLAGS, f & ~0x200)
        mu.reg_write(UC_X86_REG_IP, off)
        self.ticks += 1

    def run(self, limit=50_000_000):
        executed = 0
        while self.exit_code is None and executed < limit:
            ip = self.mu.reg_read(UC_X86_REG_IP)
            cs = self.mu.reg_read(UC_X86_REG_CS)
            self.mu.emu_start(ip, 0xfffff, count=self.n)
            executed += self.n
            if self.exit_code is None:
                self.irq2()
        return executed


def main():
    args = sys.argv[1:]
    path = args[0]
    n = 1000
    if '--n' in args:
        n = int(args[args.index('--n') + 1])
    fl = args[args.index('--float') + 1] if '--float' in args else None
    d = Dos(path, '--board' in args, fl, n)
    executed = d.run()
    print('--- stdout'); print(d.stdout.decode('ascii', 'replace'), end='')
    print('--- log'); print(d.log.decode('ascii', 'replace'), end='')
    print('--- exit', d.exit_code, 'ticks', d.ticks, 'instr~', executed, 'mask', hex(d.mask), 'rom152', hex(d.rom_bank))
    v = d.mu.mem_read(0x28, 4)
    print('vector 0A', '%04X:%04X' % (v[2] | v[3] << 8, v[0] | v[1] << 8))
    if '--io' in args:
        for p in d.io_log:
            if p[0] not in (0x188,):
                print('OUT %04X size %d %04X' % p)


if __name__ == '__main__':
    main()
