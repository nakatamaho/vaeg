/*
 *	BKMEMVA.C: PC-88VA Backup memory
 *	ToDo:
 *		ファイル読み書き失敗時のエラー通知
 */


#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memoryva.h"
#include	"bkupmemva.h"

#if defined(SUPPORT_PC88VA)


#define VABKUPMEM "vabkupmem.dat"

static BOOL bkupmemva_read(const char *path) {

	FILEH	fh;
	BOOL	ret;

	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		ret = (file_read(fh, backupmem, 0x04000) == 0x04000);
		file_close(fh);
		return(ret);
	}
	return(FAILURE);
}

void bkupmemva_load(void) {
	char	path[MAX_PATH];

	file_getstatepath(path, sizeof(path), VABKUPMEM);
	if (bkupmemva_read(path) == SUCCESS) {
		return;
	}
	getbiospath(path, VABKUPMEM, sizeof(path));
	(void)bkupmemva_read(path);
}

void bkupmemva_save(void) {
	char	path[MAX_PATH];
	FILEH	fh;

	file_getstatepath(path, sizeof(path), VABKUPMEM);
	fh = file_create(path);
	if (fh != FILEH_INVALID) {
		(void)file_write(fh, backupmem, 0x04000);
		file_close(fh);
	}

}

#endif
