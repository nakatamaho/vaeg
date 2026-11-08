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
#include "machine/pccore.h"
#include "cpucore.h"
#include "cpu/upd9002_upd70008.h"
#include "memoryva/memoryva.h"
#include "io/sysportva.h"
#include "io/iocore.h"
#include "io/memctrlva.h"
#include "io/upd9002_regs.h"
#include "io/videova.h"
#include "io/n80_crtc.h"
#include "io/upd3301.h"

static UINT n80_pcg_port;
static REG8 n80_pcg_data;
static unsigned n80_pcg_writes;

static void IOOUTCALL n80_pcg_output(UINT port, REG8 data) {
	n80_pcg_port = port;
	n80_pcg_data = data;
	n80_pcg_writes++;
}

/* Real immediate, ED and block I/O with a deliberately overlapping VA trap.
 * Controls use the identical instruction/port, changing only model or mode. */
static int n80_guest_io_selftest(void) {
	/* Bit 8 is a fixture option (PCG enabled), not a port-address bit.
	 * 04h and 0Bh are neighbouring non-PCG controls. */
	static const UINT16 ports[] = {0x30,  0x31,  0x33,  0x34,  0x35,  0x5c,  0x5d,  0x5e,  0x5f,
	                               0x71,  0x50,  0x52,  0x53,  0x54,  0x55,  0x56,  0x57,  0x58,
	                               0x59,  0x5a,  0x5b,  0x00,  0x01,  0x02,  0x03,  0x0c,  0x0d,
	                               0x0e,  0x0f,  0x04,  0x0b,  0x100, 0x101, 0x102, 0x103, 0x10c,
	                               0x10d, 0x10e, 0x10f, 0x104, 0x10b, 0x64,  0x68};
	const UINT8 saved_pcg = np2cfg.pcg_enable;
	const BYTE saved_boot_switch = backupmem[0x1fc2];
	const BOOL saved_sr_present = memoryn80_sr_present;
	const _MEMORYN80 saved = memoryn80;
	const UINT8 saved_video[2] = {memoryn80_video[0], memoryn80_video[1]};
	WORD saved_palette[8];
	const UINT8 saved_plane = memoryva_88_plane, saved31 = memoryva_88_port31;
	const UINT8 saved71 = memoryva_88_xerom;
	const UINT8 saved34 = memoryva_88_alu.port034, saved35 = memoryva_88_alu.port035;
	const WORD saved_grmode = videova.grmode, saved_grres = videova.grres;
	unsigned role, index, form, input, cases = 0;
	int result = SUCCESS;
	CopyMemory(saved_palette, videova.palette, sizeof(saved_palette));
	iocore_create();
	if (iocore_build() != SUCCESS) {
		iocore_destroy();
		return FAILURE;
	}
	memctrlva_bind();
	n80_crtc_bind();
	for (index = 0; index <= 0x0f; index++)
		if (index <= 3 || index >= 0x0c)
			iocore_attachout(index, n80_pcg_output);
	for (role = 0; role < 6 && result == SUCCESS; role++) {
		for (index = 0; index < NELEMENTS(ports) && result == SUCCESS; index++) {
			for (form = 0; form < 3 && result == SUCCESS; form++) {
				for (input = 0; input < 2 && result == SUCCESS; input++) {
					const UINT8 port = (UINT8)ports[index];
					const BOOL pcg = port <= 3 || (port >= 0x0c && port <= 0x0f);
					const BOOL enabled = (ports[index] & 0x100) != 0;
					const BOOL n80 = role == 3 || role == 4;
					/* role 1 is the PC-8801 selection: the independent uPD3301 is
					 * family-wide, so the CRTC/DMA ports are direct there too. */
					const BOOL family = role == 1 || n80;
					const BOOL direct =
					    (input ? ((port == 0x50 || port == 0x51) && family) ||
					                 (n80 && (port == 0x30 || port == 0x5c || port == 0x71 ||
					                          (role == 4 && port == 0x33)))
					           : ((port == 0x50 || port == 0x51 || port == 0x64 || port == 0x68) &&
					              family) ||
					                 (n80 && (port == 0x31 || port == 0x71 ||
					                          (port >= 0x52 && port <= 0x5f) ||
					                          (role == 4 && port >= 0x33 && port <= 0x35) ||
					                          (enabled && pcg))));
					np2cfg.pcg_enable = enabled;
					n80_pcg_writes = 0;
					memoryva_pc8801 = role == 1;
					memoryva_pc8001 = role >= 2;
					memoryva_88_mode = 0;
					upd9002_core_initialize();
					upd9002_upd70008_register();
					upd9002_core_reset();
					ZeroMemory(mem, 0x40000);
					STOREINTELWORD(mem + 0xe1 * 4, 0x1000);
					STOREINTELWORD(mem + 0xe1 * 4 + 2, 0x2000);
					STOREINTELWORD(mem + (input ? 0x7c : 0x7d) * 4, 0x3000);
					STOREINTELWORD(mem + (input ? 0x7c : 0x7d) * 4 + 2, 0x2000);
					mem[0x20100] = 0x0f;
					mem[0x20101] = 0xff;
					mem[0x20102] = 0xe1;
					mem[0x21000] = form == 0 ? (input ? 0xdb : 0xd3) : 0xed;
					mem[0x21001] = form == 0   ? port
					               : form == 1 ? (input ? 0x78 : 0x79)
					                           : (input ? 0xa2 : 0xa3);
					mem[0x21100] = 0xc1;
					CPU_CS = CPU_DS = 0x2000;
					CS_BASE = DS_BASE = 0x20000;
					CPU_SS = 0x3000;
					SS_BASE = 0x30000;
					CPU_SP = 0x100;
					CPU_BP = 0x200;
					CPU_IP = 0x100;
					CPU_AL = 0xc1;
					CPU_CX = (UINT16)(0xa500 | port);
					CPU_BX = 0x1100;
					CPU_FLAG = 0xf000;
					CPU_REMCLOCK = CPU_BASECLOCK = 100000;
					upd9002_core_step();
					memoryva_88_mode = role != 5;
					memoryn80.model = role == 3 ? 1 : role >= 4 ? 2 : 0;
					memoryn80.port31 = 0;
					memoryn80.port33 = 0x94;
					backupmem[0x1fc2] = 0xa4;
					memoryn80_sr_present = role == 4;
					memoryn80.port71 = 0;
					memoryva_88_plane = 3;
					memoryn80_video[0] = memoryn80_video[1] = 0;
					memoryva_88_alu.port034 = memoryva_88_alu.port035 = 0;
					n80_crtc_reset();
					if (!input) {
						/* Arm observable device state for the direct-route check. */
						if (port == 0x50) {
							n80_crtc.command = UPD3301_RESET;
							n80_crtc.parameter = 2;
						} else if (port == 0x51) {
							n80_crtc.frames = 0x1234;
							n80_crtc.interrupt_request = 1;
						}
					}
					ZeroMemory(&upd9002_iotrap, sizeof(upd9002_iotrap));
					upd9002_iotrap.control = 3;
					upd9002_iotrap.ranges[0] = upd9002_iotrap.ranges[2] = port;
					upd9002_core_step();
					if (direct) {
						UINT8 expected = port == 0x30   ? (role == 4 ? 0xa5 : 0xa7)
						                 : port == 0x33 ? 0x94
						                 : port == 0x71 ? 0xfe
						                 : (port == 0x50 || port == 0x51) ? 0x00
						                                                  : 0xf8;
						if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008 || CPU_IP != 0x1002 ||
						    CPU_SP != 0xfa ||
						    (input && (form == 2 ? mem[0x21100] : CPU_AL) != expected) ||
						    (!input &&
						     ((port == 0x31 && memoryn80.port31 != 0xc1) ||
						      (port == 0x33 && memoryn80.port33 != 0xc1) ||
						      (port == 0x34 && memoryva_88_alu.port034 != 0xc1) ||
						      (port == 0x35 && memoryva_88_alu.port035 != 0xc1) ||
						      (port == 0x71 && memoryn80.port71 != 1) ||
						      (port == 0x52 && memoryn80_video[0] != 0xc1) ||
						      (port == 0x53 && memoryn80_video[1] != 0xc1) ||
						      (port >= 0x54 && port <= 0x5b &&
						       videova.palette[port - 0x54] != 0x001f) ||
						      (port >= 0x5c && port <= 0x5f && memoryva_88_plane != port - 0x5c) ||
						      (port == 0x50 &&
						       (n80_crtc.command != UPD3301_RESET || n80_crtc.parameter != 3 ||
						        n80_crtc.reset[2] != 0xc1)) ||
						      (port == 0x51 && (n80_crtc.frames || n80_crtc.interrupt_request)) ||
						      (port == 0x64 &&
						       (n80_crtc_dma.address != 0xf3c1 || n80_crtc_dma.high_next != 1)) ||
						      (port == 0x68 && (n80_crtc_dma.mode != 0xc1 ||
						                        n80_crtc_dma.high_next || n80_crtc.dma_started)))))
							result = FAILURE;
					} else if (CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE || CPU_IP != 0x3000 ||
					           CPU_SP != 0xf4 || memoryn80.port31 != 0 ||
					           memoryn80.port33 != 0x94 || memoryn80.port71 != 0 ||
					           memoryva_88_plane != 3 || memoryn80_video[0] || memoryn80_video[1] ||
					           memoryva_88_alu.port034 || memoryva_88_alu.port035) {
						result = FAILURE;
					}
					if (direct && !input && pcg) {
						if (n80_pcg_writes != 1 || n80_pcg_port != port || n80_pcg_data != 0xc1)
							result = FAILURE;
					} else if (n80_pcg_writes) {
						result = FAILURE;
					}
					cases++;
					if (result != SUCCESS)
						fprintf(stderr,
						        "%s role=%u port=%02x form=%u input=%u enabled=%u ip=%04x\n",
						        port <= 0x0f ? "M144A_N80_PCG_GUEST_ROUTE" : "M144A_N80_GUEST_IO",
						        role, port, form, input, enabled, CPU_IP);
				}
			}
		}
	}
	ZeroMemory(&upd9002_iotrap, sizeof(upd9002_iotrap));
	np2cfg.pcg_enable = saved_pcg;
	backupmem[0x1fc2] = saved_boot_switch;
	memoryn80_sr_present = saved_sr_present;
	memoryn80 = saved;
	CopyMemory(memoryn80_video, saved_video, sizeof(saved_video));
	CopyMemory(videova.palette, saved_palette, sizeof(saved_palette));
	memoryva_88_plane = saved_plane;
	memoryva_88_port31 = saved31;
	memoryva_88_xerom = saved71;
	memoryva_88_alu.port034 = saved34;
	memoryva_88_alu.port035 = saved35;
	videova.grmode = saved_grmode;
	videova.grres = saved_grres;
	iocore_destroy();
	if (result == SUCCESS)
		fprintf(stderr, "N80 guest I/O: %u instruction/trap/model cases PASS\n", cases);
	return result;
}

