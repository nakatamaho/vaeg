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
#include	"compiler.h"
#include	"selftest.h"
#include	"codecnv.h"
#include	"commng.h"
#include	"dosio.h"
#include	"kbdmap.h"
#include	"np2.h"
#include	"sound.h"
#include	"opngen.h"
#include	"pccore.h"
#include	"profile.h"
#include	"romankana.h"
#include	"s98.h"
#include	"soundmng.h"

static int fail(const char *name, const char *detail) {

	fprintf(stderr, "selftest: %s failed: %s\n", name, detail);
	return(FAILURE);
}

static int test_codecnv(void) {

	const char	sjis_wave[] = {(char)0x81, (char)0x60, '\0'};
	UINT16		utf[4];
	char		euc[8];
	char		sjis[8];

	ZeroMemory(utf, sizeof(utf));
	codecnv_sjis2utf(utf, NELEMENTS(utf), sjis_wave, sizeof(sjis_wave));
	if ((utf[0] != 0xff5e) || (utf[1] != 0)) {
		return(fail("codecnv", "CP932 wave-dash maps incorrectly"));
	}

	ZeroMemory(euc, sizeof(euc));
	ZeroMemory(sjis, sizeof(sjis));
	codecnv_sjis2euc(euc, sizeof(euc), sjis_wave, sizeof(sjis_wave));
	codecnv_euc2sjis(sjis, sizeof(sjis), euc, sizeof(euc));
	if ((memcmp(sjis, sjis_wave, 2) != 0) || (sjis[2] != '\0')) {
		return(fail("codecnv", "SJIS/EUC round-trip changed bytes"));
	}
	fprintf(stderr, "selftest: codecnv ok\n");
	return(SUCCESS);
}

static int test_profile_ini(void) {

	char	path[MAX_PATH];
	char	name[32];
	char	read_name[32];
	UINT8	flag;
	UINT8	read_flag;
	UINT16	count;
	UINT16	read_count;
	UINT8	bytes[3];
	UINT8	read_bytes[3];
	PFTBL	write_tbl[] = {
		{"name", PFTYPE_STR, name, sizeof(name)},
		{"flag", PFTYPE_BOOL, &flag, 0},
		{"count", PFTYPE_UINT16, &count, 0},
		{"bytes", PFTYPE_BIN, bytes, sizeof(bytes)}
	};
	PFTBL	read_tbl[] = {
		{"name", PFTYPE_STR, read_name, sizeof(read_name)},
		{"flag", PFTYPE_BOOL, &read_flag, 0},
		{"count", PFTYPE_UINT16, &read_count, 0},
		{"bytes", PFTYPE_BIN, read_bytes, sizeof(read_bytes)}
	};

	SPRINTF(path, "vaeg-selftest-%lu.ini", (unsigned long)getpid());
	file_delete(path);
	file_cpyname(name, "portable", sizeof(name));
	flag = 1;
	count = 0x1234;
	bytes[0] = 0x12;
	bytes[1] = 0x34;
	bytes[2] = 0xab;
	ZeroMemory(read_name, sizeof(read_name));
	read_flag = 0;
	read_count = 0;
	ZeroMemory(read_bytes, sizeof(read_bytes));

	profile_iniwrite(path, "selftest", write_tbl, NELEMENTS(write_tbl),
																	NULL);
	profile_iniread(path, "selftest", read_tbl, NELEMENTS(read_tbl),
																	NULL);
	file_delete(path);

	if (strcmp(read_name, name) != 0) {
		return(fail("ini", "string value did not round-trip"));
	}
	if ((read_flag != flag) || (read_count != count) ||
		(memcmp(read_bytes, bytes, sizeof(bytes)) != 0)) {
		return(fail("ini", "typed values did not round-trip"));
	}
	fprintf(stderr, "selftest: ini ok\n");
	return(SUCCESS);
}

static int read_whole_file(const char *path, BYTE **out, UINT *out_size) {

	FILEH	fh;
	UINT	size;
	BYTE	*buf;
	int		ret;

	*out = NULL;
	*out_size = 0;
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return(FAILURE);
	}
	size = file_getsize(fh);
	buf = (BYTE *)_MALLOC(size, "selftest-file");
	if (buf == NULL) {
		file_close(fh);
		return(FAILURE);
	}
	ret = SUCCESS;
	if (file_read(fh, buf, size) != size) {
		ret = FAILURE;
	}
	file_close(fh);
	if (ret != SUCCESS) {
		_MFREE(buf);
		return(FAILURE);
	}
	*out = buf;
	*out_size = size;
	return(SUCCESS);
}

