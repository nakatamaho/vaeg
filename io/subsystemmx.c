/*
 * subsystemmx.c: PC-88VA FD Sub System (multiplexer)
 *
 */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "subsystemif.h"
#include "fdsubsys.h"
#include "subsystemmx.h"
#include "subsystem.h"

_SUBSYSTEMMXCFG subsystemmxcfg = {0};

// ---- I/F

void subsystemmx_initialize(void) {
	if (subsystemmxcfg.mockup) {
	} else {
		subsystem_initialize();
		subsystemif_initialize();
	}
}

void subsystemmx_reset(void) {
	if (subsystemmxcfg.mockup) {
		fdsubsys_reset();
	} else {
		subsystem_reset();
		subsystemif_reset();
	}
}

void subsystemmx_bind(void) {
	if (subsystemmxcfg.mockup) {
		fdsubsys_bind();
	} else {
		subsystemif_bind();
	}
}

void subsystemmx_exec(void) {
	if (subsystemmxcfg.absent) {
	} else if (subsystemmxcfg.mockup) {
	} else {
		subsystem_exec();
	}
}

/* M113: a PC-8801 without an FDD unit: ports FCh-FFh read FFh and ignore
 * writes, and the unit does not run. Rebinds the ports; call after
 * iocore_bind(). */
static void IOOUTCALL subsystemmx_o_absent(UINT port, REG8 dat) {
	(void)port;
	(void)dat;
}

static REG8 IOINPCALL subsystemmx_i_absent(UINT port) {
	(void)port;
	return 0xff;
}

void subsystemmx_set_absent(BOOL absent) {
	UINT port;

	subsystemmxcfg.absent = absent;
	if (absent) {
		for (port = 0x0fc; port <= 0x0ff; port++) {
			iocore_attachout(port, subsystemmx_o_absent);
			iocore_attachinp(port, subsystemmx_i_absent);
		}
	}
}
