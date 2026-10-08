/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Minimal PNG decoder for the frontend's embedded artwork (vaeg M106): zlib
 * inflate (stored, fixed and dynamic Huffman blocks) and the five PNG row
 * filters, for 8-bit RGB or RGBA, non-interlaced images. Anything else is
 * rejected; the result is always 8-bit RGBA.
 */

#include "compiler.h"
#include "pngdecode.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
	const BYTE *src;
	size_t size;
	size_t pos;
	UINT32 bitbuf;
	int bitcnt;
	BYTE *out;
	size_t outsize;
	size_t outpos;
} INFLATE;

typedef struct {
	UINT16 counts[16];
	UINT16 symbols[288];
} HUFFMAN;

static int inflate_bits(INFLATE *s, int need) {
	int value;

	while (s->bitcnt < need) {
		if (s->pos >= s->size) {
			return -1;
		}
		s->bitbuf |= (UINT32)s->src[s->pos++] << s->bitcnt;
		s->bitcnt += 8;
	}
	value = (int)(s->bitbuf & ((1UL << need) - 1));
	s->bitbuf >>= need;
	s->bitcnt -= need;
	return value;
}

static BOOL huffman_build(HUFFMAN *h, const BYTE *lengths, int n) {
	UINT16 offsets[16];
	int left;
	int i;

	memset(h->counts, 0, sizeof(h->counts));
	for (i = 0; i < n; i++) {
		h->counts[lengths[i]]++;
	}
	h->counts[0] = 0;
	left = 1;
	for (i = 1; i < 16; i++) {
		left <<= 1;
		left -= h->counts[i];
		if (left < 0) {
			return FALSE; /* over-subscribed */
		}
	}
	offsets[1] = 0;
	for (i = 1; i < 15; i++) {
		offsets[i + 1] = (UINT16)(offsets[i] + h->counts[i]);
	}
	for (i = 0; i < n; i++) {
		if (lengths[i]) {
			h->symbols[offsets[lengths[i]]++] = (UINT16)i;
		}
	}
	return TRUE;
}

static int huffman_decode(INFLATE *s, const HUFFMAN *h) {
	int code = 0;
	int first = 0;
	int index = 0;
	int len;

	for (len = 1; len < 16; len++) {
		const int bit = inflate_bits(s, 1);
		int count;

		if (bit < 0) {
			return -1;
		}
		code |= bit;
		count = h->counts[len];
		if (code - count < first) {
			return h->symbols[index + (code - first)];
		}
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}
	return -1;
}

static const UINT16 length_base[29] = {3,  4,  5,  6,   7,   8,   9,   10,  11, 13,
                                       15, 17, 19, 23,  27,  31,  35,  43,  51, 59,
                                       67, 83, 99, 115, 131, 163, 195, 227, 258};
