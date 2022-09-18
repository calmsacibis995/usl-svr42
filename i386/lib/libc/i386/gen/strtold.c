/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:gen/strtold.c	1.9"

#include "synonyms.h"
#include "shlib.h"
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <locale.h>
#include <values.h>
#include "../fp/fpparts.h"
#include "../../port/gen/_locale.h"

static const long double _ldpow10[] = {
		1e1L,
		1e2L,
		1e4L,
		1e8L,
		1e16L,
		1e32L,
		1e64L,
		1e128L,
		1e256L,
		1e512L,
		1e1024L,
		1e2048L,
		1e4096L
	};

#undef LDMAXEXP
#define LDMAXEXP	99999999

/* Assumptions are made about these values! */
#define	SIGN_PLUS	(1)		/* Plus i_sign value */
#define	SIGN_MINUS	(-1)		/* Minus i_sign value */

#define	X_EXPBIAS	((1 << (_LDEXPLEN-1))-1)

/* Permit const declarations by nulling out for non-ANSI C. */
#if !defined(const) && !defined(__STDC__)
#define const
#endif

/* X_MAX_DEC_EXP is a guess at the decimal exponent value that would be
** ridiculously large.  Numbers with exponents larger than this overflow.
** If smaller, they underflow.
*/
#define X_MAX_DEC_EXP	((LDMAXEXP - X_EXPBIAS) * 10)
/* #define X_MAX_DEC_EXP	4932 */	/* hardcoded from 80387 manual	*/

static long double pow10();

long double
strtold(s, endptr)
register const char *s;
char **endptr;
/* Convert a floating point number to a
** double-extended.  Accept leading spaces, leading sign.
*/
{
    long double fraction = 0.0L;
    int sign = SIGN_PLUS;		/* presumed */
    int sawdot = 0;			/* saw . in number if 1 */
    int digs = 0;			/* number of significant digits */
    int expoffset = 0;			/* exponent offset (digits after .) */
    long int exp = 0;
    char * save_s = (char *)s;		/* beginning of string s	*/

    /* Initialize the result to 0. */
    if(endptr != (char **)NULL) *endptr = (char *)s;

    while (isspace(*s))
	++s;
    
    switch( *s ){
    case '-':
	sign = SIGN_MINUS;
	/*FALLTHRU*/
    case '+':
	++s;
    }

    /* Walk through digits, decimal point. */
    while (isdigit(*s) || (*s == _numeric[0] && !sawdot)) {
	if (*s == _numeric[0])
	    sawdot = 1;
	/* Skip leading 0's in integer part. */
	else if (digs || sawdot || *s != '0') {
	    ++digs;
	    fraction *= 10;
	    fraction = fraction + (*s - '0');
	    if (sawdot)
		--expoffset;
	}
	++s;
    }
    if(sign == SIGN_MINUS) fraction = -fraction;

    if(endptr != (char **)NULL) *endptr = (char *)s - 1;
    /* Collect explicit exponent. */
    if (*s == 'e' || *s == 'E') {
	int expsign = SIGN_PLUS;

	switch(*++s) {
	case '-':
	    expsign = SIGN_MINUS;
	    /*FALLTHRU*/
	case '+':
	    ++s;
	}

	/* Collect exponent. */
	while (isdigit(*s)) {
	    if (exp < LDMAXEXP)
		exp = exp * 10 + (*s - '0');
	    ++s;
	}
	if (expsign < 0)
	    exp = -exp;
	if(endptr != (char **)NULL) *endptr = (char *)s;
    }

    if(endptr) *endptr = (char *)s; /* set location pointed to by endptr to a 
	pointer to the character ending the scan	*/

    /* Now form the number from its pieces. */
    exp += expoffset;		/* adjust exponent by digits after . */

    if (exp + digs > X_MAX_DEC_EXP) {
    	errno = ERANGE;
    	/* FIX THIS pr = sign > 0 ? &v_plusinf : &v_minusinf; */
    }
    else if (exp + digs < -X_MAX_DEC_EXP) {
    	errno = ERANGE;
	/* if no number can be formed, set *endptr to s and return zero */
    	if(endptr != (char **)NULL) *endptr = save_s;	
    	fraction = 0.0L;
    }
    else {
	long double pow10();
	long double exponent;

	if (exp < 0)
	{
		while (exp < -4932)
		{
			fraction /= 10;
			exp++;
		}
	}
	else
	{
		while (exp > 4932)
		{
			fraction *= 10;
			exp--;
		}
	}

	exponent = pow10(exp);
    	if (exp >= 0) fraction = fraction * exponent;
    	else fraction = fraction / exponent;
    }
    return(fraction);
}

static long double pow10(x)
long int x;
{
	const long double *tab = _ldpow10;
	long double res = 1.0L;
	int i;

	if(x == 0) return(1.0L);

	if(x < 0.0L) x = -x;

	i = 0;
	while(x != 0) {
		if(x & 0x1) res *= *tab;
		x >>= 1;
		i++;
		tab++;
	}

	return(res);
}
