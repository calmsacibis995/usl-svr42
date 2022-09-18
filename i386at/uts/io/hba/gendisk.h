/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_HBA_GENDISK_H	/* wrapper symbol for kernel use */
#define _IO_HBA_GENDISK_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/hba/gendisk.h	1.2"
#ident	"$Header: $"

/*
 * definitions for Generic Disk Driver
 */


#define COPY(dest,src,count) \
	{      struct k {char c[(count)];};	\
	       *((struct k *)&(dest)) = *((struct k *)&(src)); \
	}

#define MAX_VERXFER	256		/* maximum len of verify xfer */
extern struct gdev_cfg_entry disk_cfg_tbl[];
extern ushort disk_cfg_entries;

#endif /* _IO_HBA_GENDISK_H */
