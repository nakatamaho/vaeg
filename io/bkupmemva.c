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

static BOOL bkupmemva_read(const char *path) {
	FILEH fh;
	BOOL ret;

	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		ret = (file_read(fh, backupmem, 0x04000) == 0x04000);
		file_close(fh);
		return (ret);
	}
	return (FAILURE);
}

static void bkupmemva_statepath(char *path, int size) {
	if (bkupmemva_path[0] != '\0') {
		file_cpyname(path, bkupmemva_path, size);
		return;
	}
	file_cpyname(path, VABKUPMEM, size);
}

void bkupmemva_load(void) {
	char path[MAX_PATH];

	if (!bkupmemva_enabled) {
		return;
	}
	/* A missing model-specific file starts with a clean backup-RAM image. */
	ZeroMemory(backupmem, 0x04000);
	bkupmemva_statepath(path, sizeof(path));
	(void)bkupmemva_read(path);
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
