#include	"compiler.h"
#include	"np2.h"
#include	"winkbd.h"
#include	"keystat.h"


#define		NC		0xff

static UINT8 key106[256] = {
			//	    ,    ,    ,STOP,    ,    ,    ,    		; 0x00
				  NC,  NC,  NC,0x60,  NC,  NC,  NC,  NC,
			//	  BS, TAB,    ,    , CLR, ENT,    ,    		; 0x08
#if defined(SUPPORT_PC88VA)
				0x0e,0x0f,  NC,  NC,  NC,0x59,  NC,  NC,
#else
				0x0e,0x0f,  NC,  NC,  NC,0x1c,  NC,  NC,
#endif
			//	 SFT,CTRL, ALT,PAUS,CAPS,KANA,    ,    		; 0x10
#if defined(SUPPORT_PC88VA)
				0x70,0x58,  NC,0x60,0x71,0x72,  NC,  NC,
#else
				0x70,0x74,0x73,0x60,0x71,0x72,  NC,  NC,
#endif
			//	 FIN, KAN,    , ESC,XFER,NFER,    ,  MD		; 0x18
				  NC,  NC,  NC,0x00,0x35,0x51,  NC,  NC,
			//	 SPC,RLUP,RLDN, END,HOME,  ←,  ↑,  →		; 0x20
				0x34,0x37,0x36,0x3f,0x3e,0x3b,0x3a,0x3c,
			//	  ↓, SEL, PNT, EXE,COPY, INS, DEL, HLP		; 0x28
				0x3d,  NC,  NC,  NC,  NC,0x38,0x39,  NC,
			//	  ０,  １,  ２,  ３,  ４,  ５,  ６,  ７		; 0x30
				0x0a,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
			//	  ８,  ９,    ,    ,    ,    ,    ,    		; 0x38
				0x08,0x09,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,  Ａ,  Ｂ,  Ｃ,  Ｄ,  Ｅ,  Ｆ,  Ｇ		; 0x40
				  NC,0x1d,0x2d,0x2b,0x1f,0x12,0x20,0x21,
			//	  Ｈ,  Ｉ,  Ｊ,  Ｋ,  Ｌ,  Ｍ,  Ｎ,  Ｏ		; 0x48
				0x22,0x17,0x23,0x24,0x25,0x2f,0x2e,0x18,
			//	  Ｐ,  Ｑ,  Ｒ,  Ｓ,  Ｔ,  Ｕ,  Ｖ,  Ｗ		; 0x50
				0x19,0x10,0x13,0x1e,0x14,0x16,0x2c,0x11,
			//	  Ｘ,  Ｙ,  Ｚ,LWIN,RWIN, APP,    ,    		; 0x58
#if defined(SUPPORT_PC88VA)
				0x2a,0x15,0x29,  NC,  NC,0x5a,  NC,  NC,
#else
				0x2a,0x15,0x29,  NC,  NC,  NC,  NC,  NC,
#endif
			//	<０>,<１>,<２>,<３>,<４>,<５>,<６>,<７>		; 0x60
				0x4e,0x4a,0x4b,0x4c,0x46,0x47,0x48,0x42,
			//	<８>,<９>,<＊>,<＋>,<，>,<－>,<．>,<／>		; 0x68
				0x43,0x44,0x45,0x49,0x4f,0x40,0x50,0x41,
			//	 f.1, f.2, f.3, f.4, f.5, f.6, f.7, f.8		; 0x70
				0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
			//	 f.9, f10, f11, f12, f13, f14, f15, f16		; 0x78
				0x6a,0x6b,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0x80
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0x88
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	HELP, ALT,<＝>,    ,    ,    ,    ,    		; 0x90
				  NC,0x73,0x4d,  NC,  NC,  NC,  NC,  NC,			// ver0.28
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0x98
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xa0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xa8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xb0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,  ：,  ；,  ，,  －,  ．,  ／		; 0xb8
				  NC,  NC,0x27,0x26,0x30,0x0b,0x31,0x32,
			//	  ＠,    ,    ,    ,    ,    ,    ,    		; 0xc0
				0x1a,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xc8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xd0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,  ［,  ￥,  ］,  ＾,  ＿		; 0xd8
				  NC,  NC,  NC,0x1b,0x0d,0x28,0x0c,0x33,
			//	    ,    ,  ＿,    ,    ,    ,    ,    		; 0xe0
				  NC,  NC,0x33,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xe8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	CAPS,    ,KANA,KNJ ,    ,    ,    ,    		; 0xf0
#if defined(VAEG_EXT)
				0x71,  NC,0x72,0x5b,  NC,  NC,  NC,  NC,
#else
				0x71,  NC,0x72,  NC,  NC,  NC,  NC,  NC,
#endif
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xf8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC};

