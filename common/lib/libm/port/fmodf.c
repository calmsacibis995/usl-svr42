/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/fmodf.c	1.9"
/*LINTLIBRARY*/

/* fmodf(x,y)
 * single precision fmod
 * Return f =x-n*y, for some integer n, where f has same sign
 * as x and |f| < |y|
 *
 *  for y == 0, f = 0 
 * This implementation calculates IEEE rem and uses that to calculate
 * fmod
 */

#include "synonyms.h"
#include <math.h>
#include <errno.h>

float	fmodf(register float x, register float y)
{
	register float	r;

	if (y == (float)0.0) {
		struct exception exc;
		exc.type = DOMAIN;
		exc.name = "fmodf";
		exc.arg1 = (double)x;
		exc.arg2 = 0.0;
		exc.retval = (double)x;
		if (_lib_version == strict_ansi)
			errno = EDOM;
		else {
			if (!matherr(&exc)) {
				if (_lib_version == c_issue_4)
					(void)write(2,"fmodf: DOMAIN error\n",20);
				errno = EDOM;
			}
		}
		return (float)exc.retval;
	}
	if (y < (float)0.0)
		y = -y;
	r = (float)remainder(x, y);
	/*
	 * At this point we have rem(x,y)
	 * to get fmodf(x,y), we test the signs of x and of the
	 * remainder - if they're the same, we are done,
	 * else if x is negative, replace remainder with remainder -|y|,
	 * if x positive with remainder + |y| 
	 */
	if (x < (float)0.0) {
		if (r > (float)0.0)
			r -= y;
	}
	else { 
		if (r < (float)0.0)
			r += y;
	
	}
	return r;
}
