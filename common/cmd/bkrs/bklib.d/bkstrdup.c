/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bkstrdup.c	1.2.5.2"
#ident  "$Header: bkstrdup.c 1.2 91/06/21 $"

#include	<string.h>

char *
bkstrdup( s1 )
char *s1;
{
	return( s1? strdup(s1): (char *) 0 );
}
