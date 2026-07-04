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
 *
 * Ported from cpuxva/memoryva.x86 for the portable i286c core.
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"memoryva.h"
#include	"gvramva.h"
#include	"va91.h"

#if defined(SUPPORT_PC88VA)

void MEMCALL gvram_wt(UINT32 address, REG8 value);
void MEMCALL gvramw_wt(UINT32 address, REG16 value);
REG8 MEMCALL gvram_rd(UINT32 address);
REG16 MEMCALL gvramw_rd(UINT32 address);

enum {
	CPUADDR_SYSM		= 0x0a0000,
	CPUADDR_ROM0		= 0x0e0000,
	CPUADDR_ROM1		= 0x0f0000,
	CPUADDR_BACKUP		= 0x0b0000,
	BACKUPMEMORY_SIZE	= 0x04000
};

	BYTE		textmem[0x40000];
	BYTE		fontmem[0x50000];
	BYTE		backupmem[0x04000];
	BYTE		dicmem[0x80000];
	BYTE		rom0mem[0xa0000];
	BYTE		rom1mem[0x20000];

	BYTE		va91rom0mem[0xa0000];
	BYTE		va91rom1mem[0x20000];
	BYTE		va91dicmem[0x80000];

	_MEMORYVA	memoryva;
	BOOL		textmem_dirty;

	_VA91		va91;
	_VA91CFG	va91cfg;

typedef void (MEMCALL * MEM8WRITE)(UINT32 address, REG8 value);
typedef REG8 (MEMCALL * MEM8READ)(UINT32 address);
typedef void (MEMCALL * MEM16WRITE)(UINT32 address, REG16 value);
typedef REG16 (MEMCALL * MEM16READ)(UINT32 address);

static void MEMCALL i286_wt_va(UINT32 address, REG8 value);
static void MEMCALL i286_wn_va(UINT32 address, REG8 value);
static void MEMCALL bms_wt_va(UINT32 address, REG8 value);
static void MEMCALL sysm_wt(UINT32 address, REG8 value);
static void MEMCALL tvram_wt(UINT32 address, REG8 value);
static void MEMCALL gvram_wt_va(UINT32 address, REG8 value);
static void MEMCALL knj2_wt(UINT32 address, REG8 value);
static void MEMCALL va91sysm_wt(UINT32 address, REG8 value);
static void MEMCALL va91knj2_wt(UINT32 address, REG8 value);

static void MEMCALL i286w_wt_va(UINT32 address, REG16 value);
static void MEMCALL i286w_wn_va(UINT32 address, REG16 value);
static void MEMCALL bmsw_wt_va(UINT32 address, REG16 value);
static void MEMCALL sysmw_wt(UINT32 address, REG16 value);
static void MEMCALL tvramw_wt(UINT32 address, REG16 value);
static void MEMCALL gvramw_wt_va(UINT32 address, REG16 value);
static void MEMCALL knj2w_wt(UINT32 address, REG16 value);
static void MEMCALL va91sysmw_wt(UINT32 address, REG16 value);
static void MEMCALL va91knj2w_wt(UINT32 address, REG16 value);

static REG8 MEMCALL i286_rd_va(UINT32 address);
static REG8 MEMCALL i286_rn_va(UINT32 address);
static REG8 MEMCALL bms_rd_va(UINT32 address);
static REG8 MEMCALL sysm_rd(UINT32 address);
static REG8 MEMCALL tvram_rd(UINT32 address);
static REG8 MEMCALL gvram_rd_va(UINT32 address);
static REG8 MEMCALL knj1_rd(UINT32 address);
static REG8 MEMCALL knj2_rd(UINT32 address);
static REG8 MEMCALL dic1_rd(UINT32 address);
static REG8 MEMCALL dic2_rd(UINT32 address);
static REG8 MEMCALL rom0_rd(UINT32 address);
static REG8 MEMCALL stdrom0_rd(UINT32 address);
static REG8 MEMCALL rom1_rd(UINT32 address);
static REG8 MEMCALL stdrom1_rd(UINT32 address);
static REG8 MEMCALL va91sysm_rd(UINT32 address);
static REG8 MEMCALL va91knj2_rd(UINT32 address);
static REG8 MEMCALL va91dic1_rd(UINT32 address);
static REG8 MEMCALL va91dic2_rd(UINT32 address);
static REG8 MEMCALL va91rom0_rd(UINT32 address);
static REG8 MEMCALL va91rom1_rd(UINT32 address);

