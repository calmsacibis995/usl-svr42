/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)uts-x86at:boot/at386/mip/compaq.c	1.1"
#ident	"$Header: $"

#include "util/types.h"
#include "io/cram/cram.h"
#include "svc/bootinfo.h"
#include "boot/boot.h"
#include "boot/initprog.h"
#include "boot/libfm.h"
#include "boot/mip.h"

int	compaq_end();

#define RAM_CNTL_REG	0x80c00000
#define CPQ_PFC()	*(unsigned char *) 0xFFFE4

compaq(lpcbp)
struct	lpcb *lpcbp;
{
	unchar	x;
	struct	int_pb	ic;
	int 	i;
	char	*cpq_romp;

#ifdef BOOT_DEBUG
	if ( btep->db_flag & BOOTTALK )
		printf("Begin Compaq initalization.\n");
#endif
	btep->sysenvmt.m_model = CPQ_PFC();
	btep->sysenvmt.machflags |= CPQ_CACHE;

	a20();		/* set A20 to address above 1MB 		*/

/* 	establish potential memory availability 			*/
	i=0;
	btep->memrng[i].base = 0;
	btep->memrng[i].extent = 640 * 1024;	/* base 640K 		*/
	btep->memrng[i++].flags = B_MEM_BASE;

	btep->memrng[i].base = 0x100000;	/* extension memory 1MB */
	btep->memrng[i].extent = 0xEA0000;	/* extent= 14M + 640K	*/
	btep->memrng[i++].flags = B_MEM_EXPANS;

	btep->memrng[i].base = 0x1000000;
	btep->memrng[i].extent = 384 * 1024;	/* extra 384K 		*/
	btep->memrng[i++].flags = B_MEM_SHADOW | B_MEM_NODMA | B_MEM_TREV;
	btep->sysenvmt.machflags |= MEM_SHADOW;

	if (!memwrap(MEM16M,MEM16M)) {
		btep->memrng[i].base = MEM16M;
		btep->memrng[i].extent = 48 * 1024 * 1024;/* remaining 48M*/
		btep->memrng[i++].flags = B_MEM_EXPANS;
		BTEP_INFO.bootflags |= BF_16MWRAPSET;
	}
	btep->memrngcnt = i;

	btep->sysenvmt.equip = CMOSread(EB);
	BTEP_INFO.bootflags |= BF_ME_SET;

	ic.intval = 0x10;
	ic.ax	  = 0xBF03;
	if ( doint(&ic) == 0 ) {
		btep->sysenvmt.md.compaq.int_mon = ((ic.dx >> 8) & 0xFF);
		btep->sysenvmt.md.compaq.ext_mon = ic.dx & 0xFF;
		btep->sysenvmt.md.compaq.modes = ic.cx & 0xFF;
		btep->sysenvmt.md.compaq.act_disp = ((ic.bx >> 8) & 0xFF);
		btep->sysenvmt.md.compaq.ctl_mode = ic.bx & 0xFF;
	}
	else {
		x = CMOSread(0x2D);
		if ( x & 1 )
			btep->sysenvmt.int_mon = MONT_CPQDM;
	}

	/* look to see if we have a video seven board in the system */
	/* do int 10 call with 6f00 in ax returns 'V7' in bx if true */

	ic.intval = 0x10;
	ic.ax	  = 0x6F00;
	ic.bx	  = 0;
	if ( doint(&ic) == 0 ) {
		if(ic.bx == 0x5637)  /* found 'V7' in bx reginster 	*/
			btep->sysenvmt.adapter  = MADT_V7_E;
	}

/*	set handling routine for command MIP_END			*/
	MIP_end = (ulong) compaq_end;

#ifdef BOOT_DEBUG
	if ( btep->db_flag & BOOTTALK )
		printf("Complete Compaq initalization compaq_end= 0x%x.\n", MIP_end);
#endif

}

compaq_end()
{
#ifdef BOOT_DEBUG
	if ( btep->db_flag & BOOTTALK )
		printf("Compaq end procedure.\n");
#endif

	*(char *) RAM_CNTL_REG = 0xFF;	/* turn off shadow ram */

	if ((btep->sysenvmt.equip & 0x30) == 0)

		/* EGA VGA vector changes required */

		fix_ega_vect(EGA_SEGMENT);

/* 	Fix user int 6D call  - for Video 7 adapter			*/
	if (btep->sysenvmt.adapter  == MADT_V7_E )
		*(ushort *)(0x6d * 4 + 2) = EGA_SEGMENT;
}


