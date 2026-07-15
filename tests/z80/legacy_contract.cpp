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

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

/* Test-only access is required to report the exact private revision-1 ABI. */
#define private public
#include "cpucva/z80c.h"
#undef private

namespace {

constexpr int kClockMultiple = 2;
constexpr int kAcknowledgePort = 0x102;

using LegacyStatus = Z80C::Status;
using StatusBytes = std::array<uint8_t, sizeof(LegacyStatus)>;

static_assert(sizeof(bool) == 1, "revision-1 fixtures require one-byte bool");
static_assert(sizeof(uint) == 4, "revision-1 fixtures require four-byte uint");
static_assert(sizeof(sint32) == 4,
              "revision-1 fixtures require four-byte sint32");
static_assert(sizeof(Z80Reg::wordreg) == 4,
              "revision-1 fixtures require four-byte word registers");
static_assert(sizeof(Z80Reg) == 56,
              "unexpected legacy public-register ABI");
static_assert(sizeof(LegacyStatus) == 68,
              "unexpected legacy status ABI");

struct BusEvent {
    char kind;
    uint32_t address;
    uint8_t data;

    bool operator==(const BusEvent &other) const {
        return kind == other.kind && address == other.address &&
               data == other.data;
    }
};

class Memory final : public IMemoryAccess {
public:
    std::array<uint8_t, 0x10000> bytes{};
    std::vector<BusEvent> events;

    uint IFCALL Read8(uint address) override {
        const uint16_t masked = static_cast<uint16_t>(address);
        const uint8_t data = bytes[masked];
        events.push_back({'r', masked, data});
        return data;
    }

    void IFCALL Write8(uint address, uint data) override {
        const uint16_t masked = static_cast<uint16_t>(address);
        const uint8_t value = static_cast<uint8_t>(data);
        bytes[masked] = value;
        events.push_back({'w', masked, value});
    }
};

class IO final : public IIOAccess {
public:
    uint8_t acknowledge = 0x7f;
    std::vector<BusEvent> events;

    uint IFCALL In(uint port) override {
        const uint8_t data = port == kAcknowledgePort ? acknowledge : 0xff;
        events.push_back({'i', port, data});
        return data;
    }

    void IFCALL Out(uint port, uint data) override {
        events.push_back(
            {'o', port, static_cast<uint8_t>(data)});
    }

    size_t acknowledge_count() const {
        size_t count = 0;
        for (const BusEvent &event : events) {
            if (event.kind == 'i' && event.address == kAcknowledgePort) {
                count++;
            }
        }
        return count;
    }
};

class Clock final : public IClock {
public:
    uint32_t value = 0;

    uint32 IFCALL now() override {
        return value;
    }
};

class ClockCounter final : public IClockCounter {
public:
    sint32 remain = 0;

    void IFCALL past(sint32 clocks) override {
        remain -= clocks * kClockMultiple;
    }

    sint32 IFCALL GetRemainclock() override {
        return remain;
    }

    void IFCALL SetRemainclock(sint32 clocks) override {
        remain = clocks;
    }
};

struct Machine {
    Memory memory;
    IO io;
    Clock clock;
    ClockCounter counter;
    Z80C cpu;

    Machine() {
        if (!cpu.Init(&memory, &io, &clock, &counter,
                      kAcknowledgePort)) {
            std::fputs("legacy-contract: Z80C::Init failed\n", stderr);
            std::abort();
        }
    }

    void install(std::initializer_list<uint8_t> program) {
        std::copy(program.begin(), program.end(), memory.bytes.begin());
    }

    void install_at(uint16_t address,
                    std::initializer_list<uint8_t> program) {
        std::copy(program.begin(), program.end(),
                  memory.bytes.begin() + address);
    }

    void exec_to(uint32_t now) {
        clock.value = now;
        cpu.Exec();
    }

