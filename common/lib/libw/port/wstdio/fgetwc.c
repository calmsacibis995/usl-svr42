/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstdio/fgetwc.c	1.1.2.2"
#ident  "$Header: fgetwc.c 1.2 91/06/26 $"

/*
 * Fgetwc transforms the next character in EUC from the named input
 * "iop" into the Process Code, and returns it as an integer.
 * The character widths in EUC are determined from an extern variable
 * _ctype[]
 */
#include <widec.h>
#include <stdio.h>
#include <errno.h>
#include <sys/euc.h>
#include "pcode.h"

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

int
fgetwc(iop)
	FILE *iop;
{
	register int length;
	register wchar_t intcode;
	register int c;
	wchar_t mask;

	if ((c = getc(iop)) == EOF)
		return EOF;
	if (!multibyte || c < 0200)
		return c;
	if (c == SS2)
	{
		if ((length = eucw2) == 0)
			return c;
#ifdef _WCHAR16
		if (length == 1)
		{
			if ((c = getc(iop)) < 0240)	/* includes EOF */
				goto bad_byte;
			return (c & 0177) | MY_P01;
		}
#endif
		mask = MY_P01;
		intcode = 0;
	}
	else if (c == SS3)
	{
		if ((length = eucw3) == 0)
			return c;
		mask = MY_P10;
		intcode = 0;
	}
	else if (c < 0240)	/* C1 (or metacontrol) byte */
		return c;
	else
	{
		if ((length = (int)eucw1 - 1) < 0)
			goto bad_byte;
		mask = MY_P11;
		intcode = c & 0177;
	}
	while (--length >= 0)
	{
		if ((c = getc(iop)) < 0200)	/* includes EOF */
		{
bad_byte:
			ungetc(c, iop);
			errno = EILSEQ;
			return EOF;
		}
		intcode <<= MY_SHIFT;
		intcode |= c & 0177;
	}
	return intcode | mask;
}
