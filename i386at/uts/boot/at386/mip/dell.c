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

#ident	"@(#)uts-x86at:boot/at386/mip/dell.c	1.1"
#ident	"$Header: $"

/*
 *	Initalization program for dell 80386 machines.
 */

#include "util/types.h"
#include "svc/bootinfo.h"
#include "boot/boot.h"
#include "boot/initprog.h"
#include "boot/libfm.h"
#include "boot/mip.h"

#define	MCP_test	(char *)0x412
#define IDENT325	(char *)0xfe082
#define WEITEK_PRES	0x10		/* present mask */

char	id325[] = "325";

dell(lpcbp)
struct	lpcb *lpcbp;
{

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK)
		printf("Begin Dell initalization.\n");
#endif

	btep->sysenvmt.machflags |= CPQ_CACHE;

	/* Check for a model 325 for Weitek workaround */

	if (memcmp(IDENT325,id325,3) == 0) {

#ifdef BOOT_DEBUG
		if (btep->db_flag & BOOTTALK)
			printf("Have a Dell model 325\n");
#endif
		btep->sysenvmt.machine = MPC_D325; 
		btep->sysenvmt.c_speed = 25;
		if ( * MCP_test & WEITEK_PRES )
			BTEP_INFO.bootflags |= BF_WTNOTST;
	}
	a20();		/* set A20 to address above 1MB */

/*	No final startup procedure support			*/
	MIP_end = (ulong) NULL;
}

