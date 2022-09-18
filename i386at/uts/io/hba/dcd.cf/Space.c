/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/dcd.cf/Space.c	1.20"
#ident	"$Header: $"

/*
 * General boot configuration.  Will try for 1 drive on AT-compatible
 * controller first, then look for an AHA-1540 and use the first drive found
 * if it's there.  If not, tries for TMC-830 and uses its first drive.
 * If more than 1 controller is present, we'll panic with overlap in
 * minormap entry 0...
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990 INTERACTIVE Systems Corporation
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


#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/buf.h>
#include <sys/elog.h>
#include <sys/iobuf.h>
#include <sys/vtoc.h>
#include <sys/alttbl.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/dcd.h>
#include <sys/gendev.h>
#include <sys/gendisk.h>
#include <sys/athd.h>
#include <sys/mcst.h>
#include <sys/mcesdi.h>
#include <sys/ict.h>
#include <sys/errno.h>
#include "config.h"     /* In case user wants to override defaults */

/* MCST_DISKS is set to 1 during installation. If the system has 2 disks   */
/* change the 1 to 2 and rebuild the kernel using idbuild. 		   */
#define MCST_DISKS	1  

int	dcd_major = 35;		/* SCSI driver major number	*/

/* The actual configuration table */

struct  gdev_cfg_entry device_cfg_tbl[] = {

	{
	"(athd,1st) Generic ISA (MFM,RLL,ESDI)",	/* Controller Name */
	(CCAP_PIO | CCAP_RETRY | CCAP_ERRCOR),  /* capabilities */
	0x0,	/* Primary memory address */
	0x0,	/* Secondary memory address */
	0x1f0,	/* Primary I/O address */
	0x3f6,	/* Secondary I/O address */
	0,	/* Primary DMA Channel */
	0,	/* Secondary DMA Channel */
	255,	/* Max # of sector transfer count */
	2,	/* Up to 2 drives */
	10,	/* 100us drive switch delay */
	0,	/* Start at this minormap entry */
	512,	/* Default sector size */
	athd_bdinit,	/* init board function */
	athd_drvinit,	/* init drive function */
	athd_cmd,	/* command function */
	NULL,	/* no open function */
	NULL,	/* no close function */
	NULL,	/* No Master Interrupt */
		{	/* Interrupt entries */
		14, athd_int,	/* First Hardware Interrupt */
		},
		{	/* Special IOCTL handlers */
		0,	/* None present */
		},
	},
	{
	"(mcesdi,1st) Micro Channel ESDI",	/* Controller Name */
	(CCAP_DMA | CCAP_NOSEEK | CCAP_RETRY | CCAP_ERRCOR),	/* capabilities */
	0x0,	/* Primary memory address */
	0x0,	/* Secondary memory address */
	0x3510,	/* Primary I/O address */
	0x0,	/* Secondary I/O address */
	5,	/* Primary DMA Channel */
	0,	/* Secondary DMA Channel */
	255,	/* Max # of sector transfer count */
	2,	/* Up to 2 drives */
	10,	/* 100us drive switch delay */
	0,	/* Start at this minormap entry */
	512,	/* Default sector size */
	mces_bdinit,	/* init board function */
	mces_drvinit,	/* init drive function */
	mces_cmd,	/* command function */
	NULL,	/* no open function */
	NULL,	/* no close function */
	NULL,	/* No Master Interrupt */
		{	/* Interrupt entries */
		14, mces_int,	/* First Hardware Interrupt */
		},
		{	/* Special IOCTL handlers */
		0,	/* None present */
		},
	},
	{
	"(mcst,1st) Micro Channel ST-506",	/* Controller Name */
	(CCAP_DMA | CCAP_NOSEEK | CCAP_RETRY | CCAP_ERRCOR),	/* capabilities */
	0x0,	/* Primary memory address */
	0x0,	/* Secondary memory address */
	0x320,	/* Primary I/O address */
	0x0,	/* Secondary I/O address */
	3,	/* Primary DMA Channel */
	0,	/* Secondary DMA Channel */
	255,	/* Max # of sector transfer count */
	MCST_DISKS,	/* Up to specified number of drives */
	10,	/* 100us drive switch delay */
	0,	/* Start at this minormap entry */
	512,	/* Default sector size */
	mcst_bdinit,	/* init board function */
	mcst_drvinit,	/* init drive function */
	mcst_cmd,	/* command function */
	NULL,	/* no open function */
	NULL,	/* no close function */
	NULL,	/* No Master Interrupt */
		{	/* Interrupt entries */
		14, mcst_int,	/* First Hardware Interrupt */
		},
		{	/* Special IOCTL handlers */
		0,	/* None present */
		},
	},
	{
	"Non-SCSI Cartridge Tape-8", /* Controller Name */
	(CCAP_DMA | CCAP_16BIT | CCAP_NOSEEK), /* capabilities */
	0L,                     /* No memory address */
	0L,                     /* No memory address */
	0x300,                  /* Primary I/O space address (task file) */
	0x000,                  /* Secondary I/O address(Fixed Disk Register) */
	1,                      /* DMA */
	0,                      /* No DMA */
	128,                    /* Max # of sector transfer count */
	1,                      /* 1 drive */
	10,                     /* 100us drive switch delay */
	6,                      /* minormap entry */
	512,                    /* Default sector size (all you get on AT) */
	ict_bdinit,            /* init board function */
	ict_drvinit,           /* init drive function */
	ict_cmd,               /* command function */
	NULL,                   /* no open function */
	NULL,                   /* no close function */
	NULL,                   /* No Master Interrupt */
		{               /* Interrupt entries */
		5, ict_int,   /* First Hardware Interrupt */
		},
		{               /* Special IOCTL handlers */
		0,              /* None present */
		},
	},
};

