/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/abspath.c	1.5.2.2"
#ident "@(#)abspath.c	1.6 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	abspath - expand a path relative to some `.'

    SYNOPSIS
	string *abspath(const char *path, const char *dot, string *to)

    DESCRIPTION
	If "path" is relative (does not start with `.'), the
	the value of "dot" will be prepended and the result
	returned in "to". Otherwise, the value of "path" is
	returned in "to".
*/

extern string *
abspath(path, dot, to)
	const char *path;
	const char *dot;
	string *to;
{
	return (*path == '/') ? s_append(to, path) : s_xappend(to, dot, path, (char*)0);
}
