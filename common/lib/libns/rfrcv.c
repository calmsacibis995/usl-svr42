/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libns:common/lib/libns/rfrcv.c	1.1.6.2"
#ident  "$Header: rfrcv.c 1.2 91/06/26 $"

#include "tiuser.h"
#include <stdio.h>
#include "nslog.h"

/*
 * substitute for t_rcv to read in a pseudo-byte stream fashion.
 */

int
rf_rcv(fd, bufp, bytes, flagp)
int fd;
char *bufp;
int bytes;
int *flagp;
{
	register int n;
	register int count = bytes;
	register char *bp = bufp;

	LOG2(L_TRACE, "(%5d) enter: rf_rcv\n", Logstamp);
	*flagp = 0;

	do {
		n = t_rcv(fd, bp, count, flagp);
		if (n < 0) {
			LOG2(L_TRACE, "(%5d) leave: rf_rcv\n", Logstamp);
			return(n);
		}
		count -= n;
		bp += n;
	} while (count > 0);

	LOG2(L_TRACE, "(%5d) leave: rf_rcv\n", Logstamp);
	return(bp - bufp);
}
