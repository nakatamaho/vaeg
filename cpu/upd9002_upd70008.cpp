/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
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
#include "compiler.h"
#include "machine/pccore.h"
#include "cpucore.h"
#include "io/iocore.h"
#include "io/upd9002_regs.h"
#include "io/memctrlva.h"
#include "io/sysportva.h"
#include "memoryva/memoryva.h"
#include "cpu/upd9002_upd70008.h"
#include "cpu/upd9002_upd70008_core_config.h"
#include "cpu/upd780/upd780_core_config.h"
#include "cpu/z80_compat_cpu.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

// Interrupt acknowledge cycles read this virtual port, outside the 16-bit
// I/O space, so the 8214 vector never comes from a real port.
constexpr std::uint32_t kInterruptAcknowledgePort = 0x10000;

// VAEG_UPD70008_TRACE=N records N compatible-mode events (default 4096);
// VAEG_UPD70008_TRACE_SKIP=M first passes over M events, so a bounded window
// can be placed late in a long run.
static std::uint32_t compat_trace_slot() {
	static bool initialized = false;
	static std::uint32_t limit = 0;
	static std::uint32_t count = 0;
	static unsigned long long skip = 0;
	if (!initialized) {
		const char *skip_value = std::getenv("VAEG_UPD70008_TRACE_SKIP");
		if (skip_value != nullptr && skip_value[0] != '\0') {
			char *end = nullptr;
			unsigned long long parsed = std::strtoull(skip_value, &end, 10);
			if (end != skip_value && *end == '\0') {
				skip = parsed;
			}
		}
		const char *value = std::getenv("VAEG_UPD70008_TRACE");
		if (value != nullptr && value[0] != '\0') {
			char *end = nullptr;
			unsigned long parsed = std::strtoul(value, &end, 10);
			if (end != value && *end == '\0' && parsed != 0) {
				limit = parsed > 1000000UL ? 1000000U : static_cast<std::uint32_t>(parsed);
			} else {
				limit = 4096;
			}
		}
		initialized = true;
	}
	if (count >= limit) {
		return UINT32_MAX;
	}
	if (skip != 0) {
		skip--;
		return UINT32_MAX;
	}
	return count++;
}

static void compat_trace(const char *event, std::uint32_t slot, std::uint8_t op0,
                         std::uint8_t op1) {
	if (slot == UINT32_MAX) {
		return;
	}
	std::fprintf(
	    stderr,
	    "m76-compat-trace event=%s slot=%u cs=%04x ip=%04x "
	    "op=%02x/%02x af=%02x%02x bc=%04x de=%04x hl=%04x "
	    "ix=%04x iy=%04x sp=%04x nsp=%04x ss=%04x stk=%04x/%04x/%04x "
	    "flags=%04x rem=%d\n",
	    event, slot, CPU_CS, CPU_IP, op0, op1, CPU_AL, static_cast<unsigned>(CPU_FLAG & 0xff),
	    CPU_CX, CPU_DX, CPU_BX, CPU_SI, CPU_DI, CPU_BP, CPU_SP, CPU_SS,
	    static_cast<unsigned>(mem[(SS_BASE + CPU_SP) & CPU_ADRSMASK]) |
	        (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 1) & 0xffff)) & CPU_ADRSMASK]) << 8),
	    static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 2) & 0xffff)) & CPU_ADRSMASK]) |
	        (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 3) & 0xffff)) & CPU_ADRSMASK]) << 8),
	    static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 4) & 0xffff)) & CPU_ADRSMASK]) |
	        (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 5) & 0xffff)) & CPU_ADRSMASK]) << 8),
	    CPU_FLAG, CPU_REMCLOCK);
}

