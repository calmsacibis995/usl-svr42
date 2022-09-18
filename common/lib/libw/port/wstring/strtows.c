/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/strtows.c	1.1.2.2"
#ident  "$Header: strtows.c 1.2 91/06/26 $"

/*
 * Strtows transforms EUC in character string "s2" into
 * the process codes, and transfers those to wchar_t string "s1",
 * stopping after the NULL character has been processed.
 */

#include <widec.h>
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

wchar_t *
strtows(s1, s2)
	register wchar_t *s1;
	register char *s2;
{
	register int length;
	register wchar_t intcode;
	register int c;
	int retval;
	wchar_t mask, *os1 = s1;

	if (!multibyte)	/* only ask this question once */
	{
		while ((*s1++ = (unsigned char)*s2++) != '\0')
			;
		return os1;
	}
	if ((c = (unsigned char)*s2) == '\0')
	{
		*s1 = 0;
		return s1;
	}
	do
	{
		if (c < 0200)
		{
invalid_prefix:
			*s1++ = c;
			continue;
		}
		if (c == SS2)
		{
			if ((length = eucw2) == 0)
				goto invalid_prefix;
#ifdef _WCHAR16
			if (length == 1 && (unsigned char)s2[1] < 0240)
				goto bad_seq;
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
				goto bad_seq;
			mask = MY_P11;
			intcode = c & 0177;
		}
		while (--length >= 0)
		{
			if ((c = (unsigned char)*++s2) < 0200)
			{
bad_seq:
				*s1 = 0;
				errno = EILSEQ;
				return 0;
			}
			intcode <<= MY_SHIFT;
			intcode |= c & 0177;
		}
		*s1++ = intcode | mask;
	} while ((c = (unsigned char)*++s2) != '\0');
	*s1 = 0;
	return os1;
}
