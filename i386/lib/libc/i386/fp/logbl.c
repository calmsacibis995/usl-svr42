/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:fp/logbl.c	1.3"
/*LINTLIBRARY*/

/* IEEE recommended functions */

#ifdef __STDC__
	#pragma weak logbl = _logbl
	#pragma weak nextafterl = _nextafterl
#endif

#include "values.h"
#include <errno.h>
#include "fpparts.h"
#include "synonyms.h"

/* LOGBL(X)
 * logbl(x) returns the unbiased exponent of x as a long double precision
 * number, except that logbl(NaN) is NaN, logbl(infinity) is +infinity,
 * logbl(0) is -infinity and raises the divide by zero exception,
 * errno = EDOM.
 * Signalling NaNs raise the invalid op exception
 */
long double logbl(x)
long double x;
{
	register int iexp = EXPONENTLD(x);

	if (iexp == MAXEXPLD) { /* infinity  or NaN */
		SIGNBITLD(x) = 0;
		errno = EDOM;
		if ((LOFRACTIONLD(x) || HIFRACTIONLD(x)) &&
			!QNANBITLD(x))
			/* signalling nan - raise invalid op */
			return(x + 1.0);
		else
			return x;
	}
	if (iexp == 0)  {  /* de-normal  or 0*/
		if ((HIFRACTIONLD(x) == 0) && (LOFRACTIONLD(x) == 0)) { /*zero*/
			long double zero = 0.0L;
			errno = EDOM;
			return(-1.0L/zero); /* return -inf - raise div by 
					    * zero exception
					    */
		}
		else  /* de-normal */
			return(-16382.0L);
	}
	return((long double)(iexp - 16383.0L)); /* subtract bias */
}

/* NEXTAFTERL(X,Y)
 * nextafterl(x,y) returns the next representable neighbor of x
 * in the direction of y
 * Special cases:
 * 1) if either x or y is a NaN then the result is one of the NaNs, errno
 * 	=EDOM
 * 2) if x is +-inf, x is returned and errno = EDOM
 * 3) if x == y the results is x without any exceptions being signalled
 * 4) overflow  and inexact are signalled when x is finite,
 *	but nextafterl(x,y) is not, errno = ERANGE
 * 5) underflow and inexact are signalled when nextafterl(x,y)
 * 	lies between +-(2**-1022), errno = ERANGE
 */
 long double nextafterl(x,y)
 long double x,y;
 {
	if (EXPONENTLD(x) == MAXEXPLD) { /* Nan or inf */
		errno = EDOM;
		if ((LOFRACTIONLD(x) || HIFRACTIONLD(x)) &&
			!QNANBITLD(x))
			/* signalling nan - raise invalid op */
			return(x + 1.0);
		else
			return x; 
	}
	if ((EXPONENTLD(y) == MAXEXPLD) && (HIFRACTIONLD(y) || 
		LOFRACTIONLD(y))) {
		errno = EDOM;
		if (!QNANBITLD(y))
			/* signalling nan - raise invalid op */
			return(y + 1.0);
		else
			return y; /* quiet nan */
	}
	if (x == y)
		return x;
	if (((y > x) && !SIGNBITLD(x)) || ((y < x) && SIGNBITLD(x))) {
		/* y>x, x negative or y<x, x positive */

		if (LOFRACTIONLD(x) != (unsigned)0xffffffff)
			LOFRACTIONLD(x) += 0x1;
		else {
			LOFRACTIONLD(x) = 0x0;
			/* if (((unsigned)(HIWORDLD(x) & 0x7fffffff)) < (unsigned)0x7fff0000)
				HIWORDLD(x) += 0x1; */
			if((EXPONENTLD(x) < (unsigned)0x7fff) && 
				(HIFRACTIONLD(x) == 0x0))
					HIFRACTIONLD(x) += 0x1;
		}
	}
	else { /* y<x, x pos or y>x, x neg */

		if (LOFRACTIONLD(x) != 0x0)
			LOFRACTIONLD(x) -= 0x1;
		else {
/*			LOWORDLD(x) = (unsigned)0xffffffff;
			if ((HIWORDLD(x) & 0x7fffffff) != 0x0)
				HIWORDLD(x) -=0x1; */
			LOFRACTIONLD(x) = (unsigned)0xffffffff;
			if((EXPONENTLD(x) != 0x0) &&
				(HIFRACTIONLD(x) != 0x0))
					HIFRACTIONLD(x) -= 0x1;
		}
	}
	if (EXPONENTLD(x) == MAXEXPLD) { /* signal overflow and inexact */
		x = (SIGNBITLD(x) ? -MAXLONGDOUBLE : MAXLONGDOUBLE);
		errno = ERANGE;
		return( x * 2.0);
	}
	if (EXPONENTLD(x) == 0) {
	/* de-normal - signal underflow and inexact */
		x = (SIGNBITLD(x) ? -MINLONGDOUBLE : MINLONGDOUBLE);
		errno = ERANGE;
		return(x / 2.0);
	}
	return x;
}
