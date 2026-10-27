/*
 * makesprva.c: PC-88VA Sprite
 */

#include "compiler.h"
//#include	"cpucore.h"
//#include	"machine/pccore.h"
//#include	"iocore.h"
//#include	"scrnmng.h"
#include "scrndraw.h"
//#include	"scrndrawva.h"
//#include	"dispsync.h"

#include "memoryva.h"
#include "tsp.h"
#include "makesprva.h"

enum {
	SPRVA_SPRS = 0x20,
	SPRVA_MAXWIDTH = 0x400,

	TEXTSPR_WIDTH = SPRVA_MAXWIDTH,
};

/*
typedef struct {
	UINT16	sprtable;			// スプライト制御テーブル(TVRAM先頭からのオフセット)
	//BOOL	mg;					// 縦2倍拡大
} _TSP, *TSP;
*/

typedef struct {
	BOOL sw;       // 表示スイッチ
	UINT16 vlines; // 垂直サイズ(ライン)
	UINT16 yp;     // 垂直表示位置

	UINT32 spda; // 次回表示データ(TVRAM先頭からの相対アドレス)
} _SPRVA, *SPRVA;

/*
 * M141, measured with demos/tsp-sprites (TSPSPR) on the PC-88VA and
 * PC-88VA2 (photographs 2026-10-27, issue #67): a raster line shows the
 * sprites, from sprite 0 (the front) on, whose image bytes on that line
 * (4 x (XSIZE + 1) each) plus SPRVA_LINE_COST a sprite add up to at most
 * SPRVA_LINE_BYTES; the rest of that line's sprites are not drawn. N
 * sprites side by side showed about 24 of 8 bytes a line, 15 of 16, 8 of
 * 32 and all 31 of 4, the same on both machines and for 16-colour and
 * monochrome images of the same bytes.
 */
enum {
	SPRVA_LINE_BYTES = 300,
	SPRVA_LINE_COST = 4,
};

typedef struct {
	UINT screeny; // 現在処理中のラスタ(画面共通の座標系で)
	UINT y;       // 現在処理中のラスタ(スプライトの座標系で)
	_SPRVA _spr[SPRVA_SPRS];
} _SPRVAWORK;

//static	_TSP	tsp;
static _SPRVAWORK work;
//		BYTE sprraster[SURFACE_WIDTH];
BYTE sprraster[TEXTSPR_WIDTH];
// 1ラスタ分のピクセルデータ
// 各ピクセルはパレット番号(0～15)

void makesprva_initialize(void) {
	/*
	tsp.sprtable = 0x7e00;
*/
}

/* Collisions (two opaque sprite pixels on a line; with SPRON.GR only of
 * different groups, colours 1-7 and 8-15) and sprite-over (more than HSPN +
 * 1 sprites on a line) of the frame being drawn, published to the status
 * and SPROV at the next frame's start (makesprva_begin). */
static UINT8 frame_hit;
static UINT8 frame_ovs;

static void sprpixel(UINT xp, BYTE c) {
	BYTE old;

	old = sprraster[xp];
	if (old && (xp < 640) && (!tsp.gr || ((old ^ c) & 8))) {
		frame_hit |= 0x20;
	}
	sprraster[xp] = c;
}

static void drawraster(SPRVA s, BYTE *sprinfo) {
	WORD d;
	int bitcount;
	//	int bytecount;
	UINT16 xbytes; // スプライトの幅(バイト)
	UINT16 xp;     // 水平座標
	UINT8 fg;      // フォアグラウンドカラー
	UINT8 bg;      // バックグラウンドカラー
	BOOL md;       // モノクロモード
	BYTE *sprdata; // スプライトデータ

	d = LOADINTELWORD(sprinfo + 2);
	xbytes = ((d >> 11) + 1) * 4;
	xp = d & 0x03ff;
	md = d & 0x0400;
	d = LOADINTELWORD(sprinfo + 6);
	fg = (d & 0x00f0) >> 4;
	bg = d & 0x0008;
	sprdata = textmem + s->spda;
	s->spda += xbytes;

	if (md) {
		if (bg) {
			// モノクロ 背景色あり
			for (; xbytes > 0; xbytes--) {
				d = *sprdata;
				sprdata++;
				for (bitcount = 0; bitcount < 8; bitcount++) {
					sprpixel(xp, d & 0x80 ? fg : bg);
					d <<= 1;
					xp = (xp + 1) & 0x3ff;
				}
			}
		} else {
			// モノクロ 背景透明
			for (; xbytes > 0; xbytes--) {
				d = *sprdata;
				sprdata++;
				for (bitcount = 0; bitcount < 8; bitcount++) {
					if (d & 0x80)
						sprpixel(xp, fg);
					d <<= 1;
					xp = (xp + 1) & 0x3ff;
				}
			}
		}
	} else {
		// 16色
		for (; xbytes > 0; xbytes--) {
			d = *sprdata;
			sprdata++;
			// ToDo: 0以外の透明色の場合に重ねがきしないようにする
			if (d & 0xf0)
				sprpixel(xp, (BYTE)(d >> 4));
			xp = (xp + 1) & 0x3ff;
			if (d &= 0x0f)
				sprpixel(xp, (BYTE)d);
			xp = (xp + 1) & 0x3ff;
		}
	}
#if 0
	if (xp >= SURFACE_WIDTH) xp -= 0x0400;

	d = LOADINTELWORD(sprdata);
	bitcount = 0;
	bytecount = xbytes;

	if (md) {
		if (xp + xbytes * 8 <= 0) return;

		if (xp < 0) {
			int ofs;
			ofs = -xp;
			bytecount -= ofs / 8;
										// この時点で、常にxwords>0
			sprdata += ofs / 8;
			d = *sprdata;
			bitcount = ofs % 8;
			d <<= bitcount;
			xp = 0;
		}

		if (bg) {
			/*
			for (; wordcount > 0; wordcount--) {
				for (; bitcount < 16; bitcount++) {
					sprraster[xp] = d & 0x8000 ? fg : bg;
					d <<= 1;
					xp++;
				}
				sprdata += 2;
				d = LOADINTELWORD(sprdata);
				bitcount = 0;
			}
			*/
		}
		else {
			for (; bytecount > 0; bytecount--) {
				for (; bitcount < 8; bitcount++) {
					if (d & 0x80) sprraster[xp] = fg;
					d <<= 1;
					xp++;
				}
				sprdata++;
				d = *sprdata;
				bitcount = 0;
			}
		}
	}
	else {
	}

	s->spda += xbytes;
#endif
}

