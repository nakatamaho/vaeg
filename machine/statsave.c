#include "compiler.h"
#include "strres.h"
#include "dosio.h"
#include "commng.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "timemng.h"
#include "cpucore.h"
#include "upd9002_state.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "cbuscore.h"
#include "sasiio.h"
#include "scsiio.h"
#include "mpu98ii.h"
#include "sound.h"
#include "fmboard.h"
#include "beep.h"
#include "font.h"
#include "fddfile.h"
#include "common/sha256.h"
#include "fdd_mtr.h"
#include "sxsi.h"
#include "hostfat.h"
#include "machine/calendar.h"
#include "machine/keystat.h"

#include "bmsio.h"

#include <string.h>

#include "sysportva.h"
#include "cmt.h"
#include "memoryva.h"
#include "gvramva.h"
#include "videova.h"
#include "tsp.h"
#include "cgromva.h"
#include "pcg88.h"
#include "gactrlva.h"
#include "sgp.h"
#include "mouseifva.h"
#include "subsystemif.h"
#include "subsystem.h"
#include "boardsb2.h"
#include "ymfmbridge.h"
#include "va91.h"
#include "upd9002_regs.h"
#include "recording/recording_native_session.h"

#if defined(MACOS)
#define CRCONST str_cr
#elif defined(WIN32) || defined(X11) || defined(SLZAURUS)
#define CRCONST str_lf
#else
#define CRCONST str_crlf
#endif

typedef struct {
	char name[16];
	char vername[28];
	UINT32 ver;
} NP2FHDR;

typedef struct {
	char index[10];
	UINT16 ver;
	UINT32 size;
} NP2FENT;

enum {
	STATFLAG_BIN = 0,
	STATFLAG_UPD9002_CPU,
	STATFLAG_UPD9002_COMPAT,
	STATFLAG_UPD9002_ALT,
	STATFLAG_UPD9002_MODE,
	STATFLAG_TERM,
	STATFLAG_COM,
	STATFLAG_DISK,
	STATFLAG_DMA,
	STATFLAG_EVT,
	STATFLAG_EXT,
	STATFLAG_FM,
	STATFLAG_MEM,
	STATFLAG_BMS,
	STATFLAG_SUBCPU,
	STATFLAG_HOSTFAT,
	STATFLAG_UPD8087,
	STATFLAG_MAIN_CPU,
};

typedef struct {
	UINT32 id;
	void *proc;
} PROCTBL;

typedef struct {
	UINT32 id;
	int num;
} ENUMTBL;

#define PROCID(a, b, c, d) (((d) << 24) + ((c) << 16) + ((b) << 8) + (a))
#define PROC2NUM(a, b) proc2num(&(a), (b), sizeof(b) / sizeof(PROCTBL))
#define NUM2PROC(a, b) num2proc(&(a), (b), sizeof(b) / sizeof(PROCTBL))

#include "machine/statsave.tbl"

extern COMMNG cm_mpu98;
extern COMMNG cm_rs232c;

typedef struct {
	char *buf;
	int remain;
} ERR_BUF;

// ----

// Replace a serialized callback pointer with its stable table identifier.
static BOOL proc2num(void *func, const PROCTBL *tbl, int size) {
	int i;
	VAEG_INTPTR *slot;

	slot = (VAEG_INTPTR *)func;
	for (i = 0; i < size; i++) {
		if (*slot == (VAEG_INTPTR)tbl->proc) {
			*slot = (VAEG_INTPTR)tbl->id;
			return (SUCCESS);
		}
		tbl++;
	}
	return (FAILURE);
}

static BOOL num2proc(void *func, const PROCTBL *tbl, int size) {
	int i;
	VAEG_INTPTR *slot;

	slot = (VAEG_INTPTR *)func;
	for (i = 0; i < size; i++) {
		if (*slot == (VAEG_INTPTR)tbl->id) {
			*slot = (VAEG_INTPTR)tbl->proc;
			return (SUCCESS);
		}
		tbl++;
	}
	return (FAILURE);
}

// ----

enum {
	SFFILEH_WRITE = 0x0001,
	SFFILEH_BLOCK = 0x0002,
	SFFILEH_ERROR = 0x0004
};

typedef struct {
	_STFLAGH sfh;
	UINT stat;
	FILEH fh;
	UINT secpos;
	NP2FHDR f;
} _SFFILEH, *SFFILEH;

static SFFILEH statflag_open(const char *filename, char *err, int errlen) {
	FILEH fh;
	SFFILEH ret;

	fh = file_open_rb(filename);
	if (fh == FILEH_INVALID) {
		goto sfo_err1;
	}
	ret = (SFFILEH)_MALLOC(sizeof(_SFFILEH), filename);
	if (ret == NULL) {
		goto sfo_err2;
	}
	if ((file_read(fh, &ret->f, sizeof(NP2FHDR)) == sizeof(NP2FHDR)) &&
	    (!memcmp(&ret->f, &np2flagdef, sizeof(np2flagdef)))) {
		ZeroMemory(ret, sizeof(_SFFILEH));
		ret->fh = fh;
		ret->secpos = sizeof(NP2FHDR);
		if ((err) && (errlen > 0)) {
			err[0] = '\0';
			ret->sfh.err = err;
			ret->sfh.errlen = errlen;
		}
		return (ret);
	}
	_MFREE(ret);

sfo_err2:
	file_close(fh);

sfo_err1:
	if (err && errlen > 0) {
		milstr_ncpy(err, "M142_STATE_FORMAT_OR_OPEN", errlen);
	}
	return (NULL);
}

static int statflag_closesection(SFFILEH sffh) {
	UINT leng;
	BYTE zero[16];

	if (sffh == NULL) {
		goto sfcs_err1;
	}
	if (sffh->stat == (SFFILEH_BLOCK | SFFILEH_WRITE)) {
		leng = (0 - sffh->sfh.hdr.size) & 15;
		if (leng) {
			ZeroMemory(zero, sizeof(zero));
			if (file_write(sffh->fh, zero, leng) != leng) {
				goto sfcs_err2;
			}
		}
		if ((file_seek(sffh->fh, (long)sffh->secpos, FSEEK_SET) != (long)sffh->secpos) ||
		    (file_write(sffh->fh, &sffh->sfh.hdr, sizeof(sffh->sfh.hdr)) !=
		     sizeof(sffh->sfh.hdr))) {
			goto sfcs_err2;
		}
	}
	if (sffh->stat & SFFILEH_BLOCK) {
		sffh->stat &= ~SFFILEH_BLOCK;
		sffh->secpos += sizeof(sffh->sfh.hdr) + ((sffh->sfh.hdr.size + 15) & (~15));
		if (file_seek(sffh->fh, (long)sffh->secpos, FSEEK_SET) != (long)sffh->secpos) {
			goto sfcs_err2;
		}
	}
	return (STATFLAG_SUCCESS);

sfcs_err2:
	sffh->stat = SFFILEH_ERROR;

sfcs_err1:
	return (STATFLAG_FAILURE);
}

static int statflag_readsection(SFFILEH sffh) {
	int ret;

	ret = statflag_closesection(sffh);
	if (ret != STATFLAG_SUCCESS) {
		return (ret);
	}
	if ((sffh->stat == 0) &&
	    (file_read(sffh->fh, &sffh->sfh.hdr, sizeof(sffh->sfh.hdr)) == sizeof(sffh->sfh.hdr))) {
		UINT64 end = (UINT64)sffh->secpos + sizeof(sffh->sfh.hdr) +
		             (((UINT64)sffh->sfh.hdr.size + 15) & ~(UINT64)15);
		if (end > file_getsize64(sffh->fh) || end > INT32_MAX) {
			sffh->stat = SFFILEH_ERROR;
			return STATFLAG_FAILURE;
		}
		sffh->stat = SFFILEH_BLOCK;
		sffh->sfh.pos = 0;
		return (STATFLAG_SUCCESS);
	}
	sffh->stat = SFFILEH_ERROR;
	return (STATFLAG_FAILURE);
}

int statflag_read(STFLAGH sfh, void *buf, UINT size) {
	if ((sfh == NULL) || (buf == NULL) || ((sfh->pos + size) > sfh->hdr.size)) {
		goto sfr_err;
	}
	if (size) {
		if (file_read(((SFFILEH)sfh)->fh, buf, size) != size) {
			goto sfr_err;
		}
		sfh->pos += size;
	}
	return (STATFLAG_SUCCESS);

sfr_err:
	return (STATFLAG_FAILURE);
}

static SFFILEH statflag_create(const char *filename) {
	SFFILEH ret;
	FILEH fh;

	ret = (SFFILEH)_MALLOC(sizeof(_SFFILEH), filename);
	if (ret == NULL) {
		goto sfc_err1;
	}
	fh = file_create(filename);
	if (fh == FILEH_INVALID) {
		goto sfc_err2;
	}
	if (file_write(fh, &np2flagdef, sizeof(NP2FHDR)) == sizeof(NP2FHDR)) {
		ZeroMemory(ret, sizeof(_SFFILEH));
		ret->stat = SFFILEH_WRITE;
		ret->fh = fh;
		ret->secpos = sizeof(NP2FHDR);
		return (ret);
	}
	file_close(fh);
	file_delete(filename);

sfc_err2:
	_MFREE(ret);

sfc_err1:
	return (NULL);
}