// M135: the time of a compatible-mode instruction on the PC-88VA and
// PC-88VA2, from Z80TIME 2 (demos/z80-timing; issue #62): eight copies of
// one instruction a pass, steady state, without waits (the VA2's figures),
// in 1/256 clock. The core's Z80 cycles are replaced by these; memory and
// I/O waits are still charged natively (Read8, Write8, In, Out). The
// measured classes are marked M; the others take the measured class they
// resemble most (an operand access, a taken transfer, a prefix).
namespace {

constexpr std::uint32_t C(double clocks) {
	return static_cast<std::uint32_t>(clocks * 256.0 + 0.5);
}

struct CompatCost {
	std::uint8_t length = 1;
	bool conditional = false; // taken when the next PC is not after it
	bool fallback = false;    // the core's cycles (I/O, HALT, unknown)
	std::uint32_t taken = 0;
	std::uint32_t not_taken = 0;
};

CompatCost Cost(std::uint32_t clocks, std::uint8_t length) {
	CompatCost c;
	c.length = length;
	c.taken = c.not_taken = clocks;
	return c;
}

CompatCost Conditional(std::uint32_t taken, std::uint32_t not_taken, std::uint8_t length) {
	CompatCost c = Cost(taken, length);
	c.conditional = true;
	c.not_taken = not_taken;
	return c;
}

CompatCost Fallback() {
	CompatCost c;
	c.fallback = true;
	return c;
}

const std::uint32_t kReg = C(2.85);       // M: INC A, ADD A,C, EX DE,HL, ADD HL,DE
const std::uint32_t kMove = C(2.98);      // M: LD A,B
const std::uint32_t kImm = C(4.01);       // M: LD A,n
const std::uint32_t kAluImm = C(4.23);    // M: ADD A,n, AND n
const std::uint32_t kPair = C(2.08);      // M: INC DE, EXX
const std::uint32_t kLoad = C(10.94);     // M: LD A,(HL)
const std::uint32_t kStore = C(10.40);    // M: LD (HL),A
const std::uint32_t kReadModify = C(17.37); // M: INC (HL)
const std::uint32_t kJr = C(12.52);       // M: JR
const std::uint32_t kJp = C(15.16);       // M: JP nn
const std::uint32_t kJpNot = C(6.35);     // M: JP NZ not taken, LD DE,nn
const std::uint32_t kCall = C(22.54);     // M: CALL + RET 41.54 (RET 19)
const std::uint32_t kRet = C(19.00);
const std::uint32_t kStack = C(10.42);    // M: PUSH + POP 20.84
const std::uint32_t kIndexed = C(3.00);   // DD/FD on a register form: ADD IX,DE
const std::uint32_t kBlock = C(24.02);    // M: LDIR, a byte

CompatCost Unprefixed(std::uint8_t op) {
	const std::uint8_t x = op >> 6;
	const std::uint8_t y = (op >> 3) & 7;
	const std::uint8_t z = op & 7;
	if (x == 1) {
		if (op == 0x76) {
			return Fallback(); // HALT
		}
		if (z == 6) {
			return Cost(kLoad, 1);
		}
		return Cost(y == 6 ? kStore : kMove, 1);
	}
	if (x == 2) {
		return Cost(z == 6 ? kLoad : kReg, 1);
	}
	if (x == 0) {
		switch (z) {
		case 0:
			switch (y) {
			case 0:
				return Cost(C(1.97), 1); // M: NOP
			case 1:
				return Cost(kPair, 1); // EX AF,AF'
			case 2:
				return Conditional(C(18.10), C(6.00), 2); // M: DJNZ taken (block entry apart)
			case 3:
				return Cost(kJr, 2);
			default:
				return Conditional(kJr, C(5.24), 2); // M: JR NZ not taken
			}
		case 1:
			return Cost((op & 8) ? kReg : kJpNot, (op & 8) ? 1 : 3); // ADD HL,rr / LD rr,nn
		case 2:
			switch (y) {
			case 0:
			case 2:
				return Cost(kStore, 1); // LD (BC/DE),A
			case 1:
			case 3:
				return Cost(C(8.99), 1); // M: LD A,(DE)
			case 4:
			case 6:
				return Cost(C(11.98), 3); // M: LD (nn),HL, LD (nn),A
			default:
				return Cost(C(11.49), 3); // M: LD HL,(nn), LD A,(nn)
			}
		case 3:
			return Cost(kPair, 1); // INC/DEC rr
		case 4:
		case 5:
			return Cost(y == 6 ? kReadModify : kReg, 1);
		case 6:
			return Cost(y == 6 ? C(11.50) : kImm, 2); // LD (HL),n: a store and a byte
		default:
			switch (y) {
			case 4:
				return Cost(C(7.08), 1); // M: DAA
			case 5:
			case 6:
			case 7:
				return Cost(kReg, 1); // CPL, SCF, CCF
			default:
				return Cost(C(5.77), 1); // M: RLCA
			}
		}
	}
	switch (z) {
	case 0:
		return Conditional(kRet + C(1.0), C(3.88), 1); // M: RET NZ not taken
	case 1:
		switch (op) {
		case 0xc9:
			return Cost(kRet, 1);
		case 0xd9:
			return Cost(kPair, 1); // M: EXX
		case 0xe9:
			return Cost(kJr, 1); // JP (HL)
		case 0xf9:
			return Cost(kReg, 1); // LD SP,HL
		default:
			return Cost(kStack, 1); // POP
		}
	case 2:
		return Conditional(kJp, kJpNot, 3);
	case 3:
		switch (op) {
		case 0xc3:
			return Cost(kJp, 3);
		case 0xe3:
			return Cost(C(20.0), 1); // EX (SP),HL: a word read and written
		case 0xeb:
			return Cost(kReg, 1);
		case 0xf3:
		case 0xfb:
			return Cost(C(1.97), 1); // DI, EI
		default:
			return Fallback(); // OUT (n),A, IN A,(n); CB is decoded apart
		}
	case 4:
		return Conditional(kCall, kJpNot, 3);
	case 5:
		return Cost(op == 0xcd ? kCall : kStack, op == 0xcd ? 3 : 1);
	case 6:
		return Cost(kAluImm, 2);
	default:
		return Cost(kCall - C(2.5), 1); // RST
	}
}

CompatCost PrefixCb(std::uint8_t op) {
	const bool bit = (op >> 6) == 1;
	if ((op & 7) == 6) {
		return Cost(bit ? C(13.0) : C(21.79), 2); // M: SET 0,(HL)
	}
	return Cost(bit ? C(5.94) : C(8.21), 2); // M: BIT 0,A, SRL A
}

CompatCost PrefixEd(std::uint8_t op) {
	if (op >= 0x40 && op < 0x80) {
		switch (op & 0x0f) {
		case 0x00:
		case 0x01:
		case 0x08:
		case 0x09:
			return Fallback(); // IN r,(C), OUT (C),r
		case 0x02:
		case 0x0a:
			return Cost(kReg + kIndexed, 2); // SBC/ADC HL,rr
		case 0x03:
			return Cost(C(11.98) + kIndexed, 4); // LD (nn),rr
		case 0x0b:
			return Cost(C(11.49) + kIndexed, 4); // LD rr,(nn)
		case 0x04:
		case 0x0c:
			return Cost(C(4.01), 2); // M: NEG
		case 0x05:
		case 0x0d:
			return Cost(kRet + kIndexed, 2); // RETN, RETI
		case 0x07:
		case 0x0f:
			if (op == 0x67 || op == 0x6f) {
				return Cost(C(21.79), 2); // RRD, RLD
			}
			return Cost(C(5.0), 2); // LD I,A ...
		default:
			return Cost(C(4.01), 2); // IM
		}
	}
	switch (op) {
	case 0xa0:
	case 0xa8:
	case 0xb0:
	case 0xb8:
	case 0xa1:
	case 0xa9:
	case 0xb1:
	case 0xb9:
		return Cost(kBlock, 2); // LDI ... CPDR, an iteration
	default:
		return Fallback(); // block I/O (trapped), undefined
	}
}

CompatCost PrefixIndex(std::uint8_t op, std::uint8_t op3) {
	if (op == 0xcb) {
		return Cost((op3 >> 6) == 1 ? C(15.0) : C(27.44), 4); // M: RL (IX+d)
	}
	if (op == 0x34 || op == 0x35) {
		return Cost(C(25.30), 3); // M: INC (IX+d)
	}
	if (op == 0x36) {
		return Cost(C(19.50), 4); // LD (IX+d),n
	}
	if ((op & 0xc0) == 0x40 && op != 0x76) {
		if ((op & 7) == 6) {
			return Cost(C(12.91), 3); // M: LD A,(IX+d)
		}
		if ((op & 0xf8) == 0x70) {
			return Cost(C(18.79), 3); // M: LD (IX+d),A
		}
	}
	if ((op & 0xc0) == 0x80 && (op & 7) == 6) {
		return Cost(C(12.91), 3); // ALU (IX+d)
	}
	if (op == 0x21) {
		return Cost(C(8.44), 4); // M: LD IX,nn
	}
	if (op == 0xdd || op == 0xfd || op == 0xed || op == 0x76) {
		return Fallback();
	}
	CompatCost c = Unprefixed(op);
	if (c.fallback) {
		return c;
	}
	c.length = static_cast<std::uint8_t>(c.length + 1);
	c.taken += kIndexed; // M: ADD IX,DE 5.94 against ADD HL,DE 2.85
	c.not_taken += kIndexed;
	return c;
}

CompatCost CompatInstructionCost(std::uint8_t op0, std::uint8_t op1, std::uint8_t op3) {
	switch (op0) {
	case 0xcb:
		return PrefixCb(op1);
	case 0xed:
		return PrefixEd(op1);
	case 0xdd:
	case 0xfd:
		return PrefixIndex(op1, op3);
	default:
		return Unprefixed(op0);
	}
}

} // namespace

class CompatCounter final : public IClockCounter {
  public:
	void IFCALL past(std::int32_t clocks) override {
		if (measured_) {
			core_cycles_ += clocks; // replaced by the measured cost (M135)
			return;
		}
		// CPU speed (machine/pccore.c): 100 / percent machine clocks a cycle.
		remain_ -=
		    static_cast<std::int32_t>(pccore_compat_clocks(static_cast<UINT32>(clocks), FALSE));
	}

	// M135: during an instruction the core's cycles are only counted; the
	// adapter then charges the measured cost (1/256 clock, the fraction
	// carried) or, for the classes without one, the core's cycles.
	void BeginMeasured() {
		measured_ = true;
		core_cycles_ = 0;
	}

	void ResetFraction() {
		fraction_ = 0;
	}

	void EndMeasured(bool use_core, std::uint32_t cost) {
		measured_ = false;
		if (use_core) {
			if (core_cycles_ > 0) {
				past(core_cycles_);
			}
			return;
		}
		const std::uint32_t total = cost + fraction_;
		fraction_ = total & 0xff;
		const std::uint32_t clocks = total >> 8;
		if (clocks > 0) {
			remain_ -= static_cast<std::int32_t>(pccore_compat_clocks(clocks, FALSE));
		}
	}

	void past_native(std::int32_t clocks) {
		remain_ -=
		    static_cast<std::int32_t>(pccore_compat_clocks(static_cast<UINT32>(clocks), TRUE));
	}

	std::int32_t IFCALL GetRemainclock() override {
		return remain_;
	}

	void IFCALL SetRemainclock(std::int32_t clocks) override {
		remain_ = clocks;
	}

