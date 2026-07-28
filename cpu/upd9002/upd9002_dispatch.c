#include	"compiler.h"
#include	"cpucore.h"
#include	"upd9002_ops.h"
#include	"upd9002_dispatch.h"
#include	"upd9002_diagnostic.h"
#include	"upd9002_trace.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"dmap.h"
#include	"upd9002_ops.mcr"

#if defined(VAEG_UPD9002_M46_TESTING)
#include <stdlib.h>
#endif


// victory30 patch

#define	MAX_PREFIX		8

#define	NEXT_OPCODE												\
		if (UPD9002_REMCLOCK < 1) {								\
			UPD9002_BASECLOCK += (1 - UPD9002_REMCLOCK);				\
			UPD9002_REMCLOCK = 1;									\
		}

#define V30_DMAP()		upd9002_dmap()

typedef struct {
	UINT	opnum;
	UPD9002OP	v30opcode;
} V30PATCH;

static	UPD9002OP		v30op[256];
static	UPD9002OP		v30op_repne[256];
static	UPD9002OP		v30op_repe[256];
static	UPD9002OP		v30op_repnc[256];
static	UPD9002OP		v30op_repc[256];
static	UPD9002OPF6	v30ope0xf6_table[8];
static	UPD9002OPF6	v30ope0xf7_table[8];
static	BOOL		v30_dispatch_initialized;
static	UINT16		v30_repnc_ipbak;
static	UINT16		v30_repc_ipbak;
static	UINT16		v30_step_start_cs;
static	UINT16		v30_step_start_ip;

#if defined(VAEG_UPD9002_M46_TESTING)
static	UPD9002OP		v30op_snapshot[256];
static	UPD9002OP		v30op_repne_snapshot[256];
static	UPD9002OP		v30op_repe_snapshot[256];
static	UPD9002OP		v30op_repnc_snapshot[256];
static	UPD9002OP		v30op_repc_snapshot[256];
static	UPD9002OPF6	v30ope0xf6_snapshot[8];
static	UPD9002OPF6	v30ope0xf7_snapshot[8];
static	UINT		v30_dispatch_construction_count;
static	UINT		v30_dispatch_rejected_count;

static void v30_dispatch_snapshot(void) {

	UINT	i;

	for (i=0; i<256; i++) {
		v30op_snapshot[i] = v30op[i];
		v30op_repne_snapshot[i] = v30op_repne[i];
		v30op_repe_snapshot[i] = v30op_repe[i];
		v30op_repnc_snapshot[i] = v30op_repnc[i];
		v30op_repc_snapshot[i] = v30op_repc[i];
	}
	for (i=0; i<8; i++) {
		v30ope0xf6_snapshot[i] = v30ope0xf6_table[i];
		v30ope0xf7_snapshot[i] = v30ope0xf7_table[i];
	}
}

static BOOL v30_dispatch_equal(const UPD9002OP *live,
								const UPD9002OP *snapshot, UINT count) {

	UINT	i;

	for (i=0; i<count; i++) {
		if (!(live[i] == snapshot[i])) {
			return(FALSE);
		}
	}
	return(TRUE);
}

static BOOL v30_dispatch_f6_equal(const UPD9002OPF6 *live,
								const UPD9002OPF6 *snapshot, UINT count) {

	UINT	i;

	for (i=0; i<count; i++) {
		if (!(live[i] == snapshot[i])) {
			return(FALSE);
		}
	}
	return(TRUE);
}

int upd9002_dispatch_test_verify(void) {

	if ((v30_dispatch_construction_count != 1) ||
		(!v30_dispatch_equal(v30op, v30op_snapshot, 256)) ||
		(!v30_dispatch_equal(v30op_repne, v30op_repne_snapshot, 256)) ||
		(!v30_dispatch_equal(v30op_repe, v30op_repe_snapshot, 256)) ||
		(!v30_dispatch_equal(v30op_repnc, v30op_repnc_snapshot, 256)) ||
		(!v30_dispatch_equal(v30op_repc, v30op_repc_snapshot, 256)) ||
		(!v30_dispatch_f6_equal(v30ope0xf6_table,
							v30ope0xf6_snapshot, 8)) ||
		(!v30_dispatch_f6_equal(v30ope0xf7_table,
							v30ope0xf7_snapshot, 8))) {
		return(FAILURE);
	}
	return(SUCCESS);
}

void upd9002_dispatch_test_require_immutable(void) {

	if (upd9002_dispatch_test_verify() != SUCCESS) {
		abort();
	}
}

UINT upd9002_dispatch_test_construction_count(void) {

	return(v30_dispatch_construction_count);
}

UINT upd9002_dispatch_test_rejected_count(void) {

	return(v30_dispatch_rejected_count);
}
#endif


static const UINT8 shiftbase16[256] =
				{0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15};

static const UINT8 shiftbase09[256] =
				{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6,
				 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4,
				 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2,
				 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9,
				 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7,
				 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5,
				 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3,
				 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1,
				 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8,
				 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6,
				 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4,
				 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2,
				 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9,
				 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7,
				 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5,
				 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3};

static const UINT8 shiftbase17[256] =
				{0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
				16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
				15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,
				14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
				13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
				12,13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,
				11,12,13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8, 9,
				10,11,12,13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7, 8,
				 9,10,11,12,13,14,15,16,17, 1, 2, 3, 4, 5, 6, 7,
				 8, 9,10,11,12,13,14,15,16,17, 1, 2, 3, 4, 5, 6,
				 7, 8, 9,10,11,12,13,14,15,16,17, 1, 2, 3, 4, 5,
				 6, 7, 8, 9,10,11,12,13,14,15,16,17, 1, 2, 3, 4,
				 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17, 1, 2, 3,
				 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17, 1, 2,
				 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17, 1,
				 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17};


UPD9002FN v30_reserved(void) {

	UPD9002_WORKCLOCK(2);
}

