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

#ident	"@(#)nohup:nohup.c	1.5.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/nohup/nohup.c,v 1.1 91/02/28 18:41:33 ccs Exp $"

#include	<stdio.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>

char	nout[100] = "nohup.out";
char	*getenv();

static char badopen[] = ":3:Cannot open %s: %s\n";

main(argc, argv)
char **argv;
{
	char	*home;
	FILE *temp;
	int	err;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue");
	(void)setlabel("UX:nohup");

	if(argc < 2) {
		pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":4:Usage: nohup command arg ...\n");
		exit(1);
	}
	argv[argc] = 0;
	signal(1, 1);
	signal(3, 1);
	if(isatty(1)) {
		if( (temp = fopen(nout, "a")) == NULL) {
			if((home=getenv("HOME")) == NULL) {
				pfmt(stderr, MM_ERROR, badopen, nout,
					strerror(errno));
				exit(1);
			}
			strcpy(nout,home);
			strcat(nout,"/nohup.out");
			if(freopen(nout, "a", stdout) == NULL) {
				pfmt(stderr, MM_ERROR, badopen, nout,
					strerror(errno));
				exit(1);
			}
		}
		else {
			fclose(temp);
			freopen(nout, "a", stdout);
		}
		pfmt(stderr, MM_INFO, ":5:Sending output to %s\n", nout);
	}
	if(isatty(2)) {
		close(2);
		dup(1);
	}
	execvp(argv[1], &argv[1]);
	err = errno;

/* It failed, so print an error */
	freopen("/dev/tty", "w", stderr);
	pfmt(stderr, MM_ERROR, ":6:%s: %s\n", argv[1], strerror(err)); 
	exit(1);
}