  private:
	std::int32_t remain_ = 0;
	bool measured_ = false;
	std::int32_t core_cycles_ = 0;
	std::uint32_t fraction_ = 0;
};

class CompatClock final : public IClock {
  public:
	std::uint32_t IFCALL now() override {
		return CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	}
};

// VA firmware remains the board bootstrap/device bridge. The main uPD780
// has its own state, distinct from VA compatible mode and the FDC uPD780.
class MainCpuBridge final : public IMemoryAccess, public IIOAccess {
  public:
	void Reset() {
		upd70008_.Reset();
		main_upd780_.Reset();
		upd70008_.SetMemoryBases(0, 0);
		main_upd780_.SetMemoryBases(0, 0);
		main_upd780_active_ = false;
		initialized_ = false;
		have_compatible_state_ = false;
		counter_.SetRemainclock(0);
		counter_.ResetFraction(); // (the measured costs' carried fraction, M135)
	}

	// Shared by Enter() and StateLoad() so that a state loaded before the
	// first mode entry also gets the native vector reader (M102 fix).
	bool InitializeCore() {
		main_upd780_active_ = pccore_main_cpu_is_upd780() != FALSE;
		if (main_upd780_active_) {
			// Share the FDC's instruction configuration, not its state/bus/clock.
			Upd780ConfigureCore(&main_upd780_);
		} else {
			// Undefined ED 00-3F/74/75/77 read the native-side physical
			// page 0 (the x86 IVT); measured on a PC-88VA2 (M102).
			Upd9002Upd70008ConfigureCore(
			    &upd70008_,
			    [](void *, std::uint8_t index) -> std::uint8_t {
				    return static_cast<std::uint8_t>(upd9002_memoryread_nowait(index));
			    },
			    nullptr);
		}
		initialized_ = Cpu().Init(this, this, &clock_, &counter_, kInterruptAcknowledgePort);
		return initialized_;
	}

	void Enter() {
		if (!initialized_) {
			if (!InitializeCore()) {
				return;
			}
		}
		SetMemoryBases();
		Z80CompatReg state = *Cpu().GetReg();
		state.af = static_cast<std::uint16_t>((static_cast<std::uint16_t>(CPU_AL) << 8) |
		                                      (CPU_FLAG & 0xff));
		state.bc = CPU_CX;
		state.de = CPU_DX;
		state.hl = CPU_BX;
		state.ix = CPU_SI;
		state.iy = CPU_DI;
		state.sp = CPU_BP;
		state.pc = CPU_IP;
		state.iff1 = (CPU_FLAG & I_FLAG) != 0;
		state.iff2 = state.iff1;
		if (have_compatible_state_) {
			Cpu().SetMainReg(state);
		} else {
			Cpu().SetReg(state);
			have_compatible_state_ = true;
		}
		LoadAltFromNative();
		counter_.SetRemainclock(CPU_REMCLOCK);
		const std::uint32_t trace_slot = compat_trace_slot();
		compat_trace("enter", trace_slot, 0, 0);
	}

	void Step() {
		if (!initialized_) {
			return;
		}
		counter_.SetRemainclock(CPU_REMCLOCK);
		const std::uint16_t pc = static_cast<std::uint16_t>(Cpu().GetPC());
		const std::uint32_t code_address = (CPU_CS << 4) + pc;
		// (looking ahead at the instruction: not bus cycles of their own)
		const std::uint8_t op0 = static_cast<std::uint8_t>(upd9002_memoryread_nowait(code_address));
		const std::uint8_t op1 = static_cast<std::uint8_t>(upd9002_memoryread_nowait(code_address + 1));
		const std::uint32_t trace_slot = compat_trace_slot();
		compat_trace("before", trace_slot, op0, op1);

		bool input = op0 == 0xdb;
		bool plain_io = input || op0 == 0xd3;
		UINT8 port = op1;
		// ED 40/41..78/79: IN r,(C) / OUT (C),r. ED A2/A3/AA/AB/B2/B3/BA/BB:
		// block I/O, which the VA2 ROM trap handler decodes and repeats itself.
		if (op0 == 0xed && (((op1 & 0xc6) == 0x40) || ((op1 & 0xe6) == 0xa2))) {
			plain_io = true;
			input = (op1 & 1) == 0;
			port = static_cast<UINT8>(Cpu().GetReg()->bc);
		}
		if (plain_io && (upd9002_iotrap.control & (input ? 1 : 2))) {
			for (unsigned range = 0; range < 8; range += 4) {
				// Compatible I/O uses the 8-bit port number. Native DX matching
				// and FFEF bit 4 belong to the separate native interception path.
				if (port >= upd9002_iotrap.ranges[range] &&
				    port <= upd9002_iotrap.ranges[range + 2]) {
					SyncToNative();
					upd9002_core_compat_iotrap(input ? 0x7c : 0x7d, pc);
					counter_.SetRemainclock(CPU_REMCLOCK);
					compat_trace(input ? "io-trap-in" : "io-trap-out", trace_slot, op0, op1);
					return;
				}
			}
		}

		if (!main_upd780_active_ && (op0 == 0xed) && (op1 == 0xed)) {
			const std::uint8_t vector =
			    static_cast<std::uint8_t>(upd9002_memoryread_nowait(code_address + 2));
			Cpu().SetPC(static_cast<std::uint16_t>(pc + 3));
			SyncToNative();
			upd9002_core_compat_calln(vector, static_cast<REG16>(pc + 3));
			counter_.SetRemainclock(CPU_REMCLOCK);
			compat_trace("calln", trace_slot, op0, op1);
			return;
		}
		if (!main_upd780_active_ && (op0 == 0xed) && (op1 == 0xfd)) {
			Cpu().SetPC(static_cast<std::uint16_t>(pc + 2));
			SyncToNative();
			upd9002_core_compat_retem();
			counter_.SetRemainclock(CPU_REMCLOCK);
			compat_trace("retem", trace_slot, op0, op1);
			return;
		}

		const bool was_enabled = Cpu().GetReg()->iff1 != 0;
		const std::uint8_t op3 =
		    static_cast<std::uint8_t>(upd9002_memoryread_nowait(code_address + 3));
		const CompatCost cost = CompatInstructionCost(op0, op1, op3);
		const bool halted = Cpu().Halted();
		acknowledged_ = false;
		last_data_ = kNoAddress;
		counter_.BeginMeasured();
		Cpu().ExecOne();
		bool use_core = main_upd780_active_ || cost.fallback || acknowledged_ || halted;
		std::uint32_t clocks = cost.taken;
		if (!use_core && cost.conditional) {
			const std::uint16_t next = static_cast<std::uint16_t>(pc + cost.length);
			if (static_cast<std::uint16_t>(Cpu().GetPC()) == next) {
				clocks = cost.not_taken;
			}
		}
		counter_.EndMeasured(use_core, clocks);
		SyncToNative();
		CPU_REMCLOCK = counter_.GetRemainclock();
		// The machine arbitrates interrupts only between CPU time slices
		// (pic_irq). As the native STI does, end the slice one instruction
		// after EI (or a RETI/RETN restoring IFF1) when a request is waiting,
		// so it is taken in the enabled window instead of the next slice.
		if (!was_enabled && (Cpu().GetReg()->iff1 != 0) && PICEXISTINTR && (CPU_REMCLOCK > 1)) {
			CPU_BASECLOCK += 1 - CPU_REMCLOCK;
			CPU_REMCLOCK = 1;
		}
		compat_trace("after", trace_slot, op0, op1);
	}

