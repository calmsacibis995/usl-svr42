/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mkdate.c	1.1.2.2"
#ident "@(#)mkdate.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	mkdate - generate a date string of the proper format

    SYNOPSIS
	void mkdate(char datestring[60])

    DESCRIPTION
	mkdate() calls time() to get the current time, then
	formats it properly for the header of a message.
*/

/* not everyone declares this properly */
extern char *tzname[2];

void mkdate(datestring)
char *datestring;
{
    long	ltmp;		/* temp variable for time() */
    char	*tp, *zp;
    struct tm	*bp;
    /* asctime: Fri Sep 30 00:00:00 1986\n */
    /*          0123456789012345678901234  */
    /* date:    Fri Sep 30 00:00 EDT 1986  */

    time(&ltmp);
    bp = localtime(&ltmp);
    tp = asctime(bp);
    zp = tzname[bp->tm_isdst];
    sprintf(datestring, "%.16s %.3s %.4s", tp, zp, tp+20);
}
