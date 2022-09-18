/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:fp/scalbl.c	1.1"
/*LINTLIBRARY

/* SCALBL(X,N)
 * return x * 2**N without computing 2**N - this is the standard
 * C library ldexpln() routine except that signaling NANs generate
 * invalid op exception - errno = EDOM
 */

#ifdef __STDC__
	#pragma weak scalbl = _scalbl
#endif
#include "synonyms.h"
#include <values.h>
#include <math.h>
#include <errno.h>
#include "fpparts.h"
#include <limits.h>
#if	_IEEE
#include <nan.h>
#endif

extern long double ldexpl();

long double 
scalbl(x,n)
long double	x, n;
{
	long double ret;
#if _IEEE
	if ((EXPONENTLD(x) == MAXEXPLD) && !QNANBITLD(x) && (HIFRACTIONLD(x)
		|| LOFRACTIONLD(x)) ) {
		errno = EDOM;
		return (x + 1.0); /* signaling NaN - raise exception */
	}
#endif

	if ((n >= (long double)INT_MAX) || (n <= (long double)INT_MIN)) 
	{
		if(n < 0.0) return(0.0); 
			/* lim n -> -Inf of x * 2**n = 0 		*/
		else { 	
#if _IEEE
			/* 0.0 * 2**+-Inf = NaN				*/
			if ((x == 0.0) && !QNANBITLD(x)) {
				/* fake up a NaN			*/
				HIQNANLD(x);
				LOQNANLD(x);
				/* ensure the NaN is positive		*/
				((ldnan *)&(n))->nan_parts.sign = 0x0;
				return(x);	/* returns a signaling NaN */
			}
#endif
			/* lim n -> Inf of x * 2**n = -Inf or +Inf 	*/
			if (_lib_version == c_issue_4)
				return(x > 0.0 ? HUGE : -HUGE);
			else
				return(x > 0.0 ? HUGE_VAL : -HUGE_VAL);
		}
	}

	/* 0.0 * 2**n = x or x * 2**0 = x * 1 = x	*/
	else if ((x == 0.0) || (n == 0.0)) return x;

	return(ldexpl(x, (int)n));
}