	void SyncToNative() {
		const Z80CompatReg *state = Cpu().GetReg();
		StoreAltToNative();
		CPU_AL = static_cast<UINT8>(state->af >> 8);
		CPU_FLAG = static_cast<UINT16>((CPU_FLAG & 0xfc00) | (state->af & 0xff) |
		                               (state->iff1 ? I_FLAG : 0));
		CPU_CX = state->bc;
		CPU_DX = state->de;
		CPU_BX = state->hl;
		CPU_SI = state->ix;
		CPU_DI = state->iy;
		CPU_BP = state->sp;
		CPU_IP = state->pc;
	}

	void Resume() {
		SetMemoryBases();
		Z80CompatReg state = *Cpu().GetReg();
		state.af = static_cast<std::uint16_t>((static_cast<std::uint16_t>(CPU_AL) << 8) |
		                                      (CPU_FLAG & 0xff));
		state.bc = CPU_CX;
		state.de = CPU_DX;
		state.hl = CPU_BX;
		state.ix = CPU_SI;
		state.iy = CPU_DI;
		state.sp = CPU_BP;
		state.pc = CPU_IP;
		state.iff1 = (CPU_FLAG & I_FLAG) != 0;
		state.iff2 = state.iff1;
		Cpu().SetMainReg(state);
		LoadAltFromNative();
		counter_.SetRemainclock(CPU_REMCLOCK);
	}

	// M103a: the alternate set lives in machine-owned storage
	// (upd9002_alt_regs); the compat core holds a working copy.
	void LoadAltFromNative() {
		Z80CompatReg state = *Cpu().GetReg();
		state.r_af = upd9002_alt_regs.af;
		state.r_bc = upd9002_alt_regs.bc;
		state.r_de = upd9002_alt_regs.de;
		state.r_hl = upd9002_alt_regs.hl;
		Cpu().SetAltReg(state);
	}

	void StoreAltToNative() {
		const Z80CompatReg *state = Cpu().GetReg();
		upd9002_alt_regs.af = state->r_af;
		upd9002_alt_regs.bc = state->r_bc;
		upd9002_alt_regs.de = state->r_de;
		upd9002_alt_regs.hl = state->r_hl;
	}

	void LeaveHalt() {
		Cpu().LeaveHalt();
	}

	void Irq(bool asserted) {
		if (initialized_) {
			Cpu().IRQ(0, asserted ? 1 : 0);
		}
	}

	int StateSave(UINT8 *buffer, UINT size) {
		if ((buffer == NULL) || (size != UPD9002_COMPAT_STATE_SIZE) || !initialized_) {
			return FAILURE;
		}
		counter_.SetRemainclock(CPU_REMCLOCK);
		return Cpu().SaveStatus(buffer) ? SUCCESS : FAILURE;
	}

	int StateLoad(const UINT8 *buffer, UINT size) {
		if ((buffer == NULL) || (size != UPD9002_COMPAT_STATE_SIZE)) {
			return FAILURE;
		}
		if (!initialized_) {
			if (!InitializeCore()) {
				return FAILURE;
			}
		}
		if (!Cpu().LoadStatus(buffer)) {
			return FAILURE;
		}
		SetMemoryBases();
		have_compatible_state_ = true;
		// The UPD9ALT section precedes this blob in a state file and marks
		// upd9002_alt_regs_loaded; then the machine-owned storage is the
		// authority. A file written before M103a has no such section, so
		// the blob's working copy becomes the storage.
		if (upd9002_alt_regs_loaded) {
			LoadAltFromNative();
		} else {
			StoreAltToNative();
		}
		CPU_REMCLOCK = counter_.GetRemainclock();
		return SUCCESS;
	}

	// The waits of a memory access (the WCU's programmed ones, M130, and
	// the devices') are taken from the native clock, as In() does.
	// M135: the bus moves words. An instruction byte at an odd address came
	// with the byte before it (the prefetch queue fetches aligned words), and
	// the high byte of a 16-bit operand at an even address shares its low
	// byte's bus cycle: neither waits again. Measured (Z80TIME 2, VA): code
	// above 40000h adds about half a clock a byte; LD HL,(nn), PUSH and POP
	// add one wait, as LD A,(nn) does.
	bool SharesBusCycle(std::uint32_t address, bool write) {
		// The uPD780 has an 8-bit bus, not the VA's aligned word fetches.
		if (main_upd780_active_) {
			return false;
		}
		if (Cpu().CodeRead()) {
			return (address & 1) != 0;
		}
		const bool shared = ((address & 1) != 0) && (last_data_ == address - 1) &&
		                    (last_data_write_ == write);
		last_data_ = address;
		last_data_write_ = write;
		return shared;
	}

	std::uint32_t IFCALL Read8(std::uint32_t address) override {
		const SINT32 before = CPU_REMCLOCK;
		const std::uint32_t value = upd9002_memoryread(address & CPU_ADRSMASK);
		const SINT32 elapsed = before - CPU_REMCLOCK;
		const bool shared = SharesBusCycle(address & CPU_ADRSMASK, false);
		if (elapsed > 0) {
			CPU_REMCLOCK = before;
			if (!shared) {
				counter_.past_native(elapsed);
			}
		}
		return value;
	}

	void IFCALL Write8(std::uint32_t address, std::uint32_t data) override {
		const SINT32 before = CPU_REMCLOCK;
		upd9002_memorywrite(address & CPU_ADRSMASK, static_cast<UINT8>(data));
		const SINT32 elapsed = before - CPU_REMCLOCK;
		const bool shared = SharesBusCycle(address & CPU_ADRSMASK, true);
		if (elapsed > 0) {
			CPU_REMCLOCK = before;
			if (!shared) {
				counter_.past_native(elapsed);
			}
		}
	}

	std::uint32_t IFCALL In(std::uint32_t port) override {
		if (port == kInterruptAcknowledgePort) {
			acknowledged_ = true; // (the core's cycles for that step, M135)
			Cpu().IRQ(0, 0);
			return pic8214_acknowledge_compat();
		}
		const SINT32 before = CPU_REMCLOCK;
		const UINT8 value = iocore_inp8(static_cast<UINT>(port & 0xffff));
		const SINT32 elapsed = before - CPU_REMCLOCK;
		if (elapsed > 0) {
			counter_.past_native(elapsed);
		}
		return value;
	}

	void IFCALL Out(std::uint32_t port, std::uint32_t data) override {
		const SINT32 before = CPU_REMCLOCK;
		iocore_out8(static_cast<UINT>(port & 0xffff), static_cast<UINT8>(data));
		const SINT32 elapsed = before - CPU_REMCLOCK;
		if (elapsed > 0) {
			counter_.past_native(elapsed);
		}
	}

  private:
	void SetMemoryBases() {
		Cpu().SetMemoryBases(static_cast<std::uint32_t>(CPU_CS) << 4,
		                     static_cast<std::uint32_t>(CPU_DS) << 4);
	}

	Z80CompatCpu &Cpu() {
		return main_upd780_active_ ? main_upd780_ : upd70008_;
	}

	UPD70008C upd70008_;
	UPD780C main_upd780_;
	bool main_upd780_active_ = false;
	static constexpr std::uint32_t kNoAddress = 0xffffffffU;
	bool acknowledged_ = false;
	std::uint32_t last_data_ = kNoAddress;
	bool last_data_write_ = false;
	CompatCounter counter_;
	CompatClock clock_;
	bool initialized_ = false;
	bool have_compatible_state_ = false;
};

MainCpuBridge compat;

