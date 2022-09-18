/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XDisName.c	1.1"
/* $XConsortium: XDisName.c,v 11.6 91/05/11 17:03:04 gildea Exp $ */

/*
*/

/* XDisplayName.c */
/* 
 * Returns the name of the display XOpenDisplay would use.  This is better
 * than just printing the "display" variable in a program because that
 * could be NULL and/or there could be an environment variable set.
 * This makes it easier for programmers to provide meaningful error
 * messages. 
 *
 * 
 * For example, this is used in XOpenDisplay() as
 *	strncpy( displaybuf, XDisplayName( display ), sizeof(displaybuf) );
 *      if ( *displaybuf == '\0' ) return( NULL );
 *  This check is actually unnecessary because the next thing is an index()
 *  call looking for a ':' which will fail and we'll return(NULL).
 */
/* Written at Waterloo - JMSellens */

#include <stdio.h>

extern char *getenv();

char *
XDisplayName( display )
    char *display;
{
    char *d;
    if ( display != (char *)NULL && *display != '\0' )
	return( display );
    if ( (d = getenv( "DISPLAY" )) != (char *)NULL )
	return( d );
    return( "" );
}
