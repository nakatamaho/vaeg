/*
 *	BKMEMVA.H: PC-88VA Backup memory
 */

#ifdef __cplusplus
extern "C" {
#endif

void bkupmemva_load(void);
void bkupmemva_save(void);
void bkupmemva_setpath(const char *path);

#ifdef __cplusplus
}
#endif
