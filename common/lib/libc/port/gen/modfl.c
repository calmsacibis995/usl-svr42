/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/modfl.c	1.1"
/*LINTLIBRARY*/
/*
 * modfl(value, iptr) returns the signed fractional part of value
 * and stores the integer part indirectly through iptr.
 *
 */

#ifdef __STDC__
	#pragma weak modfl = _modfl
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <values.h>
#include <nan.h>


long double
modfl(value, iptr)
long double value; /* don't declare register, because of KILLNaN! */
register long double *iptr;
{
	long double absvalue;

	if (IsNANorINFLD(value)) { /* check for NAN or INF (IEEE only) */
		if (IsINFLD(value)) { /* infinity */
			*iptr = value;
			return(0.0L);
		}
		else KILLNaN(value); /* raise exception on Not-a-Number */
	}
	if ((absvalue = (value >= 0.0L) ? value : -value) >= LDMAXPOWTWO) {
		*iptr = value; /* it must be an integer */
	}
	else {
		*iptr = absvalue + LDMAXPOWTWO; /* shift fraction off right */
		*iptr -= LDMAXPOWTWO; /* shift back without fraction */
		while (*iptr > absvalue) /* above arithmetic might round */
			*iptr -= 1.0L; /* test again just to be sure */
		if (value < 0.0L)
			*iptr = -*iptr;
	}
	return (value - *iptr); /* signed fractional part */
}