void compat_reset() {
	compat.Reset();
}
void compat_enter() {
	compat.Enter();
	memctrlva_nmode_compat_entry();
}
void compat_step() {
	compat.Step();
}
// Called only when the core takes a native interrupt from compatible mode.
// The interrupt ends a HALT (V30 manual chapter 8): without this the IRET
// resumed a core that was still halted (M118).
void compat_sync_to_native() {
	compat.LeaveHalt();
	compat.SyncToNative();
}
void compat_leave() {
}
void compat_resume() {
	const std::uint32_t trace_slot = compat_trace_slot();
	compat_trace("resume-before", trace_slot, 0, 0);
	compat.Resume();
	compat_trace("resume-after", trace_slot, 0, 0);
}
int compat_state_save(UINT8 *buffer, UINT size) {
	return compat.StateSave(buffer, size);
}
int compat_state_load(const UINT8 *buffer, UINT size) {
	return compat.StateLoad(buffer, size);
}
void compat_irq(BOOL asserted) {
	compat.Irq(asserted != FALSE);
}

} // namespace

extern "C" void upd9002_upd70008_register(void) {
	static bool registered = false;
	if (registered) {
		return;
	}
	Upd9002CompatHooks hooks{};
	hooks.reset = compat_reset;
	hooks.enter = compat_enter;
	hooks.step = compat_step;
	hooks.sync_to_native = compat_sync_to_native;
	hooks.leave = compat_leave;
	hooks.resume = compat_resume;
	hooks.state_save = compat_state_save;
	hooks.state_load = compat_state_load;
	hooks.irq = compat_irq;
	upd9002_core_set_compat_hooks(&hooks);
	registered = true;
}

#if defined(VAEG_UPD9002_M76_TESTING)
static unsigned test_io_accesses;
static REG8 IOINPCALL test_trapped_in(UINT) {
	test_io_accesses++;
	return 0x99;
}
static void IOOUTCALL test_trapped_out(UINT, REG8) {
	test_io_accesses++;
}