UPD9002FN v30segprefix_es(void) {				// 26: es:

	SS_FIX = ES_BASE;
	DS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30segprefix_cs(void) {				// 2e: cs:

	SS_FIX = CS_BASE;
	DS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30segprefix_ss(void) {				// 36: ss:

	SS_FIX = SS_BASE;
	DS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30segprefix_ds(void) {				// 3e: ds:

	SS_FIX = DS_BASE;
	DS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30push_sp(void) REGPUSH(UPD9002_SP, 3)	// 54: push sp

UPD9002FN v30mov_seg_ea(void) {				// 8E:	mov		segrem, EA

	UINT	op;
	UINT	tmp;
	UINT16	ipbak;

	ipbak = UPD9002_IP;
	GET_PCBYTE(op);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		tmp = *(REG16_B20(op));
	}
	else {
		UPD9002_WORKCLOCK(5);
		tmp = upd9002_memoryread_w(CALC_EA(op));
	}
	switch(op & 0x18) {
		case 0x00:			// es
			UPD9002_ES = (UINT16)tmp;
			ES_BASE = tmp << 4;
			break;

		case 0x08:			// cs
			UPD9002_CS = (UINT16)tmp;
			CS_BASE = tmp << 4;
			break;

		case 0x10:			// ss
			UPD9002_SS = (UINT16)tmp;
			SS_BASE = tmp << 4;
			SS_FIX = SS_BASE;
			NEXT_OPCODE
			break;

		case 0x18:			// ds
			UPD9002_DS = (UINT16)tmp;
			DS_BASE = tmp << 4;
			DS_FIX = DS_BASE;
			break;
	}
}

static UINT16 v30_materialize_pushf_image(void) {

	return (UINT16)((UPD9002_FLAG & (UINT16)~O_FLAG) |
						(UPD9002_OV ? O_FLAG : 0));
}

UPD9002FN v30_pushf(void) {					// 9C:	pushf

	UPD9002_WORKCLOCK(3);
	UPD9002_SP -= 2;
	upd9002_memorywrite_seg_w(SS_BASE, UPD9002_SP, v30_materialize_pushf_image());
}

UPD9002FN v30_popf(void) {						// 9D:	popf

	UINT	flag;

	UPD9002_WORKCLOCK(5);
	REGPOP0(flag)
	flag = (flag & 0x0ed5) | 0xf002;
	UPD9002_OV = flag & O_FLAG;
	UPD9002_FLAG = flag & (UINT16)~O_FLAG;
	UPD9002_TRAP = ((flag & 0x300) == 0x300);
	UPD9002_IRQCHECKTERM
}

static UINT8 v30_ea8_read(UINT op, UINT32 *madr);
static void v30_ea8_write(UINT op, UINT32 madr, UINT8 value);
static UINT16 v30_ea16_read(UINT op, UINT32 *madr);
static void v30_ea16_write(UINT op, UINT32 madr, UINT16 value);

static UINT8 v30_shift8(UINT8 value, UINT count, UINT subform) {

	UINT8	result;
	UINT8	carry;

	if (!count) {
		return value;
	}
	switch (subform) {
	case 4:
	case 6:
		result = (count < 8) ? (UINT8)(value << count) : 0;
		carry = (count <= 8) ? (UINT8)((value >> (8 - count)) & 1) : 0;
		UPD9002_OV = ((result >> 7) ^ carry) & 1;
		break;

	case 5:
		result = (count < 8) ? (UINT8)(value >> count) : 0;
		carry = (count <= 8) ? (UINT8)((value >> (count - 1)) & 1) : 0;
		UPD9002_OV = ((result >> 7) ^ (result >> 6)) & 1;
		break;

	default:
		if (count < 8) {
			result = (UINT8)(value >> count);
			if (value & 0x80) {
				result |= (UINT8)(0xffU << (8 - count));
			}
		}
		else {
			result = (value & 0x80) ? 0xff : 0;
		}
		carry = (count <= 8) ? (UINT8)((value >> (count - 1)) & 1) :
								(UINT8)((value >> 7) & 1);
		UPD9002_OV = 0;
		break;
	}
	UPD9002_FLAGL = (UINT8)((UPD9002_FLAGL & 0x02) | carry |
								BYTESZPF(result));
	return result;
}

static UINT16 v30_shift16(UINT16 value, UINT count, UINT subform) {

	UINT16	result;
	UINT8	carry;

	if (!count) {
		return value;
	}
	switch (subform) {
	case 4:
	case 6:
		result = (count < 16) ? (UINT16)(value << count) : 0;
		carry = (count <= 16) ?
							(UINT8)((value >> (16 - count)) & 1) : 0;
		UPD9002_OV = ((result >> 15) ^ carry) & 1;
		break;

	case 5:
		result = (count < 16) ? (UINT16)(value >> count) : 0;
		carry = (count <= 16) ?
							(UINT8)((value >> (count - 1)) & 1) : 0;
		UPD9002_OV = ((result >> 15) ^ (result >> 14)) & 1;
		break;

	default:
		if (count < 16) {
			result = (UINT16)(value >> count);
			if (value & 0x8000) {
				result |= (UINT16)(0xffffU << (16 - count));
			}
		}
		else {
			result = (value & 0x8000) ? 0xffff : 0;
		}
		carry = (count <= 16) ?
							(UINT8)((value >> (count - 1)) & 1) :
							(UINT8)((value >> 15) & 1);
		UPD9002_OV = 0;
		break;
	}
	UPD9002_FLAGL = (UINT8)((UPD9002_FLAGL & 0x02) | carry |
								WORDSZPF(result));
	return result;
}

UPD9002FN v30shift_ea8_data8(void) {			// C0:	shift	EA8, DATA8

	UINT8	*out;
	UINT	op;
	UINT32	madr = 0;
	REG8	cl;

	GET_PCBYTE(op)
	if (op & 0x20) {
		UINT8	value;

		UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 8);
		value = v30_ea8_read(op, &madr);
		GET_PCBYTE(cl)
		UPD9002_WORKCLOCK(cl);
		if (cl) {
			v30_ea8_write(op, madr,
						v30_shift8(value, cl, (op >> 3) & 7));
		}
		return;
	}
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG8_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			GET_PCBYTE(cl)
			if ((op & 0x30) == 0x10) {		// rotate with carry
				cl = shiftbase09[cl];
			}
			else {
				cl = shiftbase16[cl];
			}
			UPD9002_WORKCLOCK(cl);
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	GET_PCBYTE(cl)
	if ((op & 0x30) == 0x10) {		// rotate with carry
		cl = shiftbase09[cl];
	}
	else {
		cl = shiftbase16[cl];
	}
	UPD9002_WORKCLOCK(cl);
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN v30shift_ea16_data8(void) {			// C1:	shift	EA16, DATA8

	UINT16	*out;
	UINT	op;
	UINT32	madr = 0;
	REG8	cl;

	GET_PCBYTE(op)
	if (op & 0x20) {
		UINT16	value;

		UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 8);
		value = v30_ea16_read(op, &madr);
		GET_PCBYTE(cl);
		UPD9002_WORKCLOCK(cl);
		if (cl) {
			v30_ea16_write(op, madr,
						v30_shift16(value, cl, (op >> 3) & 7));
		}
		return;
	}
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG16_B20(op);
	}
	else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			GET_PCBYTE(cl);
			if ((op & 0x30) == 0x10) {		// rotate with carry
				cl = shiftbase17[cl];
			}
			else {
				cl = shiftbase16[cl];
			}
			UPD9002_WORKCLOCK(cl);
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCBYTE(cl);
	if ((op & 0x30) == 0x10) {		// rotate with carry
		cl = shiftbase17[cl];
	}
	else {
		cl = shiftbase16[cl];
	}
	UPD9002_WORKCLOCK(cl);
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN v30shift_ea8_cl(void) {				// D2:	shift EA8, cl

	UINT8	*out;
	UINT	op;
	UINT32	madr = 0;
	REG8	cl;

	GET_PCBYTE(op)
	if (op & 0x20) {
		UINT8	value;

		UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 8);
		value = v30_ea8_read(op, &madr);
		cl = UPD9002_CL;
		UPD9002_WORKCLOCK(cl);
		if (cl) {
			v30_ea8_write(op, madr,
						v30_shift8(value, cl, (op >> 3) & 7));
		}
		return;
	}
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
			if ((op & 0x30) == 0x10) {		// rotate with carry
				cl = shiftbase09[cl];
			}
			else {
				cl = shiftbase16[cl];
			}
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	cl = UPD9002_CL;
	UPD9002_WORKCLOCK(cl);
	if ((op & 0x30) == 0x10) {		// rotate with carry
		cl = shiftbase09[cl];
	}
	else {
		cl = shiftbase16[cl];
	}
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN v30shift_ea16_cl(void) {				// D3:	shift EA16, cl

	UINT16	*out;
	UINT	op;
	UINT32	madr = 0;
	REG8	cl;

	GET_PCBYTE(op)
	if (op & 0x20) {
		UINT16	value;

		UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 8);
		value = v30_ea16_read(op, &madr);
		cl = UPD9002_CL;
		UPD9002_WORKCLOCK(cl);
		if (cl) {
			v30_ea16_write(op, madr,
						v30_shift16(value, cl, (op >> 3) & 7));
		}
		return;
	}
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
			if ((op & 0x30) == 0x10) {		// rotate with carry
				cl = shiftbase17[cl];
			}
			else {
				cl = shiftbase16[cl];
			}
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	cl = UPD9002_CL;
	UPD9002_WORKCLOCK(cl);
	if ((op & 0x30) == 0x10) {		// rotate with carry
		cl = shiftbase17[cl];
	}
	else {
		cl = shiftbase16[cl];
	}
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

static void v30_adjust_flags(UINT8 value, BOOL adjust_low,
							BOOL adjust_high, UINT overflow) {

	UPD9002_FLAGL = (UINT8)((UPD9002_FLAGL & 0x02) |
						(adjust_low ? A_FLAG : 0) |
						(adjust_high ? C_FLAG : 0) |
						BYTESZPF(value));
	UPD9002_OV = overflow;
}

UPD9002FN v30_daa(void) {						// 27:	DAA

	const UINT8 value = UPD9002_AL;
	const BOOL adjust_low =
				((UPD9002_FLAGL & A_FLAG) || ((value & 0x0f) > 9));
	const BOOL adjust_high =
				((UPD9002_FLAGL & C_FLAG) || (value > 0x9f) ||
				 ((value > 0x99) && !(UPD9002_FLAGL & A_FLAG)));
	const UINT8 delta = (UINT8)((adjust_low ? 6 : 0) +
								(adjust_high ? 0x60 : 0));
	const UINT8 result = (UINT8)(value + delta);

	UPD9002_WORKCLOCK(3);
	UPD9002_AL = result;
	v30_adjust_flags(result, adjust_low, adjust_high,
					(UINT)((~(value ^ delta) & (value ^ result)) & 0x80));
}

UPD9002FN v30_das(void) {						// 2F:	DAS

	const UINT8 value = UPD9002_AL;
	const BOOL adjust_low =
				((UPD9002_FLAGL & A_FLAG) || ((value & 0x0f) > 9));
	const BOOL adjust_high =
				((UPD9002_FLAGL & C_FLAG) || (value > 0x9f) ||
				 ((value > 0x99) && !(UPD9002_FLAGL & A_FLAG)));
	const UINT8 delta = (UINT8)((adjust_low ? 6 : 0) +
								(adjust_high ? 0x60 : 0));
	const UINT8 result = (UINT8)(value - delta);

	UPD9002_WORKCLOCK(3);
	UPD9002_AL = result;
	v30_adjust_flags(result, adjust_low, adjust_high,
					(UINT)(((value ^ delta) & (value ^ result)) & 0x80));
}

UPD9002FN v30_aaa(void) {						// 37:	AAA

	const UINT8 value = UPD9002_AL;
	const BOOL adjust =
				((UPD9002_FLAGL & A_FLAG) || ((value & 0x0f) > 9));
	UINT8 result = value;

	UPD9002_WORKCLOCK(3);
	if (adjust) {
		result = (UINT8)(value + 6);
		UPD9002_AH++;
	}
	UPD9002_AL = result & 0x0f;
	v30_adjust_flags(result, adjust, adjust,
					(UINT)((~(value ^ 6) & (value ^ result)) & 0x80));
}

UPD9002FN v30_aas(void) {						// 3F:	AAS

	const UINT8 value = UPD9002_AL;
	const BOOL adjust =
				((UPD9002_FLAGL & A_FLAG) || ((value & 0x0f) > 9));
	UINT8 result = value;

	UPD9002_WORKCLOCK(3);
	if (adjust) {
		result = (UINT8)(value - 6);
		UPD9002_AH--;
	}
	UPD9002_AL = result & 0x0f;
	v30_adjust_flags(result, adjust, adjust,
					(UINT)(((value ^ 6) & (value ^ result)) & 0x80));
}

UPD9002FN v30_aam(void) {						// D4:	AAM

	UINT8	al;
	UINT	radix;

	UPD9002_WORKCLOCK(16);
	GET_PCBYTE(radix);
	al = UPD9002_AL;
	if (radix) {
		UPD9002_AH = (UINT8)(al / radix);
		UPD9002_AL = (UINT8)(al % radix);
	}
	else {
		UPD9002_AH = 0xff;
	}
	UPD9002_FLAGL = (UPD9002_FLAGL & 0x02) | BYTESZPF(UPD9002_AL);
	UPD9002_OV = 0;
}

UPD9002FN v30_aad(void) {						// D5:	AAD

	UPD9002_WORKCLOCK(14);
	UPD9002_IP++;								// is 10
	UPD9002_AL += (UINT8)(UPD9002_AH * 10);
	UPD9002_AH = 0;
	UPD9002_FLAGL &= ~(S_FLAG | Z_FLAG | P_FLAG);
	UPD9002_FLAGL |= BYTESZPF(UPD9002_AL);
}

UPD9002FN v30_xlat(void) {						// D6:	xlat

	UPD9002_WORKCLOCK(5);
	UPD9002_AL = upd9002_memoryread(LOW16(UPD9002_AL + UPD9002_BX) + DS_FIX);
}

UPD9002FN v30_loop(void) {						// E2:	loop

	UPD9002_CX--;
	if (!UPD9002_CX) JMPNOP(5) else JMPSHORT(17)
}

UPD9002FN v30_repne(void) {					// F2:	repne

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30_repe(void) {						// F3:	repe

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30_repne_0f_diagnostic_stop(void) {

	upd9002_diagnostic_raise_rep0f(0xf2, v30_step_start_cs,
		v30_step_start_ip);
}

UPD9002FN v30_repe_0f_diagnostic_stop(void) {

	upd9002_diagnostic_raise_rep0f(0xf3, v30_step_start_cs,
		v30_step_start_ip);
}

static UINT16 v30_div_read_ea16(UINT op) {

	UINT	offset;
	UINT32	segment;

	offset = GET_EA(op, &segment);
	return (UINT16)(upd9002_memoryread(segment + offset) |
		(upd9002_memoryread(segment + LOW16(offset + 1)) << 8));
}

UPD9002_F6 v30_div_ea8(UINT op) {

	UINT16	dividend;
	UINT	flag_result;
	UINT8	src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(14);
		src = *(REG8_B20(op));
	}
	else {
		UPD9002_WORKCLOCK(17);
		src = upd9002_memoryread(CALC_EA(op));
	}
	dividend = UPD9002_AX;
	SUBBYTE(flag_result, UPD9002_AH, src)
	UPD9002_FLAGL |= 0x02;
	if (!src || (UPD9002_AH >= src)) {
		INT_NUM(0, UPD9002_IP);									// V30
		return;
	}
	UPD9002_AL = dividend / src;
	UPD9002_AH = dividend % src;
}

UPD9002_F6 v30_idiv_ea8(UINT op) {

	SINT32	dividend;
	SINT32	divisor;
	UINT32	dividend_magnitude;
	UINT32	divisor_magnitude;
	UINT32	quotient;
	UINT32	remainder;
	UINT	flag_result;
	UINT8	src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(17);
		src = *(REG8_B20(op));
	}
	else {
		UPD9002_WORKCLOCK(20);
		src = upd9002_memoryread(CALC_EA(op));
	}
	dividend = (SINT16)UPD9002_AX;
	divisor = (SINT8)src;
	dividend_magnitude = (UINT32)((dividend < 0) ? -dividend : dividend);
	divisor_magnitude = (UINT32)((divisor < 0) ? -divisor : divisor);
	SUBBYTE(flag_result, dividend_magnitude >> 8, divisor_magnitude)
	UPD9002_FLAGL |= 0x02;
	if (!divisor_magnitude ||
		((dividend_magnitude >> 8) >= divisor_magnitude)) {
		INT_NUM(0, UPD9002_IP);									// V30
		return;
	}
	quotient = dividend_magnitude / divisor_magnitude;
	remainder = dividend_magnitude % divisor_magnitude;
	UPD9002_OV = 0;
	UPD9002_FLAGL = BYTESZPF(quotient) | 0x02;
	if (quotient >= 0x80) {
		INT_NUM(0, UPD9002_IP);									// V30
		return;
	}
	UPD9002_AL = (UINT8)(((dividend < 0) != (divisor < 0)) ?
						(0U - quotient) : quotient);
	UPD9002_AH = (UINT8)((dividend < 0) ? (0U - remainder) : remainder);
}

UPD9002FN v30_ope0xf6(void) {					// F6:	

	UINT	op;

	GET_PCBYTE(op);
	v30ope0xf6_table[(op >> 3) & 7](op);
}

UPD9002_F6 v30_div_ea16(UINT op) {

	UINT32	dividend;
	UINT32	src;
	UINT32	flag_result;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(22);
		src = *(REG16_B20(op));
	}
	else {
		UPD9002_WORKCLOCK(25);
		src = v30_div_read_ea16(op);
	}
	dividend = ((UINT32)UPD9002_DX << 16) | UPD9002_AX;
	SUBWORD(flag_result, UPD9002_DX, src)
	UPD9002_FLAGL |= 0x02;
	if (!src || (UPD9002_DX >= src)) {
		INT_NUM(0, UPD9002_IP);									// V30
		return;
	}
	UPD9002_AX = dividend / src;
	UPD9002_DX = dividend % src;
}

UPD9002_F6 v30_idiv_ea16(UINT op) {

	SINT64	dividend;
	SINT64	divisor;
	UINT64	dividend_magnitude;
	UINT32	divisor_magnitude;
	UINT32	quotient;
	UINT32	remainder;
	UINT32	flag_result;
	UINT16	src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(25);
		src = *(REG16_B20(op));
	}
	else {
		UPD9002_WORKCLOCK(28);
		src = v30_div_read_ea16(op);
	}
	dividend = (SINT32)(((UINT32)UPD9002_DX << 16) | UPD9002_AX);
	divisor = (SINT16)src;
	dividend_magnitude = (UINT64)((dividend < 0) ? -dividend : dividend);
	divisor_magnitude = (UINT32)((divisor < 0) ? -divisor : divisor);
	SUBWORD(flag_result, dividend_magnitude >> 16, divisor_magnitude)
	UPD9002_FLAGL |= 0x02;
	if (!divisor_magnitude ||
		((dividend_magnitude >> 16) >= divisor_magnitude)) {
		INT_NUM(0, UPD9002_IP);									// V30
		return;
	}
	quotient = (UINT32)(dividend_magnitude / divisor_magnitude);
	remainder = (UINT32)(dividend_magnitude % divisor_magnitude);
	UPD9002_OV = 0;
	UPD9002_FLAGL = WORDSZPF(quotient) | 0x02;
	if (quotient >= 0x8000) {
		INT_NUM(0, UPD9002_IP);									// V30
		return;
	}
	UPD9002_AX = (UINT16)(((dividend < 0) != (divisor < 0)) ?
						(0U - quotient) : quotient);
	UPD9002_DX = (UINT16)((dividend < 0) ? (0U - remainder) : remainder);
}