static int statflag_createsection(SFFILEH sffh, const SFENTRY *tbl) {
	int ret;

	ret = statflag_closesection(sffh);
	if (ret != STATFLAG_SUCCESS) {
		return (ret);
	}
	if (sffh->stat != SFFILEH_WRITE) {
		sffh->stat = SFFILEH_ERROR;
		return (STATFLAG_FAILURE);
	}
	CopyMemory(sffh->sfh.hdr.index, tbl->index, sizeof(sffh->sfh.hdr.index));
	sffh->sfh.hdr.ver = tbl->ver;
	sffh->sfh.hdr.size = 0;
	return (STATFLAG_SUCCESS);
}

int statflag_write(STFLAGH sfh, const void *buf, UINT size) {
	SFFILEH sffh;

	if (sfh == NULL) {
		goto sfw_err1;
	}
	sffh = (SFFILEH)sfh;
	if (!(sffh->stat & SFFILEH_WRITE)) {
		goto sfw_err2;
	}
	if (!(sffh->stat & SFFILEH_BLOCK)) {
		sffh->stat |= SFFILEH_BLOCK;
		sfh->pos = 0;
		if (file_write(sffh->fh, &sfh->hdr, sizeof(sfh->hdr)) != sizeof(sfh->hdr)) {
			goto sfw_err2;
		}
	}
	if (size) {
		if ((buf == NULL) || (file_write(sffh->fh, buf, size) != size)) {
			goto sfw_err2;
		}
		sfh->pos += size;
		if (sfh->hdr.size < sfh->pos) {
			sfh->hdr.size = sfh->pos;
		}
	}
	return (STATFLAG_SUCCESS);

sfw_err2:
	sffh->stat = SFFILEH_ERROR;

sfw_err1:
	return (STATFLAG_FAILURE);
}

static void statflag_close(SFFILEH sffh) {
	if (sffh) {
		statflag_closesection(sffh);
		file_close(sffh->fh);
		_MFREE(sffh);
	}
}

void statflag_seterr(STFLAGH sfh, const char *str) {
	if ((sfh) && (sfh->errlen)) {
		milstr_ncat(sfh->err, str, sfh->errlen);
		milstr_ncat(sfh->err, CRCONST, sfh->errlen);
	}
}

// ---- common

static int flagsave_common(STFLAGH sfh, const SFENTRY *tbl) {
	return (statflag_write(sfh, tbl->arg1, tbl->arg2));
}

static int flagsave_upd8087(STFLAGH sfh, const SFENTRY *tbl) {
	BYTE payload[UPD8087_STATE_IMAGE_SIZE];

	if (!upd8087_store_machine_state(&upd8087, payload, sizeof(payload))) {
		return (STATFLAG_FAILURE);
	}
	return (statflag_write(sfh, payload, tbl->arg2));
}

static int flagcheck_upd8087(STFLAGH sfh, const SFENTRY *tbl) {
	BYTE payload[UPD8087_STATE_IMAGE_SIZE];
	UPD8087_STATE decoded;

	if ((sfh->hdr.ver != tbl->ver) || (sfh->hdr.size != tbl->arg2) ||
	    (statflag_read(sfh, payload, sizeof(payload)) != STATFLAG_SUCCESS) ||
	    !upd8087_load_machine_state(&decoded, payload, sizeof(payload))) {
		statflag_seterr(sfh, "8087 state payload is invalid or truncated");
		return (STATFLAG_FAILURE);
	}
	return (STATFLAG_SUCCESS);
}

static int flagload_upd8087(STFLAGH sfh, const SFENTRY *tbl) {
	BYTE payload[UPD8087_STATE_IMAGE_SIZE];

	if ((sfh->hdr.ver != tbl->ver) || (sfh->hdr.size != tbl->arg2) ||
	    (statflag_read(sfh, payload, sizeof(payload)) != STATFLAG_SUCCESS) ||
	    !upd8087_load_machine_state(&upd8087, payload, sizeof(payload))) {
		statflag_seterr(sfh, "8087 state payload is invalid or truncated");
		return (STATFLAG_FAILURE);
	}
	return (STATFLAG_SUCCESS);
}

static int flagload_common(STFLAGH sfh, const SFENTRY *tbl) {
	return (statflag_read(sfh, tbl->arg1, tbl->arg2));
}

static BOOL statflag_index_equals(const char index[10], const char *name) {
	char padded[10];
	size_t length;

	ZeroMemory(padded, sizeof(padded));
	length = strlen(name);
	if (length > sizeof(padded)) {
		length = sizeof(padded);
	}
	CopyMemory(padded, name, length);
	return (!memcmp(index, padded, sizeof(padded)));
}

static BOOL statflag_is_legacy_cpu_state(STFLAGH sfh) {
	static const char legacy_cpu_state_section[] = {'C', 'P', 'U', '2', '8', '6', '\0'};

	return (statflag_index_equals(sfh->hdr.index, legacy_cpu_state_section));
}

static BOOL statflag_is_upd9002_format_marker(STFLAGH sfh) {
	return (statflag_index_equals(sfh->hdr.index, "UPD9002"));
}

static int flagsave_upd9002_cpu(STFLAGH sfh, const SFENTRY *tbl) {
	Upd9002StateImage state;

	upd9002_state_export(&state);
	return (statflag_write(sfh, &state, tbl->arg2));
}

static int flagload_upd9002_state_image(STFLAGH sfh, UINT16 version, UINT32 payload_size,
                                        const char *truncated_error) {
	Upd9002StateImage state;
	char error[128];
	int ret;

	if ((sfh->hdr.ver != version) || (sfh->hdr.size != payload_size)) {
		statflag_seterr(sfh, UPD9002_STATE_ERROR_SIZE);
		return (STATFLAG_FAILURE);
	}
	ret = statflag_read(sfh, &state, sizeof(state));
	if (ret != STATFLAG_SUCCESS) {
		statflag_seterr(sfh, truncated_error);
		return (STATFLAG_FAILURE);
	}
	error[0] = '\0';
	if (upd9002_state_import(&state, sizeof(state), error, sizeof(error)) != SUCCESS) {
		statflag_seterr(sfh, error);
		return (STATFLAG_FAILURE);
	}
	return (STATFLAG_SUCCESS);
}

static int flagload_upd9002_cpu(STFLAGH sfh, const SFENTRY *tbl) {
	upd9002_core_queue_reset();
	return (flagload_upd9002_state_image(sfh, tbl->ver, tbl->arg2, "uPD9002 payload is truncated"));
}

static int flagsave_upd9002_compat(STFLAGH sfh, const SFENTRY *tbl) {
	UINT8 state[UPD9002_COMPAT_STATE_SIZE];

	if (upd9002_core_compat_state_save(state, sizeof(state)) != SUCCESS) {
		return STATFLAG_FAILURE;
	}
	return statflag_write(sfh, state, tbl->arg2);
}

static int flagload_upd9002_compat(STFLAGH sfh, const SFENTRY *tbl) {
	UINT8 state[UPD9002_COMPAT_STATE_SIZE];

	if ((sfh->hdr.ver != tbl->ver) || (sfh->hdr.size != tbl->arg2) ||
	    (statflag_read(sfh, state, sizeof(state)) != STATFLAG_SUCCESS)) {
		statflag_seterr(sfh, "uPD9002 uPD70008-compatible payload is invalid or truncated");
		return STATFLAG_FAILURE;
	}
	return upd9002_core_compat_state_load(state, sizeof(state)) == SUCCESS ? STATFLAG_SUCCESS
	                                                                       : STATFLAG_FAILURE;
}

static int flagsave_upd9002_mode(STFLAGH sfh, const SFENTRY *tbl) {
	UINT8 state[UPD9002_MODE_STATE_SIZE];

	upd9002_core_mode_state_save(state);
	return statflag_write(sfh, state, tbl->arg2);
}

static int flagread_upd9002_mode(STFLAGH sfh, const SFENTRY *tbl, UINT8 *state) {
	if ((sfh->hdr.ver != tbl->ver) || (sfh->hdr.size != tbl->arg2) ||
	    (statflag_read(sfh, state, UPD9002_MODE_STATE_SIZE) != STATFLAG_SUCCESS) ||
	    !upd9002_core_mode_state_valid(state)) {
		statflag_seterr(sfh, "uPD9002 mode-transition payload is invalid or truncated");
		return STATFLAG_FAILURE;
	}
	return STATFLAG_SUCCESS;
}

static int flagload_upd9002_mode(STFLAGH sfh, const SFENTRY *tbl) {
	UINT8 state[UPD9002_MODE_STATE_SIZE];

	if (flagread_upd9002_mode(sfh, tbl, state) != STATFLAG_SUCCESS) {
		return STATFLAG_FAILURE;
	}
	return upd9002_core_mode_state_load(state) == SUCCESS ? STATFLAG_SUCCESS : STATFLAG_FAILURE;
}

/* Keep UPD9Z80's layout/name. This additional section identifies which main
 * CPU owns that payload. Old files without it retain current-machine selection. */
static int flagsave_main_cpu(STFLAGH sfh) {
	const UINT8 role = pccore_main_cpu_is_upd780() ? 1 : 0;
	return statflag_write(sfh, &role, sizeof(role));
}

static int flagcheck_main_cpu(STFLAGH sfh, const SFENTRY *tbl) {
	UINT8 role;
	if ((sfh->hdr.ver != tbl->ver) || (sfh->hdr.size != 1) ||
	    (statflag_read(sfh, &role, sizeof(role)) != STATFLAG_SUCCESS) || role > 1) {
		statflag_seterr(sfh, "M139_MAIN_CPU_STATE_PAYLOAD");
		return STATFLAG_FAILURE;
	}
	if (role != (pccore_main_cpu_is_upd780() ? 1 : 0)) {
		statflag_seterr(sfh, "M139_MAIN_CPU_STATE_MACHINE_MISMATCH");
		return STATFLAG_FAILURE;
	}
	return STATFLAG_SUCCESS;
}