    StatusBytes save() {
        LegacyStatus status;
        StatusBytes bytes;
        std::memset(&status, 0xa5, sizeof(status));
        if (cpu.GetStatusSize() != sizeof(status) ||
            !cpu.SaveStatus(reinterpret_cast<uint8_t *>(&status))) {
            std::fputs("legacy-contract: save failed\n", stderr);
            std::abort();
        }
        std::memcpy(bytes.data(), &status, sizeof(status));
        return bytes;
    }
};

struct Fixture {
    std::string name;
    StatusBytes status;
    std::array<uint8_t, 0x10000> memory;
    uint8_t acknowledge;
};

struct ExpectedFixture {
    const char *name;
    const char *hex;
};

constexpr ExpectedFixture kExpectedFixtures[] = {
    {"ordinary", "20330000bc9a0000785600003412000000000000000000000090000000000000000000000000000000000000110000000007000000000000000033010000000074000000"},
    {"alternate", "0000000000000000000000000000000057130000682400000090000045de0000bc9a000078560000341200001a000000000d0000000000000000000100000000b6000000"},
    {"im0-iff-enabled", "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000050000000005000001010000000000010000000028000000"},
    {"im1-iff-enabled", "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000050000000005000101010000000000010000000028000000"},
    {"im2-iff-enabled", "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000050000000005000201010000000000010000000028000000"},
    {"iff2-only", "440000000000000000000000000000000000000000000000fe8f00000000000000000000000000000000000066000000000400000001000000000001eaffffff2c000000"},
    {"halt", "440000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000200000000000000010001f8ffffff10000000"},
    {"external-wait", "440000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000100000000000000020001f3ffffff15000000"},
    {"asserted-irq-after-exec", "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000002000000000000010000010000000010000000"},
    {"negative-remainclock", "440000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000200000000000000000001f9ffffff09000000"},
    {"nonzero-lastclock", "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000002000000000000000000010000000010000000"},
    {"ei-nop", "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000030000000003000001010000000000010000000018000000"},
    {"ei-nop-accepted-irq", "440000000000000000000000000000000000000000000000000000000000000000000000000000000000000003000000000400000000000001000001deffffff18000000"},
    {"ei-di", "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000002000000000000010000010000000010000000"},
    {"ei-ei", "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000002000000000000010000010000000010000000"},
};

[[noreturn]] void fail(const std::string &message) {
    std::fprintf(stderr, "legacy-contract: %s\n", message.c_str());
    std::exit(1);
}

LegacyStatus decode(const StatusBytes &bytes) {
    LegacyStatus status;
    std::memcpy(&status, bytes.data(), sizeof(status));
    return status;
}

bool load(Z80C &cpu, const StatusBytes &bytes) {
    LegacyStatus status;
    std::memcpy(&status, bytes.data(), sizeof(status));
    return cpu.LoadStatus(reinterpret_cast<const uint8_t *>(&status));
}

std::string hex(const StatusBytes &bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const uint8_t byte : bytes) {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 0x0f]);
    }
    return result;
}

void require(bool condition, const std::string &message) {
    if (!condition) {
        fail(message);
    }
}

void prime_flags(Machine &machine) {
    machine.install({0xaf}); // XOR A initializes architectural and lazy flags.
    machine.exec_to(8);
    require(machine.counter.remain == 0 && machine.cpu.GetPC() == 1,
            "flag initialization boundary mismatch");
}

Fixture capture(const char *name, Machine &machine) {
    return {name, machine.save(), machine.memory.bytes,
            machine.io.acknowledge};
}

Fixture ordinary_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {
        0x31, 0x00, 0x90,       // LD SP,$9000
        0x01, 0x34, 0x12,       // LD BC,$1234
        0x11, 0x78, 0x56,       // LD DE,$5678
        0x21, 0xbc, 0x9a,       // LD HL,$9abc
        0x3e, 0x11,             // LD A,$11
        0xc6, 0x22,             // ADD A,$22 (leaves lazy flags)
    });
    machine.exec_to(116);
    require(machine.counter.remain == 0, "ordinary fixture clock balance");
    return capture("ordinary", machine);
}

Fixture alternate_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {
        0x31, 0x00, 0x90,       // LD SP,$9000
        0x01, 0x34, 0x12,       // LD BC,$1234
        0x11, 0x78, 0x56,       // LD DE,$5678
        0x21, 0xbc, 0x9a,       // LD HL,$9abc
        0x3e, 0xde,             // LD A,$de
        0x37,                   // SCF
        0xd9,                   // EXX
        0x08,                   // EX AF,AF'
        0xdd, 0x21, 0x57, 0x13, // LD IX,$1357
        0xfd, 0x21, 0x68, 0x24, // LD IY,$2468
    });
    machine.exec_to(182);
    require(machine.counter.remain == 0, "alternate fixture clock balance");
    return capture("alternate", machine);
}

Fixture interrupt_mode_fixture(const char *name, uint8_t opcode) {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {0xfb, 0x00, 0xed, opcode}); // EI; NOP; IM n
    machine.exec_to(40);
    require(machine.counter.remain == 0,
            std::string(name) + " fixture clock balance");
    return capture(name, machine);
}

