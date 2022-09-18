/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_HBA_AHA_H	/* wrapper symbol for kernel use */
#define _IO_HBA_AHA_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/hba/aha.h	1.3"
#ident	"$Header: $"

extern void adscintr();
extern struct hbadata *adscgetblk();
extern long adscfreeblk();
extern long adscicmd();
extern int adscinit();
extern void adscgetinfo();
extern long adscsend();
extern void adscxlat();
extern int adscopen();
extern int adscclose();
extern int adscioctl();

#endif /* _IO_HBA_AHA_H */
