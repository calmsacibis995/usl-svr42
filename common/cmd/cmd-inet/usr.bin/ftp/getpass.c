/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/ftp/getpass.c	1.4.9.1"
#ident  "$Header: getpass.c 1.3 91/09/19 $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <stdio.h>
#include <signal.h>
#include <termio.h>
#include <sys/ttold.h>
#include <sys/types.h>
#include <sys/stropts.h>

static	struct termios ttyatt;
static	tcflag_t lflag;
static	FILE *fi;

#ifdef SYSV
#define signal(s,f)	sigset(s,f)
#endif /* SYSV */

static void
intfix()
{
	ttyatt.c_lflag = lflag;
	if (fi != NULL) {
	        if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1)
                        perror("ftp: tcsetattr");
        }
	exit(SIGINT);
}

char *
mygetpass(prompt)
char *prompt;
{
	register char *p;
	register int c;
	static char pbuf[50+1];
	void (*sig)();

	if ((fi = fopen("/dev/tty", "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);

	sig = signal(SIGINT, (void (*)())intfix);

	if (tcgetattr(fileno(fi), &ttyatt) == -1)
		perror("ftp: tcgetattr");	/* go ahead, anyway */
	lflag = ttyatt.c_lflag;
	ttyatt.c_lflag &= ~ECHO;
	if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1)
                perror("ftp: tcsetattr");
	fprintf(stderr, "%s", prompt); (void) fflush(stderr);
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (p < &pbuf[sizeof(pbuf)-1])
			*p++ = c;
	}
	*p = '\0';
	fprintf(stderr, "\n"); (void) fflush(stderr);
	ttyatt.c_lflag = lflag;
	if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1)
                perror("ftp: tcsetattr");
	(void) signal(SIGINT, sig);

	if (fi != stdin)
		(void) fclose(fi);
	return(pbuf);
}