static UINT crtc_output_count;
static UINT crtc_output_ports[256];
static BYTE crtc_output_data[256];

static void IOOUTCALL crtc_output_probe(UINT port, REG8 data) {
	if (crtc_output_count < NELEMENTS(crtc_output_data)) {
		crtc_output_ports[crtc_output_count] = port;
		crtc_output_data[crtc_output_count] = (BYTE)data;
	}
	crtc_output_count++;
}

/* Probe the existing dispatch boundary, not the CRTC geometry implementation.
 * Trapped instructions must still reach the firmware before a device write;
 * untrapped instructions must deliver each byte exactly once, including wraps. */
static int n80_crtc_dispatch_selftest(void) {
	static const BYTE ops[] = {0xd3, 0x41, 0x49, 0x51, 0x59, 0x61, 0x69,
	                           0x71, 0x79, 0xa3, 0xab, 0xb3, 0xbb};
	static const BYTE counts[] = {1, 3, 0};
	const _MEMORYN80 saved_bank = memoryn80;
	const BOOL saved_8001 = memoryva_pc8001, saved_8801 = memoryva_pc8801;
	const BOOL saved_mode = memoryva_88_mode;
	const UPD9002_IOTRAP saved_trap = upd9002_iotrap;
	UINT model, port, trap, form, count, cases = 0;
	const char *problem = NULL;
	iocore_create();
	if (iocore_build() != SUCCESS) {
		iocore_destroy();
		return FAILURE;
	}
	iocore_attachout(0x50, crtc_output_probe);
	iocore_attachout(0x51, crtc_output_probe);
	for (model = 1; model <= 2 && !problem; model++)
		for (port = 0x50; port <= 0x51 && !problem; port++)
			for (trap = 0; trap < 2 && !problem; trap++)
				for (form = 0; form < NELEMENTS(ops) && !problem; form++)
					for (count = 0; count < (form >= 9 ? NELEMENTS(counts) : 1) && !problem;
					     count++) {
						const BOOL block = form >= 9, repeat = form >= 11;
						const BOOL decrement = form == 10 || form == 12;
						const UINT8 b = block ? counts[count] : 3;
						const UINT length = repeat ? (b ? b : 256) : 1;
						const UINT16 hl = block ? (decrement ? 1 : 0xfffe) : 0x4d7a;
						const BYTE register_data[] = {0xc1, b,    (BYTE)port, 0x64, 0x29,
						                              0x4d, 0x7a, 0,          0xc1};
						UINT i, steps;
						memoryva_pc8801 = FALSE;
						memoryva_pc8001 = TRUE;
						memoryva_88_mode = FALSE;
						upd9002_core_initialize();
						upd9002_upd70008_register();
						upd9002_core_reset();
						ZeroMemory(mem, 0x40000);
						STOREINTELWORD(mem + 0xe1 * 4, 0x1000);
						STOREINTELWORD(mem + 0xe1 * 4 + 2, 0x2000);
						STOREINTELWORD(mem + 0x7d * 4, 0x3000);
						STOREINTELWORD(mem + 0x7d * 4 + 2, 0x2000);
						mem[0x20100] = 0x0f;
						mem[0x20101] = 0xff;
						mem[0x20102] = 0xe1;
						mem[0x21000] = form ? 0xed : 0xd3;
						mem[0x21001] = form ? ops[form] : (BYTE)port;
						if (block)
							for (i = 0; i < length; i++)
								mem[0x20000 + (UINT16)(decrement ? hl - i : hl + i)] =
								    (BYTE)(0x93 + 37 * i);
						CPU_CS = CPU_DS = 0x2000;
						CS_BASE = DS_BASE = 0x20000;
						CPU_SS = 0x3000;
						SS_BASE = 0x30000;
						CPU_SP = 0x100;
						CPU_BP = 0x200;
						CPU_IP = 0x100;
						CPU_AL = 0xc1;
						CPU_CX = (UINT16)((b << 8) | port);
						CPU_DX = 0x6429;
						CPU_BX = hl;
						CPU_FLAG = 0xf000;
						CPU_REMCLOCK = CPU_BASECLOCK = 100000;
						upd9002_core_step();
						memoryva_88_mode = TRUE;
						memoryn80.model = (UINT8)model;
						ZeroMemory(&upd9002_iotrap, sizeof(upd9002_iotrap));
						upd9002_iotrap.control = trap ? 2 : 0;
						upd9002_iotrap.ranges[0] = upd9002_iotrap.ranges[2] = (UINT8)port;
						crtc_output_count = 0;
						upd9002_core_step();
						/* The independent models bypass the armed VA firmware trap:
						 * actual byte delivery per instruction, including block I/O. */
						{
							for (steps = 1; steps < 256 && CPU_IP == 0x1000; steps++)
								upd9002_core_step();
							if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008 || CPU_IP != 0x1002 ||
							    CPU_SP != 0xfa || crtc_output_count != length ||
							    CPU_BX != (block ? (UINT16)(decrement ? hl - length : hl + length)
							                     : hl) ||
							    CPU_CX !=
							        (UINT16)((((UINT8)(b - (block ? length : 0))) << 8) | port))
								problem = "M144A_N80_CRTC_OUTPUT_SEQUENCE";
							for (i = 0; i < length && !problem; i++)
								if (crtc_output_ports[i] != port ||
								    crtc_output_data[i] !=
								        (block ? (BYTE)(0x93 + 37 * i) : register_data[form]))
									problem = "M144A_N80_CRTC_OUTPUT_DATA";
						}
						cases++;
						if (problem)
							fprintf(stderr,
							        "%s model=%u port=%02x trap=%u op=%02x b=%u writes=%u\n",
							        problem, model, port, trap, ops[form], b, crtc_output_count);
					}
	memoryn80 = saved_bank;
	memoryva_pc8001 = saved_8001;
	memoryva_pc8801 = saved_8801;
	memoryva_88_mode = saved_mode;
	upd9002_iotrap = saved_trap;
	iocore_destroy();
	if (!problem)
		fprintf(stderr, "N80 CRTC dispatch: %u instruction/trap/wrap cases PASS\n", cases);
	return problem ? FAILURE : SUCCESS;
}

