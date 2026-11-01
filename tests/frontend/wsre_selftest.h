/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* Included by the ROM-less frontend selftest, using its state-file helpers. */
#include "cbus/wsre.h"
static int test_wsre_w1(void) {
	UINT8 preference = np2cfg.wsreenable;
	WSRE_STATE expected;
	char path[MAX_PATH], absent[MAX_PATH], bad[MAX_PATH], error[512];
	const char *problem = NULL;
	UINT i;
	REG8 baseline_e8;
	BYTE *bytes = NULL;
	UINT size = 0, pos = 48, section = 0;
	SPRINTF(path, "vaeg-selftest-%lu-wsre.sts", (unsigned long)getpid());
	SPRINTF(absent, "vaeg-selftest-%lu-wsre-off.sts", (unsigned long)getpid());
	SPRINTF(bad, "vaeg-selftest-%lu-wsre-bad.sts", (unsigned long)getpid());
	soundmng_initialize();
	commng_initialize();
	pccore_init();
	np2cfg.wsreenable = 0;
	pccore_reset();
	soundmng_stop();
	baseline_e8 = iocore_inp8(0xe8);
	if (wsre.enabled || iocore_inp8(0x44e0) != 0xff || iocore_inp8(0x46e8) != 0xff ||
	    statsave_save(absent) != STATFLAG_SUCCESS)
		problem = "M143_WSRE_DISABLED";
	np2cfg.wsreenable = 1;
	pccore_reset();
	soundmng_stop();
	iocore_out8(0x44e0, 6);
	iocore_out8(0x45e0, 0x12);
	if (iocore_inp8(0x45e0) != 0x12)
		problem = "M143_WSRE_SR06_BYTE";
	iocore_out16(0x44e0, 0x0006);
	if (iocore_inp8(0x45e0) != 0x0f)
		problem = "M143_WSRE_SR06_LOCK";
	iocore_out16(0x44e0, 0x1206);
	if (iocore_inp8(0x45e0) != 0x12)
		problem = "M143_WSRE_SR06_WORD";
	iocore_out8(0x54e0, 0x27);
	if (iocore_inp8(0x55e0) != 0x98)
		problem = "M143_WSRE_CR27_PROVISIONAL";
	iocore_out16(0x54e0, 0x6627);
	if (iocore_inp8(0x55e0) != 0x98)
		problem = "M143_WSRE_CR27_READONLY";
	for (i = 9; i <= 10; ++i) {
		iocore_out16(0x4ee0, 0x5500 | i);
		if (iocore_inp8(0x4fe0) != 0x55)
			problem = "M143_WSRE_GR_WORD";
		iocore_out8(0x4fe0, 0xaa);
		if (iocore_inp8(0x4fe0) != 0xaa)
			problem = "M143_WSRE_GR_BYTE";
	}
	iocore_out8(0x46e8, 8);
	if (iocore_inp8(0x46e8) != 8 || iocore_inp8(0xe0) != 0xff || iocore_inp8(0xe1) != 0xff ||
	    iocore_inp8(0xe8) != baseline_e8)
		problem = "M143_WSRE_FULL_ADDRESS";
	(void)iocore_inp8(0x5ae0);
	iocore_out8(0x40e0, 0x12);
	iocore_out8(0x40e0, 0x34);
	if (iocore_inp8(0x41e0) != 0x34)
		problem = "M143_WSRE_ATTRIBUTE";
	(void)iocore_inp8(0x5ae0);
	iocore_out8(0x40e0, 0x10);
	iocore_out8(0x48e0, 0xff);
	iocore_out8(0x49e0, 0x15);
	iocore_out8(0x49e0, 0x26);
	iocore_out8(0x49e0, 0x37);
	iocore_out8(0x47e0, 0xff);
	if (iocore_inp8(0x49e0) != 0x15)
		problem = "M143_WSRE_DAC";
	expected = wsre;
	if (!problem && (statsave_save(path) != STATFLAG_SUCCESS ||
	                 statsave_check(path, error, sizeof(error)) != STATFLAG_SUCCESS))
		problem = "M143_WSRE_STATE_CONTROL";
	if (!problem && (statsave_load(absent) != STATFLAG_SUCCESS || wsre.enabled ||
	                 !np2cfg.wsreenable || iocore_inp8(0x45e0) != 0xff))
		problem = "M143_WSRE_ABSENT_RESTORE";
	np2cfg.wsreenable = 0;
	if (!problem && (statsave_load(path) != STATFLAG_SUCCESS || np2cfg.wsreenable ||
	                 memcmp(&wsre, &expected, sizeof(wsre)) || iocore_inp8(0x49e0) != 0x26))
		problem = "M143_WSRE_PRESENT_RESTORE";
	if (!problem && read_whole_file(path, &bytes, &size) != SUCCESS)
		problem = "M143_WSRE_STATE_READ";
	while (!problem && pos + 16 <= size) {
		UINT length = LOADINTELDWORD(bytes + pos + 12);
		if (!memcmp(bytes + pos, "WSRE\0\0\0\0\0\0", 10)) {
			section = pos;
			break;
		}
		if (length > size - pos - 16)
			break;
		pos += 16 + ((length + 15) & ~15u);
	}
	if (!problem && !section)
		problem = "M143_WSRE_SECTION";
	if (!problem) {
		UINT offsets[] = {10, 16 + (UINT)offsetof(WSRE_STATE, enabled),
		                  16 + (UINT)offsetof(GD5428, attr_data),
		                  16 + (UINT)offsetof(GD5428, dac_read_component)};
		for (i = 0; i < NELEMENTS(offsets); ++i) {
			BYTE original = bytes[section + offsets[i]];
			FILEH file;
			bytes[section + offsets[i]] = 0xff;
			file = file_create(bad);
			if (file == FILEH_INVALID || file_write(file, bytes, size) != size)
				problem = "M143_WSRE_MUTATION_WRITE";
			if (file != FILEH_INVALID)
				file_close(file);
			bytes[section + offsets[i]] = original;
			if (problem)
				break;
			if (statsave_check(bad, error, sizeof(error)) != STATFLAG_FAILURE) {
				problem = "M143_WSRE_MUTATION_ACCEPTED";
				break;
			}
			error[strcspn(error, "\r\n")] = 0;
			if (strcmp(error, "M143_WSRE_STATE")) {
				problem = "M143_WSRE_MUTATION_CODE";
				break;
			}
		}
	}
	if (bytes)
		_MFREE(bytes);
	np2cfg.wsreenable = preference;
	pccore_reset();
	pccore_term();
	soundmng_deinitialize();
	file_delete(path);
	file_delete(absent);
	file_delete(bad);
	if (problem)
		return fail("WSR-E W1", problem);
	fprintf(stderr, "selftest: WSR-E W1 ports/registers/optional state ok\n");
	return SUCCESS;
}
