/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/dup2.c	1.4.2.2"
#ident "@(#)dup2.c	1.3 'attmail mail(1) command'"
#include <fcntl.h>
#include <unistd.h>

int
dup2(a,b)
	int a;
	int b;
{
	if (a==b)
		return 0;
	close(b);
	return fcntl(a, F_DUPFD, b);
}