/* A nonzero UPD9Z80 payload is written only while compatible code is active. */
static int flagcheck_upd9002_compat(STFLAGH sfh, const SFENTRY *tbl, BOOL *active) {
	UINT8 state[UPD9002_COMPAT_STATE_SIZE];
	UINT i;

	if ((sfh->hdr.ver != tbl->ver) || (sfh->hdr.size != tbl->arg2) ||
	    (statflag_read(sfh, state, sizeof(state)) != STATFLAG_SUCCESS)) {
		statflag_seterr(sfh, "uPD9002 uPD70008-compatible payload is invalid or truncated");
		return STATFLAG_FAILURE;
	}
	for (i = 0; i < sizeof(state); i++) {
		if (state[i] != 0) {
			*active = TRUE;
		}
	}
	return STATFLAG_SUCCESS;
}

static int flagload_legacy_cpu_state(STFLAGH sfh) {
	return (flagload_upd9002_state_image(sfh, 0, UPD9002_STATE_PAYLOAD_SIZE,
	                                     "obsolete processor state section is truncated"));
}

// ---- memory

static int flagload_upd9002_alt(STFLAGH sfh, const SFENTRY *tbl) {
	if ((sfh->hdr.ver != tbl->ver) || (sfh->hdr.size != tbl->arg2) ||
	    (statflag_read(sfh, &upd9002_alt_regs, sizeof(upd9002_alt_regs)) != STATFLAG_SUCCESS)) {
		statflag_seterr(sfh, "uPD9002 alternate-register payload is invalid or truncated");
		return STATFLAG_FAILURE;
	}
	/* The section precedes the compat blob; its loader must let the
	 * machine-owned storage win over the blob's working copy. */
	upd9002_alt_regs_loaded = TRUE;
	return STATFLAG_SUCCESS;
}

static int flagsave_mem(STFLAGH sfh, const SFENTRY *tbl) {
	int ret;

	ret = statflag_write(sfh, mem, 0x110000);
	(void)tbl;
	return (ret);
}

static int flagload_mem(STFLAGH sfh, const SFENTRY *tbl) {
	int ret;

	ret = statflag_read(sfh, mem, 0x110000);
	(void)tbl;
	return (ret);
}

// ---- dma

static int flagsave_dma(STFLAGH sfh, const SFENTRY *tbl) {
	int i;
	_DMAC dmabak;

	dmabak = dmac;
	for (i = 0; i < 4; i++) {
		if ((PROC2NUM(dmabak.dmach[i].proc.outproc, dmaproc)) ||
		    (PROC2NUM(dmabak.dmach[i].proc.inproc, dmaproc)) ||
		    (PROC2NUM(dmabak.dmach[i].proc.extproc, dmaproc))) {
			return (STATFLAG_FAILURE);
		}
	}
	(void)tbl;
	return (statflag_write(sfh, &dmabak, sizeof(dmabak)));
}

static int flagload_dma(STFLAGH sfh, const SFENTRY *tbl) {
	int ret;
	int i;

	ret = statflag_read(sfh, &dmac, sizeof(dmac));

	for (i = 0; i < 4; i++) {
		if (NUM2PROC(dmac.dmach[i].proc.outproc, dmaproc)) {
			dmac.dmach[i].proc.outproc = dma_dummyout;
			ret |= STATFLAG_WARNING;
		}
		if (NUM2PROC(dmac.dmach[i].proc.inproc, dmaproc)) {
			dmac.dmach[i].proc.inproc = dma_dummyin;
			ret |= STATFLAG_WARNING;
		}
		if (NUM2PROC(dmac.dmach[i].proc.extproc, dmaproc)) {
			dmac.dmach[i].proc.extproc = dma_dummyproc;
			ret |= STATFLAG_WARNING;
		}
	}
	(void)tbl;
	return (ret);
}

// ---- event

typedef struct {
	UINT readyevents;
	UINT waitevents;
} NEVTSAVE;

typedef struct {
	UINT32 id;
	SINT32 clock;
	UINT32 flag;
	NEVENTCB proc;
} NEVTITEM;

static int nevent_write(STFLAGH sfh, int num) {
	NEVTITEM nit;
	UINT i;

	ZeroMemory(&nit, sizeof(nit));
	for (i = 0; i < sizeof(evtnum) / sizeof(ENUMTBL); i++) {
		if (evtnum[i].num == num) {
			nit.id = evtnum[i].id;
			break;
		}
	}
	nit.clock = nevent.item[num].clock;
	nit.flag = nevent.item[num].flag;
	nit.proc = nevent.item[num].proc;
	if (PROC2NUM(nit.proc, evtproc)) {
		nit.proc = NULL;
	}
	return (statflag_write(sfh, &nit, sizeof(nit)));
}

static int flagsave_evt(STFLAGH sfh, const SFENTRY *tbl) {
	NEVTSAVE nevt;
	int ret;
	UINT i;

	nevt.readyevents = nevent.readyevents;
	nevt.waitevents = nevent.waitevents;

	ret = statflag_write(sfh, &nevt, sizeof(nevt));
	for (i = 0; i < nevt.readyevents; i++) {
		ret |= nevent_write(sfh, nevent.level[i]);
	}
	for (i = 0; i < nevt.waitevents; i++) {
		ret |= nevent_write(sfh, nevent.waitevent[i]);
	}
	(void)tbl;
	return (ret);
}

static int nevent_read(STFLAGH sfh, UINT *tbl, UINT *pos) {
	int ret;
	NEVTITEM nit;
	UINT i;
	UINT num;

	ret = statflag_read(sfh, &nit, sizeof(nit));

	for (i = 0; i < sizeof(evtnum) / sizeof(ENUMTBL); i++) {
		if (nit.id == evtnum[i].id) {
			break;
		}
	}
	if (i < (sizeof(evtnum) / sizeof(ENUMTBL))) {
		num = evtnum[i].num;
		nevent.item[num].clock = nit.clock;
		nevent.item[num].flag = nit.flag;
		nevent.item[num].proc = nit.proc;
		if (NUM2PROC(nevent.item[num].proc, evtproc)) {
			ret |= STATFLAG_WARNING;
		} else {
			tbl[*pos] = num;
			(*pos)++;
		}
	} else {
		ret |= STATFLAG_WARNING;
	}
	return (ret);
}

static int flagload_evt(STFLAGH sfh, const SFENTRY *tbl) {
	int ret;
	NEVTSAVE nevt;
	UINT i;

	ret = statflag_read(sfh, &nevt, sizeof(nevt));

	nevent.readyevents = 0;
	nevent.waitevents = 0;

	for (i = 0; i < nevt.readyevents; i++) {
		ret |= nevent_read(sfh, nevent.level, &nevent.readyevents);
	}
	for (i = 0; i < nevt.waitevents; i++) {
		ret |= nevent_read(sfh, nevent.waitevent, &nevent.waitevents);
	}
	(void)tbl;
	return (ret);
}

// ---- extmem

static int flagsave_ext(STFLAGH sfh, const SFENTRY *tbl) {
	int ret;

	ret = STATFLAG_SUCCESS;
	if (CPU_EXTMEM) {
		ret = statflag_write(sfh, CPU_EXTMEM, CPU_EXTMEMSIZE);
	}
	(void)tbl;
	return (ret);
}

static int flagload_ext(STFLAGH sfh, const SFENTRY *tbl) {
	int ret;

	ret = STATFLAG_SUCCESS;
	if (CPU_EXTMEM) {
		ret = statflag_read(sfh, CPU_EXTMEM, CPU_EXTMEMSIZE);
	}
	(void)tbl;
	return (ret);
}

// ---- FM

enum {
	FLAG_FM1A = 0x0002,
	FLAG_FM1B = 0x0004,
	FLAG_FM2A = 0x0008,
	FLAG_FM2B = 0x0010,
	FLAG_PSG1 = 0x0020,
	FLAG_PSG2 = 0x0040,
	FLAG_PSG3 = 0x0080,
	FLAG_RHYTHM = 0x0100,
	FLAG_ADPCM = 0x0200,
	FLAG_YMFM = 0x0400,
	FLAG_FMBOARDVA = 0x8000,
};

typedef struct {
	BYTE keyreg[OPNCH_MAX];
	BYTE extop[4];
} OPNKEY;

static BYTE *fm_backend_restore;
static UINT32 fm_backend_restore_size;

static UINT fm_save_flags(UINT32 type) {
	switch (type) {
	case FMBOARD_VA_OPN:
		return (FLAG_FM1A | FLAG_PSG1 | FLAG_YMFM | FLAG_FMBOARDVA);

	case FMBOARD_VA_OPNA:
		return (FLAG_FM1A | FLAG_FM1B | FLAG_PSG1 | FLAG_RHYTHM | FLAG_ADPCM | FLAG_YMFM |
		        FLAG_FMBOARDVA);

	default:
		return (0);
	}
}

static UINT32 fm_state_size(UINT32 type) {
	UINT32 size;

	size = sizeof(type);
	switch (type) {
	case FMBOARD_NONE:
		break;

	case FMBOARD_VA_OPN:
	case FMBOARD_VA_OPNA:
		size += sizeof(fmtimer) + sizeof(opn) + sizeof(OPNKEY) + sizeof(PSGREG) +
		        sizeof(fmboardva) + sizeof(UINT32) + ymfm_opn_state_size();
		if (type == FMBOARD_VA_OPNA) {
			size += sizeof(adpcm);
		}
		break;

	default:
		size = 0;
		break;
	}
	return (size);
}

