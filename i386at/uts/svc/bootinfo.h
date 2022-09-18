/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _SVC_BOOTINFO_H	/* wrapper symbol for kernel use */
#define _SVC_BOOTINFO_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:svc/bootinfo.h	1.13"
#ident	"$Header: $"

/*
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1987,1988 INTERACTIVE Systems Corporation
 *	All rights reserved.
 *
 *	RESTRICTED RIGHTS
 *
 *	These programs are supplied under a license.  They may be used,
 *	disclosed, and/or copied only as permitted under such license
 *	agreement.  Any copy must contain the above copyright notice and
 *	this restricted rights notice.  Use, copying, and/or disclosure
 *	of the programs is strictly prohibited unless otherwise provided
 *	in the license agreement.
 */
#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _SVC_SYSENVMT_H
#include <svc/sysenvmt.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/sysenvmt.h>

#else

#include <sys/sysenvmt.h>

#endif /* _KERNEL_HEADERS */

/*
 *	Definition of bootinfo structure.  This is used to pass
 *	information between the bootstrap and the kernel.
 */

#define BKI_MAGIC	0xff1234ff

#define B_MAXARGS	15		/* max. number of boot args */
#define B_STRSIZ	50		/* max length of boot arg. string */
#define B_PATHSIZ     	30              /* max length of file path string */
#define B_MAXMEMR    	5               /* max memrng entry count */

struct bootmem {
	paddr_t		base;
	long		extent;
	ushort		flags;
	};

struct bootinfo {
	ulong	bootflags;		/* miscellaneous flags */

	struct hdparams { 		/* hard disk parameters */
		ushort	hdp_ncyl;	/* # cylinders (0 = no disk) */
		unchar	hdp_nhead;	/* # heads */
		unchar	hdp_nsect;	/* # sectors per track */
		ushort	hdp_precomp;	/* write precomp cyl */
		ushort	hdp_lz;		/* landing zone */
	} hdparams[2];			/* hard disk parameters */

	int	memavailcnt;
	struct	bootmem	memavail[B_MAXARGS];

	int	memusedcnt;
	struct	bootmem	memused[B_MAXARGS];

	int	bargc;			    /* count of boot arguments */
	char	bargv[B_MAXARGS][B_STRSIZ]; /* argument strings */

	char	id[5];			/* Contents of F000:E000 */
	int	checksum;
};

struct	bootenv {
	struct	bootinfo bootinfo;	/* boot information		*/
	unchar	bf_minor;		/* boot floppy minor #		*/
	unchar	bf_pad1;		/* unused			*/
	ushort	db_flag;		/* debug control flags		*/
	int	memrngcnt;		/* default memory sections	*/
	struct	bootmem memrng[B_MAXMEMR];
	paddr_t sectaddr[B_MAXARGS];	/* memused load address		*/
	long	bootsize;		/* top memory loc. of boot prog */
	int	timeout;		/* timeout value		*/
	char	initprog[B_PATHSIZ];	/* machine dependent init prog	*/
	char	bootmsg[B_STRSIZ];	/* boot greeting message	*/
	struct	sysenvmt sysenvmt;	/* system environment data	*/
	paddr_t bf_resv_base;		/* base of memory reserved boot */
	long	bf_resv[10];		/* reserved fields		*/
};
#define BTE_INFO bootenv.bootinfo

/* flags for struct mem flags */

#define B_MEM_NODMA	0x01
#define B_MEM_KTEXT	0x02
#define	B_MEM_KDATA	0x04
#define B_MEM_BASE	0x100
#define B_MEM_EXPANS	0x200
#define B_MEM_SHADOW	0x400
#define B_MEM_TREV	0x1000	/* Test Memory in high to low order	*/
#define B_MEM_FORCE	0x2000	/* Ignore CMOS and continue probing memory */
#define B_MEM_BOOTSTRAP	0x8000	/* Used internally by bootstrap */

/* Flag definitions for bootflags */

#define BF_FLOPPY	0x01		/* booted from floppy 		*/
#define BF_FPNOTST      0x02            /* 80?87 math chip no test|done */
#define BF_WTNOTST      0x04            /* Weitek math chip no tst|done */
#define BF_A20SET       0x08            /* A20 line set complete 	*/
#define BF_NOCONS       0x10            /* Host has no console  	*/
#define BF_ME_SET	0x20		/* Machine equip byte set 	*/
#define BF_EGAVSET	0x40		/* EGA?VGA determination made 	*/
#define BF_MEMAVAILSET	0x80		/* Mem available array is set 	*/
#define	BF_BIOSRAM	0x100		/* restore ram used by BIOS	*/
#define	BF_IGNCMOSRAM	0x200		/* ignore CMOS xtended mem val	*/
#define BF_16MWRAPSET	0x400		/* above 16M ram wrap test done	*/
#define BF_NOTSTSCSI	0x20000		/* Donot probe for SCSI cntrl 	*/
#define BF_NOTSTSCSIAD	0x40000		/* Donot probe for Adaptec SCSI */
#define BF_NOTSTSCSIFD	0x80000		/* Donot probe for Future Domain SCSI */
#define BF_MFLOP_BOOT	0x8000000	/* Multiple Floppy boot mode 	*/
#define BF_MB2SA        0x20000000      /* Kernel booted from MSA 	*/
#define BF_TAPE         0x40000000      /* Kernel booted from tape 	*/
#define BF_DEBUG        0x80000000      /* Bootloader debug flg set-usr */

/* Flag definitions for db_flag */

#define	BOOTDBG		0x01		/* F2 basic boot debug messages */
#define	MEMDEBUG	0x02		/* F3 memory debug messages 	*/
#define	LOADDBG		0x04		/* F4 loader debug messages 	*/
#define	ENVDBG		0x08		/* F5 environment debug messages */
#define	BOOTTALK	0x10		/* F10 make boot process verbose */

#define BOOTINFO_LOC	((paddr_t)0x600)
#define KPTBL_LOC	((paddr_t)0x1000)  /* Reserved for kernel page table */

extern struct bootinfo bootinfo;

#endif	/* _SVC_BOOTINFO_H */
