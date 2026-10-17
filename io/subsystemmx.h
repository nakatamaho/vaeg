/*
 * subsystemmx.h: PC-88VA FD Sub System (multiplexer)
 *
 */

#if !defined(_SUBSYSTEMMX_H_)
#define _SUBSYSTEMMX_H_

typedef struct {
	BOOL mockup;
	BOOL absent; /* M113: no FDD unit (ports FCh-FFh float, unit stopped) */
} _SUBSYSTEMMXCFG, *SUBSYSTEMMXCFG;

#ifdef __cplusplus
extern "C" {
#endif

extern _SUBSYSTEMMXCFG subsystemmxcfg;

void subsystemmx_initialize(void);
void subsystemmx_reset(void);
void subsystemmx_bind(void);
void subsystemmx_exec(void);
void subsystemmx_set_absent(BOOL absent);

#ifdef __cplusplus
}
#endif

#endif /* _SUBSYSTEMMX_H_ */