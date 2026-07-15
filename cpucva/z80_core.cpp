/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "z80_core.h"

#include "z80_legacy_state.h"
#include "z80.hpp"

#include <cstring>
#include <limits>
#include <new>

namespace {

constexpr std::uint8_t kIff1 = 0x01;
constexpr std::uint8_t kIff2 = 0x04;
constexpr std::uint8_t kIffNmi = 0x40;
constexpr std::uint8_t kIffHalt = 0x80;

std::uint16_t MakeWord(std::uint8_t high, std::uint8_t low) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(high) << 8) | low);
}

void SplitWord(std::uint16_t value, std::uint8_t *high,
               std::uint8_t *low) {
    *high = static_cast<std::uint8_t>(value >> 8);
    *low = static_cast<std::uint8_t>(value);
}

std::int32_t SignedFromBits(std::uint32_t value) {
    if (value <= static_cast<std::uint32_t>(
                     std::numeric_limits<std::int32_t>::max())) {
        return static_cast<std::int32_t>(value);
    }
    return static_cast<std::int32_t>(
        static_cast<std::int64_t>(value) - (INT64_C(1) << 32));
}

std::int32_t AddClockDelta(std::int32_t balance, std::uint32_t delta) {
    return SignedFromBits(static_cast<std::uint32_t>(balance) + delta);
}

Z80Reg ExportRegisters(const Z80::Register &source) {
    Z80Reg result{};
    result.af = MakeWord(source.pair.A, source.pair.F);
    result.hl = MakeWord(source.pair.H, source.pair.L);
    result.de = MakeWord(source.pair.D, source.pair.E);
    result.bc = MakeWord(source.pair.B, source.pair.C);
    result.ix = source.IX;
    result.iy = source.IY;
    result.sp = source.SP;
    result.r_af = MakeWord(source.back.A, source.back.F);
    result.r_hl = MakeWord(source.back.H, source.back.L);
    result.r_de = MakeWord(source.back.D, source.back.E);
    result.r_bc = MakeWord(source.back.B, source.back.C);
    result.pc = source.PC;
    result.ireg = source.I;
    result.rreg = source.R & 0x7f;
    result.rreg7 = source.R & 0x80;
    result.intmode = source.interrupt & 0x03;
    result.iff1 = (source.IFF & kIff1) != 0;
    result.iff2 = (source.IFF & kIff2) != 0;
    return result;
}

void ImportRegisters(const Z80Reg &source, bool halted,
                     bool ei_inhibited, Z80::Register *destination) {
    std::memset(destination, 0, sizeof(*destination));
    SplitWord(source.af, &destination->pair.A, &destination->pair.F);
    SplitWord(source.hl, &destination->pair.H, &destination->pair.L);
    SplitWord(source.de, &destination->pair.D, &destination->pair.E);
    SplitWord(source.bc, &destination->pair.B, &destination->pair.C);
    SplitWord(source.r_af, &destination->back.A, &destination->back.F);
    SplitWord(source.r_hl, &destination->back.H, &destination->back.L);
    SplitWord(source.r_de, &destination->back.D, &destination->back.E);
    SplitWord(source.r_bc, &destination->back.B, &destination->back.C);
    destination->PC = source.pc;
    destination->SP = source.sp;
    destination->IX = source.ix;
    destination->IY = source.iy;
    destination->R =
        static_cast<std::uint8_t>((source.rreg & 0x7f) |
                                  (source.rreg7 & 0x80));
    destination->I = source.ireg;
    destination->IFF =
        static_cast<std::uint8_t>((source.iff1 ? kIff1 : 0) |
                                  (source.iff2 ? kIff2 : 0) |
                                  (halted ? kIffHalt : 0));
    destination->interrupt = source.intmode & 0x03;
    destination->execEI = ei_inhibited ? 1 : 0;
}

} // namespace

struct Z80C::Impl {
    explicit Impl(Z80C *owner)
        : cpu(&Z80C::ReadMemory, &Z80C::WriteMemory, &Z80C::Input,
              &Z80C::Output, owner, false) {
        cpu.setConsumeClockCallback(&Z80C::ConsumeClock);
        cpu.setInterruptAcknowledgeCallback(&Z80C::Acknowledge);
    }