static REG16 MEMCALL i286w_rd_va(UINT32 address);
static REG16 MEMCALL i286w_rn_va(UINT32 address);
static REG16 MEMCALL bmsw_rd_va(UINT32 address);
static REG16 MEMCALL sysmw_rd(UINT32 address);
static REG16 MEMCALL tvramw_rd(UINT32 address);
static REG16 MEMCALL gvramw_rd_va(UINT32 address);
static REG16 MEMCALL knj1w_rd(UINT32 address);
static REG16 MEMCALL knj2w_rd(UINT32 address);
static REG16 MEMCALL dic1w_rd(UINT32 address);
static REG16 MEMCALL dic2w_rd(UINT32 address);
static REG16 MEMCALL rom0w_rd(UINT32 address);
static REG16 MEMCALL stdrom0w_rd(UINT32 address);
static REG16 MEMCALL rom1w_rd(UINT32 address);
static REG16 MEMCALL stdrom1w_rd(UINT32 address);
static REG16 MEMCALL va91sysmw_rd(UINT32 address);
static REG16 MEMCALL va91knj2w_rd(UINT32 address);
static REG16 MEMCALL va91dic1w_rd(UINT32 address);
static REG16 MEMCALL va91dic2w_rd(UINT32 address);
static REG16 MEMCALL va91rom0w_rd(UINT32 address);
static REG16 MEMCALL va91rom1w_rd(UINT32 address);

static MEM8WRITE membyte_write[16] = {
	i286_wt_va, i286_wt_va, i286_wt_va, i286_wt_va,
	i286_wt_va, i286_wt_va, i286_wt_va, i286_wt_va,
	bms_wt_va, bms_wt_va, sysm_wt, sysm_wt,
	sysm_wt, sysm_wt, i286_wn_va, i286_wn_va
};

static MEM8WRITE sysmbyte_write[16] = {
	i286_wn_va, tvram_wt, i286_wn_va, i286_wn_va,
	gvram_wt_va, i286_wn_va, i286_wn_va, i286_wn_va,
	i286_wn_va, knj2_wt, i286_wn_va, i286_wn_va,
	i286_wn_va, i286_wn_va, i286_wn_va, i286_wn_va
};

static MEM8WRITE va91sysmbyte_write[16] = {
	i286_wn_va, i286_wn_va, i286_wn_va, i286_wn_va,
	i286_wn_va, i286_wn_va, i286_wn_va, i286_wn_va,
	i286_wn_va, va91knj2_wt, i286_wn_va, i286_wn_va,
	i286_wn_va, i286_wn_va, i286_wn_va, i286_wn_va
};

static MEM16WRITE memword_write[16] = {
	i286w_wt_va, i286w_wt_va, i286w_wt_va, i286w_wt_va,
	i286w_wt_va, i286w_wt_va, i286w_wt_va, i286w_wt_va,
	bmsw_wt_va, bmsw_wt_va, sysmw_wt, sysmw_wt,
	sysmw_wt, sysmw_wt, i286w_wn_va, i286w_wn_va
};

static MEM16WRITE sysmword_write[16] = {
	i286w_wn_va, tvramw_wt, i286w_wn_va, i286w_wn_va,
	gvramw_wt_va, i286w_wn_va, i286w_wn_va, i286w_wn_va,
	i286w_wn_va, knj2w_wt, i286w_wn_va, i286w_wn_va,
	i286w_wn_va, i286w_wn_va, i286w_wn_va, i286w_wn_va
};