/*
lParam bit24によってキーコードを変化させる場合、以下のテーブルでbit24=0の
場合のキーコードを指定する。
*/
#if defined(SUPPORT_PC88VA)
/*
  	キーコードbit7
		0.SHIFT同時押し
		1.SHIFT同時押しせず
*/
#endif

static const UINT8 key106ext[256] = {
			//	    ,    ,    ,STOP,    ,    ,    ,    		; 0x00
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	  BS, TAB,    ,    , CLR, ENT,    ,    		; 0x08
#if defined(SUPPORT_PC88VA)
				  NC,  NC,  NC,  NC,  NC,0x9c,  NC,  NC,
#else
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
#endif
			//	 SFT,CTRL, ALT,PAUS,CAPS,KANA,    ,    		; 0x10
#if defined(SUPPORT_PC88VA)
				  NC,0xf4,0xf3,  NC,  NC,  NC,  NC,  NC,
#else
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
#endif
			//	 FIN, KAN,    , ESC,XFER,NFER,    ,  MD		; 0x18
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	 SPC,RLUP,RLDN, END,HOME,  ←,  ↑,  →		; 0x20
				  NC,0x44,0x4c,0x4a,0x42,0x46,0x43,0x48,
			//	  ↓, SEL, PNT, EXE,COPY, INS, DEL, HLP		; 0x28
				0x4b,  NC,  NC,  NC,  NC,0x4e,0x50,  NC,
			//	  ０,  １,  ２,  ３,  ４,  ５,  ６,  ７		; 0x30
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	  ８,  ９,    ,    ,    ,    ,    ,    		; 0x38
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,  Ａ,  Ｂ,  Ｃ,  Ｄ,  Ｅ,  Ｆ,  Ｇ		; 0x40
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	  Ｈ,  Ｉ,  Ｊ,  Ｋ,  Ｌ,  Ｍ,  Ｎ,  Ｏ		; 0x48
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	  Ｐ,  Ｑ,  Ｒ,  Ｓ,  Ｔ,  Ｕ,  Ｖ,  Ｗ		; 0x50
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	  Ｘ,  Ｙ,  Ｚ,LWIN,RWIN, APP,    ,    		; 0x58
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	<０>,<１>,<２>,<３>,<４>,<５>,<６>,<７>		; 0x60
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	<８>,<９>,<＊>,<＋>,<，>,<－>,<．>,<／>		; 0x68
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	 f.1, f.2, f.3, f.4, f.5, f.6, f.7, f.8		; 0x70
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	 f.9, f10, f11, f12, f13, f14, f15, f16		; 0x78
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0x80
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0x88
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	HELP, ALT,<＝>,    ,    ,    ,    ,    		; 0x90
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0x98
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xa0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xa8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xb0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,  ：,  ；,  ，,  －,  ．,  ／		; 0xb8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	  ＠,    ,    ,    ,    ,    ,    ,    		; 0xc0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xc8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xd0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,  ［,  ￥,  ］,  ＾,    		; 0xd8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,  ＿,    ,    ,    ,    ,    		; 0xe0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xe8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	CAPS,    ,KANA,    ,    ,    ,    ,    		; 0xf0
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
			//	    ,    ,    ,    ,    ,    ,    ,    		; 0xf8
				  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC};

#if defined(SUPPORT_PC88VA)
static const UINT8 f12keys[] = {
			0x61, 0x60, 0x4d, 0x4f, 0x76, 0x77, 0x5b};
static const UINT8 altrkeys[] = {
			0x61, 0x60, 0x4d, 0x4f, 0x76, 0x77, 0x5b, 0x73};
#else
static const UINT8 f12keys[] = {
			0x61, 0x60, 0x4d, 0x4f, 0x76, 0x77};
#endif


#if defined(VAEG_EXT)
static UINT8 knjdown = 0;	// 漢字キーが押された場合に非0
							// (エミュレーションマシンの)VSYNCごとにカウントダウンし
							// 0になったらキーを離した状態にする
#endif


