#ident	"@(#)uts-x86at:svc/oem.c	1.10"
#ident	"$Header: $"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */


#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <svc/uadmin.h>
#include <svc/pit.h>
#include <svc/systm.h>
#include <svc/bootinfo.h>
#include <mem/immu.h>
#include <mem/vmparam.h>
#include <fs/buf.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <io/conf.h>
#include <io/cram/cram.h>

#include <mem/rdma.h>	/* for RESTRICTED_DMA support */

#define	IO_ROM_INIT	0xfff0
#define	IO_ROM_SEG	0xf000
#define	RESET_FLAG	0x1234
#define	SHUT_DOWN	0x8f
#define	SHUT5		5
#define	STAT_8042	0x64

extern int eisa_bus, sanity_clk;


/*
 * general hook for things which need to run prior to turning
 * on interrupts.
 */
oem_pre_clk_init()
{
		oem_fclex();    /* Assure NDP busy latch is clear */
}

/*
 * oeminit() - oem specific initialization.
 *
 * This routine is called after the system initialization
 * functions have been called (io_init[]) and after interrupts
 * have been enabled.  This routine is called from init_tbl[].
 */
oeminit()
{
}

/*
 * oem_doarg(s) - oem specific boot argument parsing
 *
 * This routine is called after first checking an argument
 * against the standard set of supported arguments.  Unknown
 * (non-standard) arguments will be passed to this routine.
 *
 * Return Values:	 0 if argument handled
 *			-1 if argument unknown
 */	 
oem_doarg(s)
char *s;
{
	return(-1);		/* unknown argument */
}

/*
 * Machine dependent idle routine
 */
void
oemidle()
{
}

/*
 * Machine dependent code to reboot.
 */

/* ARGSUSED */
void
mdboot(fcn, mdep)
	int fcn;
	int mdep;
{
	switch (fcn) {
	default:
		cmn_err(CE_CONT, "sysi86, mdboot, wrong arg, assuming AD_IBOOT\n");
		/* FALL THROUGH */
	case AD_HALT:
	case AD_IBOOT:
		splhi();
		oemreboot(0);	/* no return expected */
		/* NOTREACHED */
		break;
	case AD_BOOT:
		splhi();
		oemreboot(1);	/* no return expected */
		/* NOTREACHED */
		break;
	}
}

/*
 * Configurable panic (via uadmin())
 * allows the system to stay down or reboot.
 *
 *  zero	-> no auto reboot
 *  non-zero	-> automatic reboot
 */
STATIC int bootpanic = 0;

int
ConfigurePanic(mdep)
int mdep;
{
	bootpanic = mdep;
	return(0);
}

void
oemsysdump()
{
	static buf_t	bp;
	uint	i, j, sz;
	caddr_t	vaddr;
	register unsigned int maj;
	extern int physmem;

	printf("Trying to dump %d Pages\n", physmem);
	spl0();

	bp.b_edev = dumpdev;
	bp.b_dev = cmpdev(dumpdev);

	maj = getmajor(dumpdev);

	/* Since memory may be in more than one piece, use memsegs */
	i = j = 0;
	while (i < bootinfo.memavailcnt && 
			(sz = bootinfo.memavail[i].extent) != 0) {
		vaddr = (caddr_t)xphystokv(bootinfo.memavail[i].base);
		while (sz > 0 & !(bp.b_flags & B_ERROR)) {
			bp.b_blkno = ptod(j++);
			bp.b_un.b_addr = vaddr;
			sz -= (bp.b_bcount = NBPP);
			bp.b_flags = (B_KERNBUF | B_BUSY | B_WRITE);
/* RESTRICTED_DMA Support */
			if (rdma_enabled) {
				ASSERT(bdevsw[maj].d_flag);
				if (*bdevsw[maj].d_flag & D_DMA)
					rdma_physio(bdevsw[maj].d_strategy,
						    &bp, B_WRITE);
				else
					(*bdevsw[maj].d_strategy)(&bp);
			} else
/* End RESTRICTED_DMA Support */
				(*bdevsw[maj].d_strategy)(&bp);
			while ((bp.b_flags & B_DONE) == 0) ;
			vaddr += NBPP;
			if (!(j&0xf)) printf(".");
		}
		i++;
	}
	printf("\n%d Pages dumped\n\n", j);

	oemreboot(bootpanic);
	rtnfirm();	/* oemreboot() should do this, but just in case... */
	/* NOTREACHED */
}

/*
 * Intended use for flag values:
 * 	flag == 0	halt the system and wait for user interaction
 * 	flag >= 1	automatic reboot, no user interaction required
 */
oemreboot(flag)
int flag;
{
	void dhalt();

	/* if sanity timer in use, turn off to allow clean soft reboot */
	if (eisa_bus && sanity_clk)
		outb(SANITY_CHECK, RESET_SANITY);
	dhalt();
	if (flag) {
		cmn_err(CE_CONT, "\nThe system is down.\n");
		cmn_err(CE_CONT, "\nAutomatic Boot Procedure\n");
	} else {
		rmc_pwroff();	/* call to the rmc driver */
		cmn_err(CE_CONT, "\nThe system is down.\n");
		cmn_err(CE_CONT, "Press CTRL-ALT-DEL to reboot your computer.\n");
		rtnfirm();
	}
	softreset();
	outb( STAT_8042, 0xfe );	/* trigger reboot */

	for ( ; ; ) {
		asm("	cli	");
		asm("	hlt	");
	}
}

softreset()
{
	int io_rom;

	/* do soft reboot; only do memory check after power-on */
	io_rom = phystokv( 0x467 );
	*(long *)io_rom = (((unsigned long)IO_ROM_SEG) << 16) |
			  (unsigned long)IO_ROM_INIT;
	io_rom = phystokv( 0x472 );
	*(short *)io_rom = RESET_FLAG;

	/* set shutdown flag to reset using int 19 */
	outb(CMOS_ADDR, SHUT_DOWN);
	outb(CMOS_DATA, SHUT5);
}
