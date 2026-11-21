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
#include "compiler.h"
#include "cpucore.h"
#include "machine/nevent.h"
#include "iocore.h"
#include "io/upd9002_regs.h"

#include <stdio.h>

int upd9002_interrupt_shadow_main(void);

enum {
	START = 0x100,
	IRQ_HANDLER = 0x300,
	TRAP_HANDLER = 0x400,
	IRQ_BIT = 2
};

static void setup(const UINT8 *code, size_t len, UINT16 flags) {
	upd9002_core_reset();
	upd9002_wcu_legacy();
	pic_reset();
	pic_select_8259_mode();
	pic.pi[0].pry = 0;
	pic.pi[0].imr = (UINT8)~IRQ_BIT;
	pic.pi[0].icw[1] = 0x20;
	pic.pi[0].icw[2] = 0;
	pic.pi[0].icw[3] = 0;
	ZeroMemory(mem, 0x10000);
	CopyMemory(mem + START, code, len);
	STOREINTELWORD(mem + 0x21 * 4, IRQ_HANDLER);
	STOREINTELWORD(mem + 1 * 4, TRAP_HANDLER);
	CPU_CS = 0;
	CS_BASE = 0;
	CPU_IP = START;
	CPU_SP = 0x900;
	CPU_AX = 0x100;
	CPU_FLAG = flags | 2;
	STOREINTELWORD(mem + 0x900, 0x100); /* POP SS source on the old stack. */
	STOREINTELWORD(mem + 0x200, 0x100); /* MOV SS memory operand. */
}

static void step(SINT32 clocks) {
	/* Force explicit scheduler boundaries even if the instruction overshoots. */
	CPU_BASECLOCK = CPU_REMCLOCK = clocks;
	upd9002_core_step();
}

static BOOL irq_not_acknowledged(void) {
	return (pic.pi[0].irr & IRQ_BIT) && !(pic.pi[0].isr & IRQ_BIT);
}

static int irq_stack_switch(const UINT8 *code, size_t len, UINT16 sp_instruction) {
	setup(code, len, I_FLAG);
	pic_setirq(1);
	/* Request already pending when the SS load executes. The first
	 * INTA opportunity in this fixture is the boundary after that load. */
	step(1);
	pic_irq();
	if (CPU_IP != sp_instruction || CPU_SS != 0x100 || !CPU_isEI || !irq_not_acknowledged() ||
	    LOADINTELWORD(mem + SS_BASE + CPU_SP - 6) != 0) {
		return FAILURE;
	}
	step(1);
	pic_irq();
	return (CPU_IP == IRQ_HANDLER && CPU_SP == 0x7fa && !(pic.pi[0].irr & IRQ_BIT) &&
	        (pic.pi[0].isr & IRQ_BIT) && LOADINTELWORD(mem + 0x17fa) == START + len - 1)
	           ? SUCCESS
	           : FAILURE;
}

static int trap_stack_switch(const UINT8 *code, size_t len, UINT16 sp_instruction) {
	setup(code, len, T_FLAG); /* Trap independent of IF. */
	step(1);
	if (CPU_IP != sp_instruction || !(CPU_FLAG & T_FLAG)) {
		return FAILURE;
	}
	step(1);
	return (CPU_IP == TRAP_HANDLER && CPU_SP == 0x7fa && !(CPU_FLAG & T_FLAG) &&
	        LOADINTELWORD(mem + 0x17fa) == START + len - 1 &&
	        (LOADINTELWORD(mem + 0x17fe) & T_FLAG))
	           ? SUCCESS
	           : FAILURE;
}

