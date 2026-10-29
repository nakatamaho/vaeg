/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Independently specified NEC/A-only contract, matched by the bounded VA2
// observations in M140. Exhaustive SOFTWARE input coverage is not exhaustive
// physical validation, and does not qualify the original VA or Zilog Q rules.
#include "cpu/upd780/upd780_core_config.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

class Harness final : public IMemoryAccess, public IIOAccess, public IClock, public IClockCounter {
  public:
	std::array<std::uint8_t, 65536> memory{};
	UPD780C cpu;
	std::uint32_t clock = 0;
	std::int32_t remain = 0;

	Harness() {
		if (!cpu.Init(this, this, this, this, 0x102)) {
			std::fputs("M140_VA2_INIT\n", stderr);
			std::exit(1);
		}
		// Prove the production configurator overrides the incompatible VA-main
		// profile; do not change the production instruction implementation.
		cpu.SetFlagProfile(Z80CompatFlagProfile::kUpd9002);
		Upd780ConfigureCore(&cpu);
	}
	std::uint32_t IFCALL Read8(std::uint32_t a) override {
		return memory[a & 0xffff];
	}
	void IFCALL Write8(std::uint32_t a, std::uint32_t v) override {
		memory[a & 0xffff] = static_cast<std::uint8_t>(v);
	}
	std::uint32_t IFCALL In(std::uint32_t) override {
		return 0xff;
	}
	void IFCALL Out(std::uint32_t, std::uint32_t) override {
	}
	std::uint32_t IFCALL now() override {
		return clock;
	}
	void IFCALL past(std::int32_t v) override {
		clock += v;
		remain -= v;
	}
	std::int32_t IFCALL GetRemainclock() override {
		return remain;
	}
	void IFCALL SetRemainclock(std::int32_t v) override {
		remain = v;
	}

	std::uint16_t Run(unsigned af, unsigned op, unsigned prefix) {
		cpu.Reset();
		Z80CompatReg reg{};
		reg.sp = 0x7eff;
		cpu.SetReg(reg);
		// Real guest POP/PUSH path, not SetReg(AF) alone.
		const std::uint8_t bytes[] = {
		    0x01, static_cast<std::uint8_t>(af),     static_cast<std::uint8_t>(af >> 8), 0xc5,
		    0xf1, static_cast<std::uint8_t>(prefix), static_cast<std::uint8_t>(op),      0xf5,
		    0xd1};
		for (unsigned i = 0; i < sizeof(bytes); ++i)
			memory[i] = bytes[i];
		// prefix=0 is NOP, an explicit flag-neutral boundary.
		for (unsigned i = 0; i < (prefix ? 6u : 7u); ++i)
			cpu.ExecOne();
		if (cpu.GetPC() != sizeof(bytes) || cpu.GetReg()->sp != 0x7eff ||
		    cpu.GetReg()->de != cpu.GetReg()->af) {
			std::fputs("M140_VA2_EXPORT_OR_FLOW\n", stderr);
			std::exit(1);
		}
		return cpu.GetReg()->de;
	}
};

std::uint16_t Expected(unsigned af, unsigned op) {
	const unsigned f = af & 255;
	return static_cast<std::uint16_t>((af & 0xff00) | (f & 0xc4) | ((af >> 8) & 0x28) |
	                                  (op == 0x37 ? 1 : (f & 1 ? 0x10 : 1)));
}

} // namespace

int main() {
	Harness h;
	unsigned cases = 0;
	for (unsigned af = 0; af < 65536; ++af) {
		for (const unsigned op : {0x37u, 0x3fu}) {
			for (const unsigned prefix : {0u, 0xddu, 0xfdu}) {
				const auto actual = h.Run(af, op, prefix);
				const auto expected = Expected(af, op);
				if (actual != expected) {
					std::fprintf(stderr,
					             "M140_VA2_AF af=%04X op=%02X prefix=%02X got=%04X expected=%04X\n",
					             af, op, prefix, actual, expected);
					return 1;
				}
				++cases;
			}
		}
	}
	// Known published-result anchors also discriminate OR and uPD9002 flags.
	if (h.Run(0x0028, 0x37, 0) != 0x0001 || h.Run(0x0029, 0x3f, 0) != 0x0010 ||
	    h.Run(0x28ff, 0x37, 0) != 0x28ed)
		return 1;
	std::printf("M140_VA2_FLAGS_PASS cases=%u (software exhaustive; bounded physical scope only)\n",
	            cases);
	return 0;
}
