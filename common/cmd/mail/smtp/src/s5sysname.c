/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/s5sysname.c	1.4.2.2"
#ident "@(#)s5sysname.c	1.4 'attmail mail(1) command'"
/* get the system's name -- System V */

#include <sys/utsname.h>

extern char *
sysname_read()
{
	static struct utsname s;

	if (uname(&s)<0)
		return "";
	return s.nodename;
}
