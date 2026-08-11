/*
 * iocoreva.h: PC-88VA I/O
 */

#include	"memctrlva.h"
#include	"tsp.h"
#include	"videova.h"
#include	"sysportva.h"
#include	"mouseifva.h"
#include	"gactrlva.h"
#include	"cgromva.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL iocore_attachvaout(UINT port, IOOUT func);
BOOL iocore_attachvainp(UINT port, IOINP func);







#ifdef __cplusplus
}
#endif
