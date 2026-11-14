#ifndef VAEG_IO_HFPLUS_H
#define VAEG_IO_HFPLUS_H

/* M149: Digital Arts HyPER-FRAME+ board model (C bus, 640x400 24-bit
 * frame buffer), built from the VA hardware measurements of the M149
 * probes (docs/agents/tasks/M149_hyperframe_640x400_display.md, 8-9) and
 * the task file's section 9. Only behaviour measured on the real VA is
 * modelled:
 *   - the ports 00D0/00D3/00D4/00D5/00D6/00D7/00D8/00DA/00DC/00EE are
 *     undriven (FF) until the first write, and the stop sequence
 *     D0=00 EE=00 returns them to undriven;
 *   - OUT 00D0/00D3 hold their values for IN (the probe read D0=E1 and
 *     D3=00 back after the viewer's initialisation);
 *   - coordinates reach the frame buffer by byte writes (D7 D6 = Y,
 *     D5 D4 = X) and by word OUTs (D6, D4), and IN 00D8/00DA/00DC read
 *     the R/G/B byte at the current coordinate (12-point readback);
 *   - IN 00EE is not driven even after OUT 00EE (the probe read FF).
 * The video output is NOT modelled: on the real VA the board produced
 * no signal without the CN2 (main RGB) input cable, and no display
 * enable was found in D0/EE/D3. The frame buffer is exposed through
 * hfplus_framebuffer() and hfplus_write_ppm() for tests and debugging.
 */

enum {
    HFPLUS_W = 640,
    HFPLUS_H = 400,
    /* 6 banks of 128KiB; rows 400 and up hold the special-coordinate
     * area the viewer writes at Y=0191/0193 (semantics unmeasured). */
    HFPLUS_FB_SIZE = 786432
};

/* How the tentative display shows the board's frame buffer in the vaeg
 * window while the board is driven. PiP is the default so the VA text
 * screen (the probes' CSV log) stays visible. */
enum {
    HFPLUS_VIEW_PIP = 0,
    HFPLUS_VIEW_FULL,
    HFPLUS_VIEW_OFF,
    HFPLUS_VIEW_COUNT
};

extern UINT8 hfplus_view;

typedef struct {
    UINT8 control; /* last OUT 00D0 */
    UINT8 d3;      /* last OUT 00D3 */
    UINT8 ee;      /* last OUT 00EE */
    UINT8 driven;  /* ports answer while set */
    UINT16 x;      /* 00D4 low, 00D5 high */
    UINT16 y;      /* 00D6 low, 00D7 high */
    UINT16 padding;
} _HFPLUS, *HFPLUS;

#ifdef __cplusplus
extern "C" {
#endif

extern _HFPLUS hfplus;

void hfplus_reset(void);
void hfplus_bind(void);
UINT8 *hfplus_framebuffer(void);
int hfplus_write_ppm(const char *path);

#ifdef __cplusplus
}
#endif

#endif