static int flagcheck_fm(STFLAGH sfh, const SFENTRY *tbl) {
	UINT32 type;
	UINT32 expected;

	if (sfh->hdr.ver != tbl->ver) {
		statflag_seterr(sfh, "FMBOARD state version is unsupported");
		return (STATFLAG_FAILURE);
	}
	if ((sfh->hdr.size < sizeof(type)) ||
	    (statflag_read(sfh, &type, sizeof(type)) != STATFLAG_SUCCESS)) {
		statflag_seterr(sfh, "FMBOARD state is truncated");
		return (STATFLAG_FAILURE);
	}
	expected = fm_state_size(type);
	if (expected == 0) {
		statflag_seterr(sfh, "FMBOARD state uses retired sound hardware");
		return (STATFLAG_FAILURE);
	}
	if (sfh->hdr.size != expected) {
		statflag_seterr(sfh, "FMBOARD state payload size is unsupported");
		return (STATFLAG_FAILURE);
	}
	return (STATFLAG_SUCCESS);
}

static int flagsave_fm(STFLAGH sfh, const SFENTRY *tbl) {
	int ret;
	UINT saveflg;
	UINT32 backend_size;
	BYTE *backend_state;
	OPNKEY opnkey;

	saveflg = fm_save_flags(usesound);

	ret = statflag_write(sfh, &usesound, sizeof(usesound));
	if (saveflg & FLAG_FM1A) {
		ret |= statflag_write(sfh, &fmtimer, sizeof(fmtimer));
		ret |= statflag_write(sfh, &opn, sizeof(opn));
		CopyMemory(opnkey.keyreg, opngen.keyreg, sizeof(opngen.keyreg));
		opnkey.extop[0] = opnch[2].extop;
		opnkey.extop[1] = opnch[5].extop;
		opnkey.extop[2] = opnch[8].extop;
		opnkey.extop[3] = opnch[11].extop;
		ret |= statflag_write(sfh, &opnkey, sizeof(opnkey));
	}
	if (saveflg & FLAG_PSG1) {
		ret |= statflag_write(sfh, &psg1.reg, sizeof(PSGREG));
	}
	if (saveflg & FLAG_PSG2) {
		ret |= statflag_write(sfh, &psg2.reg, sizeof(PSGREG));
	}
	if (saveflg & FLAG_PSG3) {
		ret |= statflag_write(sfh, &psg3.reg, sizeof(PSGREG));
	}
	if (saveflg & FLAG_ADPCM) {
		ret |= statflag_write(sfh, &adpcm, sizeof(adpcm));
	}
	if (saveflg & FLAG_FMBOARDVA) {
		ret |= statflag_write(sfh, &fmboardva, sizeof(fmboardva));
	}
	if (saveflg & FLAG_YMFM) {
		backend_size = ymfm_opn_state_size();
		ret |= statflag_write(sfh, &backend_size, sizeof(backend_size));
		backend_state = (BYTE *)_MALLOC(backend_size, "ymfm-state-save");
		if ((backend_state == NULL) ||
		    (ymfm_opn_state_save(backend_state, backend_size) != SUCCESS)) {
			ret = STATFLAG_FAILURE;
		} else {
			ret |= statflag_write(sfh, backend_state, backend_size);
		}
		if (backend_state != NULL) {
			_MFREE(backend_state);
		}
	}
	(void)tbl;
	return (ret);
}

static int flagload_fm(STFLAGH sfh, const SFENTRY *t) {
	int ret;
	UINT saveflg;
	UINT32 backend_size;
	UINT32 expected_backend_size;
	OPNKEY opnkey;

	fm_backend_restore = NULL;
	fm_backend_restore_size = 0;
	ret = statflag_read(sfh, &usesound, sizeof(usesound));
	fmboard_reset(usesound);
	saveflg = fm_save_flags(usesound);

	if (saveflg & FLAG_FM1A) {
		ret |= statflag_read(sfh, &fmtimer, sizeof(fmtimer));
		ret |= statflag_read(sfh, &opn, sizeof(opn));
		ret |= statflag_read(sfh, &opnkey, sizeof(opnkey));
		CopyMemory(opngen.keyreg, &opnkey.keyreg, sizeof(opngen.keyreg));
		opngen_setextch(2, opnkey.extop[0]);
		opngen_setextch(5, opnkey.extop[1]);
		opngen_setextch(8, opnkey.extop[2]);
		opngen_setextch(11, opnkey.extop[3]);
	}
	if (saveflg & FLAG_PSG1) {
		ret |= statflag_read(sfh, &psg1.reg, sizeof(PSGREG));
	}
	if (saveflg & FLAG_PSG2) {
		ret |= statflag_read(sfh, &psg2.reg, sizeof(PSGREG));
	}
	if (saveflg & FLAG_PSG3) {
		ret |= statflag_read(sfh, &psg3.reg, sizeof(PSGREG));
	}
	if (saveflg & FLAG_ADPCM) {
		ret |= statflag_read(sfh, &adpcm, sizeof(adpcm));
	}
	if (saveflg & FLAG_FMBOARDVA) {
		ret |= statflag_read(sfh, &fmboardva, sizeof(fmboardva));
	}
	if (saveflg & FLAG_YMFM) {
		expected_backend_size = ymfm_opn_state_size();
		ret |= statflag_read(sfh, &backend_size, sizeof(backend_size));
		if (backend_size != expected_backend_size) {
			statflag_seterr(sfh, "FMBOARD YMFM state payload size is unsupported");
			ret = STATFLAG_FAILURE;
		} else {
			fm_backend_restore = (BYTE *)_MALLOC(backend_size, "ymfm-state-load");
			if ((fm_backend_restore == NULL) ||
			    (statflag_read(sfh, fm_backend_restore, backend_size) != STATFLAG_SUCCESS)) {
				if (fm_backend_restore != NULL) {
					_MFREE(fm_backend_restore);
				}
				fm_backend_restore = NULL;
				ret = STATFLAG_FAILURE;
			} else {
				fm_backend_restore_size = backend_size;
			}
		}
	}

	// Recompute ADPCM derived state after restoring its register image.
	adpcm_update(&adpcm);
	(void)t;
	return (ret);
}

// ---- disk

enum {
	STATDISK_PATH_BYTES = STATSAVE_DISK_PATH_BYTES,
	STATDISK_RECORD_BYTES = STATSAVE_DISK_RECORD_BYTES,
	STATDISK_COUNT = STATSAVE_DISK_COUNT
};

typedef struct {
	char path[STATDISK_PATH_BYTES];
	int readonly;
	UINT64 size;
	BYTE identity[32];
} STATDISK;

static STATSAVE_DISK_RESOLVER disk_resolver;

STATSAVE_DISK_RESOLVER statsave_set_disk_resolver(STATSAVE_DISK_RESOLVER resolver) {
	STATSAVE_DISK_RESOLVER previous = disk_resolver;
	disk_resolver = resolver;
	return previous;
}

/* v2: path[4096], flags/u32, file size/u64, SHA-256[32], reserved[20].
 * Timestamps and native structures are not media identities. */
static UINT diskrecord_size(STFLAGH sfh) {
	if (sfh->hdr.ver != 2) {
		statflag_seterr(sfh, "M142_DISK_VERSION");
		return 0;
	}
	if (sfh->hdr.size != STATDISK_COUNT * STATDISK_RECORD_BYTES) {
		statflag_seterr(sfh, "M142_DISK_SIZE");
		return 0;
	}
	return STATDISK_RECORD_BYTES;
}

static int diskrecord_read(STFLAGH sfh, STATDISK *st) {
	BYTE wire[STATDISK_RECORD_BYTES];
	UINT size = diskrecord_size(sfh);
	UINT i;
	const BYTE *nul;

	ZeroMemory(st, sizeof(*st));
	if (!size || statflag_read(sfh, wire, size) != STATFLAG_SUCCESS)
		return STATFLAG_FAILURE;
	nul = (const BYTE *)memchr(wire, 0, STATDISK_PATH_BYTES);
	if (!nul) {
		statflag_seterr(sfh, "M142_DISK_PATH");
		return STATFLAG_FAILURE;
	}
	for (i = (UINT)(nul - wire); i < STATDISK_PATH_BYTES; i++) {
		if (wire[i]) {
			statflag_seterr(sfh, "M142_DISK_PATH");
			return STATFLAG_FAILURE;
		}
	}
	CopyMemory(st->path, wire, STATDISK_PATH_BYTES);
	st->readonly = (int)LOADINTELDWORD(wire + 4096);
	st->size = LOADINTELDWORD(wire + 4100) | ((UINT64)LOADINTELDWORD(wire + 4104) << 32);
	CopyMemory(st->identity, wire + 4108, sizeof(st->identity));
	for (i = st->path[0] ? 4140 : 4096; i < sizeof(wire); i++) {
		if (wire[i]) {
			statflag_seterr(sfh, "M142_DISK_FLAGS");
			return STATFLAG_FAILURE;
		}
	}
	if (st->readonly & ~0xff01) {
		statflag_seterr(sfh, "M142_DISK_FLAGS");
		return STATFLAG_FAILURE;
	}
	return STATFLAG_SUCCESS;
}

static BOOL diskrecord_resolve(STATDISK *st) {
	char local[STATDISK_PATH_BYTES];

	if (st->path[0] && disk_resolver && disk_resolver(st->path, local, sizeof(local)))
		file_cpyname(st->path, local, sizeof(st->path));
	/* Do not truncate a foreign path into a different locally valid name. */
	return strlen(st->path) < MAX_PATH;
}

