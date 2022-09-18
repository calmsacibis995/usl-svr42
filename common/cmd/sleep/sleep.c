/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sleep:sleep.c	1.3.1.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sleep/sleep.c,v 1.1 91/02/28 20:09:47 ccs Exp $"
/*
**	sleep -- suspend execution for an interval
**
**		sleep time
*/

#include	<stdio.h>
#include	<locale.h>
#include	<pfmt.h>

main(argc, argv)
char **argv;
{
	int	c, n;
	char	*s;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:sleep");

	n = 0;
	if(argc < 2) {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":564:Usage: sleep time\n");
		exit(2);
	}
	s = argv[1];
	while(c = *s++) {
		if(c<'0' || c>'9') {
			pfmt(stderr, MM_ERROR, ":565:Bad character in argument\n");
			exit(2);
		}
		n = n*10 + c - '0';
	}
	sleep(n);
	exit(0);
}