    Z80 cpu;
};

Z80C::Z80C()
    : impl_(nullptr), memory_(nullptr), bus_(nullptr), clock_(nullptr),
      clockcounter_(nullptr), lastclock_(0), acknowledge_port_(0),
      external_wait_(false), irq_asserted_(false), public_registers_{} {
}

Z80C::~Z80C() {
    delete impl_;
}

bool Z80C::Init(IMemoryAccess *memory, IIOAccess *bus, IClock *clock,
                IClockCounter *clockcounter,
                int interrupt_acknowledge_port) {
    if (memory == nullptr || bus == nullptr || clock == nullptr ||
        clockcounter == nullptr) {
        return false;
    }

    delete impl_;
    impl_ = nullptr;
    memory_ = memory;
    bus_ = bus;
    clock_ = clock;
    clockcounter_ = clockcounter;
    acknowledge_port_ = interrupt_acknowledge_port;
    impl_ = new (std::nothrow) Impl(this);
    if (impl_ == nullptr) {
        return false;
    }
    Reset();
    return true;
}

void Z80C::Exec() {
    if (impl_ == nullptr || clock_ == nullptr || clockcounter_ == nullptr) {
        return;
    }

    const std::uint32_t now = clock_->now();
    const std::uint32_t delta =
        now - static_cast<std::uint32_t>(lastclock_);
    clockcounter_->SetRemainclock(
        AddClockDelta(clockcounter_->GetRemainclock(), delta));

    if (external_wait_) {
        const std::int32_t remaining = clockcounter_->GetRemainclock();
        if (remaining > 0) {
            clockcounter_->past(remaining);
        }
    } else {
        while (clockcounter_->GetRemainclock() > 0) {
            impl_->cpu.execute(1);
        }
    }

    SynchronizePublicMirror();
    lastclock_ = SignedFromBits(now);
}

void IOCALL Z80C::Reset(std::uint32_t, std::uint32_t) {
    if (impl_ == nullptr) {
        public_registers_ = Z80Reg{};
        return;
    }

    std::memset(&impl_->cpu.reg, 0, sizeof(impl_->cpu.reg));
    impl_->cpu.setIRQLine(false);
    external_wait_ = false;
    irq_asserted_ = false;
    if (clockcounter_ != nullptr) {
        clockcounter_->SetRemainclock(0);
    }
    lastclock_ = clock_ != nullptr ? SignedFromBits(clock_->now()) : 0;
    SynchronizePublicMirror();
}

void IOCALL Z80C::IRQ(std::uint32_t, std::uint32_t asserted) {
    irq_asserted_ = asserted != 0;
    if (impl_ != nullptr) {
        impl_->cpu.setIRQLine(irq_asserted_);
    }
}

void IOCALL Z80C::NMI(std::uint32_t, std::uint32_t) {
    if (impl_ == nullptr || memory_ == nullptr || clockcounter_ == nullptr) {
        return;
    }

    Z80::Register &reg = impl_->cpu.reg;
    reg.R = static_cast<std::uint8_t>(((reg.R + 1) & 0x7f) |
                                      (reg.R & 0x80));
    if ((reg.IFF & kIff1) != 0) {
        reg.IFF |= kIff2;
    } else {
        reg.IFF &= static_cast<std::uint8_t>(~kIff2);
    }
    reg.IFF = static_cast<std::uint8_t>((reg.IFF & ~kIffHalt) | kIffNmi);
    reg.IFF &= static_cast<std::uint8_t>(~kIff1);
    const std::uint16_t pc = reg.PC;
    reg.SP = static_cast<std::uint16_t>(reg.SP - 1);
    memory_->Write8(reg.SP, static_cast<std::uint8_t>(pc >> 8));
    reg.SP = static_cast<std::uint16_t>(reg.SP - 1);
    memory_->Write8(reg.SP, static_cast<std::uint8_t>(pc));
    reg.PC = 0x0066;
    clockcounter_->past(11);
    SynchronizePublicMirror();
}

