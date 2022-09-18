/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_TARGET_BADSEC_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_BADSEC_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/target/badsec.h	1.2"
#ident	"$Header: $"

#define	BADSECFILE	"/etc/scsi/badsec"

#define	MAXBLENT	4
struct	badsec_lst {
	int	bl_cnt;
	struct	badsec_lst *bl_nxt;
	int	bl_sec[MAXBLENT];
};

#define BADSLSZ		sizeof(struct badsec_lst)

#endif /* _IO_TARGET_BADSEC_H */
