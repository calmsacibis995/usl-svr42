#ident	"@(#)r5fontlib:font/fonttime.c	1.1"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <sys/time.h>

long
GetFontTime()
{
    struct timeval tp;

    gettimeofday(&tp, 0);
    return ((tp.tv_sec * 1000) + (tp.tv_usec / 1000));
}

