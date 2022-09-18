/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpsched/lpsched/getkey.c	1.1.7.3"
#ident	"$Header: $"

#include "sys/types.h"
#include "time.h"
#include "stdlib.h"

#include "lpsched.h"

long
#ifdef	__STDC__
getkey (
	void
)
#else
getkey ()
#endif
{
	DEFINE_FNNAME (getkey)

	static int		seeded = 0;

	if (!seeded) {
		srand48 (time((time_t *)0));
		seeded = 1;
	}
	return (lrand48());
}