ushort  device_cfg_entries = sizeof(device_cfg_tbl)/sizeof(struct gdev_cfg_entry);
int     dsk_devflag = 0;

/*
 * Generic Device Driver space allocations.
 * This version supports both DISK and TAPE.
 */

/*
 * extern defs for driver initialization functions
 */
extern int dsk_init();


ushort  gdev_maxctls = GDEV_MAXCTLS;
ushort  gdev_maxdrivs = GDEV_MAXDRIVS;
ushort  gdev_max_int_span = GDEV_MAX_INT;
ushort  gdev_intents = GDEV_INTENTS;
ushort  gdev_sharemax = GDEV_SHAREMAX;

/*
 * The following defines a pool of gdev_ctl_blocks and gdev_parm_blocks
 * for use during initialization.  Also, a sample configuration table
 * is declared.  This must be filled in for actual controllers expected
 * to be present in a system.  The sample given is for an
 * Adaptec/WD/<stock AT> controller using minor numbers 0-31.
 */

struct gdev_ctl_block gdev_ctl_blocks[GDEV_MAXCTLS];
struct gdev_parm_block gdev_parm_blocks[GDEV_MAXDRIVS];

ushort gdev_nextctl = 0;        /* Next gdev_ctl_block to allocate */
ushort gdev_nextdriv = 0;       /* Next gdev_parm_block to allocate */
ushort gdev_next_int = 0;       /* next gdev_int_routines to use */

ushort gdev_nextminor = 0;      /* next minormap entry to use */
ushort gdev_minormaps = GDEV_MINORMAPS;
ushort gdev_majormaps = GDEV_MAJORMAPS;
struct minormap minormap[GDEV_MINORMAPS];       /* The actual minormap table */
struct majormap majormap[GDEV_MAJORMAPS];


struct  gdev_int_entry  *gdev_int_routines[GDEV_MAX_INT];  /* actual table */

struct  gdev_int_entry  gdev_int_entries[GDEV_INTENTS]; /* Pool of entries */

/*
 * The following table is used to process device errors.
 * It is indexed by the Generic error code in dpb_drverror.
 */

struct gdev_err_msg gdev_err_msgs[] =
	{
	{EIO, ERF_PANIC, "Logic Problem.  NO ERROR FOUND"},
	{EIO, 0, "Data Address Mark not found"},
	{EIO, ERF_PANIC, "Track 0 not found (unable to recalibrate)"},
	{EIO, ERF_PANIC, "Write fault on drive"},
	{EIO, ERF_NORETRY, "Drive is not ready"},
	{EIO, 0, "Controller will not come ready"},
	{EIO, ERF_PANIC, "Seek will not complete"},
	{EIO, 0, "Seek error (wrong cylinder found)"},
	{EIO, 0, "No Index signal found"},
	{EIO, ERF_NORETRY, "Medium is write-protected"},
	{EIO, ERF_NORETRY, "Medium not present in drive"},
	{EIO, 0, "Error found in sector ID field"},
	{EIO, 0, "Sector not found"},
	{EIO, 0, "Uncorrectable data error in sector"},
	{EIO, ERF_QUIET, "Sector or track was marked bad"},
	{EIO, 0, "Error during FORMAT operation"},
	{EIO, 0, "Illegal or erroneous command"},
	{EIO, ERF_PANIC, "Controller error or failure"},
	{EIO, 0, "Controller command Aborted"},
	{EIO, 0, "Drive is still seeking"},
	{EIO, ERF_NORETRY, "Medium has been changed in drive"},
	{EIO, ERF_NORETRY, "Attempt to do I/O past end of drive"},
	{EIO, 0, "Data overrun"},
	{EIO, ERF_PANIC, "Command Timeout"},
	{EIO, 0, "Unable to get valid drive configuration"},
	{EIO, 0, "Undetermined error"},
	{0,   ERF_NORETRY | ERF_QUIET, "EOF/EOM Detected"},
	};

int (*gdev_init_routines[])() =
	{
	dcd_init,
	NULL,
	};

ushort  gdev_lowlev_size = sizeof(ulong) * GDEV_LOWLEV;

struct	hba_idata	dcd_idata[DCD__CNTLS]	= {
#ifdef	DCD__0
	{ 1, "DCD",
	  7, DCD__0_SIOA, DCD__0_CHAN, DCD__0_VECT, DCD__0, 0 }
#endif
#ifdef	DCD__1
	,
	{ 1, "DCD",
	  7, DCD__1_SIOA, DCD__1_CHAN, DCD__1_VECT, DCD__1, 0 }
#endif
};

int	dcd__cntls	= DCD__CNTLS;

int	dcd_halt_delay = 2;	/** Number of seconds dcd_halt() will **/
				/** "delay" the system shutdown. This **/
				/** allows DCD disk drives/controllers **/
				/** time to flush any onboard caches   **/