UPD9002FN v30_ope0xf7(void) {					// F7:	

	UINT	op;

	GET_PCBYTE(op);
	v30ope0xf7_table[(op >> 3) & 7](op);
}

static UINT8 v30_ea8_read(UINT op, UINT32 *madr) {

	if (op >= 0xc0) {
		return *REG8_B20(op);
	}
	*madr = CALC_EA(op);
	return upd9002_memoryread(*madr);
}

static void v30_ea8_write(UINT op, UINT32 madr, UINT8 value) {

	if (op >= 0xc0) {
		*REG8_B20(op) = value;
	}
	else {
		upd9002_memorywrite(madr, value);
	}
}

static UINT16 v30_ea16_read(UINT op, UINT32 *madr) {

	if (op >= 0xc0) {
		return *REG16_B20(op);
	}
	*madr = CALC_EA(op);
	return upd9002_memoryread_w(*madr);
}

static void v30_ea16_write(UINT op, UINT32 madr, UINT16 value) {

	if (op >= 0xc0) {
		*REG16_B20(op) = value;
	}
	else {
		upd9002_memorywrite_w(madr, value);
	}
}

UPD9002FN v30_test1_ea8_cl(void) {				// 0F 10: test1 EA8, CL

	UINT	op;
	UINT32	madr = 0;
	UINT8	value;
	UINT8	mask;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?3:12);
	value = v30_ea8_read(op, &madr);
	mask = (UINT8)(1U << (UPD9002_CL & 7));
	UPD9002_OV = 0;
	UPD9002_FLAGL = BYTESZPF(value & mask);
}

