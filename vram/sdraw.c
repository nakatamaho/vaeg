#include	"compiler.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"sdraw.h"
#include	"palettes.h"

#if !defined(SIZE_QVGA) || defined(SIZE_VGATEST)


#define	SDSYM(sym)				sdraw16##sym
#define	SDSETPIXEL(ptr, pal)	*(UINT16 *)(ptr) = np2_pal16[(pal)]
#include	"sdraw.mcr"
#undef	SDSYM
#undef	SDSETPIXEL




const SDRAWFN *sdraw_getproctbl(const SCRNSURF *surf) {

	(void)surf;
	return(sdraw16p);
}


#endif
