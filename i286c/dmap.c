#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"dmap.h"
#if defined(SUPPORT_PC88VA)
#include	"memoryva.h"
#endif


void dmap_i286(void) {

	DMACH	ch;
	REG8	bit;
	REG8	dat;

	if (dmac.working) {
		ch = dmac.dmach;
		bit = 1;
		do {
			if (dmac.working & bit) {
#if defined(VAEG_FIX) || defined(VAEG_EXT)
				if (ch->proc.extproc(DMAEXT_DRQ)) {
					goto next_channel;
				}
#endif
				// DMA working !
				if (!ch->leng.w) {
					dmac.stat |= bit;
					dmac.working &= ~bit;
					ch->proc.extproc(DMAEXT_END);
				}
				ch->leng.w--;

				switch(ch->mode & 0x0c) {
					case 0x00:		// verifty
						ch->proc.inproc();
						break;

					case 0x04:		// port->mem
						dat = ch->proc.inproc();
#if defined(SUPPORT_PC88VA)
						memoryva.dma_access = 0x80;
#endif
						i286_memorywrite(ch->adrs.d, dat);
#if defined(SUPPORT_PC88VA)
						memoryva.dma_access = 0x00;
#endif
						break;

					default:
#if defined(SUPPORT_PC88VA)
						memoryva.dma_access = 0x80;
#endif
						dat = i286_memoryread(ch->adrs.d);
#if defined(SUPPORT_PC88VA)
						memoryva.dma_access = 0x00;
#endif
						ch->proc.outproc(dat);
						break;
				}
				ch->adrs.d += ((ch->mode & 0x20)?-1:1);
			}
#if defined(VAEG_FIX) || defined(VAEG_EXT)
next_channel:
#endif
			ch++;
			bit <<= 1;
		} while(bit & 0x0f);
	}
}

void dmap_v30(void) {

	DMACH	ch;
	REG8	bit;

	if (dmac.working) {
		ch = dmac.dmach;
		bit = 1;
		do {
			if (dmac.working & bit) {
				// DMA working !
				if (!ch->leng.w) {
					dmac.stat |= bit;
					dmac.working &= ~bit;
					ch->proc.extproc(DMAEXT_END);
				}
				ch->leng.w--;

				switch(ch->mode & 0x0c) {
					case 0x00:		// verifty
						ch->proc.inproc();
						break;

					case 0x04:		// port->mem
						i286_memorywrite(ch->adrs.d, ch->proc.inproc());
						break;

					default:
						ch->proc.outproc(i286_memoryread(ch->adrs.d));
						break;
				}
				ch->adrs.w[DMA16_LOW] += ((ch->mode & 0x20)?-1:1);
			}
			ch++;
			bit <<= 1;
		} while(bit & 0x0f);
	}
}