UPD9002FN v30_test1_ea16_cl(void) {			// 0F 11: test1 EA16, CL

	UINT	op;
	UINT32	madr = 0;
	UINT16	value;
	UINT16	mask;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?3:12);
	value = v30_ea16_read(op, &madr);
	mask = (UINT16)(1U << (UPD9002_CL & 15));
	UPD9002_OV = 0;
	UPD9002_FLAGL = WORDSZPF(value & mask);
}

UPD9002FN v30_clr1_ea8_cl(void) {				// 0F 12: clr1 EA8, CL

	UINT	op;
	UINT32	madr = 0;
	UINT8	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?5:14);
	value = v30_ea8_read(op, &madr);
	value &= (UINT8)~(1U << (UPD9002_CL & 7));
	v30_ea8_write(op, madr, value);
}

UPD9002FN v30_clr1_ea16_cl(void) {			// 0F 13: clr1 EA16, CL

	UINT	op;
	UINT32	madr = 0;
	UINT16	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?5:14);
	value = v30_ea16_read(op, &madr);
	value &= (UINT16)~(1U << (UPD9002_CL & 15));
	v30_ea16_write(op, madr, value);
}

UPD9002FN v30_set1_ea8_cl(void) {				// 0F 14: set1 EA8, CL

	UINT	op;
	UINT32	madr = 0;
	UINT8	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?4:13);
	value = v30_ea8_read(op, &madr);
	value |= (UINT8)(1U << (UPD9002_CL & 7));
	v30_ea8_write(op, madr, value);
}

