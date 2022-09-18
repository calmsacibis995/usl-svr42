/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libns:common/lib/libns/ns_error.c	1.2.5.2"
#ident  "$Header: ns_error.c 1.2 91/06/26 $"
/*	@(#)nserror.c	*/
#include <stdio.h>
#include "nslog.h"

extern int ns_errno, ns_err, strlen(), write();
extern char *ns_errlist[];

void
nserror(s)
char	*s;
{
	register char *c;
	register int n;

	LOG2(L_TRACE, "(%5d) enter: nserror\n", Logstamp);
	c = "Unknown error";
	if(ns_errno < ns_err)
		c = ns_errlist[ns_errno];
	n = strlen(s);
	if(n) {
		(void) write(2, s, (unsigned)n);
		(void) write(2, ": ", 2);
	}
	(void) write(2, c, (unsigned)strlen(c));
	(void) write(2, "\n", 1);
	LOG2(L_TRACE, "(%5d) leave: nserror\n", Logstamp);
}
