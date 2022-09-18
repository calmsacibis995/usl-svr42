/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)curses:common/lib/xlibcurses/screen/mbstowcs.c	1.2.2.2"
#ident  "$Header: mbstowcs.c 1.2 91/06/26 $"
/*LINTLIBRARY*/

#include <widec.h>
#include "synonyms.h"
#include <stdlib.h>

size_t
_curs_mbstowcs(pwcs, s, n)
wchar_t	*pwcs;
const char *s;
size_t n;
{
	int	i, val;

	for (i = 0; i < n; i++) {
		if ((val = _curs_mbtowc(pwcs++, s, MB_CUR_MAX)) == -1)
			return(val);
		if (val == 0)
			break;
		s += val;
	}
	return(i);
}
