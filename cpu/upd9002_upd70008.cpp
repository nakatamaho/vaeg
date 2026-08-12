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
#include "cpu/upd9002_upd70008.h"
#include "cpu/z80_compat_cpu.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

static std::uint32_t compat_trace_slot() {
    static bool initialized = false;
    static std::uint32_t limit = 0;
    static std::uint32_t count = 0;
    if (!initialized) {
        const char *value = std::getenv("VAEG_UPD70008_TRACE");
        if (value != nullptr && value[0] != '\0') {
            char *end = nullptr;
            unsigned long parsed = std::strtoul(value, &end, 10);
            if (end != value && *end == '\0' && parsed != 0) {
                limit = parsed > 1000000UL ? 1000000U :
                    static_cast<std::uint32_t>(parsed);
            }
            else {
                limit = 4096;
            }
        }
        initialized = true;
    }
    if (count >= limit) {
        return UINT32_MAX;
    }
    return count++;
}

static void compat_trace(const char *event, std::uint32_t slot,
                         std::uint8_t op0, std::uint8_t op1) {
    if (slot == UINT32_MAX) {
        return;
    }
    std::fprintf(stderr,
        "m76-compat-trace event=%s slot=%u cs=%04x ip=%04x "
        "op=%02x/%02x af=%02x%02x bc=%04x de=%04x hl=%04x "
        "ix=%04x iy=%04x sp=%04x nsp=%04x ss=%04x stk=%04x/%04x/%04x "
        "flags=%04x rem=%d\n",
        event, slot, CPU_CS, CPU_IP, op0, op1, CPU_AL,
        static_cast<unsigned>(CPU_FLAG & 0xff), CPU_CX, CPU_DX, CPU_BX,
        CPU_SI, CPU_DI, CPU_BP, CPU_SP, CPU_SS,
        static_cast<unsigned>(mem[(SS_BASE + CPU_SP) & CPU_ADRSMASK]) |
            (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 1) & 0xffff)) & CPU_ADRSMASK]) << 8),
        static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 2) & 0xffff)) & CPU_ADRSMASK]) |
            (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 3) & 0xffff)) & CPU_ADRSMASK]) << 8),
        static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 4) & 0xffff)) & CPU_ADRSMASK]) |
            (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 5) & 0xffff)) & CPU_ADRSMASK]) << 8),
        CPU_FLAG, CPU_REMCLOCK);
}

class CompatCounter final : public IClockCounter {
public:
    void IFCALL past(std::int32_t clocks) override {
        remain_ -= clocks;
    }

    std::int32_t IFCALL GetRemainclock() override {
        return remain_;
    }

    void IFCALL SetRemainclock(std::int32_t clocks) override {
        remain_ = clocks;
    }

private:
    std::int32_t remain_ = 0;
};

class CompatClock final : public IClock {
public:
    std::uint32_t IFCALL now() override {
        return CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
    }
};

class Upd9002Upd70008Compat final : public IMemoryAccess, public IIOAccess {
public:
    void Reset() {
        if (initialized_) {
            upd70008_.Reset();
        }
        upd70008_.SetMemoryBases(0, 0);
        initialized_ = false;
        have_compatible_state_ = false;
        counter_.SetRemainclock(0);
    }

