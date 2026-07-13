#include	"compiler.h"
#include	"dosio.h"
#include	"newdisk.h"
#include	"fddfile.h"
#include	"sxsi.h"
#include	"hddboot.res"


// ---- fdd

void newdisk_fdd(const char *fname, REG8 type, const char *label) {

	_D88HEAD	d88head;
	FILEH		fh;

	ZeroMemory(&d88head, sizeof(d88head));
	STOREINTELDWORD(d88head.fd_size, sizeof(d88head));
	milstr_ncpy((char *)d88head.fd_name, label, sizeof(d88head.fd_name));
	d88head.fd_type = type;
	fh = file_create(fname);
	if (fh != FILEH_INVALID) {
		file_write(fh, &d88head, sizeof(d88head));
		file_close(fh);
	}
}

typedef struct {
	UINT8	cylinders;
	UINT8	heads;
	UINT8	sectors;
	UINT8	sector_n;
	UINT16	sector_size;
	UINT8	d88_type;
	UINT8	sectors_per_cluster;
	UINT8	media;
	UINT16	root_entries;
	UINT16	fat_sectors;
} NEWDISKFDDMSDOS;

static const NEWDISKFDDMSDOS newdisk_fdd_msdos_geometry[] = {
	{77, 2, 8, 3, 1024, 0x20, 1, 0xfe, 192, 2},
	{80, 2, 8, 2,  512, 0x10, 2, 0xfb, 112, 2}
};

static void newdisk_fdd_msdos_boot(BYTE *sector,
							const NEWDISKFDDMSDOS *geometry) {

	UINT16	total_sectors;

	total_sectors = geometry->cylinders * geometry->heads *
											geometry->sectors;
	sector[0] = 0xeb;
	sector[1] = 0x3c;
	sector[2] = 0x90;
	CopyMemory(sector + 3, "VAEG1.0 ", 8);
	STOREINTELWORD(sector + 11, geometry->sector_size);
	sector[13] = geometry->sectors_per_cluster;
	STOREINTELWORD(sector + 14, 1);
	sector[16] = 2;
	STOREINTELWORD(sector + 17, geometry->root_entries);
	STOREINTELWORD(sector + 19, total_sectors);
	sector[21] = geometry->media;
	STOREINTELWORD(sector + 22, geometry->fat_sectors);
	STOREINTELWORD(sector + 24, geometry->sectors);
	STOREINTELWORD(sector + 26, geometry->heads);
	STOREINTELDWORD(sector + 28, 0);
	STOREINTELDWORD(sector + 32, 0);
	sector[36] = 0;
	sector[37] = 0;
	sector[38] = 0x29;
	STOREINTELDWORD(sector + 39,
					0x56414547U ^ ((UINT32)total_sectors << 8) ^
					geometry->sector_size);
	CopyMemory(sector + 43, "NO NAME    ", 11);
	CopyMemory(sector + 54, "FAT12   ", 8);
	sector[510] = 0x55;
	sector[511] = 0xaa;
}

static BOOL newdisk_fdd_msdos_sector(FILEH fh,
							const NEWDISKFDDMSDOS *geometry,
							UINT32 logical_sector, BYTE *work) {

	UINT32	second_fat;

	ZeroMemory(work, geometry->sector_size);
	if (logical_sector == 0) {
		newdisk_fdd_msdos_boot(work, geometry);
	}
	second_fat = 1 + geometry->fat_sectors;
	if ((logical_sector == 1) || (logical_sector == second_fat)) {
		work[0] = geometry->media;
		work[1] = 0xff;
		work[2] = 0xff;
	}
	return(file_write(fh, work, geometry->sector_size) ==
										geometry->sector_size);
}

