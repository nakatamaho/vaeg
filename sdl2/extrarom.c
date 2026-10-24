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
 * M112: discovery of user-supplied PC-8801/8001 family ROMs below
 * extra/<machine>/ in the ROM directory. Any revision is accepted; a ROM is
 * checked only for its size and for not being blank. Known dumps are named
 * from MAME's published ROM definitions (extrarom_mame.inc); a MAME match is
 * a reference, not proof of a good dump. See docs/modernization/extra-roms.md.
 */

#include "compiler.h"
#include "dosio.h"
#include "machine/pccore.h"
#include "extrarom.h"
#include "romcheck.h"
#include "strres.h"
#include "memoryva.h"
#include "cgromva.h"
#include "videova.h"
#include "subsystem.h"
#include "subsystemmx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char extrarom_last_error[160];

const char *const extrarom_role_names[EXTRAROM_ROLES] = {
    "n88",  "n88_0",  "n88_1",  "n88_2", "n88_3",  "n80",  "n80_2",    "n80_2sr", "n80_3",
    "disk", "kanji1", "kanji2", "jisyo", "cdbios", "font", "font80sr", "fonthira"};

/* Accepted sizes of each role (0 ends the list). */
static const UINT32 role_sizes[EXTRAROM_ROLES][2] = {
    {0x8000, 0},  {0x2000, 0},  {0x2000, 0}, {0x2000, 0},     {0x2000, 0},  {0x6000, 0x8000},
    {0x8000, 0},  {0x8000, 0},  {0xa000, 0}, {0x800, 0x2000}, {0x20000, 0}, {0x20000, 0},
    {0x80000, 0}, {0x10000, 0}, {0x800, 0},  {0x1000, 0},     {0x800, 0}};

/*
 * Models and the roles each needs; "a|b" accepts either. Requirements
 * follow MAME's ROM definitions. disk.rom is the FDD unit's ROM: optional
 * only for the PC-8801, which has no FDD inside (the external PC-80S31);
 * required from the mkII on, whose every model carries it (a model 10 runs
 * a drive once one is fitted, maintainer). The mkII takes its 8x8 characters from
 * kanji1.rom, as later models do; MAME notes that its known dump holds only
 * the main ROMs.
 *
 * M113: `base` is the VA model whose V1/V2 mode runs the machine (0: not
 * startable yet), `v2` whether it has N88-BASIC V2 (from the mkIISR on) and
 * `high` whether it runs at 8 MHz (H; from the FH/MH on). The mkIITR, FE
 * and FE2 are listed in MAME only as commented-out machines without ROM
 * definitions; their roles follow the mkIISR, FA and FA. PC-88VA and
 * PC-88VA2 are PC-8801-layout sets taken from the VA ROMs.
 *
 * M114: the PC-8001 family (`n_role` set) runs in the N mode of the VA2's
 * V1/V2 mode with its own N-BASIC ROM in place of a PC-8801's n80.rom; the
 * VA's N88-BASIC stays, as the VA firmware still starts the machine.
 */
#define PC8801_V2_ROLES "n88 n88_0 n88_1 n88_2 n88_3 n80 disk kanji1"
static const struct {
	const char *name;
	const char *required;
	UINT8 base;
	UINT8 v2;
	UINT8 high;
	UINT8 n_role;
} machine_table[] = {
    {"PC-8001", "n80 font", 2, 0, 0, EXTRAROM_ROLE_N80},
    {"PC-8001mkII", "n80_2 font", 2, 0, 0, EXTRAROM_ROLE_N80_2},
    {"PC-8001mkIISR", "n80_2sr n80_3 font font80sr fonthira", 2, 0, 0, EXTRAROM_ROLE_N80_2SR},
    {"PC-8801", "n88 n88_0 n80 font|kanji1", 2, 0, 0},
    {"PC-8801mkII", "n88 n88_0 n80 disk kanji1", 2, 0, 0},
    {"PC-8801mkIISR", PC8801_V2_ROLES, 2, 1, 0},
    {"PC-8801mkIITR", PC8801_V2_ROLES, 2, 1, 0},
    {"PC-8801mkIIFR", PC8801_V2_ROLES, 2, 1, 0},
    {"PC-8801mkIIMR", PC8801_V2_ROLES " kanji2", 2, 1, 0},
    {"PC-8801FH", PC8801_V2_ROLES " kanji2", 2, 1, 1},
    {"PC-8801MH", PC8801_V2_ROLES " kanji2", 2, 1, 1},
    {"PC-8801FA", PC8801_V2_ROLES " kanji2", 2, 1, 1},
    {"PC-8801MA", PC8801_V2_ROLES " kanji2 jisyo", 2, 1, 1},
    {"PC-8801FE", PC8801_V2_ROLES " kanji2", 2, 1, 1},
    {"PC-8801MA2", PC8801_V2_ROLES " kanji2 jisyo", 2, 1, 1},
    {"PC-8801FE2", PC8801_V2_ROLES " kanji2", 2, 1, 1},
    {"PC-8801MC", PC8801_V2_ROLES " kanji2 jisyo cdbios", 2, 1, 1},
    {"PC-98DO", "n88 n88_0 n88_1 n88_2 n88_3 n80 disk", 2, 1, 1},
    {"PC-88VA", "n88 n88_0 n88_1 n88_2 n88_3 disk kanji1", 1, 1, 1},
    {"PC-88VA2", "n88 n88_0 n88_1 n88_2 n88_3 disk kanji1", 2, 1, 1},
};

