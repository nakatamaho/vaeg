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
/*
 * Cassette tape for V1/V2 mode (vaeg extension, M103j).
 *
 * [VA-TM] The VA's uPD8251 serves RS-232C only and port 30h defines only
 * bits 0 and 1, so a real VA has no cassette. [ROM] Its N-88 BASIC still
 * carries the PC-8801 cassette code: port 30h bit 3 switches the motor and
 * bits 5-4 select the 8251 channel (00 = cassette 600 baud, 01 = 1200
 * baud, 1x = RS-232C); BASIC writes by polling TXRDY and reads through the
 * receive interrupt (8214 level 0). See docs/agents/tasks/M103j_*.md.
 *
 * Tape images are a raw byte stream, or T88. A T88's data blocks are played
 * back in order, and its blank, space (1200 Hz) and mark (2400 Hz) blocks
 * take their recorded time before the data that follows them (M106).
 * Saving records the transmitted bytes as a raw image.
 */

#include "compiler.h"
#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "sound.h"
#include "cmt.h"

#include <string.h>

enum {
	CMT_MAXSIZE = 0x400000,
	/* Received bytes keep the request asserted, like the RXRDY level. */
	CMT_REQUEST_HZ = 600,
	/* T88 times are in 1/4800 s. */
	CMT_TICKS_PER_SECOND = 4800,
	/* Raw-image leader, as on most recorded tapes: 1200 Hz for 1.5 s, then
	 * 2400 Hz for 0.5 s (M106). */
	CMT_LEADER_SPACE_TICKS = 7200,
	CMT_LEADER_MARK_TICKS = 2400
};

/* A timed tape block without data, before the data byte at offset at. */
enum {
	CMT_TONE_NONE = 0,
	CMT_TONE_BLANK,
	CMT_TONE_SPACE,
	CMT_TONE_MARK
};
typedef struct {
	UINT32 at;
	UINT32 ticks;
	UINT8 kind;
} CMTTONE;

static const CMTTONE cmt_raw_leader[2] = {{0, CMT_LEADER_SPACE_TICKS, CMT_TONE_SPACE},
                                          {0, CMT_LEADER_MARK_TICKS, CMT_TONE_MARK}};

static const char t88_signature[] = "PC-8801 Tape Image(T88)";

static struct {
	BYTE *tape;
	UINT32 size;
	UINT32 pos;
	BYTE *out;
	UINT32 outsize;
	UINT32 outcap;
	char outpath[MAX_PATH];
	BOOL saving;
	UINT8 control; /* 88-mode port 30h */
	UINT8 data;
	BOOL rxready;
	BOOL rxe;
	CMTTONE *tones; /* T88 blank/space/mark blocks, in tape order */
	UINT32 ntones;
	UINT32 tonepos; /* next T88 block */
	UINT32 leadpos; /* next raw-image leader block; 2 = none pending */
	UINT8 tone;     /* block sounding; no data until it ends */
	BOOL toneisraw; /* the block sounding is from cmt_raw_leader */
	BOOL inevent;   /* inside cmt_event: time from its due clock */
} cmt;

/*
 * Tape sound (the loading "pi-gaa"): each byte as one start bit, eight data
 * bits LSB first and one stop bit, a 0 as 1200 Hz and a 1 as 2400 Hz
 * (PC-8801 cassette FSK), and 2400 Hz between bytes while the motor runs.
 * Played at the real baud rate: with fast load, bytes that arrive while
 * one is still sounding are skipped, and gaps while data flows are filled
 * with data rather than carrier, so the sound keeps its pitch, rhythm and
 * 1200/2400 Hz balance. Display/sound only; it does not affect the data path.
 */
static struct {
	UINT16 frame; /* bits still to play, LSB first */
	UINT8 bits;   /* number of bits left in frame */
	BOOL pending; /* a byte waits to be played */
	UINT8 recent; /* frames to keep sounding data after the last byte */
	UINT8 next;
	UINT32 bitpos; /* 16.16 position within the current bit */
	UINT32 phase;  /* 16.16 tone phase */
	SINT32 level;  /* amplitude */
} cmtsnd;

void cmt_setvol(UINT vol) {
	if (vol > 128) {
		vol = 128;
	}
	cmtsnd.level = (SINT32)vol * 32;
}

static void cmtsnd_push(REG8 dat) {
	cmtsnd.next = (UINT8)dat;
	cmtsnd.pending = TRUE;
	cmtsnd.recent = 8;
}

