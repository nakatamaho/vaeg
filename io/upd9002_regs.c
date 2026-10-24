/*
 * upd9002_regs.c: PC-88VA CPU port
 */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "upd9002_regs.h"

UPD9002_REGS upd9002_regs = {0};
UPD9002_IOTRAP upd9002_iotrap = {0};
UPD9002_WCU upd9002_wcu = {0};
UPD9002_WAITMAP upd9002_waitmap = {0};

/* WMB's 3-bit partition sizes (V50). */
static const UINT32 wcu_block_size[8] = {0x08000, 0x10000, 0x18000, 0x20000,
                                         0x30000, 0x40000, 0x60000, 0x80000};

void upd9002_wcu_update(void) {
	upd9002_waitmap.lower_end = wcu_block_size[(upd9002_wcu.wmb >> 4) & 7];
	upd9002_waitmap.upper_start = 0x100000 - wcu_block_size[upd9002_wcu.wmb & 7];
	upd9002_waitmap.lower = (BYTE)(upd9002_wcu.wcy1 & 3);
	upd9002_waitmap.middle = (BYTE)((upd9002_wcu.wcy1 >> 2) & 3);
	upd9002_waitmap.upper = (BYTE)((upd9002_wcu.wcy1 >> 4) & 3);
	upd9002_waitmap.active =
	    (BYTE)((upd9002_waitmap.lower | upd9002_waitmap.middle | upd9002_waitmap.upper) != 0);
}

/* A state saved before M130 has no WCU section: those builds inserted no
 * programmed memory waits, so keep that until the section says otherwise. */
void upd9002_wcu_legacy(void) {
	upd9002_wcu.wmb = 0x53;
	upd9002_wcu.wcy1 = 0x00;
	upd9002_wcu.wcy2 = 0x00;
	upd9002_wcu.reserved = 0;
	upd9002_wcu_update();
}

static void IOOUTCALL upd9002_offf4(UINT port, REG8 dat) {
	switch (port) {
	case 0xfff4:
		upd9002_wcu.wmb = (BYTE)(dat & 0x77);
		break;
	case 0xfff5:
		upd9002_wcu.wcy1 = (BYTE)dat;
		break;
	default:
		upd9002_wcu.wcy2 = (BYTE)(dat & 0x0f);
		break;
	}
	upd9002_wcu_update();
}

static void IOOUTCALL upd9002_iotrap_range_out(UINT port, REG8 dat) {
	upd9002_iotrap.ranges[port - 0xffe0] = (BYTE)dat;
}

static void IOOUTCALL upd9002_iotrap_control_out(UINT port, REG8 dat) {
	upd9002_iotrap.control = (BYTE)dat;
	(void)port;
}

static void IOOUTCALL upd9002_offf0(UINT port, REG8 dat) {
	upd9002_regs.tcks = dat;
	pit_ontckschanged();
}

static REG8 IOINPCALL upd9002_ifff0(UINT port) {
	(void)port;
	return (upd9002_regs.tcks);
}

// ---- I/F

void upd9002_regs_reset(void) {
	ZeroMemory(&upd9002_regs, sizeof(upd9002_regs));
	ZeroMemory(&upd9002_iotrap, sizeof(upd9002_iotrap));
	upd9002_wcu.wmb = 0x77; /* V50 power-on values: 3 waits everywhere */
	upd9002_wcu.wcy1 = 0xff;
	upd9002_wcu.wcy2 = 0x0f;
	upd9002_wcu.reserved = 0;
	upd9002_wcu_update();
}

void upd9002_regs_bind(void) {
	UINT port;
	for (port = 0xffe0; port <= 0xffe7; port++) {
		iocore_attachout(port, upd9002_iotrap_range_out);
	}
	iocore_attachout(0xffef, upd9002_iotrap_control_out);
	iocore_attachout(0xfff0, upd9002_offf0);
	iocore_attachinp(0xfff0, upd9002_ifff0);
	iocore_attachout(0xfff4, upd9002_offf4);
	iocore_attachout(0xfff5, upd9002_offf4);
	iocore_attachout(0xfff6, upd9002_offf4);
}
