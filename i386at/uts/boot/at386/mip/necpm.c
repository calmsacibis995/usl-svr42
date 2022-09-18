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

#ident	"@(#)uts-x86at:boot/at386/mip/necpm.c	1.1"
#ident	"$Header: $"

#include "util/types.h"
#include "svc/bootinfo.h"
#include "boot/boot.h"
#include "boot/initprog.h"
#include "boot/libfm.h"
#include "boot/mip.h"

necpm(lpcbp)
struct	lpcb *lpcbp;
{
#ifdef BOOT_DEBUG
	if ( btep->db_flag & BOOTTALK )
		printf("Begin NEC Powermate initalization.\n");
#endif

	/* establish potential memory availability */

	btep->memrng[0].base = 0;
	btep->memrng[0].extent = 640 * 1024;	/* base 640K 		   */
	btep->memrng[0].flags = B_MEM_BASE;

	btep->memrng[1].base = 0x100000;	/* expansion memory at 1MB */
	btep->memrng[1].extent = 0xEC0000;	/* up to 14.75Mbyte	   */
	btep->memrng[1].flags = B_MEM_EXPANS;

	btep->memrngcnt = 2;

	a20();		/* set A20 to address above 1MB */

/*	No final startup procedure support			*/
	MIP_end = (ulong) NULL;
}

