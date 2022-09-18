/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/loopfork.c	1.1.2.2"
#ident "@(#)loopfork.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	loopfork - looping version of fork(2)

    SYNOPSIS
	int loopfork()

    DESCRIPTION
	Do a fork(2), looping on errors with a sleep
	in between.
*/

pid_t loopfork()
{
    pid_t p;
    unsigned int count = 0;
    for (count = 0; (p = fork()) == (pid_t)-1; count++)
	if (count == 40)
	    return p;
	else
	    sleep(1 + count / 10);
    return p;
}
