/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DDI_I386AT_H
#define _IO_DDI_I386AT_H

#ident	"@(#)uts-x86at:io/ddi_i386at.h	1.2"
#ident	"$Header: $"


extern int drv_gethardware();

/* parameter values for drv_gethardware() */
#define	PROC_INFO	0
#define	IOBUS_TYPE	1
#define	TOTAL_MEM	2
#define	DMA_SIZE	3
#define	BOOT_DEV	4
#define	HD_PARMS	5

/* structure argument for PROC_INFO */

struct cpuparms {
	ulong_t cpu_id;	       /* CPU identification */
	ulong_t cpu_step;     /* Step (revision) number */
	ulong_t cpu_resrvd[2];  /* RESERVED for future expansion */
};

/* Legal values for cpu_id */

#define	CPU_UNK		0
#define	CPU_i386	1
#define	CPU_i486	2
#define	CPU_i586	3
#define	CPU_i686	4
#define	CPU_i786	5


/* Legal values for cpu_step */

#define STEP_UNK	0
#define STEP_B1		1

/* Return values for IOBUS_TYPE */

#define	BUS_ISA		0
#define	BUS_EISA	1
#define	BUS_MCA		2

/* Structure used with BOOT_DEV */

struct bootdev {
	ulong_t	bdv_type;	/* Type of the boot device */
	ulong_t	bdv_unit;	/* Unit number of the boot device */
	ulong_t	bdv_resrvd[2];	/* RESERVED for future expansion */
};


/* Legal values for bdv_type */

#define BOOT_FLOPPY	1
#define BOOT_DISK	2
#define BOOT_CDROM	3
#define BOOT_NETWORK	4

/* Structure used with HD_PARMS */

struct hdparms {
	ulong_t	hp_unit;	/* Hard disk unit number */
	ulong_t	hp_ncyls;	/* # of cylinders (0 == no disk) */
	ulong_t	hp_nheads;	/* # of heads */
	ulong_t	hp_nsects;	/* # of sectors per track */
	ulong_t	hp_resrvd[3];	/* RESERVED for future expansion */
};

#endif	/* _IO_DDI_I386AT_H */
