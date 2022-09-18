/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)nserve:nsrec.h	1.3.4.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/nserve/nsrec.h,v 1.1 91/02/28 18:42:32 ccs Exp $"
/*
 * defines needed for recovery
 */
#define MAXCHILDREN	10
#define MAXDOMAINS	10
#define R_UNUSED	0
#define R_PRIME		1
#define R_NOTPRIME	2
#define R_DEAD		3
#define R_UNK		4
#define R_PENDING	5
#define POLLTIME	300

extern char	*Mydomains[];
extern int	Mylastdom;
extern struct address	*Paddress[];
extern struct address	*Myaddress;
extern int	Recover;
