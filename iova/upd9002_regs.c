/*
 * upd9002_regs.c: PC-88VA CPU port
 */


#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"upd9002_regs.h"


	UPD9002_REGS	upd9002_regs = {0};



static void IOOUTCALL upd9002_offf0(UINT port, REG8 dat) {
	upd9002_regs.tcks = dat;
	pit_ontckschanged();
}



static REG8 IOINPCALL upd9002_ifff0(UINT port) {
	(void)port;
	return(upd9002_regs.tcks);
}


// ---- I/F

void upd9002_regs_reset(void) {
	ZeroMemory(&upd9002_regs, sizeof(upd9002_regs));
}

void upd9002_regs_bind(void) {

	iocoreva_attachout(0xfff0, upd9002_offf0);
	iocoreva_attachinp(0xfff0, upd9002_ifff0);
}