void winkbd_keydown(WPARAM wParam, LPARAM lParam) {

	BYTE	data;

#if defined(VAEG_EXT)
	if (wParam == 0xf4) wParam = 0xf3;	
		// 漢字キーは押すたびに交互にwParamの値がf3/f4で入れ替わる
		// ここでは0xf3に統一する
	if (wParam == 0xf3) knjdown = 10;
#endif

	data = key106[wParam & 0xff];
	if (data != NC) {
		if ((data == 0x73) &&
				(np2oscfg.KEYBOARD == KEY_KEY101) &&
				(lParam & 0x01000000)) {
			data = 0x72;
		}
		else if ((np2oscfg.KEYBOARD != KEY_PC98) &&
				(!(lParam & 0x01000000)) &&
				(key106ext[wParam & 0xff] != NC)) {			// ver0.28
#if defined(SUPPORT_PC88VA)
			data = key106ext[wParam & 0xff];
			if (data & 0x80) {
				data &= ~0x80;
			}
			else {
				keystat_senddata(0x70);							// PC/AT only!
			}
#else
			keystat_senddata(0x70);							// PC/AT only!
			data = key106ext[wParam & 0xff];
#endif
		}
		keystat_senddata(data);
	}
	else {													// ver0.28
		if ((!np2oscfg.KEYBOARD != KEY_PC98) && (wParam == 0x0c)) {
			keystat_senddata(0x70);							// PC/AT only
			keystat_senddata(0x47);
		}
	}
}

void winkbd_keyup(WPARAM wParam, LPARAM lParam) {

	BYTE	data;

	data = key106[wParam & 0xff];
	if (data != NC) {
		if ((data == 0x73) &&
				(np2oscfg.KEYBOARD == KEY_KEY101) &&
				(lParam & 0x01000000)) {
			; // none !
		}
		else if ((np2oscfg.KEYBOARD != KEY_PC98) &&
				(!(lParam & 0x01000000)) &&
				(key106ext[wParam & 0xff] != NC)) {		// ver0.28
#if defined(SUPPORT_PC88VA)
			data = key106ext[wParam & 0xff];
			if (data & 0x80) {
				data &= ~0x80;
			}
			else {
				keystat_senddata(0x70 | 0x80);				// PC/AT only
			}
#else
			keystat_senddata(0x70 | 0x80);				// PC/AT only
			data = key106ext[wParam & 0xff];
#endif
		}
		keystat_senddata((BYTE)(data | 0x80));
	}
	else {												// ver0.28
		if ((np2oscfg.KEYBOARD != KEY_PC98) && (wParam == 0x0c)) {
			keystat_senddata(0x70 | 0x80);				// PC/AT only
			keystat_senddata(0x47 | 0x80);
		}
	}
}

#if defined(VAEG_EXT)
void winkbd_callback(void) {
	if (knjdown) {
		if (!--knjdown) {
			winkbd_keyup(0xf3, 0);
		}
	}
}
#endif

void winkbd_roll(BOOL pcat) {

	if (pcat) {
		key106[0x21] = 0x36;
		key106[0x22] = 0x37;
	}
	else {
		key106[0x21] = 0x37;
		key106[0x22] = 0x36;
	}
}

void winkbd_setf12(UINT f12key) {

	UINT8	key;

	f12key--;
	if (f12key < (sizeof(f12keys)/sizeof(UINT8))) {
		key = f12keys[f12key];
	}
	else {
		key = NC;
	}
	key106[0x7b] = key;
}

void winkbd_resetf12(void) {

	UINT	i;

	for (i=0; i<(sizeof(f12keys)/sizeof(UINT8)); i++) {
		keystat_forcerelease(f12keys[i]);
	}
}

#if defined(VAEG_EXT)
void winkbd_setaltr(UINT altrkey) {

	UINT8	key;

	altrkey--;
	if (altrkey < (sizeof(altrkeys)/sizeof(UINT8))) {
		key = altrkeys[altrkey];
	}
	else {
		key = 0x73;	//NCにすると、key106extが参照されず、ALT(L)が認識されなくなる
					//このため、デフォルトは0x73(GRPH)とする
	}
	key106[0x12] = key;
}

void winkbd_resetaltr(void) {

	UINT	i;

	for (i=0; i<(sizeof(altrkeys)/sizeof(UINT8)); i++) {
		keystat_forcerelease(altrkeys[i]);
	}
}
#endif
