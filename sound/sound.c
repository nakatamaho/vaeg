#include "compiler.h"
#include "wavefile.h"
#include "dosio.h"
#include "soundmng.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "sound.h"
#include "sound_generation.h"
#include "sound_capture.h"
#include "sndcsec.h"
#include "beep.h"
#include "getsnd.h"

SOUNDCFG soundcfg;

#define STREAM_CBMAX 16
#define SOUND_CAPTURE_CHUNK_FRAMES 512

typedef VAEG_SOUND_GENERATOR_SLOT CBTBL;

typedef struct {
	SINT32 *buffer;
	SINT32 *ptr;
	UINT samples;
	UINT reserve;
	UINT remain;
	UINT64 generated_sample_count;
	UINT32 generation_epoch;
	BOOL accounting_overflow;
#if defined(SUPPORT_WAVEREC)
	WAVEWR rec;
#endif
	CBTBL *cbreg;
	CBTBL cb[STREAM_CBMAX];
} SNDSTREAM;

static SNDSTREAM sndstream;

typedef struct {
	SINT16 *buffer;
	SINT32 *scratch;
	VAEG_SOUND_CAPTURE_FIFO fifo;
	SOUND_RECORDING_ERROR error;
	UINT64 playback_dropped_frames;
	UINT64 playback_underrun_frames;
	BOOL active;
	BOOL failed;
} SOUNDRECORDING;

static SOUNDRECORDING sndrecording;
static volatile int locks = 0;

static void sound_recording_fail_locked(SOUND_RECORDING_ERROR error) {
	sndrecording.active = FALSE;
	sndrecording.failed = TRUE;
	sndrecording.error = error;
}

static void streamreset(void) {
	SNDCSEC_ENTER;
	if (sndrecording.active) {
		sound_recording_fail_locked(SOUND_RECORDING_ERROR_EPOCH);
	}
	sndstream.ptr = sndstream.buffer;
	sndstream.remain = sndstream.samples + sndstream.reserve;
	sndstream.generated_sample_count = 0;
	sndstream.accounting_overflow = FALSE;
	sndstream.generation_epoch++;
	sndstream.cbreg = sndstream.cb;
	SNDCSEC_LEAVE;
}

static void sound_account_generated(UINT count) {
	if (UINT64_MAX - sndstream.generated_sample_count < count) {
		sndstream.generated_sample_count = UINT64_MAX;
		sndstream.accounting_overflow = TRUE;
	} else {
		sndstream.generated_sample_count += count;
	}
}

static void streamprepare(UINT samples) {
	UINT count;

	count = min(sndstream.remain, samples);
	if (count) {
		count = vaeg_sound_generate_s32_stereo(
			sndstream.ptr, count, count, sndstream.cb,
			(UINT)(sndstream.cbreg - sndstream.cb));
		sound_account_generated(count);
		sndstream.ptr += count * 2;
		sndstream.remain -= count;
	}
}

static void sound_playback_append_locked(const SINT32 *source, UINT frames) {
	UINT capacity;
	UINT pending;
	UINT available;
	UINT copied;

	if (source == NULL || frames == 0u) {
		return;
	}
	capacity = sndstream.samples + sndstream.reserve;
	pending = capacity - sndstream.remain;
	available = capacity - pending;
	copied = min(frames, available);
	if (copied != 0u) {
		CopyMemory(sndstream.buffer + pending * 2u, source,
		           copied * 2u * sizeof(*source));
		pending += copied;
		sndstream.ptr = sndstream.buffer + pending * 2u;
		sndstream.remain = capacity - pending;
	}
	if (copied < frames && UINT64_MAX - sndrecording.playback_dropped_frames >= frames - copied) {
		sndrecording.playback_dropped_frames += frames - copied;
	} else if (copied < frames) {
		sndrecording.playback_dropped_frames = UINT64_MAX;
	}
}

/* Return 1 when the complete request was generated, 0 when it was rejected
 * before synthesis, and -1 when a post-generation accounting failure occurred. */
