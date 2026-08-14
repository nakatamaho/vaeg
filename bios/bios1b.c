#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "bios.h"
#include "biosmem.h"
#include "fddfile.h"
#include "fdd_mtr.h"
#include "sxsi.h"

enum {
	CACHE_TABLES = 4,
	CACHE_BUFFER = 32768
};

// ---- FDD

static BOOL setfdcmode(REG8 drv, REG8 type, REG8 rpm) {
	int i;
	if (drv >= 4) {
		return (FAILURE);
	}
	if ((rpm) && (!fdc.support144)) {
		return (FAILURE);
	}
	fdc.chgreg = type;
	fdc.rpm[drv] = rpm;
	if (type & 2) {
		for (i = 0; i < 4; i++)
			CTRL_FDMEDIA[i] = DISKTYPE_2HD;
	} else {
		for (i = 0; i < 4; i++)
			CTRL_FDMEDIA[i] = DISKTYPE_2DD;
	}
	return (SUCCESS);
}

void fddbios_equip(REG8 type, BOOL clear) {
	REG16 diskequip;

	diskequip = GETBIOSMEM16(MEMW_DISK_EQUIP);
	if (clear) {
		diskequip &= 0x0f00;
	}
	if (type & 1) {
		diskequip &= 0xfff0;
		diskequip |= (fdc.equip & 0x0f);
	} else {
		diskequip &= 0x0fff;
		diskequip |= (fdc.equip & 0x0f) << 12;
	}
	SETBIOSMEM16(MEMW_DISK_EQUIP, diskequip);
}

static BOOL biosfd_seek(REG8 track, BOOL ndensity) {
	if (ndensity) {
		if (track < 42) {
			track <<= 1;
		} else {
			track = 42 * 2;
		}
	}
	fdc.ncn = track;
	if (fdd_seek()) {
		return (FAILURE);
	}
	return (SUCCESS);
}

// -------------------------------------------------------------------- BIOS

static UINT16 boot_fd1(REG8 type, REG8 rpm) {
	UINT remain;
	UINT size;
	UINT32 pos;
	UINT16 bootseg;

	if (setfdcmode(fdc.us, type, rpm) != SUCCESS) {
		return (0);
	}
	if (biosfd_seek(0, 0)) {
		return (0);
	}
	fdc.hd = 0;
	fdc.mf = 0x40; // とりあえず MFMモードでリード
	if (fdd_readid()) {
		fdc.mf = 0x00; // FMモードでリトライ
		if (fdd_readid()) {
			return (0);
		}
	}
	remain = 0x400;
	pos = 0x1fc00;
	if ((!fdc.N) || (!fdc.mf) || (rpm)) {
		pos = 0x1fe00;
		remain = 0x200;
	}
	fdc.R = 1;
	bootseg = (UINT16)(pos >> 4);
	while (remain) {
		if (fdd_read()) {
			return (0);
		}
		if (fdc.N < 3) {
			size = 128 << fdc.N;
		} else {
			size = 128 << 3;
		}
		if (remain < size) {
			CopyMemory(mem + pos, fdc.buf, remain);
			break;
		} else {
			CopyMemory(mem + pos, fdc.buf, size);
			pos += size;
			remain -= size;
			fdc.R++;
		}
	}
	return (bootseg);
}

static UINT16 boot_fd(REG8 drv, REG8 type) {
	UINT16 bootseg;

	if (drv >= 4) {
		return (0);
	}
	fdc.us = drv;
	if (!fdd_diskready(fdc.us)) {
		return (0);
	}

	// 2HD
	if (type & 1) {
		// 1.25MB
		bootseg = boot_fd1(3, 0);
		if (bootseg) {
			mem[MEMB_DISK_BOOT] = (UINT8)(0x90 + drv);
			fddbios_equip(3, TRUE);
			return (bootseg);
		}
		// 1.44MB
		bootseg = boot_fd1(3, 1);
		if (bootseg) {
			mem[MEMB_DISK_BOOT] = (UINT8)(0x30 + drv);
			fddbios_equip(3, TRUE);
			return (bootseg);
		}
	}
	if (type & 2) {
		// 2DD
		bootseg = boot_fd1(0, 0);
		if (bootseg) {
			mem[MEMB_DISK_BOOT] = (BYTE)(0x70 + drv);
			fddbios_equip(0, TRUE);
			return (bootseg);
		}
	}
	return (0);
}

static REG16 boot_hd(REG8 drv) {
	REG8 ret;

	ret = sxsi_read(drv, 0, mem + 0x1fc00, 0x400);
	if (ret < 0x20) {
		mem[MEMB_DISK_BOOT] = drv;
		return (0x1fc0);
	}
	return (0);
}

REG16 bootstrapload(void) {
	BYTE i;
	REG16 bootseg;

	//	fdmode = 0;
	bootseg = 0;
	switch (mem[MEMB_MSW5] & 0xf0) { // うぐぅ…本当はALレジスタの値から
	case 0x00:                       // ノーマル
		break;

	case 0x20: // 640KB FDD
		for (i = 0; (i < 4) && (!bootseg); i++) {
			if (fdd_diskready(i)) {
				bootseg = boot_fd(i, 2);
			}
		}
		break;

	case 0x40: // 1.2MB FDD
		for (i = 0; (i < 4) && (!bootseg); i++) {
			if (fdd_diskready(i)) {
				bootseg = boot_fd(i, 1);
			}
		}
		break;

	case 0x60: // MO
		break;

	case 0xa0: // SASI 1
		bootseg = boot_hd(0x80);
		break;

	case 0xb0: // SASI 2
		bootseg = boot_hd(0x81);
		break;

	case 0xc0: // SCSI
		for (i = 0; (i < 4) && (!bootseg); i++) {
			bootseg = boot_hd((REG8)(0xa0 + i));
		}
		break;

	default: // ROM
		return (0);
	}
	for (i = 0; (i < 4) && (!bootseg); i++) {
		if (fdd_diskready(i)) {
			bootseg = boot_fd(i, 3);
		}
	}
	for (i = 0; (i < 2) && (!bootseg); i++) {
		bootseg = boot_hd((REG8)(0x80 + i));
	}
	for (i = 0; (i < 4) && (!bootseg); i++) {
		bootseg = boot_hd((REG8)(0xa0 + i));
	}
	return (bootseg);
}

// --------------------------------------------------------------------------

UINT bios0x1b_wait(void) {
	UINT addr;
	REG8 bit;

	if (fddmtr.busy) {
		CPU_IP--;
		CPU_REMCLOCK = -1;
	} else {
		if (fdc.chgreg & 1) {
			addr = MEMB_DISK_INTL;
			bit = 0x01;
		} else {
			addr = MEMB_DISK_INTH;
			bit = 0x10;
		}
		bit <<= fdc.us;
		if (mem[addr] & bit) {
			mem[addr] &= ~bit;
			return (0);
		} else {
			CPU_REMCLOCK -= 1000;
		}
	}
	CPU_IP--;
	return (1);
}
