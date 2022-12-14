/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dispadmin:common/cmd/dispadmin/subr.c	1.5.3.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/dispadmin/subr.c,v 1.1 91/02/28 16:55:39 ccs Exp $"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/hrtcntl.h>


/*
 * Utility functions for dispadmin command.
 */


/* VARARGS1 */
void
fatalerr(format, a1, a2, a3, a4, a5)
char	*format;
{
	fprintf(stderr, format, a1, a2, a3, a4, a5);
	exit(1);
}


/*
 * hrtconvert() returns the interval specified by htp as a single
 * value in resolution htp->hrt_res.  Returns -1 on overflow.
 */
long
hrtconvert(htp)
register hrtime_t	*htp;
{
	register long	sum;
	register long	product;

	product = htp->hrt_secs * htp->hrt_res;

	if (product / htp->hrt_res == htp->hrt_secs) {
		sum = product + htp->hrt_rem;
		if (sum - htp->hrt_rem == product) {
			return(sum);
		}
	}
	return(-1);
}