static int sound_recording_generate_locked(UINT samples) {
	UINT remaining;
	UINT count;
	VAEG_SOUND_CAPTURE_RESULT capture_result;

	if (!sndrecording.active) {
		return 0;
	}
	if (!vaeg_sound_capture_fifo_can_append(&sndrecording.fifo, samples)) {
		sound_recording_fail_locked(SOUND_RECORDING_ERROR_CAPACITY);
		return 0;
	}
	remaining = samples;
	while (remaining != 0u) {
		count = min(remaining, SOUND_CAPTURE_CHUNK_FRAMES);
		count = vaeg_sound_generate_s32_stereo(
			sndrecording.scratch, SOUND_CAPTURE_CHUNK_FRAMES, count,
			sndstream.cb, (UINT)(sndstream.cbreg - sndstream.cb));
		if (count == 0u) {
			sound_recording_fail_locked(SOUND_RECORDING_ERROR_OVERFLOW);
			return -1;
		}
		sound_account_generated(count);
		if (sndstream.accounting_overflow) {
			sound_recording_fail_locked(SOUND_RECORDING_ERROR_OVERFLOW);
			return -1;
		}
		capture_result = vaeg_sound_capture_fifo_append_s32(
			&sndrecording.fifo, sndrecording.scratch, count,
			!soundmng_isenabled());
		if (capture_result != VAEG_SOUND_CAPTURE_OK) {
			sound_recording_fail_locked(
				(capture_result == VAEG_SOUND_CAPTURE_CAPACITY) ?
				SOUND_RECORDING_ERROR_CAPACITY : SOUND_RECORDING_ERROR_OVERFLOW);
			return -1;
		}
		sound_playback_append_locked(sndrecording.scratch, count);
		remaining -= count;
	}
	return 1;
}

#if defined(SUPPORT_WAVEREC)
// ---- wave rec

BOOL sound_recstart(const char *filename) {
	WAVEWR rec;

	sound_recstop();
	if (sndstream.buffer == NULL || sndrecording.active) {
		return (FAILURE);
	}
	rec = wavewr_open(filename, soundcfg.rate, 16, 2);
	sndstream.rec = rec;
	if (rec) {
		return (SUCCESS);
	}
	return (FAILURE);
}

void sound_recstop(void) {
	WAVEWR rec;

	rec = sndstream.rec;
	sndstream.rec = NULL;
	wavewr_close(rec);
}

static void streamfilewrite(UINT samples) {
	UINT count;
	SINT32 buf32[2 * 512];
	BYTE buf[2 * 2 * 512];
	UINT r;
	UINT i;
	SINT32 samp;

	while (samples) {
		count = min(samples, 512);
		count = vaeg_sound_generate_s32_stereo(
			buf32, count, count, sndstream.cb,
			(UINT)(sndstream.cbreg - sndstream.cb));
		sound_account_generated(count);
		r = min(sndstream.remain, count);
		if (r) {
			CopyMemory(sndstream.ptr, buf32, r * 2 * sizeof(SINT32));
			sndstream.ptr += r * 2;
			sndstream.remain -= r;
		}
		for (i = 0; i < count * 2; i++) {
			samp = buf32[i];
			if (samp > 32767) {
				samp = 32767;
			} else if (samp < -32768) {
				samp = -32768;
			}
			// little endianなので satuation_s16は使えない
			buf[i * 2 + 0] = (BYTE)samp;
			buf[i * 2 + 1] = (BYTE)(samp >> 8);
		}
		wavewr_write(sndstream.rec, buf, count * 4);
		samples -= count;
	}
}

static void filltailsample(UINT count) {
	SINT32 *ptr;
	UINT orgsize;
	SINT32 sampl;
	SINT32 sampr;

	count = min(sndstream.remain, count);
	if (count) {
		ptr = sndstream.ptr;
		orgsize = (ptr - sndstream.buffer) / 2;
		if (orgsize == 0) {
			sampl = 0;
			sampr = 0;
		} else {
			sampl = *(ptr - 2);
			sampr = *(ptr - 1);
		}
		sndstream.ptr += count * 2;
		sndstream.remain -= count;
		do {
			ptr[0] = sampl;
			ptr[1] = sampr;
			ptr += 2;
		} while (--count);
	}
}
#endif