static const BYTE length_extra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                      2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const UINT16 dist_base[30] = {1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
                                     33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
                                     1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static const BYTE dist_extra[30] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static BOOL inflate_codes(INFLATE *s, const HUFFMAN *lencode, const HUFFMAN *distcode) {
	for (;;) {
		int symbol = huffman_decode(s, lencode);

		if (symbol < 0) {
			return FALSE;
		}
		if (symbol < 256) {
			if (s->outpos >= s->outsize) {
				return FALSE;
			}
			s->out[s->outpos++] = (BYTE)symbol;
		} else if (symbol == 256) {
			return TRUE;
		} else {
			int len;
			int extra;
			size_t dist;

			symbol -= 257;
			if (symbol >= 29) {
				return FALSE;
			}
			extra = inflate_bits(s, length_extra[symbol]);
			if (extra < 0) {
				return FALSE;
			}
			len = length_base[symbol] + extra;
			symbol = huffman_decode(s, distcode);
			if ((symbol < 0) || (symbol >= 30)) {
				return FALSE;
			}
			extra = inflate_bits(s, dist_extra[symbol]);
			if (extra < 0) {
				return FALSE;
			}
			dist = (size_t)dist_base[symbol] + (size_t)extra;
			if ((dist > s->outpos) || (s->outpos + (size_t)len > s->outsize)) {
				return FALSE;
			}
			while (len--) {
				s->out[s->outpos] = s->out[s->outpos - dist];
				s->outpos++;
			}
		}
	}
}

static BOOL inflate_stored(INFLATE *s) {
	size_t len;

	s->bitbuf = 0;
	s->bitcnt = 0;
	if (s->pos + 4 > s->size) {
		return FALSE;
	}
	len = (size_t)s->src[s->pos] | ((size_t)s->src[s->pos + 1] << 8);
	if ((len ^ 0xffff) != ((size_t)s->src[s->pos + 2] | ((size_t)s->src[s->pos + 3] << 8))) {
		return FALSE;
	}
	s->pos += 4;
	if ((s->pos + len > s->size) || (s->outpos + len > s->outsize)) {
		return FALSE;
	}
	memcpy(s->out + s->outpos, s->src + s->pos, len);
	s->pos += len;
	s->outpos += len;
	return TRUE;
}

static BOOL inflate_fixed(INFLATE *s) {
	static HUFFMAN lencode;
	static HUFFMAN distcode;
	static BOOL ready = FALSE;

	if (!ready) {
		BYTE lengths[288];
		int i;

		for (i = 0; i < 144; i++) {
			lengths[i] = 8;
		}
		for (; i < 256; i++) {
			lengths[i] = 9;
		}
		for (; i < 280; i++) {
			lengths[i] = 7;
		}
		for (; i < 288; i++) {
			lengths[i] = 8;
		}
		huffman_build(&lencode, lengths, 288);
		for (i = 0; i < 30; i++) {
			lengths[i] = 5;
		}
		huffman_build(&distcode, lengths, 30);
		ready = TRUE;
	}
	return inflate_codes(s, &lencode, &distcode);
}

static BOOL inflate_dynamic(INFLATE *s) {
	static const BYTE order[19] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
	                               11, 4,  12, 3, 13, 2, 14, 1, 15};
	BYTE lengths[320];
	HUFFMAN lencode;
	HUFFMAN distcode;
	int nlen;
	int ndist;
	int ncode;
	int index;

	nlen = inflate_bits(s, 5);
	ndist = inflate_bits(s, 5);
	ncode = inflate_bits(s, 4);
	if ((nlen < 0) || (ndist < 0) || (ncode < 0)) {
		return FALSE;
	}
	nlen += 257;
	ndist += 1;
	ncode += 4;
	if ((nlen > 286) || (ndist > 30)) {
		return FALSE;
	}
	memset(lengths, 0, sizeof(lengths));
	for (index = 0; index < ncode; index++) {
		const int len = inflate_bits(s, 3);
		if (len < 0) {
			return FALSE;
		}
		lengths[order[index]] = (BYTE)len;
	}
	if (!huffman_build(&lencode, lengths, 19)) {
		return FALSE;
	}
	index = 0;
	while (index < nlen + ndist) {
		int symbol = huffman_decode(s, &lencode);
		int repeat;
		BYTE value;

		if (symbol < 0) {
			return FALSE;
		}
		if (symbol < 16) {
			lengths[index++] = (BYTE)symbol;
			continue;
		}
		if (symbol == 16) {
			if (index == 0) {
				return FALSE;
			}
			value = lengths[index - 1];
			repeat = inflate_bits(s, 2);
			repeat = (repeat < 0) ? -1 : repeat + 3;
		} else if (symbol == 17) {
			value = 0;
			repeat = inflate_bits(s, 3);
			repeat = (repeat < 0) ? -1 : repeat + 3;
		} else {
			value = 0;
			repeat = inflate_bits(s, 7);
			repeat = (repeat < 0) ? -1 : repeat + 11;
		}
		if ((repeat < 0) || (index + repeat > nlen + ndist)) {
			return FALSE;
		}
		while (repeat--) {
			lengths[index++] = value;
		}
	}
	if (lengths[256] == 0) {
		return FALSE;
	}
	if (!huffman_build(&lencode, lengths, nlen) ||
	    !huffman_build(&distcode, lengths + nlen, ndist)) {
		return FALSE;
	}
	return inflate_codes(s, &lencode, &distcode);
}

/* zlib stream (RFC 1950/1951) into a buffer of exactly outsize bytes */
static BOOL zlib_inflate(const BYTE *src, size_t size, BYTE *out, size_t outsize) {
	INFLATE s;
	int last;

	if ((size < 2) || ((src[0] & 0x0f) != 8) || ((((UINT)src[0] << 8) | src[1]) % 31) ||
	    (src[1] & 0x20)) {
		return FALSE;
	}
	memset(&s, 0, sizeof(s));
	s.src = src;
	s.size = size;
	s.pos = 2;
	s.out = out;
	s.outsize = outsize;
	do {
		int type;
		BOOL ok;

		last = inflate_bits(&s, 1);
		type = inflate_bits(&s, 2);
		if ((last < 0) || (type < 0)) {
			return FALSE;
		}
		switch (type) {
		case 0:
			ok = inflate_stored(&s);
			break;
		case 1:
			ok = inflate_fixed(&s);
			break;
		case 2:
			ok = inflate_dynamic(&s);
			break;
		default:
			ok = FALSE;
			break;
		}
		if (!ok) {
			return FALSE;
		}
	} while (!last);
	return (s.outpos == outsize) ? TRUE : FALSE;
}

