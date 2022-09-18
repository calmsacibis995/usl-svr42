/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:i386/sqrtf.c	1.3"

/*LINTLIBRARY*/

/*
 *	sqrtf returns the square root of its single-precision argument
 *	using the 80387 processor or the 80387 emulator.
 *	Returns EDOM error and value 0 if argument negative.
 */

#include "synonyms.h"
#include <errno.h>
#include <math.h>			/* temporary location	*/
#include <values.h>
#include "fpparts.h"

asm	float xsqrtf(float x)
{
%mem	x;
	flds	x
	fsqrt
}

float
sqrtf(float x)
{
	float xsqrtf(float);

	if (FISMAXEXP(x) || FSIGNBIT(x)) {
		double q1 = 0.0, q2 = 0.0;
		struct exception exc;
		int	mx = FEXPONENT(x);
		if ((FFRACTION(x) == 0) &&
			((mx == 0) ||
			(mx == MAXEXPF && !FSIGNBIT(x))))
			return (x); /* +infinity or 0 */
		exc.type = DOMAIN;
		exc.name = "sqrtf";
		exc.arg1 = (double)x;

		if (mx != MAXEXPF || !FQNANBIT(x))
			q1 /= q2; /* raise invalid op exception
				   * except for quiet nans */
		if (mx == MAXEXPF && FFRACTION(x))
		{
			FQNANBIT(x) = 1;
			exc.retval = x;
		}
		else if (_lib_version == strict_ansi) {
			HIQNAN(exc.retval); /* return NaN */
			LOQNAN(exc.retval);
		}
		else
			exc.retval = 0.0;
		if (_lib_version == strict_ansi)
			errno = EDOM;
		else if (!matherr(&exc)) {
			if (_lib_version == c_issue_4)
				(void) write(2, "sqrtf: DOMAIN error\n", 20);
			errno = EDOM;
		}
		return (float)(exc.retval);
	}

	return(xsqrtf(x));
}