// ----

BOOL sound_create(UINT rate, UINT ms) {
	UINT samples;
	UINT reserve;

	ZeroMemory(&sndstream, sizeof(sndstream));
	ZeroMemory(&sndrecording, sizeof(sndrecording));
	switch (rate) {
	case 11025:
	case 22050:
	case 44100:
		break;

	default:
		return (FAILURE);
	}
	samples = soundmng_create(rate, ms);
	if (samples == 0) {
		goto scre_err1;
	}
	soundmng_reset();

	soundcfg.rate = rate;
	sound_changeclock();

#if defined(SOUNDRESERVE)
	reserve = rate * SOUNDRESERVE / 1000;
#else
	reserve = 0;
#endif
	sndstream.buffer = (SINT32 *)_MALLOC((samples + reserve) * 2 * sizeof(SINT32), "stream");
	if (sndstream.buffer == NULL) {
		goto scre_err2;
	}
	sndstream.samples = samples;
	sndstream.reserve = reserve;

	SNDCSEC_INIT;
	streamreset();
	return (SUCCESS);

scre_err2:
	soundmng_destroy();

scre_err1:
	return (FAILURE);
}

SOUND_RECORDING_ERROR sound_recording_start(UINT capacity_frames) {
	SINT16 *capture_buffer;
	SINT32 *scratch;
	UINT capacity;
	UINT pending;
	UINT64 first_sample;
	VAEG_SOUND_CAPTURE_RESULT capture_result;

	if (sndstream.buffer == NULL) {
		return SOUND_RECORDING_ERROR_NOT_READY;
	}
#if defined(SUPPORT_WAVEREC)
	if (sndstream.rec != NULL) {
		return SOUND_RECORDING_ERROR_BUSY;
	}
#endif
	if (capacity_frames == 0u || capacity_frames > UINT_MAX / 2u ||
	    capacity_frames > UINT_MAX / (2u * sizeof(SINT16))) {
		return SOUND_RECORDING_ERROR_INVALID_CAPACITY;
	}
	if (sndrecording.buffer != NULL || sndrecording.active) {
		return SOUND_RECORDING_ERROR_BUSY;
	}
	capture_buffer = (SINT16 *)_MALLOC(
		capacity_frames * 2u * sizeof(SINT16), "recording audio");
	scratch = (SINT32 *)_MALLOC(
		SOUND_CAPTURE_CHUNK_FRAMES * 2u * sizeof(SINT32), "recording audio scratch");
	if (capture_buffer == NULL || scratch == NULL) {
		if (capture_buffer != NULL) {
			_MFREE(capture_buffer);
		}
		if (scratch != NULL) {
			_MFREE(scratch);
		}
		return SOUND_RECORDING_ERROR_ALLOCATION;
	}

	soundmng_stop();
	SNDCSEC_ENTER;
	if (locks != 0 || sndrecording.buffer != NULL || sndrecording.active) {
		SNDCSEC_LEAVE;
		_MFREE(capture_buffer);
		_MFREE(scratch);
		soundmng_play();
		return SOUND_RECORDING_ERROR_BUSY;
	}
	capacity = sndstream.samples + sndstream.reserve;
	pending = capacity - sndstream.remain;
	if (sndstream.accounting_overflow || pending > capacity_frames ||
	    sndstream.generated_sample_count < pending) {
		SNDCSEC_LEAVE;
		_MFREE(capture_buffer);
		_MFREE(scratch);
		soundmng_play();
		return sndstream.accounting_overflow ? SOUND_RECORDING_ERROR_OVERFLOW :
		                                      SOUND_RECORDING_ERROR_CAPACITY;
	}
	first_sample = sndstream.generated_sample_count - pending;
	capture_result = vaeg_sound_capture_fifo_init(
		&sndrecording.fifo, capture_buffer, capacity_frames,
		first_sample, sndstream.generation_epoch);
	if (capture_result != VAEG_SOUND_CAPTURE_OK) {
		SNDCSEC_LEAVE;
		_MFREE(capture_buffer);
		_MFREE(scratch);
		soundmng_play();
		return SOUND_RECORDING_ERROR_INVALID_CAPACITY;
	}
	capture_result = vaeg_sound_capture_fifo_append_s32(
		&sndrecording.fifo, sndstream.buffer, pending, FALSE);
	if (capture_result != VAEG_SOUND_CAPTURE_OK) {
		vaeg_sound_capture_fifo_clear(&sndrecording.fifo);
		SNDCSEC_LEAVE;
		_MFREE(capture_buffer);
		_MFREE(scratch);
		soundmng_play();
		return (capture_result == VAEG_SOUND_CAPTURE_CAPACITY) ?
			SOUND_RECORDING_ERROR_CAPACITY : SOUND_RECORDING_ERROR_OVERFLOW;
	}
	sndrecording.buffer = capture_buffer;
	sndrecording.scratch = scratch;
	sndrecording.error = SOUND_RECORDING_ERROR_OK;
	sndrecording.playback_dropped_frames = 0;
	sndrecording.playback_underrun_frames = 0;
	sndrecording.failed = FALSE;
	sndrecording.active = TRUE;
	SNDCSEC_LEAVE;
	soundmng_play();
	return SOUND_RECORDING_ERROR_OK;
}

