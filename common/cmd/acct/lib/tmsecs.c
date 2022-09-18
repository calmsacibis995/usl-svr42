/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/lib/tmsecs.c	1.5.1.3"
#ident "$Header: $"
/*
 *	tmsecs returns number of seconds from t1 to t2,
 *	times expressed in localtime format.
 *	assumed that t1 <= t2, and are in same day.
 */

#include <time.h>
long
tmsecs(t1, t2)
register struct tm *t1, *t2;
{
	return((t2->tm_sec - t1->tm_sec) +
		60*(t2->tm_min - t1->tm_min) +
		3600L*(t2->tm_hour - t1->tm_hour));
}
