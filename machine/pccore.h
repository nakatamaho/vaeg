
#include "machine/nevent.h"
#include "machine/statsave.h"
#include "machine/clockscale.h"

enum {
	PCBASECLOCK40 = 3993600,
	PCBASECLOCK25 = 2457600,
	PCBASECLOCK20 = 1996800,
	PCCORE_STANDARD_MULTIPLE = 2,
	PCCORE_CPU_MULTIPLE_MAX = 32
};

enum {
	CPUMODE_8MHZ = 0x20,
	CPUMODE_BASE4MHZ = 0x40,

	PCMODEL_VA = 1,

	PCHDD_SASI = 0x01,
	PCHDD_SCSI = 0x02,

	PCROM_BIOS = 0x01,
	PCROM_SOUND = 0x02,
	PCROM_SASI = 0x04,
	PCROM_SCSI = 0x08,

	PCSOUND_NONE = 0x00,

	PCCBUS_MPU98 = 0x0002,

	PCMODEL_VA1 = 1,
	PCMODEL_VA2 = 2,

	FMBOARD_NONE = 0x0000,
	FMBOARD_VA_OPN = 0x0100,
	FMBOARD_VA_OPNA = 0x0200,
};

typedef struct {
	// Retired non-VA display configuration; kept only as struct padding.
	UINT8 uPD72020;
	UINT8 DISPSYNC;
	UINT8 RASTER;
	UINT8 realpal;
	UINT8 LCD_MODE;
	UINT8 skipline;
	UINT16 skiplight;

	UINT8 KEY_MODE;
	UINT8 XSHIFT;
	UINT8 BTN_RAPID;
	UINT8 BTN_MODE;

	UINT8 dipsw[3];
	UINT8 MOUSERAPID;

	UINT8 calendar;
	UINT8 usefd144;
	UINT8 wait[6]; // retired non-VA timing padding

	// リセット時とかあんまり参照されない奴
	OEMCHAR model[8];
	UINT baseclock;
	UINT multiple;
	UINT8 sgp_speed_mode;
	UINT8 sgp_multiplier;

	UINT8 memsw[8];

	UINT8 ITF_WORK;
	UINT8 EXTMEM;
	UINT8 grcg;    // retired non-VA display state padding
	UINT8 color16; // retired non-VA display state padding
	UINT32 BG_COLOR;
	UINT32 FG_COLOR;

	UINT16 samplingrate;
	UINT16 delayms;
	UINT16 SOUND_SW;
	UINT8 snd_x;

	UINT8 snd14opt[3];
	UINT8 snd26opt; // retired non-VA board state padding
	UINT8 snd86opt; // retired non-VA board state padding
	UINT8 spbopt;
	UINT8 spb_vrc; // ver0.30
	UINT8 spb_vrl; // ver0.30
	UINT8 spb_x;   // ver0.30

	UINT8 BEEP_VOL;
	UINT8 vol14[6];
	UINT8 vol_fm;
	UINT8 vol_ssg;
	UINT8 vol_adpcm;
	UINT8 vol_pcm; // retired non-VA PCM state padding
	UINT8 vol_rhythm;

	UINT8 mpuenable;
	UINT8 mpuopt;

	UINT8 fddequip;
	UINT8 MOTOR;
	UINT8 MOTORVOL;
	UINT8 PROTECTMEM;
	UINT8 hdrvacc;

	UINT8 lockedkey;

	OEMCHAR sasihdd[2][MAX_PATH]; // ver0.74
	OEMCHAR scsihdd[7][MAX_PATH]; // ver0.74
	OEMCHAR fontfile[MAX_PATH];
	OEMCHAR biospath[MAX_PATH];
	OEMCHAR hdrvroot[MAX_PATH];
} NP2CFG;

typedef struct {
	UINT32 baseclock;
	UINT multiple;

	UINT8 cpumode;
	UINT8 model;
	UINT8 hddif;
	UINT8 extmem;
	UINT8 dipsw[3]; // リセット時のDIPSW
	UINT8 rom;

	UINT32 sound;
	UINT32 device;

	UINT32 realclock;

	UINT8 model_va;
} PCCORE;

enum {
	COREEVENT_SHUT = 0,
	COREEVENT_RESET = 1,
	COREEVENT_EXIT = 2
};

#ifdef __cplusplus
extern "C" {
#endif

extern const OEMCHAR np2version[];

extern NP2CFG np2cfg;
extern PCCORE pccore;
extern CLOCKSCALE pccore_cpu_scale;
extern UINT8 screenupdate;
extern int soundrenewal;
extern BOOL drawframe;
extern UINT drawcount;
extern BOOL hardwarereset;

void getbiospath(OEMCHAR *path, const OEMCHAR *fname, int maxlen);
void screendispva(NEVENTITEM item);
void screenvsyncva(NEVENTITEM item);
//void screenvsyncva2(NEVENTITEM item);
void sysp4vsyncint(NEVENTITEM item);
void sysp4vsyncstart(NEVENTITEM item);
void sysp4vsyncend(NEVENTITEM item);

void pccore_cfgupdate(void);
BOOL pccore_cpu_multiple_valid(UINT multiple);
void pccore_clockrestore(void);
UINT pccore_cpu_multiple(void);
UINT32 pccore_cpu_clock(void);

void pccore_init(void);
void pccore_term(void);
void pccore_reset(void);
void pccore_exec(BOOL draw);
void pccore_redraw(void);

void pccore_postevent(UINT32 event);

#if defined(USEIPTRACE) && defined(TRACE) // Shinra
void iptrace_out(void);
extern int treafter;
#endif

//@@@@@
void pccore_debugint(UINT32 no);
void pccore_debugmem(UINT32 op, UINT32 addr, UINT16 data);
//@@@@@

#ifdef __cplusplus
}
#endif