static const struct {
	const char *sha1;
	const char *machine;
	const char *file;
	const char *bios;
	BOOL bad_dump;
} mame_table[] = {
#include "extrarom_mame.inc"
};

/* Combined PC88.ROM (M88 layout): offset and size of each part. */
#define PC88ROM_FULL 0x1c000
#define PC88ROM_SHORT 0x16000 /* without the N-BASIC head */
static const struct {
	UINT role;
	UINT32 offset;
	UINT32 size;
} pc88rom_parts[] = {
    {EXTRAROM_ROLE_N88, 0x00000, 0x8000},   {EXTRAROM_ROLE_N88_0, 0x0c000, 0x2000},
    {EXTRAROM_ROLE_N88_1, 0x0e000, 0x2000}, {EXTRAROM_ROLE_N88_2, 0x10000, 0x2000},
    {EXTRAROM_ROLE_N88_3, 0x12000, 0x2000}, {EXTRAROM_ROLE_DISK, 0x14000, 0x2000},
};

static int lower(int c) {
	return ((c >= 'A') && (c <= 'Z')) ? (c - 'A' + 'a') : c;
}

static BOOL same_name(const char *a, const char *b) {
	while ((*a != '\0') && (lower((BYTE)*a) == lower((BYTE)*b))) {
		a++;
		b++;
	}
	return (*a == '\0') && (*b == '\0');
}

/* MAME's machine name for a model directory: "PC-8801mkIISR" -> "pc8801mk2sr". */
static void mame_machine(const char *name, char *out, UINT size) {
	UINT n = 0;

	while ((*name != '\0') && (n + 1 < size)) {
		if (*name == '-') {
			name++;
		} else if (strncmp(name, "mkII", 4) == 0) {
			if (n + 3 < size) {
				memcpy(out + n, "mk2", 3);
				n += 3;
			}
			name += 4;
		} else {
			out[n++] = (char)lower((BYTE)*name++);
		}
	}
	out[n] = '\0';
}

BOOL extrarom_mame_lookup(const char *sha1, const char *machine, char *mame, UINT mame_size,
                          char *bios, UINT bios_size, BOOL *bad_dump) {
	char driver[32];
	int best = -1;
	int score = -1;
	UINT i;

	mame_machine((machine != NULL) ? machine : "", driver, sizeof(driver));
	for (i = 0; i < NELEMENTS(mame_table); i++) {
		int s;
		if (strcmp(sha1, mame_table[i].sha1) != 0) {
			continue;
		}
		/* Prefer this model's own entry, then one with a BIOS label. */
		s = ((strcmp(mame_table[i].machine, driver) == 0) ? 2 : 0) +
		    ((mame_table[i].bios[0] != '\0') ? 1 : 0);
		if (s > score) {
			best = (int)i;
			score = s;
		}
	}
	if (best < 0) {
		if (mame_size > 0) {
			mame[0] = '\0';
		}
		if (bios_size > 0) {
			bios[0] = '\0';
		}
		if (bad_dump != NULL) {
			*bad_dump = FALSE;
		}
		return FALSE;
	}
	if (mame_size > 0) {
		snprintf(mame, mame_size, "%s %s", mame_table[best].machine, mame_table[best].file);
	}
	if (bios_size > 0) {
		milstr_ncpy(bios, mame_table[best].bios, bios_size);
	}
	if (bad_dump != NULL) {
		*bad_dump = mame_table[best].bad_dump;
	}
	return TRUE;
}

/*
 * The version banners the ROMs carry, without the leading "NEC ", joined
 * with " + " when one ROM holds two BASICs (PC-8001mkII). Returns the count.
 */
UINT extrarom_banner(const BYTE *data, UINT size, char *banner, UINT banner_size) {
	static const char *const prefixes[] = {"N-88 BASIC Version ", "PC-8001 BASIC Ver ",
	                                       "N-80 BASIC Ver ", "N80SR-BASIC Ver "};
	UINT found = 0;
	UINT pos;
	UINT i;

	if (banner_size > 0) {
		banner[0] = '\0';
	}
	for (pos = 0; pos < size; pos++) {
		for (i = 0; i < NELEMENTS(prefixes); i++) {
			const UINT len = (UINT)strlen(prefixes[i]);
			UINT end = pos + len;
			UINT used;
			if ((end >= size) || (memcmp(data + pos, prefixes[i], len) != 0) || (data[end] < '0') ||
			    (data[end] > '9')) {
				continue;
			}
			while ((end < size) &&
			       (((data[end] >= '0') && (data[end] <= '9')) || (data[end] == '.'))) {
				end++;
			}
			while ((end > pos + len) && (data[end - 1] == '.')) {
				end--;
			}
			used = (UINT)strlen(banner);
			if ((found > 0) && (used + 3 < banner_size)) {
				milstr_ncat(banner, " + ", banner_size);
				used += 3;
			}
			if (used + (end - pos) < banner_size) {
				memcpy(banner + used, data + pos, end - pos);
				banner[used + (end - pos)] = '\0';
			}
			found++;
			pos = end - 1;
			break;
		}
	}
	return found;
}

