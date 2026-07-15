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

#ifndef CPUCVA_Z80_CORE_H
#define CPUCVA_Z80_CORE_H

#include "z80_bus.h"
#include "z80_registers.h"

#include <cstdint>

class Z80Diag;

class Z80C {
public:
    Z80C();
    ~Z80C();

    bool Init(IMemoryAccess *memory, IIOAccess *bus, IClock *clock,
              IClockCounter *clockcounter, int interrupt_acknowledge_port);
    void Exec();

    void IOCALL Reset(std::uint32_t = 0, std::uint32_t = 0);
    void IOCALL IRQ(std::uint32_t, std::uint32_t asserted);
    void IOCALL NMI(std::uint32_t = 0, std::uint32_t = 0);
    void Wait(bool asserted);

    std::uint32_t IFCALL GetStatusSize();
    bool IFCALL SaveStatus(std::uint8_t *status);
    bool IFCALL LoadStatus(const std::uint8_t *status);

    std::uint32_t GetPC();
    void SetPC(std::uint32_t new_pc);
    const Z80Reg *GetReg();

    Z80Diag *GetDiag();

private:
    struct Impl;

    static std::uint8_t ReadMemory(void *opaque, std::uint16_t address);
    static void WriteMemory(void *opaque, std::uint16_t address,
                            std::uint8_t data);
    static std::uint8_t Input(void *opaque, std::uint16_t port);
    static void Output(void *opaque, std::uint16_t port,
                       std::uint8_t data);
    static void ConsumeClock(void *opaque, int clocks);
    static std::uint8_t Acknowledge(void *opaque);

    void SynchronizePublicMirror();

    Z80C(const Z80C &) = delete;
    Z80C &operator=(const Z80C &) = delete;

    Impl *impl_;
    IMemoryAccess *memory_;
    IIOAccess *bus_;
    IClock *clock_;
    IClockCounter *clockcounter_;
    std::int32_t lastclock_;
    std::int32_t acknowledge_port_;
    bool external_wait_;
    bool irq_asserted_;
    Z80Reg public_registers_;
};

#endif
