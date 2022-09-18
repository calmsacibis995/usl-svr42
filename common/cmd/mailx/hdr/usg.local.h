/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:hdr/usg.local.h	1.6.2.4"
#ident "@(#)usg.local.h	1.11 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Declarations and constants specific to an installation.
 */
 
/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 */

#ifdef preSVr4
# define	MAIL	"/bin/rmail"	/* Mail delivery agent */
#else
# define	MAIL	"/usr/bin/rmail"/* Mail delivery agent */
#endif
#define	EDITOR		"ed"		/* Name of text editor */
#define	VISUAL		"vi"		/* Name of display editor */
#ifdef preSVr4
# define	SHELL	"/bin/sh"	/* Standard shell */
#else
# define	SHELL	"/usr/bin/sh"	/* Standard shell */
#endif
#define	HELPFILE	helppath("mailx.help")
					/* Name of casual help file */
#define	THELPFILE	helppath("mailx.help.~")
					/* Name of casual tilde help */
#ifdef preSVr4
# define	MASTER	libpath("mailx.rc")
#else
# define	MASTER	"/etc/mail/mailx.rc"
#endif
#define	APPEND				/* New mail goes to end of mailbox */
#define CANLOCK				/* Locking protocol actually works */
#define	UTIME				/* System implements utime(2) */