static int compat_entry_selftest(UINT8 entry_opcode) {
	const UINT16 code_segment = 0x2000;
	const UINT16 native_stack_segment = 0x3000;
	const UINT16 code_offset = 0x0100;
	const UINT16 compatible_offset = 0x1000;
	const UINT16 native_offset = 0x3000;
	const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
	const UINT32 native_stack_base = static_cast<UINT32>(native_stack_segment) << 4;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_upd70008_register();
	upd9002_core_reset();

	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = native_stack_segment;
	CPU_IP = code_offset;
	CPU_SP = 0x0100;
	CPU_BP = 0x0200;
	CPU_FLAG = 0xf202;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = native_stack_base;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_CLOCK = 0;

	mem[(0x00e1U * 4) + 0] = 0x00;
	mem[(0x00e1U * 4) + 1] = 0x10;
	mem[(0x00e1U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e1U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	mem[(0x00e0U * 4) + 0] = static_cast<UINT8>(native_offset);
	mem[(0x00e0U * 4) + 1] = static_cast<UINT8>(native_offset >> 8);
	mem[(0x00e0U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e0U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	const UINT16 native_interrupt_offset = 0x3100;
	mem[(0x00e2U * 4) + 0] = static_cast<UINT8>(native_interrupt_offset);
	mem[(0x00e2U * 4) + 1] = static_cast<UINT8>(native_interrupt_offset >> 8);
	mem[(0x00e2U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e2U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);

	mem[code_base + code_offset + 0] = 0x0f;
	mem[code_base + code_offset + 1] = entry_opcode;
	mem[code_base + code_offset + 2] = 0xe1;
	mem[code_base + compatible_offset + 0] = 0x3e;
	mem[code_base + compatible_offset + 1] = 0x42;
	mem[code_base + compatible_offset + 2] = 0x18;
	mem[code_base + compatible_offset + 3] = 0x03;
	mem[code_base + compatible_offset + 4] = 0x3e;
	mem[code_base + compatible_offset + 5] = 0x99;
	mem[code_base + compatible_offset + 6] = 0x00;
	mem[code_base + compatible_offset + 7] = 0xdd;
	mem[code_base + compatible_offset + 8] = 0x21;
	mem[code_base + compatible_offset + 9] = 0x78;
	mem[code_base + compatible_offset + 10] = 0x56;
	mem[code_base + compatible_offset + 11] = 0xfd;
	mem[code_base + compatible_offset + 12] = 0x21;
	mem[code_base + compatible_offset + 13] = 0xbc;
	mem[code_base + compatible_offset + 14] = 0x9a;
	mem[code_base + compatible_offset + 15] = 0xed;
	mem[code_base + compatible_offset + 16] = 0xed;
	mem[code_base + compatible_offset + 17] = 0xe0;
	mem[code_base + compatible_offset + 18] = 0x21;
	mem[code_base + compatible_offset + 19] = 0x34;
	mem[code_base + compatible_offset + 20] = 0x12;
	mem[code_base + compatible_offset + 21] = 0xed;
	mem[code_base + compatible_offset + 22] = 0xfd;
	mem[code_base + native_offset] = 0xcf;
	mem[code_base + native_interrupt_offset] = 0xcf;

	upd9002_core_step();
	if ((CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) || (CPU_CS != code_segment) ||
	    (CPU_IP != compatible_offset) || (CPU_SP != 0x00fa) || (CPU_DS != code_segment) ||
	    (CPU_SS != native_stack_segment) ||
	    (upd9002_memoryread_w(native_stack_base + 0x00fa) != code_offset + 3) ||
	    (upd9002_memoryread_w(native_stack_base + 0x00fc) != code_segment) ||
	    (upd9002_memoryread_w(native_stack_base + 0x00fe) != 0xf202)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if (CPU_AL != 0x42 || CPU_IP != compatible_offset + 2) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	// Trap immediate and ordinary ED I/O before any device side effect.
	iocore_create();
	if (iocore_build() != SUCCESS) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	iocore_attachinp(0x50, test_trapped_in);
	iocore_attachout(0x50, test_trapped_out);
	iocore_attachinp(0x00, test_trapped_in);
	iocore_attachout(0x00, test_trapped_out);
	upd9002_iotrap = {};
	upd9002_iotrap.ranges[0] = 0x50;
	upd9002_iotrap.ranges[2] = 0x50;
	upd9002_iotrap.control = 3;
	test_io_accesses = 0;
	static const UINT8 block_io[8] = {0xa2, 0xa3, 0xaa, 0xab, 0xb2, 0xb3, 0xba, 0xbb};
	for (unsigned form = 0; form < 26; form++) {
		const bool output = (form & 1) != 0;
		const unsigned vector = output ? 0x7d : 0x7c;
		upd9002_iotrap.ranges[0] = form < 2 ? 0x50 : 0x00;
		upd9002_iotrap.ranges[2] = upd9002_iotrap.ranges[0];
		upd9002_memorywrite_w(vector * 4, native_offset);
		upd9002_memorywrite_w(vector * 4 + 2, code_segment);
		mem[code_base + compatible_offset + 2] = form < 2 ? (output ? 0xd3 : 0xdb) : 0xed;
		mem[code_base + compatible_offset + 3] =
		    form < 2    ? 0x50
		    : form < 18 ? static_cast<UINT8>(0x40 + ((form - 2) / 2) * 8 + (form & 1))
		                : block_io[form - 18];
		upd9002_core_step();
		bool ok = CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE && CPU_IP == native_offset &&
		          CPU_SP == 0x00f4 && !(CPU_FLAG & I_FLAG) && test_io_accesses == 0 &&
		          upd9002_memoryread_w(native_stack_base + 0xf4) == compatible_offset + 2;
		// The synthetic IRET handler deliberately retries the same IP so the
		// next I/O form and then the original JR can reuse this location.
		if (ok) {
			upd9002_core_step();
		}
		ok = ok && CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008 && CPU_IP == compatible_offset + 2 &&
		     CPU_AL == 0x42 && CPU_SP == 0xfa;
		if (!ok) {
			upd9002_iotrap = {};
			iocore_destroy();
			upd9002_core_deinitialize();
			return FAILURE;
		}
	}
	upd9002_iotrap = {};
	iocore_destroy();
	mem[code_base + compatible_offset + 2] = 0x18;
	mem[code_base + compatible_offset + 3] = 0x03;
	upd9002_core_step();
	if (CPU_IP != compatible_offset + 7) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if (CPU_SI != 0x5678 || CPU_IP != compatible_offset + 11) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if (CPU_DI != 0x9abc || CPU_IP != compatible_offset + 15) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	UINT8 saved_compat[UPD9002_COMPAT_STATE_SIZE];
	if (upd9002_core_compat_state_save(saved_compat, sizeof(saved_compat)) != SUCCESS) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if ((CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE) || (CPU_IP != native_offset) ||
	    (CPU_SP != 0x00f4)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_interrupt(0xe2);
	upd9002_core_step();
	if ((CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE) || (CPU_IP != native_offset) ||
	    (CPU_SP != 0x00f4) || (CPU_COMPAT_RETURN_PENDING == 0)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if ((CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) || (CPU_IP != compatible_offset + 18) ||
	    (CPU_SP != 0x00fa)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if (CPU_BX != 0x1234 || CPU_IP != compatible_offset + 21) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	int passed = (CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE) && (CPU_CS == code_segment) &&
	             (CPU_IP == code_offset + 3) && (CPU_SP == 0x0100) && (CPU_AL == 0x42) &&
	             (CPU_BX == 0x1234) && (CPU_SI == 0x5678) && (CPU_DI == 0x9abc);
	if (passed) {
		upd9002_core_reset();
		CPU_CS = code_segment;
		CPU_DS = code_segment;
		CPU_SS = native_stack_segment;
		CPU_SP = 0x00fa;
		CPU_BP = 0x0200;
		CPU_FLAG = 0xf202;
		CS_BASE = code_base;
		DS_BASE = code_base;
		SS_BASE = native_stack_base;
		CPU_REMCLOCK = 100000;
		CPU_BASECLOCK = 100000;
		CPU_COMPAT_RETURN_PENDING = 1;
		if (upd9002_core_compat_state_load(saved_compat, sizeof(saved_compat)) != SUCCESS) {
			passed = 0;
		} else {
			CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
			upd9002_core_step();
			passed = (CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE) && (CPU_IP == native_offset) &&
			         (CPU_SP == 0x00f4);
			upd9002_core_step();
			passed = passed && (CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008) &&
			         (CPU_IP == compatible_offset + 18);
		}
	}
	upd9002_core_deinitialize();
	return passed ? SUCCESS : FAILURE;
}

// M118: a native interrupt ends a compatible-mode HALT; the IRET resumes
// at the instruction after it (V30 manual chapter 8; measured on the
// PC-88VA and PC-88VA2 with a compatible-mode test disk).
static int compat_halt_selftest(void) {
	const UINT16 code_segment = 0x2000;
	const UINT16 native_stack_segment = 0x3000;
	const UINT16 code_offset = 0x0100;
	const UINT16 compatible_offset = 0x1000;
	const UINT16 native_interrupt_offset = 0x3100;
	const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
	const UINT32 native_stack_base = static_cast<UINT32>(native_stack_segment) << 4;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_upd70008_register();
	upd9002_core_reset();
	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = native_stack_segment;
	CPU_IP = code_offset;
	CPU_SP = 0x0100;
	CPU_BP = 0x0200;
	CPU_FLAG = 0xf202;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = native_stack_base;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_CLOCK = 0;
	upd9002_memorywrite_w(0xe1 * 4, compatible_offset);
	upd9002_memorywrite_w(0xe1 * 4 + 2, code_segment);
	upd9002_memorywrite_w(0xe2 * 4, native_interrupt_offset);
	upd9002_memorywrite_w(0xe2 * 4 + 2, code_segment);
	static const UINT8 native_code[] = {0x0f, 0xff, 0xe1}; // BRKEM E1h
	static const UINT8 compatible_code[] = {
	    0xfb,       // EI
	    0x76,       // HALT
	    0x3e, 0x55, // LD A,55h
	    0xed, 0xfd, // RETEM
	};
	CopyMemory(mem + code_base + code_offset, native_code, sizeof(native_code));
	CopyMemory(mem + code_base + compatible_offset, compatible_code, sizeof(compatible_code));
	mem[code_base + native_interrupt_offset] = 0xcf; // IRET

	upd9002_core_step(); // BRKEM
	upd9002_core_step(); // EI
	upd9002_core_step(); // HALT
	for (unsigned i = 0; i < 4; i++) {
		upd9002_core_step(); // halted
	}
	bool ok = CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008 && CPU_IP == compatible_offset + 2 &&
	          CPU_AL == 0x00;
	upd9002_core_interrupt(0xe2);
	ok = ok && CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE && CPU_IP == native_interrupt_offset &&
	     upd9002_memoryread_w(native_stack_base + 0x00f4) == compatible_offset + 2;
	upd9002_core_step(); // IRET
	ok = ok && CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008 && CPU_IP == compatible_offset + 2;
	upd9002_core_step(); // LD A,55h
	ok = ok && CPU_AL == 0x55 && CPU_IP == compatible_offset + 4;
	upd9002_core_deinitialize();
	return ok ? SUCCESS : FAILURE;
}

// Compatible mode fetches every byte of an instruction (prefixes,
// opcode, displacement, immediate operands) through PS and addresses data
// through DS0 (V30 manual; measured on the PC-88VA and PC-88VA2 with
// pc88va-gg's start-up check). Here PS and DS0 hold different bytes at the
// same offsets: an operand read through DS0 gives FFh, a data read through
// PS gives 11h.
static int compat_split_fetch_selftest(void) {
	const UINT16 code_segment = 0x2000;
	const UINT16 data_segment = 0x4000;
	const UINT16 native_stack_segment = 0x3000;
	const UINT16 code_offset = 0x0100;
	const UINT16 compatible_offset = 0x1000;
	const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
	const UINT32 data_base = static_cast<UINT32>(data_segment) << 4;
	const UINT32 native_stack_base = static_cast<UINT32>(native_stack_segment) << 4;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_upd70008_register();
	upd9002_core_reset();
	CPU_CS = code_segment;
	CPU_DS = data_segment;
	CPU_SS = native_stack_segment;
	CPU_IP = code_offset;
	CPU_SP = 0x0100;
	CPU_BP = 0x0200;
	CPU_FLAG = 0xf202;
	CS_BASE = code_base;
	DS_BASE = data_base;
	SS_BASE = native_stack_base;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_CLOCK = 0;
	upd9002_memorywrite_w(0xe1 * 4, compatible_offset);
	upd9002_memorywrite_w(0xe1 * 4 + 2, code_segment);
	static const UINT8 native_code[] = {0x0f, 0xff, 0xe1}; // BRKEM E1h
	static const UINT8 compatible_code[] = {
	    0x3e, 0x5a,             // 1000 LD A,5Ah
	    0x21, 0x34, 0x12,       // 1002 LD HL,1234h
	    0xdd, 0x21, 0x00, 0x03, // 1005 LD IX,0300h
	    0xdd, 0x7e, 0x02,       // 1009 LD A,(IX+2)
	    0xed, 0x4b, 0x10, 0x03, // 100C LD BC,(0310h)
	    0xc3, 0x18, 0x10,       // 1010 JP 1018h
	    0x76, 0x76, 0x76, 0x76, 0x76, // 1013 HALT (not reached)
	    0x3e, 0x66,                   // 1018 LD A,66h
	};
	CopyMemory(mem + code_base + code_offset, native_code, sizeof(native_code));
	CopyMemory(mem + code_base + compatible_offset, compatible_code, sizeof(compatible_code));
	FillMemory(mem + data_base + compatible_offset, sizeof(compatible_code), 0xff);
	mem[code_base + 0x0302] = 0x11;
	mem[code_base + 0x0310] = 0x11;
	mem[code_base + 0x0311] = 0x11;
	mem[data_base + 0x0302] = 0xa5;
	mem[data_base + 0x0310] = 0xef;
	mem[data_base + 0x0311] = 0xbe;

	upd9002_core_step(); // BRKEM
	bool ok = CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008 && CPU_IP == compatible_offset;
	upd9002_core_step(); // LD A,5Ah
	ok = ok && CPU_AL == 0x5a;
	upd9002_core_step(); // LD HL,1234h
	ok = ok && CPU_BX == 0x1234;
	upd9002_core_step(); // LD IX,0300h
	ok = ok && CPU_SI == 0x0300;
	upd9002_core_step(); // LD A,(IX+2)
	ok = ok && CPU_AL == 0xa5;
	upd9002_core_step(); // LD BC,(0310h)
	ok = ok && CPU_CX == 0xbeef;
	upd9002_core_step(); // JP 1018h
	ok = ok && CPU_IP == compatible_offset + 0x18;
	upd9002_core_step(); // LD A,66h
	ok = ok && CPU_AL == 0x66 && CPU_IP == compatible_offset + 0x1a;
	upd9002_core_deinitialize();
	return ok ? SUCCESS : FAILURE;
}

extern "C" int upd9002_upd70008_compat_selftest(void) {
	const UINT8 encodings[] = {0xff, 0xfe};
	for (UINT8 encoding : encodings) {
		if (compat_entry_selftest(encoding) != SUCCESS) {
			std::fprintf(stderr, "compatible entry selftest failed: 0F %02X\n", encoding);
			return FAILURE;
		}
	}
	if (compat_halt_selftest() != SUCCESS) {
		std::fprintf(stderr, "compatible HALT selftest failed\n");
		return FAILURE;
	}
	if (compat_split_fetch_selftest() != SUCCESS) {
		std::fprintf(stderr, "compatible operand fetch selftest failed\n");
		return FAILURE;
	}
	return SUCCESS;
}

// M105/M132: real adapter execution, not only clock-ratio arithmetic.
static SINT32 compat_nop_clocks(UINT percent, BOOL mode88, BOOL standard, UINT8 io_opcode = 0) {
	const UINT8 saved_88 = memoryva_88_mode;
	const UINT8 saved_a = sysportva.a;
	const UINT saved_percent = pccore_cpu_speed();
	const UINT saved_machine_multiple = pccore.multiple;
	const UINT16 code_segment = 0x2000;
	const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
	SINT32 before;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_upd70008_register();
	upd9002_core_reset();
	memoryva_88_mode = mode88 ? 1 : 0;
	sysportva.a = (sysportva.a & ~0x20) | (standard ? 0 : 0x20);
	pccore_set_cpu_speed(percent);
	if (io_opcode != 0) {
		// The standalone CPU fixture need not have initialized the machine.
		pccore.multiple = PCCORE_STANDARD_MULTIPLE;
		iocore_build();
		iocore_bind(); // unassigned probe port, two native clocks per I/O
	}
	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = 0x3000;
	CPU_IP = 0x0100;
	CPU_SP = 0x0100;
	CPU_FLAG = 0xf202;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = 0x30000;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_CLOCK = 0;
	mem[(0x00e1U * 4) + 0] = 0x00;
	mem[(0x00e1U * 4) + 1] = 0x10;
	mem[(0x00e1U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e1U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	mem[code_base + 0x0100] = 0x0f; // BRKEM E1h
	mem[code_base + 0x0101] = 0xff;
	mem[code_base + 0x0102] = 0xe1;
	if (io_opcode != 0) {
		for (int i = 0; i < 10; i++) {
			mem[code_base + 0x1000 + i * 2] = io_opcode;
			mem[code_base + 0x1001 + i * 2] = 0x7e;
		}
	}
	upd9002_core_step();
	before = CPU_REMCLOCK;
	for (int i = 0; i < 10; i++) {
		upd9002_core_step(); // NOP, or immediate-port IN/OUT
	}
	before -= CPU_REMCLOCK;
	memoryva_88_mode = saved_88;
	sysportva.a = saved_a;
	pccore_set_cpu_speed(saved_percent);
	if (io_opcode != 0) {
		pccore.multiple = saved_machine_multiple;
		iocore_build();
		iocore_bind();
	}
	upd9002_core_deinitialize();
	return before;
}

extern "C" int upd9002_upd70008_cpu_speed_selftest(void) {
	const SINT32 full = compat_nop_clocks(100, FALSE, FALSE);
	const SINT32 half = compat_nop_clocks(50, FALSE, FALSE);
	const SINT32 high88 = compat_nop_clocks(100, TRUE, FALSE);
	const SINT32 standard88 = compat_nop_clocks(100, TRUE, TRUE);
	const SINT32 standard_half = compat_nop_clocks(50, TRUE, TRUE);
	const SINT32 native_standard = compat_nop_clocks(100, FALSE, TRUE);

	if ((full <= 0) || (half != full * 2) || high88 != full || standard88 != full * 2 ||
	    standard_half != full * 4 || native_standard != full) {
		std::fprintf(
		    stderr,
		    "compatible CPU speed selftest failed: base=%d half=%d H=%d S=%d S50=%d V3S=%d\n",
		    static_cast<int>(full), static_cast<int>(half), static_cast<int>(high88),
		    static_cast<int>(standard88), static_cast<int>(standard_half),
		    static_cast<int>(native_standard));
		return FAILURE;
	}
	const UINT8 io_opcodes[] = {0xdb, 0xd3};
	for (const UINT8 opcode : io_opcodes) {
		const SINT32 high = compat_nop_clocks(100, TRUE, FALSE, opcode);
		const SINT32 low = compat_nop_clocks(100, TRUE, TRUE, opcode);
		const SINT32 low50 = compat_nop_clocks(50, TRUE, TRUE, opcode);
		const SINT32 v3 = compat_nop_clocks(100, FALSE, TRUE, opcode);
		// Ten accesses charge 20 native clocks, never an H/S or CPU-speed ratio.
		if (high <= 20 || low != (high - 20) * 2 + 20 || low50 != (high - 20) * 4 + 20 ||
		    v3 != high) {
			std::fprintf(
			    stderr, "compatible native-wait selftest failed: op=%02x H=%d S=%d S50=%d V3S=%d\n",
			    opcode, high, low, low50, v3);
			return FAILURE;
		}
		std::fprintf(stderr, "selftest: uPD70008 IO %02x clocks H=%d S=%d S50=%d V3S=%d\n", opcode,
		             high, low, low50, v3);
	}
	std::fprintf(stderr, "selftest: uPD70008 NOP clocks H=%d S=%d S50=%d V3S=%d\n",
	             static_cast<int>(high88), static_cast<int>(standard88),
	             static_cast<int>(standard_half), static_cast<int>(native_standard));
	return SUCCESS;
}

// M103a: alternate register storage and the load-before-enter vector reader.
extern "C" int upd9002_upd70008_alt_regs_selftest(void) {
	const UINT16 code_segment = 0x2000;
	const UINT16 native_stack_segment = 0x3000;
	const UINT16 code_offset = 0x0100;
	const UINT16 compatible_offset = 0x1000;
	const UINT16 native_offset = 0x3000;
	const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
	const UINT32 native_stack_base = static_cast<UINT32>(native_stack_segment) << 4;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_upd70008_register();
	upd9002_core_reset();

	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = native_stack_segment;
	CPU_IP = code_offset;
	CPU_SP = 0x0100;
	CPU_BP = 0x0200;
	CPU_FLAG = 0xf202;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = native_stack_base;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_CLOCK = 0;

	// IVT: vector E1 = BRKEM target, E0 = CALLN target; bytes 30h/31h are
	// the word the undefined ED 20 reads with C = 30h.
	mem[(0x00e1U * 4) + 0] = static_cast<UINT8>(compatible_offset);
	mem[(0x00e1U * 4) + 1] = static_cast<UINT8>(compatible_offset >> 8);
	mem[(0x00e1U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e1U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	mem[(0x00e0U * 4) + 0] = static_cast<UINT8>(native_offset);
	mem[(0x00e0U * 4) + 1] = static_cast<UINT8>(native_offset >> 8);
	mem[(0x00e0U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e0U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	mem[0x30] = 0x00;
	mem[0x31] = 0x28;

	mem[code_base + code_offset + 0] = 0x0f; // BRKEM E1h
	mem[code_base + code_offset + 1] = 0xff;
	mem[code_base + code_offset + 2] = 0xe1;
	static const UINT8 program[] = {
	    0xaf,             // XOR A            F = 44h: N (bit 1) clear
	    0x01, 0x11, 0x11, // LD BC,1111h
	    0xd9,             // EXX
	    0x01, 0x22, 0x22, // LD BC,2222h
	    0xd9,             // EXX              BC=1111h, BC'=2222h
	    0xed, 0xed, 0xe0, // CALLN E0h
	    0xd9,             // EXX              BC = storage value
	    0x0e, 0x30,       // LD C,30h
	    0xed, 0x20,       // undefined: HL = IVT word at 30h
	    0xed, 0xfd,       // RETEM
	};
	for (size_t i = 0; i < sizeof(program); ++i) {
		mem[code_base + compatible_offset + i] = program[i];
	}
	mem[code_base + native_offset] = 0xcf; // IRET

	auto fail = [](int line) {
		fprintf(
		    stderr,
		    "alt-regs selftest failed at line %d: mode=%d CX=%04X BX=%04X alt.bc=%04X IP=%04X\n",
		    line, (int)CPU_COMPAT_MODE, CPU_CX, CPU_BX, upd9002_alt_regs.bc, CPU_IP);
		upd9002_core_deinitialize();
		return FAILURE;
	};

	upd9002_core_step(); // BRKEM
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008)
		return fail(__LINE__);
	upd9002_core_step(); // XOR A
	if ((CPU_FLAG & 0xff) != 0x44)
		return fail(__LINE__);
	upd9002_core_step(); // LD BC,1111h
	if (CPU_CX != 0x1111)
		return fail(__LINE__);
	upd9002_core_step(); // EXX: BC takes the reset value of BC'
	if (CPU_CX != 0xffff || upd9002_alt_regs.bc != 0x1111)
		return fail(__LINE__);
	upd9002_core_step(); // LD BC,2222h
	upd9002_core_step(); // EXX
	if (CPU_CX != 0x1111 || upd9002_alt_regs.bc != 0x2222)
		return fail(__LINE__);

	UINT8 blob[UPD9002_COMPAT_STATE_SIZE];
	if (upd9002_core_compat_state_save(blob, sizeof(blob)) != SUCCESS)
		return fail(__LINE__);

	upd9002_core_step(); // CALLN -> native
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE)
		return fail(__LINE__);
	// UPD9MODE: the return-pending SP is what lets IRET resume compatible
	// mode. Losing it (as UPD9CPU import alone did) breaks the return.
	UINT8 mode_state[UPD9002_MODE_STATE_SIZE];
	upd9002_core_mode_state_save(mode_state);
	static const UINT8 lost_sp[UPD9002_MODE_STATE_SIZE] = {0, 1, 0, 0};
	static const UINT8 invalid[][UPD9002_MODE_STATE_SIZE] = {
	    {2, 0, 0, 0}, {0, 2, 0, 0}, {1, 1, 0, 0}};
	for (const auto &bad : invalid) {
		if (upd9002_core_mode_state_valid(bad) || upd9002_core_mode_state_load(bad) == SUCCESS)
			return fail(__LINE__);
	}
	if (mode_state[0] != UPD9002_COMPAT_NATIVE || mode_state[1] != 1 ||
	    upd9002_core_mode_state_load(lost_sp) != SUCCESS || upd9002_core_compat_iret_is_return())
		return fail(__LINE__);
	if (upd9002_core_mode_state_load(mode_state) != SUCCESS ||
	    !upd9002_core_compat_iret_is_return())
		return fail(__LINE__);
	// Native code cannot address the alternate set, but the machine-owned
	// storage is the authority: a change there is what the compat core sees
	// when it resumes.
	upd9002_alt_regs.bc = 0x3333;
	upd9002_core_step(); // IRET -> resumes compatible mode
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008 || (CPU_FLAG & 0xff) != 0x44 || CPU_AL != 0)
		return fail(__LINE__);
	upd9002_core_step(); // EXX
	if (CPU_CX != 0x3333 || upd9002_alt_regs.bc != 0x1111)
		return fail(__LINE__);
	upd9002_core_step(); // LD C,30h
	upd9002_core_step(); // ED 20
	if (CPU_BX != 0x2800)
		return fail(__LINE__);
	upd9002_core_step(); // RETEM
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE)
		return fail(__LINE__);

	// Hardware reset sets the storage to all ones (M103c).
	upd9002_core_reset();
	if (upd9002_alt_regs.af != 0xffff || upd9002_alt_regs.bc != 0xffff ||
	    upd9002_alt_regs.de != 0xffff || upd9002_alt_regs.hl != 0xffff || upd9002_alt_regs_loaded)
		return fail(__LINE__);

	// Old state file: compat blob without a UPD9ALT section. The blob's
	// alternate set (BC' = 2222h) becomes the storage.
	CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
	if (upd9002_core_compat_state_load(blob, sizeof(blob)) != SUCCESS)
		return fail(__LINE__);
	if (upd9002_alt_regs.bc != 0x2222)
		return fail(__LINE__);

	// New state file: UPD9ALT loaded first, then the blob. The storage wins
	// and the load-before-enter path still reads the IVT (reader installed).
	upd9002_core_reset();
	upd9002_upd70008_register();
	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = native_stack_segment;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = native_stack_base;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
	upd9002_alt_regs.bc = 0x4444;
	upd9002_alt_regs_loaded = TRUE;
	if (upd9002_core_compat_state_load(blob, sizeof(blob)) != SUCCESS)
		return fail(__LINE__);
	// The blob's PC points at the CALLN; replace that code with EXX / LD C /
	// ED 20 / RETEM for this check.
	static const UINT8 tail[] = {0xd9, 0x0e, 0x30, 0xed, 0x20, 0xed, 0xfd};
	for (size_t i = 0; i < sizeof(tail); ++i) {
		mem[code_base + compatible_offset + 9 + i] = tail[i];
	}
	upd9002_core_step(); // EXX
	if (CPU_CX != 0x4444)
		return fail(__LINE__);
	upd9002_core_step(); // LD C,30h
	upd9002_core_step(); // ED 20 -- zero without the StateLoad reader fix
	if (CPU_BX != 0x2800)
		return fail(__LINE__);

	upd9002_core_deinitialize();
	return SUCCESS;
}
#endif