BOOL newdisk_fdd_msdos(const char *fname, UINT format) {

	const NEWDISKFDDMSDOS	*geometry;
	_D88HEAD				d88head;
	_D88SEC				d88sec;
	FILEH					fh;
	BYTE					work[1024];
	UINT32					logical_sector;
	UINT32					file_offset;
	UINT32					track_size;
	UINT32					total_sectors;
	UINT					track;
	UINT					sector;
	BOOL					result;

	if ((fname == NULL) || (fname[0] == '\0') ||
		(format >= NEWDISK_FDD_MSDOS_COUNT) ||
		(file_attr(fname) != (short)-1)) {
		return(FAILURE);
	}
	geometry = newdisk_fdd_msdos_geometry + format;
	total_sectors = geometry->cylinders * geometry->heads *
											geometry->sectors;
	track_size = geometry->sectors *
							(sizeof(_D88SEC) + geometry->sector_size);
	file_offset = sizeof(d88head);
	ZeroMemory(&d88head, sizeof(d88head));
	CopyMemory(d88head.fd_name, "MS-DOS", 6);
	d88head.fd_type = geometry->d88_type;
	for (track=0; track<(UINT)(geometry->cylinders * geometry->heads);
		 track++) {
		STOREINTELDWORD(d88head.trackp[track], file_offset);
		file_offset += track_size;
	}
	STOREINTELDWORD(d88head.fd_size, file_offset);

	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		return(FAILURE);
	}
	result = (file_write(fh, &d88head, sizeof(d88head)) ==
											sizeof(d88head));
	logical_sector = 0;
	for (track=0; result &&
		 track<(UINT)(geometry->cylinders * geometry->heads); track++) {
		for (sector=0; result && sector<geometry->sectors; sector++) {
			ZeroMemory(&d88sec, sizeof(d88sec));
			d88sec.c = (BYTE)(track / geometry->heads);
			d88sec.h = (BYTE)(track % geometry->heads);
			d88sec.r = (BYTE)(sector + 1);
			d88sec.n = geometry->sector_n;
			STOREINTELWORD(d88sec.sectors, geometry->sectors);
			STOREINTELWORD(d88sec.size, geometry->sector_size);
			result = (file_write(fh, &d88sec, sizeof(d88sec)) ==
												sizeof(d88sec));
			if (result) {
				result = newdisk_fdd_msdos_sector(fh, geometry,
										logical_sector, work);
			}
			logical_sector++;
		}
	}
	file_close(fh);
	if ((!result) || (logical_sector != total_sectors)) {
		file_delete(fname);
		return(FAILURE);
	}
	return(SUCCESS);
}


// ---- hdd

static BOOL writezero(FILEH fh, UINT size) {

	BYTE	work[256];
	UINT	wsize;

	ZeroMemory(work, sizeof(work));
	while(size) {
		wsize = min(size, sizeof(work));
		if (file_write(fh, work, wsize) != wsize) {
			return(FAILURE);
		}
		size -= wsize;
	}
	return(SUCCESS);
}

static BOOL writehddipl(FILEH fh, UINT ssize, UINT32 tsize) {

	BYTE	work[1024];
	UINT	size;

	ZeroMemory(work, sizeof(work));
	CopyMemory(work, hdddiskboot, sizeof(hdddiskboot));
	if (ssize < 1024) {
		work[ssize - 2] = 0x55;
		work[ssize - 1] = 0xaa;
	}
	if (file_write(fh, work, sizeof(work)) != sizeof(work)) {
		return(FAILURE);
	}
	if (tsize > sizeof(work)) {
		tsize -= sizeof(work);
		ZeroMemory(work, sizeof(work));
		while(tsize) {
			size = min(tsize, sizeof(work));
			tsize -= size;
			if (file_write(fh, work, size) != size) {
				return(FAILURE);
			}
		}
	}
	return(SUCCESS);
}

void newdisk_thd(const char *fname, UINT hddsize) {

	FILEH	fh;
	BYTE	work[256];
	UINT	size;
	BOOL	r;

	if ((fname == NULL) || (hddsize < 5) || (hddsize > 256)) {
		goto ndthd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndthd_err;
	}
	ZeroMemory(work, 256);
	size = hddsize * 15;
	STOREINTELWORD(work, size);
	r = (file_write(fh, work, 256) != 256);
	r |= writehddipl(fh, 256, 0);
	file_close(fh);
	if (r) {
		file_delete(fname);
	}

ndthd_err:
	return;
}

