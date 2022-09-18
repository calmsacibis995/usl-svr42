/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bkst_today.c	1.1.5.2"
#ident  "$Header: bkst_today.c 1.2 91/06/21 $"

#include <sys/types.h>
#include <time.h>

/* Compute the date for the history table - midnight today */
bkst_today()
{
	long current = time( 0 );
	struct tm	*current_tm;
	current_tm = localtime( &current );
	current -= current_tm->tm_sec + current_tm->tm_min * 60 
		+ current_tm->tm_hour * 60 * 60;
	return( current );
}
