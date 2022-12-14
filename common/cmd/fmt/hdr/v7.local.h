/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1983, 1984, 1985, 1986, 1987, 1988, Sun Microsystems, Inc. */
/* All Rights Reserved.						            */

#ident	"@(#)fmt:hdr/v7.local.h	1.2.3.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fmt/hdr/v7.local.h,v 1.1 91/02/28 17:19:40 ccs Exp $"

/*
 * Declarations and constants specific to an installation.
 */
 
/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 */

#define	GETHOST				/* Have gethostname syscall */
/* #define	UNAME			/* System has uname syscall */
#define	LOCAL		EMPTYID		/* Dynamically determined local host */

#define MYDOMAIN	".uucp"		/* Appended to local host name */

#define	MAIL		"/usr/bin/mail"	/* Name of mail sender */
#define SENDMAIL	"/usr/lib/sendmail"
					/* Name of classy mail deliverer */
#define	EDITOR		"/usr/ucb/ex"	/* Name of text editor */
#define	VISUAL		"/usr/ucb/vi"	/* Name of display editor */
#define	MORE		(value("PAGER") ? value("PAGER") : "/usr/bin/more")
					/* Standard output pager */
#define	LS		(value("LISTER") ? value("LISTER") : "/usr/bin/ls")
					/* Name of directory listing prog*/
#define	SHELL		"/usr/bin/sh"	/* Standard shell */
#define	HELPFILE	libpath("Mail.help")
					/* Name of casual help file */
#define	THELPFILE	libpath("Mail.help.~")
					/* Name of casual tilde help */
#define	UIDMASK		0177777		/* Significant uid bits */
#define	MASTER		libpath("Mail.rc")
#define	APPEND				/* New mail goes to end of mailbox */
#define CANLOCK				/* Locking protocol actually works */
#define	UTIME				/* System implements utime(2) */

#ifndef VMUNIX
#include "sigretro.h"			/* Retrofit signal defs */
#endif