static BOOL cmtsnd_carrier(void) {
	return (cmt_selected() && (cmt.control & 0x08) &&
	        ((cmt.pos < cmt.size) || cmt.rxready || cmt.saving))
	           ? TRUE
	           : FALSE;
}

void cmt_getpcm(void *hdl, SINT32 *pcm, UINT count) {
	const UINT baud = (cmt.control & 0x10) ? 1200 : 600;
	UINT32 bitstep;
	UINT32 rate;

	(void)hdl;
	rate = soundcfg.rate;
	if ((rate == 0) || (cmtsnd.level == 0)) {
		cmtsnd.pending = FALSE;
		return;
	}
	if (!cmt_selected() || !(cmt.control & 0x08)) {
		/* Motor off: the tape stops, and so does its sound. */
		cmtsnd.bits = 0;
		cmtsnd.pending = FALSE;
		return;
	}
	bitstep = (UINT32)(((UINT64)baud << 16) / rate);
	while (count--) {
		int bit;
		UINT32 freq;
		SINT32 samp;

		if (cmt.tone != CMT_TONE_NONE) {
			/* A block without data: 1200 Hz, 2400 Hz, or silence. */
			cmtsnd.bits = 0;
			if (cmt.tone != CMT_TONE_BLANK) {
				freq = (cmt.tone == CMT_TONE_MARK) ? 2400 : 1200;
				cmtsnd.phase += (UINT32)(((UINT64)freq << 16) / rate);
				samp = (cmtsnd.phase & 0x8000) ? cmtsnd.level : -cmtsnd.level;
				pcm[0] += samp;
				pcm[1] += samp;
			}
			pcm += 2;
			continue;
		}
		if (cmtsnd.bits == 0) {
			if (cmtsnd.pending) {
				cmtsnd.frame = (UINT16)(0x200 | ((UINT16)cmtsnd.next << 1));
				cmtsnd.bits = 10;
				cmtsnd.pending = FALSE;
			} else if (np2cfg.cmt_fast && cmtsnd.recent) {
				/* Fast load: between sampled bytes keep sounding data (the
				 * tape at its current position) rather than the carrier,
				 * whose 2400 Hz would otherwise dominate. */
				const UINT8 dat = (cmt.pos < cmt.size) ? cmt.tape[cmt.pos] : cmtsnd.next;

				cmtsnd.recent--;
				cmtsnd.frame = (UINT16)(0x200 | ((UINT16)dat << 1));
				cmtsnd.bits = 10;
			} else if (cmtsnd_carrier()) {
				cmtsnd.frame = 1;
				cmtsnd.bits = 1;
			} else {
				pcm += 2;
				continue;
			}
			cmtsnd.bitpos = 0;
		}
		bit = cmtsnd.frame & 1;
		freq = bit ? 2400 : 1200;
		cmtsnd.phase += (UINT32)(((UINT64)freq << 16) / rate);
		samp = (cmtsnd.phase & 0x8000) ? cmtsnd.level : -cmtsnd.level;
		pcm[0] += samp;
		pcm[1] += samp;
		pcm += 2;
		cmtsnd.bitpos += bitstep;
		if (cmtsnd.bitpos >= 0x10000) {
			cmtsnd.bitpos -= 0x10000;
			cmtsnd.frame >>= 1;
			cmtsnd.bits--;
		}
	}
}

void cmt_initialize(void) {
	ZeroMemory(&cmt, sizeof(cmt));
	ZeroMemory(&cmtsnd, sizeof(cmtsnd));
	cmt.leadpos = 2;
	cmt_setvol(np2cfg.cmt_vol);
}

void cmt_deinitialize(void) {
	cmt_save_end();
	cmt_eject();
	if (cmt.tones != NULL) {
		_MFREE(cmt.tones);
	}
	if (cmt.out != NULL) {
		_MFREE(cmt.out);
	}
	ZeroMemory(&cmt, sizeof(cmt));
}

void cmt_reset(void) {
	cmtsnd.bits = 0;
	cmtsnd.pending = FALSE;
	cmt.control = 0;
	cmt.rxready = FALSE;
	cmt.rxe = FALSE;
	cmt.tone = CMT_TONE_NONE;
	cmt.leadpos = 2;
	nevent_reset(NEVENT_CMT);
}

BOOL cmt_selected(void) {
	return (memoryva_88_mode && !(cmt.control & 0x20)) ? TRUE : FALSE;
}

static BOOL cmt_running(void) {
	return (cmt_selected() && (cmt.control & 0x08) && cmt.rxe) ? TRUE : FALSE;
}

/* CPU clocks per tape byte: start, 8 data and stop bits; faster when the
 * cassette fast-load option is on. */
