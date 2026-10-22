/*
 * upd9002_regs.h: PC-88VA CPU port
 */

typedef struct {
	BYTE tcks; // タイマ、カウンタの入力クロック指定
	           // bit4-2 タイマ2-0への供給クロック 0..内部 1..外部
	           // bit1-0 分周比　00..2 01..4 10..8 11..16
	BYTE dmy[15];
} UPD9002_REGS;

/* FFF4h-FFF6h, the wait control unit (V50 layout, M130): WMB b6-b4 LMB,
 * b2-b0 UMB (partition sizes); WCY1 b5-b4 UMW, b3-b2 MMW, b1-b0 LMW (memory
 * waits of the upper, middle and lower partitions), b7-b6 IOW; WCY2 b3-b2
 * DMAW, b1-b0 RFW. Its own state section. */
typedef struct {
	BYTE wmb;
	BYTE wcy1;
	BYTE wcy2;
	BYTE reserved;
} UPD9002_WCU;

/* The programmed memory waits by address, derived from UPD9002_WCU. */
typedef struct {
	UINT32 lower_end;   /* the lower partition is below this address */
	UINT32 upper_start; /* the upper partition starts here (below 1 MiB) */
	BYTE lower;
	BYTE middle;
	BYTE upper;
	BYTE active; /* any partition has a wait */
} UPD9002_WAITMAP;

/* Separate state section: do not repurpose the legacy CPU-port padding. */
typedef struct {
	BYTE ranges[8]; /* FFE0h..FFE7h, low/high bytes of two inclusive ranges. */
	BYTE control;   /* FFEFh: IN/OUT enables and port-matching mode. */
} UPD9002_IOTRAP;

#ifdef __cplusplus
extern "C" {
#endif

extern UPD9002_REGS upd9002_regs;
extern UPD9002_IOTRAP upd9002_iotrap;
extern UPD9002_WCU upd9002_wcu;
extern UPD9002_WAITMAP upd9002_waitmap;

void upd9002_regs_reset(void);
void upd9002_regs_bind(void);
void upd9002_wcu_update(void);
void upd9002_wcu_legacy(void);

/* Programmed waits of one CPU bus cycle at ADDRESS. The system area
 * A0000h-DFFFFh is left out: its devices (GVRAM, TVRAM, kanji ROM) hold
 * the CPU with external READY for longer than the programmed count, which
 * the WCU inserts first (measured: GVRAM accesses cost the same on the
 * PC-88VA, which programs 1 wait there, as on the PC-88VA2, which programs
 * none); their waits are modelled by the devices. */
static INLINE UINT upd9002_programmed_wait(UINT32 address) {
	if ((address >= 0x0a0000) && (address < 0x0e0000)) {
		return 0;
	}
	if (address >= 0x100000) {
		return 0;
	}
	if (address < upd9002_waitmap.lower_end) {
		return upd9002_waitmap.lower;
	}
	if (address >= upd9002_waitmap.upper_start) {
		return upd9002_waitmap.upper;
	}
	return upd9002_waitmap.middle;
}

#ifdef __cplusplus
}
#endif