static MEM16WRITE va91sysmword_write[16] = {
	i286w_wn_va, i286w_wn_va, i286w_wn_va, i286w_wn_va,
	i286w_wn_va, i286w_wn_va, i286w_wn_va, i286w_wn_va,
	i286w_wn_va, va91knj2w_wt, i286w_wn_va, i286w_wn_va,
	i286w_wn_va, i286w_wn_va, i286w_wn_va, i286w_wn_va
};

static MEM8READ membyte_read[16] = {
	i286_rd_va, i286_rd_va, i286_rd_va, i286_rd_va,
	i286_rd_va, i286_rd_va, i286_rd_va, i286_rd_va,
	bms_rd_va, bms_rd_va, sysm_rd, sysm_rd,
	sysm_rd, sysm_rd, rom0_rd, rom1_rd
};

static MEM8READ sysmbyte_read[16] = {
	i286_rn_va, tvram_rd, i286_rn_va, i286_rn_va,
	gvram_rd_va, i286_rn_va, i286_rn_va, i286_rn_va,
	knj1_rd, knj2_rd, i286_rn_va, i286_rn_va,
	dic1_rd, dic2_rd, i286_rn_va, i286_rn_va
};

static MEM8READ rom0byte_read[32] = {
	stdrom0_rd, stdrom0_rd, stdrom0_rd, stdrom0_rd,
	stdrom0_rd, stdrom0_rd, stdrom0_rd, stdrom0_rd,
	stdrom0_rd, stdrom0_rd, stdrom0_rd, stdrom0_rd,
	stdrom0_rd, stdrom0_rd, stdrom0_rd, stdrom0_rd,
	i286_rn_va, i286_rn_va, i286_rn_va, i286_rn_va,
	i286_rn_va, i286_rn_va, i286_rn_va, i286_rn_va,
	i286_rn_va, i286_rn_va, i286_rn_va, i286_rn_va,
	i286_rn_va, i286_rn_va, i286_rn_va, i286_rn_va
};

static MEM8READ rom1byte_read[16] = {
	stdrom1_rd, stdrom1_rd, stdrom1_rd, stdrom1_rd,
	stdrom1_rd, stdrom1_rd, stdrom1_rd, stdrom1_rd,
	i286_rn_va, i286_rn_va, i286_rn_va, i286_rn_va,
	i286_rn_va, i286_rn_va, i286_rn_va, i286_rn_va
};

static MEM8READ va91sysmbyte_read[16] = {
	i286_rn_va, i286_rn_va, i286_rn_va, i286_rn_va,
	i286_rn_va, i286_rn_va, i286_rn_va, i286_rn_va,
	i286_rn_va, va91knj2_rd, i286_rn_va, i286_rn_va,
	va91dic1_rd, va91dic2_rd, i286_rn_va, i286_rn_va
};

static MEM16READ memword_read[16] = {
	i286w_rd_va, i286w_rd_va, i286w_rd_va, i286w_rd_va,
	i286w_rd_va, i286w_rd_va, i286w_rd_va, i286w_rd_va,
	bmsw_rd_va, bmsw_rd_va, sysmw_rd, sysmw_rd,
	sysmw_rd, sysmw_rd, rom0w_rd, rom1w_rd
};

static MEM16READ sysmword_read[16] = {
	i286w_rn_va, tvramw_rd, i286w_rn_va, i286w_rn_va,
	gvramw_rd_va, i286w_rn_va, i286w_rn_va, i286w_rn_va,
	knj1w_rd, knj2w_rd, i286w_rn_va, i286w_rn_va,
	dic1w_rd, dic2w_rd, i286w_rn_va, i286w_rn_va
};

static MEM16READ rom0word_read[32] = {
	stdrom0w_rd, stdrom0w_rd, stdrom0w_rd, stdrom0w_rd,
	stdrom0w_rd, stdrom0w_rd, stdrom0w_rd, stdrom0w_rd,
	stdrom0w_rd, stdrom0w_rd, stdrom0w_rd, stdrom0w_rd,
	stdrom0w_rd, stdrom0w_rd, stdrom0w_rd, stdrom0w_rd,
	i286w_rn_va, i286w_rn_va, i286w_rn_va, i286w_rn_va,
	i286w_rn_va, i286w_rn_va, i286w_rn_va, i286w_rn_va,
	i286w_rn_va, i286w_rn_va, i286w_rn_va, i286w_rn_va,
	i286w_rn_va, i286w_rn_va, i286w_rn_va, i286w_rn_va
};

