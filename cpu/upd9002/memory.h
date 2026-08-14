#ifndef VAEG_CPU_UPD9002_MEMORY_H
#define VAEG_CPU_UPD9002_MEMORY_H

#ifndef MEMCALL
#define MEMCALL
#endif

/*
 * Native VA CPU-visible memory is decoded by memoryva. The backing mem array
 * stores conventional main RAM and the 64 KiB HMA only; expansion memory is
 * owned by the uPD9002 core context.
 */
enum {
    UPD9002_MAINRAM_LIMIT = 0x0a0000,
    USE_HIMEM = 0x110000,

    /*
     * These offsets are private backing storage for the retained simulated
     * boot BIOS, host font conversion, and legacy MEMORY save-state payload.
     * They are not entries in the native VA CPU memory decoder.
     */
    VRAM_STEP = 0x100000,
    VRAM_B = 0x0a8000,
    VRAM_R = 0x0b0000,
    VRAM_G = 0x0b8000,
    VRAM_E = 0x0e0000,
    VRAM0_B = VRAM_B,
    VRAM0_R = VRAM_R,
    VRAM0_G = VRAM_G,
    VRAM0_E = VRAM_E,
    VRAM1_B = VRAM_STEP + VRAM_B,
    VRAM1_R = VRAM_STEP + VRAM_R,
    VRAM1_G = VRAM_STEP + VRAM_G,
    VRAM1_E = VRAM_STEP + VRAM_E,
    FONT_ADRS = 0x110000,
    ITF_ADRS = VRAM_STEP + 0x0f8000
};

#define VRAMADDRMASKEX(a) ((a) & (VRAM_STEP | 0x7fff))

#ifdef __cplusplus
extern "C" {
#endif

extern BYTE mem[0x200000];

/* Raw storage helpers used only by native VA map entries for main RAM. */
REG8 MEMCALL upd9002_mainram_read(UINT32 address);
REG16 MEMCALL upd9002_mainram_read_w(UINT32 address);
void MEMCALL upd9002_mainram_write(UINT32 address, REG8 value);
void MEMCALL upd9002_mainram_write_w(UINT32 address, REG16 value);

#if defined(VAEG_UPD9002_SSTS_TESTING)
/* Flat-memory scope for CPU-only tests; absent from production builds. */
void upd9002_test_flat_memory_set(BOOL active);
#endif

REG8 MEMCALL upd9002_memoryread(UINT32 address);
REG16 MEMCALL upd9002_memoryread_w(UINT32 address);
void MEMCALL upd9002_memorywrite(UINT32 address, REG8 value);
void MEMCALL upd9002_memorywrite_w(UINT32 address, REG16 value);
REG16 MEMCALL upd9002_memoryread_seg_w(UINT32 segment_base, UINT off);
void MEMCALL upd9002_memorywrite_seg_w(UINT32 segment_base, UINT off,
                                      REG16 value);

REG8 MEMCALL meml_read8(UINT seg, UINT off);
REG16 MEMCALL meml_read16(UINT seg, UINT off);
void MEMCALL meml_write8(UINT seg, UINT off, REG8 value);
void MEMCALL meml_write16(UINT seg, UINT off, REG16 value);

void MEMCALL meml_readstr(UINT seg, UINT off, void *dat, UINT leng);
void MEMCALL meml_writestr(UINT seg, UINT off, const void *dat, UINT leng);

void MEMCALL meml_read(UINT32 address, void *dat, UINT leng);
void MEMCALL meml_write(UINT32 address, const void *dat, UINT leng);

/* Physical address space used by DMA clients. */
#define MEMP_READ8(addr) upd9002_memoryread((addr))
#define MEMP_WRITE8(addr, dat) upd9002_memorywrite((addr), (dat))

/* Segmented and linear address helpers used by BIOS services. */
#define MEML_READ8(seg, off) meml_read8((seg), (off))
#define MEML_READ16(seg, off) meml_read16((seg), (off))
#define MEML_WRITE8(seg, off, dat) meml_write8((seg), (off), (dat));
#define MEML_WRITE16(seg, off, dat) meml_write16((seg), (off), (dat));
#define MEML_READSTR(seg, off, dat, leng) \
    meml_readstr((seg), (off), (dat), (leng))
#define MEML_WRITESTR(seg, off, dat, leng) \
    meml_writestr((seg), (off), (dat), (leng))
#define MEML_READ(addr, dat, leng) meml_read((addr), (dat), (leng))
#define MEML_WRITE(addr, dat, leng) meml_write((addr), (dat), (leng))

#ifdef __cplusplus
}
#endif

#endif
