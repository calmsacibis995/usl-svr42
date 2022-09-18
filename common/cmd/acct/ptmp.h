/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/ptmp.h	1.5.1.3"
#ident "$Header: $"
/*
 *	per process temporary data
 */
struct ptmp {
	uid_t	pt_uid;			/* userid */
	char	pt_name[8];		/* login name */
	long	pt_cpu[2];		/* CPU (sys+usr) P/NP time tics */
	unsigned pt_mem;		/* avg. memory size (64byte clicks) */
};	