static BOOL size_ok(UINT role, UINT32 size) {
	return (size == role_sizes[role][0]) ||
	       ((role_sizes[role][1] != 0) && (size == role_sizes[role][1]));
}

static BOOL is_blank(const BYTE *data, UINT32 size) {
	UINT32 i;

	for (i = 1; i < size; i++) {
		if (data[i] != data[0]) {
			return FALSE;
		}
	}
	return TRUE;
}

/* Role and revision of a file name: <role>.rom or <role>_v<label>.rom. */
static BOOL parse_name(const char *file, UINT *role, char *revision, UINT revision_size) {
	char stem[64];
	UINT len = (UINT)strlen(file);
	UINT i;

	if ((len <= 4) || (len - 4 >= sizeof(stem)) || !same_name(file + len - 4, ".rom")) {
		return FALSE;
	}
	for (i = 0; i < len - 4; i++) {
		stem[i] = (char)lower((BYTE)file[i]);
	}
	stem[len - 4] = '\0';
	for (i = 0; i < EXTRAROM_ROLES; i++) {
		if (strcmp(stem, extrarom_role_names[i]) == 0) {
			*role = i;
			revision[0] = '\0';
			return TRUE;
		}
	}
	for (i = 0; i < EXTRAROM_ROLES; i++) {
		const UINT n = (UINT)strlen(extrarom_role_names[i]);
		if ((strncmp(stem, extrarom_role_names[i], n) == 0) && (strncmp(stem + n, "_v", 2) == 0) &&
		    (stem[n + 2] != '\0') && (strlen(file + n + 2) - 4 < revision_size)) {
			*role = i;
			milstr_ncpy(revision, file + n + 2, revision_size);
			revision[strlen(revision) - 4] = '\0';
			return TRUE;
		}
	}
	return FALSE;
}

static void skip(EXTRAROM_MACHINE *out, const char *file, EXTRAROM_IGNORED reason) {
	if (out->ignored < EXTRAROM_MAX_IGNORED) {
		milstr_ncpy(out->skipped[out->ignored].file, file, sizeof(out->skipped[0].file));
		out->skipped[out->ignored].reason = reason;
	}
	out->ignored++;
}

static void add(EXTRAROM_MACHINE *out, const char *file, UINT role, const char *revision,
                const BYTE *data, UINT32 offset, UINT32 size) {
	EXTRAROM_FILE *f;
	ROMCHECKSUM sum;

	if (out->count >= EXTRAROM_MAX_FILES) {
		return;
	}
	f = &out->files[out->count++];
	ZeroMemory(f, sizeof(*f));
	milstr_ncpy(f->file, file, sizeof(f->file));
	f->role = role;
	milstr_ncpy(f->revision, revision, sizeof(f->revision));
	f->offset = offset;
	f->size = size;
	romcheck_buffer(data, size, &sum);
	romcheck_sha1_string(sum.sha1, f->sha1);
	if (size <= 0x10000) {
		extrarom_banner(data, size, f->banner, sizeof(f->banner));
	}
	extrarom_mame_lookup(f->sha1, out->name, f->mame, sizeof(f->mame), f->mame_bios,
	                     sizeof(f->mame_bios), &f->mame_bad_dump);
}

static BYTE *read_file(const char *path, UINT32 *size) {
	FILEH fh;
	BYTE *data;
	UINT32 n;

	*size = 0;
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return NULL;
	}
	n = file_getsize(fh);
	data = (n > 0) && (n <= 0x100000) ? (BYTE *)malloc(n) : NULL;
	if ((data != NULL) && (file_read(fh, data, n) != n)) {
		free(data);
		data = NULL;
	}
	file_close(fh);
	*size = n;
	return data;
}

/* Split a combined PC88.ROM into its parts (only roles not already given). */
static void add_pc88rom(EXTRAROM_MACHINE *out, const char *file, const BYTE *data, UINT32 size) {
	BYTE n80[0x8000];
	UINT i;

	for (i = 0; i < NELEMENTS(pc88rom_parts); i++) {
		UINT32 part = pc88rom_parts[i].size;
		if (pc88rom_parts[i].role == EXTRAROM_ROLE_DISK) {
			/* A 2 KiB DISK ROM is padded with FFh to 8 KiB. */
			UINT32 j;
			for (j = 0x800; (j < 0x2000) && (data[0x14000 + j] == 0xff); j++) {
			}
			if (j == 0x2000) {
				part = 0x800;
			}
		}
		if (is_blank(data + pc88rom_parts[i].offset, part)) {
			continue;
		}
		add(out, file, pc88rom_parts[i].role, "PC88.ROM", data + pc88rom_parts[i].offset,
		    pc88rom_parts[i].offset, part);
	}
	if (size == PC88ROM_FULL) {
		/* N-BASIC: its first 24 KiB at 16000h, its last 8 KiB at 8000h. */
		memcpy(n80, data + 0x16000, 0x6000);
		memcpy(n80 + 0x6000, data + 0x8000, 0x2000);
		if (!is_blank(n80, sizeof(n80))) {
			add(out, file, EXTRAROM_ROLE_N80, "PC88.ROM", n80, 0x16000, sizeof(n80));
		}
	}
}

