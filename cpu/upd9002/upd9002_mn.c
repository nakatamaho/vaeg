#include	"compiler.h"
#include	"cpucore.h"
#include	"upd9002_ops.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"upd9002_ops.mcr"


#define	MAX_PREFIX		8


#define	NEXT_OPCODE												\
		if (UPD9002_REMCLOCK < 1) {								\
			UPD9002_BASECLOCK += (1 - UPD9002_REMCLOCK);				\
			UPD9002_REMCLOCK = 1;									\
		}

#define	REMAIN_ADJUST(c)										\
		if (UPD9002_REMCLOCK != (c)) {								\
			UPD9002_BASECLOCK += ((c) - UPD9002_REMCLOCK);			\
			UPD9002_REMCLOCK = (c);								\
		}


// ----

UPD9002FN _reserved(void) {

	INT_NUM(6, UPD9002_IP - 1);
}

UPD9002FN _add_ea_r8(void) {						// 00: add EA, REG8

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			ADDBYTE(res, dst, src);
			upd9002_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ADDBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _add_ea_r16(void) {						// 01: add EA, REG16

	UINT16	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			ADDWORD(res, dst, src);
			upd9002_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ADDWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _add_r8_ea(void) {						// 02: add REG8, EA

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ADDBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _add_r16_ea(void) {						// 03: add REG16, EA

	UINT16	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ADDWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _add_al_data8(void) {					// 04: add al, DATA8

	UINT	src;
	UINT	res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	ADDBYTE(res, UPD9002_AL, src);
	UPD9002_AL = (BYTE)res;
}

UPD9002FN _add_ax_data16(void) {					// 05: add ax, DATA16

	UINT	src;
	UINT	res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	ADDWORD(res, UPD9002_AX, src);
	UPD9002_AX = (UINT16)res;
}

UPD9002FN _push_es(void) {							// 06: push es

	REGPUSH(UPD9002_ES, 3);
}

UPD9002FN _pop_es(void) {							// 07: pop es

	UINT	tmp;

	REGPOP(tmp, 5)
	UPD9002_ES = tmp;
	ES_BASE = SEGSELECT(tmp);
}

UPD9002FN _or_ea_r8(void) {						// 08: or EA, REG8

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			ORBYTE(dst, src);
			upd9002_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ORBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _or_ea_r16(void) {							// 09: or EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			ORWORD(dst, src);
			upd9002_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ORWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _or_r8_ea(void) {						// 0a: or REG8, EA

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ORBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _or_r16_ea(void) {						// 0b: or REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ORWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _or_al_data8(void) {						// 0c: or al, DATA8

	UINT	src;
	UINT	dst;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = UPD9002_AL;
	ORBYTE(dst, src);
	UPD9002_AL = (BYTE)dst;
}

UPD9002FN _or_ax_data16(void) {					// 0d: or ax, DATA16

	UINT32	src;
	UINT32	dst;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = UPD9002_AX;
	ORWORD(dst, src);
	UPD9002_AX = (UINT16)dst;
}

UPD9002FN _push_cs(void) {							// 0e: push cs

	REGPUSH(UPD9002_CS, 3);
}

UPD9002FN _adc_ea_r8(void) {						// 10: adc EA, REG8

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			ADCBYTE(res, dst, src);
			upd9002_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ADCBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _adc_ea_r16(void) {						// 11: adc EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			ADCWORD(res, dst, src);
			upd9002_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ADCWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _adc_r8_ea(void) {						// 12: adc REG8, EA

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ADCBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _adc_r16_ea(void) {						// 13: adc REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ADCWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _adc_al_data8(void) {					// 14: adc al, DATA8

	UINT	src;
	UINT	res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	ADCBYTE(res, UPD9002_AL, src);
	UPD9002_AL = (BYTE)res;
}

UPD9002FN _adc_ax_data16(void) {					// 15: adc ax, DATA16

	UINT32	src;
	UINT32	res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	ADCWORD(res, UPD9002_AX, src);
	UPD9002_AX = (UINT16)res;
}

UPD9002FN _push_ss(void) {							// 16: push ss

	REGPUSH(UPD9002_SS, 3);
}

UPD9002FN _pop_ss(void) {							// 17: pop ss

	UINT	tmp;

	REGPOP(tmp, 5)
	UPD9002_SS = tmp;
	SS_BASE = SEGSELECT(tmp);
	SS_FIX = SS_BASE;
	NEXT_OPCODE
}

UPD9002FN _sbb_ea_r8(void) {						// 18: sbb EA, REG8

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			SBBBYTE(res, dst, src);
			upd9002_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	SBBBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _sbb_ea_r16(void) {						// 19: sbb EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			SBBWORD(res, dst, src);
			upd9002_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	SBBWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _sbb_r8_ea(void) {						// 1a: sbb REG8, EA

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT32	res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	SBBBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _sbb_r16_ea(void) {						// 1b: sbb REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	SBBWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _sbb_al_data8(void) {					// 1c: adc al, DATA8

	UINT	src;
	UINT	res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	SBBBYTE(res, UPD9002_AL, src);
	UPD9002_AL = (BYTE)res;
}

UPD9002FN _sbb_ax_data16(void) {					// 1d: adc ax, DATA16

	UINT32	src;
	UINT32	res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	SBBWORD(res, UPD9002_AX, src);
	UPD9002_AX = (UINT16)res;
}

UPD9002FN _push_ds(void) {							// 1e: push ds

	REGPUSH(UPD9002_DS, 3);
}

UPD9002FN _pop_ds(void) {							// 1f: pop ds

	UINT	tmp;

	REGPOP(tmp, 5)
	UPD9002_DS = tmp;
	DS_BASE = SEGSELECT(tmp);
	DS_FIX = DS_BASE;
}

UPD9002FN _and_ea_r8(void) {						// 20: and EA, REG8

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			ANDBYTE(dst, src);
			upd9002_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ANDBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _and_ea_r16(void) {						// 21: and EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			ANDWORD(dst, src);
			upd9002_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ANDWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _and_r8_ea(void) {						// 22: and REG8, EA

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ANDBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _and_r16_ea(void) {						// 23: and REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ANDWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _and_al_data8(void) {					// 24: and al, DATA8

	UINT	src;
	UINT	dst;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = UPD9002_AL;
	ANDBYTE(dst, src);
	UPD9002_AL = (BYTE)dst;
}

UPD9002FN _and_ax_data16(void) {					// 25: and ax, DATA16

	UINT32	src;
	UINT32	dst;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = UPD9002_AX;
	ANDWORD(dst, src);
	UPD9002_AX = (UINT16)dst;
}

UPD9002FN _segprefix_es(void) {					// 26: es:

	SS_FIX = ES_BASE;
	DS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _daa(void) {								// 27: daa

	UPD9002_WORKCLOCK(3);
	UPD9002_OV = ((UPD9002_AL < 0x80) && 
				((UPD9002_AL >= 0x7a) ||
				((UPD9002_AL >= 0x1a) && (UPD9002_FLAGL & C_FLAG))));
	if ((UPD9002_FLAGL & A_FLAG) || ((UPD9002_AL & 0x0f) > 9)) {
		UPD9002_FLAGL |= A_FLAG;
		UPD9002_FLAGL |= (BYTE)((UPD9002_AL + 6) >> 8);
		UPD9002_AL += 6;
	}
	if ((UPD9002_FLAGL & C_FLAG) || (UPD9002_AL > 0x9f)) {
		UPD9002_FLAGL |= C_FLAG;
		UPD9002_AL += 0x60;
	}
	UPD9002_FLAGL &= A_FLAG | C_FLAG;
	UPD9002_FLAGL |= BYTESZPF(UPD9002_AL);
}

UPD9002FN _sub_ea_r8(void) {						// 28: sub EA, REG8

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			SUBBYTE(res, dst, src);
			upd9002_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	SUBBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _sub_ea_r16(void) {						// 29: sub EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			SUBWORD(res, dst, src);
			upd9002_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	SUBWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _sub_r8_ea(void) {						// 2a: sub REG8, EA

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	SUBBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _sub_r16_ea(void) {						// 2b: sub REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	SUBWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _sub_al_data8(void) {					// 2c: sub al, DATA8

	UINT	src;
	UINT	res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	SUBBYTE(res, UPD9002_AL, src);
	UPD9002_AL = (BYTE)res;
}

UPD9002FN _sub_ax_data16(void) {					// 2d: sub ax, DATA16

	UINT32	src;
	UINT32	res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	SUBWORD(res, UPD9002_AX, src);
	UPD9002_AX = (UINT16)res;
}

UPD9002FN _segprefix_cs(void) {					// 2e: cs:

	SS_FIX = CS_BASE;
	DS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _das(void) {								// 2f: das

	UPD9002_WORKCLOCK(3);
	if ((UPD9002_FLAGL & C_FLAG) || (UPD9002_AL > 0x99)) {
		UPD9002_FLAGL |= C_FLAG;
		UPD9002_AL -= 0x60;
	}
	if ((UPD9002_FLAGL & A_FLAG) || ((UPD9002_AL & 0x0f) > 9)) {
		UPD9002_FLAGL |= A_FLAG;
		UPD9002_FLAGL |= ((UPD9002_AL - 6) >> 8) & 1;
		UPD9002_AL -= 6;
	}
	UPD9002_FLAGL &= A_FLAG | C_FLAG;
	UPD9002_FLAGL |= BYTESZPF(UPD9002_AL);
}

UPD9002FN _xor_ea_r8(void) {						// 30: xor EA, REG8

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			XORBYTE(dst, src);
			upd9002_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	XORBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _xor_ea_r16(void) {						// 31: xor EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			XORWORD(dst, src);
			upd9002_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	XORWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _xor_r8_ea(void) {						// 32: xor REG8, EA

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	XORBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _xor_r16_ea(void) {						// 33: or REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	XORWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _xor_al_data8(void) {					// 34: or al, DATA8

	UINT	src;
	UINT	dst;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = UPD9002_AL;
	XORBYTE(dst, src);
	UPD9002_AL = (BYTE)dst;
}

UPD9002FN _xor_ax_data16(void) {					// 35: or ax, DATA16

	UINT32	src;
	UINT32	dst;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = UPD9002_AX;
	XORWORD(dst, src);
	UPD9002_AX = (UINT16)dst;
}

UPD9002FN _segprefix_ss(void) {					// 36: ss:

	SS_FIX = SS_BASE;
	DS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _aaa(void) {								// 37: aaa

	UPD9002_WORKCLOCK(3);
	if ((UPD9002_FLAGL & A_FLAG) || ((UPD9002_AL & 0xf) > 9)) {
		UPD9002_FLAGL |= A_FLAG | C_FLAG;
		UPD9002_AX += 6;
		UPD9002_AH++;
	}
	else {
		UPD9002_FLAGL &= ~(A_FLAG | C_FLAG);
	}
	UPD9002_AL &= 0x0f;
}

UPD9002FN _cmp_ea_r8(void) {						// 38: cmp EA, REG8

	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		dst = *(REG8_B20(op));
		SUBBYTE(res, dst, src);
	}
	else {
		UPD9002_WORKCLOCK(7);
		dst = upd9002_memoryread(CALC_EA(op));
		SUBBYTE(res, dst, src);
	}
}

UPD9002FN _cmp_ea_r16(void) {						// 39: cmp EA, REG16

	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		dst = *(REG16_B20(op));
		SUBWORD(res, dst, src);
	}
	else {
		UPD9002_WORKCLOCK(7);
		dst = upd9002_memoryread_w(CALC_EA(op));
		SUBWORD(res, dst, src);
	}
}

UPD9002FN _cmp_r8_ea(void) {						// 3a: cmp REG8, EA

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	dst;
	UINT	res;

	PREPART_REG8_EA(op, src, out, 2, 6);
	dst = *out;
	SUBBYTE(res, dst, src);
}

UPD9002FN _cmp_r16_ea(void) {						// 3b: cmp REG16, EA

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	dst;
	UINT32	res;

	PREPART_REG16_EA(op, src, out, 2, 6);
	dst = *out;
	SUBWORD(res, dst, src);
}

UPD9002FN _cmp_al_data8(void) {					// 3c: cmp al, DATA8

	UINT	src;
	UINT	res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	SUBBYTE(res, UPD9002_AL, src);
}

UPD9002FN _cmp_ax_data16(void) {					// 3d: cmp ax, DATA16

	UINT32	src;
	UINT32	res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	SUBWORD(res, UPD9002_AX, src);
}

UPD9002FN _segprefix_ds(void) {					// 3e: ds:

	SS_FIX = DS_BASE;
	DS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _aas(void) {								// 3f: aas

	UPD9002_WORKCLOCK(3);
	if ((UPD9002_FLAGL & A_FLAG) || ((UPD9002_AL & 0xf) > 9)) {
		UPD9002_FLAGL |= A_FLAG | C_FLAG;
		UPD9002_AX -= 6;
		UPD9002_AH--;
	}
	else {
		UPD9002_FLAGL &= ~(A_FLAG | C_FLAG);
	}
}

UPD9002FN _inc_ax(void) INCWORD2(UPD9002_AX, 2) 	// 40:	inc		ax
UPD9002FN _inc_cx(void) INCWORD2(UPD9002_CX, 2)	// 41:	inc		cx
UPD9002FN _inc_dx(void) INCWORD2(UPD9002_DX, 2)	// 42:	inc		dx
UPD9002FN _inc_bx(void) INCWORD2(UPD9002_BX, 2)	// 43:	inc		bx
UPD9002FN _inc_sp(void) INCWORD2(UPD9002_SP, 2)	// 44:	inc		sp
UPD9002FN _inc_bp(void) INCWORD2(UPD9002_BP, 2)	// 45:	inc		bp
UPD9002FN _inc_si(void) INCWORD2(UPD9002_SI, 2)	// 46:	inc		si
UPD9002FN _inc_di(void) INCWORD2(UPD9002_DI, 2)	// 47:	inc		di
UPD9002FN _dec_ax(void) DECWORD2(UPD9002_AX, 2)	// 48:	dec		ax
UPD9002FN _dec_cx(void) DECWORD2(UPD9002_CX, 2)	// 49:	dec		cx
UPD9002FN _dec_dx(void) DECWORD2(UPD9002_DX, 2)	// 4a:	dec		dx
UPD9002FN _dec_bx(void) DECWORD2(UPD9002_BX, 2)	// 4b:	dec		bx
UPD9002FN _dec_sp(void) DECWORD2(UPD9002_SP, 2)	// 4c:	dec		sp
UPD9002FN _dec_bp(void) DECWORD2(UPD9002_BP, 2)	// 4d:	dec		bp
UPD9002FN _dec_si(void) DECWORD2(UPD9002_SI, 2)	// 4e:	dec		si
UPD9002FN _dec_di(void) DECWORD2(UPD9002_DI, 2)	// 4f:	dec		di

UPD9002FN _push_ax(void) REGPUSH(UPD9002_AX, 3)	// 50:	push	ax
UPD9002FN _push_cx(void) REGPUSH(UPD9002_CX, 3)	// 51:	push	cx
UPD9002FN _push_dx(void) REGPUSH(UPD9002_DX, 3)	// 52:	push	dx
UPD9002FN _push_bx(void) REGPUSH(UPD9002_BX, 3)	// 53:	push	bx
UPD9002FN _push_sp(void) SP_PUSH(UPD9002_SP, 3)	// 54:	push	sp
UPD9002FN _push_bp(void) REGPUSH(UPD9002_BP, 3)	// 55:	push	bp
UPD9002FN _push_si(void) REGPUSH(UPD9002_SI, 3)	// 56:	push	si
UPD9002FN _push_di(void) REGPUSH(UPD9002_DI, 3)	// 57:	push	di
UPD9002FN _pop_ax(void) REGPOP(UPD9002_AX, 5)		// 58:	pop		ax
UPD9002FN _pop_cx(void) REGPOP(UPD9002_CX, 5)		// 59:	pop		cx
UPD9002FN _pop_dx(void) REGPOP(UPD9002_DX, 5)		// 5A:	pop		dx
UPD9002FN _pop_bx(void) REGPOP(UPD9002_BX, 5)		// 5B:	pop		bx
UPD9002FN _pop_sp(void) SP_POP(UPD9002_SP, 5)		// 5C:	pop		sp
UPD9002FN _pop_bp(void) REGPOP(UPD9002_BP, 5)		// 5D:	pop		bp
UPD9002FN _pop_si(void) REGPOP(UPD9002_SI, 5)		// 5E:	pop		si
UPD9002FN _pop_di(void) REGPOP(UPD9002_DI, 5)		// 5F:	pop		di

#if (defined(ARM) || defined(X11)) && defined(BYTESEX_LITTLE)

UPD9002FN _pusha(void) {						// 60:	pusha

	REG16	tmp;
	UINT32	addr;

	UPD9002_WORKCLOCK(17);
	tmp = UPD9002_SP;
	addr = tmp + SS_BASE;
	if ((tmp < 16) || (INHIBIT_WORDP(addr))) {
		REGPUSH0(UPD9002_AX)
		REGPUSH0(UPD9002_CX)
		REGPUSH0(UPD9002_DX)
		REGPUSH0(UPD9002_BX)
	    REGPUSH0(tmp)
		REGPUSH0(UPD9002_BP)
		REGPUSH0(UPD9002_SI)
		REGPUSH0(UPD9002_DI)
	}
	else {
		*(UINT16 *)(mem + addr - 2) = UPD9002_AX;
		*(UINT16 *)(mem + addr - 4) = UPD9002_CX;
		*(UINT16 *)(mem + addr - 6) = UPD9002_DX;
		*(UINT16 *)(mem + addr - 8) = UPD9002_BX;
		*(UINT16 *)(mem + addr - 10) = tmp;
		*(UINT16 *)(mem + addr - 12) = UPD9002_BP;
		*(UINT16 *)(mem + addr - 14) = UPD9002_SI;
		*(UINT16 *)(mem + addr - 16) = UPD9002_DI;
		UPD9002_SP -= 16;
	}
}

UPD9002FN _popa(void) {						// 61:	popa

	UINT	tmp;
	UINT32	addr;

	UPD9002_WORKCLOCK(19);
	tmp = UPD9002_SP + 16;
	addr = tmp + SS_BASE;
	if ((tmp >= 0x10000) || (INHIBIT_WORDP(addr))) {
		UPD9002_DI = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_SI = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_BP = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_SP += 2;
		UPD9002_BX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_DX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_CX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_AX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
	}
	else {
		UPD9002_DI = *(UINT16 *)(mem + addr - 16);
		UPD9002_SI = *(UINT16 *)(mem + addr - 14);
		UPD9002_BP = *(UINT16 *)(mem + addr - 12);
		UPD9002_BX = *(UINT16 *)(mem + addr - 8);
		UPD9002_DX = *(UINT16 *)(mem + addr - 6);
		UPD9002_CX = *(UINT16 *)(mem + addr - 4);
		UPD9002_AX = *(UINT16 *)(mem + addr - 2);
		UPD9002_SP = tmp;
	}
}

#else

UPD9002FN _pusha(void) {						// 60:	pusha

	REG16	tmp;

	tmp = UPD9002_SP;
	REGPUSH0(UPD9002_AX)
	REGPUSH0(UPD9002_CX)
	REGPUSH0(UPD9002_DX)
	REGPUSH0(UPD9002_BX)
    REGPUSH0(tmp)
	REGPUSH0(UPD9002_BP)
	REGPUSH0(UPD9002_SI)
	REGPUSH0(UPD9002_DI)
	UPD9002_WORKCLOCK(17);
}

UPD9002FN _popa(void) {						// 61:	popa

	UPD9002_DI = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_SI = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_BP = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_SP += 2;
	UPD9002_BX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_DX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_CX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_AX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_WORKCLOCK(19);
}

#endif

UPD9002FN _bound(void) {						// 62:	bound

	UINT	vect = 0;
	UINT	op;
	UINT32	madr;
	SINT16	reg;
	SINT16	lower;
	SINT16	upper;

	UPD9002_WORKCLOCK(13);										// ToDo
	GET_PCBYTE(op);
	if (op < 0xc0) {
		reg = (SINT16)*(REG16_B53(op));
		madr = CALC_EA(op);
		lower = (SINT16)upd9002_memoryread_w(madr);
		madr += 2;											// ToDo
		upper = (SINT16)upd9002_memoryread_w(madr);
		if ((reg >= lower) && (reg <= upper)) {
			return;
		}
		vect = 5;
	}
	else {
		vect = 6;
	}
	INT_NUM(vect, UPD9002_IP);
}

UPD9002FN _push_data16(void) {				// 68:	push	DATA16

	UINT16	tmp;

	GET_PCWORD(tmp)
	REGPUSH(tmp, 3)
}

UPD9002FN _imul_reg_ea_data16(void) {		// 69:	imul	REG, EA, DATA16

	UINT16	*out;
	UINT	op;
	SINT16	src;
	SINT16	dst;
	SINT32	res;

	PREPART_REG16_EA(op, src, out, 21, 24)
	GET_PCWORD(dst)
	WORD_IMUL(res, dst, src)
	*out = (UINT16)res;
}

UPD9002FN _push_data8(void) {				// 6A:	push	DATA8

	UINT16	tmp;

	GET_PCBYTES(tmp)
	REGPUSH(tmp, 3)
}

UPD9002FN _imul_reg_ea_data8(void) {		// 6B:	imul	REG, EA, DATA8

	UINT16	*out;
	UINT	op;
	SINT16	src;
	SINT16	dst;
	SINT32	res;

	PREPART_REG16_EA(op, src, out, 21, 24)
	GET_PCBYTES(dst)
	WORD_IMUL(res, dst, src)
	*out = (UINT16)res;
}

UPD9002FN _insb(void) {						// 6C:	insb

	REG8	dat;

	UPD9002_WORKCLOCK(5);
	dat = iocore_inp8(UPD9002_DX);
	upd9002_memorywrite(UPD9002_DI + ES_BASE, dat);
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _insw(void) {						// 6D:	insw

	REG16	dat;

	UPD9002_WORKCLOCK(5);
	dat = iocore_inp16(UPD9002_DX);
	upd9002_memorywrite_w(UPD9002_DI + ES_BASE, dat);
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _outsb(void) {						// 6E:	outsb

	REG8	dat;

	UPD9002_WORKCLOCK(3);
	dat = upd9002_memoryread(UPD9002_SI + DS_FIX);
	UPD9002_SI += STRING_DIR;
	iocore_out8(UPD9002_DX, (BYTE)dat);
}

UPD9002FN _outsw(void) {						// 6F:	outsw

	REG16	dat;

	UPD9002_WORKCLOCK(3);
	dat = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
	UPD9002_SI += STRING_DIRx2;
	iocore_out16(UPD9002_DX, (UINT16)dat);
}

UPD9002FN _jo_short(void) {					// 70:	jo short

	if (!UPD9002_OV) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jno_short(void) {					// 71:	jno short

	if (UPD9002_OV) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jc_short(void) {					// 72:	jnae/jb/jc short

	if (!(UPD9002_FLAGL & C_FLAG)) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnc_short(void) {					// 73:	jae/jnb/jnc short

	if (UPD9002_FLAGL & C_FLAG) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jz_short(void) {					// 74:	je/jz short

	if (!(UPD9002_FLAGL & Z_FLAG)) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnz_short(void) {					// 75:	jne/jnz short

	if (UPD9002_FLAGL & Z_FLAG) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jna_short(void) {					// 76:	jna/jbe short

	if (!(UPD9002_FLAGL & (Z_FLAG | C_FLAG))) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _ja_short(void) {					// 77:	ja/jnbe short
	if (UPD9002_FLAGL & (Z_FLAG | C_FLAG)) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _js_short(void) {					// 78:	js short

	if (!(UPD9002_FLAGL & S_FLAG)) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jns_short(void) {					// 79:	jns short

	if (UPD9002_FLAGL & S_FLAG) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jp_short(void) {					// 7A:	jp/jpe short

	if (!(UPD9002_FLAGL & P_FLAG)) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnp_short(void) {					// 7B:	jnp/jpo short

	if (UPD9002_FLAGL & P_FLAG) JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jl_short(void) {					// 7C:	jl/jnge short

	if (((UPD9002_FLAGL & S_FLAG) == 0) == (UPD9002_OV == 0))
												JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnl_short(void) {					// 7D:	jnl/jge short

	if (((UPD9002_FLAGL & S_FLAG) == 0) != (UPD9002_OV == 0))
												JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jle_short(void) {					// 7E:	jle/jng short

	if ((!(UPD9002_FLAGL & Z_FLAG)) &&
		(((UPD9002_FLAGL & S_FLAG) == 0) == (UPD9002_OV == 0)))
												JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnle_short(void) {					// 7F:	jg/jnle short

	if ((UPD9002_FLAGL & Z_FLAG) ||
		(((UPD9002_FLAGL & S_FLAG) == 0) != (UPD9002_OV == 0)))
												JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _calc_ea8_i8(void) {					// 80:	op		EA8, DATA8
											// 82:	op		EA8, DATA8
	BYTE	*out;
	UINT	op;
	UINT32	madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			c_op8xext8_table[(op >> 3) & 7](madr);
			return;
		}
		out = mem + madr;
	}
	c_op8xreg8_table[(op >> 3) & 7](out);
}

UPD9002FN _calc_ea16_i16(void) {				// 81:	op		EA16, DATA16

	UINT16	*out;
	UINT	op;
	UINT32	madr;
	UINT32	src;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		{
			UINT32 seg;
			UINT off;

			off = GET_EA(op, &seg);
			madr = seg + off;
			if ((LOW16(off) == 0xffff) || INHIBIT_WORDP(madr)) {
				UINT16 tmp;

				GET_PCWORD(src);
				tmp = upd9002_memoryread_seg_w(seg, off);
				c_op8xreg16_table[(op >> 3) & 7](&tmp, src);
				if (((op >> 3) & 7) != 7) {
					upd9002_memorywrite_seg_w(seg, off, tmp);
				}
				return;
			}
		}
		if (INHIBIT_WORDP(madr)) {
			GET_PCWORD(src);
			c_op8xext16_table[(op >> 3) & 7](madr, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCWORD(src);
	c_op8xreg16_table[(op >> 3) & 7](out, src);
}

UPD9002FN _calc_ea16_i8(void) {				// 83:	op		EA16, DATA8

	UINT16	*out;
	UINT	op;
	UINT32	madr;
	UINT32	src;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		{
			UINT32 seg;
			UINT off;

			off = GET_EA(op, &seg);
			madr = seg + off;
			if ((LOW16(off) == 0xffff) || INHIBIT_WORDP(madr)) {
				UINT16 tmp;

				GET_PCBYTES(src);
				tmp = upd9002_memoryread_seg_w(seg, off);
				c_op8xreg16_table[(op >> 3) & 7](&tmp, src);
				if (((op >> 3) & 7) != 7) {
					upd9002_memorywrite_seg_w(seg, off, tmp);
				}
				return;
			}
		}
		if (INHIBIT_WORDP(madr)) {
			GET_PCBYTES(src);
			c_op8xext16_table[(op >> 3) & 7](madr, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCBYTES(src);
	c_op8xreg16_table[(op >> 3) & 7](out, src);
}

UPD9002FN _test_ea_r8(void) {					// 84:	test	EA, REG8

	BYTE	*out;
	UINT	op;
	UINT	src;
	UINT	tmp;
	UINT32	madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(6);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			tmp = upd9002_memoryread(madr);
			ANDBYTE(tmp, src);
			return;
		}
		out = mem + madr;
	}
	tmp = *out;
	ANDBYTE(tmp, src);
}

UPD9002FN _test_ea_r16(void) {					// 85:	test	EA, REG16

	UINT16	*out;
	UINT	op;
	UINT32	src;
	UINT32	tmp;
	UINT32	madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			tmp = upd9002_memoryread_w(madr);
			ANDWORD(tmp, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	tmp = *out;
	ANDWORD(tmp, src);
}

UPD9002FN _xchg_ea_r8(void) {					// 86:	xchg	EA, REG8

	BYTE	*out;
	BYTE	*src;
	UINT	op;
	UINT32	madr;

	PREPART_EA_REG8P(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(5);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			BYTE tmp = upd9002_memoryread(madr);
			upd9002_memorywrite(madr, *src);
			*src = tmp;
			return;
		}
		out = mem + madr;
	}
	SWAPBYTE(*out, *src);
}

UPD9002FN _xchg_ea_r16(void) {					// 87:	xchg	EA, REG16

	UINT16	*out;
	UINT16	*src;
	UINT	op;
	UINT32	madr;

	PREPART_EA_REG16P(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(5);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			UINT16 tmp = upd9002_memoryread_w(madr);
			upd9002_memorywrite_w(madr, *src);
			*src = tmp;
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	SWAPWORD(*out, *src);
}

UPD9002FN _mov_ea_r8(void) {					// 88:	mov		EA, REG8

	BYTE	src;
	UINT	op;
	UINT32	madr;

	PREPART_EA_REG8(op, src)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		*(REG8_B20(op)) = src;
	}
	else {
		UPD9002_WORKCLOCK(3);
		madr = CALC_EA(op);
		upd9002_memorywrite(madr, src);
	}
}

UPD9002FN _mov_ea_r16(void) {					// 89:	mov		EA, REG16

	UINT16	src;
	UINT	op;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		*(REG16_B20(op)) = src;
	}
	else {
		UPD9002_WORKCLOCK(3);
		upd9002_memorywrite_w(CALC_EA(op), src);
	}
}

UPD9002FN _mov_r8_ea(void) {					// 8A:	mov		REG8, EA

	BYTE	*out;
	BYTE	src;
	UINT	op;

	PREPART_REG8_EA(op, src, out, 2, 5);
	*out = src;
}

UPD9002FN _mov_r16_ea(void) {					// 8B:	mov		REG16, EA

	UINT16	*out;
	UINT16	src;
	UINT	op;

	PREPART_REG16_EA(op, src, out, 2, 5);
	*out = src;
}

UPD9002FN _mov_ea_seg(void) {					// 8C:	mov		EA, segreg

	UINT	op;
	UINT16	tmp;

	GET_PCBYTE(op);
	tmp = *SEGMENTPTR((op >> 3) & 3);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		*(REG16_B20(op)) = tmp;
	}
	else {
		UPD9002_WORKCLOCK(3);
		upd9002_memorywrite_w(CALC_EA(op), tmp);
	}
}

UPD9002FN _lea_r16_ea(void) {					// 8D:	lea		REG16, EA

	UINT	op;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		*(REG16_B53(op)) = CALC_LEA(op);
	}
	else {
		INT_NUM(6, UPD9002_SP - 2);
	}
}

UPD9002FN _pop_ea(void) {						// 8F:	pop		EA

	UINT	op;
	UINT16	tmp;

	UPD9002_WORKCLOCK(5);
	REGPOP0(tmp)

	GET_PCBYTE(op)
	if (op < 0xc0) {
		upd9002_memorywrite_w(CALC_EA(op), tmp);
	}
	else {
		*(REG16_B20(op)) = tmp;
	}
}

UPD9002FN _nop(void) {							// 90: nop / bios func

#if 1										// call BIOS
	UINT32	adrs;

	adrs = LOW16(UPD9002_IP - 1) + CS_BASE;
	if ((adrs >= 0xf8000) && (adrs < 0x100000)) {
		biosfunc(adrs);
		ES_BASE = UPD9002_ES << 4;
		CS_BASE = UPD9002_CS << 4;
		SS_BASE = UPD9002_SS << 4;
		SS_FIX = SS_BASE;
		DS_BASE = UPD9002_DS << 4;
		DS_FIX = DS_BASE;
	}
#endif
	UPD9002_WORKCLOCK(3);
}

UPD9002FN _xchg_ax_cx(void) { 					// 91:	xchg	ax, cx

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_CX);
}

UPD9002FN _xchg_ax_dx(void) { 					// 92:	xchg	ax, dx

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_DX);
}

UPD9002FN _xchg_ax_bx(void) { 					// 93:	xchg	ax, bx

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_BX);
}

UPD9002FN _xchg_ax_sp(void) { 					// 94:	xchg	ax, sp

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_SP);
}

UPD9002FN _xchg_ax_bp(void) { 					// 95:	xchg	ax, bp

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_BP);
}

UPD9002FN _xchg_ax_si(void) { 					// 96:	xchg	ax, si

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_SI);
}

UPD9002FN _xchg_ax_di(void) { 					// 97:	xchg	ax, di

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_DI);
}

UPD9002FN _cbw(void) {							// 98:	cbw

	UPD9002_WORKCLOCK(2);
	UPD9002_AX = __CBW(UPD9002_AL);
}

UPD9002FN _cwd(void) {							// 99:	cwd

	UPD9002_WORKCLOCK(2);
	UPD9002_DX = ((UPD9002_AH & 0x80)?0xffff:0x0000);
}

UPD9002FN _call_far(void) {					// 9A:	call far

	UINT16	newip;

	UPD9002_WORKCLOCK(13);
	REGPUSH0(UPD9002_CS)
	GET_PCWORD(newip)
	GET_PCWORD(UPD9002_CS)
	CS_BASE = SEGSELECT(UPD9002_CS);
	REGPUSH0(UPD9002_IP)
	UPD9002_IP = newip;
}

UPD9002FN _wait(void) {						// 9B:	wait

	UPD9002_WORKCLOCK(2);
}

UPD9002FN _pushf(void) {						// 9C:	pushf

	UPD9002_WORKCLOCK(3);
	UPD9002_SP -= 2;
	upd9002_memorywrite_seg_w(SS_BASE, UPD9002_SP, REAL_FLAGREG);
}

UPD9002FN _popf(void) {						// 9D:	popf

	UINT	flag;

	REGPOP0(flag)
	UPD9002_OV = flag & O_FLAG;
	UPD9002_FLAG = flag & (0xfff ^ O_FLAG);
	UPD9002_TRAP = ((flag & 0x300) == 0x300);
	UPD9002_WORKCLOCK(5);
#if defined(INTR_FAST)
	if ((UPD9002_TRAP) || ((flag & I_FLAG) && (PICEXISTINTR))) {
		UPD9002_IRQCHECKTERM
	}
#else
	UPD9002_IRQCHECKTERM
#endif
}

UPD9002FN _sahf(void) {						// 9E:	sahf

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAGL = (UINT8)((UPD9002_AH & 0xd5) | 0x02);
}

UPD9002FN _lahf(void) {						// 9F:	lahf

	UPD9002_WORKCLOCK(2);
	UPD9002_AH = UPD9002_FLAGL;
}

UPD9002FN _mov_al_m8(void) {					// A0:	mov		al, m8

	UINT	op;

	UPD9002_WORKCLOCK(5);
	GET_PCWORD(op)
	UPD9002_AL = upd9002_memoryread(DS_FIX + op);
}

UPD9002FN _mov_ax_m16(void) {					// A1:	mov		ax, m16

	UINT	op;

	UPD9002_WORKCLOCK(5);
	GET_PCWORD(op)
	UPD9002_AX = upd9002_memoryread_w(DS_FIX + op);
}

UPD9002FN _mov_m8_al(void) {					// A2:	mov		m8, al

	UINT	op;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(op)
	upd9002_memorywrite(DS_FIX + op, UPD9002_AL);
}

UPD9002FN _mov_m16_ax(void) {					// A3:	mov		m16, ax

	UINT	op;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(op);
	upd9002_memorywrite_w(DS_FIX + op, UPD9002_AX);
}

UPD9002FN _movsb(void) {						// A4:	movsb

	BYTE	tmp;

	UPD9002_WORKCLOCK(5);
	tmp = upd9002_memoryread(UPD9002_SI + DS_FIX);
	upd9002_memorywrite(UPD9002_DI + ES_BASE, tmp);
	UPD9002_SI += STRING_DIR;
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _movsw(void) {						// A5:	movsw

	UINT16	tmp;

	UPD9002_WORKCLOCK(5);
	tmp = upd9002_memoryread_seg_w(DS_FIX, UPD9002_SI);
	upd9002_memorywrite_seg_w(ES_BASE, UPD9002_DI, tmp);
	UPD9002_SI += STRING_DIRx2;
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _cmpsb(void) {						// A6:	cmpsb

	UINT	src;
	UINT	dst;
	UINT	res;

	UPD9002_WORKCLOCK(8);
	dst = upd9002_memoryread(UPD9002_SI + DS_FIX);
	src = upd9002_memoryread(UPD9002_DI + ES_BASE);
	SUBBYTE(res, dst, src)
	UPD9002_SI += STRING_DIR;
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _cmpsw(void) {						// A7:	cmpsw

	UINT32	src;
	UINT32	dst;
	UINT32	res;

	UPD9002_WORKCLOCK(8);
	dst = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
	src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
	SUBWORD(res, dst, src)
	UPD9002_SI += STRING_DIRx2;
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _test_al_data8(void) {				// A8:	test	al, DATA8

	UINT	src;
	UINT	dst;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src)
	dst = UPD9002_AL;
	ANDBYTE(dst, src)
}

UPD9002FN _test_ax_data16(void) {				// A9:	test	ax, DATA16

	UINT32	src;
	UINT32	dst;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src)
	dst = UPD9002_AX;
	ANDWORD(dst, src)
}

UPD9002FN _stosb(void) {						// AA:	stosw

	UPD9002_WORKCLOCK(3);
	upd9002_memorywrite(UPD9002_DI + ES_BASE, UPD9002_AL);
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _stosw(void) {						// AB:	stosw

	UPD9002_WORKCLOCK(3);
	upd9002_memorywrite_w(UPD9002_DI + ES_BASE, UPD9002_AX);
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _lodsb(void) {						// AC:	lodsb

	UPD9002_WORKCLOCK(5);
	UPD9002_AL = upd9002_memoryread(UPD9002_SI + DS_FIX);
	UPD9002_SI += STRING_DIR;
}

UPD9002FN _lodsw(void) {						// AD:	lodsw

	UPD9002_WORKCLOCK(5);
	UPD9002_AX = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
	UPD9002_SI += STRING_DIRx2;
}

UPD9002FN _scasb(void) {						// AE:	scasb

	UINT	src;
	UINT	dst;
	UINT	res;

	UPD9002_WORKCLOCK(7);
	src = upd9002_memoryread(UPD9002_DI + ES_BASE);
	dst = UPD9002_AL;
	SUBBYTE(res, dst, src)
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _scasw(void) {						// AF:	scasw

	UINT32	src;
	UINT32	dst;
	UINT32	res;

	UPD9002_WORKCLOCK(7);
	src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
	dst = UPD9002_AX;
	SUBWORD(res, dst, src)
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _mov_al_imm(void) MOVIMM8(UPD9002_AL)	// B0:	mov		al, imm8
UPD9002FN _mov_cl_imm(void) MOVIMM8(UPD9002_CL)	// B1:	mov		cl, imm8
UPD9002FN _mov_dl_imm(void) MOVIMM8(UPD9002_DL)	// B2:	mov		dl, imm8
UPD9002FN _mov_bl_imm(void) MOVIMM8(UPD9002_BL)	// B3:	mov		bl, imm8
UPD9002FN _mov_ah_imm(void) MOVIMM8(UPD9002_AH)	// B4:	mov		ah, imm8
UPD9002FN _mov_ch_imm(void) MOVIMM8(UPD9002_CH)	// B5:	mov		ch, imm8
UPD9002FN _mov_dh_imm(void) MOVIMM8(UPD9002_DH)	// B6:	mov		dh, imm8
UPD9002FN _mov_bh_imm(void) MOVIMM8(UPD9002_BH)	// B7:	mov		bh, imm8
UPD9002FN _mov_ax_imm(void) MOVIMM16(UPD9002_AX)	// B8:	mov		ax, imm16
UPD9002FN _mov_cx_imm(void) MOVIMM16(UPD9002_CX)	// B9:	mov		cx, imm16
UPD9002FN _mov_dx_imm(void) MOVIMM16(UPD9002_DX)	// BA:	mov		dx, imm16
UPD9002FN _mov_bx_imm(void) MOVIMM16(UPD9002_BX)	// BB:	mov		bx, imm16
UPD9002FN _mov_sp_imm(void) MOVIMM16(UPD9002_SP)	// BC:	mov		sp, imm16
UPD9002FN _mov_bp_imm(void) MOVIMM16(UPD9002_BP)	// BD:	mov		bp, imm16
UPD9002FN _mov_si_imm(void) MOVIMM16(UPD9002_SI)	// BE:	mov		si, imm16
UPD9002FN _mov_di_imm(void) MOVIMM16(UPD9002_DI)	// BF:	mov		di, imm16

UPD9002FN _shift_ea8_data8(void) {				// C0:	shift	EA8, DATA8

	BYTE	*out;
	UINT	op;
	UINT32	madr;
	BYTE	cl;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			GET_PCBYTE(cl)
			UPD9002_WORKCLOCK(cl);
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	GET_PCBYTE(cl)
	UPD9002_WORKCLOCK(cl);
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN _shift_ea16_data8(void) {			// C1:	shift	EA16, DATA8

	UINT16	*out;
	UINT	op;
	UINT32	madr;
	BYTE	cl;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			GET_PCBYTE(cl);
			UPD9002_WORKCLOCK(cl);
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCBYTE(cl);
	UPD9002_WORKCLOCK(cl);
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN _ret_near_data16(void) {				// C2:	ret near DATA16

	UINT16	ad;

	UPD9002_WORKCLOCK(11);
	GET_PCWORD(ad)
	REGPOP0(UPD9002_IP)
	UPD9002_SP += ad;
}

UPD9002FN _ret_near(void) {					// C3:	ret near

	UPD9002_WORKCLOCK(11);
	REGPOP0(UPD9002_IP)
}

UPD9002FN _les_r16_ea(void) {					// C4:	les		REG16, EA

	UINT	op;
	UINT32	seg;
	UINT	ad;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		ad = GET_EA(op, &seg);
		*(REG16_B53(op)) = upd9002_memoryread_seg_w(seg, ad);
		UPD9002_ES = upd9002_memoryread_seg_w(seg, LOW16(ad + 2));
		ES_BASE = SEGSELECT(UPD9002_ES);
	}
	else {
		INT_NUM(6, UPD9002_IP - 2);
	}
}

UPD9002FN _lds_r16_ea(void) {					// C5:	lds		REG16, EA

	UINT	op;
	UINT32	seg;
	UINT	ad;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		ad = GET_EA(op, &seg);
		*(REG16_B53(op)) = upd9002_memoryread_seg_w(seg, ad);
		UPD9002_DS = upd9002_memoryread_seg_w(seg, LOW16(ad + 2));
		DS_BASE = SEGSELECT(UPD9002_DS);
		DS_FIX = DS_BASE;
	}
	else {
		INT_NUM(6, UPD9002_IP - 2);
	}
}

UPD9002FN _mov_ea8_data8(void) {				// C6:	mov		EA8, DATA8

	UINT	op;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		GET_PCBYTE(*(REG8_B20(op)))
	}
	else {				// 03/11/23
		UINT32 ad;
		BYTE val;
		UPD9002_WORKCLOCK(3);
		ad = CALC_EA(op);
		GET_PCBYTE(val)
		upd9002_memorywrite(ad, val);
	}
}

UPD9002FN _mov_ea16_data16(void) {				// C7:	mov		EA16, DATA16

	UINT	op;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		GET_PCWORD(*(REG16_B20(op)))
	}
	else {				// 03/11/23
		UINT32	ad;
		UINT16	val;
		UPD9002_WORKCLOCK(3);
		ad = CALC_EA(op);
		GET_PCWORD(val)
		upd9002_memorywrite_w(ad, val);
	}
}

UPD9002FN _enter(void) {						// C8:	enter	DATA16, DATA8

	UINT16	dimsize;
	BYTE	level;

	GET_PCWORD(dimsize)
	GET_PCBYTE(level)
	REGPUSH0(UPD9002_BP)
	level &= 0x1f;
	if (!level) {								// enter level=0
		UPD9002_WORKCLOCK(11);
		UPD9002_BP = UPD9002_SP;
		UPD9002_SP -= dimsize;
	}
	else {
		level--;
		if (!level) {							// enter level=1
			UINT16 tmp;
			UPD9002_WORKCLOCK(15);
			tmp = UPD9002_SP;
			REGPUSH0(tmp)
			UPD9002_BP = tmp;
			UPD9002_SP -= dimsize;
		}
		else {									// enter level=2-31
			UINT16 bp;
			UPD9002_WORKCLOCK(12 + level*4);
			bp = UPD9002_BP;
			UPD9002_BP = UPD9002_SP;
			while(level--) {
#if 1											// なにやってんだヲレ
				REG16 val;
				bp -= 2;
				UPD9002_SP -= 2;
				val = upd9002_memoryread_seg_w(SS_BASE, bp);
				upd9002_memorywrite_seg_w(SS_BASE, UPD9002_SP, val);
#else
				UINT16 val = upd9002_memoryread_seg_w(SS_BASE, bp);
				upd9002_memorywrite_seg_w(SS_BASE, UPD9002_SP, val);
				bp -= 2;
				UPD9002_SP -= 2;
#endif
			}
			REGPUSH0(UPD9002_BP)
			UPD9002_SP -= dimsize;
		}
	}
}

UPD9002FN fleave(void) {						// C9:	leave

	UPD9002_WORKCLOCK(5);
	UPD9002_SP = UPD9002_BP;
	REGPOP0(UPD9002_BP)
}

UPD9002FN _ret_far_data16(void) {				// CA:	ret far	DATA16

	UINT16	ad;

	UPD9002_WORKCLOCK(15);
	GET_PCWORD(ad)
	REGPOP0(UPD9002_IP)
	REGPOP0(UPD9002_CS)
	UPD9002_SP += ad;
	CS_BASE = SEGSELECT(UPD9002_CS);
}

UPD9002FN _ret_far(void) {						// CB:	ret far

	UPD9002_WORKCLOCK(15);
	REGPOP0(UPD9002_IP)
	REGPOP0(UPD9002_CS)
	CS_BASE = SEGSELECT(UPD9002_CS);
}

UPD9002FN _int_03(void) {						// CC:	int		3

	UPD9002_WORKCLOCK(3);
	INT_NUM(3, UPD9002_IP);
}

UPD9002FN _int_data8(void) {					// CD:	int		DATA8

	UINT	vect;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(vect)
#if 0 // defined(TRACE)
	if ((vect >= 0xa0) && (vect < 0xb0)) {
extern void lio_look(UINT vect);
		lio_look(vect);
	}
#endif
	INT_NUM(vect, UPD9002_IP);
}

UPD9002FN _into(void) {						// CE:	into

	UPD9002_WORKCLOCK(4);
	if (UPD9002_OV) {
		INT_NUM(4, UPD9002_IP);
	}
}

UPD9002FN _iret(void) {						// CF:	iret

	UINT	flag;

	REGPOP0(UPD9002_IP)
	REGPOP0(UPD9002_CS)
	REGPOP0(flag)
	UPD9002_OV = flag & O_FLAG;
	UPD9002_FLAG = flag & (0xfff ^ O_FLAG);
	UPD9002_TRAP = ((flag & 0x300) == 0x300);
	CS_BASE = UPD9002_CS << 4;
//	CS_BASE = SEGSELECT(UPD9002_CS);
	UPD9002_WORKCLOCK(31);
#if defined(INTR_FAST)
	if ((UPD9002_TRAP) || ((flag & I_FLAG) && (PICEXISTINTR))) {
		UPD9002_IRQCHECKTERM
	}
#else
	UPD9002_IRQCHECKTERM
#endif
}

UPD9002FN _shift_ea8_1(void) {				// D0:	shift EA8, 1

	BYTE	*out;
	UINT	op;
	UINT32	madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			sft_e8_table[(op >> 3) & 7](madr);
			return;
		}
		out = mem + madr;
	}
	sft_r8_table[(op >> 3) & 7](out);
}

UPD9002FN _shift_ea16_1(void) {			// D1:	shift EA16, 1

	UINT16	*out;
	UINT	op;
	UINT32	madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(7);
		{
			UINT32 seg;
			UINT off;

			off = GET_EA(op, &seg);
			madr = seg + off;
			if ((LOW16(off) == 0xffff) || INHIBIT_WORDP(madr)) {
				UINT16 tmp;

				tmp = upd9002_memoryread_seg_w(seg, off);
				sft_r16_table[(op >> 3) & 7](&tmp);
				upd9002_memorywrite_seg_w(seg, off, tmp);
				return;
			}
		}
		if (INHIBIT_WORDP(madr)) {
			sft_e16_table[(op >> 3) & 7](madr);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	sft_r16_table[(op >> 3) & 7](out);
}

UPD9002FN _shift_ea8_cl(void) {			// D2:	shift EA8, cl

	BYTE	*out;
	UINT	op;
	UINT32	madr;
	REG8	cl;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			cl = UPD9002_CL;
			UPD9002_WORKCLOCK(cl);
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	cl = UPD9002_CL;
	UPD9002_WORKCLOCK(cl);
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN _shift_ea16_cl(void) {			// D3:	shift EA16, cl

	UINT16	*out;
	UINT	op;
	UINT32	madr;
	REG8	cl;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			cl = UPD9002_CL;
			UPD9002_WORKCLOCK(cl);
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	cl = UPD9002_CL;
	UPD9002_WORKCLOCK(cl);
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN _aam(void) {							// D4:	AAM

	BYTE	al;
	BYTE	div;

	UPD9002_WORKCLOCK(16);
	GET_PCBYTE(div);
	if (div) {
		al = UPD9002_AL;
		UPD9002_AH = al / div;
		UPD9002_AL = al % div;
		UPD9002_FLAGL &= ~(S_FLAG | Z_FLAG | P_FLAG);
		UPD9002_FLAGL |= WORDSZPF(UPD9002_AX);
	}
	else {
		INT_NUM(0, UPD9002_IP - 2);				// 80286
//		INT_NUM(0, UPD9002_IP);					// V30
	}
}

UPD9002FN _aad(void) {							// D5:	AAD

	BYTE	mul;

	UPD9002_WORKCLOCK(14);
	GET_PCBYTE(mul);
	UPD9002_AL += (BYTE)(UPD9002_AH * mul);
	UPD9002_AH = 0;
	UPD9002_FLAGL &= ~(S_FLAG | Z_FLAG | P_FLAG);
	UPD9002_FLAGL |= BYTESZPF(UPD9002_AL);
}

UPD9002FN _setalc(void) {						// D6:	setalc (80286)

	UPD9002_AL = ((UPD9002_FLAGL & C_FLAG)?0xff:0);
}

UPD9002FN _xlat(void) {						// D7:	xlat

	UPD9002_WORKCLOCK(5);
	UPD9002_AL = upd9002_memoryread(LOW16(UPD9002_AL + UPD9002_BX) + DS_FIX);
}

UPD9002FN _esc(void) {							// D8:	esc

	UINT	op;

	UPD9002_WORKCLOCK(2);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		CALC_LEA(op);
	}
}

UPD9002FN _loopnz(void) {						// E0:	loopnz

	UPD9002_CX--;
	if ((!UPD9002_CX) || (UPD9002_FLAGL & Z_FLAG)) JMPNOP(4) else JMPSHORT(8)
}

UPD9002FN _loopz(void) {						// E1:	loopz

	UPD9002_CX--;
	if ((!UPD9002_CX) || (!(UPD9002_FLAGL & Z_FLAG))) JMPNOP(4) else JMPSHORT(8)
}

UPD9002FN _loop(void) {						// E2:	loop

	UPD9002_CX--;
	if (!UPD9002_CX) JMPNOP(4) else JMPSHORT(8)
}

UPD9002FN _jcxz(void) {						// E3:	jcxz

	if (UPD9002_CX) JMPNOP(4) else JMPSHORT(8)
}

UPD9002FN _in_al_data8(void) {					// E4:	in		al, DATA8

	UINT	port;

	UPD9002_WORKCLOCK(5);
	GET_PCBYTE(port)
	UPD9002_INPADRS = CS_BASE + UPD9002_IP;
	UPD9002_AL = iocore_inp8(port);
	UPD9002_INPADRS = 0;
}

UPD9002FN _in_ax_data8(void) {					// E5:	in		ax, DATA8

	UINT	port;

	UPD9002_WORKCLOCK(5);
	GET_PCBYTE(port)
	UPD9002_AX = iocore_inp16(port);
}

UPD9002FN _out_data8_al(void) {				// E6:	out		DATA8, al

	UINT	port;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(port);
	iocore_out8(port, UPD9002_AL);
}

UPD9002FN _out_data8_ax(void) {				// E7:	out		DATA8, ax

	UINT	port;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(port);
	iocore_out16(port, UPD9002_AX);
}

UPD9002FN _call_near(void) {					// E8:	call near

	UINT16	ad;

	UPD9002_WORKCLOCK(7);
	GET_PCWORD(ad)
	REGPUSH0(UPD9002_IP)
	UPD9002_IP += ad;
}

UPD9002FN _jmp_near(void) {					// E9:	jmp near

	UINT16	ad;

	UPD9002_WORKCLOCK(7);
	GET_PCWORD(ad)
	UPD9002_IP += ad;
}

UPD9002FN _jmp_far(void) {						// EA:	jmp far

	UINT16	ad;

	UPD9002_WORKCLOCK(11);
	GET_PCWORD(ad);
	GET_PCWORD(UPD9002_CS);
	UPD9002_IP = ad;
	CS_BASE = SEGSELECT(UPD9002_CS);
}

UPD9002FN _jmp_short(void) {					// EB:	jmp short

	UINT16	ad;

	UPD9002_WORKCLOCK(7);
	GET_PCBYTES(ad)
	UPD9002_IP += ad;
}

UPD9002FN _in_al_dx(void) {					// EC:	in		al, dx

	UPD9002_WORKCLOCK(5);
	UPD9002_AL = iocore_inp8(UPD9002_DX);
}

UPD9002FN _in_ax_dx(void) {					// ED:	in		ax, dx

	UPD9002_WORKCLOCK(5);
	UPD9002_AX = iocore_inp16(UPD9002_DX);
}

UPD9002FN _out_dx_al(void) {					// EE:	out		dx, al

	UPD9002_WORKCLOCK(3);
	iocore_out8(UPD9002_DX, UPD9002_AL);
}

UPD9002FN _out_dx_ax(void) {					// EF:	out		dx, ax

	UPD9002_WORKCLOCK(3);
	iocore_out16(UPD9002_DX, UPD9002_AX);
}

UPD9002FN _lock(void) {						// F0:	lock
											// F1:	lock
	UPD9002_WORKCLOCK(2);
}

UPD9002FN _repne(void) {						// F2:	repne

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repe(void) {						// F3:	repe

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _hlt(void) {							// F4:	hlt

	UPD9002_REMCLOCK = -1;
	UPD9002_IP--;
}

UPD9002FN _cmc(void) {							// F5:	cmc

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAGL ^= C_FLAG;
}

UPD9002FN _ope0xf6(void) {						// F6:	

	UINT	op;

	GET_PCBYTE(op);
	c_ope0xf6_table[(op >> 3) & 7](op);
}

UPD9002FN _ope0xf7(void) {						// F7:	

	UINT	op;

	GET_PCBYTE(op);
	c_ope0xf7_table[(op >> 3) & 7](op);
}

UPD9002FN _clc(void) {							// F8:	clc

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAGL &= ~C_FLAG;
}

UPD9002FN _stc(void) {							// F9:	stc

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAGL |= C_FLAG;
}

UPD9002FN _cli(void) {							// FA:	cli

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAG &= ~I_FLAG;
	UPD9002_TRAP = 0;
}

UPD9002FN _sti(void) {							// FB:	sti

	UPD9002_WORKCLOCK(2);
#if defined(INTR_FAST)
	if (UPD9002_FLAG & I_FLAG) {
		NEXT_OPCODE;
		return;									// 更新の意味なし
	}
#endif
	UPD9002_FLAG |= I_FLAG;
	UPD9002_TRAP = (UPD9002_FLAG & T_FLAG) >> 8;
#if defined(INTR_FAST)
	if ((UPD9002_TRAP) || (PICEXISTINTR)) {
		REMAIN_ADJUST(1)
	}
	else {
		NEXT_OPCODE;
	}
#else
	REMAIN_ADJUST(1)
#endif
}

UPD9002FN _cld(void) {							// FC:	cld

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAG &= ~D_FLAG;
}

UPD9002FN _std(void) {							// FD:	std

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAG |= D_FLAG;
}

UPD9002FN _ope0xfe(void) {						// FE:	

	UINT	op;

	GET_PCBYTE(op);
	c_ope0xfe_table[(op >> 3) & 1](op);
}

UPD9002FN _ope0xff(void) {						// FF:	

	UINT	op;

	GET_PCBYTE(op);
	c_ope0xff_table[(op >> 3) & 7](op);
}

// -------------------------------------------------------------------------

const UPD9002OP upd9002op[] = {
			_add_ea_r8,						// 00:	add		EA, REG8
			_add_ea_r16,					// 01:	add		EA, REG16
			_add_r8_ea,						// 02:	add		REG8, EA
			_add_r16_ea,					// 03:	add		REG16, EA
			_add_al_data8,					// 04:	add		al, DATA8
			_add_ax_data16,					// 05:	add		ax, DATA16
			_push_es,						// 06:	push	es
			_pop_es,						// 07:	pop		es
			_or_ea_r8,						// 08:	or		EA, REGF8
			_or_ea_r16,						// 09:	or		EA, REG16
			_or_r8_ea,						// 0A:	or		REG8, EA
			_or_r16_ea,						// 0B:	or		REG16, EA
			_or_al_data8,					// 0C:	or		al, DATA8
			_or_ax_data16,					// 0D:	or		ax, DATA16
			_push_cs,						// 0E:	push	cs
			_reserved,						// 0F:	reserved placeholder

			_adc_ea_r8,						// 10:	adc		EA, REG8
			_adc_ea_r16,					// 11:	adc		EA, REG16
			_adc_r8_ea,						// 12:	adc		REG8, EA
			_adc_r16_ea,					// 13:	adc		REG16, EA
			_adc_al_data8,					// 14:	adc		al, DATA8
			_adc_ax_data16,					// 15:	adc		ax, DATA16
			_push_ss,						// 16:	push	ss
			_pop_ss,						// 17:	pop		ss
			_sbb_ea_r8,						// 18:	sbb		EA, REG8
			_sbb_ea_r16,					// 19:	sbb		EA, REG16
			_sbb_r8_ea,						// 1A:	sbb		REG8, EA
			_sbb_r16_ea,					// 1B:	sbb		REG16, EA
			_sbb_al_data8,					// 1C:	sbb		al, DATA8
			_sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			_push_ds,						// 1E:	push	ds
			_pop_ds,						// 1F:	pop		ds

			_and_ea_r8,						// 20:	and		EA, REG8
			_and_ea_r16,					// 21:	and		EA, REG16
			_and_r8_ea,						// 22:	and		REG8, EA
			_and_r16_ea,					// 23:	and		REG16, EA
			_and_al_data8,					// 24:	and		al, DATA8
			_and_ax_data16,					// 25:	and		ax, DATA16
			_segprefix_es,					// 26:	es:
			_daa,							// 27:	daa
			_sub_ea_r8,						// 28:	sub		EA, REG8
			_sub_ea_r16,					// 29:	sub		EA, REG16
			_sub_r8_ea,						// 2A:	sub		REG8, EA
			_sub_r16_ea,					// 2B:	sub		REG16, EA
			_sub_al_data8,					// 2C:	sub		al, DATA8
			_sub_ax_data16,					// 2D:	sub		ax, DATA16
			_segprefix_cs,					// 2E:	cs:
			_das,							// 2F:	das

			_xor_ea_r8,						// 30:	xor		EA, REG8
			_xor_ea_r16,					// 31:	xor		EA, REG16
			_xor_r8_ea,						// 32:	xor		REG8, EA
			_xor_r16_ea,					// 33:	xor		REG16, EA
			_xor_al_data8,					// 34:	xor		al, DATA8
			_xor_ax_data16,					// 35:	xor		ax, DATA16
			_segprefix_ss,					// 36:	ss:
			_aaa,							// 37:	aaa
			_cmp_ea_r8,						// 38:	cmp		EA, REG8
			_cmp_ea_r16,					// 39:	cmp		EA, REG16
			_cmp_r8_ea,						// 3A:	cmp		REG8, EA
			_cmp_r16_ea,					// 3B:	cmp		REG16, EA
			_cmp_al_data8,					// 3C:	cmp		al, DATA8
			_cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			_segprefix_ds,					// 3E:	ds:
			_aas,							// 3F:	aas

			_inc_ax,						// 40:	inc		ax
			_inc_cx,						// 41:	inc		cx
			_inc_dx,						// 42:	inc		dx
			_inc_bx,						// 43:	inc		bx
			_inc_sp,						// 44:	inc		sp
			_inc_bp,						// 45:	inc		bp
			_inc_si,						// 46:	inc		si
			_inc_di,						// 47:	inc		di
			_dec_ax,						// 48:	dec		ax
			_dec_cx,						// 49:	dec		cx
			_dec_dx,						// 4A:	dec		dx
			_dec_bx,						// 4B:	dec		bx
			_dec_sp,						// 4C:	dec		sp
			_dec_bp,						// 4D:	dec		bp
			_dec_si,						// 4E:	dec		si
			_dec_di,						// 4F:	dec		di

			_push_ax,						// 50:	push	ax
			_push_cx,						// 51:	push	cx
			_push_dx,						// 52:	push	dx
			_push_bx,						// 53:	push	bx
			_push_sp,						// 54:	push	sp
			_push_bp,						// 55:	push	bp
			_push_si,						// 56:	push	si
			_push_di,						// 57:	push	di
			_pop_ax,						// 58:	pop		ax
			_pop_cx,						// 59:	pop		cx
			_pop_dx,						// 5A:	pop		dx
			_pop_bx,						// 5B:	pop		bx
			_pop_sp,						// 5C:	pop		sp
			_pop_bp,						// 5D:	pop		bp
			_pop_si,						// 5E:	pop		si
			_pop_di,						// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_reserved,						// 63:	reserved placeholder
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			_push_data16,					// 68:	push	DATA16
			_imul_reg_ea_data16,			// 69:	imul	REG, EA, DATA16
			_push_data8,					// 6A:	push	DATA8
			_imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			_insb,							// 6C:	insb
			_insw,							// 6D:	insw
			_outsb,							// 6E:	outsb
			_outsw,							// 6F:	outsw

			_jo_short,						// 70:	jo short
			_jno_short,						// 71:	jno short
			_jc_short,						// 72:	jnae/jb/jc short
			_jnc_short,						// 73:	jae/jnb/jnc short
			_jz_short,						// 74:	je/jz short
			_jnz_short,						// 75:	jne/jnz short
			_jna_short,						// 76:	jna/jbe short
			_ja_short,						// 77:	ja/jnbe short
			_js_short,						// 78:	js short
			_jns_short,						// 79:	jns short
			_jp_short,						// 7A:	jp/jpe short
			_jnp_short,						// 7B:	jnp/jpo short
			_jl_short,						// 7C:	jl/jnge short
			_jnl_short,						// 7D:	jnl/jge short
			_jle_short,						// 7E:	jle/jng short
			_jnle_short,					// 7F:	jg/jnle short

			_calc_ea8_i8,					// 80:	op		EA8, DATA8
			_calc_ea16_i16,					// 81:	op		EA16, DATA16
			_calc_ea8_i8,					// 82:	op		EA8, DATA8
			_calc_ea16_i8,					// 83:	op		EA16, DATA8
			_test_ea_r8,					// 84:	test	EA, REG8
			_test_ea_r16,					// 85:	test	EA, REG16
			_xchg_ea_r8,					// 86:	xchg	EA, REG8
			_xchg_ea_r16,					// 87:	xchg	EA, REG16
			_mov_ea_r8,						// 88:	mov		EA, REG8
			_mov_ea_r16,					// 89:	mov		EA, REG16
			_mov_r8_ea,						// 8A:	mov		REG8, EA
			_mov_r16_ea,					// 8B:	mov		REG16, EA
			_mov_ea_seg,					// 8C:	mov		EA, segreg
			_lea_r16_ea,					// 8D:	lea		REG16, EA
			_reserved,						// 8E:	reserved placeholder
			_pop_ea,						// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			_xchg_ax_cx,					// 91:	xchg	ax, cx
			_xchg_ax_dx,					// 92:	xchg	ax, dx
			_xchg_ax_bx,					// 93:	xchg	ax, bx
			_xchg_ax_sp,					// 94:	xchg	ax, sp
			_xchg_ax_bp,					// 95:	xchg	ax, bp
			_xchg_ax_si,					// 96:	xchg	ax, si
			_xchg_ax_di,					// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			_call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			_mov_al_m8,						// A0:	mov		al, m8
			_mov_ax_m16,					// A1:	mov		ax, m16
			_mov_m8_al,						// A2:	mov		m8, al
			_mov_m16_ax,					// A3:	mov		m16, ax
			_movsb,							// A4:	movsb
			_movsw,							// A5:	movsw
			_cmpsb,							// A6:	cmpsb
			_cmpsw,							// A7:	cmpsw
			_test_al_data8,					// A8:	test	al, DATA8
			_test_ax_data16,				// A9:	test	ax, DATA16
			_stosb,							// AA:	stosw
			_stosw,							// AB:	stosw
			_lodsb,							// AC:	lodsb
			_lodsw,							// AD:	lodsw
			_scasb,							// AE:	scasb
			_scasw,							// AF:	scasw

			_mov_al_imm,					// B0:	mov		al, imm8
			_mov_cl_imm,					// B1:	mov		cl, imm8
			_mov_dl_imm,					// B2:	mov		dl, imm8
			_mov_bl_imm,					// B3:	mov		bl, imm8
			_mov_ah_imm,					// B4:	mov		ah, imm8
			_mov_ch_imm,					// B5:	mov		ch, imm8
			_mov_dh_imm,					// B6:	mov		dh, imm8
			_mov_bh_imm,					// B7:	mov		bh, imm8
			_mov_ax_imm,					// B8:	mov		ax, imm16
			_mov_cx_imm,					// B9:	mov		cx, imm16
			_mov_dx_imm,					// BA:	mov		dx, imm16
			_mov_bx_imm,					// BB:	mov		bx, imm16
			_mov_sp_imm,					// BC:	mov		sp, imm16
			_mov_bp_imm,					// BD:	mov		bp, imm16
			_mov_si_imm,					// BE:	mov		si, imm16
			_mov_di_imm,					// BF:	mov		di, imm16

			_shift_ea8_data8,				// C0:	shift	EA8, DATA8
			_shift_ea16_data8,				// C1:	shift	EA16, DATA8
			_ret_near_data16,				// C2:	ret near DATA16
			_ret_near,						// C3:	ret near
			_les_r16_ea,					// C4:	les		REG16, EA
			_lds_r16_ea,					// C5:	lds		REG16, EA
			_mov_ea8_data8,					// C6:	mov		EA8, DATA8
			_mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			fleave,							// C9:	leave
			_ret_far_data16,				// CA:	ret far	DATA16
			_ret_far,						// CB:	ret far
			_int_03,						// CC:	int		3
			_int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			_shift_ea8_1,					// D0:	shift EA8, 1
			_shift_ea16_1,					// D1:	shift EA16, 1
			_shift_ea8_cl,					// D2:	shift EA8, cl
			_shift_ea16_cl,					// D3:	shift EA16, cl
			_aam,							// D4:	AAM
			_aad,							// D5:	AAD
			_setalc,						// D6:	setalc (80286)
			_xlat,							// D7:	xlat
			_esc,							// D8:	esc
			_esc,							// D9:	esc
			_esc,							// DA:	esc
			_esc,							// DB:	esc
			_esc,							// DC:	esc
			_esc,							// DD:	esc
			_esc,							// DE:	esc
			_esc,							// DF:	esc

			_loopnz,						// E0:	loopnz
			_loopz,							// E1:	loopz
			_loop,							// E2:	loop
			_jcxz,							// E3:	jcxz
			_in_al_data8,					// E4:	in		al, DATA8
			_in_ax_data8,					// E5:	in		ax, DATA8
			_out_data8_al,					// E6:	out		DATA8, al
			_out_data8_ax,					// E7:	out		DATA8, ax
			_call_near,						// E8:	call near
			_jmp_near,						// E9:	jmp near
			_jmp_far,						// EA:	jmp far
			_jmp_short,						// EB:	jmp short
			_in_al_dx,						// EC:	in		al, dx
			_in_ax_dx,						// ED:	in		ax, dx
			_out_dx_al,						// EE:	out		dx, al
			_out_dx_ax,						// EF:	out		dx, ax

			_lock,							// F0:	lock
			_lock,							// F1:	lock
			_repne,							// F2:	repne
			_repe,							// F3:	repe
			_hlt,							// F4:	hlt
			_cmc,							// F5:	cmc
			_ope0xf6,						// F6:	
			_ope0xf7,						// F7:	
			_clc,							// F8:	clc
			_stc,							// F9:	stc
			_cli,							// FA:	cli
			_sti,							// FB:	sti
			_cld,							// FC:	cld
			_std,							// FD:	std
			_ope0xfe,						// FE:	
			_ope0xff,						// FF:	
};



// ----------------------------------------------------------------- repe

UPD9002FN _repe_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repe_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repe_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repe_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

const UPD9002OP upd9002op_repe[] = {
			_add_ea_r8,						// 00:	add		EA, REG8
			_add_ea_r16,					// 01:	add		EA, REG16
			_add_r8_ea,						// 02:	add		REG8, EA
			_add_r16_ea,					// 03:	add		REG16, EA
			_add_al_data8,					// 04:	add		al, DATA8
			_add_ax_data16,					// 05:	add		ax, DATA16
			_push_es,						// 06:	push	es
			_pop_es,						// 07:	pop		es
			_or_ea_r8,						// 08:	or		EA, REGF8
			_or_ea_r16,						// 09:	or		EA, REG16
			_or_r8_ea,						// 0A:	or		REG8, EA
			_or_r16_ea,						// 0B:	or		REG16, EA
			_or_al_data8,					// 0C:	or		al, DATA8
			_or_ax_data16,					// 0D:	or		ax, DATA16
			_push_cs,						// 0E:	push	cs
			_reserved,						// 0F:	reserved placeholder

			_adc_ea_r8,						// 10:	adc		EA, REG8
			_adc_ea_r16,					// 11:	adc		EA, REG16
			_adc_r8_ea,						// 12:	adc		REG8, EA
			_adc_r16_ea,					// 13:	adc		REG16, EA
			_adc_al_data8,					// 14:	adc		al, DATA8
			_adc_ax_data16,					// 15:	adc		ax, DATA16
			_push_ss,						// 16:	push	ss
			_pop_ss,						// 17:	pop		ss
			_sbb_ea_r8,						// 18:	sbb		EA, REG8
			_sbb_ea_r16,					// 19:	sbb		EA, REG16
			_sbb_r8_ea,						// 1A:	sbb		REG8, EA
			_sbb_r16_ea,					// 1B:	sbb		REG16, EA
			_sbb_al_data8,					// 1C:	sbb		al, DATA8
			_sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			_push_ds,						// 1E:	push	ds
			_pop_ds,						// 1F:	pop		ds

			_and_ea_r8,						// 20:	and		EA, REG8
			_and_ea_r16,					// 21:	and		EA, REG16
			_and_r8_ea,						// 22:	and		REG8, EA
			_and_r16_ea,					// 23:	and		REG16, EA
			_and_al_data8,					// 24:	and		al, DATA8
			_and_ax_data16,					// 25:	and		ax, DATA16
			_repe_segprefix_es,				// 26:	repe es:
			_daa,							// 27:	daa
			_sub_ea_r8,						// 28:	sub		EA, REG8
			_sub_ea_r16,					// 29:	sub		EA, REG16
			_sub_r8_ea,						// 2A:	sub		REG8, EA
			_sub_r16_ea,					// 2B:	sub		REG16, EA
			_sub_al_data8,					// 2C:	sub		al, DATA8
			_sub_ax_data16,					// 2D:	sub		ax, DATA16
			_repe_segprefix_cs,				// 2E:	repe cs:
			_das,							// 2F:	das

			_xor_ea_r8,						// 30:	xor		EA, REG8
			_xor_ea_r16,					// 31:	xor		EA, REG16
			_xor_r8_ea,						// 32:	xor		REG8, EA
			_xor_r16_ea,					// 33:	xor		REG16, EA
			_xor_al_data8,					// 34:	xor		al, DATA8
			_xor_ax_data16,					// 35:	xor		ax, DATA16
			_repe_segprefix_ss,				// 36:	repe ss:
			_aaa,							// 37:	aaa
			_cmp_ea_r8,						// 38:	cmp		EA, REG8
			_cmp_ea_r16,					// 39:	cmp		EA, REG16
			_cmp_r8_ea,						// 3A:	cmp		REG8, EA
			_cmp_r16_ea,					// 3B:	cmp		REG16, EA
			_cmp_al_data8,					// 3C:	cmp		al, DATA8
			_cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			_repe_segprefix_ds,				// 3E:	repe ds:
			_aas,							// 3F:	aas

			_inc_ax,						// 40:	inc		ax
			_inc_cx,						// 41:	inc		cx
			_inc_dx,						// 42:	inc		dx
			_inc_bx,						// 43:	inc		bx
			_inc_sp,						// 44:	inc		sp
			_inc_bp,						// 45:	inc		bp
			_inc_si,						// 46:	inc		si
			_inc_di,						// 47:	inc		di
			_dec_ax,						// 48:	dec		ax
			_dec_cx,						// 49:	dec		cx
			_dec_dx,						// 4A:	dec		dx
			_dec_bx,						// 4B:	dec		bx
			_dec_sp,						// 4C:	dec		sp
			_dec_bp,						// 4D:	dec		bp
			_dec_si,						// 4E:	dec		si
			_dec_di,						// 4F:	dec		di

			_push_ax,						// 50:	push	ax
			_push_cx,						// 51:	push	cx
			_push_dx,						// 52:	push	dx
			_push_bx,						// 53:	push	bx
			_push_sp,						// 54:	push	sp
			_push_bp,						// 55:	push	bp
			_push_si,						// 56:	push	si
			_push_di,						// 57:	push	di
			_pop_ax,						// 58:	pop		ax
			_pop_cx,						// 59:	pop		cx
			_pop_dx,						// 5A:	pop		dx
			_pop_bx,						// 5B:	pop		bx
			_pop_sp,						// 5C:	pop		sp
			_pop_bp,						// 5D:	pop		bp
			_pop_si,						// 5E:	pop		si
			_pop_di,						// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_reserved,						// 63:	reserved placeholder
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			_push_data16,					// 68:	push	DATA16
			_imul_reg_ea_data16,			// 69:	imul	REG, EA, DATA16
			_push_data8,					// 6A:	push	DATA8
			_imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			upd9002_rep_insb,					// 6C:	rep insb
			upd9002_rep_insw,					// 6D:	rep insw
			upd9002_rep_outsb,				// 6E:	rep outsb
			upd9002_rep_outsb,				// 6F:	rep outsw

			_jo_short,						// 70:	jo short
			_jno_short,						// 71:	jno short
			_jc_short,						// 72:	jnae/jb/jc short
			_jnc_short,						// 73:	jae/jnb/jnc short
			_jz_short,						// 74:	je/jz short
			_jnz_short,						// 75:	jne/jnz short
			_jna_short,						// 76:	jna/jbe short
			_ja_short,						// 77:	ja/jnbe short
			_js_short,						// 78:	js short
			_jns_short,						// 79:	jns short
			_jp_short,						// 7A:	jp/jpe short
			_jnp_short,						// 7B:	jnp/jpo short
			_jl_short,						// 7C:	jl/jnge short
			_jnl_short,						// 7D:	jnl/jge short
			_jle_short,						// 7E:	jle/jng short
			_jnle_short,					// 7F:	jg/jnle short

			_calc_ea8_i8,					// 80:	op		EA8, DATA8
			_calc_ea16_i16,					// 81:	op		EA16, DATA16
			_calc_ea8_i8,					// 82:	op		EA8, DATA8
			_calc_ea16_i8,					// 83:	op		EA16, DATA8
			_test_ea_r8,					// 84:	test	EA, REG8
			_test_ea_r16,					// 85:	test	EA, REG16
			_xchg_ea_r8,					// 86:	xchg	EA, REG8
			_xchg_ea_r16,					// 87:	xchg	EA, REG16
			_mov_ea_r8,						// 88:	mov		EA, REG8
			_mov_ea_r16,					// 89:	mov		EA, REG16
			_mov_r8_ea,						// 8A:	mov		REG8, EA
			_mov_r16_ea,					// 8B:	add		REG16, EA
			_mov_ea_seg,					// 8C:	mov		EA, segreg
			_lea_r16_ea,					// 8D:	lea		REG16, EA
			_reserved,						// 8E:	reserved placeholder
			_pop_ea,						// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			_xchg_ax_cx,					// 91:	xchg	ax, cx
			_xchg_ax_dx,					// 92:	xchg	ax, dx
			_xchg_ax_bx,					// 93:	xchg	ax, bx
			_xchg_ax_sp,					// 94:	xchg	ax, sp
			_xchg_ax_bp,					// 95:	xchg	ax, bp
			_xchg_ax_si,					// 96:	xchg	ax, si
			_xchg_ax_di,					// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			_call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			_mov_al_m8,						// A0:	mov		al, m8
			_mov_ax_m16,					// A1:	mov		ax, m16
			_mov_m8_al,						// A2:	mov		m8, al
			_mov_m16_ax,					// A3:	mov		m16, ax
			upd9002_rep_movsb,				// A4:	rep movsb
			upd9002_rep_movsw,				// A5:	rep movsw
			upd9002_repe_cmpsb,				// A6:	repe cmpsb
			upd9002_repe_cmpsw,				// A7:	repe cmpsw
			_test_al_data8,					// A8:	test	al, DATA8
			_test_ax_data16,				// A9:	test	ax, DATA16
			upd9002_rep_stosb,				// AA:	rep stosb
			upd9002_rep_stosw,				// AB:	rep stosw
			upd9002_rep_lodsb,				// AC:	rep lodsb
			upd9002_rep_lodsw,				// AD:	rep lodsw
			upd9002_repe_scasb,				// AE:	repe scasb
			upd9002_repe_scasw,				// AF:	repe scasw

			_mov_al_imm,					// B0:	mov		al, imm8
			_mov_cl_imm,					// B1:	mov		cl, imm8
			_mov_dl_imm,					// B2:	mov		dl, imm8
			_mov_bl_imm,					// B3:	mov		bl, imm8
			_mov_ah_imm,					// B4:	mov		ah, imm8
			_mov_ch_imm,					// B5:	mov		ch, imm8
			_mov_dh_imm,					// B6:	mov		dh, imm8
			_mov_bh_imm,					// B7:	mov		bh, imm8
			_mov_ax_imm,					// B8:	mov		ax, imm16
			_mov_cx_imm,					// B9:	mov		cx, imm16
			_mov_dx_imm,					// BA:	mov		dx, imm16
			_mov_bx_imm,					// BB:	mov		bx, imm16
			_mov_sp_imm,					// BC:	mov		sp, imm16
			_mov_bp_imm,					// BD:	mov		bp, imm16
			_mov_si_imm,					// BE:	mov		si, imm16
			_mov_di_imm,					// BF:	mov		di, imm16

			_shift_ea8_data8,				// C0:	shift	EA8, DATA8
			_shift_ea16_data8,				// C1:	shift	EA16, DATA8
			_ret_near_data16,				// C2:	ret near DATA16
			_ret_near,						// C3:	ret near
			_les_r16_ea,					// C4:	les		REG16, EA
			_lds_r16_ea,					// C5:	lds		REG16, EA
			_mov_ea8_data8,					// C6:	mov		EA8, DATA8
			_mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			fleave,							// C9:	leave
			_ret_far_data16,				// CA:	ret far	DATA16
			_ret_far,						// CB:	ret far
			_int_03,						// CC:	int		3
			_int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			_shift_ea8_1,					// D0:	shift EA8, 1
			_shift_ea16_1,					// D1:	shift EA16, 1
			_shift_ea8_cl,					// D2:	shift EA8, cl
			_shift_ea16_cl,					// D3:	shift EA16, cl
			_aam,							// D4:	AAM
			_aad,							// D5:	AAD
			_setalc,						// D6:	setalc (80286)
			_xlat,							// D7:	xlat
			_esc,							// D8:	esc
			_esc,							// D9:	esc
			_esc,							// DA:	esc
			_esc,							// DB:	esc
			_esc,							// DC:	esc
			_esc,							// DD:	esc
			_esc,							// DE:	esc
			_esc,							// DF:	esc

			_loopnz,						// E0:	loopnz
			_loopz,							// E1:	loopz
			_loop,							// E2:	loop
			_jcxz,							// E3:	jcxz
			_in_al_data8,					// E4:	in		al, DATA8
			_in_ax_data8,					// E5:	in		ax, DATA8
			_out_data8_al,					// E6:	out		DATA8, al
			_out_data8_ax,					// E7:	out		DATA8, ax
			_call_near,						// E8:	call near
			_jmp_near,						// E9:	jmp near
			_jmp_far,						// EA:	jmp far
			_jmp_short,						// EB:	jmp short
			_in_al_dx,						// EC:	in		al, dx
			_in_ax_dx,						// ED:	in		ax, dx
			_out_dx_al,						// EE:	out		dx, al
			_out_dx_ax,						// EF:	out		dx, ax

			_lock,							// F0:	lock
			_lock,							// F1:	lock
			_repne,							// F2:	repne
			_repe,							// F3:	repe
			_hlt,							// F4:	hlt
			_cmc,							// F5:	cmc
			_ope0xf6,						// F6:	
			_ope0xf7,						// F7:	
			_clc,							// F8:	clc
			_stc,							// F9:	stc
			_cli,							// FA:	cli
			_sti,							// FB:	sti
			_cld,							// FC:	cld
			_std,							// FD:	std
			_ope0xfe,						// FE:	
			_ope0xff,						// FF:	
};


// ----------------------------------------------------------------- repne

UPD9002FN _repne_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repne_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repne_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repne_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

const UPD9002OP upd9002op_repne[] = {
			_add_ea_r8,						// 00:	add		EA, REG8
			_add_ea_r16,					// 01:	add		EA, REG16
			_add_r8_ea,						// 02:	add		REG8, EA
			_add_r16_ea,					// 03:	add		REG16, EA
			_add_al_data8,					// 04:	add		al, DATA8
			_add_ax_data16,					// 05:	add		ax, DATA16
			_push_es,						// 06:	push	es
			_pop_es,						// 07:	pop		es
			_or_ea_r8,						// 08:	or		EA, REGF8
			_or_ea_r16,						// 09:	or		EA, REG16
			_or_r8_ea,						// 0A:	or		REG8, EA
			_or_r16_ea,						// 0B:	or		REG16, EA
			_or_al_data8,					// 0C:	or		al, DATA8
			_or_ax_data16,					// 0D:	or		ax, DATA16
			_push_cs,						// 0E:	push	cs
			_reserved,						// 0F:	reserved placeholder

			_adc_ea_r8,						// 10:	adc		EA, REG8
			_adc_ea_r16,					// 11:	adc		EA, REG16
			_adc_r8_ea,						// 12:	adc		REG8, EA
			_adc_r16_ea,					// 13:	adc		REG16, EA
			_adc_al_data8,					// 14:	adc		al, DATA8
			_adc_ax_data16,					// 15:	adc		ax, DATA16
			_push_ss,						// 16:	push	ss
			_pop_ss,						// 17:	pop		ss
			_sbb_ea_r8,						// 18:	sbb		EA, REG8
			_sbb_ea_r16,					// 19:	sbb		EA, REG16
			_sbb_r8_ea,						// 1A:	sbb		REG8, EA
			_sbb_r16_ea,					// 1B:	sbb		REG16, EA
			_sbb_al_data8,					// 1C:	sbb		al, DATA8
			_sbb_ax_data16,					// 1D:	sbb		ax, DATA16
			_push_ds,						// 1E:	push	ds
			_pop_ds,						// 1F:	pop		ds

			_and_ea_r8,						// 20:	and		EA, REG8
			_and_ea_r16,					// 21:	and		EA, REG16
			_and_r8_ea,						// 22:	and		REG8, EA
			_and_r16_ea,					// 23:	and		REG16, EA
			_and_al_data8,					// 24:	and		al, DATA8
			_and_ax_data16,					// 25:	and		ax, DATA16
			_repne_segprefix_es,			// 26:	repne es:
			_daa,							// 27:	daa
			_sub_ea_r8,						// 28:	sub		EA, REG8
			_sub_ea_r16,					// 29:	sub		EA, REG16
			_sub_r8_ea,						// 2A:	sub		REG8, EA
			_sub_r16_ea,					// 2B:	sub		REG16, EA
			_sub_al_data8,					// 2C:	sub		al, DATA8
			_sub_ax_data16,					// 2D:	sub		ax, DATA16
			_repne_segprefix_cs,			// 2E:	repne cs:
			_das,							// 2F:	das

			_xor_ea_r8,						// 30:	xor		EA, REG8
			_xor_ea_r16,					// 31:	xor		EA, REG16
			_xor_r8_ea,						// 32:	xor		REG8, EA
			_xor_r16_ea,					// 33:	xor		REG16, EA
			_xor_al_data8,					// 34:	xor		al, DATA8
			_xor_ax_data16,					// 35:	xor		ax, DATA16
			_repne_segprefix_ss,			// 36:	repne ss:
			_aaa,							// 37:	aaa
			_cmp_ea_r8,						// 38:	cmp		EA, REG8
			_cmp_ea_r16,					// 39:	cmp		EA, REG16
			_cmp_r8_ea,						// 3A:	cmp		REG8, EA
			_cmp_r16_ea,					// 3B:	cmp		REG16, EA
			_cmp_al_data8,					// 3C:	cmp		al, DATA8
			_cmp_ax_data16,					// 3D:	cmp		ax, DATA16
			_repne_segprefix_ds,			// 3E:	repne ds:
			_aas,							// 3F:	aas

			_inc_ax,						// 40:	inc		ax
			_inc_cx,						// 41:	inc		cx
			_inc_dx,						// 42:	inc		dx
			_inc_bx,						// 43:	inc		bx
			_inc_sp,						// 44:	inc		sp
			_inc_bp,						// 45:	inc		bp
			_inc_si,						// 46:	inc		si
			_inc_di,						// 47:	inc		di
			_dec_ax,						// 48:	dec		ax
			_dec_cx,						// 49:	dec		cx
			_dec_dx,						// 4A:	dec		dx
			_dec_bx,						// 4B:	dec		bx
			_dec_sp,						// 4C:	dec		sp
			_dec_bp,						// 4D:	dec		bp
			_dec_si,						// 4E:	dec		si
			_dec_di,						// 4F:	dec		di

			_push_ax,						// 50:	push	ax
			_push_cx,						// 51:	push	cx
			_push_dx,						// 52:	push	dx
			_push_bx,						// 53:	push	bx
			_push_sp,						// 54:	push	sp
			_push_bp,						// 55:	push	bp
			_push_si,						// 56:	push	si
			_push_di,						// 57:	push	di
			_pop_ax,						// 58:	pop		ax
			_pop_cx,						// 59:	pop		cx
			_pop_dx,						// 5A:	pop		dx
			_pop_bx,						// 5B:	pop		bx
			_pop_sp,						// 5C:	pop		sp
			_pop_bp,						// 5D:	pop		bp
			_pop_si,						// 5E:	pop		si
			_pop_di,						// 5F:	pop		di

			_pusha,							// 60:	pusha
			_popa,							// 61:	popa
			_bound,							// 62:	bound
			_reserved,						// 63:	reserved placeholder
			_reserved,						// 64:	reserved
			_reserved,						// 65:	reserved
			_reserved,						// 66:	reserved
			_reserved,						// 67:	reserved
			_push_data16,					// 68:	push	DATA16
			_imul_reg_ea_data16,			// 69:	imul	REG, EA, DATA16
			_push_data8,					// 6A:	push	DATA8
			_imul_reg_ea_data8,				// 6B:	imul	REG, EA, DATA8
			upd9002_rep_insb,					// 6C:	rep insb
			upd9002_rep_insw,					// 6D:	rep insw
			upd9002_rep_outsb,				// 6E:	rep outsb
			upd9002_rep_outsb,				// 6F:	rep outsw

			_jo_short,						// 70:	jo short
			_jno_short,						// 71:	jno short
			_jc_short,						// 72:	jnae/jb/jc short
			_jnc_short,						// 73:	jae/jnb/jnc short
			_jz_short,						// 74:	je/jz short
			_jnz_short,						// 75:	jne/jnz short
			_jna_short,						// 76:	jna/jbe short
			_ja_short,						// 77:	ja/jnbe short
			_js_short,						// 78:	js short
			_jns_short,						// 79:	jns short
			_jp_short,						// 7A:	jp/jpe short
			_jnp_short,						// 7B:	jnp/jpo short
			_jl_short,						// 7C:	jl/jnge short
			_jnl_short,						// 7D:	jnl/jge short
			_jle_short,						// 7E:	jle/jng short
			_jnle_short,					// 7F:	jg/jnle short

			_calc_ea8_i8,					// 80:	op		EA8, DATA8
			_calc_ea16_i16,					// 81:	op		EA16, DATA16
			_calc_ea8_i8,					// 82:	op		EA8, DATA8
			_calc_ea16_i8,					// 83:	op		EA16, DATA8
			_test_ea_r8,					// 84:	test	EA, REG8
			_test_ea_r16,					// 85:	test	EA, REG16
			_xchg_ea_r8,					// 86:	xchg	EA, REG8
			_xchg_ea_r16,					// 87:	xchg	EA, REG16
			_mov_ea_r8,						// 88:	mov		EA, REG8
			_mov_ea_r16,					// 89:	mov		EA, REG16
			_mov_r8_ea,						// 8A:	mov		REG8, EA
			_mov_r16_ea,					// 8B:	add		REG16, EA
			_mov_ea_seg,					// 8C:	mov		EA, segreg
			_lea_r16_ea,					// 8D:	lea		REG16, EA
			_reserved,						// 8E:	reserved placeholder
			_pop_ea,						// 8F:	pop		EA

			_nop,							// 90:	xchg	ax, ax
			_xchg_ax_cx,					// 91:	xchg	ax, cx
			_xchg_ax_dx,					// 92:	xchg	ax, dx
			_xchg_ax_bx,					// 93:	xchg	ax, bx
			_xchg_ax_sp,					// 94:	xchg	ax, sp
			_xchg_ax_bp,					// 95:	xchg	ax, bp
			_xchg_ax_si,					// 96:	xchg	ax, si
			_xchg_ax_di,					// 97:	xchg	ax, di
			_cbw,							// 98:	cbw
			_cwd,							// 99:	cwd
			_call_far,						// 9A:	call far
			_wait,							// 9B:	wait
			_pushf,							// 9C:	pushf
			_popf,							// 9D:	popf
			_sahf,							// 9E:	sahf
			_lahf,							// 9F:	lahf

			_mov_al_m8,						// A0:	mov		al, m8
			_mov_ax_m16,					// A1:	mov		ax, m16
			_mov_m8_al,						// A2:	mov		m8, al
			_mov_m16_ax,					// A3:	mov		m16, ax
			upd9002_rep_movsb,				// A4:	rep movsb
			upd9002_rep_movsw,				// A5:	rep movsw
			upd9002_repne_cmpsb,				// A6:	repne cmpsb
			upd9002_repne_cmpsw,				// A7:	repne cmpsw
			_test_al_data8,					// A8:	test	al, DATA8
			_test_ax_data16,				// A9:	test	ax, DATA16
			upd9002_rep_stosb,				// AA:	rep stosb
			upd9002_rep_stosw,				// AB:	rep stosw
			upd9002_rep_lodsb,				// AC:	rep lodsb
			upd9002_rep_lodsw,				// AD:	rep lodsw
			upd9002_repne_scasb,				// AE:	repne scasb
			upd9002_repne_scasw,				// AF:	repne scasw

			_mov_al_imm,					// B0:	mov		al, imm8
			_mov_cl_imm,					// B1:	mov		cl, imm8
			_mov_dl_imm,					// B2:	mov		dl, imm8
			_mov_bl_imm,					// B3:	mov		bl, imm8
			_mov_ah_imm,					// B4:	mov		ah, imm8
			_mov_ch_imm,					// B5:	mov		ch, imm8
			_mov_dh_imm,					// B6:	mov		dh, imm8
			_mov_bh_imm,					// B7:	mov		bh, imm8
			_mov_ax_imm,					// B8:	mov		ax, imm16
			_mov_cx_imm,					// B9:	mov		cx, imm16
			_mov_dx_imm,					// BA:	mov		dx, imm16
			_mov_bx_imm,					// BB:	mov		bx, imm16
			_mov_sp_imm,					// BC:	mov		sp, imm16
			_mov_bp_imm,					// BD:	mov		bp, imm16
			_mov_si_imm,					// BE:	mov		si, imm16
			_mov_di_imm,					// BF:	mov		di, imm16

			_shift_ea8_data8,				// C0:	shift	EA8, DATA8
			_shift_ea16_data8,				// C1:	shift	EA16, DATA8
			_ret_near_data16,				// C2:	ret near DATA16
			_ret_near,						// C3:	ret near
			_les_r16_ea,					// C4:	les		REG16, EA
			_lds_r16_ea,					// C5:	lds		REG16, EA
			_mov_ea8_data8,					// C6:	mov		EA8, DATA8
			_mov_ea16_data16,				// C7:	mov		EA16, DATA16
			_enter,							// C8:	enter	DATA16, DATA8
			fleave,							// C9:	leave
			_ret_far_data16,				// CA:	ret far	DATA16
			_ret_far,						// CB:	ret far
			_int_03,						// CC:	int		3
			_int_data8,						// CD:	int		DATA8
			_into,							// CE:	into
			_iret,							// CF:	iret

			_shift_ea8_1,					// D0:	shift EA8, 1
			_shift_ea16_1,					// D1:	shift EA16, 1
			_shift_ea8_cl,					// D2:	shift EA8, cl
			_shift_ea16_cl,					// D3:	shift EA16, cl
			_aam,							// D4:	AAM
			_aad,							// D5:	AAD
			_setalc,						// D6:	setalc (80286)
			_xlat,							// D7:	xlat
			_esc,							// D8:	esc
			_esc,							// D9:	esc
			_esc,							// DA:	esc
			_esc,							// DB:	esc
			_esc,							// DC:	esc
			_esc,							// DD:	esc
			_esc,							// DE:	esc
			_esc,							// DF:	esc

			_loopnz,						// E0:	loopnz
			_loopz,							// E1:	loopz
			_loop,							// E2:	loop
			_jcxz,							// E3:	jcxz
			_in_al_data8,					// E4:	in		al, DATA8
			_in_ax_data8,					// E5:	in		ax, DATA8
			_out_data8_al,					// E6:	out		DATA8, al
			_out_data8_ax,					// E7:	out		DATA8, ax
			_call_near,						// E8:	call near
			_jmp_near,						// E9:	jmp near
			_jmp_far,						// EA:	jmp far
			_jmp_short,						// EB:	jmp short
			_in_al_dx,						// EC:	in		al, dx
			_in_ax_dx,						// ED:	in		ax, dx
			_out_dx_al,						// EE:	out		dx, al
			_out_dx_ax,						// EF:	out		dx, ax

			_lock,							// F0:	lock
			_lock,							// F1:	lock
			_repne,							// F2:	repne
			_repe,							// F3:	repe
			_hlt,							// F4:	hlt
			_cmc,							// F5:	cmc
			_ope0xf6,						// F6:	
			_ope0xf7,						// F7:	
			_clc,							// F8:	clc
			_stc,							// F9:	stc
			_cli,							// FA:	cli
			_sti,							// FB:	sti
			_cld,							// FC:	cld
			_std,							// FD:	std
			_ope0xfe,						// FE:	
			_ope0xff,						// FF:	
};
