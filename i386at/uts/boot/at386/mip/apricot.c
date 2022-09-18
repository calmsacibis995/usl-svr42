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

#ident	"@(#)uts-x86at:boot/at386/mip/apricot.c	1.1"
#ident	"$Header: $"

#include "util/types.h"
#include "svc/bootinfo.h"
#include "boot/boot.h"
#include "boot/initprog.h"
#include "boot/libfm.h"
#include "boot/mip.h"

apricot(lpcbp)
struct	lpcb *lpcbp;
{
	if (btep->sysenvmt.machflags & MC_BUS) {
#ifdef BOOT_DEBUG
		if (btep->db_flag & BOOTTALK)
			printf("Begin Apricot micro-channel initalization.\n");
#endif
		mc_a20();		/* set A20 to address above 1MB */
		BTEP_INFO.bootflags |= BF_WTNOTST;
		btep->sysenvmt.adapter  = MADT_VGA;
	} else {
#ifdef BOOT_DEBUG
		if (btep->db_flag & BOOTTALK)
			printf("Begin Apricot AT bus initalization.\n");
#endif
		a20();
	}
/*	No final startup procedure support			*/
	MIP_end = (ulong) NULL;
}