static BOOL statsave_section_body_stable(const BYTE *hdr) {

	if (!memcmp(hdr, "CGWINDOW", 8) || !memcmp(hdr, "TEXTRAM", 7)) {
		return(FALSE);
	}
	return(TRUE);
}

static int compare_statsave_stable_sections(const char *left,
													const char *right) {

	BYTE	*ldata;
	BYTE	*rdata;
	UINT	lsize;
	UINT	rsize;
	UINT	pos;
	int		ret;

	ldata = NULL;
	rdata = NULL;
	if ((read_whole_file(left, &ldata, &lsize) != SUCCESS) ||
		(read_whole_file(right, &rdata, &rsize) != SUCCESS)) {
		if (ldata != NULL) {
			_MFREE(ldata);
		}
		if (rdata != NULL) {
			_MFREE(rdata);
		}
		return(FAILURE);
	}
	ret = FAILURE;
	if ((lsize != rsize) || (lsize < 0x30) || memcmp(ldata, rdata, 0x30)) {
		goto cmp_done;
	}
	pos = 0x30;
	while(pos < lsize) {
		UINT	size;
		UINT	padded;
		UINT	body;

		if ((pos + 16) > lsize) {
			goto cmp_done;
		}
		if (memcmp(ldata + pos, rdata + pos, 16)) {
			goto cmp_done;
		}
		size = LOADINTELDWORD(ldata + pos + 12);
		padded = (size + 15) & ~15U;
		body = pos + 16;
		if ((body + padded) > lsize) {
			goto cmp_done;
		}
		if (statsave_section_body_stable(ldata + pos) &&
			memcmp(ldata + body, rdata + body, padded)) {
			goto cmp_done;
		}
		pos = body + padded;
	}
	ret = SUCCESS;

cmp_done:
	_MFREE(ldata);
	_MFREE(rdata);
	return(ret);
}

static int test_statsave(void) {

	char	path1[MAX_PATH];
	char	path2[MAX_PATH];
	char	err[256];
	int		ret;

	SPRINTF(path1, "vaeg-selftest-%lu-1.sts", (unsigned long)getpid());
	SPRINTF(path2, "vaeg-selftest-%lu-2.sts", (unsigned long)getpid());
	file_delete(path1);
	file_delete(path2);

	soundmng_initialize();
	commng_initialize();
	pccore_init();
	S98_init();
	pccore_reset();

	ret = statsave_save(path1);
	if (ret == STATFLAG_SUCCESS) {
		ZeroMemory(err, sizeof(err));
		ret = statsave_check(path1, err, sizeof(err));
	}
	if (ret == STATFLAG_SUCCESS) {
		ret = statsave_load(path1);
	}
	if (ret == STATFLAG_SUCCESS) {
		ret = statsave_save(path2);
	}
	S98_trash();
	pccore_term();
	soundmng_deinitialize();

	if (ret != STATFLAG_SUCCESS) {
		file_delete(path1);
		file_delete(path2);
		return(fail("statsave", "save/check/load returned failure"));
	}
	if (compare_statsave_stable_sections(path1, path2) != SUCCESS) {
		file_delete(path1);
		file_delete(path2);
		return(fail("statsave", "save/load/save bytes differ"));
	}

	file_delete(path1);
	file_delete(path2);
	fprintf(stderr, "selftest: statsave ok\n");
	return(SUCCESS);
}

typedef struct {
	char	text[256];
} TOKENBUF;

static void tokenbuf_emit(const char *token, void *arg) {

	TOKENBUF *buf;

	buf = (TOKENBUF *)arg;
	if (buf->text[0] != '\0') {
		milstr_ncat(buf->text, ",", sizeof(buf->text));
	}
	milstr_ncat(buf->text, token, sizeof(buf->text));
}

