/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/getmajors.c	1.1"
#ident	"$Header:"

/* This routine parses an entry from the master file and
 * gets the block and character majors. The multiple majors
 * feature allows the specification of more than one major
 * per device. If more than one major is specified, a "range"
 * notation is used, as in "s-e", where 's' is the first major
 * number in the range and 'e' is the last.
 */

#include "inst.h"
#include <ctype.h>


int getmajors(mstring, start, end)
char *mstring;
short *start;
short *end;
{
	register char *p;
	char savestring[20];
	int dash = 0;

	for(p = mstring; *p != 0; p++) {
		if (!isdigit(*p) && *p != '-')
			return IERR_MAJOR;
		if (*p == '-') {
			*p++ = 0;
			dash++;
			break;
		}
	}

	if (!isdigit(*mstring) || (dash && !isdigit(*p)))
		return IERR_MAJOR;

	*start = atoi(mstring);

	if (!dash)
		*end = *start;
	else
		*end = atoi(p);

	if (*end < *start)
		return IERR_MMRANGE;

	return 0;
}
