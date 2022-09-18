/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_TARGET_SD01_IOCTL_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SD01_IOCTL_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/target/sd01_ioctl.h	1.2"
#ident	"$Header: $"

#define SD_CHAR		('D' << 8)
#define	SD_ELEV		(SD_CHAR | 0x1)		/* Elevator Algorithm */
#define	SD_PDLOC	(SD_CHAR | 0x2)		/* Absolute PD sector */

#endif /* _IO_TARGET_SD01_IOCTL_H */
