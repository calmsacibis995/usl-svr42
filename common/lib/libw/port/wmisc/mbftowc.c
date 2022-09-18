/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wmisc/mbftowc.c	1.1.2.2"
#ident  "$Header: mbftowc.c 1.2 91/06/26 $"
#include <widec.h>
#include <ctype.h>
#include <stdlib.h>
#include "_wchar.h"

#ifdef _WCHAR16
# define MY_P11		H_P11
# define MY_P01		H_P01
# define MY_P10		H_P10
# define MY_SHIFT	8
#else
# define MY_P11		P11
# define MY_P01		P01
# define MY_P10		P10
# define MY_SHIFT	7
#endif
	
/* returns number of bytes read by *f */
int
mbftowc(s, wchar, f, peekc)
	char *s;
	wchar_t *wchar;
	int (*f)();
	int *peekc;
{
	register int length;
	register wchar_t intcode;
	register int c;
	char *olds;
	wchar_t mask;

	if ((c = (*f)()) < 0)
		return 0;
	*s = c;
	if (!multibyte || c < 0200)
	{
invalid_prefix:
		*wchar = c;
		return 1;
	}
	if (c == SS2)
	{
		if ((length = eucw2) == 0)
			goto invalid_prefix;
#ifdef _WCHAR16
		if (length == 1)
		{
			if ((c = (*f)()) >= 0240)
			{
				*++s = c;
				*wchar = (c & 0177) | MY_P01;
				return 2;
			}
			if (c >= 0)
				*peekc = c;
			return -1;
		}
#endif
		mask = MY_P01;
		intcode = 0;
	}
	else if (c == SS3)
	{
		if ((length = eucw3) == 0)
			goto invalid_prefix;
		mask = MY_P10;
		intcode = 0;
	}
	else if (c < 0240)	/* C1 (or metacontrol) byte */
		goto invalid_prefix;
	else
	{
		if ((length = (int)eucw1 - 1) < 0)
			return -1;
		mask = MY_P11;
		intcode = c & 0177;
	}
	olds = s;
	while (--length >= 0)
	{
		if ((c = (*f)()) < 0200)
		{
			if (c >= 0)
				*peekc = c;
			return -(s - olds);
		}
		*++s = c;
		intcode <<= MY_SHIFT;
		intcode |= c & 0177;
	}
	*wchar = intcode | mask;
	return s - olds + 1;
}
