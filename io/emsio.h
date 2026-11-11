#ifndef VAEG_IO_EMSIO_H
#define VAEG_IO_EMSIO_H

enum {
	EMSIO_DEFAULT_MEGABYTES = 0,
	EMSIO_MIN_MEGABYTES = 1,
	/* M146: the 4-bit target register (08E9h) selects 1MB targets 1-15;
	 * 13 was NP2's ExMemory limit, not a PC-88VA board limit. */
	EMSIO_MAX_MEGABYTES = 15
};

typedef struct {
	UINT8 maxmem;
	UINT8 target;
	UINT16 padding;
	UINT32 addr[4];
} _EMSIO, *EMSIO;

#ifdef __cplusplus
extern "C" {
#endif

extern _EMSIO emsio;

void emsio_reset(void);
void emsio_bind(void);

#ifdef __cplusplus
}
#endif

#endif
