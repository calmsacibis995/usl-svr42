/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:i386/cmd/oampkg/hdrs/install.h	1.3.9.4"
#ident  "$Header: $"

#define MAILCMD	"/usr/bin/mail"
#define DATSTRM	"datastream"
#define SHELL	"/sbin/sh"
#define PKGINFO	"pkginfo"
#define PKGMAP	"pkgmap"
#define	SETINFO	"setinfo"
#define isdot(x)	((x[0]=='.')&&(!x[1]||(x[1]=='/')))
#define isdotdot(x)	((x[0]=='.')&&(x[1]=='.')&&(!x[2]||(x[2]=='/')))
#define INPBUF	128

struct mergstat {
	char	*setuid;
	char	*setgid;
	char	contchg;
	char	attrchg;
	char	shared;
};

struct admin {
	char	*mail;
	char	*instance;
	char	*partial;
	char	*runlevel;
	char	*idepend;
	char	*rdepend;
	char	*space;
	char	*setuid;
	char	*conflict;
	char	*action;
	char	*basedir;
	char	*list_files;
};

#define ADM(x, y)	((adm.x != NULL) && (y != NULL) && !strcmp(adm.x, y))

/*
 * Value returned by Set Installation Package request script
 * when no set member packages are selected for installation.
 */
#define	NOSET	77

#define MSG_REBOOT \
"\\n*** IMPORTANT NOTICE ***\\n\
\\tIf installation of all desired packages is complete,\\n\
\\tthe machine should be rebooted in order to\\n\
\\tensure sane operation. Execute the shutdown\\n\
\\tcommand with the appropriate options and wait for\\n\
\\tthe \"Console Login:\" prompt."

