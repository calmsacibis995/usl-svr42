/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/sd01.cf/Space.c	1.7"
#ident  "$Header: $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/elog.h>
#include <sys/vtoc.h>
#include <sys/alttbl.h>
#include <sys/altsctr.h>
#include <sys/sd01.h>
#include "config.h"

struct disk	*Sd01_dp[SD01_BMAJORS*16];	/* Disk structure pointer	*/
struct job	*Sd01_jp;		/* Job structure pointer	*/
int sd01_hiwat = 100;	/* Hiwater level for jobs to HBA level */

struct dev_spec *sd01_dev_spec[] = {
	0
};

struct dev_cfg SD01_dev_cfg[] = {
{	SDI_CLAIM|SDI_ADD, 0xffff, 0xff, 0xff, ID_RANDOM, 0, ""	},
};

int SD01_dev_cfg_size = sizeof(SD01_dev_cfg)/sizeof(struct dev_cfg);

struct drv_majors Sd01_majors[] = {
	SD01_BMAJOR_0, SD01_CMAJOR_0
#if defined(SD01_BMAJOR_1) && defined(SD01_CMAJOR_1)
	,
	SD01_BMAJOR_1, SD01_CMAJOR_1
#endif
#if defined(SD01_BMAJOR_2) && defined(SD01_CMAJOR_2)
        ,
	SD01_BMAJOR_2, SD01_CMAJOR_2
#endif
#if defined(SD01_BMAJOR_3) && defined(SD01_CMAJOR_3)
        ,
	SD01_BMAJOR_3, SD01_CMAJOR_3
#endif
#if defined(SD01_BMAJOR_4) && defined(SD01_CMAJOR_4)
        ,
	SD01_BMAJOR_4, SD01_CMAJOR_4
#endif
#if defined(SD01_BMAJOR_5) && defined(SD01_CMAJOR_5)
        ,
	SD01_BMAJOR_5, SD01_CMAJOR_5
#endif
#if defined(SD01_BMAJOR_6) && defined(SD01_CMAJOR_6)
        ,
	SD01_BMAJOR_6, SD01_CMAJOR_6
#endif
};

char   Sd01_debug[] = {
	0,0,0,0,0,0,0,0,0,0		/* Debug levels			*/
};

#if defined(SD01_CMAJORS)
int	Sd01_cmajors = SD01_CMAJORS;	/* Number of c major numbers	*/
#else
int	Sd01_cmajors = 1;		/* Default 1 major number	*/
#endif

int	Sd01log_marg = 0;		/* Marginal bad block logging   */
unsigned int Sd01bbh_flg = 0;		/* bad block handling flag	*/
					/* see sd01.h for values	*/

/*
 * Define the logical geometry to be used for the various
 * bytes/sector values.  Array can be indexed by the
 * number of bit-shifts required to convert 512 into
 * the desired bps value.
 * Note: Hex value = <secs/trk> and <# of heads>
 */
short	Sd01diskinfo[] = {
	0x2040,				/* 512 bytes/sector		*/
	0x2020,				/* 1K bps			*/
	0x2010,				/* 2K bps			*/
	0x2008,				/* 4K bps			*/
	0x2004,				/* 8K bps			*/
	0x2002,				/* 16K bps			*/
	0x2001,				/* 32K bps			*/
	0x1001,				/* 64K bps			*/
	0x0801,				/* 128K bps			*/
	0x0401,				/* 256K bps			*/
	0x0201,				/* 512K bps			*/
	0x0101,				/* 1M bps			*/
};

int	Sd01_jobs    = 200;		/* Number of job structures	*/

#ifdef RAMD_BOOT
int Sd01_boot_time = 1;
#else
int Sd01_boot_time = 0;
#endif