/* Independent instruction fixture, never copied from private guest software. */
int upd780_main_role_selftest(void) {
	static const UINT8 code[] = {
	    0xcb, 0x47, /* BIT 0,A: H distinguishes the main CPUs */
	    0x00,       /* NOP: four uPD780 T states, not VA measured cost */
	    0x00, 0x00, /* NOP at S, then S with CPU speed 50% */
	    0xed, 0x20, /* undefined uPD780 instruction, not VA native-vector read */
	    0xed, 0xed, /* not CALLN on the uPD780 */
	    0xed, 0xfd  /* not RETEM on the uPD780 */
	};
	static const UINT8 roles[][2] = {{0, 0}, {1, 0}, {0, 1}, {0, 0}, {1, 0}};
	const BOOL saved_pc8801 = memoryva_pc8801;
	const BOOL saved_pc8001 = memoryva_pc8001;
	const UINT8 saved_mode = memoryva_88_mode;
	const UINT8 saved_port_a = sysportva.a;
	const UINT saved_multiple = np2cfg.multiple;
	const UINT8 saved_speed = np2cfg.cpu_speed;
	UINT i;
	int result = SUCCESS;

	np2cfg.multiple = PCCORE_STANDARD_MULTIPLE;
	np2cfg.cpu_speed = 100;
	pccore_clockrestore();
	memoryva_88_mode = 0;
	for (i = 0; i < NELEMENTS(roles); i++) {
		SINT32 before;
		const BOOL standalone = roles[i][0] || roles[i][1];
		memoryva_pc8801 = roles[i][0];
		memoryva_pc8001 = roles[i][1];
		upd9002_core_initialize();
		upd9002_upd70008_register();
		upd9002_core_reset();
		ZeroMemory(mem, 0x40000);
		STOREINTELWORD(mem + 0xe1 * 4, 0x1000);
		STOREINTELWORD(mem + 0xe1 * 4 + 2, 0x2000);
		mem[0x20100] = 0x0f;
		mem[0x20101] = 0xff;
		mem[0x20102] = 0xe1;
		CopyMemory(mem + 0x21000, code, sizeof(code));
		CPU_CS = CPU_DS = 0x2000;
		CS_BASE = DS_BASE = 0x20000;
		CPU_SS = 0x3000;
		SS_BASE = 0x30000;
		CPU_SP = 0x100;
		CPU_BP = 0x200;
		CPU_IP = 0x100;
		CPU_AL = 1;
		CPU_BX = 0xbeef;
		CPU_FLAG = 0xf000;
		CPU_REMCLOCK = CPU_BASECLOCK = 100000;
		upd9002_core_step(); /* board bootstrap hands off to selected CPU */
		upd9002_core_step(); /* BIT */
		if ((pccore_main_cpu_is_upd780() != standalone) ||
		    ((CPU_FLAG & 0x10) != (standalone ? 0x10 : 0))) {
			fprintf(stderr, "M139_MAIN_CPU_BIT_PROFILE role=%u flags=%04x\n", i, CPU_FLAG);
			result = FAILURE;
			break;
		}
		before = CPU_REMCLOCK;
		upd9002_core_step(); /* NOP */
		if (standalone && before - CPU_REMCLOCK != 4) {
			fprintf(stderr, "M139_MAIN_CPU_TSTATES role=%u elapsed=%d\n", i,
			        (int)(before - CPU_REMCLOCK));
			result = FAILURE;
			break;
		}
		if (standalone) {
			UINT op;
			memoryva_88_mode = 1;
			sysportva.a &= (UINT8)~0x20;
			for (op = 0; op < 2; op++) {
				pccore_set_cpu_speed(op ? 50 : 100);
				before = CPU_REMCLOCK;
				upd9002_core_step();
				if (before - CPU_REMCLOCK != (op ? 16 : 8)) {
					fprintf(stderr, "M139_MAIN_CPU_CLOCK_SCALE role=%u case=%u\n", i, op);
					result = FAILURE;
					break;
				}
			}
			memoryva_88_mode = 0;
			sysportva.a = saved_port_a;
			pccore_set_cpu_speed(100);
			if (result != SUCCESS) {
				break;
			}
			for (op = 0; op < 3; op++) {
				upd9002_core_step();
				if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008 || CPU_COMPAT_RETURN_PENDING ||
				    CPU_IP != 0x1007 + op * 2 || CPU_BX != 0xbeef) {
					fprintf(stderr, "M139_MAIN_CPU_NATIVE_OPCODE role=%u op=%u\n", i, op);
					result = FAILURE;
					break;
				}
			}
		}
		if (result != SUCCESS) {
			break;
		}
	}
	if (result == SUCCESS)
		result = n80_guest_io_selftest();
	if (result == SUCCESS)
		result = n80_crtc_dispatch_selftest();
	memoryva_pc8801 = saved_pc8801;
	memoryva_pc8001 = saved_pc8001;
	memoryva_88_mode = saved_mode;
	sysportva.a = saved_port_a;
	np2cfg.multiple = saved_multiple;
	np2cfg.cpu_speed = saved_speed;
	pccore_clockrestore();
	upd9002_core_reset();
	if (result == SUCCESS) {
		fprintf(
		    stderr,
		    "main CPU roles: VA/PC-8801/PC-8001 reset selection, BIT, T states, native-opcode isolation PASS\n");
	}
	return result;
}