static MEM16READ rom1word_read[16] = {
	stdrom1w_rd, stdrom1w_rd, stdrom1w_rd, stdrom1w_rd,
	stdrom1w_rd, stdrom1w_rd, stdrom1w_rd, stdrom1w_rd,
	i286w_rn_va, i286w_rn_va, i286w_rn_va, i286w_rn_va,
	i286w_rn_va, i286w_rn_va, i286w_rn_va, i286w_rn_va
};

static MEM16READ va91sysmword_read[16] = {
	i286w_rn_va, i286w_rn_va, i286w_rn_va, i286w_rn_va,
	i286w_rn_va, i286w_rn_va, i286w_rn_va, i286w_rn_va,
	i286w_rn_va, va91knj2w_rd, i286w_rn_va, i286w_rn_va,
	va91dic1w_rd, va91dic2w_rd, i286w_rn_va, i286w_rn_va
};

static UINT32 top_index(UINT32 address) {

	return((address >> 16) & 0x0f);
}

static UINT32 inc_cx(UINT32 address) {

	return((address & 0xffff0000) | LOW16(address + 1));
}

static UINT sysm_bank(void) {

	UINT	bank;

	bank = memoryva.dma_sysm_bank;
	if (!(bank & memoryva.dma_access)) {
		bank = memoryva.sysm_bank;
	}
	return(bank & 0x0f);
}

static UINT va91sysm_bank(void) {

	return(va91.sysm_bank & 0x0f);
}

static REG16 duplicate_word(REG8 value) {

	return((REG16)value | ((REG16)value << 8));
}

static REG16 read_pair16(MEM8READ rd, UINT32 address) {

	REG8	lo;
	REG8	hi;

	if (!(address & 1)) {
		return(duplicate_word(rd(address)));
	}
	hi = rd(inc_cx(address));
	lo = rd(address);
	return((REG16)lo | ((REG16)hi << 8));
}

static void write_pair16(MEM8WRITE wt, UINT32 address, REG16 value) {

	if (!(address & 1)) {
		wt(address, (REG8)value);
	}
	else {
		wt(inc_cx(address), (REG8)(value >> 8));
		wt(address, (REG8)value);
	}
}

static REG8 rom1invalid_rd(UINT32 address) {

	REG16	ret;

	ret = (REG16)address & 0xfffe;
	if (!(address & 1)) {
		return((REG8)(ret >> 8));
	}
	return((REG8)ret);
}

static REG16 rom1invalidw_rd(UINT32 address) {

	REG16	ret;

	ret = (REG16)address & 0xfffe;
	if (!(address & 1)) {
		ret = (REG16)(((ret & 0x00ff) << 8) |
						(((ret >> 8) + 2) & 0x00ff));
	}
	return(ret);
}

static void MEMCALL i286_wt_va(UINT32 address, REG8 value) {

	UINT8	save;

	save = memmode_va;
	memmode_va = 0;
	i286_memorywrite(address, value);
	memmode_va = save;
}

static void MEMCALL i286_wn_va(UINT32 address, REG8 value) {

	(void)address;
	(void)value;
}

static void MEMCALL bms_wt_va(UINT32 address, REG8 value) {

	i286_wt_va(address, value);
}

static void MEMCALL sysm_wt(UINT32 address, REG8 value) {

	sysmbyte_write[sysm_bank()](address, value);
}

static void MEMCALL tvram_wt(UINT32 address, REG8 value) {

	textmem[address - CPUADDR_SYSM] = (BYTE)value;
	textmem_dirty = TRUE;
}

static void MEMCALL gvram_wt_va(UINT32 address, REG8 value) {

	gvram_wt(address - CPUADDR_SYSM, value);
}

