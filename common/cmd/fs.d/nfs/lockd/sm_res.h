/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/sm_res.h	1.1.4.3"
#ident	"$Header: $"

struct stat_res {
	res res_stat;
	union {
		sm_stat_res stat;
		int rpc_err;
	}u;
};

#define sm_stat 	u.stat.res_stat
#define sm_state 	u.stat.state

