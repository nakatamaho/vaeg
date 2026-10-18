
#ifdef __cplusplus
extern "C" {
#endif

BOOL fddd88_set(FDDFILE fdd, const char *fname, int ro);
/* M120: a D88 file may hold several disks back to back. */
enum {
	FDDD88_NAME_SIZE = 18,
	FDDD88_MAX_IMAGES = 255
};
UINT fddd88_images(const char *fname, char (*names)[FDDD88_NAME_SIZE], UINT max);
BOOL fddd88_eject(FDDFILE fdd);

BOOL fdd_diskaccess_d88(void);
BOOL fdd_seek_d88(void);
BOOL fdd_seeksector_d88(void);
BOOL fdd_read_d88(void);
BOOL fdd_write_d88(void);
BOOL fdd_diagread_d88(void);
BOOL fdd_readid_d88(void);
BOOL fdd_writeid_d88(void);

BOOL fdd_formatinit_d88(void);
BOOL fdd_formating_d88(const BYTE *ID);
BOOL fdd_isformating_d88(void);

#ifdef __cplusplus
}
#endif