static void MEMCALL knj2_wt(UINT32 address, REG8 value) {

	UINT32	offset;

	if ((address >= 0x0b1fc0) && (address < 0x0b2000) &&
		memoryva.backupmem_wp) {
		return;
	}
	if (address < CPUADDR_BACKUP) {
		return;
	}
	offset = address - CPUADDR_BACKUP;
	if (offset >= BACKUPMEMORY_SIZE) {
		return;
	}
	backupmem[offset] = (BYTE)value;
}

static void MEMCALL va91sysm_wt(UINT32 address, REG8 value) {

	va91sysmbyte_write[va91sysm_bank()](address, value);
}

static void MEMCALL va91knj2_wt(UINT32 address, REG8 value) {

	UINT32	offset;

	if (address < (CPUADDR_BACKUP + 0x2000)) {
		return;
	}
	offset = address - (CPUADDR_BACKUP + 0x2000);
	if (offset >= (BACKUPMEMORY_SIZE - 0x2000)) {
		return;
	}
	backupmem[0x2000 + offset] = (BYTE)value;
}

static void MEMCALL i286w_wt_va(UINT32 address, REG16 value) {

	UINT8	save;

	save = memmode_va;
	memmode_va = 0;
	i286_memorywrite_w(address, value);
	memmode_va = save;
}

static void MEMCALL i286w_wn_va(UINT32 address, REG16 value) {

	(void)address;
	(void)value;
}

static void MEMCALL bmsw_wt_va(UINT32 address, REG16 value) {

	i286w_wt_va(address, value);
}

static void MEMCALL sysmw_wt(UINT32 address, REG16 value) {

	sysmword_write[sysm_bank()](address, value);
}

static void MEMCALL tvramw_wt(UINT32 address, REG16 value) {

	STOREINTELWORD(textmem + address - CPUADDR_SYSM, value);
	textmem_dirty = TRUE;
}

static void MEMCALL gvramw_wt_va(UINT32 address, REG16 value) {

	gvramw_wt(address - CPUADDR_SYSM, value);
}

static void MEMCALL knj2w_wt(UINT32 address, REG16 value) {

	write_pair16(knj2_wt, address, value);
}

static void MEMCALL va91sysmw_wt(UINT32 address, REG16 value) {

	va91sysmword_write[va91sysm_bank()](address, value);
}

static void MEMCALL va91knj2w_wt(UINT32 address, REG16 value) {

	write_pair16(va91knj2_wt, address, value);
}

static REG8 MEMCALL i286_rd_va(UINT32 address) {

	UINT8	save;
	REG8	ret;

	save = memmode_va;
	memmode_va = 0;
	ret = i286_memoryread(address);
	memmode_va = save;
	return(ret);
}

static REG8 MEMCALL i286_rn_va(UINT32 address) {

	(void)address;
	return(0xff);
}

static REG8 MEMCALL bms_rd_va(UINT32 address) {

	return(i286_rd_va(address));
}

static REG8 MEMCALL sysm_rd(UINT32 address) {

	return(sysmbyte_read[sysm_bank()](address));
}

static REG8 MEMCALL tvram_rd(UINT32 address) {

	return(textmem[address - CPUADDR_SYSM]);
}

static REG8 MEMCALL gvram_rd_va(UINT32 address) {

	return(gvram_rd(address - CPUADDR_SYSM));
}

static REG8 MEMCALL knj1_rd(UINT32 address) {

	return(fontmem[address - CPUADDR_SYSM]);
}

static REG8 MEMCALL knj2_rd(UINT32 address) {

	if (address >= (CPUADDR_BACKUP + BACKUPMEMORY_SIZE)) {
		return(0xff);
	}
	if (address >= CPUADDR_BACKUP) {
		return(backupmem[address - CPUADDR_BACKUP]);
	}
	return(fontmem[address - (CPUADDR_SYSM - 0x40000)]);
}

static REG8 MEMCALL dic1_rd(UINT32 address) {

	return(dicmem[address - CPUADDR_SYSM]);
}

static REG8 MEMCALL dic2_rd(UINT32 address) {

	return(dicmem[address - (CPUADDR_SYSM - 0x40000)]);
}

