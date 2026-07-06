#include	"compiler.h"
#include	"soundmng.h"
#include	"pccore.h"
#include	"fdd_mtr.h"
#if defined(SUPPORT_SWSEEKSND)
#include	"sound.h"
#include	"fdd_mtr.res"
#endif


#if defined(SUPPORT_SWSEEKSND)

enum {
	FDDMTRSND_SEEK		= 0,
	FDDMTRSND_SEEK1,
	FDDMTRSND_HEADON,
	FDDMTRSND_HEADOFF,
	FDDMTRSND_TRACKS
};

static struct {
	UINT	enable;
	struct {
		PMIXHDR	hdr;
		PMIXTRK	trk[FDDMTRSND_TRACKS];
	}		snd;
} mtrsnd;

static void fddmtrsnd_load(PMIXDAT *dat, const char *fname,
											void *fallback, UINT fallbacksize,
											UINT rate) {

	char	path[MAX_PATH];

	getbiospath(path, fname, sizeof(path));
	if (pcmmix_regfile(dat, path, rate) == SUCCESS) {
		return;
	}
	if (pcmmix_regfile(dat, fname, rate) == SUCCESS) {
		return;
	}
	if ((fallback != NULL) && (fallbacksize != 0)) {
		pcmmix_regist(dat, fallback, fallbacksize, rate);
	}
}

void fddmtrsnd_initialize(UINT rate) {

	ZeroMemory(&mtrsnd, sizeof(mtrsnd));
	if (rate == 0) {
		return;
	}
	mtrsnd.enable = 1;
	mtrsnd.snd.hdr.enable = (1 << FDDMTRSND_TRACKS) - 1;
	fddmtrsnd_load(&mtrsnd.snd.trk[FDDMTRSND_SEEK].data,
							"seek.wav", (void *)fddseek, sizeof(fddseek), rate);
	mtrsnd.snd.trk[FDDMTRSND_SEEK].flag =
										PMIXFLAG_L | PMIXFLAG_R | PMIXFLAG_LOOP;
	fddmtrsnd_load(&mtrsnd.snd.trk[FDDMTRSND_SEEK1].data,
							"seek1.wav", (void *)fddseek1, sizeof(fddseek1), rate);
	mtrsnd.snd.trk[FDDMTRSND_SEEK1].flag = PMIXFLAG_L | PMIXFLAG_R;
	fddmtrsnd_load(&mtrsnd.snd.trk[FDDMTRSND_HEADON].data,
							"headon.wav", NULL, 0, rate);
	mtrsnd.snd.trk[FDDMTRSND_HEADON].flag = PMIXFLAG_L | PMIXFLAG_R;
	fddmtrsnd_load(&mtrsnd.snd.trk[FDDMTRSND_HEADOFF].data,
							"headoff.wav", NULL, 0, rate);
	mtrsnd.snd.trk[FDDMTRSND_HEADOFF].flag = PMIXFLAG_L | PMIXFLAG_R;
	fddmtrsnd_volume(np2cfg.MOTORVOL);
}

void fddmtrsnd_bind(void) {

	if (mtrsnd.enable) {
		sound_streamregist(&mtrsnd.snd, (SOUNDCB)pcmmix_getpcm);
	}
}

void fddmtrsnd_deinitialize(void) {

	int		i;
	void	*ptr;

	for (i=0; i<FDDMTRSND_TRACKS; i++) {
		ptr = mtrsnd.snd.trk[i].data.sample;
		mtrsnd.snd.trk[i].data.sample = NULL;
		if (ptr) {
			_MFREE(ptr);
		}
	}
}

void fddmtrsnd_volume(UINT volume) {

	SINT32	vol;
	int		i;

	vol = (SINT32)((volume << 12) / 100);
	for (i=0; i<FDDMTRSND_TRACKS; i++) {
		mtrsnd.snd.trk[i].volume = vol;
	}
}

static void fddmtrsnd_play(UINT num, BOOL play) {

	PMIXTRK	*trk;

	if ((mtrsnd.enable) && (num < FDDMTRSND_TRACKS)) {
		sound_sync();
		trk = mtrsnd.snd.trk + num;
		if (play) {
			if (trk->data.sample) {
				trk->pcm = trk->data.sample;
				trk->remain = trk->data.samples;
				mtrsnd.snd.hdr.playing |= (1 << num);
			}
		}
		else {
			mtrsnd.snd.hdr.playing &= ~(1 << num);
		}
	}
}
#endif


// ----

enum {
	MOVE1TCK_MS		= 15,
	MOVEMOTOR1_MS	= 25,
	DISK1ROL_MS		= 166
};

	_FDDMTR		fddmtr;

static void fddmtr_event(void);

