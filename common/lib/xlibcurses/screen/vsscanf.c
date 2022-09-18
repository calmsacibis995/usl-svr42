/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/vsscanf.c	1.7.2.2"
#ident  "$Header: vsscanf.c 1.2 91/06/27 $"

#include "curses_inc.h"

#ifdef __STDC__
#include	<stdarg.h>
#else
#include <varargs.h>
#endif

/*
 *	This routine implements vsscanf (nonportably) until such time
 *	as one is available in the system (if ever).
 */

/*VARARGS2*/
vsscanf(buf, fmt, ap)
char	*buf;
char	*fmt;
va_list	ap;
{
	FILE	junk;
	extern int _doscan();

#ifdef SYSV
	junk._flag = _IOREAD;
	junk._file = -1;
	junk._base = junk._ptr = (unsigned char *) buf;
#else
	junk._flag = _IOREAD|_IOSTRG;
	junk._base = junk._ptr = buf;
#endif
	junk._cnt = strlen(buf);
	return _doscan(&junk, fmt, ap);
}
