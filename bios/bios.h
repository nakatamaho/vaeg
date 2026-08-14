
enum {
	BIOS_SEG = 0xfd80,
	BIOS_BASE = (BIOS_SEG << 4),

	BIOS_TABLE = 0x0040,

	BIOSOFST_ITF = 0x0080,
	BIOSOFST_INIT = 0x0084,

	BIOSOFST_09 = 0x0088, // Keyboard
	BIOSOFST_0c = 0x008c, // Serial

	BIOSOFST_12 = 0x0090, // FDC
	BIOSOFST_13 = 0x0094, // FDC

	BIOSOFST_WAIT = 0x00b4 // FDD waiting
};

#ifdef __cplusplus
extern "C" {
#endif

// extern	BOOL	biosrom;

void bios_initialize(void);
UINT MEMCALL biosfunc(UINT32 adrs);

void bios0x09(void);
void bios0x09_init(void);

void bios0x0c(void);

void bios0x12(void);
void bios0x13(void);

UINT bios0x1b_wait(void);
void fddbios_equip(REG8 type, BOOL clear);

REG16 bootstrapload(void);

#ifdef __cplusplus
}
#endif