#if defined(SUPPORT_SWSEEKSND)
void fddmtrsnd_stop(void) {

	UINT	i;

	for (i=0; i<FDDMTRSND_TRACKS; i++) {
		fddmtrsnd_play(i, FALSE);
	}
	fddmtr.curevent = 0;
}

void fddmtrsnd_seek(BOOL one_track, UINT duration_ms) {

	if (one_track) {
		if (fddmtr.curevent < 80) {
			fddmtr_event();
			fddmtrsnd_play(FDDMTRSND_SEEK1, TRUE);
			fddmtr.curevent = 80;
			fddmtr.nextevent = GETTICK() + MOVEMOTOR1_MS;
		}
	}
	else if (fddmtr.curevent < 100) {
		fddmtr_event();
		fddmtrsnd_play(FDDMTRSND_SEEK, TRUE);
		fddmtr.curevent = 100;
		fddmtr.nextevent = GETTICK() + duration_ms;
	}
}

void fddmtrsnd_headload(BOOL one_track) {

	fddmtrsnd_play(FDDMTRSND_HEADOFF, FALSE);
	if (one_track) {
		fddmtrsnd_play(FDDMTRSND_SEEK1, FALSE);
		fddmtrsnd_play(FDDMTRSND_SEEK1, TRUE);
	}
	else {
		fddmtrsnd_play(FDDMTRSND_HEADON, FALSE);
		fddmtrsnd_play(FDDMTRSND_HEADON, TRUE);
	}
}

void fddmtrsnd_headunload(void) {

	fddmtrsnd_play(FDDMTRSND_HEADOFF, FALSE);
	fddmtrsnd_play(FDDMTRSND_HEADOFF, TRUE);
}
#endif

static void fddmtr_event(void) {

	switch(fddmtr.curevent) {
		case 100:
#if defined(SUPPORT_SWSEEKSND)
			fddmtrsnd_play(FDDMTRSND_SEEK, FALSE);
#else
			soundmng_pcmstop(SOUND_PCMSEEK);
#endif
			fddmtr.curevent = 0;
			break;

		default:
			fddmtr.curevent = 0;
			break;
	}
}

void fddmtr_initialize(void) {

#if defined(SUPPORT_SWSEEKSND)
	fddmtrsnd_stop();
#else
	soundmng_pcmstop(SOUND_PCMSEEK);
#endif
	ZeroMemory(&fddmtr, sizeof(fddmtr));
	FillMemory(fddmtr.head, sizeof(fddmtr.head), 42);
}

void fddmtr_callback(UINT time) {

	if ((fddmtr.curevent) && (time >= fddmtr.nextevent)) {
		fddmtr_event();
	}
}

void fdbiosout(NEVENTITEM item) {

	fddmtr.busy = 0;
	(void)item;
}

void fddmtr_seek(REG8 drv, REG8 c, UINT size) {

	int		regmove;
	SINT32	waitcnt;

	drv &= 3;
	regmove = c - fddmtr.head[drv];
	fddmtr.head[drv] = c;

	if (!np2cfg.MOTOR) {
		if (size) {
			fddmtr.busy = 1;
			nevent_set(NEVENT_FDBIOSBUSY, size * pccore.multiple,
												fdbiosout, NEVENT_ABSOLUTE);
		}
		return;
	}

	waitcnt = (size * DISK1ROL_MS) / (1024 * 8);
	if (regmove < 0) {
		regmove = 0 - regmove;
	}
	if (regmove == 1) {
		if (fddmtr.curevent < 80) {
			fddmtr_event();
#if defined(SUPPORT_SWSEEKSND)
			fddmtrsnd_play(FDDMTRSND_SEEK1, TRUE);
#else
			soundmng_pcmplay(SOUND_PCMSEEK1, FALSE);
#endif
			fddmtr.curevent = 80;
			fddmtr.nextevent = GETTICK() + MOVEMOTOR1_MS;
		}
	}
	else if (regmove) {
		if (fddmtr.curevent < 100) {
			fddmtr_event();
#if defined(SUPPORT_SWSEEKSND)
			fddmtrsnd_play(FDDMTRSND_SEEK, TRUE);
#else
			soundmng_pcmplay(SOUND_PCMSEEK, TRUE);
#endif
			fddmtr.curevent = 100;
			fddmtr.nextevent = GETTICK() + (regmove * MOVE1TCK_MS);
		}
		if (regmove >= 32) {
			waitcnt += DISK1ROL_MS;
		}
	}
	if (waitcnt) {
		fddmtr.busy = 1;
		nevent_setbyms(NEVENT_FDBIOSBUSY,
										waitcnt, fdbiosout, NEVENT_ABSOLUTE);
	}
	(void)drv;
}