#define REQUIRE(condition)                                                                         \
	do {                                                                                           \
		if (!(condition)) {                                                                        \
			fprintf(stderr, "interrupt-shadow: %s (line %d)\n", #condition, __LINE__);             \
			return FAILURE;                                                                        \
		}                                                                                          \
	} while (0)

int upd9002_interrupt_shadow_main(void) {
	static const UINT8 mov_ss[] = {0x8e, 0xd0, 0xbc, 0, 8, 0x90};
	static const UINT8 pop_ss[] = {0x17, 0xbc, 0, 8, 0x90};
	static const UINT8 prefixed_mov_ss[] = {0x26, 0xf3, 0x8e, 0x16, 0, 2, 0xbc, 0, 8, 0x90};
	static const UINT8 prefixed_pop_ss[] = {0xf2, 0x3e, 0x17, 0xbc, 0, 8, 0x90};
	static const UINT8 other_segments[] = {0x8e, 0xc0, 0x1f, 0x07, 0x8e, 0xd8, 0x90};
	static const UINT8 nop[] = {0x90, 0x90};
	static const UINT8 sti[] = {0xfb, 0x90, 0x90};
	static const UINT8 stos[] = {0x8e, 0xd0, 0xf3, 0xaa, 0x90};
	static const UINT8 int3[] = {0x8e, 0xd0, 0xcc};
	static const UINT8 halt[] = {0x8e, 0xd0, 0xf4};

	upd9002_core_initialize();
	REQUIRE(irq_stack_switch(mov_ss, sizeof(mov_ss), START + 2) == SUCCESS);
	REQUIRE(irq_stack_switch(pop_ss, sizeof(pop_ss), START + 1) == SUCCESS);
	REQUIRE(irq_stack_switch(prefixed_mov_ss, sizeof(prefixed_mov_ss), START + 6) == SUCCESS);
	REQUIRE(irq_stack_switch(prefixed_pop_ss, sizeof(prefixed_pop_ss), START + 3) == SUCCESS);
	REQUIRE(trap_stack_switch(mov_ss, sizeof(mov_ss), START + 2) == SUCCESS);
	REQUIRE(trap_stack_switch(pop_ss, sizeof(pop_ss), START + 1) == SUCCESS);

	setup(other_segments, sizeof(other_segments), T_FLAG);
	for (UINT i = 0; i < 4; i++) {
		step(1);
		REQUIRE(CPU_INTERRUPT_SHADOW == UPD9002_SHADOW_SEGMENT && (CPU_FLAG & T_FLAG));
	}
	step(1);
	REQUIRE(CPU_IP == TRAP_HANDLER && !CPU_INTERRUPT_SHADOW);

	setup(nop, sizeof(nop), I_FLAG);
	pic_setirq(1);
	step(1);
	pic_irq();
	REQUIRE(CPU_IP == IRQ_HANDLER && LOADINTELWORD(mem + 0x8fa) == START + 1);

	setup(sti, sizeof(sti), 0);
	pic_setirq(1);
	step(1);
	pic_irq();
	REQUIRE(CPU_IP == START + 1 && CPU_isEI && irq_not_acknowledged());
	step(1);
	pic_irq();
	REQUIRE(CPU_IP == IRQ_HANDLER && LOADINTELWORD(mem + 0x8fa) == START + 2);

	setup(sti, sizeof(sti), T_FLAG);
	step(1);
	REQUIRE(CPU_IP == TRAP_HANDLER && LOADINTELWORD(mem + 0x8fa) == START + 1);

	setup(stos, sizeof(stos), T_FLAG);
	CPU_CX = 2;
	step(1);
	step(1);
	REQUIRE(CPU_IP == START + 2 && CPU_CX == 1 && CPU_INTERRUPT_SHADOW);
	step(1);
	REQUIRE(CPU_IP == TRAP_HANDLER && CPU_CX == 0 && LOADINTELWORD(mem + 0x18fa) == START + 4);

	setup(int3, sizeof(int3), T_FLAG);
	STOREINTELWORD(mem + 3 * 4, IRQ_HANDLER);
	step(1);
	step(1);
	REQUIRE(CPU_IP == IRQ_HANDLER && CPU_SP == 0x8fa && !CPU_INTERRUPT_SHADOW);

	setup(halt, sizeof(halt), T_FLAG);
	step(1);
	step(1);
	REQUIRE(CPU_IP == TRAP_HANDLER && LOADINTELWORD(mem + 0x18fa) == START + 3);

	setup(mov_ss, sizeof(mov_ss), 0);
	step(1);
	REQUIRE(CPU_INTERRUPT_SHADOW);
	upd9002_core_reset();
	REQUIRE(!CPU_INTERRUPT_SHADOW);
	upd9002_core_deinitialize();
	puts(
	    "interrupt-shadow: IRQ, trap, prefixes, other segments, STI, REP, INT3, HLT and reset passed");
	return SUCCESS;
}