static SINT32 cmt_byteclock(void) {
	UINT baud = (cmt.control & 0x10) ? 1200 : 600;
	SINT32 clock = (SINT32)(pccore.realclock / (baud / 10));

	if (np2cfg.cmt_fast) {
		clock /= 16;
	}
	return (clock > 0) ? clock : 1;
}

/*
 * Blocks without data (decision C11): a T88's blank, space and mark blocks
 * sound for their recorded time when the tape reaches them. A raw image (or
 * a T88 without such blocks) has no times, so when the motor starts for
 * reading it gets the leader of most recorded tapes, 1200 Hz for 1.5 s then
 * 2400 Hz for 0.5 s ([MEAS] the space and mark blocks before the first data
 * in 87 T88 images of commercial tapes). No byte is delivered while a block
 * sounds, and the carrier (port 40h bit 2) is up only during 2400 Hz, so
 * BASIC prints "Found" after them. All are skipped with fast load and while
 * recording. A block cut off by the motor stopping starts again with it.
 */
static void cmt_leader_start(void) {
	cmt.leadpos = (cmt.ntones == 0) ? 0 : 2;
}

static BOOL cmt_tone_start(void) {
	const CMTTONE *t;

	if (!cmt_selected() || !(cmt.control & 0x08) || cmt.saving || (cmt.tape == NULL) ||
	    (cmt.pos >= cmt.size)) {
		return FALSE;
	}
	for (;;) {
		if (np2cfg.cmt_fast) {
			cmt.leadpos = 2;
			while ((cmt.tonepos < cmt.ntones) && (cmt.tones[cmt.tonepos].at <= cmt.pos)) {
				cmt.tonepos++;
			}
			return FALSE;
		}
		if (cmt.leadpos < 2) {
			t = &cmt_raw_leader[cmt.leadpos++];
			cmt.toneisraw = TRUE;
		} else if ((cmt.tonepos < cmt.ntones) && (cmt.tones[cmt.tonepos].at <= cmt.pos)) {
			t = &cmt.tones[cmt.tonepos++];
			cmt.toneisraw = FALSE;
		} else {
			return FALSE;
		}
		if (t->ticks != 0) {
			break;
		}
	}
	cmt.tone = t->kind;
	nevent_reset(NEVENT_CMT); /* a byte may have been scheduled */
	/* After a block or byte the next starts at its due time; from a port
	 * write (the motor starting) it starts now. */
	nevent_set(NEVENT_CMT, (SINT32)(((UINT64)pccore.realclock * t->ticks) / CMT_TICKS_PER_SECOND),
	           cmt_event, cmt.inevent ? NEVENT_RELATIVE : NEVENT_ABSOLUTE);
	return TRUE;
}

static void cmt_tone_stop(void) {
	/* The motor stopped in the middle of a block: play it again later. */
	if (cmt.toneisraw) {
		cmt.leadpos--;
	} else {
		cmt.tonepos--;
	}
	cmt.tone = CMT_TONE_NONE;
	nevent_reset(NEVENT_CMT);
}

static void cmt_schedule(void) {
	if (cmt.tone != CMT_TONE_NONE) {
		if (!cmt_selected() || !(cmt.control & 0x08)) {
			cmt_tone_stop();
		}
		return;
	}
	if (!cmt.rxready && cmt_tone_start()) {
		return;
	}
	if (cmt_running() && (cmt.rxready || (cmt.pos < cmt.size))) {
		if (!nevent_iswork(NEVENT_CMT)) {
			nevent_set(NEVENT_CMT,
			           cmt.rxready ? (SINT32)(pccore.realclock / CMT_REQUEST_HZ) : cmt_byteclock(),
			           cmt_event, NEVENT_RELATIVE);
		}
	} else if (nevent_iswork(NEVENT_CMT)) {
		nevent_reset(NEVENT_CMT);
	}
}

void cmt_event(NEVENTITEM item) {
	(void)item;
	cmt.inevent = TRUE;
	if (cmt.tone != CMT_TONE_NONE) {
		cmt.tone = CMT_TONE_NONE;
		cmt_schedule();
		cmt.inevent = FALSE;
		return;
	}
	if (!cmt_running()) {
		cmt.inevent = FALSE;
		return;
	}
	if (!cmt.rxready && (cmt.pos < cmt.size)) {
		cmt.data = cmt.tape[cmt.pos++];
		cmt.rxready = TRUE;
		cmtsnd_push(cmt.data);
	}
	if (cmt.rxready) {
		pic8214_request(PIC8214_RXRDY);
	}
	cmt_schedule();
	cmt.inevent = FALSE;
}

