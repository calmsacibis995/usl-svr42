/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOT_BOOT_H	/* wrapper symbol for kernel use */
#define _BOOT_BOOT_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:boot/boot.h	1.4"
#ident  "$Header: $"

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

#ifndef _BOOT_BOOTDEF_H
#include <boot/bootdef.h>
#endif	/* _BOOT_BOOTDEF_H */

/* definitions for generic AT(386) hard/floppy disk boot code */

#ifdef BOOTDEBUG
#define debug(x)	x
#else
#define debug(x)	/*x/**/
#endif

#define TRUE	1
#define FALSE	0

#define NULL	0

#define HINBL	(unchar)0xF0

/* Definitions for micro-channel HD type, Should be in cram.h */

#define MC_FD0TB	0x11	/* Drive 0 type */
#define MC_FD1TB	0x12	/* Drive 1 type */

/*	Definitions for key BIOS ram loactions */

#define FDBvect	*(ulong *)FDB_ADDR	/* Floppy base parameter table ptr */
#define HD0p	(faddr_t *)HD0V_ADDR	/* hard disk 0 parameter table ptr */
#define HD1p	(faddr_t *)HD1V_ADDR	/* hard disk 1 parameter table pte */
#define NUMHD()	*(short *)NUMHD_ADDR	/* number of HD drives 		   */
#define MEM_BASE() *(short *)MEMBASE_ADDR	/* Base memory size 	   */
#define COMM_B(x)  *(short *)(COMM_ADDR + (x-1))/* base addr for com port  */
#define LPT_B(x)   *(short *)(LPT_ADDR + (x-1)) /* base addr for lpt port  */

#define segoftop(s,o)	(paddr_t)( (uint)(s<<4) + o)

/*	To read a word(short) from CMOS */

#define CMOSreadwd(x)	(ushort) ((CMOSread(x+1) << 8)+CMOSread(x))

#pragma	pack(1)
struct	hdpt	{		/* hard disk parameter table entry */
	unsigned short	mxcyl;
	unsigned char	mxhead;
	unsigned char	dmy1[2];
	unsigned short	precomp;
	unsigned char	mxecc;
	unsigned char	drvcntl;
	unsigned char	dmy2[3];
	unsigned short	lz;
	unsigned char	spt;
	unsigned char	dmy3;
};

struct	fdpt	{		/* floppy disk parameter table entry */
	unsigned char	step;
	unsigned char	load;
	unsigned char	motor;
	unsigned char	secsiz;
	unsigned char	spt;
	unsigned char	dgap;
	unsigned char	dtl;
	unsigned char	fgap;
	unsigned char	fill;
	unsigned char	headsetl;
	unsigned char	motrsetl;
	unsigned char	mxtrk;
	unsigned char	dtr;
};
#pragma pack()

#define physaddr(x)	(paddr_t)(x)

#endif	/* _BOOT_BOOT_H */