void Z80C::Wait(bool asserted) {
    external_wait_ = asserted;
}

std::uint32_t IFCALL Z80C::GetStatusSize() {
    return static_cast<std::uint32_t>(vaeg::z80::kRevision1Size);
}

bool IFCALL Z80C::SaveStatus(std::uint8_t *status) {
    if (impl_ == nullptr || clockcounter_ == nullptr || status == nullptr) {
        return false;
    }

    vaeg::z80::LegacyState state;
    state.registers = ExportRegisters(impl_->cpu.reg);
    state.halted = (impl_->cpu.reg.IFF & kIffHalt) != 0;
    state.external_wait = external_wait_;
    state.irq_asserted = irq_asserted_;
    state.ei_inhibited = impl_->cpu.reg.execEI != 0;
    state.remainclock = clockcounter_->GetRemainclock();
    state.lastclock = lastclock_;
    vaeg::z80::EncodeRevision1(state, status);
    return true;
}

bool IFCALL Z80C::LoadStatus(const std::uint8_t *status) {
    if (impl_ == nullptr || clockcounter_ == nullptr || status == nullptr) {
        return false;
    }

    vaeg::z80::LegacyState state;
    if (!vaeg::z80::DecodeRevision1(status, &state)) {
        return false;
    }

    ImportRegisters(state.registers, state.halted, state.ei_inhibited,
                    &impl_->cpu.reg);
    external_wait_ = state.external_wait;
    irq_asserted_ = state.irq_asserted;
    impl_->cpu.setIRQLine(irq_asserted_);
    clockcounter_->SetRemainclock(state.remainclock);
    lastclock_ = state.lastclock;
    SynchronizePublicMirror();
    return true;
}

std::uint32_t Z80C::GetPC() {
    return impl_ != nullptr ? impl_->cpu.reg.PC : 0;
}

void Z80C::SetPC(std::uint32_t new_pc) {
    const std::uint16_t pc = static_cast<std::uint16_t>(new_pc);
    if (impl_ != nullptr) {
        impl_->cpu.reg.PC = pc;
    }
    public_registers_.pc = pc;
}

const Z80Reg *Z80C::GetReg() {
    return &public_registers_;
}

Z80Diag *Z80C::GetDiag() {
    return nullptr;
}

std::uint8_t Z80C::ReadMemory(void *opaque, std::uint16_t address) {
    Z80C *cpu = static_cast<Z80C *>(opaque);
    return static_cast<std::uint8_t>(
        cpu->memory_->Read8(static_cast<std::uint16_t>(address)));
}

void Z80C::WriteMemory(void *opaque, std::uint16_t address,
                       std::uint8_t data) {
    Z80C *cpu = static_cast<Z80C *>(opaque);
    cpu->memory_->Write8(static_cast<std::uint16_t>(address), data);
}

std::uint8_t Z80C::Input(void *opaque, std::uint16_t port) {
    Z80C *cpu = static_cast<Z80C *>(opaque);
    return static_cast<std::uint8_t>(cpu->bus_->In(port & 0xffU));
}

void Z80C::Output(void *opaque, std::uint16_t port, std::uint8_t data) {
    Z80C *cpu = static_cast<Z80C *>(opaque);
    cpu->bus_->Out(port & 0xffU, data);
}

void Z80C::ConsumeClock(void *opaque, int clocks) {
    Z80C *cpu = static_cast<Z80C *>(opaque);
    cpu->clockcounter_->past(static_cast<std::int32_t>(clocks));
}

std::uint8_t Z80C::Acknowledge(void *opaque) {
    Z80C *cpu = static_cast<Z80C *>(opaque);
    return static_cast<std::uint8_t>(cpu->bus_->In(
        static_cast<std::uint32_t>(cpu->acknowledge_port_)));
}

void Z80C::SynchronizePublicMirror() {
    if (impl_ != nullptr) {
        public_registers_ = ExportRegisters(impl_->cpu.reg);
    }
}