    void Enter() {
        if (!initialized_) {
            initialized_ = upd70008_.Init(this, this, &clock_, &counter_, 0);
            if (!initialized_) {
                return;
            }
        }
        SetMemoryBases();
        UPD70008Reg state = *upd70008_.GetReg();
        state.af = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(CPU_AL) << 8) |
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
            upd70008_.SetMainReg(state);
        }
        else {
            upd70008_.SetReg(state);
            have_compatible_state_ = true;
        }
        counter_.SetRemainclock(CPU_REMCLOCK);
        const std::uint32_t trace_slot = compat_trace_slot();
        compat_trace("enter", trace_slot, 0, 0);
    }

    void Step() {
        if (!initialized_) {
            return;
        }
        counter_.SetRemainclock(CPU_REMCLOCK);
        const std::uint16_t pc = static_cast<std::uint16_t>(upd70008_.GetPC());
        const std::uint32_t code_address = (CPU_CS << 4) + pc;
        const std::uint8_t op0 = static_cast<std::uint8_t>(
            upd9002_memoryread(code_address));
        const std::uint8_t op1 = static_cast<std::uint8_t>(
            upd9002_memoryread(code_address + 1));
        const std::uint32_t trace_slot = compat_trace_slot();
        compat_trace("before", trace_slot, op0, op1);

        if ((op0 == 0xed) && (op1 == 0xed)) {
            const std::uint8_t vector = static_cast<std::uint8_t>(
                upd9002_memoryread(code_address + 2));
            upd70008_.SetPC(static_cast<std::uint16_t>(pc + 3));
            SyncToNative();
            upd9002_core_compat_calln(vector, static_cast<REG16>(pc + 3));
            counter_.SetRemainclock(CPU_REMCLOCK);
            compat_trace("calln", trace_slot, op0, op1);
            return;
        }
        if ((op0 == 0xed) && (op1 == 0xfd)) {
            upd70008_.SetPC(static_cast<std::uint16_t>(pc + 2));
            SyncToNative();
            upd9002_core_compat_retem();
            counter_.SetRemainclock(CPU_REMCLOCK);
            compat_trace("retem", trace_slot, op0, op1);
            return;
        }

        upd70008_.ExecOne();
        SyncToNative();
        CPU_REMCLOCK = counter_.GetRemainclock();
        compat_trace("after", trace_slot, op0, op1);
    }

    void SyncToNative() {
        const UPD70008Reg *state = upd70008_.GetReg();
        CPU_AL = static_cast<UINT8>(state->af >> 8);
        CPU_FLAG = static_cast<UINT16>((CPU_FLAG & 0xfc00) |
                                       (state->af & 0xff) |
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
        UPD70008Reg state = *upd70008_.GetReg();
        state.af = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(CPU_AL) << 8) |
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
        upd70008_.SetMainReg(state);
        counter_.SetRemainclock(CPU_REMCLOCK);
    }

    int StateSave(UINT8 *buffer, UINT size) {
        if ((buffer == NULL) || (size != UPD9002_COMPAT_STATE_SIZE) ||
                !initialized_) {
            return FAILURE;
        }
        counter_.SetRemainclock(CPU_REMCLOCK);
        return upd70008_.SaveStatus(buffer) ? SUCCESS : FAILURE;
    }

    int StateLoad(const UINT8 *buffer, UINT size) {
        if ((buffer == NULL) || (size != UPD9002_COMPAT_STATE_SIZE)) {
            return FAILURE;
        }
        if (!initialized_) {
            initialized_ = upd70008_.Init(this, this, &clock_, &counter_, 0);
            if (!initialized_) {
                return FAILURE;
            }
        }
        if (!upd70008_.LoadStatus(buffer)) {
            return FAILURE;
        }
        SetMemoryBases();
        have_compatible_state_ = true;
        CPU_REMCLOCK = counter_.GetRemainclock();
        return SUCCESS;
    }

    std::uint32_t IFCALL Read8(std::uint32_t address) override {
        return upd9002_memoryread(address & CPU_ADRSMASK);
    }

    void IFCALL Write8(std::uint32_t address, std::uint32_t data) override {
        upd9002_memorywrite(address & CPU_ADRSMASK,
                            static_cast<UINT8>(data));
    }

    std::uint32_t IFCALL In(std::uint32_t port) override {
        const SINT32 before = CPU_REMCLOCK;
        const UINT8 value = iocore_inp8(static_cast<UINT>(port & 0xffff));
        const SINT32 elapsed = before - CPU_REMCLOCK;
        if (elapsed > 0) {
            counter_.past(elapsed);
        }
        return value;
    }

    void IFCALL Out(std::uint32_t port, std::uint32_t data) override {
        const SINT32 before = CPU_REMCLOCK;
        iocore_out8(static_cast<UINT>(port & 0xffff),
                    static_cast<UINT8>(data));
        const SINT32 elapsed = before - CPU_REMCLOCK;
        if (elapsed > 0) {
            counter_.past(elapsed);
        }
    }

