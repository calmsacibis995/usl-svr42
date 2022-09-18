/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:usg.local.c	1.3.2.5"
#ident "@(#)usg.local.c	1.10 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Local routines that are installation dependent.
 */

#include "rcv.h"

/*
 * Locate the user's mailbox file (ie, the place where new, unread
 * mail is queued).  In SVr4 UNIX, it is in /var/mail/name.
 * In preSVr4 UNIX, it is in either /usr/mail/name or /usr/spool/mail/name.
 */
void
findmail()
{
	register char *cp;

	cp = copy(maildir, mailname);
	copy(myname, cp);
	if (isdir(mailname)) {
		strcat(mailname, "/");
		strcat(mailname, myname);
	}
}

/*
 * Return the value of $PAGER
 */
const char *pager()
{
	char *pg = value("PAGER");
	if (!pg)		/* default to pg -e if not set */
		pg = "pg -e";
	else if (pg && !*pg)	/* default to cat if no value */
		pg = "cat";
	return pg;
}

/*
 * Return the value of $LISTER
 */
const char *lister()
{
	char *ls = value("LISTER");
	if (!ls || !*ls)	/* default to ls if not set or no value */
		ls = "ls";
	return ls;
}


