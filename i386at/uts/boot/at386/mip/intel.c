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

#ident	"@(#)uts-x86at:boot/at386/mip/intel.c	1.1"
#ident	"$Header: $"

#include "util/types.h"
#include "io/cram/cram.h"
#include "svc/bootinfo.h"
#include "boot/boot.h"
#include "boot/initprog.h"
#include "boot/libfm.h"
#include "boot/mip.h"

#define	INTL_SETREG	0x28	/* CMOS setup register */
#define	INTL_CACHEOFF	0x80	/* CMOS setup reg chache disabled flag */
#define	INTL_16MBLIM	0x40	/* CMOS setup reg above 16MB disabled flag */
#define	INTL_BIOSSHAD	0x01	/* CMOS setup reg BIOS shadow disabled */
#define	INTL_VBIOSSHAD	0x02	/* CMOS setup reg video BIOS shadow disabled */

intel(lpcbp)
struct	lpcb *lpcbp;
{
	int	i;

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK)
		printf("Begin Intel 386 initalization.\n");
#endif

	a20();		/* set A20 to address above 1MB */

	i=0;
	btep->memrng[i].base = 0;
	btep->memrng[i].extent = 640 * 1024;
	btep->memrng[i++].flags = B_MEM_BASE;
	btep->memrng[i].base = 0x100000;
	btep->memrng[i].extent = 0xF00000;
	btep->memrng[i++].flags = B_MEM_EXPANS;

	btep->sysenvmt.md.intel.set_reg = CMOSread(INTL_SETREG);
	if (!(btep->sysenvmt.md.intel.set_reg & INTL_16MBLIM)) {
		btep->memrng[i].base = MEM16M;
		btep->memrng[i].extent = MEM16M;
		btep->memrng[i++].flags = B_MEM_EXPANS;
	}
	btep->memrngcnt = i;

	if (btep->sysenvmt.machine == MPC_INTEL30X) {
		btep->sysenvmt.m_model = *(unchar *)0xFED04;
		btep->sysenvmt.m_revno = *(unchar *)0xFED05;
	}

/*	No final startup procedure support			*/
	MIP_end = (ulong) NULL;
}

