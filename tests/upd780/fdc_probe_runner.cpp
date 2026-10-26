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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
// Execute the IDENTICAL RAM payload with a synthetic monitor transport.
// Not a ROM/FDC integration test, independent Zilog oracle or physical timing.
#include "cpu/upd780/upd780_core_config.h"
#include "cpu/z80_compat_cpu.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>

struct Machine final : IMemoryAccess, IIOAccess, IClock, IClockCounter {
	UPD780C cpu;
	std::array<std::uint8_t, 65536> memory{};
	std::uint32_t clock = 0;
	std::int32_t remain = 0;
	std::uint64_t consumed = 0, start = 0;
	bool failed = false, done = false, timing = false;
	unsigned frames = 0, motor_stops = 0;
	std::uint32_t IFCALL Read8(std::uint32_t a) override {
		if (a >= 0x4000 && a < 0x7f00)
			return memory[a];
		if (cpu.CodeRead() &&
		    ((a >= 0x18 && a <= 0x1a) || a == 0x2a || (a == 0x2b && cpu.Halted())))
			return memory[a];
		std::fprintf(stderr, "M139_FDC_MAP_READ address=%08x code=%d\n", a, cpu.CodeRead());
		failed = true;
		remain = 0;
		return 0xff;
	}
	void IFCALL Write8(std::uint32_t a, std::uint32_t v) override {
		if ((a >= 0x4000 && a < 0x7f00) || (a == 0x7f09 && v == 0)) {
			memory[a] = v;
			return;
		}
		std::fprintf(stderr, "M139_FDC_MAP_WRITE address=%08x\n", a);
		failed = true;
		remain = 0;
	}
	std::uint32_t IFCALL In(std::uint32_t) override {
		failed = true;
		return 0xff;
	}
	void IFCALL Out(std::uint32_t p, std::uint32_t v) override {
		if (p == 0xf8 && v == 0) {
			++motor_stops;
			return;
		}
		if (p != 0 || cpu.GetPC() != 0x1a) {
			failed = true;
			return;
		}
		switch (v) {
		case 1:
			if (timing)
				failed = true;
			timing = true;
			start = consumed;
			break;
		case 2:
			if (!timing)
				failed = true;
			timing = false;
			++frames;
			std::cout << " MODEL_T=" << consumed - start << "\r\n";
			break;
		case 4:
			done = true;
			break;
		case 5:
			break;
		default:
			std::cout.put(static_cast<char>(v));
			break;
		}
	}
	std::uint32_t IFCALL now() override {
		return clock;
	}
	void IFCALL past(std::int32_t n) override {
		remain -= n;
		consumed += n;
	}
	std::int32_t IFCALL GetRemainclock() override {
		return remain;
	}
	void IFCALL SetRemainclock(std::int32_t n) override {
		remain = n;
	}
};
int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "usage: fdc_probe_runner payload.bin\n";
		return 2;
	}
	Machine m;
	std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
	if (!file || file.tellg() <= 0 || file.tellg() >= 0x3000) {
		std::cerr << "M139_FDC_PAYLOAD_SIZE\n";
		return 2;
	}
	const auto size = file.tellg();
	file.seekg(0);
	file.read(reinterpret_cast<char *>(m.memory.data() + 0x4000), size);
	// Independent tiny synthetic monitor: OUT (0),A; RET and HALT.
	// Every operand read outside RAM fails; these are the ONLY allowed ROM fetches.
	m.memory[0x18] = 0xd3;
	m.memory[0x19] = 0;
	m.memory[0x1a] = 0xc9;
	m.memory[0x2a] = 0x76;
	Upd780ConfigureCore(&m.cpu);
	if (!m.cpu.Init(&m, &m, &m, &m, 0x102)) {
		std::cerr << "M139_FDC_INIT\n";
		return 2;
	}
	m.cpu.SetPC(0x4000);
	const auto begin = std::chrono::steady_clock::now();
	while (!m.failed && !m.cpu.Halted() && m.consumed < 60000000000ULL) {
		m.clock += 1000000;
		m.cpu.Exec();
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(600))
			break;
	}
	if (m.failed || !m.done || !m.cpu.Halted() || m.cpu.GetPC() != 0x2b || m.timing ||
	    m.motor_stops != 1) {
		std::cerr << "M139_FDC_HOST_EXEC_FAILED pc=" << std::hex << m.cpu.GetPC() << "\n";
		return 1;
	}
	std::cerr << "M139_FDC_HOST_COMPLETE clock=" << m.consumed << " timing_records=" << m.frames
	          << "\n";
	return 0;
}