Fixture iff2_only_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {
        0xfb, 0x00,             // EI; NOP
        0x31, 0x00, 0x90,       // LD SP,$9000
    });
    machine.exec_to(44);
    machine.cpu.NMI();
    require(machine.cpu.GetReg()->iff1 == false &&
                machine.cpu.GetReg()->iff2 == true,
            "NMI did not produce the IFF2-only boundary");
    return capture("iff2-only", machine);
}

Fixture halt_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {0x76}); // HALT
    machine.exec_to(16);
    return capture("halt", machine);
}

Fixture wait_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.cpu.Wait(true);
    machine.exec_to(21);
    return capture("external-wait", machine);
}

Fixture asserted_irq_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {0x00}); // NOP
    machine.exec_to(16);
    machine.cpu.IRQ(0, 1);   // Assert only after Exec() has returned.
    require(machine.io.acknowledge_count() == 0,
            "asserting IRQ after Exec performed acknowledge read");
    return capture("asserted-irq-after-exec", machine);
}

Fixture negative_remain_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {0x00}); // NOP
    machine.exec_to(9);
    require(machine.counter.remain == -7,
            "negative remainclock fixture did not overshoot");
    return capture("negative-remainclock", machine);
}

Fixture nonzero_lastclock_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {0x00}); // NOP
    machine.exec_to(16);
    return capture("nonzero-lastclock", machine);
}

Fixture ei_nop_fixture(bool irq) {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {0xfb, 0x00, 0x00}); // EI; NOP; NOP
    if (irq) {
        machine.cpu.IRQ(0, 1);
    }
    machine.exec_to(24);
    if (irq) {
        require(machine.io.acknowledge_count() == 1,
                "EI/NOP accepted IRQ without exactly one acknowledge");
    }
    return capture(irq ? "ei-nop-accepted-irq" : "ei-nop", machine);
}

Fixture ei_di_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {0xfb, 0xf3, 0x00}); // EI; DI; NOP
    machine.cpu.IRQ(0, 1);
    machine.exec_to(16);
    require(machine.cpu.GetPC() == 2 &&
                machine.io.acknowledge_count() == 0,
            "EI/DI return boundary mismatch");
    return capture("ei-di", machine);
}

Fixture ei_ei_fixture() {
    Machine machine;
    prime_flags(machine);
    machine.install_at(1, {0xfb, 0xfb, 0x00}); // EI; EI; NOP
    machine.cpu.IRQ(0, 1);
    machine.exec_to(16);
    require(machine.cpu.GetPC() == 2 &&
                machine.io.acknowledge_count() == 0,
            "EI/EI return boundary mismatch");
    return capture("ei-ei", machine);
}

std::vector<Fixture> make_fixtures() {
    std::vector<Fixture> fixtures;
    fixtures.push_back(ordinary_fixture());
    fixtures.push_back(alternate_fixture());
    fixtures.push_back(interrupt_mode_fixture("im0-iff-enabled", 0x46));
    fixtures.push_back(interrupt_mode_fixture("im1-iff-enabled", 0x56));
    fixtures.push_back(interrupt_mode_fixture("im2-iff-enabled", 0x5e));
    fixtures.push_back(iff2_only_fixture());
    fixtures.push_back(halt_fixture());
    fixtures.push_back(wait_fixture());
    fixtures.push_back(asserted_irq_fixture());
    fixtures.push_back(negative_remain_fixture());
    fixtures.push_back(nonzero_lastclock_fixture());
    fixtures.push_back(ei_nop_fixture(false));
    fixtures.push_back(ei_nop_fixture(true));
    fixtures.push_back(ei_di_fixture());
    fixtures.push_back(ei_ei_fixture());
    return fixtures;
}

void verify_immediate_round_trip(const Fixture &fixture) {
    Machine restored;
    restored.memory.bytes = fixture.memory;
    restored.io.acknowledge = fixture.acknowledge;
    require(load(restored.cpu, fixture.status),
            fixture.name + " LoadStatus failed");
    require(restored.save() == fixture.status,
            fixture.name + " immediate save/load bytes differ");
}

