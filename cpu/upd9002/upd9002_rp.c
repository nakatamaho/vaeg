#include	"compiler.h"
#include	"cpucore.h"
#include	"upd9002_ops.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"upd9002_ops.mcr"


// ---------------------------------------------------------------------- ins

UPD9002EXT upd9002_rep_insb(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		do {
			REG8 dat = iocore_inp8(UPD9002_DX);
			upd9002_memorywrite(UPD9002_DI + ES_BASE, dat);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(4);
		} while (--UPD9002_CX);
	}
}

UPD9002EXT upd9002_rep_insw(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		do {
			REG16 dat = iocore_inp16(UPD9002_DX);
			upd9002_memorywrite_w(UPD9002_DI + ES_BASE, dat);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(4);
		} while(--UPD9002_CX);
	}
}

// ---------------------------------------------------------------------- outs

UPD9002EXT upd9002_rep_outsb(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		do {
			REG8 dat = upd9002_memoryread(UPD9002_SI + DS_FIX);
			UPD9002_SI += stp;
			iocore_out8(UPD9002_DX, (BYTE)dat);
			UPD9002_WORKCLOCK(4);
		} while(--UPD9002_CX);
	}
}

UPD9002EXT upd9002_rep_outsw(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		do {
			REG16 dat = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
			UPD9002_SI += stp;
			iocore_out16(UPD9002_DX, (UINT16)dat);
			UPD9002_WORKCLOCK(4);
		} while(--UPD9002_CX);
	}
}


// ---------------------------------------------------------------------- movs