static int diskrecord_identify(const char *path, UINT64 *size, BYTE identity[32]) {
	BYTE buffer[65536];
	UINT64 total = 0;
	UINT got;
	FILEH file = file_open_rb(path);
	VAEG_SHA256 sha;
	if (file == FILEH_INVALID) return STATFLAG_FAILURE;
	*size = file_getsize64(file);
	vaeg_sha256_initialize(&sha);
	while ((got = file_read(file, buffer, sizeof(buffer))) != 0) {
		total += got;
		vaeg_sha256_update(&sha, buffer, got);
	}
	file_close(file);
	if (total != *size) return STATFLAG_FAILURE;
	vaeg_sha256_finish(&sha, identity);
	return STATFLAG_SUCCESS;
}

static int diskrecord_validate(STFLAGH sfh, STATDISK *st, UINT slot) {
	UINT64 size;
	BYTE identity[32];
	if (!st->path[0]) return STATFLAG_SUCCESS;
	if (slot >= 4 && st->readonly) {
		statflag_seterr(sfh, "M142_DISK_FLAGS");
		return STATFLAG_FAILURE;
	}
	if (!diskrecord_resolve(st) ||
	    diskrecord_identify(st->path, &size, identity) != STATFLAG_SUCCESS) {
		statflag_seterr(sfh, "M142_DISK_MISSING");
		return STATFLAG_FAILURE;
	}
	if (size != st->size || memcmp(identity, st->identity, sizeof(identity))) {
		statflag_seterr(sfh, "M142_DISK_IDENTITY");
		return STATFLAG_FAILURE;
	}
	if ((slot < 4 ? fdd_validate(st->path, (st->readonly >> 8) & 0xff, st->readonly & 1) :
	     slot < 6 ? sxsi_hddvalidate_sasi(st->path) : sxsi_hddvalidate_scsi(st->path)) != SUCCESS) {
		statflag_seterr(sfh, "M142_DISK_MEDIA");
		return STATFLAG_FAILURE;
	}
	return STATFLAG_SUCCESS;
}

#if defined(SUPPORT_OPRECORD)
STATSAVE_LOAD_DISK_HOOK flagload_disk_hook = NULL;
#endif

static int disksave(STFLAGH sfh, const char *path, int readonly) {
	STATDISK st;
	BYTE wire[STATDISK_RECORD_BYTES];

	ZeroMemory(&st, sizeof(st));
	ZeroMemory(wire, sizeof(wire));
	if (path && path[0]) {
		if (strlen(path) >= STATDISK_PATH_BYTES) {
			statflag_seterr(sfh, "M142_DISK_PATH");
			return STATFLAG_FAILURE;
		}
		file_cpyname(st.path, path, sizeof(st.path));
		st.readonly = readonly;
		if (diskrecord_identify(path, &st.size, st.identity) != STATFLAG_SUCCESS) {
			statflag_seterr(sfh, "M142_DISK_MISSING");
			return STATFLAG_FAILURE;
		}
	}
	CopyMemory(wire, st.path, STATDISK_PATH_BYTES);
	STOREINTELDWORD(wire + 4096, st.readonly);
	STOREINTELDWORD(wire + 4100, (UINT32)st.size);
	STOREINTELDWORD(wire + 4104, (UINT32)(st.size >> 32));
	CopyMemory(wire + 4108, st.identity, sizeof(st.identity));
	return statflag_write(sfh, wire, sizeof(wire));
}

static int flagsave_disk(STFLAGH sfh, const SFENTRY *tbl) {
	int ret;
	BYTE i;

	if (fdd_flush() != SUCCESS) {
		statflag_seterr(sfh, "M142_DISK_FLUSH");
		return STATFLAG_FAILURE;
	}
	sxsi_flash();
	ret = STATFLAG_SUCCESS;
	/* Bits 15-8 of flags hold the selected image of a multi-image D88. */
	for (i = 0; i < 4; i++) {
		ret |= disksave(sfh, fdd_diskname(i),
		                (fdd_diskprotect(i) ? 1 : 0) | (int)(fdd_diskimage(i) << 8));
	}
	for (i = 0x00; i < 0x02; i++) {
		ret |= disksave(sfh, sxsi_getname(i), 0);
	}
	for (i = 0x20; i < 0x20 + SCSIHDD_MAX; i++) {
		ret |= disksave(sfh, sxsi_getname(i), 0);
	}
	(void)tbl;
	return (ret);
}

static int diskcheck(STFLAGH sfh, UINT slot, STATDISK *st) {
	int ret = diskrecord_read(sfh, st);
	if (ret == STATFLAG_SUCCESS) ret = diskrecord_validate(sfh, st, slot);
	if (ret != STATFLAG_SUCCESS) {
		char name[32];
		if (slot < 4) SPRINTF(name, "FDD%u", slot + 1);
		else if (slot < 6) SPRINTF(name, "SASI%u", slot - 3);
		else SPRINTF(name, "SCSI ID %u", slot - 6);
		statflag_seterr(sfh, name);
	}
	return ret;
}

static int flagcheck_disk(STFLAGH sfh, const SFENTRY *tbl) {
	UINT i;
	STATDISK st;
	(void)tbl;
	if (!diskrecord_size(sfh)) return STATFLAG_FAILURE;
	if (fdd_flush() != SUCCESS) {
		statflag_seterr(sfh, "M142_DISK_FLUSH");
		return STATFLAG_FAILURE;
	}
	sxsi_flash();
	for (i = 0; i < STATDISK_COUNT; i++)
		if (diskcheck(sfh, i, &st) != STATFLAG_SUCCESS) return STATFLAG_FAILURE;
	return STATFLAG_SUCCESS;
}

static int flagload_disk(STFLAGH sfh, const SFENTRY *tbl) {
	STATDISK *prepared = (STATDISK *)_MALLOC(sizeof(STATDISK) * STATDISK_COUNT, "state media");
	UINT i;
	int ret = STATFLAG_FAILURE;
	(void)tbl;
	if (!prepared) return STATFLAG_FAILURE;
	/* Resolve every reference while the reader's existing mounts still exist.
	 * Never resolve drive N after changing the search context for drive N-1. */
	for (i = 0; i < STATDISK_COUNT; i++)
		if (diskcheck(sfh, i, prepared + i) != STATFLAG_SUCCESS) goto done;
	for (i = 0; i < 4; i++) {
		STATDISK *st = prepared + i;
#if defined(SUPPORT_OPRECORD)
		if (st->path[0] && flagload_disk_hook) {
			flagload_disk_hook(i, st->path, sizeof(st->path), &st->readonly);
			if (!st->path[0] || diskrecord_validate(sfh, st, i) != STATFLAG_SUCCESS) goto done;
		}
#endif
		if (st->path[0]) {
			fddfile[i].num = (BYTE)((st->readonly >> 8) & 0xff);
			if (fdd_set(i, st->path, FTYPE_NONE, st->readonly & 1) != SUCCESS) goto done;
		} else {
			fdd_eject(i);
		}
	}
	/* Empty records are an instruction to detach, not to retain stale disks. */
	sxsi_trash();
	for (i = 4; i < STATDISK_COUNT; i++) {
		UINT8 drive = (UINT8)(i < 6 ? i - 4 : 0x20 + i - 6);
		if (prepared[i].path[0] && sxsi_hddopen(drive, prepared[i].path) != SUCCESS) goto done;
	}
	ret = STATFLAG_SUCCESS;
done:
	_MFREE(prepared);
	return ret;
}

// ---- com

static int flagsave_com(STFLAGH sfh, const SFENTRY *tbl) {
	UINT device;
	COMMNG cm;
	int ret;
	COMFLAG flag;

	device = (UINT)(VAEG_INTPTR)tbl->arg1;
	switch (device) {
	case 0:
		cm = cm_mpu98;
		break;

	case 1:
		cm = cm_rs232c;
		break;

	default:
		cm = NULL;
		break;
	}
	ret = STATFLAG_SUCCESS;
	if (cm) {
		flag = (COMFLAG)cm->msg(cm, COMMSG_GETFLAG, 0);
		if (flag) {
			ret |= statflag_write(sfh, flag, flag->size);
			_MFREE(flag);
		}
	}
	return (ret);
}

static int flagload_com(STFLAGH sfh, const SFENTRY *tbl) {
	UINT device;
	COMMNG cm;
	int ret;
	_COMFLAG fhdr;
	COMFLAG flag;

	ret = statflag_read(sfh, &fhdr, sizeof(fhdr));
	if (ret != STATFLAG_SUCCESS) {
		goto flcom_err1;
	}
	if (fhdr.size < sizeof(fhdr)) {
		goto flcom_err1;
	}
	flag = (COMFLAG)_MALLOC(fhdr.size, "com stat flag");
	if (flag == NULL) {
		goto flcom_err1;
	}
	CopyMemory(flag, &fhdr, sizeof(fhdr));
	ret |= statflag_read(sfh, flag + 1, fhdr.size - sizeof(fhdr));
	if (ret != STATFLAG_SUCCESS) {
		goto flcom_err2;
	}

	device = (UINT)(VAEG_INTPTR)tbl->arg1;
	switch (device) {
	case 0:
		commng_destroy(cm_mpu98);
		cm = commng_create(COMCREATE_MPU98II);
		cm_mpu98 = cm;
		break;

	case 1:
		commng_destroy(cm_rs232c);
		cm = commng_create(COMCREATE_SERIAL);
		cm_rs232c = cm;
		break;

	default:
		cm = NULL;
		break;
	}
	if (cm) {
		cm->msg(cm, COMMSG_SETFLAG, (VAEG_INTPTR)flag);
	}

flcom_err2:
	_MFREE(flag);

flcom_err1:
	return (ret);
}

