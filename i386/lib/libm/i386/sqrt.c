/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:i386/sqrt.c	1.5"

/*LINTLIBRARY*/

/*
 *	sqrt returns the square root of its double-precision argument
 *	using the 80287 processor or the 80287 emulator.
 *	Returns EDOM error and value 0 if argument negative.
 */

#include "synonyms.h"
#include <errno.h>
#include <math.h>			/* temporary location	*/
#include <values.h>
#include "fpparts.h"

asm	double xsqrt(x)
{
%mem	x;
	fldl	x
	fsqrt
}

double
sqrt(x)
double x;
{

	if (ISMAXEXP(x) || SIGNBIT(x)) {
		struct exception exc;
		double q1 = 0.0, q2 = 0.0;
		int	mx = EXPONENT(x);

		if (LOWORD(x) == 0 && (HIFRACTION(x) == 0) &&
			(mx == 0 || (mx == MAXEXP && !SIGNBIT(x))))
			/* 0 or +infinity */
			return x;
		exc.type = DOMAIN;
		exc.name = "sqrt";
		exc.arg1 = x;
		if (mx != MAXEXP || !QNANBIT(x))
			q1 /= q2; /* raise invalid op exception 
				   * except for quiet Nans
				   */
		if (mx == MAXEXP && (LOFRACTION(x) || HIFRACTION(x)))
		{
			/* NaN */
			QNANBIT(x) = 1;
			exc.retval = x;
		}
		else if (_lib_version == strict_ansi) {
			HIQNAN(exc.retval); /* return NaN */
			LOQNAN(exc.retval);
			errno = EDOM;
		}
		else {
			exc.retval = 0.0;
		}
		if (_lib_version != strict_ansi) {
			if (!matherr(&exc)) {
				if (_lib_version == c_issue_4)
					(void)write(2,"sqrt: DOMAIN error\n",19);
				errno = EDOM;
			}
		}
		return exc.retval;
	}

	return(xsqrt(x));
}