void newdisk_nhd(const char *fname, UINT hddsize) {

	FILEH	fh;
	NHDHDR	nhd;
	UINT	size;
	BOOL	r;

	if ((fname == NULL) || (hddsize < 5) || (hddsize > 512)) {
		goto ndnhd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndnhd_err;
	}
	ZeroMemory(&nhd, sizeof(nhd));
	CopyMemory(&nhd.sig, sig_nhd, 15);
	STOREINTELDWORD(nhd.headersize, sizeof(nhd));
	size = hddsize * 15;
	STOREINTELDWORD(nhd.cylinders, size);
	STOREINTELWORD(nhd.surfaces, 8);
	STOREINTELWORD(nhd.sectors, 17);
	STOREINTELWORD(nhd.sectorsize, 512);
	r = (file_write(fh, &nhd, sizeof(nhd)) != sizeof(nhd));
	r |= writehddipl(fh, 512, size * 8 * 17 * 512);
	file_close(fh);
	if (r) {
		file_delete(fname);
	}

ndnhd_err:
	return;
}

#if defined(VAEG_FIX)
// hddtype = 0:5MB / 1:10MB / 2:15MB / 4:20MB / 5:30MB / 6:40MB
#else
// hddtype = 0:5MB / 1:10MB / 2:15MB / 3:20MB / 5:30MB / 6:40MB
#endif
void newdisk_hdi(const char *fname, UINT hddtype) {

const SASIHDD	*sasi;
	FILEH		fh;
	HDIHDR		hdi;
	UINT32		size;
	BOOL		r;

	hddtype &= 7;
	if ((fname == NULL) || (hddtype == 7)) {
		goto ndhdi_err;
	}
	sasi = sasihdd + hddtype;
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndhdi_err;
	}
	ZeroMemory(&hdi, sizeof(hdi));
	size = 256 * sasi->sectors * sasi->surfaces * sasi->cylinders;
//	STOREINTELDWORD(hdi.hddtype, 0);
	STOREINTELDWORD(hdi.headersize, 4096);
	STOREINTELDWORD(hdi.hddsize, size);
	STOREINTELDWORD(hdi.sectorsize, 256);
	STOREINTELDWORD(hdi.sectors, sasi->sectors);
	STOREINTELDWORD(hdi.surfaces, sasi->surfaces);
	STOREINTELDWORD(hdi.cylinders, sasi->cylinders);
	r = (file_write(fh, &hdi, sizeof(hdi)) != sizeof(hdi));
	r |= writezero(fh, 4096 - sizeof(hdi));
	r |= writehddipl(fh, 256, size);
	file_close(fh);
	if (r) {
		file_delete(fname);
	}

ndhdi_err:
	return;
}

void newdisk_vhd(const char *fname, UINT hddsize) {

	FILEH	fh;
	VHDHDR	vhd;
	UINT	tmp;
	BOOL	r;

	if ((fname == NULL) || (hddsize < 2) || (hddsize > 512)) {
		goto ndvhd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndvhd_err;
	}
	ZeroMemory(&vhd, sizeof(vhd));
	CopyMemory(&vhd.sig, sig_vhd, 7);
	STOREINTELWORD(vhd.mbsize, (UINT16)hddsize);
	STOREINTELWORD(vhd.sectorsize, 256);
	vhd.sectors = 32;
	vhd.surfaces = 8;
	tmp = hddsize *	16;		// = * 1024 * 1024 / (8 * 32 * 256);
	STOREINTELWORD(vhd.cylinders, (UINT16)tmp);
	tmp *= 8 * 32;
	STOREINTELDWORD(vhd.totals, tmp);
	r = (file_write(fh, &vhd, sizeof(vhd)) != sizeof(vhd));
	r |= writehddipl(fh, 256, 0);
	file_close(fh);
	if (r) {
		file_delete(fname);
	}

ndvhd_err:
	return;
}
