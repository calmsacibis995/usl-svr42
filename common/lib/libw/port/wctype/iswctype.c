/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wctype/iswctype.c	1.1.1.2"
#ident  "$Header: iswctype.c 1.2 91/06/26 $"

/*
	_iswctype(c,x) returns true if 'c' is classified x
*/

#include <widec.h>
#include "_wchar.h"
#include <ctype.h>
#include <wctype.h>
#include <locale.h>
#include "_locale.h"

extern	int	_lflag;
extern	struct	_wctype *_wcptr[];

unsigned _iswctype(c,x)
register wchar_t c;
register int	x;
{
	register int s;
	int i,size;
	wchar_t w;
	static char save_locale[LC_NAMELEN];

	if (_lflag == 0) {
		strcpy(save_locale, _cur_locale[LC_CTYPE]);
		_loadtab();
	} else if (strcmp(save_locale, _cur_locale[LC_CTYPE]) != 0)
		_loadtab();
	if (iscodeset1(c)) {
		s = 0;
		size = eucw1;	/* length of code set 1 */
	} else if (iscodeset2(c)) {
		s = 1;
		size = eucw2;	/* length of code set 2 */
	} else if (iscodeset3(c)) {
		s = 2;
		size = eucw3;	/* length of code set 3 */
	} else
		return(0);
	/*
	 *	pick up the 7 bits data from c
	 */
#if !defined(_WCHAR16)
	c &= 0x001FFFFF; /* under 21 bit mask */
#else
	for (i = 0, w = 0; size--; i++)
		w |= ((c & (0x7f << (8 * i))) >> (1 * i));	
	c = w;
#endif
	if (_wcptr[s] == 0 || _wcptr[s]->index == 0 ||
	    c < _wcptr[s]->tmin || c > _wcptr[s]->tmax)
		return(0);
	return(x & _wcptr[s]->type[_wcptr[s]->index[c - _wcptr[s]->tmin]]);
}
