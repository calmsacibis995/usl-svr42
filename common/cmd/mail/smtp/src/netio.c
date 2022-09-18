/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/netio.c	1.6.2.5"
#ident "@(#)netio.c	1.8 'attmail mail(1) command'"

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "smtp.h"
#include "miscerrs.h"
#ifdef PRIV
# include <sys/types.h>
# include <priv.h>
#endif
#ifdef SVR4_1
#include <pfmt.h>
#include <errno.h>
#endif

#ifdef NOBOMB
#define bomb exit
#endif

#ifdef _STDC__
extern void bomb(int);
#else
extern void bomb();
#endif

extern int debug;
extern char *progname;

int
tgets(line, size, fi)		/* fgets from TCP */
char *line;
int size;
FILE *fi;
{
	register char *cr, *rp;

	*line = 0;
#ifdef PRIV
	procprivl(SETPRV, DEV_W, (priv_t)0);
	rp = fgets(line, size, fi);
	procprivl(CLRPRV, DEV_W, (priv_t)0);
#else
	rp = fgets(line, size, fi);
#endif
	if (ferror(fi) || rp==NULL) {
#ifdef SVR4_1
		pfmt(stderr, MM_ERROR, ":54:error reading from smtp: %s\n", strerror(errno));
#else
		perror("error reading from smtp");
#endif
		bomb(E_IOERR);
	}

	/* convert \r\n -> \n */
	cr = line + strlen(line) - 2;
	if (cr >= line && *cr == '\r' && *(cr+1) == '\n') {	/* CRLF there? */
		*cr++ = '\n';
		*cr = 0;
	} else				/* no CRLF present */
		cr += 2;		/* point at NUL byte */

	if(debug)
		(void) fprintf(stderr, "<<< %s", line);
	if (feof(fi)) {
#ifdef SVR4_1
                pfmt(stderr, MM_ERROR, ":55:read eof from smtp: %s\n", strerror(errno));
#else
		perror("read eof from smtp");
#endif
		bomb(E_IOERR);
	}
	return cr - line;
}

int
tputs(line, fo)			/* fputs to TCP */
char *line;
FILE *fo;
{
	char buf[MAXSTR];
	register char *nl;
	extern int debug;

	(void) strcpy(buf, line);
	if(debug)
		(void) fprintf(stderr, ">>> %s", buf);
	/* replace terminating \n by \r\n */
	nl = buf + strlen(buf) - 1;		/* presumably \n */
	if (nl >= buf && *nl=='\n') {		/* if it is ... */
		*nl++ = '\r';
		*nl++ = '\n';
		*nl = 0;
	} /* else
		printf("%s: unterminated line: <%s>\n", progname, buf); */

#ifdef PRIV
	procprivl(SETPRV, DEV_W, (priv_t)0);
	(void) fputs(buf, fo);
	(void) fflush(fo);
	procprivl(CLRPRV, DEV_W, (priv_t)0);
#else
	(void) fputs(buf, fo);
	(void) fflush(fo);
#endif
	if (ferror(fo)) {
#ifdef SVR4_1
                pfmt(stderr, MM_ERROR, ":56:error writing to smtp: %s\n", strerror(errno));
#else
		perror("error writing to smtp");
#endif
		bomb(E_IOERR);
	}
	return 0;
}
