/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wctype/trwctype.c	1.1.2.2"
#ident  "$Header: trwctype.c 1.2 91/06/26 $"

/*
	_tuwctype(c) converts lower-case character to upper-case 
*/

#include <widec.h>
#include "_wchar.h"
#include <ctype.h>
#include <wctype.h>
#include <locale.h>
#include "_locale.h"

extern	int	_lflag;
extern	struct	_wctype *_wcptr[];

wchar_t	_trwctype(c,x)
register wchar_t c;
{
	register int s;
	register wchar_t w;
	int i, size;
	wchar_t wchar;
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
		return(c);
	/*
	 *	pick up the 7 bits data from c
	 */
#if !defined(_WCHAR16)
	w = c & 0x001FFFFF; /* under 21 bit mask */
#else
	for (i = 0, wchar = 0; size--; i++)
		wchar |= ((c & (0x7f << (8 * i))) >> (1 * i));	
	w = wchar;
#endif
	if (_wcptr[s] == 0 || _wcptr[s]->code == 0 ||
	    w < _wcptr[s]->cmin || w > _wcptr[s]->cmax)
		return(c);
	if( ! _iswctype(c,x) ) return(c);
	w =  _wcptr[s]->code[w - _wcptr[s]->cmin]; /* fetch value */
#if !defined(_WCHAR16)
	return( w | (s==0?P11:s==1?P01:P10));   /* conv to 32-bit PC */
#else                                                             
	w = (w & 0x7f) | ((w & 0x3f80) << 1);    /* conv to 16-bit PC */
	return( w | (s==0 ? H_P11 : s==1 ? H_P01 : H_P10));
#endif
}