static BOOL has_role(const EXTRAROM_MACHINE *m, const char *role, UINT len) {
	UINT i;

	for (i = 0; i < m->count; i++) {
		const char *name = extrarom_role_names[m->files[i].role];
		if ((strlen(name) == len) && (strncmp(name, role, len) == 0)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* Fill `missing` with the required roles (or alternatives) not present. */
static void check_required(EXTRAROM_MACHINE *m, const char *required) {
	const char *p = required;

	m->missing[0] = '\0';
	while (*p != '\0') {
		const char *end = strchr(p, ' ');
		const char *alt = p;
		BOOL found = FALSE;
		UINT len;
		if (end == NULL) {
			end = p + strlen(p);
		}
		while (alt < end) {
			const char *bar = (const char *)memchr(alt, '|', (size_t)(end - alt));
			const char *stop = (bar != NULL) ? bar : end;
			if (has_role(m, alt, (UINT)(stop - alt))) {
				found = TRUE;
			}
			alt = (bar != NULL) ? (bar + 1) : end;
		}
		if (!found) {
			len = (UINT)strlen(m->missing);
			if ((len > 0) && (len + 1 < sizeof(m->missing))) {
				m->missing[len++] = ' ';
				m->missing[len] = '\0';
			}
			if (len + (UINT)(end - p) < sizeof(m->missing)) {
				memcpy(m->missing + len, p, (size_t)(end - p));
				m->missing[len + (end - p)] = '\0';
			}
		}
		p = (*end == ' ') ? (end + 1) : end;
	}
}

/* Menu names: MAME's BIOS label, else the banner, else the size; the first
 * eight hex digits of the SHA-1 are added when two files of a role would
 * read the same. */
static void make_labels(EXTRAROM_MACHINE *m) {
	BOOL repeated[EXTRAROM_MAX_FILES];
	UINT i;
	UINT j;

	for (i = 0; i < m->count; i++) {
		EXTRAROM_FILE *f = &m->files[i];
		if (f->mame_bios[0] != '\0') {
			milstr_ncpy(f->label, f->mame_bios, sizeof(f->label));
		} else if (f->banner[0] != '\0') {
			milstr_ncpy(f->label, f->banner, sizeof(f->label));
		} else {
			snprintf(f->label, sizeof(f->label), "%u KiB", (unsigned)(f->size / 1024));
		}
	}
	/* Find every repeat before any label is changed. */
	for (i = 0; i < m->count; i++) {
		repeated[i] = FALSE;
		for (j = 0; j < m->count; j++) {
			if ((i != j) && (m->files[i].role == m->files[j].role) &&
			    (strcmp(m->files[i].label, m->files[j].label) == 0) &&
			    (strcmp(m->files[i].sha1, m->files[j].sha1) != 0)) {
				repeated[i] = TRUE;
			}
		}
	}
	for (i = 0; i < m->count; i++) {
		if (repeated[i]) {
			char tail[12];
			snprintf(tail, sizeof(tail), " [%.8s]", m->files[i].sha1);
			milstr_ncat(m->files[i].label, tail, sizeof(m->files[i].label));
		}
	}
}

static int compare_files(const void *a, const void *b) {
	const EXTRAROM_FILE *x = (const EXTRAROM_FILE *)a;
	const EXTRAROM_FILE *y = (const EXTRAROM_FILE *)b;

	if (x->role != y->role) {
		return (x->role < y->role) ? -1 : 1;
	}
	return strcmp(x->revision, y->revision);
}

BOOL extrarom_scan_machine(const char *dir, const char *name, EXTRAROM_MACHINE *out) {
	char path[MAX_PATH];
	char names[EXTRAROM_MAX_FILES * 2][64];
	UINT nnames = 0;
	FLINFO fli;
	FLISTH fl;
	UINT i;

	ZeroMemory(out, sizeof(*out));
	milstr_ncpy(out->name, name, sizeof(out->name));
	milstr_ncpy(out->dir, dir, sizeof(out->dir));
	fl = file_list1st(dir, &fli);
	if (fl == FLISTH_INVALID) {
		return FALSE;
	}
	do {
		if ((nnames < NELEMENTS(names)) && (strlen(fli.path) < sizeof(names[0])) &&
		    (strlen(fli.path) > 4) && same_name(fli.path + strlen(fli.path) - 4, ".rom")) {
			milstr_ncpy(names[nnames++], fli.path, sizeof(names[0]));
		}
	} while (file_listnext(fl, &fli) == SUCCESS);
	file_listclose(fl);
	qsort(names, nnames, sizeof(names[0]), (int (*)(const void *, const void *))strcmp);

	for (i = 0; i < nnames; i++) {
		const BOOL combined = same_name(names[i], "pc88.rom");
		UINT role = 0;
		char revision[24];
		BYTE *data;
		UINT32 size;
		if (!combined && !parse_name(names[i], &role, revision, sizeof(revision))) {
			skip(out, names[i], EXTRAROM_IGNORED_NAME);
			continue;
		}
		file_cpyname(path, dir, sizeof(path));
		file_setseparator(path, sizeof(path));
		file_catname(path, names[i], sizeof(path));
		data = read_file(path, &size);
		if (data == NULL) {
			skip(out, names[i], (size > 0x100000) ? EXTRAROM_IGNORED_SIZE : EXTRAROM_IGNORED_READ);
		} else if (combined ? ((size != PC88ROM_FULL) && (size != PC88ROM_SHORT))
		                    : !size_ok(role, size)) {
			skip(out, names[i], EXTRAROM_IGNORED_SIZE);
		} else if (is_blank(data, size)) {
			skip(out, names[i], EXTRAROM_IGNORED_BLANK);
		} else if (combined) {
			add_pc88rom(out, names[i], data, size);
		} else {
			add(out, names[i], role, revision, data, 0, size);
		}
		free(data);
	}
	qsort(out->files, out->count, sizeof(out->files[0]), compare_files);

	for (i = 0; i < NELEMENTS(machine_table); i++) {
		if (same_name(name, machine_table[i].name)) {
			out->known = TRUE;
			check_required(out, machine_table[i].required);
		}
	}
	make_labels(out);
	return TRUE;
}

void extrarom_dir(char *path, int size) {
	getbiospath(path, "extra", size);
}

static int machine_order(const char *name) {
	UINT i;

	for (i = 0; i < NELEMENTS(machine_table); i++) {
		if (same_name(name, machine_table[i].name)) {
			return (int)i;
		}
	}
	return (int)NELEMENTS(machine_table);
}

static int compare_machines(const void *a, const void *b) {
	const EXTRAROM_MACHINE *x = (const EXTRAROM_MACHINE *)a;
	const EXTRAROM_MACHINE *y = (const EXTRAROM_MACHINE *)b;
	const int ox = machine_order(x->name);
	const int oy = machine_order(y->name);

	return (ox != oy) ? (ox - oy) : strcmp(x->name, y->name);
}

/* Every directory below extra/, known models first in model order. */
UINT extrarom_scan(EXTRAROM_MACHINE *machines, UINT count) {
	char base[MAX_PATH];
	char dirs[EXTRAROM_MAX_MACHINES][64];
	UINT ndirs = 0;
	UINT found = 0;
	FLINFO fli;
	FLISTH fl;
	UINT i;

	extrarom_dir(base, sizeof(base));
	fl = file_list1st(base, &fli);
	if (fl == FLISTH_INVALID) {
		return 0;
	}
	do {
		char path[MAX_PATH];
		if ((fli.path[0] == '.') || (ndirs >= NELEMENTS(dirs)) ||
		    (strlen(fli.path) >= sizeof(dirs[0]))) {
			continue;
		}
		file_cpyname(path, base, sizeof(path));
		file_setseparator(path, sizeof(path));
		file_catname(path, fli.path, sizeof(path));
		if ((file_attr(path) != -1) && (file_attr(path) & FILEATTR_DIRECTORY)) {
			milstr_ncpy(dirs[ndirs++], fli.path, sizeof(dirs[0]));
		}
	} while (file_listnext(fl, &fli) == SUCCESS);
	file_listclose(fl);

	for (i = 0; (i < ndirs) && (found < count); i++) {
		char path[MAX_PATH];
		file_cpyname(path, base, sizeof(path));
		file_setseparator(path, sizeof(path));
		file_catname(path, dirs[i], sizeof(path));
		if (extrarom_scan_machine(path, dirs[i], &machines[found])) {
			found++;
		}
	}
	qsort(machines, found, sizeof(machines[0]), compare_machines);
	return found;
}

/* The files of one role, in name order (the default file has revision ""). */
UINT extrarom_revisions(const EXTRAROM_MACHINE *machine, UINT role, const EXTRAROM_FILE **files,
                        UINT count) {
	UINT n = 0;
	UINT i;

	for (i = 0; i < machine->count; i++) {
		if (machine->files[i].role == role) {
			if (n < count) {
				files[n] = &machine->files[i];
			}
			n++;
		}
	}
	return n;
}

/* Find "<machine>/<role>=" in the selection string. */
static char *find_selection(const char *machine, UINT role, size_t *length) {
	char key[96];
	char *p = np2cfg.extra_rom_select;
	const size_t keylen =
	    (size_t)snprintf(key, sizeof(key), "%s/%s=", machine, extrarom_role_names[role]);

	while (*p != '\0') {
		char *end = strchr(p, ';');
		const size_t len = (end != NULL) ? (size_t)(end - p) : strlen(p);
		if ((len >= keylen) && (strncmp(p, key, keylen) == 0)) {
			*length = len;
			return p;
		}
		p += len;
		if (*p == ';') {
			p++;
		}
	}
	return NULL;
}

/* The revision chosen in the menu, "" for the default. */
const char *extrarom_selected(const char *machine, UINT role) {
	static char value[24];
	size_t length;
	const char *p;
	size_t keylen;

	value[0] = '\0';
	if (role >= EXTRAROM_ROLES) {
		return value;
	}
	p = find_selection(machine, role, &length);
	if (p != NULL) {
		keylen = strlen(machine) + 1 + strlen(extrarom_role_names[role]) + 1;
		if (length - keylen < sizeof(value)) {
			memcpy(value, p + keylen, length - keylen);
			value[length - keylen] = '\0';
		}
	}
	return value;
}

/* Remember a revision for a role; "" goes back to the default. */
void extrarom_select(const char *machine, UINT role, const char *revision) {
	char *s = np2cfg.extra_rom_select;
	size_t length;
	char *p;
	char entry[160];

	if (role >= EXTRAROM_ROLES) {
		return;
	}
	p = find_selection(machine, role, &length);
	if (p != NULL) {
		if (p[length] == ';') {
			length++;
		}
		memmove(p, p + length, strlen(p + length) + 1);
		length = strlen(s);
		if ((length > 0) && (s[length - 1] == ';')) {
			s[length - 1] = '\0';
		}
	}
	if ((revision == NULL) || (revision[0] == '\0')) {
		return;
	}
	snprintf(entry, sizeof(entry), "%s%s/%s=%s", (s[0] != '\0') ? ";" : "", machine,
	         extrarom_role_names[role], revision);
	if (strlen(s) + strlen(entry) < sizeof(np2cfg.extra_rom_select)) {
		milstr_ncat(s, entry, sizeof(np2cfg.extra_rom_select));
	}
}

/* The file used for a role: the chosen revision, else <role>.rom, else the
 * part of PC88.ROM, else the first revision; NULL without any. */
const EXTRAROM_FILE *extrarom_choice(const EXTRAROM_MACHINE *machine, UINT role) {
	const EXTRAROM_FILE *files[EXTRAROM_MAX_FILES];
	const char *chosen = extrarom_selected(machine->name, role);
	const UINT n = extrarom_revisions(machine, role, files, EXTRAROM_MAX_FILES);
	UINT i;

	if (n == 0) {
		return NULL;
	}
	if (chosen[0] != '\0') {
		for (i = 0; i < n; i++) {
			if (strcmp(files[i]->revision, chosen) == 0) {
				return files[i];
			}
		}
	}
	for (i = 0; i < n; i++) {
		if (files[i]->revision[0] == '\0') {
			return files[i];
		}
	}
	for (i = 0; i < n; i++) {
		if (strcmp(files[i]->revision, "PC88.ROM") == 0) {
			return files[i];
		}
	}
	return files[0];
}

/* Read the ROM `file` describes; `size` must be its size. */
BOOL extrarom_read(const EXTRAROM_MACHINE *machine, const EXTRAROM_FILE *file, BYTE *data,
                   UINT size) {
	char path[MAX_PATH];
	BYTE *whole;
	UINT32 whole_size;
	BOOL ok;

	if ((file == NULL) || (size != file->size)) {
		return FALSE;
	}
	file_cpyname(path, machine->dir, sizeof(path));
	file_setseparator(path, sizeof(path));
	file_catname(path, file->file, sizeof(path));
	whole = read_file(path, &whole_size);
	if (whole == NULL) {
		return FALSE;
	}
	ok = TRUE;
	if ((file->role == EXTRAROM_ROLE_N80) && (strcmp(file->revision, "PC88.ROM") == 0)) {
		ok = (whole_size == PC88ROM_FULL);
		if (ok) {
			memcpy(data, whole + 0x16000, 0x6000);
			memcpy(data + 0x6000, whole + 0x8000, 0x2000);
		}
	} else if (file->offset + size <= whole_size) {
		memcpy(data, whole + file->offset, size);
	} else {
		ok = FALSE;
	}
	if (ok) {
		ROMCHECKSUM sum;
		char sha1[41];
		romcheck_buffer(data, size, &sum);
		romcheck_sha1_string(sum.sha1, sha1);
		ok = (strcmp(sha1, file->sha1) == 0); /* changed since the scan */
	}
	free(whole);
	return ok;
}

/* --list-extra-roms: what the 起動 menu shows, for scripts and support. */
int extrarom_list(void) {
	static const char *const reasons[] = {"unknown role", "wrong size", "blank", "unreadable"};
	EXTRAROM_MACHINE *machines;
	char base[MAX_PATH];
	UINT count;
	UINT i;
	UINT j;

	machines = (EXTRAROM_MACHINE *)malloc(sizeof(*machines) * EXTRAROM_MAX_MACHINES);
	if (machines == NULL) {
		return FAILURE;
	}
	extrarom_dir(base, sizeof(base));
	count = extrarom_scan(machines, EXTRAROM_MAX_MACHINES);
	printf("extra ROM directory: %s (%u machine%s)\n", base, count, (count == 1) ? "" : "s");
	for (i = 0; i < count; i++) {
		const EXTRAROM_MACHINE *m = &machines[i];
		printf("%s: %s%s\n", m->name,
		       !m->known ? "unknown model" : ((m->missing[0] != '\0') ? "missing " : "complete"),
		       m->known ? m->missing : "");
		for (j = 0; j < m->count; j++) {
			const EXTRAROM_FILE *f = &m->files[j];
			printf("  %-8s %-16s %6u %s %s%s%s%s%s\n", extrarom_role_names[f->role], f->file,
			       (unsigned)f->size, f->sha1, f->label, (f->mame[0] != '\0') ? " | MAME " : "",
			       f->mame, f->mame_bad_dump ? " BAD_DUMP" : "",
			       (extrarom_choice(m, f->role) == f) ? " *" : "");
		}
		for (j = 0; (j < m->ignored) && (j < EXTRAROM_MAX_IGNORED); j++) {
			printf("  ignored  %s (%s)\n", m->skipped[j].file, reasons[m->skipped[j].reason]);
		}
	}
	free(machines);
	return SUCCESS;
}

/* M113: how a model directory is started; FALSE for an unknown name. */
const char *extrarom_machine_name(const char *name) {
	UINT i;

	for (i = 0; i < NELEMENTS(machine_table); i++) {
		if (same_name(name, machine_table[i].name)) return machine_table[i].name;
	}
	return NULL;
}

BOOL extrarom_profile(const char *name, EXTRAROM_PROFILE *out) {
	UINT i;

	for (i = 0; i < NELEMENTS(machine_table); i++) {
		if (same_name(name, machine_table[i].name)) {
			out->base_model = (machine_table[i].base == 1) ? str_VA1 : str_VA2;
			out->startable = (machine_table[i].base != 0) ? TRUE : FALSE;
			out->v2 = machine_table[i].v2 ? TRUE : FALSE;
			out->high = machine_table[i].high ? TRUE : FALSE;
			out->n_role = machine_table[i].n_role;
			return TRUE;
		}
	}
	return FALSE;
}

/* A machine that can be started: a known, startable model with N88-BASIC.
 * Other missing roles do not prevent a start (maintainer, M113); they read
 * as FFh, or leave no FDD unit or no N mode. */
BOOL extrarom_startable(const EXTRAROM_MACHINE *machine) {
	EXTRAROM_PROFILE profile;

	return machine->known && extrarom_profile(machine->name, &profile) && profile.startable &&
	       (extrarom_choice(machine, profile.n_role ? profile.n_role : (UINT)EXTRAROM_ROLE_N88) !=
	        NULL);
}

static BOOL apply_role(const EXTRAROM_MACHINE *m, UINT role, BYTE *data, UINT size) {
	const EXTRAROM_FILE *f = extrarom_choice(m, role);

	return (f != NULL) && (f->size == size) && extrarom_read(m, f, data, size);
}

/*
 * M114: a PC-8001 model: its N-BASIC ROM (24 KiB, the 6000h-7FFFh socket
 * reading FFh, or 32 KiB) replaces N-BASIC, the 8x8 characters come from
 * font.rom, the external FDD unit runs disk.rom when one is given, and the
 * machine starts in N mode. N88-BASIC, its banks and the kanji ROMs stay
 * the VA's (the PC-8001 family has neither).
 */
static void apply_pc8001(const EXTRAROM_MACHINE *machine, UINT n_role) {
	const EXTRAROM_FILE *f = extrarom_choice(machine, n_role);
	UINT i;

	FillMemory(memoryva_n80, sizeof(memoryva_n80), 0xff);
	memoryva_n80_exist = (f != NULL) && (f->size <= sizeof(memoryva_n80)) &&
	                     extrarom_read(machine, f, memoryva_n80, f->size);
	milstr_ncpy(memoryva_n80_file, memoryva_n80_exist ? f->file : "", sizeof(memoryva_n80_file));
	f = extrarom_choice(machine, EXTRAROM_ROLE_DISK);
	if ((f != NULL) && extrarom_read(machine, f, subsystem.rom, f->size)) {
		for (i = f->size; i < sizeof(subsystem.rom); i++) {
			subsystem.rom[i] = subsystem.rom[i % f->size];
		}
		subsystem.romexist = TRUE;
	} else {
		subsystemmx_set_absent(TRUE);
	}
	(void)apply_role(machine, EXTRAROM_ROLE_FONT, fontmem + 0x41000, 0x800);
	np2cfg.boot_mode = BOOTMODE_N;
	memoryva_pc8001 = TRUE;
	/* The PC-8001 family programs its CRTC for a 15 kHz monitor (10-raster
	 * rows at 200 lines); a 24 kHz VA shows them at half height. Run at
	 * 15 kHz (SW1) for this machine; videova_reset() ran before this hook. */
	videova.crtmode = 0;
	fprintf(stderr, "INFO: PC-8001 machine %s (N mode)\n", machine->name);
}

/*
 * M113: romva_machine_hook. With np2cfg.extra_machine set, replace the ROM
 * images V1/V2 mode reads with that machine's ROMs: N88-BASIC and its banks,
 * N-BASIC, the FDD unit's ROM (none for a PC-8801 without disk.rom), the
 * kanji ROMs, the 8x8 characters and the dictionary. The VA firmware still
 * starts the machine. Roles a machine lacks read as FFh.
 */
void extrarom_apply(void) {
	static EXTRAROM_MACHINE machine;
	EXTRAROM_PROFILE profile;
	static BYTE kanji[2][0x20000];
	static BYTE n80[0x8000];
	char base[MAX_PATH];
	char dir[MAX_PATH];
	const EXTRAROM_FILE *f;
	UINT i;

	extrarom_last_error[0] = '\0';
	if (np2cfg.extra_machine[0] == '\0') {
		return;
	}
	extrarom_dir(base, sizeof(base));
	file_cpyname(dir, base, sizeof(dir));
	file_setseparator(dir, sizeof(dir));
	file_catname(dir, np2cfg.extra_machine, sizeof(dir));
	if (!extrarom_scan_machine(dir, np2cfg.extra_machine, &machine) ||
	    !extrarom_startable(&machine)) {
		snprintf(extrarom_last_error, sizeof(extrarom_last_error), "%.60s: no BASIC ROM in extra/",
		         np2cfg.extra_machine);
		fprintf(stderr, "WARNING: %s; the VA ROMs are used\n", extrarom_last_error);
		return;
	}
	if (extrarom_profile(machine.name, &profile) && profile.n_role) {
		apply_pc8001(&machine, profile.n_role);
		return;
	}
	if (!apply_role(&machine, EXTRAROM_ROLE_N88, rom1mem + 0x10000, 0x8000)) {
		snprintf(extrarom_last_error, sizeof(extrarom_last_error), "%s: n88 unreadable",
		         machine.name);
		fprintf(stderr, "WARNING: %s; the VA ROMs are used\n", extrarom_last_error);
		return;
	}
	FillMemory(rom1mem + 0x18000, 0x8000, 0xff);
	for (i = 0; i < 4; i++) {
		(void)apply_role(&machine, EXTRAROM_ROLE_N88_0 + i, rom1mem + 0x18000 + i * 0x2000, 0x2000);
	}
	memoryva_n80_exist = apply_role(&machine, EXTRAROM_ROLE_N80, n80, sizeof(n80));
	if (memoryva_n80_exist) {
		CopyMemory(memoryva_n80, n80, sizeof(n80));
		snprintf(memoryva_n80_file, sizeof(memoryva_n80_file), "%s", "n80.rom");
	} else {
		memoryva_n80_file[0] = '\0';
	}
	/* The FDD unit: 8 KiB, or 2 KiB repeated over the 8 KiB ROM space. */
	f = extrarom_choice(&machine, EXTRAROM_ROLE_DISK);
	if ((f != NULL) && extrarom_read(&machine, f, subsystem.rom, f->size)) {
		for (i = f->size; i < sizeof(subsystem.rom); i++) {
			subsystem.rom[i] = subsystem.rom[i % f->size];
		}
		subsystem.romexist = TRUE;
	} else {
		subsystemmx_set_absent(TRUE);
	}
	for (i = 0; i < 2; i++) {
		if (!apply_role(&machine, EXTRAROM_ROLE_KANJI1 + i, kanji[i], 0x20000)) {
			FillMemory(kanji[i], 0x20000, 0xff);
		}
		cgrom88_kanji[i] = kanji[i];
	}
	/* 8x8 characters: font.rom, else kanji1.rom 1000h-17FFh (MAME). */
	if (!apply_role(&machine, EXTRAROM_ROLE_FONT, fontmem + 0x41000, 0x800) &&
	    (extrarom_choice(&machine, EXTRAROM_ROLE_KANJI1) != NULL)) {
		CopyMemory(fontmem + 0x41000, kanji[0] + 0x1000, 0x800);
	}
	if (!apply_role(&machine, EXTRAROM_ROLE_JISYO, dicmem, 0x80000)) {
		FillMemory(dicmem, 0x80000, 0xff);
	}
	if (np2cfg.boot_mode == BOOTMODE_AUTO) {
		np2cfg.boot_mode = BOOTMODE_V1V2;
	}
	/* M129: the PC-88VA/VA2 sets are VA ROMs on the VA and keep its display. */
	memoryva_pc8801 = !same_name(machine.name, "PC-88VA") && !same_name(machine.name, "PC-88VA2");
	fprintf(stderr, "INFO: PC-8801 machine %s from %s\n", machine.name, dir);
}
