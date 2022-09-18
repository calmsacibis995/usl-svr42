/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:gen/ldexpl.c	1.1"
/*LINTLIBRARY*/
/*
 *	long double ldexpl (value, exp)
 *		long double value;
 *		int exp;
 *
 *	Ldexp returns value * 2**exp, if that result is in range.
 *	If underflow occurs, it returns zero.  If overflow occurs,
 *	it returns a value of appropriate sign and largest single-
 *	precision magnitude.  In case of underflow or overflow,
 *	the external int "errno" is set to ERANGE.  Note that errno is
 *	not modified if no error occurs, so if you intend to test it
 *	after you use ldexpl, you had better set it to something
 *	other than ERANGE first (zero is a reasonable value to use).
 */

#include "synonyms.h"
#include "shlib.h"
#include <values.h>
#include <math.h>
#include <errno.h>
/* Largest signed long int power of 2 */
#define MAXSHIFT	(BITSPERBYTE * sizeof(long) - 2)

asm	long double
xldexpl(x,n)
{
%mem	x; reg n;
	pushl	n
	fildl	(%esp)
	popl	%eax
	fldt	x
	fscale
	fstp	%st(1)
%mem	x,n;
	fildl	n
	fldt	x
	fscale
	fstp	%st(1)
}

extern long double frexpl();

long double
ldexpl(value, exp)
long double value;
int exp;
{
	int old_exp;
	long double ret;

	if (exp == 0 || value == 0.0) /* nothing to do for zero */
		return (value);
	(void) frexpl(value, &old_exp);
	if (exp > 0) {
		if ( (exp > (MAXINT-MAXBEXP))
		/* guard against integer overflow in next addition */
	    	|| ((exp + old_exp) > MAXBEXP) ) {
		/* we have floating point overflow condition */

			errno = ERANGE;
			if (_lib_version == c_issue_4)
				return (value < 0 ? -HUGE : HUGE);
			else
				return (value < 0 ? -HUGE_VAL : HUGE_VAL);
		}
		return (xldexpl(value,exp));
	}
	if ( (exp < -(MAXINT-MAXBEXP)) ||
		/* guard against integer overflow in next addition */
		(exp + old_exp < MINBEXP) ) { /* underflow */
		errno = ERANGE;
		return (0.0);
	}
	return (xldexpl(value,exp));
}