/* Carrier on the tape: [ROM] BASIC waits for port 40h bit 2 before it
 * starts reading (7EDFh). Inside a block without data it is the 2400 Hz. */
BOOL cmt_carrier(void) {
	if (!cmt_selected() || !(cmt.control & 0x08) || (cmt.pos >= cmt.size)) {
		return FALSE;
	}
	return (cmt.tone == CMT_TONE_NONE) || (cmt.tone == CMT_TONE_MARK);
}

void cmt_port30(REG8 dat) {
	const BOOL motor = (cmt.control & 0x08) ? TRUE : FALSE;

	cmt.control = (UINT8)(dat & 0x3c);
	if (!motor && (cmt.control & 0x08)) {
		cmt_leader_start();
	}
	cmt_schedule();
}

void cmt_command(REG8 cmd) {
	cmt.rxe = (cmd & 0x04) ? TRUE : FALSE;
	cmt_schedule();
}

/* 8251 status: DSR, TXE and TXRDY always set; RXRDY while a byte waits. */
REG8 cmt_status(REG8 rxe) {
	(void)rxe;
	return (REG8)(0x85 | (cmt.rxready ? 0x02 : 0));
}

REG8 cmt_read(void) {
	cmt.rxready = FALSE;
	if ((cmt.tone == CMT_TONE_NONE) && nevent_iswork(NEVENT_CMT)) {
		nevent_reset(NEVENT_CMT);
	}
	cmt_schedule();
	return cmt.data;
}

void cmt_write(REG8 dat) {
	BYTE *grown;

	if (!cmt.saving || !(cmt.control & 0x08)) {
		return;
	}
	if (cmt.outsize >= cmt.outcap) {
		if (cmt.outcap >= CMT_MAXSIZE) {
			return;
		}
		grown = (BYTE *)_MALLOC(cmt.outcap ? cmt.outcap * 2 : 0x4000, "cmt-save");
		if (grown == NULL) {
			return;
		}
		if (cmt.out != NULL) {
			CopyMemory(grown, cmt.out, cmt.outsize);
			_MFREE(cmt.out);
		}
		cmt.out = grown;
		cmt.outcap = cmt.outcap ? cmt.outcap * 2 : 0x4000;
	}
	cmt.out[cmt.outsize++] = (BYTE)dat;
	cmtsnd_push(dat);
}

/* T88 blank (0100h), space (0102h) and mark (0103h) tags carry begin and
 * length ticks; each is kept with the offset of the data that follows it.
 * Returns the count; dst may be NULL. */
static UINT32 cmt_t88_tones(const BYTE *src, UINT size, CMTTONE *dst) {
	UINT32 p = sizeof(t88_signature);
	UINT32 n = 0;
	UINT32 at = 0;

	while (p + 4 <= size) {
		const UINT type = LOADINTELWORD(src + p);
		const UINT len = LOADINTELWORD(src + p + 2);

		p += 4;
		if ((type == 0x0000) || (p + len > size)) {
			break;
		}
		if ((type == 0x0101) && (len >= 12)) {
			UINT bytes = LOADINTELWORD(src + p + 8);

			at += (bytes > len - 12) ? len - 12 : bytes;
		} else if (((type == 0x0100) || (type == 0x0102) || (type == 0x0103)) && (len >= 8)) {
			if (dst != NULL) {
				dst[n].at = at;
				dst[n].ticks = LOADINTELDWORD(src + p + 4);
				dst[n].kind = (type == 0x0100)   ? CMT_TONE_BLANK
				              : (type == 0x0102) ? CMT_TONE_SPACE
				                                 : CMT_TONE_MARK;
			}
			n++;
		}
		p += len;
	}
	return n;
}

/* T88: 24-byte signature, then tags (type, length, body); data tags carry
 * begin and length ticks, data size and type, then the bytes. */
static UINT32 cmt_t88_data(const BYTE *src, UINT size, BYTE *dst) {
	UINT32 p = sizeof(t88_signature);
	UINT32 n = 0;

	while (p + 4 <= size) {
		const UINT type = LOADINTELWORD(src + p);
		const UINT len = LOADINTELWORD(src + p + 2);

		p += 4;
		if ((type == 0x0000) || (p + len > size)) {
			break;
		}
		if ((type == 0x0101) && (len >= 12)) {
			UINT bytes = LOADINTELWORD(src + p + 8);

			if (bytes > len - 12) {
				bytes = len - 12;
			}
			if (dst != NULL) {
				CopyMemory(dst + n, src + p + 12, bytes);
			}
			n += bytes;
		}
		p += len;
	}
	return n;
}