static int test_romankana(void) {

	ROMANKANA_STATE	state;
	TOKENBUF		buf;

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "AkaShi", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "a,ka,shi") != 0) {
		return(fail("romankana", "uppercase/basic syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "shi si tsu tu", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "shi,?,shi,?,tsu,?,tsu") != 0) {
		return(fail("romankana", "shi/si and tsu/tu aliases failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "nn n' nka kko", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "nn,?,nn,?,nn,ka,?,xtsu,ko") != 0) {
		return(fail("romankana", "n and doubled-consonant handling failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "gaza daba papa", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "ga,za,?,da,ba,?,pa,pa") != 0) {
		return(fail("romankana", "voiced syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "nya nyu nyo kya sha syo chu tyo ryo gya ja pyo",
				   tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text,
			   "nya,?,nyu,?,nyo,?,kya,?,sha,?,sho,?,chu,?,cho,?,ryo,?,gya,?,ja,?,pyo") != 0) {
		return(fail("romankana", "yoon syllables failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "xya lyu xyo xa li xo xtu ltu",
				   tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "xya,?,xyu,?,xyo,?,xa,?,xi,?,xo,?,xtsu,?,xtsu") != 0) {
		return(fail("romankana", "small kana aliases failed"));
	}

	romankana_reset(&state);
	ZeroMemory(&buf, sizeof(buf));
	romankana_feed(&state, "va vi vu ve vo", tokenbuf_emit, &buf);
	romankana_flush(&state, tokenbuf_emit, &buf);
	if (strcmp(buf.text, "va,?,vi,?,vu,?,ve,?,vo") != 0) {
		return(fail("romankana", "vu syllables failed"));
	}
	fprintf(stderr, "selftest: romankana ok\n");
	return(SUCCESS);
}

static int test_keyboard_mapping(void) {

	if (kbdmap_selftest() != SUCCESS) {
		return(fail("keyboard-map", "mapping lookup/persistence failed"));
	}
	if (test_romankana() != SUCCESS) {
		return(FAILURE);
	}
	fprintf(stderr, "selftest: keyboard mapping ok\n");
	return(SUCCESS);
}

static void program_opn_test_voice(void) {

	static const REG8 slots[] = {0x00, 0x04, 0x08, 0x0c};
	UINT index;

	for (index=0; index<NELEMENTS(slots); index++) {
		opngen_setreg(0, 0x30 + slots[index], 0x01);
		opngen_setreg(0, 0x40 + slots[index], 0x00);
		opngen_setreg(0, 0x50 + slots[index], 0x1f);
		opngen_setreg(0, 0x60 + slots[index], 0x00);
		opngen_setreg(0, 0x70 + slots[index], 0x00);
		opngen_setreg(0, 0x80 + slots[index], 0x0f);
		opngen_setreg(0, 0x90 + slots[index], 0x00);
	}
	opngen_setreg(0, 0xa0, 0x98);
	opngen_setreg(0, 0xa4, 0x22);
	opngen_setreg(0, 0xb0, 0x07);
	opngen_setreg(0, 0xb4, 0xc0);
	opngen_keyon(0, 0xf0);
}

static int opn_backend_produces_audio(UINT backend, REG8 channels,
												UINT flags) {

	SINT32 pcm[4096 * 2];
	UINT index;

	opngen_setbackend(backend);
	opngen_reset();
	opngen_setcfg(channels, flags);
	opngen_setvol(64);
	program_opn_test_voice();
	ZeroMemory(pcm, sizeof(pcm));
	opngen_getpcm(NULL, pcm, NELEMENTS(pcm) / 2);
	for (index=0; index<NELEMENTS(pcm); index++) {
		if (pcm[index] != 0) {
			return(SUCCESS);
		}
	}
	return(FAILURE);
}

static int test_opn_backends(void) {

	opngen_initialize(44100);
	if (opngen_parsebackend("np2") != OPN_BACKEND_NP2 ||
		opngen_parsebackend("ymfm") != OPN_BACKEND_YMFM ||
		opngen_parsebackend("invalid") != OPN_BACKEND_YMFM ||
		opngen_parsebackend("") != OPN_BACKEND_YMFM ||
		opngen_parsebackend(NULL) != OPN_BACKEND_YMFM) {
		return(fail("opn-backend", "backend name parsing failed"));
	}
	if (opn_backend_produces_audio(OPN_BACKEND_NP2, 3,
										OPN_MONORAL | 0x007) != SUCCESS) {
		return(fail("opn-backend", "NP2 YM2203 path was silent"));
	}
	if (opn_backend_produces_audio(OPN_BACKEND_YMFM, 3,
										OPN_MONORAL | 0x007) != SUCCESS) {
		return(fail("opn-backend", "ymfm YM2203 path was silent"));
	}
	if (opn_backend_produces_audio(OPN_BACKEND_YMFM, 6,
										OPN_STEREO | 0x03f) != SUCCESS) {
		return(fail("opn-backend", "ymfm YM2608 path was silent"));
	}
	opngen_setbackend(OPN_BACKEND_YMFM);
	opngen_reset();
	fprintf(stderr, "selftest: OPN backends ok\n");
	return(SUCCESS);
}

int vaeg_selftest_run(void) {

	if (test_codecnv() != SUCCESS) {
		return(FAILURE);
	}
	if (test_profile_ini() != SUCCESS) {
		return(FAILURE);
	}
	if (test_keyboard_mapping() != SUCCESS) {
		return(FAILURE);
	}
	if (test_opn_backends() != SUCCESS) {
		return(FAILURE);
	}
	if (test_statsave() != SUCCESS) {
		return(FAILURE);
	}
	fprintf(stderr, "selftest: all tests passed\n");
	return(SUCCESS);
}
