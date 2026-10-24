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
#ifndef VAEG_SDL2_EXTRAROM_H
#define VAEG_SDL2_EXTRAROM_H

/*
 * M112: user-supplied ROMs of the PC-8801 and PC-8001 families in
 * extra/<machine>/ below the ROM directory, one file per role (n88.rom ...
 * font.rom), revisions as <role>_v<label>.rom, and a combined PC88.ROM
 * that is split. See docs/modernization/extra-roms.md.
 */

enum {
	EXTRAROM_ROLE_N88 = 0,
	EXTRAROM_ROLE_N88_0,
	EXTRAROM_ROLE_N88_1,
	EXTRAROM_ROLE_N88_2,
	EXTRAROM_ROLE_N88_3,
	EXTRAROM_ROLE_N80,
	EXTRAROM_ROLE_N80_2,
	EXTRAROM_ROLE_N80_2SR,
	EXTRAROM_ROLE_N80_3,
	EXTRAROM_ROLE_DISK,
	EXTRAROM_ROLE_KANJI1,
	EXTRAROM_ROLE_KANJI2,
	EXTRAROM_ROLE_JISYO,
	EXTRAROM_ROLE_CDBIOS,
	EXTRAROM_ROLE_FONT,
	EXTRAROM_ROLE_FONT80SR,
	EXTRAROM_ROLE_FONTHIRA,
	EXTRAROM_ROLES
};

enum {
	EXTRAROM_MAX_MACHINES = 32,
	EXTRAROM_MAX_FILES = 32,
	EXTRAROM_MAX_IGNORED = 8
};

/* Why a file in a machine directory is not used. */
typedef enum {
	EXTRAROM_IGNORED_NAME = 0, /* *.rom that names no role */
	EXTRAROM_IGNORED_SIZE,     /* wrong size for its role */
	EXTRAROM_IGNORED_BLANK,    /* every byte the same */
	EXTRAROM_IGNORED_READ      /* could not be read */
} EXTRAROM_IGNORED;

typedef struct {
	char file[64];     /* "n80_v1.01.rom", "PC88.ROM" */
	UINT role;         /* EXTRAROM_ROLE_* */
	char revision[24]; /* "" for <role>.rom, the label of <role>_v<label>.rom, or "PC88.ROM" */
	UINT32 offset;     /* of this ROM within the file (PC88.ROM) */
	UINT32 size;
	char sha1[41];
	char banner[64];    /* "N-88 BASIC Version 1.9", "" without one */
	char mame[64];      /* "pc8001 n80v101.rom", "" when MAME does not list it */
	char mame_bios[32]; /* MAME's BIOS label ("N-BASIC v1.01"), or "" */
	BOOL mame_bad_dump;
	char label[96]; /* name for the menu: MAME BIOS label, banner, or size */
} EXTRAROM_FILE;

typedef struct {
	char file[64];
	EXTRAROM_IGNORED reason;
} EXTRAROM_SKIP;

typedef struct {
	char name[64];      /* directory name, "PC-8801MA" */
	char dir[MAX_PATH]; /* its path */
	BOOL known;         /* a model vaeg knows the required roles of */
	UINT count;
	EXTRAROM_FILE files[EXTRAROM_MAX_FILES];
	UINT ignored;
	EXTRAROM_SKIP skipped[EXTRAROM_MAX_IGNORED];
	char missing[128]; /* required roles not found, "n80_2sr font80sr" */
} EXTRAROM_MACHINE;

/* M113: how a model directory is started. */
typedef struct {
	const char *base_model; /* str_VA1 or str_VA2 */
	BOOL startable;         /* a model vaeg can start */
	BOOL v2;                /* N88-BASIC V2 */
	BOOL high;              /* 8 MHz (H) as well as 4 MHz (S) */
	UINT n_role;            /* M114: PC-8001 family: its N-BASIC role; 0 otherwise */
} EXTRAROM_PROFILE;

#ifdef __cplusplus
extern "C" {
#endif

extern const char *const extrarom_role_names[EXTRAROM_ROLES];
extern char extrarom_last_error[160];

void extrarom_dir(char *path, int size);
UINT extrarom_scan(EXTRAROM_MACHINE *machines, UINT count);
BOOL extrarom_scan_machine(const char *dir, const char *name, EXTRAROM_MACHINE *out);
UINT extrarom_banner(const BYTE *data, UINT size, char *banner, UINT banner_size);
BOOL extrarom_mame_lookup(const char *sha1, const char *machine, char *mame, UINT mame_size,
                          char *bios, UINT bios_size, BOOL *bad_dump);
UINT extrarom_revisions(const EXTRAROM_MACHINE *machine, UINT role, const EXTRAROM_FILE **files,
                        UINT count);
const EXTRAROM_FILE *extrarom_choice(const EXTRAROM_MACHINE *machine, UINT role);
const char *extrarom_selected(const char *machine, UINT role);
void extrarom_select(const char *machine, UINT role, const char *revision);
BOOL extrarom_read(const EXTRAROM_MACHINE *machine, const EXTRAROM_FILE *file, BYTE *data,
                   UINT size);
int extrarom_list(void);
const char *extrarom_machine_name(const char *name);
BOOL extrarom_profile(const char *name, EXTRAROM_PROFILE *out);
BOOL extrarom_startable(const EXTRAROM_MACHINE *machine);
void extrarom_apply(void);

#ifdef __cplusplus
}
#endif

#endif