private:
    void SetMemoryBases() {
        upd70008_.SetMemoryBases(static_cast<std::uint32_t>(CPU_CS) << 4,
                            static_cast<std::uint32_t>(CPU_DS) << 4);
    }

    UPD70008C upd70008_;
    CompatCounter counter_;
    CompatClock clock_;
    bool initialized_ = false;
    bool have_compatible_state_ = false;
};

Upd9002Upd70008Compat compat;

void compat_reset() { compat.Reset(); }
void compat_enter() { compat.Enter(); }
void compat_step() { compat.Step(); }
void compat_sync_to_native() { compat.SyncToNative(); }
void compat_leave() {}
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
    upd9002_core_set_compat_hooks(&hooks);
    registered = true;
}

#if defined(VAEG_UPD9002_M76_TESTING)
extern "C" int upd9002_upd70008_compat_selftest(void) {
    const UINT16 code_segment = 0x2000;
    const UINT16 native_stack_segment = 0x3000;
    const UINT16 code_offset = 0x0100;
    const UINT16 compatible_offset = 0x1000;
    const UINT16 native_offset = 0x3000;
    const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
    const UINT32 native_stack_base =
        static_cast<UINT32>(native_stack_segment) << 4;

    upd9002_core_initialize();
    upd9002_memorymap(0);
    memmode_va = 0;
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
    mem[code_base + code_offset + 1] = 0xff;
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
    if ((CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) ||
            (CPU_CS != code_segment) || (CPU_IP != compatible_offset) ||
            (CPU_SP != 0x00fa)) {
        upd9002_core_deinitialize();
        return FAILURE;
    }
    upd9002_core_step();
    if (CPU_AL != 0x42 || CPU_IP != compatible_offset + 2) {
        upd9002_core_deinitialize();
        return FAILURE;
    }
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
    if (upd9002_core_compat_state_save(saved_compat, sizeof(saved_compat)) !=
            SUCCESS) {
        upd9002_core_deinitialize();
        return FAILURE;
    }
    upd9002_core_step();
    if ((CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE) ||
            (CPU_IP != native_offset) || (CPU_SP != 0x00f4)) {
        upd9002_core_deinitialize();
        return FAILURE;
    }
    upd9002_core_interrupt(0xe2);
    upd9002_core_step();
    if ((CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE) ||
            (CPU_IP != native_offset) || (CPU_SP != 0x00f4) ||
            (CPU_COMPAT_RETURN_PENDING == 0)) {
        upd9002_core_deinitialize();
        return FAILURE;
    }
    upd9002_core_step();
    if ((CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) ||
            (CPU_IP != compatible_offset + 18) || (CPU_SP != 0x00fa)) {
        upd9002_core_deinitialize();
        return FAILURE;
    }
    upd9002_core_step();
    if (CPU_BX != 0x1234 || CPU_IP != compatible_offset + 21) {
        upd9002_core_deinitialize();
        return FAILURE;
    }
    upd9002_core_step();
    int passed = (CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE) &&
        (CPU_CS == code_segment) && (CPU_IP == code_offset + 3) &&
        (CPU_SP == 0x0100) && (CPU_AL == 0x42) &&
        (CPU_BX == 0x1234) && (CPU_SI == 0x5678) &&
        (CPU_DI == 0x9abc);
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
        if (upd9002_core_compat_state_load(saved_compat,
                    sizeof(saved_compat)) != SUCCESS) {
            passed = 0;
        }
        else {
            CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
            upd9002_core_step();
            passed = (CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE) &&
                (CPU_IP == native_offset) && (CPU_SP == 0x00f4);
            upd9002_core_step();
            passed = passed &&
                (CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008) &&
                (CPU_IP == compatible_offset + 18);
        }
    }
    upd9002_core_deinitialize();
    return passed ? SUCCESS : FAILURE;
}
#endif