UPD9002FN v30_set1_ea16_cl(void) {			// 0F 15: set1 EA16, CL

	UINT	op;
	UINT32	madr = 0;
	UINT16	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?4:13);
	value = v30_ea16_read(op, &madr);
	value |= (UINT16)(1U << (UPD9002_CL & 15));
	v30_ea16_write(op, madr, value);
}

UPD9002FN v30_not1_ea8_cl(void) {				// 0F 16: not1 EA8, CL

	UINT	op;
	UINT32	madr = 0;
	UINT8	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?4:13);
	value = v30_ea8_read(op, &madr);
	value ^= (UINT8)(1U << (UPD9002_CL & 7));
	v30_ea8_write(op, madr, value);
}

UPD9002FN v30_not1_ea16_cl(void) {			// 0F 17: not1 EA16, CL

	UINT	op;
	UINT32	madr = 0;
	UINT16	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?4:13);
	value = v30_ea16_read(op, &madr);
	value ^= (UINT16)(1U << (UPD9002_CL & 15));
	v30_ea16_write(op, madr, value);
}

UPD9002FN v30_test1_ea8_i3(void) {				// 0F 18: test1 EA8, imm3

	UINT	op;
	UINT	imm;
	UINT32	madr = 0;
	UINT8	value;
	UINT8	mask;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?4:13);
	value = v30_ea8_read(op, &madr);
	GET_PCBYTE(imm);
	mask = (UINT8)(1U << (imm & 7));
	UPD9002_OV = 0;
	UPD9002_FLAGL = BYTESZPF(value & mask);
}

UPD9002FN v30_test1_ea16_i4(void) {			// 0F 19: test1 EA16, imm4

	UINT	op;
	UINT	imm;
	UINT32	madr = 0;
	UINT16	value;
	UINT16	mask;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?4:13);
	value = v30_ea16_read(op, &madr);
	GET_PCBYTE(imm);
	mask = (UINT16)(1U << (imm & 15));
	UPD9002_OV = 0;
	UPD9002_FLAGL = WORDSZPF(value & mask);
}

UPD9002FN v30_clr1_ea8_i3(void) {				// 0F 1A: clr1 EA8, imm3

	UINT	op;
	UINT	imm;
	UINT32	madr = 0;
	UINT8	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?6:15);
	value = v30_ea8_read(op, &madr);
	GET_PCBYTE(imm);
	value &= (UINT8)~(1U << (imm & 7));
	v30_ea8_write(op, madr, value);
}

UPD9002FN v30_clr1_ea16_i4(void) {				// 0F 1B: clr1 EA16, imm4

	UINT	op;
	UINT	imm;
	UINT32	madr = 0;
	UINT16	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?6:15);
	value = v30_ea16_read(op, &madr);
	GET_PCBYTE(imm);
	value &= (UINT16)~(1U << (imm & 15));
	v30_ea16_write(op, madr, value);
}

UPD9002FN v30_set1_ea8_i3(void) {				// 0F 1C: set1 EA8, imm3

	UINT	op;
	UINT	imm;
	UINT32	madr = 0;
	UINT8	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?5:14);
	value = v30_ea8_read(op, &madr);
	GET_PCBYTE(imm);
	value |= (UINT8)(1U << (imm & 7));
	v30_ea8_write(op, madr, value);
}

UPD9002FN v30_set1_ea16_i4(void) {				// 0F 1D: set1 EA16, imm4

	UINT	op;
	UINT	imm;
	UINT32	madr = 0;
	UINT16	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?5:14);
	value = v30_ea16_read(op, &madr);
	GET_PCBYTE(imm);
	value |= (UINT16)(1U << (imm & 15));
	v30_ea16_write(op, madr, value);
}

UPD9002FN v30_not1_ea8_i3(void) {				// 0F 1E: not1 EA8, imm3

	UINT	op;
	UINT	imm;
	UINT32	madr = 0;
	UINT8	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?5:14);
	value = v30_ea8_read(op, &madr);
	GET_PCBYTE(imm);
	value ^= (UINT8)(1U << (imm & 7));
	v30_ea8_write(op, madr, value);
}

UPD9002FN v30_not1_ea16_i4(void) {			// 0F 1F: not1 EA16, imm4

	UINT	op;
	UINT	imm;
	UINT32	madr = 0;
	UINT16	value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0)?5:14);
	value = v30_ea16_read(op, &madr);
	GET_PCBYTE(imm);
	value ^= (UINT16)(1U << (imm & 15));
	v30_ea16_write(op, madr, value);
}

static UINT8 v30_add8_flag(UINT8 dst, UINT8 src, UINT8 carry, UINT8 *result) {

	UINT	res;

	res = dst + src + (carry & C_FLAG);
	*result = (UINT8)res;
	return (UINT8)(((res ^ dst ^ src) & A_FLAG) | BYTESZPCF(res));
}

static UINT8 v30_sub8_flag(UINT8 dst, UINT8 src, UINT8 borrow, UINT8 *result) {

	UINT	res;

	res = dst - src - (borrow & C_FLAG);
	*result = (UINT8)res;
	return (UINT8)(((res ^ dst ^ src) & A_FLAG) | BYTESZPCF2(res));
}

static UINT8 v30_daa_local(UINT8 value, UINT8 flags, UINT8 *outflags) {

	const BOOL adjust_low = ((flags & A_FLAG) || ((value & 0x0f) > 9));
	const BOOL adjust_high = ((flags & C_FLAG) || (value > 0x9f) ||
					((value > 0x99) && !(flags & A_FLAG)));
	const UINT8 result = (UINT8)(value + (adjust_low ? 6 : 0) +
								(adjust_high ? 0x60 : 0));

	*outflags = (UINT8)((adjust_low ? A_FLAG : 0) |
						(adjust_high ? C_FLAG : 0) |
						BYTESZPF(result));
	return result;
}

static UINT8 v30_das_local(UINT8 value, UINT8 flags, UINT8 *outflags) {

	const BOOL adjust_low = ((flags & A_FLAG) || ((value & 0x0f) > 9));
	const BOOL adjust_high = ((flags & C_FLAG) || (value > 0x9f) ||
					((value > 0x99) && !(flags & A_FLAG)));
	const UINT8 result = (UINT8)(value - (adjust_low ? 6 : 0) -
								(adjust_high ? 0x60 : 0));

	*outflags = (UINT8)((adjust_low ? A_FLAG : 0) |
						(adjust_high ? C_FLAG : 0) |
						BYTESZPF(result));
	return result;
}

static UINT v30_addsub4s_extra_count(void) {

	UINT8	count;

	count = (UINT8)((UPD9002_CL + 1) >> 1);
	count = (UINT8)(count - 1);
	return count & 0x7f;
}

static void v30_addsub4s_finish(UINT8 flags) {

	if (flags & C_FLAG) {
		UPD9002_FLAGL = 0x93;
	}
	else if (flags & Z_FLAG) {
		UPD9002_FLAGL = 0x46;
	}
	else {
		UPD9002_FLAGL = 0x02;
	}
	UPD9002_OV = 0;
}

