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
#include "diagnostics/causal_trace.h"

_SUBSYSTEMMXCFG subsystemmxcfg = {0};

// ---- I/F

void subsystemmx_initialize(void) {
	vaeg_causal_trace_named("device_schedule", "machine", "fd-subsystem",
	                       "initialize", 0, 0, 0);
	if (subsystemmxcfg.mockup) {
	} else {
		subsystem_initialize();
		subsystemif_initialize();
	}
}

void subsystemmx_reset(void) {
	vaeg_causal_trace_named("device_schedule", "machine", "fd-subsystem",
	                       "reset", 0, 0, 0);
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
	if (subsystemmxcfg.mockup) {
	} else {
		subsystem_exec();
	}
}