static REG8 MEMCALL rom0_rd(UINT32 address) {

	return(rom0byte_read[memoryva.rom0_bank & 0x1f](address));
}

static REG8 MEMCALL stdrom0_rd(UINT32 address) {

	UINT	bank;
	UINT32	offset;

	bank = memoryva.rom0_bank & 0x1f;
	if (bank >= 0x0a) {
		return(0xff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM0;
	return(rom0mem[offset]);
}

static REG8 MEMCALL rom1_rd(UINT32 address) {

	return(rom1byte_read[memoryva.rom1_bank & 0x0f](address));
}

static REG8 MEMCALL stdrom1_rd(UINT32 address) {

	UINT	bank;
	UINT32	offset;

	bank = memoryva.rom1_bank & 0x03;
	if (bank & 0x02) {
		return(rom1invalid_rd(address));
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM1;
	return(rom1mem[offset]);
}

static REG8 MEMCALL va91sysm_rd(UINT32 address) {

	return(va91sysmbyte_read[va91sysm_bank()](address));
}

static REG8 MEMCALL va91knj2_rd(UINT32 address) {

	if ((address < (CPUADDR_BACKUP + 0x2000)) ||
		(address >= (CPUADDR_BACKUP + BACKUPMEMORY_SIZE))) {
		return(0xff);
	}
	return(backupmem[address - CPUADDR_BACKUP]);
}

static REG8 MEMCALL va91dic1_rd(UINT32 address) {

	return(va91dicmem[address - CPUADDR_SYSM]);
}

static REG8 MEMCALL va91dic2_rd(UINT32 address) {

	return(va91dicmem[address - (CPUADDR_SYSM - 0x40000)]);
}

static REG8 MEMCALL va91rom0_rd(UINT32 address) {

	UINT	bank;
	UINT32	offset;

	bank = va91.rom0_bank;
	if (bank >= 0x0a) {
		return(0xff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM0;
	return(va91rom0mem[offset]);
}

static REG8 MEMCALL va91rom1_rd(UINT32 address) {

	UINT	bank;
	UINT32	offset;

	bank = va91.rom1_bank;
	if (bank >= 0x02) {
		return(0xff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM1;
	return(va91rom1mem[offset]);
}

static REG16 MEMCALL i286w_rd_va(UINT32 address) {

	UINT8	save;
	REG16	ret;

	save = memmode_va;
	memmode_va = 0;
	ret = i286_memoryread_w(address);
	memmode_va = save;
	return(ret);
}

static REG16 MEMCALL i286w_rn_va(UINT32 address) {

	(void)address;
	return(0xffff);
}

static REG16 MEMCALL bmsw_rd_va(UINT32 address) {

	return(i286w_rd_va(address));
}

static REG16 MEMCALL sysmw_rd(UINT32 address) {

	return(sysmword_read[sysm_bank()](address));
}

static REG16 MEMCALL tvramw_rd(UINT32 address) {

	return(LOADINTELWORD(textmem + address - CPUADDR_SYSM));
}

static REG16 MEMCALL gvramw_rd_va(UINT32 address) {

	return(gvramw_rd(address - CPUADDR_SYSM));
}

static REG16 MEMCALL knj1w_rd(UINT32 address) {

	return(read_pair16(knj1_rd, address));
}

static REG16 MEMCALL knj2w_rd(UINT32 address) {

	return(read_pair16(knj2_rd, address));
}

static REG16 MEMCALL dic1w_rd(UINT32 address) {

	return(LOADINTELWORD(dicmem + address - CPUADDR_SYSM));
}

static REG16 MEMCALL dic2w_rd(UINT32 address) {

	return(LOADINTELWORD(dicmem + address - (CPUADDR_SYSM - 0x40000)));
}

static REG16 MEMCALL rom0w_rd(UINT32 address) {

	return(rom0word_read[memoryva.rom0_bank & 0x1f](address));
}

static REG16 MEMCALL stdrom0w_rd(UINT32 address) {

	UINT	bank;
	UINT32	offset;

	bank = memoryva.rom0_bank & 0x1f;
	if (bank >= 0x0a) {
		return(0xffff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM0;
	return(LOADINTELWORD(rom0mem + offset));
}

static REG16 MEMCALL rom1w_rd(UINT32 address) {

	return(rom1word_read[memoryva.rom1_bank & 0x0f](address));
}

static REG16 MEMCALL stdrom1w_rd(UINT32 address) {

	UINT	bank;
	UINT32	offset;

	bank = memoryva.rom1_bank & 0x03;
	if (bank & 0x02) {
		return(rom1invalidw_rd(address));
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM1;
	return(LOADINTELWORD(rom1mem + offset));
}

static REG16 MEMCALL va91sysmw_rd(UINT32 address) {

	return(va91sysmword_read[va91sysm_bank()](address));
}

static REG16 MEMCALL va91knj2w_rd(UINT32 address) {

	return(read_pair16(va91knj2_rd, address));
}

static REG16 MEMCALL va91dic1w_rd(UINT32 address) {

	return(LOADINTELWORD(va91dicmem + address - CPUADDR_SYSM));
}

static REG16 MEMCALL va91dic2w_rd(UINT32 address) {

	return(LOADINTELWORD(va91dicmem +
						address - (CPUADDR_SYSM - 0x40000)));
}

static REG16 MEMCALL va91rom0w_rd(UINT32 address) {

	UINT	bank;
	UINT32	offset;

	bank = va91.rom0_bank;
	if (bank >= 0x0a) {
		return(0xffff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM0;
	return(LOADINTELWORD(va91rom0mem + offset));
}

static REG16 MEMCALL va91rom1w_rd(UINT32 address) {

	UINT	bank;
	UINT32	offset;

	bank = va91.rom1_bank;
	if (bank >= 0x02) {
		return(0xffff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM1;
	return(LOADINTELWORD(va91rom1mem + offset));
}

void MEMCALL i286_memorywrite_va(UINT32 address, REG8 value) {

	pccore_debugmem(0, address, value);
	membyte_write[top_index(address)](address, value);
}

void MEMCALL i286_memorywrite_va_w(UINT32 address, REG16 value) {

	UINT32	next;

	pccore_debugmem(1, address, value);
	next = address + 1;
	if (next) {
		memword_write[top_index(next)](address, value);
	}
	else {
		membyte_write[0](next, (REG8)(value >> 8));
		membyte_write[0x0f](address, (REG8)value);
	}
}

REG8 MEMCALL i286_memoryread_va(UINT32 address) {

	return(membyte_read[top_index(address)](address));
}

REG16 MEMCALL i286_memoryread_va_w(UINT32 address) {

	UINT32	next;
	REG8	lo;
	REG8	hi;

	next = address + 1;
	if (next) {
		return(memword_read[top_index(next)](address));
	}
	hi = membyte_read[0](next);
	lo = membyte_read[0x0f](address);
	return((REG16)lo | ((REG16)hi << 8));
}

void MEMCALL i286_memorymap_va(void) {

	if (va91.cfg.enabled & 1) {
		sysmbyte_write[0x0f] = va91sysm_wt;
		sysmword_write[0x0f] = va91sysmw_wt;
		sysmbyte_read[0x0f] = va91sysm_rd;
		sysmword_read[0x0f] = va91sysmw_rd;
		rom0byte_read[0x0f] = va91rom0_rd;
		rom0word_read[0x0f] = va91rom0w_rd;
		rom1byte_read[0x0f] = va91rom1_rd;
		rom1word_read[0x0f] = va91rom1w_rd;
	}
	else {
		sysmbyte_write[0x0f] = i286_wn_va;
		sysmword_write[0x0f] = i286w_wn_va;
		sysmbyte_read[0x0f] = i286_rn_va;
		sysmword_read[0x0f] = i286w_rn_va;
		rom0byte_read[0x0f] = stdrom0_rd;
		rom0word_read[0x0f] = stdrom0w_rd;
		rom1byte_read[0x0f] = stdrom1_rd;
		rom1word_read[0x0f] = stdrom1w_rd;
	}
}

#endif