// ---- bms

enum { BMS_STATE_HEADER_SIZE = 16 };

/* Device configuration and its RAM are a single, explicit little-endian
 * section. Never infer the saved allocation from the reader's reset prefs. */
static BOOL bms_state_header_valid(const BYTE *wire, UINT32 size) {
	UINT32 bytes = wire[0] ? (UINT32)wire[1] * BMSIO_BANK_BYTES : 0;
	UINT8 nomem = wire[2] && (!wire[0] || wire[2] > wire[1]);
	return wire[0] <= 1 && wire[3] == nomem &&
	       (wire[0] ? wire[1] != 0 : wire[1] == 0) &&
	       LOADINTELWORD(wire + 6) == BMSIO_PORT_MASK &&
	       LOADINTELDWORD(wire + 8) == bytes && LOADINTELDWORD(wire + 12) == 0 &&
	       size == BMS_STATE_HEADER_SIZE + bytes;
}

static int flagcheck_bms(STFLAGH sfh, const SFENTRY *tbl) {
	BYTE wire[BMS_STATE_HEADER_SIZE];
	if (sfh->hdr.ver != tbl->ver) {
		statflag_seterr(sfh, "M142_BMS_VERSION");
		return STATFLAG_FAILURE;
	}
	if (statflag_read(sfh, wire, sizeof(wire)) != STATFLAG_SUCCESS ||
	    !bms_state_header_valid(wire, sfh->hdr.size)) {
		statflag_seterr(sfh, "M142_BMS_PAYLOAD");
		return STATFLAG_FAILURE;
	}
	return STATFLAG_SUCCESS;
}

static int flagsave_bms(STFLAGH sfh, const SFENTRY *tbl) {
	BYTE wire[BMS_STATE_HEADER_SIZE] = {0};
	UINT32 bytes;
	(void)tbl;
	wire[0] = bmsio.cfg.enabled ? 1 : 0;
	wire[1] = bmsio.cfg.numbanks;
	wire[2] = bmsio.bank;
	wire[3] = bmsio.nomem;
	STOREINTELWORD(wire + 4, bmsio.cfg.port);
	STOREINTELWORD(wire + 6, bmsio.cfg.portmask);
	bytes = wire[0] ? (UINT32)wire[1] * BMSIO_BANK_BYTES : 0;
	STOREINTELDWORD(wire + 8, bytes);
	if (!bms_state_header_valid(wire, sizeof(wire) + bytes) ||
	    bmsiowork.bmsmemsize != bytes || (bytes && !bmsiowork.bmsmem)) {
		statflag_seterr(sfh, "M142_BMS_LIVE_STATE");
		return STATFLAG_FAILURE;
	}
	if (statflag_write(sfh, wire, sizeof(wire)) != STATFLAG_SUCCESS)
		return STATFLAG_FAILURE;
	return bytes ? statflag_write(sfh, bmsiowork.bmsmem, bytes) : STATFLAG_SUCCESS;
}

static int flagload_bms(STFLAGH sfh, const SFENTRY *tbl) {
	BYTE wire[BMS_STATE_HEADER_SIZE];
	BYTE *memory = NULL;
	UINT32 bytes;
	_BMSIO restored;
	if (sfh->hdr.ver != tbl->ver ||
	    statflag_read(sfh, wire, sizeof(wire)) != STATFLAG_SUCCESS ||
	    !bms_state_header_valid(wire, sfh->hdr.size))
		return STATFLAG_FAILURE;
	bytes = LOADINTELDWORD(wire + 8);
	if (bytes) {
		memory = (BYTE *)_MALLOC(bytes, "BMS state RAM");
		if (!memory || statflag_read(sfh, memory, bytes) != STATFLAG_SUCCESS) {
			if (memory) _MFREE(memory);
			return STATFLAG_FAILURE;
		}
	}
	ZeroMemory(&restored, sizeof(restored));
	restored.cfg.enabled = wire[0];
	restored.cfg.numbanks = wire[1];
	restored.bank = wire[2];
	restored.nomem = wire[3];
	restored.cfg.port = LOADINTELWORD(wire + 4);
	restored.cfg.portmask = LOADINTELWORD(wire + 6);
	if (bmsiowork.bmsmem) _MFREE(bmsiowork.bmsmem);
	bmsiowork.bmsmem = memory;
	bmsiowork.bmsmemsize = bytes;
	bmsio = restored;
	/* bmsiocfg remains the user's next-reset preference. */
	return STATFLAG_SUCCESS;
}

// ---- FD sub system CPU

static int flagsave_subsystemcpu(STFLAGH sfh, const SFENTRY *tbl) {
	UINT bufsize;
	UINT8 *buf;
	int ret;

	bufsize = subsystem_getcpustatussize();
	buf = (UINT8 *)_MALLOC(bufsize, "SUBCPUSTS");
	if (!buf)
		return STATFLAG_FAILURE;

	if (subsystem_savecpustatus(buf)) {
		ret = statflag_write(sfh, buf, bufsize);
	} else {
		ret = STATFLAG_FAILURE;
	}
	_MFREE(buf);

	return (ret);
}

static int flagload_subsystemcpu(STFLAGH sfh, const SFENTRY *tbl) {
	UINT bufsize;
	UINT8 *buf;
	int ret;

	bufsize = subsystem_getcpustatussize();
	buf = (UINT8 *)_MALLOC(bufsize, "SUBCPUSTS");
	if (!buf)
		return STATFLAG_FAILURE;

	ret = statflag_read(sfh, buf, bufsize);
	if ((ret == STATFLAG_SUCCESS) && !subsystem_loadcpustatus(buf)) {
		ret = STATFLAG_FAILURE;
	}
	_MFREE(buf);

	return (ret);
}

// ---- HOSTFAT immutable snapshot identity

enum {
	HOSTFAT_STATE_VERSION = 1,
	HOSTFAT_STATE_MOUNTED = 1,
	HOSTFAT_STATE_IDENTITY_OFFSET = 4,
	HOSTFAT_STATE_SIZE = HOSTFAT_STATE_IDENTITY_OFFSET + HOSTFAT_IDENTITY_SIZE
};

static int flagsave_hostfat(STFLAGH sfh, const SFENTRY *tbl) {
	BYTE payload[HOSTFAT_STATE_SIZE];

	(void)tbl;
	ZeroMemory(payload, sizeof(payload));
	payload[0] = HOSTFAT_STATE_VERSION;
	if (hostfat_is_mounted()) {
		payload[1] = HOSTFAT_STATE_MOUNTED;
		if (hostfat_snapshot_identity(payload + HOSTFAT_STATE_IDENTITY_OFFSET,
		                              HOSTFAT_IDENTITY_SIZE) != SUCCESS) {
			return (STATFLAG_FAILURE);
		}
	}
	return (statflag_write(sfh, payload, sizeof(payload)));
}

static int flagcheck_hostfat(STFLAGH sfh, const SFENTRY *tbl, BOOL allow_identity_mismatch) {
	BYTE payload[HOSTFAT_STATE_SIZE];
	BYTE current[HOSTFAT_IDENTITY_SIZE];
	BOOL saved_mounted;
	BOOL current_mounted;

	if ((sfh->hdr.ver != tbl->ver) || (sfh->hdr.size != HOSTFAT_STATE_SIZE) ||
	    (tbl->arg2 != HOSTFAT_STATE_SIZE)) {
		statflag_seterr(sfh, "HOSTFAT state identity format is unsupported");
		return (STATFLAG_FAILURE);
	}
	if (statflag_read(sfh, payload, sizeof(payload)) != STATFLAG_SUCCESS) {
		statflag_seterr(sfh, "HOSTFAT state identity is truncated");
		return (STATFLAG_FAILURE);
	}
	if ((payload[0] != HOSTFAT_STATE_VERSION) || (payload[1] > HOSTFAT_STATE_MOUNTED) ||
	    (payload[2] != 0) || (payload[3] != 0)) {
		statflag_seterr(sfh, "HOSTFAT state identity is malformed");
		return (STATFLAG_FAILURE);
	}
	saved_mounted = payload[1] ? TRUE : FALSE;
	current_mounted = hostfat_is_mounted();
	if (saved_mounted != current_mounted) {
		if (allow_identity_mismatch) {
			return (STATFLAG_SUCCESS);
		}
		statflag_seterr(sfh, "HOSTFAT snapshot is missing or differs from the saved state");
		return (STATFLAG_FAILURE);
	}
	if (saved_mounted) {
		if ((hostfat_snapshot_identity(current, sizeof(current)) != SUCCESS) ||
		    memcmp(current, payload + HOSTFAT_STATE_IDENTITY_OFFSET, HOSTFAT_IDENTITY_SIZE)) {
			if (allow_identity_mismatch) {
				return (STATFLAG_SUCCESS);
			}
			statflag_seterr(sfh, "HOSTFAT snapshot is missing or differs from the saved state");
			return (STATFLAG_FAILURE);
		}
	}
	return (STATFLAG_SUCCESS);
}

static int flagload_hostfat(STFLAGH sfh, const SFENTRY *tbl) {
	BYTE payload[HOSTFAT_STATE_SIZE];

	(void)tbl;
	return (statflag_read(sfh, payload, sizeof(payload)));
}

// ----

