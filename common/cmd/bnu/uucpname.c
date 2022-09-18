/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bnu:uucpname.c	2.3.6.2"
#ident "$Header: uucpname.c 1.1 91/02/28 $"

#include "uucp.h"
#include <sys/utsname.h>

/*
 * get the uucp name
 * return:
 *	none
 */
void
uucpname(name)
register char *name;
{
	char *s;

	struct utsname utsn;

	uname(&utsn);
	s = utsn.nodename;

	(void) strncpy(name, s, MAXBASENAME);
	name[MAXBASENAME] = '\0';
	return;
}