UPD9002FN v30_add4s(void) {					// 0F 20: add4s

	UINT16	srcoffset;
	UINT16	dstoffset;
	UINT	count;
	UINT8	src;
	UINT8	dst;
	UINT8	flags;
	UINT8	result;
	BOOL	all_zero;

	UPD9002_WORKCLOCK(26);
	srcoffset = UPD9002_SI;
	dstoffset = UPD9002_DI;
	flags = 0;
	all_zero = TRUE;
	for (count = v30_addsub4s_extra_count() + 1; count; count--) {
		const UINT32 srcaddr =
			(DS_FIX + srcoffset) & CPU_ADRSMASK;
		const UINT32 dstaddr =
			(ES_BASE + dstoffset) & CPU_ADRSMASK;

		src = upd9002_memoryread(srcaddr);
		dst = upd9002_memoryread(dstaddr);
		flags = v30_add8_flag(dst, src, flags, &result);
		result = v30_daa_local(result, flags, &flags);
		upd9002_memorywrite(dstaddr, result);
		all_zero = (BOOL)(all_zero && !result);
		srcoffset++;
		dstoffset++;
		if (count > 1) {
			UPD9002_WORKCLOCK(19);
		}
	}
	if (all_zero) {
		flags |= Z_FLAG;
	}
	else {
		flags &= (UINT8)~Z_FLAG;
	}
	v30_addsub4s_finish(flags);
}

static void v30_subcmp4s(BOOL compare_only) {

	UINT16	srcoffset;
	UINT16	dstoffset;
	UINT	count;
	UINT8	src;
	UINT8	dst;
	UINT8	flags;
	UINT8	result;
	BOOL	all_zero;

	UPD9002_WORKCLOCK(26);
	srcoffset = UPD9002_SI;
	dstoffset = UPD9002_DI;
	flags = 0;
	all_zero = TRUE;
	for (count = v30_addsub4s_extra_count() + 1; count; count--) {
		const UINT32 srcaddr =
			(DS_FIX + srcoffset) & CPU_ADRSMASK;
		const UINT32 dstaddr =
			(ES_BASE + dstoffset) & CPU_ADRSMASK;

		src = upd9002_memoryread(srcaddr);
		dst = upd9002_memoryread(dstaddr);
		flags = v30_sub8_flag(dst, src, flags, &result);
		result = v30_das_local(result, flags, &flags);
		if (!compare_only) {
			upd9002_memorywrite(dstaddr, result);
		}
		all_zero = (BOOL)(all_zero && !result);
		srcoffset++;
		dstoffset++;
		if (count > 1) {
			UPD9002_WORKCLOCK(19);
		}
	}
	if (all_zero) {
		flags |= Z_FLAG;
	}
	else {
		flags &= (UINT8)~Z_FLAG;
	}
	v30_addsub4s_finish(flags);
}

UPD9002FN v30_sub4s(void) {					// 0F 22: sub4s

	v30_subcmp4s(FALSE);
}

UPD9002FN v30_cmp4s(void) {					// 0F 26: cmp4s

	v30_subcmp4s(TRUE);
}

UPD9002FN v30_rol4_ea8(void) {				// 0F 28: rol4 EA8

	UINT	op;
	UINT32	madr = 0;
	UINT8	value;
	UINT8	oldal;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK(25);
	value = v30_ea8_read(op, &madr);
	oldal = UPD9002_AL;
	v30_ea8_write(op, madr,
				(UINT8)((value << 4) | (oldal & 0x0f)));
	UPD9002_AL = (UINT8)((oldal << 4) | (value >> 4));
}

UPD9002FN v30_ror4_ea8(void) {				// 0F 2A: ror4 EA8

	UINT	op;
	UINT32	madr = 0;
	UINT8	value;
	UINT8	oldal;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK(25);
	value = v30_ea8_read(op, &madr);
	oldal = UPD9002_AL;
	v30_ea8_write(op, madr,
				(UINT8)((value >> 4) | ((oldal & 0x0f) << 4)));
	UPD9002_AL = value;
}

UPD9002FN v30_reserved_repc(void) {

	UPD9002_WORKCLOCK(2);
	UPD9002_IP = v30_repc_ipbak;
}

UPD9002FN v30_reserved_repnc(void) {

	UPD9002_WORKCLOCK(2);
	UPD9002_IP = v30_repnc_ipbak;
}