/*
 * M117: the CURDEF cursor (CE) is shown only while the text display runs
 * (DSPON). After SYNC without DSPON the hardware shows no cursor (a commercial
 * game, PC-88VA and VA2), while sprites with SW set stay until DSPOFF.
 */
BOOL makesprva_cursor_shown(void) {
	return (tsp.ce && tsp.dspon) ? TRUE : FALSE;
}

void makesprva_begin(void) {
	int i;
	BYTE *sprinfo;

	/* The last frame's collision and sprite-over: the status's SC (until
	 * SPROV reads it) and SPROV's byte. */
	if (frame_hit) {
		tsp.status |= 0x20;
		tsp.sprov = (UINT8)(frame_hit | (frame_ovs & 0x1f));
	}
	frame_hit = 0;
	frame_ovs = 0;

	work.y = 0;
	work.screeny = 0;

	sprinfo = textmem + tsp.sprtable;
	for (i = 0; i < SPRVA_SPRS; i++) {
		WORD d;

		d = LOADINTELWORD(sprinfo + 0);
		work._spr[i].sw = d & 0x0200;
		work._spr[i].vlines = ((d >> 10) + 1) * 4;
		work._spr[i].yp = d & 0x01ff;
		if ((i == tsp.curn) && makesprva_cursor_shown()) {
			work._spr[i].sw = TRUE;
		}
		d = LOADINTELWORD(sprinfo + 4);

		// TSPアドレスをTVRAM先頭からの相対バイトアドレスに変換
		if (d & 0x8000)
			d = (d << 1) + 0x8000;
		else
			d <<= 1;
		work._spr[i].spda = d;

		// カーソルの点滅
		if ((i == tsp.curn) && makesprva_cursor_shown() && tsp.be && (tsp.blinkcnt2 & 0x08)) {
			work._spr[i].sw = FALSE;
		}

		sprinfo += 8;
	}
}

void makesprva_blankraster(void) {
	ZeroMemory(sprraster, sizeof(sprraster));
}

void makesprva_raster(void) {
	int i;
	BYTE *sprinfo;
	SPRVA s;
	BOOL mg;

	mg = tsp.mg && !((tsp.syncparam[0] & 0xc0) == 0x40);
	/* SYNC stops the display; the sprites, as the text, show only after
	 * DSPON (TSPSPR on the VA and VA2: SYNC and SPRON alone showed none). */
	if (!tsp.spron || !tsp.dspon) {
		ZeroMemory(sprraster, sizeof(sprraster));
		if (!mg || (work.screeny & 1) == 0) {
			work.y++;
		}
		work.screeny++;
		return;
	}

	if (!mg || (work.screeny & 1) == 0) {
		BOOL shown[SPRVA_SPRS];
		UINT bytes;
		UINT count;

		/* The line's sprites from the front: those within the line's
		 * bytes are drawn; more than HSPN + 1 is a sprite-over. */
		bytes = 0;
		count = 0;
		sprinfo = textmem + tsp.sprtable;
		for (i = 0; i < SPRVA_SPRS; i++) {
			s = &work._spr[i];
			shown[i] = FALSE;
			if (s->sw && ((work.y - s->yp) & 0x01ff) < s->vlines) {
				UINT cost;

				cost = ((LOADINTELWORD(sprinfo + i * 8 + 2) >> 11) + 1) * 4 + SPRVA_LINE_COST;
				if (bytes + cost <= SPRVA_LINE_BYTES) {
					shown[i] = TRUE;
				}
				bytes += cost;
				if (++count == (UINT)tsp.hspn + 2 && !(frame_hit & 0x40)) {
					frame_hit |= 0x40;
					frame_ovs = (UINT8)i;
				}
			}
		}
		ZeroMemory(sprraster, sizeof(sprraster));
		sprinfo = textmem + tsp.sprtable + SPRVA_SPRS * 8;
		s = &work._spr[SPRVA_SPRS];
		for (i = SPRVA_SPRS - 1; i >= 0; i--) {
			s--;
			sprinfo -= 8;

			if (shown[i]) {
				drawraster(s, sprinfo);
			} else if (s->sw && ((work.y - s->yp) & 0x01ff) < s->vlines) {
				/* (not drawn: its rows still advance) */
				s->spda += ((LOADINTELWORD(sprinfo + 2) >> 11) + 1) * 4;
			}
		}

		work.y++;
	}
	work.screeny++;
}
