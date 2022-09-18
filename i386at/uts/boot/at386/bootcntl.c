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

#ident	"@(#)uts-x86at:boot/at386/bootcntl.c	1.1"
#ident	"$Header: $"


#include "util/types.h"
#include "svc/bootinfo.h"
#include "svc/bootcntl.h"
#include "boot/bootdef.h"


struct	bootcntl	bootcntl = {
	BPRG_MAGIC,		/* boot program ID			*/
	BF_BIOSRAM,		/* Init value for bootflags in bootinfo */
				/* add BF_MFLOP_BOOT for multiflpy boot */
	30,			/* 30 sec delay for kernel path 	*/
#ifdef BOOT_DEBUG
	LOADDBG|BOOTDBG|BOOTTALK|MEMDEBUG,		/* debug flags 	*/
#else
	0,			
#endif
	1,			/* Autoboot = TRUE 			*/
	3,			/* memory range count			*/
	{			/* Begin default memrng definitions 	*/
	0, 640 * 1024, B_MEM_BASE,		/* 640K 0-640K		*/
	0x100000, 15*1024*1024, B_MEM_EXPANS,	/* 15M  1M-16M		*/
	0x1000000, 0x3000000, B_MEM_EXPANS,	/* 48M  16M-64M		*/
	0,0,0,
	0,0,0
	},
	"/unix",
	"/etc/initprog/sip",
	"/etc/initprog/mip",
	"Booting the UNIX System...",
	"Enter the name of a kernel to boot:",
#ifdef WINI
	0,
	"",
	"",
	"",
	""	
#else
	3,
	"rootdev=ramd(0,0)",
	"swapdev=ramd(1,0)",
	"pipedev=ramd(0,0)",
	""	
#endif
};

