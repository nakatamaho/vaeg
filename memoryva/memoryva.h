#ifndef VAEG_MEMORYVA_MEMORYVA_H
#define VAEG_MEMORYVA_MEMORYVA_H

#ifndef MEMCALL
#define MEMCALL
#endif

/*
 * Runtime state for the native VA banked-memory decoder. Bank registers are
 * restored from save states before upd9002_memorymap_va() rebuilds optional
 * VA91 entries.
 */
typedef struct {
	UINT8 sysm_bank;
	UINT8 rom0_bank;
	UINT8 rom1_bank;
	UINT8 dma_sysm_bank;
	UINT8 dma_access;
	UINT8 backupmem_wp;
	UINT8 dmy0;
	UINT8 dmy1;

	UINT32 rom0exist;
	UINT32 rom1exist;
	UINT32 sysmromexist;
} _MEMORYVA;

#ifdef __cplusplus
extern "C" {
#endif

extern BYTE textmem[0x40000];
extern BYTE fontmem[0x50000];
extern BYTE backupmem[0x04000];
extern BYTE dicmem[0x80000];
extern BYTE rom0mem[0xa0000];
extern BYTE rom1mem[0x20000];

extern _MEMORYVA memoryva;
extern BOOL textmem_dirty;

void MEMCALL upd9002_memorymap_va(void);
void MEMCALL upd9002_memorywrite_va(UINT32 address, REG8 value);
void MEMCALL upd9002_memorywrite_va_w(UINT32 address, REG16 value);
REG8 MEMCALL upd9002_memoryread_va(UINT32 address);
REG16 MEMCALL upd9002_memoryread_va_w(UINT32 address);

#ifdef __cplusplus
}
#endif

#endif
