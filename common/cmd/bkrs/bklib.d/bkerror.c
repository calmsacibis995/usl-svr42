/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bkerror.c	1.1.5.2"
#ident  "$Header: bkerror.c 1.2 91/06/21 $"

#include	<stdio.h>
#include	<varargs.h>

extern	char	*errmsgs[];
extern	int	lasterrmsg;
extern	char	*brcmdname;

/*VARARGS*/
void
bkerror( va_alist )
va_dcl
{
	va_list	args;
	FILE	*fptr;
	int	msgid;
	va_start( args );
	fptr = va_arg( args, FILE *);
	msgid = va_arg( args, int );
	if( msgid < lasterrmsg ) {
		(void) fprintf( fptr, "%s: ", brcmdname );
		(void) vfprintf( fptr, errmsgs[ msgid ], args );
	}
	va_end( args );
}
