/*
 *	BKMEMVA.C: PC-88VA Backup memory
 *	ToDo:
 *		ファイル読み書き失敗時のエラー通知
 */

#include "compiler.h"
#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "bkupmemva.h"

#define VABKUPMEM "vabkupmem.dat"

static char bkupmemva_path[MAX_PATH];
static BOOL bkupmemva_enabled = TRUE;

void bkupmemva_setpath(const char *path) {
	file_cpyname(bkupmemva_path, (path != NULL) ? path : "", sizeof(bkupmemva_path));
}

void bkupmemva_setenabled(BOOL enabled) {
	bkupmemva_enabled = enabled ? TRUE : FALSE;
}

static int bkupmemva_read(const char *path) {
	FILEH fh;

	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		int result;

		result = (file_read(fh, backupmem, 0x04000) == 0x04000) ? SUCCESS : FAILURE;
		file_close(fh);
		return result;
	}
	return FAILURE;
}

static void bkupmemva_statepath(char *path, int size) {
	if (bkupmemva_path[0] != '\0') {
		file_cpyname(path, bkupmemva_path, size);
		return;
	}
	file_cpyname(path, VABKUPMEM, size);
}

static void bkupmemva_initialize_mainram(void) {
	UINT8 capacity_code;
	UINT8 checksum;
	int i;

	capacity_code = (UINT8)((pccore_mainram_kb() / 128) - 1);
	ZeroMemory(backupmem, 0x04000);
	/* VA BIOS backup record: memory settings, signature, reserved bytes, checksum. */
	backupmem[0x1fc2] = 0xdb;
	backupmem[0x1fc3] = 0x02;
	backupmem[0x1fc4] = (BYTE)(0x60 | capacity_code);
	backupmem[0x1fc5] = 0x04;
	backupmem[0x1fc6] = 0xf9;
	backupmem[0x1fc7] = 0x0a;
	backupmem[0x1fc8] = 0x4b;
	backupmem[0x1fc9] = 0x5a;
	backupmem[0x1fca] = 0x4d;
	backupmem[0x1fcb] = 0xff;
	backupmem[0x1fcc] = 0xff;
	checksum = 0;
	for (i = 0; i < 8; i++) {
		checksum = (UINT8)(checksum + backupmem[0x1fc0 + i]);
	}
	backupmem[0x1fcd] = checksum;
}

void bkupmemva_load(void) {
	char path[MAX_PATH];

	if (!bkupmemva_enabled) {
		return;
	}
	/* A missing model-specific file starts with a clean backup-RAM image. */
	ZeroMemory(backupmem, 0x04000);
	bkupmemva_statepath(path, sizeof(path));
	if (bkupmemva_read(path) != SUCCESS) {
		/* Seed the BIOS selection from the installed physical-RAM ceiling. */
		bkupmemva_initialize_mainram();
	}
}

void bkupmemva_save(void) {
	char path[MAX_PATH];
	FILEH fh;

	if (!bkupmemva_enabled) {
		return;
	}
	bkupmemva_statepath(path, sizeof(path));
	fh = file_create(path);
	if (fh != FILEH_INVALID) {
		(void)file_write(fh, backupmem, 0x04000);
		file_close(fh);
	}
}
