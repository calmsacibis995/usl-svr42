/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/tacct.h	1.5.1.3"
#ident "$Header: $"
/*
 *	total accounting (for acct period), also for day
 */

struct	tacct	{
	uid_t		ta_uid;		/* userid */
	char		ta_name[8];	/* login name */
	float		ta_cpu[2];	/* cum. cpu time, p/np (mins) */
	float		ta_kcore[2];	/* cum kcore-minutes, p/np */
	float		ta_con[2];	/* cum. connect time, p/np, mins */
	float		ta_du;		/* cum. disk usage */
	long		ta_pc;		/* count of processes */
	unsigned short	ta_sc;		/* count of login sessions */
	unsigned short	ta_dc;		/* count of disk samples */
	unsigned short	ta_fee;		/* fee for special services */
};