static int flagcheck_versize(STFLAGH sfh, const SFENTRY *tbl) {
	if ((sfh->hdr.ver == tbl->ver) && (sfh->hdr.size == tbl->arg2)) {
		return (STATFLAG_SUCCESS);
	}
	return (STATFLAG_FAILURE);
}

static int flagcheck_veronly(STFLAGH sfh, const SFENTRY *tbl) {
	if (sfh->hdr.ver == tbl->ver) {
		return (STATFLAG_SUCCESS);
	}
	return (STATFLAG_FAILURE);
}

static int flagcheck_upd9002_state_image(STFLAGH sfh, UINT16 version, UINT32 payload_size,
                                         const char *version_error, const char *truncated_error) {
	Upd9002StateImage state;
	char error[128];
	int ret;

	if (sfh->hdr.ver != version) {
		statflag_seterr(sfh, version_error);
		return (STATFLAG_FAILURE);
	}
	if (sfh->hdr.size != payload_size) {
		statflag_seterr(sfh, UPD9002_STATE_ERROR_SIZE);
		return (STATFLAG_FAILURE);
	}
	ret = statflag_read(sfh, &state, sizeof(state));
	if (ret != STATFLAG_SUCCESS) {
		statflag_seterr(sfh, truncated_error);
		return (STATFLAG_FAILURE);
	}
	error[0] = '\0';
	if (upd9002_state_validate(&state, sizeof(state), error, sizeof(error)) != SUCCESS) {
		statflag_seterr(sfh, error);
		return (STATFLAG_FAILURE);
	}
	return (STATFLAG_SUCCESS);
}

static int flagcheck_upd9002_cpu(STFLAGH sfh, const SFENTRY *tbl) {
	return (flagcheck_upd9002_state_image(sfh, tbl->ver, tbl->arg2,
	                                      "uPD9002 payload version is unsupported",
	                                      "uPD9002 payload is truncated"));
}

static int flagcheck_legacy_cpu_state(STFLAGH sfh) {
	return (flagcheck_upd9002_state_image(sfh, 0, UPD9002_STATE_PAYLOAD_SIZE,
	                                      UPD9002_STATE_ERROR_LEGACY_SECTION,
	                                      "obsolete processor state section is truncated"));
}

// ----

int statsave_save(const char *filename) {
	SFFILEH sffh;
	int ret;
	const SFENTRY *tbl;
	const SFENTRY *tblterm;

	sffh = statflag_create(filename);
	if (sffh == NULL) {
		return (STATFLAG_FAILURE);
	}

	ret = STATFLAG_SUCCESS;
	tbl = np2tbl;
	tblterm = tbl + (sizeof(np2tbl) / sizeof(SFENTRY));
	while (tbl < tblterm) {
		ret |= statflag_createsection(sffh, tbl);
		switch (tbl->type) {
		case STATFLAG_BIN:
		case STATFLAG_TERM:
			ret |= flagsave_common(&sffh->sfh, tbl);
			break;

		case STATFLAG_UPD9002_CPU:
			ret |= flagsave_upd9002_cpu(&sffh->sfh, tbl);
			break;

		case STATFLAG_UPD9002_COMPAT:
			ret |= flagsave_upd9002_compat(&sffh->sfh, tbl);
			break;

		case STATFLAG_MAIN_CPU:
			ret |= flagsave_main_cpu(&sffh->sfh);
			break;

		case STATFLAG_UPD9002_ALT:
			ret |= flagsave_common(&sffh->sfh, tbl);
			break;

		case STATFLAG_UPD9002_MODE:
			ret |= flagsave_upd9002_mode(&sffh->sfh, tbl);
			break;

		case STATFLAG_UPD8087:
			ret |= flagsave_upd8087(&sffh->sfh, tbl);
			break;

		case STATFLAG_COM:
			ret |= flagsave_com(&sffh->sfh, tbl);
			break;

		case STATFLAG_DISK:
			ret |= flagsave_disk(&sffh->sfh, tbl);
			break;

		case STATFLAG_DMA:
			ret |= flagsave_dma(&sffh->sfh, tbl);
			break;

		case STATFLAG_EVT:
			ret |= flagsave_evt(&sffh->sfh, tbl);
			break;

		case STATFLAG_EXT:
			ret |= flagsave_ext(&sffh->sfh, tbl);
			break;

		case STATFLAG_FM:
			ret |= flagsave_fm(&sffh->sfh, tbl);
			break;

		case STATFLAG_MEM:
			ret |= flagsave_mem(&sffh->sfh, tbl);
			break;

		case STATFLAG_BMS:
			ret |= flagsave_bms(&sffh->sfh, tbl);
			break;

		case STATFLAG_SUBCPU:
			ret |= flagsave_subsystemcpu(&sffh->sfh, tbl);
			break;

		case STATFLAG_HOSTFAT:
			ret |= flagsave_hostfat(&sffh->sfh, tbl);
			break;
		}
		tbl++;
	}
	statflag_close(sffh);
	return (ret);
}

static int statsave_check_internal(const char *filename, char *buf, int size,
                                   BOOL allow_hostfat_mismatch) {
	SFFILEH sffh;
	int ret;
	BOOL done;
	BOOL hostfat_seen;
	BOOL legacy_cpu_state_seen;
	BOOL upd9002_format_marker_seen;
	BOOL compat_active_seen;
	BOOL mode_state_seen;
	BOOL bms_seen = FALSE;
	BOOL disk_seen = FALSE;
	BOOL seen[sizeof(np2tbl) / sizeof(np2tbl[0])] = {0};
	const SFENTRY *tbl;
	const SFENTRY *tblterm;

	sffh = statflag_open(filename, buf, size);
	if (sffh == NULL) {
		return (STATFLAG_FAILURE);
	}

	done = FALSE;
	hostfat_seen = FALSE;
	legacy_cpu_state_seen = FALSE;
	upd9002_format_marker_seen = FALSE;
	compat_active_seen = FALSE;
	mode_state_seen = FALSE;
	ret = STATFLAG_SUCCESS;
	while ((!done) && (ret != STATFLAG_FAILURE)) {
		if (statflag_readsection(sffh) != STATFLAG_SUCCESS) {
			statflag_seterr(&sffh->sfh, "M142_STATE_CONTAINER");
			ret = STATFLAG_FAILURE;
			break;
		}
		if (statflag_is_upd9002_format_marker(&sffh->sfh)) {
			upd9002_format_marker_seen = TRUE;
		}
		tbl = np2tbl;
		tblterm = tbl + (sizeof(np2tbl) / sizeof(SFENTRY));
		while (tbl < tblterm) {
			if (!memcmp(sffh->sfh.hdr.index, tbl->index, 10)) {
				break;
			}
			tbl++;
		}
		if (tbl < tblterm) {
			if (seen[tbl - np2tbl]) {
				statflag_seterr(&sffh->sfh, "M142_STATE_DUPLICATE");
				ret = STATFLAG_FAILURE;
				break;
			}
			seen[tbl - np2tbl] = TRUE;
			switch (tbl->type) {
			case STATFLAG_BIN:
			case STATFLAG_MEM:
				ret |= flagcheck_versize(&sffh->sfh, tbl);
				break;

			case STATFLAG_UPD9002_CPU:
				ret |= flagcheck_upd9002_cpu(&sffh->sfh, tbl);
				break;

			case STATFLAG_UPD9002_COMPAT:
				ret |= flagcheck_upd9002_compat(&sffh->sfh, tbl, &compat_active_seen);
				break;

			case STATFLAG_MAIN_CPU:
				ret |= flagcheck_main_cpu(&sffh->sfh, tbl);
				break;

			case STATFLAG_UPD9002_MODE: {
				UINT8 mode_state[UPD9002_MODE_STATE_SIZE];
				ret |= flagread_upd9002_mode(&sffh->sfh, tbl, mode_state);
				mode_state_seen = TRUE;
				break;
			}

			case STATFLAG_UPD9002_ALT:
				ret |= flagcheck_versize(&sffh->sfh, tbl);
				break;

				case STATFLAG_UPD8087:
					ret |= flagcheck_upd8087(&sffh->sfh, tbl);
					break;

			case STATFLAG_HOSTFAT:
				hostfat_seen = TRUE;
				ret |= flagcheck_hostfat(&sffh->sfh, tbl, allow_hostfat_mismatch);
				break;

			case STATFLAG_TERM:
				if (sffh->sfh.hdr.ver != tbl->ver || sffh->sfh.hdr.size ||
				    (UINT64)sffh->secpos + sizeof(sffh->sfh.hdr) != file_getsize64(sffh->fh)) {
					statflag_seterr(&sffh->sfh, "M142_STATE_CONTAINER");
					ret = STATFLAG_FAILURE;
				}
				done = TRUE;
				break;

			case STATFLAG_COM:
			case STATFLAG_DMA:
			case STATFLAG_EVT:
			case STATFLAG_EXT:
			case STATFLAG_SUBCPU:
				ret |= flagcheck_veronly(&sffh->sfh, tbl);
				break;

			case STATFLAG_FM:
				ret |= flagcheck_fm(&sffh->sfh, tbl);
				break;

			case STATFLAG_BMS:
				if (bms_seen) {
					statflag_seterr(&sffh->sfh, "M142_STATE_DUPLICATE");
					ret = STATFLAG_FAILURE;
				} else {
					bms_seen = TRUE;
					ret |= flagcheck_bms(&sffh->sfh, tbl);
				}
				break;

			case STATFLAG_DISK:
				if (disk_seen) {
					statflag_seterr(&sffh->sfh, "M142_STATE_DUPLICATE");
					ret = STATFLAG_FAILURE;
				} else {
					disk_seen = TRUE;
					ret |= flagcheck_disk(&sffh->sfh, tbl);
				}
				break;

			default:
				ret |= STATFLAG_WARNING;
				break;
			}
		} else {
			if (statflag_is_legacy_cpu_state(&sffh->sfh)) {
				legacy_cpu_state_seen = TRUE;
				ret |= flagcheck_legacy_cpu_state(&sffh->sfh);
			} else {
				ret |= STATFLAG_WARNING;
			}
		}
	}
	if (ret != STATFLAG_FAILURE && (!bms_seen || !disk_seen)) {
		statflag_seterr(&sffh->sfh, "M142_STATE_REQUIRED");
		ret = STATFLAG_FAILURE;
	}
	if ((ret != STATFLAG_FAILURE) && legacy_cpu_state_seen && !upd9002_format_marker_seen) {
		statflag_seterr(&sffh->sfh, UPD9002_STATE_ERROR_LEGACY_MARKER);
		ret = STATFLAG_FAILURE;
	}
	/* Older files lack UPD9MODE; one saved while compatible code was active
	 * cannot return to compatible mode and would run Z80 code as native. */
	if ((ret != STATFLAG_FAILURE) && compat_active_seen && !mode_state_seen) {
		statflag_seterr(&sffh->sfh, "saved in compatible mode by an older build; it cannot be "
		                            "resumed");
		ret = STATFLAG_FAILURE;
	}
	if ((ret != STATFLAG_FAILURE) && hostfat_is_mounted() && !hostfat_seen) {
		statflag_seterr(&sffh->sfh, "saved state has no HOSTFAT snapshot identity");
		ret = STATFLAG_FAILURE;
	}
	if (ret != STATFLAG_SUCCESS && ret != STATFLAG_FAILURE) {
		statflag_seterr(&sffh->sfh, "M142_STATE_UNSUPPORTED_SECTION");
		ret = STATFLAG_FAILURE;
	}
	statflag_close(sffh);
	return (ret);
}

