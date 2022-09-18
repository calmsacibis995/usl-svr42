/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstdio/fputwc.c	1.1.2.2"
#ident  "$Header: fputwc.c 1.2 91/06/26 $"

/*
 * Fputwc transforms the process code c into the EUC,
 * and writes it onto the output stream "iop".
 */
#include <limits.h>
#include <widec.h>
#include <stdio.h>
#include <errno.h>
#include <sys/euc.h>
#include "pcode.h"
#define putbyte(x,p)	{ if (putc((x),(p)) == EOF) return(EOF); }

extern int  _flsbuf();

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

int
fputwc(c, iop)
	register wchar_t c;
	register FILE *iop;
{
	char ans[MB_LEN_MAX + 1];
	register int length, n;
	register char *s;
	int wchar, cs1;

	switch (c & MY_EUCMASK)
	{
	default:
		if (c >= 0400 || multibyte && c >= 0240)
		{
bad_seq:
			errno = EILSEQ;
			return EOF;
		}
		putbyte(c, iop);
		return c;
	case MY_P11:
		if ((n = eucw1) == 0)
			goto bad_seq;
		length = n;
		cs1 = 1;
		break;
	case MY_P01:
#ifdef _WCHAR16
		if (c < 0240 || !multibyte && c < 0400)
		{
			putbyte(c, iop);
			return c;
		}
#endif
		if ((n = eucw2) == 0)
			goto bad_seq;
		ans[0] = SS2;
		length = n + 1;
		cs1 = 0;
		break;
	case MY_P10:
		if ((n = eucw3) == 0)
			goto bad_seq;
		ans[0] = SS3;
		length = n + 1;
		cs1 = 0;
		break;
	}
	wchar = c;
	s = &ans[length];	/* fill in array backwards */
	*s = '\0';
	do
	{
		*--s = (c | 0200) & 0377;	/* hope "& 0377" is tossed */
		c >>= MY_SHIFT;
	} while (--n != 0);
	if (cs1 && (unsigned char)*s < 0240)	/* C1 byte cannot be first */
		goto bad_seq;
	s = ans;
	do
	{
		putbyte(*s, iop);	/* output string byte-by-byte */
	} while (*++s != '\0');
	return wchar;
}
