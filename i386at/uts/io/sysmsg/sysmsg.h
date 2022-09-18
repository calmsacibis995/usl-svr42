/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_SYSMSG_SYSMSG_H	/* wrapper symbol for kernel use */
#define _IO_SYSMSG_SYSMSG_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/sysmsg/sysmsg.h	1.4"
#ident	"$Header: $"

#define SMSGIOC		('U' << 8)
#define	SMSG_GETFLAGS	SMSGIOC | 200
#define	SMSG_SETFLAGS	SMSGIOC | 201

struct smsg_flags
{
	int	static_cmf;
	int	dynamic_cmf;
	int	static_rcmf;
	int	dynamic_rcmf;
	int	acef;
	int	rcef;
	int	cmos_acef;
	int	ac_baud;
	int	rc_baud;
};

#endif /* _IO_SYSMSG_SYSMSG_H */
