/*
 * memctrlva.h: PC-88VA memory control
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

void memctrlva_reset(void);
void memctrlva_bind(void);
BOOL memctrlva_nmode_active(void);
BOOL memctrlva_nmode_boot_search(void);
void memctrlva_nmode_reset(void);
void memctrlva_nmode_compat_entry(void);
/* M144: register a port of the manual's V1/V2-only block (0000h-00FFh);
 * outside V1/V2 mode the main unit does not decode it. */
BOOL memctrlva_attachout_v1v2(UINT port, void (*func)(UINT port, REG8 dat));
BOOL memctrlva_attachinp_v1v2(UINT port, REG8 (*func)(UINT port));

#ifdef __cplusplus
}
#endif