void sound_recording_stop(void) {
	soundmng_stop();
	if (sndstream.buffer != NULL) {
		SNDCSEC_ENTER;
		if (sndrecording.active) {
			sndrecording.active = FALSE;
		}
		SNDCSEC_LEAVE;
	}
	soundmng_play();
}

void sound_recording_release(void) {
	SINT16 *capture_buffer;
	SINT32 *scratch;

	sound_recording_stop();
	if (sndstream.buffer == NULL) {
		return;
	}
	SNDCSEC_ENTER;
	capture_buffer = sndrecording.buffer;
	scratch = sndrecording.scratch;
	sndrecording.buffer = NULL;
	sndrecording.scratch = NULL;
	vaeg_sound_capture_fifo_clear(&sndrecording.fifo);
	sndrecording.error = SOUND_RECORDING_ERROR_OK;
	sndrecording.failed = FALSE;
	sndrecording.playback_dropped_frames = 0;
	sndrecording.playback_underrun_frames = 0;
	SNDCSEC_LEAVE;
	if (capture_buffer != NULL) {
		_MFREE(capture_buffer);
	}
	if (scratch != NULL) {
		_MFREE(scratch);
	}
}

BOOL sound_recording_is_active(void) {
	BOOL active;

	if (sndstream.buffer == NULL) {
		return FALSE;
	}
	SNDCSEC_ENTER;
	active = sndrecording.active;
	SNDCSEC_LEAVE;
	return active;
}

UINT sound_recording_read(SINT16 *destination, UINT capacity_frames,
	                      UINT64 *sample_begin, UINT32 *epoch) {
	UINT count;

	if (sndstream.buffer == NULL || destination == NULL || capacity_frames == 0u) {
		return 0u;
	}
	SNDCSEC_ENTER;
	count = vaeg_sound_capture_fifo_read(
		&sndrecording.fifo, destination, capacity_frames, sample_begin, epoch);
	SNDCSEC_LEAVE;
	return count;
}

BOOL sound_recording_get_state(SOUNDRECORDINGSTATE *state) {
	if (state == NULL) {
		return FAILURE;
	}
	ZeroMemory(state, sizeof(*state));
	if (sndstream.buffer == NULL) {
		return FAILURE;
	}
	SNDCSEC_ENTER;
	state->rate = soundcfg.rate;
	state->capacity_frames = sndrecording.fifo.capacity_frames;
	state->pending_frames = vaeg_sound_capture_fifo_pending(&sndrecording.fifo);
	state->first_sample = sndrecording.fifo.first_sample;
	state->next_sample = sndrecording.fifo.next_sample;
	state->produced_frames = sndrecording.fifo.produced_frames;
	state->consumed_frames = sndrecording.fifo.consumed_frames;
	state->playback_dropped_frames = sndrecording.playback_dropped_frames;
	state->playback_underrun_frames = sndrecording.playback_underrun_frames;
	state->epoch = sndrecording.fifo.epoch;
	state->error = sndrecording.error;
	state->active = sndrecording.active;
	state->failed = sndrecording.failed;
	SNDCSEC_LEAVE;
	return SUCCESS;
}

