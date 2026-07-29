#include	"compiler.h"
#include	"cpucore.h"
#include	"vram.h"


	VRAM_T	vramop;
	BYTE	tramupdate[0x1000];
	BYTE	vramupdate[0x8000];


void vram_initialize(void) {

	ZeroMemory(&vramop, sizeof(vramop));
	upd9002_vram_dispatch(0);
}

