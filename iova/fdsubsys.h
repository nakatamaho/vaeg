/*
 * fdsubsys.h: PC-88VA FD Sub System
 */


#ifdef __cplusplus
extern "C" {
#endif

void fdsubsys_reset(void);
void fdsubsys_bind(void);
BOOL fdsubsys_selftest(void);

#ifdef __cplusplus
}
#endif
