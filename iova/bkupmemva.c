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

static void bkupmemva_statepath(char *path, int size) {

#if defined(OSLANG_SJIS)
	getbiospath(path, VABKUPMEM, size);
#elif defined(OSLANG_UTF8)
	file_getstatepath(path, size, VABKUPMEM);
#else
	getbiospath(path, VABKUPMEM, size);
#endif
}

void bkupmemva_load(void) {
	char	path[MAX_PATH];

	bkupmemva_statepath(path, sizeof(path));
	if (bkupmemva_read(path) == SUCCESS) {
		return;
	}
#if defined(OSLANG_UTF8) && !defined(OSLANG_SJIS)
	getbiospath(path, VABKUPMEM, sizeof(path));
	(void)bkupmemva_read(path);
#endif
}

void bkupmemva_save(void) {
	char	path[MAX_PATH];
	FILEH	fh;

	bkupmemva_statepath(path, sizeof(path));
	fh = file_create(path);
	if (fh != FILEH_INVALID) {
		(void)file_write(fh, backupmem, 0x04000);
		file_close(fh);
	}

}

#endif
