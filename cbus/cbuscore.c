#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "cbuscore.h"
#include "sasiio.h"
#include "scsiio.h"
#include "mpu98ii.h"
#include "bmsio.h"

static const IOCBFN resetfn[] = {
    sasiio_reset,
    scsiio_reset,
    mpu98ii_reset,
    bmsio_reset,
};

static const IOCBFN bindfn[] = {
    sasiio_bind,
    scsiio_bind,
    mpu98ii_bind,
    bmsio_bind,
};

void cbuscore_reset(void) {
	iocore_cb(resetfn, sizeof(resetfn) / sizeof(IOCBFN));
}

void cbuscore_bind(void) {
	iocore_cb(bindfn, sizeof(bindfn) / sizeof(IOCBFN));
}