BOOL cmt_open_memory(const BYTE *data, UINT size) {
	BOOL t88;
	UINT32 bytes;
	UINT32 ntones;
	BYTE *tape;
	CMTTONE *tones = NULL;

	t88 = (size >= sizeof(t88_signature)) &&
	      (memcmp(data, t88_signature, sizeof(t88_signature)) == 0);
	bytes = t88 ? cmt_t88_data(data, size, NULL) : size;
	if ((bytes == 0) || (bytes > CMT_MAXSIZE)) {
		return FAILURE;
	}
	tape = (BYTE *)_MALLOC(bytes, "cmt-tape");
	if (tape == NULL) {
		return FAILURE;
	}
	ntones = t88 ? cmt_t88_tones(data, size, NULL) : 0;
	if (ntones != 0) {
		tones = (CMTTONE *)_MALLOC(ntones * sizeof(CMTTONE), "cmt-tones");
		if (tones == NULL) {
			_MFREE(tape);
			return FAILURE;
		}
		cmt_t88_tones(data, size, tones);
	}
	if (t88) {
		cmt_t88_data(data, size, tape);
	} else {
		CopyMemory(tape, data, bytes);
	}
	cmt_eject();
	cmt.tape = tape;
	cmt.size = bytes;
	cmt.pos = 0;
	cmt.tones = tones;
	cmt.ntones = ntones;
	cmt.tonepos = 0;
	cmt_schedule();
	return SUCCESS;
}

BOOL cmt_open(const char *path) {
	FILEH fh;
	UINT size;
	BYTE *buf;
	BOOL ret;

	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return FAILURE;
	}
	size = file_getsize(fh);
	if ((size == 0) || (size > CMT_MAXSIZE)) {
		file_close(fh);
		return FAILURE;
	}
	buf = (BYTE *)_MALLOC(size, "cmt-file");
	if (buf == NULL) {
		file_close(fh);
		return FAILURE;
	}
	ret = (file_read(fh, buf, size) == size) ? cmt_open_memory(buf, size) : FAILURE;
	_MFREE(buf);
	file_close(fh);
	return ret;
}

void cmt_eject(void) {
	if (cmt.tape != NULL) {
		_MFREE(cmt.tape);
	}
	if (cmt.tones != NULL) {
		_MFREE(cmt.tones);
	}
	cmt.tape = NULL;
	cmt.size = 0;
	cmt.pos = 0;
	cmt.tones = NULL;
	cmt.ntones = 0;
	cmt.tonepos = 0;
	cmt.leadpos = 2;
	cmt.rxready = FALSE;
	cmt.tone = CMT_TONE_NONE;
	nevent_reset(NEVENT_CMT);
	cmt_schedule();
}

void cmt_rewind(void) {
	cmt.pos = 0;
	cmt.tonepos = 0;
	cmt.leadpos = 2;
	cmt.rxready = FALSE;
	cmt.tone = CMT_TONE_NONE;
	nevent_reset(NEVENT_CMT);
	cmt_schedule();
}

BOOL cmt_inserted(void) {
	return (cmt.tape != NULL) ? TRUE : FALSE;
}

UINT32 cmt_position(void) {
	return cmt.pos;
}

UINT32 cmt_length(void) {
	return cmt.size;
}

BOOL cmt_save_begin(const char *path) {
	cmt_save_end();
	if ((path == NULL) || (path[0] == '\0')) {
		return FAILURE;
	}
	milstr_ncpy(cmt.outpath, path, sizeof(cmt.outpath));
	cmt.outsize = 0;
	cmt.saving = TRUE;
	return SUCCESS;
}

BOOL cmt_save_end(void) {
	FILEH fh;
	BOOL ret = SUCCESS;

	if (!cmt.saving) {
		return SUCCESS;
	}
	cmt.saving = FALSE;
	if (cmt.outpath[0] != '\0') {
		fh = file_create(cmt.outpath);
		if (fh == FILEH_INVALID) {
			ret = FAILURE;
		} else {
			if (file_write(fh, cmt.out, cmt.outsize) != cmt.outsize) {
				ret = FAILURE;
			}
			file_close(fh);
		}
	}
	return ret;
}

void cmt_save_discard(void) {
	cmt.saving = FALSE;
	cmt.outpath[0] = '\0';
	cmt.outsize = 0;
}

BOOL cmt_saving(void) {
	return cmt.saving;
}

UINT32 cmt_saved_bytes(void) {
	return cmt.outsize;
}

const BYTE *cmt_saved_data(void) {
	return cmt.out;
}