int statsave_check(const char *filename, char *buf, int size) {
	return (statsave_check_internal(filename, buf, size, FALSE));
}

int statsave_check_hostfat_override(const char *filename, char *buf, int size) {
	return (statsave_check_internal(filename, buf, size, TRUE));
}

static int statsave_load_internal(const char *filename, BOOL allow_hostfat_mismatch,
                                 char *error, int error_size) {
	SFFILEH sffh;
	int ret;
	BOOL done;
	const SFENTRY *tbl;
	const SFENTRY *tblterm;

	ret = statsave_check_internal(filename, error, error_size, allow_hostfat_mismatch);
	if (ret != STATFLAG_SUCCESS) return STATFLAG_FAILURE;

	sffh = statflag_open(filename, error, error_size);
	if (sffh == NULL) {
		return (STATFLAG_FAILURE);
	}

	// PCCORE read!
	ret = statflag_readsection(sffh);
	if ((ret != STATFLAG_SUCCESS) || (memcmp(sffh->sfh.hdr.index, np2tbl[0].index, 10))) {
		statflag_close(sffh);
		return (STATFLAG_FAILURE);
	}

	soundmng_stop();
	vaeg_recording_native_session_epoch_change();
	rs232c_midipanic();
	mpu98ii_midipanic();
	/* Keep current HDD directories available until all saved references have
	 * been resolved; flagload_disk replaces/detaches the complete device set. */

	ret |= flagload_common(&sffh->sfh, np2tbl);
	{
		UPD8087_CONFIG config;

		/* Older save files have no device section.  Start from the current
		 * configuration so an omitted section cannot inherit a previous
		 * machine's live NDP state; a current UPD8087 section overwrites this
		 * baseline below. */
		config.enabled = np2cfg.upd8087_enable != 0;
		config.clock_hz = np2cfg.upd8087_clock_hz;
		upd8087_initialize(&upd8087, &config);
	}
	pccore_clockrestore();
	sgp_configure_speed();

	CPU_RESET();
	CPU_SETEXTSIZE((UINT32)pccore.extmem);
	nevent_allreset();

	sound_changeclock();
	beep_changeclock();
	sound_reset();
	fddmtrsnd_bind();

	iocore_reset(); // Sound-board reset calls the native PIC interface.
	/* A state without PIC8214 predates 8214 mode: those builds always
	 * behaved as 8259 mode, so keep that unless the section says otherwise. */
	pic8214.mode8259 = 1;
	/* A state without UPD9WCU predates the programmed memory waits (M130). */
	upd9002_wcu_legacy();
	cbuscore_reset();
	fmboard_reset(pccore.sound);

	done = FALSE;
	while ((!done) && (ret != STATFLAG_FAILURE)) {
		if (statflag_readsection(sffh) != STATFLAG_SUCCESS) {
			statflag_seterr(&sffh->sfh, "M142_STATE_CONTAINER");
			ret = STATFLAG_FAILURE;
			break;
		}
		tbl = np2tbl + 1;
		tblterm = np2tbl + (sizeof(np2tbl) / sizeof(SFENTRY));
		while (tbl < tblterm) {
			if (!memcmp(sffh->sfh.hdr.index, tbl->index, 10)) {
				break;
			}
			tbl++;
		}
		if (tbl < tblterm) {
			switch (tbl->type) {
			case STATFLAG_BIN:
				ret |= flagload_common(&sffh->sfh, tbl);
				break;

			case STATFLAG_UPD9002_CPU:
				ret |= flagload_upd9002_cpu(&sffh->sfh, tbl);
				break;

			case STATFLAG_UPD9002_COMPAT:
				ret |= flagload_upd9002_compat(&sffh->sfh, tbl);
				break;

			case STATFLAG_MAIN_CPU:
				ret |= flagcheck_main_cpu(&sffh->sfh, tbl);
				break;

			case STATFLAG_UPD9002_ALT:
				ret |= flagload_upd9002_alt(&sffh->sfh, tbl);
				break;

			case STATFLAG_UPD9002_MODE:
				ret |= flagload_upd9002_mode(&sffh->sfh, tbl);
				break;

			case STATFLAG_UPD8087:
				ret |= flagload_upd8087(&sffh->sfh, tbl);
				break;

			case STATFLAG_TERM:
				done = TRUE;
				break;

			case STATFLAG_COM:
				ret |= flagload_com(&sffh->sfh, tbl);
				break;

			case STATFLAG_DISK:
				ret |= flagload_disk(&sffh->sfh, tbl);
				break;

			case STATFLAG_DMA:
				ret |= flagload_dma(&sffh->sfh, tbl);
				break;

			case STATFLAG_EVT:
				ret |= flagload_evt(&sffh->sfh, tbl);
				break;

			case STATFLAG_EXT:
				ret |= flagload_ext(&sffh->sfh, tbl);
				break;

			case STATFLAG_FM:
				ret |= flagload_fm(&sffh->sfh, tbl);
				break;

			case STATFLAG_MEM:
				ret |= flagload_mem(&sffh->sfh, tbl);
				break;

			case STATFLAG_BMS:
				ret |= flagload_bms(&sffh->sfh, tbl);
				break;

			case STATFLAG_SUBCPU:
				ret |= flagload_subsystemcpu(&sffh->sfh, tbl);
				break;

			case STATFLAG_HOSTFAT:
				ret |= flagload_hostfat(&sffh->sfh, tbl);
				break;

			default:
				ret |= STATFLAG_WARNING;
				break;
			}
		} else {
			if (statflag_is_legacy_cpu_state(&sffh->sfh)) {
				ret |= flagload_legacy_cpu_state(&sffh->sfh);
			} else {
				ret |= STATFLAG_WARNING;
			}
		}
	}
	statflag_close(sffh);
	if (ret != STATFLAG_FAILURE) {
		/* The PIC section is restored before this point.  Rebind the
		 * source-backed NDP input after the complete state image is present;
		 * this does not acknowledge or clear the 8087 exception. */
		pic_setirq_level(IRQ_NDP,
		                 upd8087_interrupt_pending(&upd8087) ? TRUE : FALSE);
	}

	// Rebuild the native VA I/O map after restoring device state.
	upd9002_memorymap_va();
	if (iocore_build() != SUCCESS) ret = STATFLAG_FAILURE;
	iocore_bind();
	cbuscore_bind();
	fmboard_bind();
	if (fm_backend_restore != NULL) {
		if (ymfm_opn_state_load(fm_backend_restore, fm_backend_restore_size) != SUCCESS) {
			ret = STATFLAG_FAILURE;
		}
		_MFREE(fm_backend_restore);
		fm_backend_restore = NULL;
		fm_backend_restore_size = 0;
	}

	soundmng_play();
	if (ret != STATFLAG_SUCCESS) {
		if (error && error_size > 0 && !error[0])
			milstr_ncpy(error, "M142_STATE_RESTORE", error_size);
		return STATFLAG_FAILURE;
	}
	return STATFLAG_SUCCESS;
}

int statsave_load_checked(const char *filename, char *error, int error_size) {
	return statsave_load_internal(filename, FALSE, error, error_size);
}

int statsave_load(const char *filename) {
	return statsave_load_checked(filename, NULL, 0);
}

int statsave_load_hostfat_override_checked(const char *filename, char *error, int error_size) {
	return statsave_load_internal(filename, TRUE, error, error_size);
}

int statsave_load_hostfat_override(const char *filename) {
	return statsave_load_hostfat_override_checked(filename, NULL, 0);
}

#if defined(SUPPORT_OPRECORD)
void statsave_set_load_disk_hook(STATSAVE_LOAD_DISK_HOOK hook) {
	flagload_disk_hook = hook;
}
#endif
