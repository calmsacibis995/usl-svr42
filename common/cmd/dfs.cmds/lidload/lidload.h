/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)dfs.cmds:lidload/lidload.h	1.1.2.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/dfs.cmds/lidload/lidload.h,v 1.1 91/02/28 16:54:33 ccs Exp $"

#include <sys/types.h>
#include <sys/tiuser.h>

/*
 * struct lap: format of an entry in /etc/dfs/lid_and_priv
 * is the first arg to makelist()
 */
struct lap {
	char	domainname[1024];
	char	hostname[1024];
	char	lvlname[1024];
	char	privlist[1024];
};

/*
 * struct lpgen: the converted internal form of an entry
 */
struct lpgen {
	char		*lp_hostname;
	struct netbuf	*lp_addr;
	struct netbuf	*lp_mask;
	lid_t		lp_lid;
	pvec_t		lp_priv;
	int		lp_valid;	
};
