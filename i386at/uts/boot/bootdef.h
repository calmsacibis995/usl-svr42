/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOT_BOOTDEF_H	/* wrapper symbol for kernel use */
#define _BOOT_BOOTDEF_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:boot/bootdef.h	1.5"
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

/*	Definitions for key BIOS ram loactions */

#define FDB_ADDR 	0x78	/* Floppy base parameter table 		*/
#define HD0V_ADDR 	0x104	/* hard disk drive 0 vector address	*/
#define HD1V_ADDR 	0x118	/* hard disk drive 1 vector address	*/
#define NUMHD_ADDR	0x475	/* number of HD drives 			*/
#define MEMBASE_ADDR	0x413	/* Base memory size 			*/
#define COMM_ADDR	0x400 	/* base address for communication port	*/
#define LPT_ADDR	0x408 	/* base address for lpt port		*/

#define RESERVED_SIZE	0x03000 /* size of the initial page tables 	*/

#define BOOTDRV_MASK	0x80	/* mask for bootdriv 			*/
#define BOOTDRV_HD	0x80	/* bootdriv - harddisk 			*/
#define BOOTDRV_FP	0x00	/* bootdriv - floppy diskette		*/

#define SECBOOT_ADDR	0x2e00	/* memory location for secondary boot	*/
#define	PRIBOOT_ADDR	0x7c00	/* memory location for primary boot 	*/
#define	SECBOOT_STACKSIZ 1024	/* secondary boot stack size		*/
#define PROG_GAP	1024	/* reserved 1K between loadable module	*/

/*	fdisk information						*/
/*	B_BOOTSZ, B_ACTIVE and B_FD_NUMPART are from io/hd/fdisk.h	*/
#define	B_BOOTSZ	446	/* size of master boot in hard disk	*/
#define B_FD_NUMPART	4	/* number of fdisk partitions		*/
#define	B_ACTIVE	0x80	/* active partition			*/

/*	disk buffer definition						*/
#define GBUFSZ		(9*1024)/* global disk buffer size		*/

/*	stack index of input parameters from priboot boot		*/ 
#define STK_SBML	1	/* secboot_mem_loc			*/
#define	STK_SPC		2	/* spc					*/
#define	STK_SPT		3	/* spt					*/
#define	STK_BPS		4	/* bps					*/
#define	STK_EDS		5	/* entry ds:si location			*/
#define	STK_AP		6	/* active partition pointer		*/	

/*	ELF file header	identification					*/
#define ELFMAGIC 0x457f

/*	routine return status code					*/
#define SUCCESS	0
#define FAILURE	1

/*	video segment address						*/
#define EGA_SEGMENT	(unsigned short) 0xC000

/*	control register CR0 bit definition				*/
#define	CR0_PG		0x80000000
#define CR0_EM		0x4
#define PROTMASK	0x1
#define	NOPROTMASK	0x7ffffffe

/*	GDT definitions							*/
#define	B_GDT		0x08	/* big flat data descriptor		*/
#define C_GDT		0x10	/* flat code descriptor			*/
#define C16GDT		0x18	/* use 16 code descriptor		*/
#define D_GDT		0x20	/* flat data descriptor			*/

/*	Kernel paging data						*/
#define	KPD_LOC	0x2000

/*	BKI magic - from bootinfo.h					*/
#define	B_BKI_MAGIC	0xff1234ff
/*	selector definition - from seg.h				*/
#define KTSSSEL		0x150	/* TSS for the scheduler		*/
#define JTSSSEL		0x170

/*	disk read ecc code						*/
#define ECC_COR_ERR     0x11    /* ECC corrected disk error 		*/
#define RD_RETRY	0x2	/* retry count				*/
#define FD_ADAPT_LEV	0x2	/* fd read adaptive level		*/

/*	memory test pattern						*/
#define MEMTEST0        (ulong)0x00000000
#define MEMTEST1        (ulong)0xA5A5A5A5
#define MEMTEST2        (ulong)0x5A5A5A5A
#define MEM16M          (ulong)0x1000000

#endif	/* _BOOT_BOOTDEF_H */
