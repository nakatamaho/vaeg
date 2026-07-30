
// ---- plasma display

// vram off
static void SCRNCALL SDSYM(p_0)(SDRAW sdraw, int maxy) {

	BYTE	*p;
	int		y;
	int		x;

	p = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(p, NP2PAL_TEXT2);
				p += sdraw->xalign;
			}
			p -= sdraw->xbytes;
		}
		p += sdraw->yalign;
	} while(++y < maxy);

	sdraw->dst = p;
	sdraw->y = y;
}

// text or grph 1プレーン
static void SCRNCALL SDSYM(p_1)(SDRAW sdraw, int maxy) {

const BYTE	*p;
	BYTE	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph
static void SCRNCALL SDSYM(p_2)(SDRAW sdraw, int maxy) {

const BYTE	*p;
const BYTE	*q;
	BYTE	*r;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

// text + (grph:interleave)
static void SCRNCALL SDSYM(p_ti)(SDRAW sdraw, int maxy) {

const BYTE	*p;
	BYTE	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, (p[x] >> 4) + NP2PAL_TEXT);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// grph:interleave
static void SCRNCALL SDSYM(p_gi)(SDRAW sdraw, int maxy) {

const BYTE	*p;
	BYTE	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, NP2PAL_TEXT);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph:interleave
static void SCRNCALL SDSYM(p_2i)(SDRAW sdraw, int maxy) {

const BYTE	*p;
const BYTE	*q;
	BYTE	*r;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		q += SURFACE_WIDTH;
		r += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(r, (q[x] >> 4) + NP2PAL_TEXT);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		p += (SURFACE_WIDTH * 2);
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

//	grph:interleave ex
static void SCRNCALL SDSYM(p_gie)(SDRAW sdraw, int maxy) {

const BYTE	*p;
	BYTE	*q;
	int		y;
	int		x;

	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			sdraw->dirty[y+1] |= 0xff;
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_GRPH);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		q += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(q, p[x] + NP2PAL_SKIP);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += (SURFACE_WIDTH * 2);
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

//	text + grph:interleave ex
static void SCRNCALL SDSYM(p_2ie)(SDRAW sdraw, int maxy) {

const BYTE	*p;
const BYTE	*q;
	BYTE	*r;
	int		y;
	int		x;
	BYTE	c;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			sdraw->dirty[y+1] |= 0xff;
			for (x=0; x<sdraw->width; x++) {
				SDSETPIXEL(r, p[x] + q[x] + NP2PAL_GRPH);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		q += SURFACE_WIDTH;
		r += sdraw->yalign;

		if (sdraw->dirty[y+1]) {
			for (x=0; x<sdraw->width; x++) {
				c = q[x] >> 4;
				if (!c) {
					c = p[x] + NP2PALS_TXT;
				}
				SDSETPIXEL(r, c + NP2PAL_TEXT);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		p += (SURFACE_WIDTH * 2);
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

// text or grph 1プレーン(15kHz)
static void SCRNCALL SDSYM(p_1d)(SDRAW sdraw, int maxy) {

const BYTE	*p;
	BYTE	*q;
	int		a;
	int		y;
	int		x;
	int		c;

	p = sdraw->src;
	q = sdraw->dst;
	a = sdraw->yalign;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				c = p[x] + NP2PAL_GRPH;
				SDSETPIXEL(q, c);
				SDSETPIXEL((q + a), c);
				q += sdraw->xalign;
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += a * 2;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph (15kHz)
static void SCRNCALL SDSYM(p_2d)(SDRAW sdraw, int maxy) {

const BYTE	*p;
const BYTE	*q;
	BYTE	*r;
	int		a;
	int		y;
	int		x;
	int		c;

	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	a = sdraw->yalign;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x++) {
				c = p[x] + q[x] + NP2PAL_GRPH;
				SDSETPIXEL(r, c);
				SDSETPIXEL((r + a), c);
				r += sdraw->xalign;
			}
			r -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += a * 2;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

static const SDRAWFN SDSYM(p)[] = {
		SDSYM(p_0),		SDSYM(p_1),		SDSYM(p_1),		SDSYM(p_2),
		SDSYM(p_0),		SDSYM(p_ti),	SDSYM(p_gi),	SDSYM(p_2i),
		SDSYM(p_0),		SDSYM(p_ti),	SDSYM(p_gie),	SDSYM(p_2ie),
		SDSYM(p_0),		SDSYM(p_1d),	SDSYM(p_1d),	SDSYM(p_2d),
	};
