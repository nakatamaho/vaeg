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

#ifndef CPU_Z80_COMPAT_CPU_H
#define CPU_Z80_COMPAT_CPU_H

#include "z80_compat_bus.h"
#include "z80_compat_registers.h"

#include <cstdint>

class Z80CompatCpu {
  public:
	Z80CompatCpu();
	~Z80CompatCpu();

	bool Init(IMemoryAccess *memory, IIOAccess *bus, IClock *clock, IClockCounter *clockcounter,
	          int interrupt_acknowledge_port);
	void Exec();
	void ExecOne();

	void IOCALL Reset(std::uint32_t = 0, std::uint32_t = 0);
	void IOCALL IRQ(std::uint32_t, std::uint32_t asserted);
	void IOCALL NMI(std::uint32_t = 0, std::uint32_t = 0);
	void Wait(bool asserted);

	std::uint32_t IFCALL GetStatusSize();
	bool IFCALL SaveStatus(std::uint8_t *status);
	bool IFCALL LoadStatus(const std::uint8_t *status);

	std::uint32_t GetPC();
	void SetPC(std::uint32_t new_pc);
	void SetReg(const Z80CompatReg &reg);
	void SetMainReg(const Z80CompatReg &reg);
	void SetMemoryBases(std::uint32_t code_base, std::uint32_t data_base);
	const Z80CompatReg *GetReg();

  private:
	struct Impl;

	static std::uint8_t ReadMemory(void *opaque, std::uint16_t address);
	static void WriteMemory(void *opaque, std::uint16_t address, std::uint8_t data);
	static std::uint8_t Input(void *opaque, std::uint16_t port);
	static void Output(void *opaque, std::uint16_t port, std::uint8_t data);
	static void ConsumeClock(void *opaque, int clocks);
	static std::uint8_t Acknowledge(void *opaque);

	void SynchronizePublicMirror();
	void ApplyInstructionCorrections();
	void ExecuteOne();
	std::uint32_t TranslateCodeAddress(std::uint16_t address) const;
	std::uint32_t TranslateDataAddress(std::uint16_t address) const;

	Z80CompatCpu(const Z80CompatCpu &) = delete;
	Z80CompatCpu &operator=(const Z80CompatCpu &) = delete;

	Impl *impl_;
	IMemoryAccess *memory_;
	IIOAccess *bus_;
	IClock *clock_;
	IClockCounter *clockcounter_;
	std::int32_t lastclock_;
	std::int32_t acknowledge_port_;
	bool external_wait_;
	bool irq_asserted_;
	bool instruction_fetch_started_;
	bool prefix_fetch_pending_;
	std::uint8_t first_opcode_;
	std::uint8_t prefixed_opcode_;
	bool restore_iff1_after_instruction_;
	bool materialize_i_flags_after_instruction_;
	bool materialize_r_flags_after_instruction_;
	std::uint32_t code_base_;
	std::uint32_t data_base_;
	Z80CompatReg public_registers_;
};

using UPD70008C = Z80CompatCpu;
using UPD780C = Z80CompatCpu;

#endif
