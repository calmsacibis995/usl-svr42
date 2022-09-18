/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstdio/putwchar.c	1.1.1.2"
#ident  "$Header: putwchar.c 1.2 91/06/26 $"

/*
 * A subroutine version of the macro putchar
 */
#include <stdio.h>
#include <widec.h>
#undef putwchar

int
putwchar(c)
register c;
{
	return(putwc(c, stdout));
}