void compare_continuation(const Fixture &fixture, uint32_t clock_delta,
                          bool assert_irq) {
    Machine baseline;
    Machine restored;
    baseline.memory.bytes = fixture.memory;
    restored.memory.bytes = fixture.memory;
    baseline.io.acknowledge = fixture.acknowledge;
    restored.io.acknowledge = fixture.acknowledge;
    require(load(baseline.cpu, fixture.status),
            fixture.name + " baseline load failed");
    require(load(restored.cpu, fixture.status),
            fixture.name + " restored load failed");
    if (assert_irq) {
        baseline.cpu.IRQ(0, 1);
        restored.cpu.IRQ(0, 1);
    }
    const uint32_t next_clock =
        static_cast<uint32_t>(decode(fixture.status).lastclock) + clock_delta;
    baseline.memory.events.clear();
    baseline.io.events.clear();
    restored.memory.events.clear();
    restored.io.events.clear();
    baseline.exec_to(next_clock);
    restored.exec_to(next_clock);
    require(baseline.save() == restored.save(),
            fixture.name + " continuation status mismatch");
    require(baseline.memory.bytes == restored.memory.bytes,
            fixture.name + " continuation memory mismatch");
    require(baseline.memory.events == restored.memory.events,
            fixture.name + " continuation memory trace mismatch");
    require(baseline.io.events == restored.io.events,
            fixture.name + " continuation I/O trace mismatch");
}

void verify_fixtures(const std::vector<Fixture> &fixtures) {
    require(fixtures.size() ==
                sizeof(kExpectedFixtures) / sizeof(kExpectedFixtures[0]),
            "fixture count differs from the retained baseline");
    for (const Fixture &fixture : fixtures) {
        const LegacyStatus status = decode(fixture.status);
        require(status.rev == 1, fixture.name + " revision is not one");
        require(status.remainclock <= 0,
                fixture.name + " exposed a positive remainclock");
        verify_immediate_round_trip(fixture);
        bool found = false;
        for (const ExpectedFixture &expected : kExpectedFixtures) {
            if (fixture.name == expected.name) {
                require(hex(fixture.status) == expected.hex,
                        fixture.name + " bytes differ from retained baseline");
                found = true;
                break;
            }
        }
        require(found, fixture.name + " has no retained baseline");
    }

    for (const Fixture &fixture : fixtures) {
        if (fixture.name == "ei-nop") {
            compare_continuation(fixture, 8, true);
        } else if (fixture.name == "ei-nop-accepted-irq") {
            compare_continuation(fixture, 8, false);
        } else if (fixture.name == "ei-di") {
            compare_continuation(fixture, 8, false);
        } else if (fixture.name == "ei-ei") {
            compare_continuation(fixture, 16, false);
        }
    }
}

void print_offset(const char *name, size_t offset) {
    std::printf("offset.%s=%zu\n", name, offset);
}

