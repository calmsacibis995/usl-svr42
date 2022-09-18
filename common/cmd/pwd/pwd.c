/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)pwd:pwd.c	1.14.1.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/pwd/pwd.c,v 1.1 91/02/28 19:27:49 ccs Exp $"
/*
**	Print working (current) directory
*/

#include	<stdio.h>
#include	<unistd.h>
#include	<limits.h>
#include	<locale.h>
#include	<pfmt.h>

char	name[PATH_MAX+1];

main()
{
	int length;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:pwd");

	if (getcwd(name, PATH_MAX + 1) == (char *)0) {
		pfmt(stderr, MM_ERROR, ":129:Cannot determine current directory");
		putc('\n', stderr);
		exit(2);
	}
	length = strlen(name);
	name[length] = '\n';
	write(1, name, length + 1);
	exit(0);
}