UINT sound_playback_read_s16(SINT16 *destination, UINT destination_bytes,
	                         UINT requested_frames) {
	UINT capacity;
	UINT pending;
	UINT destination_frames;
	UINT count;
	UINT remaining;

	if (destination == NULL || destination_bytes < 2u * sizeof(SINT16) ||
	    requested_frames == 0u || sndstream.buffer == NULL) {
		return 0u;
	}
	destination_frames = destination_bytes / (2u * sizeof(SINT16));
	count = min(requested_frames, destination_frames);
	SNDCSEC_ENTER;
	capacity = sndstream.samples + sndstream.reserve;
	pending = capacity - sndstream.remain;
	count = min(count, pending);
	if (count != 0u) {
		if (soundmng_isenabled()) {
			(void)vaeg_sound_pack_s16_or_silence(
				destination, count * 2u * sizeof(SINT16),
				sndstream.buffer, count * 2u);
		} else {
			ZeroMemory(destination, count * 2u * sizeof(SINT16));
		}
		remaining = pending - count;
		if (remaining != 0u) {
			CopyMemory(sndstream.buffer, sndstream.buffer + count * 2u,
			           remaining * 2u * sizeof(SINT32));
		}
		sndstream.ptr = sndstream.buffer + remaining * 2u;
		sndstream.remain = capacity - remaining;
	}
	if (sndrecording.active && count < requested_frames &&
	    UINT64_MAX - sndrecording.playback_underrun_frames >= requested_frames - count) {
		sndrecording.playback_underrun_frames += requested_frames - count;
	} else if (sndrecording.active && count < requested_frames) {
		sndrecording.playback_underrun_frames = UINT64_MAX;
	}
	SNDCSEC_LEAVE;
	return count;
}

void sound_destroy(void) {
	if (sndstream.buffer) {
		sound_recording_release();
#if defined(SUPPORT_WAVEREC)
		sound_recstop();
#endif
		soundmng_stop();
		streamreset();
		soundmng_destroy();
		SNDCSEC_TERM;
		_MFREE(sndstream.buffer);
		sndstream.buffer = NULL;
	}
}

void sound_reset(void) {
	if (sndstream.buffer) {
		if (sndrecording.active) {
			sound_recording_stop();
			SNDCSEC_ENTER;
			if (sndrecording.buffer != NULL) {
				sound_recording_fail_locked(SOUND_RECORDING_ERROR_EPOCH);
			}
			SNDCSEC_LEAVE;
		}
		soundmng_reset();
		streamreset();
		soundcfg.lastclock = CPU_CLOCK;
		beep_eventreset();
	}
}

void sound_changeclock(void) {
	UINT32 clock;
	UINT hz;
	UINT hzmax;

	if (sndstream.buffer == NULL) {
		return;
	}
	if (sndrecording.active) {
		sound_recording_stop();
		SNDCSEC_ENTER;
		if (sndrecording.buffer != NULL) {
			sound_recording_fail_locked(SOUND_RECORDING_ERROR_EPOCH);
		}
		SNDCSEC_LEAVE;
	}

	// とりあえず 25で割り切れる。
	clock = pccore.realclock / 25;
	hz = soundcfg.rate / 25;

	// で、クロック数に合せて調整。(64bit演算しろよな的)
	hzmax = (1 << (32 - 8)) / (clock >> 8);
	while (hzmax < hz) {
		clock = (clock + 1) >> 1;
		hz = (hz + 1) >> 1;
	}
	TRACEOUT(("hzbase/clockbase = %d/%d", hz, clock));
	soundcfg.hzbase = hz;
	soundcfg.clockbase = clock;
	soundcfg.minclock = 2 * clock / hz;
	soundcfg.lastclock = CPU_CLOCK;
}