void print_abi() {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    const char *byte_order = "little";
#elif defined(_WIN32)
    const char *byte_order = "little";
#else
    const char *byte_order = "unknown";
#endif

#if defined(__clang__)
    const char *compiler = "clang " __clang_version__;
#elif defined(__GNUC__)
    const char *compiler = "gcc " __VERSION__;
#elif defined(_MSC_VER)
    const char *compiler = "msvc";
#else
    const char *compiler = "unknown";
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    const char *architecture = "aarch64";
#elif defined(__x86_64__) || defined(_M_X64)
    const char *architecture = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    const char *architecture = "x86";
#else
    const char *architecture = "unknown";
#endif

    std::printf("abi.compiler=%s\n", compiler);
    std::printf("abi.architecture=%s\n", architecture);
    std::printf("abi.byte_order=%s\n", byte_order);
    std::printf("sizeof.bool=%zu\n", sizeof(bool));
    std::printf("sizeof.uint=%zu\n", sizeof(uint));
    std::printf("sizeof.sint32=%zu\n", sizeof(sint32));
    std::printf("sizeof.uint8=%zu\n", sizeof(uint8));
    std::printf("sizeof.uint16=%zu\n", sizeof(uint16));
    std::printf("sizeof.uint32=%zu\n", sizeof(uint32));
    std::printf("sizeof.pointer=%zu\n", sizeof(void *));
    std::printf("sizeof.z80_wordreg=%zu\n", sizeof(Z80Reg::wordreg));
    std::printf("sizeof.z80_reg=%zu\n", sizeof(Z80Reg));
    std::printf("sizeof.z80_status=%zu\n", sizeof(LegacyStatus));

    const size_t reg = offsetof(LegacyStatus, reg);
    const size_t words = offsetof(Z80Reg, r) + offsetof(Z80Reg::regs, w);
    const size_t bytes = offsetof(Z80Reg, r) + offsetof(Z80Reg::regs, b);
    print_offset("reg.r.w.af", reg + words +
                                  offsetof(Z80Reg::regs::shorts, af));
    print_offset("reg.r.w.hl", reg + words +
                                  offsetof(Z80Reg::regs::shorts, hl));
    print_offset("reg.r.w.de", reg + words +
                                  offsetof(Z80Reg::regs::shorts, de));
    print_offset("reg.r.w.bc", reg + words +
                                  offsetof(Z80Reg::regs::shorts, bc));
    print_offset("reg.r.w.ix", reg + words +
                                  offsetof(Z80Reg::regs::shorts, ix));
    print_offset("reg.r.w.iy", reg + words +
                                  offsetof(Z80Reg::regs::shorts, iy));
    print_offset("reg.r.w.sp", reg + words +
                                  offsetof(Z80Reg::regs::shorts, sp));
    print_offset("reg.r.b.flags", reg + bytes +
                                     offsetof(Z80Reg::regs::words, flags));
    print_offset("reg.r.b.a", reg + bytes +
                                 offsetof(Z80Reg::regs::words, a));
    print_offset("reg.r.b.l", reg + bytes +
                                 offsetof(Z80Reg::regs::words, l));
    print_offset("reg.r.b.h", reg + bytes +
                                 offsetof(Z80Reg::regs::words, h));
    print_offset("reg.r.b.e", reg + bytes +
                                 offsetof(Z80Reg::regs::words, e));
    print_offset("reg.r.b.d", reg + bytes +
                                 offsetof(Z80Reg::regs::words, d));
    print_offset("reg.r.b.c", reg + bytes +
                                 offsetof(Z80Reg::regs::words, c));
    print_offset("reg.r.b.b", reg + bytes +
                                 offsetof(Z80Reg::regs::words, b));
    print_offset("reg.r.b.xl", reg + bytes +
                                  offsetof(Z80Reg::regs::words, xl));
    print_offset("reg.r.b.xh", reg + bytes +
                                  offsetof(Z80Reg::regs::words, xh));
    print_offset("reg.r.b.yl", reg + bytes +
                                  offsetof(Z80Reg::regs::words, yl));
    print_offset("reg.r.b.yh", reg + bytes +
                                  offsetof(Z80Reg::regs::words, yh));
    print_offset("reg.r.b.spl", reg + bytes +
                                   offsetof(Z80Reg::regs::words, spl));
    print_offset("reg.r.b.sph", reg + bytes +
                                   offsetof(Z80Reg::regs::words, sph));
    print_offset("reg.r_af", reg + offsetof(Z80Reg, r_af));
    print_offset("reg.r_hl", reg + offsetof(Z80Reg, r_hl));
    print_offset("reg.r_de", reg + offsetof(Z80Reg, r_de));
    print_offset("reg.r_bc", reg + offsetof(Z80Reg, r_bc));
    print_offset("reg.pc", reg + offsetof(Z80Reg, pc));
    print_offset("reg.ireg", reg + offsetof(Z80Reg, ireg));
    print_offset("reg.rreg", reg + offsetof(Z80Reg, rreg));
    print_offset("reg.rreg7", reg + offsetof(Z80Reg, rreg7));
    print_offset("reg.intmode", reg + offsetof(Z80Reg, intmode));
    print_offset("reg.iff1", reg + offsetof(Z80Reg, iff1));
    print_offset("reg.iff2", reg + offsetof(Z80Reg, iff2));
    print_offset("intr", offsetof(LegacyStatus, intr));
    print_offset("wait", offsetof(LegacyStatus, wait));
    print_offset("xf", offsetof(LegacyStatus, xf));
    print_offset("rev", offsetof(LegacyStatus, rev));
    print_offset("remainclock", offsetof(LegacyStatus, remainclock));
    print_offset("lastclock", offsetof(LegacyStatus, lastclock));
}

} // namespace

int main() {
    const std::vector<Fixture> fixtures = make_fixtures();
    verify_fixtures(fixtures);
    print_abi();
    for (const Fixture &fixture : fixtures) {
        const LegacyStatus status = decode(fixture.status);
        std::printf("fixture.%s.pc=%04x\n", fixture.name.c_str(),
                    static_cast<unsigned int>(status.reg.pc & 0xffff));
        std::printf("fixture.%s.remainclock=%d\n", fixture.name.c_str(),
                    status.remainclock);
        std::printf("fixture.%s.lastclock=%d\n", fixture.name.c_str(),
                    status.lastclock);
        std::printf("fixture.%s.hex=%s\n", fixture.name.c_str(),
                    hex(fixture.status).c_str());
    }
    std::printf("legacy-contract: %zu fixtures passed\n", fixtures.size());
    return 0;
}
