/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wsntostr.c	1.1.2.2"
#ident  "$Header: wsntostr.c 1.2 91/06/26 $"

/*
 * Wsntostr transforms process codes in wchar_t string "s2"
 * into EUC, and transfers those to character string "s1", until
 * "n" characters or the wchar_t type NULL character have been
 * processed.
 */

#include <widec.h>
#include <errno.h>
#include <sys/euc.h>
#include "pcode.h"

#ifdef _WCHAR16
# define MY_EUCMASK	H_EUCMASK
# define MY_P11		H_P11
# define MY_P01		H_P01
# define MY_P10		H_P10
# define MY_SHIFT	8
#else
# define MY_EUCMASK	EUCMASK
# define MY_P11		P11
# define MY_P01		P01
# define MY_P10		P10
# define MY_SHIFT	7
#endif

char *
_wsntostr(s1, s2, n, p)
	register char *s1;
	wchar_t *s2;
	int n;
	wchar_t **p;
{
	register char *s;
	register wchar_t wchar;
	register int size, length;
	int cs1;
	char *os1 = s1;

	if (n > 0 && (wchar = *s2) != 0)
	{
		do
		{
			switch (wchar & MY_EUCMASK)
			{
			default:
				if (wchar >= 0400 || multibyte && wchar >= 0240)
				{
bad_seq:
					if (p != 0)
						*p = s2;
					*s1 = '\0';
					errno = EILSEQ;
					return 0;
				}
				*s1++ = wchar;
				n--;
				continue;
			case MY_P11:
				if ((size = eucw1) == 0)
					goto bad_seq;
				length = size;
				cs1 = 1;
				break;
			case MY_P01:
#ifdef _WCHAR16
				if (wchar < 0240 || !multibyte && wchar < 0400)
				{
					*s1++ = wchar;
					n--;
					continue;
				}
#endif
				if ((size = eucw2) == 0)
					goto bad_seq;
				*s1 = SS2;
				length = size + 1;
				cs1 = 0;
				break;
			case MY_P10:
				if ((size = eucw3) == 0)
					goto bad_seq;
				*s1 = SS3;
				length = size + 1;
				cs1 = 0;
				break;
			}
			if ((n -= length) < 0)	/* not enough room */
			{
				*s1 = '\0';
				break;
			}
			s = s1 + length;	/* fill in backwards */
			do
			{
				*--s = (wchar | 0200) & 0377;
				wchar >>= MY_SHIFT;
			} while (--size != 0);
			if (cs1 && (unsigned char)*s < 0240)	/* C1 not first */
				goto bad_seq;
			s1 += length;
		} while ((wchar = *++s2) != 0 && n > 0);
	}
	if (p != 0)
		*p = s2;
	return os1;
}