void sound_streamregist(void *hdl, SOUNDCB cbfn) {
	if (sndstream.buffer) {
		if ((cbfn) && (sndstream.cbreg < (sndstream.cb + STREAM_CBMAX))) {
			sndstream.cbreg->hdl = hdl;
			sndstream.cbreg->cbfn = cbfn;
			sndstream.cbreg++;
		}
	}
}

// ----

void sound_sync(void) {
	UINT32 length;

	if (sndstream.buffer == NULL) {
		return;
	}

	length = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK - soundcfg.lastclock;
	if (length < soundcfg.minclock) {
		return;
	}
	length = (length * soundcfg.hzbase) / soundcfg.clockbase;
	if (length == 0) {
		return;
	}
	SNDCSEC_ENTER;
#if defined(SUPPORT_WAVEREC)
	if (sndstream.rec) {
		streamfilewrite(length);
	} else
#endif
	if (sndrecording.active) {
		if (sound_recording_generate_locked(length) == 0) {
			streamprepare(length);
		}
	} else {
		streamprepare(length);
	}
	soundcfg.lastclock += length * soundcfg.clockbase / soundcfg.hzbase;
	beep_eventreset();
	SNDCSEC_LEAVE;

	soundcfg.writecount += length;
	if (soundcfg.writecount >= 100) {
		soundcfg.writecount = 0;
		soundmng_sync();
	}
}

const SINT32 *sound_pcmlock(void) {
	const SINT32 *ret;

	if (locks) {
		TRACEOUT(("sound pcm lock: already locked"));
		return (NULL);
	}
	locks++;
	ret = sndstream.buffer;
	if (ret) {
		SNDCSEC_ENTER;
		if (sndrecording.active) {
			SNDCSEC_LEAVE;
			locks--;
			return (NULL);
		}
		if (sndstream.remain > sndstream.reserve)
#if defined(SUPPORT_WAVEREC)
			if (sndstream.rec) {
				filltailsample(sndstream.remain - sndstream.reserve);
			} else
#endif
			{
				streamprepare(vaeg_sound_lookahead_frames(sndstream.remain,
				                                           sndstream.reserve));
				soundcfg.lastclock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
				beep_eventreset();
			}
	} else {
		locks--;
	}
	return (ret);
}

void sound_pcmunlock(const SINT32 *hdl) {
	int leng;

	if (hdl) {
		leng = sndstream.reserve - sndstream.remain;
		if (leng > 0) {
			CopyMemory(sndstream.buffer, sndstream.buffer + (sndstream.samples * 2),
			           leng * 2 * sizeof(SINT32));
		}
		sndstream.ptr = sndstream.buffer + (leng * 2);
		sndstream.remain = sndstream.samples + sndstream.reserve - leng;
		//		sndstream.remain += sndstream.samples;
		SNDCSEC_LEAVE;
		locks--;
	}
}

BOOL sound_get_stream_state(SOUNDSTREAMSTATE *state) {
	UINT capacity;

	if (state == NULL) {
		return (FAILURE);
	}
	ZeroMemory(state, sizeof(*state));
	if (sndstream.buffer == NULL) {
		return (FAILURE);
	}
	SNDCSEC_ENTER;
	state->rate = soundcfg.rate;
	state->buffer_samples = sndstream.samples;
	state->reserve_samples = sndstream.reserve;
	state->remain_samples = sndstream.remain;
	capacity = sndstream.samples;
	if (sndstream.reserve <= (UINT_MAX - capacity)) {
		capacity += sndstream.reserve;
		state->pending_samples = capacity - sndstream.remain;
	}
	state->generated_sample_count = sndstream.generated_sample_count;
	state->lastclock = soundcfg.lastclock;
	state->generation_epoch = sndstream.generation_epoch;
	state->pcm_locked = (locks != 0) ? TRUE : FALSE;
	state->accounting_overflow = sndstream.accounting_overflow;
	SNDCSEC_LEAVE;
	return (SUCCESS);
}

// ---- pcmmix