static UINT32 load_be32(const BYTE *p) {
	return ((UINT32)p[0] << 24) | ((UINT32)p[1] << 16) | ((UINT32)p[2] << 8) | p[3];
}

static BYTE paeth(BYTE a, BYTE b, BYTE c) {
	const int p = (int)a + (int)b - (int)c;
	const int pa = abs(p - (int)a);
	const int pb = abs(p - (int)b);
	const int pc = abs(p - (int)c);

	if ((pa <= pb) && (pa <= pc)) {
		return a;
	}
	return (pb <= pc) ? b : c;
}

BYTE *vaeg_png_decode_rgba(const BYTE *data, size_t size, UINT *width, UINT *height) {
	static const BYTE signature[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
	BYTE *idat = NULL;
	size_t idatsize = 0;
	BYTE *raw = NULL;
	BYTE *rgba = NULL;
	size_t pos;
	UINT32 w = 0;
	UINT32 h = 0;
	UINT channels = 0;
	size_t stride;
	UINT32 y;

	if ((data == NULL) || (size < 8) || memcmp(data, signature, 8)) {
		return NULL;
	}
	for (pos = 8; pos + 12 <= size;) {
		const UINT32 len = load_be32(data + pos);
		const BYTE *type = data + pos + 4;
		const BYTE *body = data + pos + 8;

		if ((len > size) || (pos + 12 + len > size)) {
			goto fail;
		}
		if (!memcmp(type, "IHDR", 4)) {
			if ((len < 13) || (body[8] != 8) || (body[10] != 0) || (body[11] != 0) ||
			    (body[12] != 0)) {
				goto fail; /* 8-bit, deflate, adaptive filtering, no interlace only */
			}
			w = load_be32(body);
			h = load_be32(body + 4);
			channels = (body[9] == 6) ? 4 : (body[9] == 2) ? 3 : 0;
			if ((channels == 0) || (w == 0) || (h == 0) || (w > 8192) || (h > 8192)) {
				goto fail;
			}
		} else if (!memcmp(type, "IDAT", 4)) {
			BYTE *grown = (BYTE *)realloc(idat, idatsize + len);
			if (grown == NULL) {
				goto fail;
			}
			idat = grown;
			memcpy(idat + idatsize, body, len);
			idatsize += len;
		} else if (!memcmp(type, "IEND", 4)) {
			break;
		}
		pos += 12 + len;
	}
	if ((channels == 0) || (idat == NULL)) {
		goto fail;
	}
	stride = (size_t)w * channels;
	raw = (BYTE *)malloc((stride + 1) * h);
	rgba = (BYTE *)malloc((size_t)w * h * 4);
	if ((raw == NULL) || (rgba == NULL) || !zlib_inflate(idat, idatsize, raw, (stride + 1) * h)) {
		goto fail;
	}
	for (y = 0; y < h; y++) {
		BYTE *line = raw + y * (stride + 1);
		const BYTE filter = line[0];
		BYTE *cur = line + 1;
		const BYTE *prev = (y > 0) ? raw + (y - 1) * (stride + 1) + 1 : NULL;
		size_t x;

		for (x = 0; x < stride; x++) {
			const BYTE a = (x >= channels) ? cur[x - channels] : 0;
			const BYTE b = prev ? prev[x] : 0;
			const BYTE c = (prev && (x >= channels)) ? prev[x - channels] : 0;

			switch (filter) {
			case 0:
				break;
			case 1:
				cur[x] = (BYTE)(cur[x] + a);
				break;
			case 2:
				cur[x] = (BYTE)(cur[x] + b);
				break;
			case 3:
				cur[x] = (BYTE)(cur[x] + ((a + b) >> 1));
				break;
			case 4:
				cur[x] = (BYTE)(cur[x] + paeth(a, b, c));
				break;
			default:
				goto fail;
			}
		}
		for (x = 0; x < w; x++) {
			BYTE *d = rgba + ((size_t)y * w + x) * 4;
			const BYTE *p = cur + x * channels;
			d[0] = p[0];
			d[1] = p[1];
			d[2] = p[2];
			d[3] = (channels == 4) ? p[3] : 0xff;
		}
	}
	free(idat);
	free(raw);
	*width = w;
	*height = h;
	return rgba;

fail:
	free(idat);
	free(raw);
	free(rgba);
	return NULL;
}
