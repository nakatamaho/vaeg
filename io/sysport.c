#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "sound.h"
#include "beep.h"

/*
 * Native VA system ports use the shared sysport latch for serial and beeper
 * state. The PC-98 0031H-0037H decoder was removed; sysportva owns all guest
 * access to this state.
 */
void systemport_reset(void) {
	sysport.c = 0xf9;
	beep_oneventset();
}

void systemport_bind(void) {
}
