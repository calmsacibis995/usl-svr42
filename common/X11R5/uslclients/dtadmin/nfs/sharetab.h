/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/sharetab.h	1.2"
#endif

/* this file is also known as :
#ident	"@(#)nfs.cmds:nfs/share/sharetab.h	1.2.4.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/nfs/share/sharetab.h,v 1.1 91/02/28 17:24:35 ccs Exp $"
*/

struct share {
	char *sh_path;
	char *sh_res;
	char *sh_fstype;
	char *sh_opts;
	char *sh_descr;
};

#define SHARETAB  "/etc/dfs/sharetab"

/* generic options */
#define SHOPT_RO	"ro"
#define SHOPT_RW	"rw"

/* options for nfs */
#define SHOPT_ROOT	"root"
#define SHOPT_ANON	"anon"
#define SHOPT_SECURE	"secure"
#define SHOPT_WINDOW	"window"

int		getshare();
int		putshare();
int		remshare();
char *		getshareopt();
