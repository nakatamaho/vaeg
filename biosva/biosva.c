/*
 *	biosva.c: PC-88VA ROM control
 *	ToDo:
 *		ファイル読み込み失敗時のエラー通知
 */

#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memoryva.h"
#include	"biosva.h"
#include	"subsystem.h"


#define VAFONTROM "vafont.rom"
#define VADICROM  "vadic.rom"
#define VAROM00ROM "varom00.rom"
#define VAROM08ROM "varom08.rom"
#define VAROM1ROM  "varom1.rom"
#define VAFONTROM_VA2 "vafont_va2.rom"
#define VADICROM_VA2  "vadic_va2.rom"
#define VAROM00ROM_VA2 "varom00_va2.rom"
#define VAROM08ROM_VA2 "varom08_va2.rom"
#define VAROM1ROM_VA2  "varom1_va2.rom"
#define VASUBSYSROM "vasubsys.rom"

/* VA2 names follow MAME's pc88va2 ROM set; do not fall back to VA names. */
static const char *modelrom(const char *va, const char *va2) {

	return((pccore.model_va == PCMODEL_VA2) ? va2 : va);
}

void biosva_initialize(void) {
	char	path[MAX_PATH];
	FILEH	fh;
	BOOL	success;

	memoryva.rom0exist = 0;
	memoryva.rom1exist = 0;
	memoryva.sysmromexist = 0;
	subsystem.romexist = FALSE;

	getbiospath(path, modelrom(VAFONTROM, VAFONTROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, fontmem, 0x50000) == 0x50000);
		if (success) memoryva.sysmromexist |= 0x300;	// bank 8,9
		file_close(fh);
	}

	getbiospath(path, modelrom(VADICROM, VADICROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, dicmem, 0x80000) == 0x80000);
		if (success) memoryva.sysmromexist |= 0x3000;	// bank C,D
		file_close(fh);
	}

	getbiospath(path, modelrom(VAROM00ROM, VAROM00ROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom0mem, 0x80000) == 0x80000);
		if (success) memoryva.rom0exist |= 0xff;		// bank 0-7
		file_close(fh);
	}

	getbiospath(path, modelrom(VAROM08ROM, VAROM08ROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom0mem + 0x80000, 0x20000) == 0x20000);
		if (success) memoryva.rom0exist |= 0x300;		// bank 8,9
		file_close(fh);
	}

	getbiospath(path, modelrom(VAROM1ROM, VAROM1ROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom1mem, 0x20000) == 0x20000);
		if (success) memoryva.rom1exist |= 0x03;		// bank 0,1
		file_close(fh);
	}

	getbiospath(path, VASUBSYSROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, subsystem.rom, 0x2000) == 0x2000);
		if (success) subsystem.romexist = TRUE;
		file_close(fh);
	}


}
