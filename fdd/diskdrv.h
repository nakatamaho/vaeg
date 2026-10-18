
#ifdef __cplusplus
extern "C" {
#endif

extern int diskdrv_delay[4];
extern char diskdrv_fname[4][MAX_PATH];
/* M120: the disk of a multi-image D88 that diskdrv_fname will insert */
UINT diskdrv_pendingimage(REG8 drv);

void diskdrv_sethdd(REG8 drv, const char *fname);

void diskdrv_setfdd(REG8 drv, const char *fname, int readonly);
void diskdrv_setfddex(REG8 drv, const char *fname, int readonly, UINT image);
void diskdrv_callback(void);

#ifdef __cplusplus
}
#endif