#if 1
UPD9002EXT upd9002_rep_movsb(void) {

	UINT16	r_cx;
	int		stp;
	UINT16	r_si;
	UINT16	r_di;

	UPD9002_WORKCLOCK(5);
	r_cx = UPD9002_CX;
	if (r_cx) {
		stp = STRING_DIR;
		r_si = UPD9002_SI;
		r_di = UPD9002_DI;
		while(1) {
			REG8 dat = upd9002_memoryread(DS_FIX + r_si);
			upd9002_memorywrite(ES_BASE + r_di, dat);
			r_si += stp;
			r_di += stp;
			UPD9002_WORKCLOCK(4);
			r_cx--;
			if (!r_cx) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
		UPD9002_CX = r_cx;
		UPD9002_SI = r_si;
		UPD9002_DI = r_di;
	}
}

UPD9002EXT upd9002_rep_movsw(void) {

	UINT16	r_cx;
	int		stp;
	UINT16	r_si;
	UINT16	r_di;

	UPD9002_WORKCLOCK(5);
	r_cx = UPD9002_CX;
	if (r_cx) {
		stp = STRING_DIRx2;
		r_si = UPD9002_SI;
		r_di = UPD9002_DI;
		while(1) {
			REG16 dat = upd9002_memoryread_seg_w(DS_FIX, r_si);
			upd9002_memorywrite_seg_w(ES_BASE, r_di, dat);
			r_si += stp;
			r_di += stp;
			UPD9002_WORKCLOCK(4);
			r_cx--;
			if (!r_cx) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
		UPD9002_CX = r_cx;
		UPD9002_SI = r_si;
		UPD9002_DI = r_di;
	}
}
#else
UPD9002EXT upd9002_rep_movsb(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		while(1) {
			REG8 dat = upd9002_memoryread(UPD9002_SI + DS_FIX);
			upd9002_memorywrite(UPD9002_DI + ES_BASE, dat);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(4);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

UPD9002EXT upd9002_rep_movsw(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		while(1) {
			REG16 dat = upd9002_memoryread_seg_w(DS_FIX, UPD9002_SI);
			upd9002_memorywrite_seg_w(ES_BASE, UPD9002_DI, dat);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(4);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}
#endif


// ---------------------------------------------------------------------- lods

UPD9002EXT upd9002_rep_lodsb(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		while(1) {
			UPD9002_AL = upd9002_memoryread(UPD9002_SI + DS_FIX);
			UPD9002_SI += stp;
			UPD9002_WORKCLOCK(4);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

UPD9002EXT upd9002_rep_lodsw(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		while(1) {
			UPD9002_AX = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
			UPD9002_SI += stp;
			UPD9002_WORKCLOCK(4);
		 	UPD9002_CX--;
		 	if (!UPD9002_CX) {
		 		break;
		 	}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}


// ---------------------------------------------------------------------- stos

UPD9002EXT upd9002_rep_stosb(void) {

	UPD9002_WORKCLOCK(4);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		while(1) {
			upd9002_memorywrite(UPD9002_DI + ES_BASE, UPD9002_AL);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(3);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

UPD9002EXT upd9002_rep_stosw(void) {

	UPD9002_WORKCLOCK(4);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		while(1) {
			upd9002_memorywrite_w(UPD9002_DI + ES_BASE, UPD9002_AX);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(3);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}


// ---------------------------------------------------------------------- cmps

UPD9002EXT upd9002_repe_cmpsb(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		do {
			UINT res;
			UINT dst = upd9002_memoryread(UPD9002_SI + DS_FIX);
			UINT src = upd9002_memoryread(UPD9002_DI + ES_BASE);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(9);
			SUBBYTE(res, dst, src)
			UPD9002_CX--;
		} while((UPD9002_CX) && (UPD9002_FLAGL & Z_FLAG));
	}
}

UPD9002EXT upd9002_repne_cmpsb(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		do {
			UINT res;
			UINT dst = upd9002_memoryread(UPD9002_SI + DS_FIX);
			UINT src = upd9002_memoryread(UPD9002_DI + ES_BASE);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(9);
			SUBBYTE(res, dst, src)
			UPD9002_CX--;
		} while((UPD9002_CX) && (!(UPD9002_FLAGL & Z_FLAG)));
	}
}

UPD9002EXT upd9002_repe_cmpsw(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		do {
			UINT32 res;
			UINT32 dst = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
			UINT32 src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(9);
			SUBWORD(res, dst, src)
			UPD9002_CX--;
		} while((UPD9002_CX) && (UPD9002_FLAGL & Z_FLAG));
	}
}

UPD9002EXT upd9002_repne_cmpsw(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		do {
			UINT32 res;
			UINT32 dst = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
			UINT32 src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(9);
			SUBWORD(res, dst, src)
			UPD9002_CX--;
		} while((UPD9002_CX) && (!(UPD9002_FLAGL & Z_FLAG)));
	}
}


// ---------------------------------------------------------------------- scas

UPD9002EXT upd9002_repe_scasb(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		UINT dst = UPD9002_AL;
		do {
			UINT res;
			UINT src = upd9002_memoryread(UPD9002_DI + ES_BASE);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(8);
			SUBBYTE(res, dst, src)
			UPD9002_CX--;
		} while((UPD9002_CX) && (UPD9002_FLAGL & Z_FLAG));
	}
}

UPD9002EXT upd9002_repne_scasb(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		UINT dst = UPD9002_AL;
		do {
			UINT res;
			UINT src = upd9002_memoryread(UPD9002_DI + ES_BASE);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(8);
			SUBBYTE(res, dst, src)
			UPD9002_CX--;
		} while((UPD9002_CX) && (!(UPD9002_FLAGL & Z_FLAG)));
	}
}

UPD9002EXT upd9002_repe_scasw(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		UINT32 dst = UPD9002_AX;
		do {
			UINT32 res;
			UINT32 src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(8);
			SUBWORD(res, dst, src)
			UPD9002_CX--;
		} while((UPD9002_CX) && (UPD9002_FLAGL & Z_FLAG));
	}
}

UPD9002EXT upd9002_repne_scasw(void) {

	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		UINT32 dst = UPD9002_AX;
		do {
			UINT32 res;
			UINT32 src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(8);
			SUBWORD(res, dst, src)
			UPD9002_CX--;
		} while((UPD9002_CX) && (!(UPD9002_FLAGL & Z_FLAG)));
	}
}
