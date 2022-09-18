/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:hostname.c	1.5.2.5"
#ident "@(#)hostname.c	1.11 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Code to figure out what host we are on.
 */

#include "rcv.h"

/*
 * Initialize the network name of the current host.
 */
void
inithost()
{
	char *tmp = mailsystem(0);
	host = pcalloc(strlen(tmp) + 1);
	strcpy(host, tmp);

	tmp = maildomain();
	domain = pcalloc(strlen(host) + strlen(tmp) + 1);
	strcpy(domain, host);
	strcat(domain, tmp);
}
