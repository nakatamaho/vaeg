
#ifdef __cplusplus
extern "C" {
#endif

void newdisk_fdd(const char *fname, REG8 type, const char *label);

enum {
	NEWDISK_FDD_MSDOS_2HD = 0,
	NEWDISK_FDD_MSDOS_2DD,
	NEWDISK_FDD_MSDOS_COUNT
};

enum {
	NEWDISK_FDD_CONTAINER_D88 = 0,
	NEWDISK_FDD_CONTAINER_RAW,
	NEWDISK_FDD_CONTAINER_COUNT
};

BOOL newdisk_fdd_msdos(const char *fname, UINT format);
BOOL newdisk_fdd_msdos_ex(const char *fname, UINT format, UINT container);

void newdisk_thd(const char *fname, UINT hddsize);
void newdisk_nhd(const char *fname, UINT hddsize);
void newdisk_hdi(const char *fname, UINT hddtype);
void newdisk_vhd(const char *fname, UINT hddsize);

#ifdef __cplusplus
}
#endif
