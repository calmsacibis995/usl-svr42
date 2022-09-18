/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/errmsg.c	1.1.13.2"
#ident  "$Header: errmsg.c 2.0 91/07/13 $"

#include	<stdio.h>
#include	<varargs.h>
#include	<pfmt.h>

extern	char	*errmsgs[];
extern	int	lasterrmsg;
extern	char	*msg_label;

/*
	synopsis: errmsg( msgid, (arg1, ..., argN) )
*/

/*VARARGS*/
void
errmsg(va_alist)
va_dcl
{
	va_list	args;
	int	msgid;

	setlabel(msg_label);

	va_start(args);

	msgid = va_arg(args, int);

	if (msgid >= 0 && msgid < lasterrmsg) {
		(void) vpfmt(stderr, MM_ERROR, errmsgs[msgid], args);
	}

	va_end(args);
}
