/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/V3.vidputs.c	1.3.2.3"
#ident  "$Header: V3.vidputs.c 1.2 91/06/26 $"

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
#undef	vidputs
vidputs(a, o)
_ochtype	a;
#ifdef __STDC__
int		(*o)(int);
#else
int		(*o)();
#endif
{
    vidupdate(_FROM_OCHTYPE(a), cur_term->sgr_mode, o);
    return (OK);
}
#endif	/* _VR3_COMPAT_CODE */