BOOL pcmmix_regist(PMIXDAT *dat, void *datptr, UINT datsize, UINT rate) {
	GETSND gs;
	BYTE tmp[256];
	UINT size;
	UINT r;
	SINT16 *buf;

	gs = getsnd_create(datptr, datsize);
	if (gs == NULL) {
		goto pmr_err1;
	}
	if (getsnd_setmixproc(gs, rate, 1) != SUCCESS) {
		goto pmr_err2;
	}
	size = 0;
	do {
		r = getsnd_getpcmbyleng(gs, tmp, sizeof(tmp));
		size += r;
	} while (r);
	getsnd_destroy(gs);
	if (size == 0) {
		goto pmr_err1;
	}

	buf = (SINT16 *)_MALLOC(size, "PCM DATA");
	if (buf == NULL) {
		goto pmr_err1;
	}
	gs = getsnd_create(datptr, datsize);
	if (gs == NULL) {
		goto pmr_err1;
	}
	if (getsnd_setmixproc(gs, rate, 1) != SUCCESS) {
		goto pmr_err2;
	}
	r = getsnd_getpcmbyleng(gs, buf, size);
	getsnd_destroy(gs);
	dat->sample = buf;
	dat->samples = r / 2;
	return (SUCCESS);

pmr_err2:
	getsnd_destroy(gs);

pmr_err1:
	return (FAILURE);
}

BOOL pcmmix_regfile(PMIXDAT *dat, const char *fname, UINT rate) {
	FILEH fh;
	UINT size;
	BYTE *ptr;
	BOOL r;

	r = FAILURE;
	fh = file_open_rb(fname);
	if (fh == FILEH_INVALID) {
		goto pmrf_err1;
	}
	size = file_getsize(fh);
	if (size == 0) {
		goto pmrf_err2;
	}
	ptr = (BYTE *)_MALLOC(size, fname);
	if (ptr == NULL) {
		goto pmrf_err2;
	}
	file_read(fh, ptr, size);
	file_close(fh);
	r = pcmmix_regist(dat, ptr, size, rate);
	_MFREE(ptr);
	return (r);

pmrf_err2:
	file_close(fh);

pmrf_err1:
	return (FAILURE);
}

void SOUNDCALL pcmmix_getpcm(PCMMIX hdl, SINT32 *pcm, UINT count) {
	UINT32 bitmap;
	PMIXTRK *t;
	const SINT16 *s;
	UINT srem;
	SINT32 *d;
	UINT drem;
	UINT r;
	UINT j;
	UINT flag;
	SINT32 vol;
	SINT32 samp;

	if ((hdl->hdr.playing == 0) || (count == 0)) {
		return;
	}
	t = hdl->trk;
	bitmap = 1;
	do {
		if (hdl->hdr.playing & bitmap) {
			s = t->pcm;
			srem = t->remain;
			d = pcm;
			drem = count;
			flag = t->flag;
			vol = t->volume;
			do {
				r = min(srem, drem);
				switch (flag & (PMIXFLAG_L | PMIXFLAG_R)) {
				case PMIXFLAG_L:
					for (j = 0; j < r; j++) {
						d[j * 2 + 0] += (s[j] * vol) >> 12;
					}
					break;

				case PMIXFLAG_R:
					for (j = 0; j < r; j++) {
						d[j * 2 + 1] += (s[j] * vol) >> 12;
					}
					break;

				case PMIXFLAG_L | PMIXFLAG_R:
					for (j = 0; j < r; j++) {
						samp = (s[j] * vol) >> 12;
						d[j * 2 + 0] += samp;
						d[j * 2 + 1] += samp;
					}
					break;
				}
				s += r;
				d += r * 2;
				srem -= r;
				if (srem == 0) {
					if (flag & PMIXFLAG_LOOP) {
						s = t->data.sample;
						srem = t->data.samples;
					} else {
						hdl->hdr.playing &= ~bitmap;
						break;
					}
				}
				drem -= r;
			} while (drem);
			t->pcm = s;
			t->remain = srem;
		}
		t++;
		bitmap <<= 1;
	} while (bitmap < hdl->hdr.enable);
}
