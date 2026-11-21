/*
 * i8255.c: i8255
 *
 */

#include "compiler.h"
#include "i8255.h"

#define CTRL_PORTADIR 0x10
#define CTRL_PORTBDIR 0x02
#define CTRL_PORTCHDIR 0x08
#define CTRL_PORTCLDIR 0x01

// ----

void i8255_init(I8255CFG p, I8255 s) {
	ZeroMemory(p, sizeof(_I8255CFG));
	ZeroMemory(s, sizeof(_I8255));
	p->s = s;
}

static void i8255_setmode(I8255CFG p, BYTE dat, BOOL notify);

void i8255_reset(I8255CFG p) {
	ZeroMemory(p->s, sizeof(_I8255));
	/* The peer resets itself; do not drive its inputs from here. */
	i8255_setmode(p, 0x80, FALSE);
}

// ---- CPU との I/F

void i8255_outporta(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (!(s->mode & CTRL_PORTADIR)) {
		s->porta = dat;
		if (p->busoutporta)
			p->busoutporta(dat);
	}
}

/*
 * An input port reads its pins. When the peer drives them, that is the
 * peer's output latch, even if the peer wrote it before this port became
 * an input (M157); otherwise the last value latched from the bus.
 */
static BYTE i8255_input_pins(I8255_peek peek, BYTE latched) {
	if (peek) {
		BOOL driven = FALSE;
		const BYTE value = peek(&driven);
		if (driven) {
			return value;
		}
	}
	return latched;
}

BYTE i8255_inporta(I8255CFG p) {
	I8255 s = p->s;

	if (s->mode & CTRL_PORTADIR) {
		s->porta = i8255_input_pins(p->peekporta, s->porta);
	}
	return s->porta;
}

BYTE i8255_drivenporta(I8255CFG p, BOOL *driven) {
	*driven = (p->s->mode & CTRL_PORTADIR) ? FALSE : TRUE;
	return p->s->porta;
}

BYTE i8255_drivenportb(I8255CFG p, BOOL *driven) {
	*driven = (p->s->mode & CTRL_PORTBDIR) ? FALSE : TRUE;
	return p->s->portb;
}

void i8255_outportb(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (!(s->mode & CTRL_PORTBDIR)) {
		s->portb = dat;
		if (p->busoutportb)
			p->busoutportb(dat);
	}
}

BYTE i8255_inportb(I8255CFG p) {
	I8255 s = p->s;

	if (s->mode & CTRL_PORTBDIR) {
		s->portb = i8255_input_pins(p->peekportb, s->portb);
	}
	return s->portb;
}

void i8255_outportc(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	s->portc = (s->portc & s->portcinmask) | (dat & ~s->portcinmask);
	if (s->portcinmask != 0xff) {
		if (p->busoutportc)
			p->busoutportc(dat);
	}
}

BYTE i8255_inportc(I8255CFG p) {
	return p->s->portc;
}

static void i8255_setmode(I8255CFG p, BYTE dat, BOOL notify) {
	I8255 s = p->s;

	s->mode = dat;
	s->portcinmask = 0;
	if (s->mode & CTRL_PORTCHDIR)
		s->portcinmask |= 0xf0;
	if (s->mode & CTRL_PORTCLDIR)
		s->portcinmask |= 0x0f;
	/*
	 * A mode set resets every output latch, port C's status bits included
	 * (uPD8255/i8255 data sheet). PC-8801 disk software relies on it: one
	 * loader's subsystem code sets PC4 for a transfer and drops it only by
	 * rewriting the mode word (M155).
	 */
	if (!(s->mode & CTRL_PORTADIR)) {
		s->porta = 0;
		if (notify && p->busoutporta)
			p->busoutporta(0);
	}
	if (!(s->mode & CTRL_PORTBDIR)) {
		s->portb = 0;
		if (notify && p->busoutportb)
			p->busoutportb(0);
	}
	s->portc &= s->portcinmask;
	if (notify && (s->portcinmask != 0xff) && p->busoutportc)
		p->busoutportc(s->portc);
}

void i8255_outctrl(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (dat & 0x80) {
		i8255_setmode(p, dat, TRUE);
	} else {
		int bit = (dat >> 1) & 7;
		BYTE pat = 1 << bit;
		pat &= ~s->portcinmask;
		if (dat & 1) {
			// set
			s->portc |= pat;
		} else {
			// reset
			s->portc &= ~pat;
		}
		if (s->portcinmask != 0xff) {
			if (p->busoutportc)
				p->busoutportc(s->portc);
		}
	}
}

// ---- 外部装置との I/F

void i8255_businporta(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (s->mode & CTRL_PORTADIR) {
		s->porta = dat;
	}
}

void i8255_businportb(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (s->mode & CTRL_PORTBDIR) {
		s->portb = dat;
	}
}

void i8255_businportc(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	s->portc = (s->portc & ~s->portcinmask) | (dat & s->portcinmask);
}
