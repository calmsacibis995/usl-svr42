/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/ctmp.h	1.5.3.3"
#ident "$Header: $"
/*
 *	connect time record (various intermediate files)
 */
struct ctmp {
	dev_t	ct_tty;			/* major minor */
	uid_t	ct_uid;			/* userid */
	char	ct_name[8];		/* login name */
	long	ct_con[2];		/* connect time (p/np) secs */
	time_t	ct_start;		/* session start time */
};
