
#ifndef SOUNDCALL
#define SOUNDCALL
#endif

typedef void(SOUNDCALL *SOUNDCB)(void *hdl, SINT32 *pcm, UINT count);

typedef struct {
	UINT rate;
	UINT32 hzbase;
	UINT32 clockbase;
	UINT32 minclock;
	UINT32 lastclock;
	UINT writecount;
} SOUNDCFG;

typedef struct {
	UINT rate;
	UINT buffer_samples;
	UINT reserve_samples;
	UINT remain_samples;
	UINT pending_samples;
	UINT64 generated_sample_count;
	UINT32 lastclock;
	UINT32 generation_epoch;
	BOOL pcm_locked;
	BOOL accounting_overflow;
} SOUNDSTREAMSTATE;

typedef enum {
	SOUND_RECORDING_ERROR_OK = 0,
	SOUND_RECORDING_ERROR_NOT_READY,
	SOUND_RECORDING_ERROR_BUSY,
	SOUND_RECORDING_ERROR_INVALID_CAPACITY,
	SOUND_RECORDING_ERROR_ALLOCATION,
	SOUND_RECORDING_ERROR_CAPACITY,
	SOUND_RECORDING_ERROR_OVERFLOW,
	SOUND_RECORDING_ERROR_EPOCH
} SOUND_RECORDING_ERROR;

typedef struct {
	UINT rate;
	UINT capacity_frames;
	UINT pending_frames;
	UINT64 first_sample;
	UINT64 next_sample;
	UINT64 produced_frames;
	UINT64 consumed_frames;
	UINT64 playback_dropped_frames;
	UINT64 playback_underrun_frames;
	UINT32 epoch;
	SOUND_RECORDING_ERROR error;
	BOOL active;
	BOOL failed;
} SOUNDRECORDINGSTATE;

#ifdef __cplusplus
extern "C" {
#endif

extern SOUNDCFG soundcfg;

BOOL sound_create(UINT rate, UINT ms);
void sound_destroy(void);

void sound_reset(void);
void sound_changeclock(void);
void sound_streamregist(void *hdl, SOUNDCB cbfn);

void sound_sync(void);

const SINT32 *sound_pcmlock(void);
void sound_pcmunlock(const SINT32 *hdl);
BOOL sound_get_stream_state(SOUNDSTREAMSTATE *state);
SOUND_RECORDING_ERROR sound_recording_start(UINT capacity_frames);
void sound_recording_stop(void);
void sound_recording_release(void);
BOOL sound_recording_is_active(void);
UINT sound_recording_read(SINT16 *destination, UINT capacity_frames,
	                      UINT64 *sample_begin, UINT32 *epoch);
BOOL sound_recording_get_state(SOUNDRECORDINGSTATE *state);
UINT sound_playback_read_s16(SINT16 *destination, UINT destination_bytes,
	                         UINT requested_frames);

#if defined(SUPPORT_WAVEREC)
BOOL sound_recstart(const char *filename);
void sound_recstop(void);
#endif

#ifdef __cplusplus
}
#endif

// ---- PCM MIX

enum {
	PMIXFLAG_L = 0x0001,
	PMIXFLAG_R = 0x0002,
	PMIXFLAG_LOOP = 0x0004
};

typedef struct {
	UINT32 playing;
	UINT32 enable;
} PMIXHDR;

typedef struct {
	SINT16 *sample;
	UINT samples;
} PMIXDAT;

typedef struct {
	const SINT16 *pcm;
	UINT remain;
	PMIXDAT data;
	UINT flag;
	SINT32 volume;
} PMIXTRK;

typedef struct {
	PMIXHDR hdr;
	PMIXTRK trk[1];
} _PCMMIX, *PCMMIX;

#ifdef __cplusplus
extern "C" {
#endif

BOOL pcmmix_regist(PMIXDAT *dat, void *datptr, UINT datsize, UINT rate);
BOOL pcmmix_regfile(PMIXDAT *dat, const char *fname, UINT rate);

void SOUNDCALL pcmmix_getpcm(PCMMIX hdl, SINT32 *pcm, UINT count);

#ifdef __cplusplus
}
#endif