UPD9002FN v30_repnc(void) {					// 64: repnc

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		v30_repnc_ipbak = (UINT16)(UPD9002_IP - 1);
		GET_PCBYTE(op);
		v30op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30_repc(void) {					// 65: repc

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		v30_repc_ipbak = (UINT16)(UPD9002_IP - 1);
		GET_PCBYTE(op);
		v30op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repnc_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		GET_PCBYTE(op);
		v30op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repnc_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		GET_PCBYTE(op);
		v30op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repnc_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		GET_PCBYTE(op);
		v30op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repnc_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		GET_PCBYTE(op);
		v30op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repc_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		GET_PCBYTE(op);
		v30op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repc_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		GET_PCBYTE(op);
		v30op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repc_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		GET_PCBYTE(op);
		v30op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repc_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT	op;

		GET_PCBYTE(op);
		v30op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

static const V30PATCH v30patch_repnc[] = {
			{0x26, v30repnc_segprefix_es},	// 26:	repnc es:
			{0x2e, v30repnc_segprefix_cs},	// 2E:	repnc cs:
			{0x36, v30repnc_segprefix_ss},	// 36:	repnc ss:
			{0x3e, v30repnc_segprefix_ds},	// 3E:	repnc ds:
			{0x64, v30_repnc},				// 64:	repnc
			{0x65, v30_repc},				// 65:	repc
			{0xa4, upd9002_repnc_movsb},	// A4:	repnc movsb
			{0xa5, upd9002_repnc_movsw},	// A5:	repnc movsw
			{0xa6, upd9002_repnc_cmpsb},	// A6:	repnc cmpsb
			{0xa7, upd9002_repnc_cmpsw},	// A7:	repnc cmpsw
			{0xaa, upd9002_repnc_stosb},	// AA:	repnc stosb
			{0xab, upd9002_repnc_stosw},	// AB:	repnc stosw
			{0xac, upd9002_repnc_lodsb},	// AC:	repnc lodsb
			{0xad, upd9002_repnc_lodsw},	// AD:	repnc lodsw
			{0xae, upd9002_repnc_scasb},	// AE:	repnc scasb
			{0xaf, upd9002_repnc_scasw}};	// AF:	repnc scasw

static const V30PATCH v30patch_repc[] = {
			{0x26, v30repc_segprefix_es},	// 26:	repc es:
			{0x2e, v30repc_segprefix_cs},	// 2E:	repc cs:
			{0x36, v30repc_segprefix_ss},	// 36:	repc ss:
			{0x3e, v30repc_segprefix_ds},	// 3E:	repc ds:
			{0x64, v30_repnc},				// 64:	repnc
			{0x65, v30_repc},				// 65:	repc
			{0xa4, upd9002_repc_movsb},		// A4:	repc movsb
			{0xa5, upd9002_repc_movsw},		// A5:	repc movsw
			{0xa6, upd9002_repc_cmpsb},		// A6:	repc cmpsb
			{0xa7, upd9002_repc_cmpsw},		// A7:	repc cmpsw
			{0xaa, upd9002_repc_stosb},		// AA:	repc stosb
			{0xab, upd9002_repc_stosw},		// AB:	repc stosw
			{0xac, upd9002_repc_lodsb},		// AC:	repc lodsb
			{0xad, upd9002_repc_lodsw},		// AD:	repc lodsw
			{0xae, upd9002_repc_scasb},		// AE:	repc scasb
			{0xaf, upd9002_repc_scasw}};	// AF:	repc scasw

UPD9002FN v30_reserved_0x0f(void) {

	UPD9002_WORKCLOCK(2);
}

UPD9002FN v30_iret(void) {					// CF: iret

	UINT	flag;

	REGPOP0(UPD9002_IP)
	REGPOP0(UPD9002_CS)
	REGPOP0(flag)
	CS_BASE = UPD9002_CS << 4;
	flag = (flag & 0x0fd7) | 0xf002;
	UPD9002_OV = flag & O_FLAG;
	UPD9002_FLAG = flag & (0xfff ^ O_FLAG);
	UPD9002_TRAP = ((flag & T_FLAG) != 0);
	UPD9002_WORKCLOCK(31);
	if ((UPD9002_TRAP) || ((flag & I_FLAG) && (PICEXISTINTR))) {
		UPD9002_IRQCHECKTERM
	}
}

static const UPD9002OP v30ope0x0f_table[64] = {
			v30_reserved_0x0f,				// 00:
			v30_reserved_0x0f,				// 01:
			v30_reserved_0x0f,				// 02:
			v30_reserved_0x0f,				// 03:
			v30_reserved_0x0f,				// 04:
			v30_reserved_0x0f,				// 05:
			v30_reserved_0x0f,				// 06:
			v30_reserved_0x0f,				// 07:
			v30_reserved_0x0f,				// 08:
			v30_reserved_0x0f,				// 09:
			v30_reserved_0x0f,				// 0A:
			v30_reserved_0x0f,				// 0B:
			v30_reserved_0x0f,				// 0C:
			v30_reserved_0x0f,				// 0D:
			v30_reserved_0x0f,				// 0E:
			v30_reserved_0x0f,				// 0F:

			v30_test1_ea8_cl,				// 10:
			v30_test1_ea16_cl,				// 11:
			v30_clr1_ea8_cl,				// 12:
			v30_clr1_ea16_cl,				// 13:
			v30_set1_ea8_cl,				// 14:
			v30_set1_ea16_cl,				// 15:
			v30_not1_ea8_cl,				// 16:
			v30_not1_ea16_cl,				// 17:
			v30_test1_ea8_i3,				// 18:
			v30_test1_ea16_i4,				// 19:
			v30_clr1_ea8_i3,				// 1A:
			v30_clr1_ea16_i4,				// 1B:
			v30_set1_ea8_i3,				// 1C:
			v30_set1_ea16_i4,				// 1D:
			v30_not1_ea8_i3,				// 1E:
			v30_not1_ea16_i4,				// 1F:

			v30_add4s,					// 20:
			v30_reserved_0x0f,				// 21:
			v30_sub4s,					// 22:
			v30_reserved_0x0f,				// 23:
			v30_reserved_0x0f,				// 24:
			v30_reserved_0x0f,				// 25:
			v30_cmp4s,					// 26:
			v30_reserved_0x0f,				// 27:
			v30_rol4_ea8,					// 28:
			v30_reserved_0x0f,				// 29:
			v30_ror4_ea8,					// 2A:
			v30_reserved_0x0f,				// 2B:
			v30_reserved_0x0f,				// 2C:
			v30_reserved_0x0f,				// 2D:
			v30_reserved_0x0f,				// 2E:
			v30_reserved_0x0f,				// 2F:

			v30_reserved_0x0f,				// 30:
			v30_reserved_0x0f,				// 31:
			v30_reserved_0x0f,				// 32:
			v30_reserved_0x0f,				// 33:
			v30_reserved_0x0f,				// 34:
			v30_reserved_0x0f,				// 35:
			v30_reserved_0x0f,				// 36:
			v30_reserved_0x0f,				// 37:
			v30_reserved_0x0f,				// 38:
			v30_reserved_0x0f,				// 39:
			v30_reserved_0x0f,				// 3A:
			v30_reserved_0x0f,				// 3B:
			v30_reserved_0x0f,				// 3C:
			v30_reserved_0x0f,				// 3D:
			v30_reserved_0x0f,				// 3E:
			v30_reserved_0x0f};				// 3F:

UPD9002FN v30_ope0x0f(void) {				// 0F:

	UINT	op;

	op = upd9002_memoryread(CS_BASE + UPD9002_IP);
	if (op & 0xc0) {
		v30_reserved_0x0f();
		return;
	}
	UPD9002_IP++;
	v30ope0x0f_table[op]();
}

static const V30PATCH v30patch_op[] = {
			{0x0f, v30_ope0x0f},			// 0F:
			{0x26, v30segprefix_es},		// 26:	es:
			{0x27, v30_daa},				// 27:	daa
			{0x2e, v30segprefix_cs},		// 2E:	cs:
			{0x2f, v30_das},				// 2F:	das
			{0x36, v30segprefix_ss},		// 36:	ss:
			{0x37, v30_aaa},				// 37:	aaa
			{0x3e, v30segprefix_ds},		// 3E:	ds:
			{0x3f, v30_aas},				// 3F:	aas
			{0x54, v30push_sp},				// 54:	push	sp
			{0x63, v30_reserved},			// 63:	reserved
			{0x64, v30_repnc},				// 64:	repnc
			{0x65, v30_repc},				// 65:	repc
			{0x66, v30_reserved},			// 66:	reserved
			{0x67, v30_reserved},			// 67:	reserved
			{0x8e, v30mov_seg_ea},			// 8E:	mov		segrem, EA
			{0x9c, v30_pushf},				// 9C:	pushf
			{0x9d, v30_popf},				// 9D:	popf
			{0xc0, v30shift_ea8_data8},		// C0:	shift	EA8, DATA8
			{0xc1, v30shift_ea16_data8},	// C1:	shift	EA16, DATA8
			{0xcf, v30_iret},				// CF:	iret
			{0xd2, v30shift_ea8_cl},		// D2:	shift EA8, cl
			{0xd3, v30shift_ea16_cl},		// D3:	shift EA16, cl
			{0xd4, v30_aam},				// D4:	AAM
			{0xd5, v30_aad},				// D5:	AAD
			{0xd6, v30_xlat},				// D6:	xlat (8086/V30)
			{0xe2, v30_loop},				// E2:	loop
			{0xf2, v30_repne},				// F2:	repne
			{0xf3, v30_repe},				// F3:	repe
			{0xf6, v30_ope0xf6},			// F6:
			{0xf7, v30_ope0xf7}};			// F7:


// ----------------------------------------------------------------- repe

UPD9002FN v30repe_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repe_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repe_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repe_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

static const V30PATCH v30patch_repe[] = {
			{0x0f, v30_repe_0f_diagnostic_stop},
			{0x26, v30repe_segprefix_es},	// 26:	repe es:
			{0x2e, v30repe_segprefix_cs},	// 2E:	repe cs:
			{0x36, v30repe_segprefix_ss},	// 36:	repe ss:
			{0x3e, v30repe_segprefix_ds},	// 3E:	repe ds:
			{0x54, v30push_sp},				// 54:	push	sp
			{0x63, v30_reserved},			// 63:	reserved
			{0x64, v30_reserved},			// 64:	reserved
			{0x65, v30_reserved},			// 65:	reserved
			{0x66, v30_reserved},			// 66:	reserved
			{0x67, v30_reserved},			// 67:	reserved
			{0x8e, v30mov_seg_ea},			// 8E:	mov		segrem, EA
			{0x9c, v30_pushf},				// 9C:	pushf
			{0x9d, v30_popf},				// 9D:	popf
			{0xc0, v30shift_ea8_data8},		// C0:	shift	EA8, DATA8
			{0xc1, v30shift_ea16_data8},	// C1:	shift	EA16, DATA8
			{0xcf, v30_iret},				// CF:	iret
			{0xd2, v30shift_ea8_cl},		// D2:	shift EA8, cl
			{0xd3, v30shift_ea16_cl},		// D3:	shift EA16, cl
			{0xd4, v30_aam},				// D4:	AAM
			{0xd5, v30_aad},				// D5:	AAD
			{0xd6, v30_xlat},				// D6:	xlat (8086/V30)
			{0xf2, v30_repne},				// F2:	repne
			{0xf3, v30_repe},				// F3:	repe
			{0xf6, v30_ope0xf6},			// F6:
			{0xf7, v30_ope0xf7}};			// F7:


// ----------------------------------------------------------------- repne

UPD9002FN v30repne_segprefix_es(void) {

	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repne_segprefix_cs(void) {

	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repne_segprefix_ss(void) {

	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN v30repne_segprefix_ds(void) {

	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		v30op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	}
	else {
		INT_NUM(6, UPD9002_IP);
	}
}

static const V30PATCH v30patch_repne[] = {
			{0x0f, v30_repne_0f_diagnostic_stop},
			{0x26, v30repne_segprefix_es},	// 26:	repne es:
			{0x2e, v30repne_segprefix_cs},	// 2E:	repne cs:
			{0x36, v30repne_segprefix_ss},	// 36:	repne ss:
			{0x3e, v30repne_segprefix_ds},	// 3E:	repne ds:
			{0x54, v30push_sp},				// 54:	push	sp
			{0x63, v30_reserved},			// 63:	reserved
			{0x64, v30_reserved},			// 64:	reserved
			{0x65, v30_reserved},			// 65:	reserved
			{0x66, v30_reserved},			// 66:	reserved
			{0x67, v30_reserved},			// 67:	reserved
			{0x8e, v30mov_seg_ea},			// 8E:	mov		segrem, EA
			{0x9c, v30_pushf},				// 9C:	pushf
			{0x9d, v30_popf},				// 9D:	popf
			{0xc0, v30shift_ea8_data8},		// C0:	shift	EA8, DATA8
			{0xc1, v30shift_ea16_data8},	// C1:	shift	EA16, DATA8
			{0xcf, v30_iret},				// CF:	iret
			{0xd2, v30shift_ea8_cl},		// D2:	shift EA8, cl
			{0xd3, v30shift_ea16_cl},		// D3:	shift EA16, cl
			{0xd4, v30_aam},				// D4:	AAM
			{0xd5, v30_aad},				// D5:	AAD
			{0xd6, v30_xlat},				// D6:	xlat (8086/V30)
			{0xf2, v30_repne},				// F2:	repne
			{0xf3, v30_repe},				// F3:	repe
			{0xf6, v30_ope0xf6},			// F6:
			{0xf7, v30_ope0xf7}};			// F7:


// ---------------------------------------------------------------------------

static void v30patching(UPD9002OP *op, const V30PATCH *patch, int cnt) {

	do {
		op[patch->opnum] = patch->v30opcode;
		patch++;
	} while(--cnt);
}

#define	V30PATCHING(a, b)	v30patching(a, b, sizeof(b)/sizeof(V30PATCH))

void upd9002_dispatch_initialize(void) {

	UINT	i;

	/* ADR-0012: this is the sole live dispatch-construction path. */
	if (v30_dispatch_initialized) {
#if defined(VAEG_UPD9002_M46_TESTING)
		v30_dispatch_rejected_count++;
#endif
		return;
	}
	CopyMemory(v30op, upd9002op, sizeof(v30op));
	V30PATCHING(v30op, v30patch_op);
	CopyMemory(v30op_repne, upd9002op_repne, sizeof(v30op_repne));
	V30PATCHING(v30op_repne, v30patch_repne);
	CopyMemory(v30op_repe, upd9002op_repe, sizeof(v30op_repe));
	V30PATCHING(v30op_repe, v30patch_repe);
	CopyMemory(v30ope0xf6_table, c_ope0xf6_table, sizeof(v30ope0xf6_table));
	v30ope0xf6_table[6] = v30_div_ea8;
	v30ope0xf6_table[7] = v30_idiv_ea8;
	CopyMemory(v30ope0xf7_table, c_ope0xf7_table, sizeof(v30ope0xf7_table));
	v30ope0xf7_table[6] = v30_div_ea16;
	v30ope0xf7_table[7] = v30_idiv_ea16;
	for (i=0; i<0x100; i++) {
		v30op_repnc[i] = v30_reserved_repnc;
		v30op_repc[i] = v30_reserved_repc;
	}
	V30PATCHING(v30op_repnc, v30patch_repnc);
	V30PATCHING(v30op_repc, v30patch_repc);
#if defined(VAEG_UPD9002_M46_TESTING)
	v30_dispatch_snapshot();
	v30_dispatch_construction_count++;
#endif
	v30_dispatch_initialized = TRUE;
}

void upd9002_core_step(void) {

	UINT	opcode;
	BOOL	preserve_state;
	Upd9002RuntimeState state_before;

	if (upd9002_diagnostic_pending()) {
		return;
	}

	upd9002_trace_step_begin();
	v30_step_start_cs = UPD9002_CS;
	v30_step_start_ip = UPD9002_IP;
	opcode = upd9002_memoryread(CS_BASE + UPD9002_IP);
	preserve_state = (opcode == 0x26) || (opcode == 0x2e) ||
		(opcode == 0x36) || (opcode == 0x3e) ||
		(opcode == 0xf2) || (opcode == 0xf3);
	if (preserve_state) {
		state_before = upd9002_core_context.s;
	}
	UPD9002_OV = UPD9002_FLAG & O_FLAG;
	UPD9002_FLAG &= ~(O_FLAG);

	UPD9002_IP++;
	v30op[opcode]();

	UPD9002_FLAG &= ~(O_FLAG);
	if (UPD9002_OV) {
		UPD9002_FLAG |= (O_FLAG);
	}
	if (upd9002_diagnostic_pending()) {
		/*
		 * Every active path to the diagnostic starts with one of the
		 * prefixes above. Restore the complete runtime image so the
		 * unresolved encoding has no architectural effect.
		 */
		if (preserve_state) {
			upd9002_core_context.s = state_before;
		}
		upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU,
			"diagnostic-stop-rep0f", CS_BASE + UPD9002_IP, opcode, 1);
		upd9002_trace_step_end();
		return;
	}
	V30_DMAP();
	upd9002_trace_step_end();
}
