/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/converse.c	1.6.2.2"
#ident "@(#)converse.c	1.11 'attmail mail(1) command'"
/*
 * Do the necessary commands for a smtp transfer.  Start by waiting for the
 * connection to open, then send HELO, MAIL, RCPT, and DATA.  Check the
 * reply codes and give up if needed.
 * 
 * This code modified from the MIT UNIX TCP implementation:
 * Copyright 1984 Massachusetts Institute of Technology
 * 
 * Permission to use, copy, modify, and distribute this file
 * for any purpose and without fee is hereby granted, provided
 * that this copyright and permission notice appear on all copies
 * and supporting documentation, the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * program without specific prior permission, and notice be given
 * in supporting documentation that copying and distribution is
 * by permission of M.I.T.  M.I.T. makes no representations about
 * the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include "xmail.h"
#include "smtp.h"
#include "s_string.h"
#include "smtp_decl.h"
#include "miscerrs.h"
#ifdef SVR4_1
#include <pfmt.h>
#include <errno.h>
#endif

#define TRUE 1
#define FALSE 0
#define MINUTES * 60
#define HOURS * 60 MINUTES
#define DAYS * 24 HOURS

#ifdef	BSD
char *sprintf();
#else	/*SV(ID)*/
#ifndef SVR4
int sprintf();
#endif
#endif

char	*timemsg;
int	timelim;
extern int batching;
extern int debug;
extern int catch_bombs;
extern char *progname;
extern void bad_input_file();

void converse(unixformat, from, rcpts, domain, sfi, sfo, mlfd)
char	*from;				/* from address */
namelist *rcpts;			/* to addresses */
char	*domain;
FILE	*sfi;				/* smtp input */
FILE	*sfo;				/* smtp output */
FILE	*mlfd;				/* mail file descriptor */
{
	extern char *helohost;
	extern jmp_buf bomb_jmp;
	char buf[MAXSTR];
	namelist *np;
	int mailfailed;

	(void) signal(SIGALRM, death);

	setalarm(5 MINUTES, ":37:timer (%d sec) expired: initial handshake.\n");
	expect(220, sfi, sfo);			/* expect a service ready msg */

	(void) sprintf(buf, "HELO %s\n", helohost);
	tputs(buf, sfo);
	expect(250, sfi, sfo);			/* expect an OK */

	while (from != NULL) {
		mailfailed = 0;	/* Set if longjmp() called */
		if (batching)
			catch_bombs = 1;
		if (setjmp(bomb_jmp) == 0) {
			(void) strcpy(buf, "MAIL FROM:<");
			(void) strcat(buf, from);
			(void) strcat(buf, ">\n");
			tputs(buf, sfo);
			setalarm(10 MINUTES, ":38:timer (%d sec) expired: response to MAIL FROM/RCPT TO.\n");
			expect(250, sfi, sfo);			/* expect OK */

			for (np = rcpts; np != NULL; np = np->next) {
				(void) strcpy(buf, "RCPT TO:<");
				(void) strcat(buf, np->name);
				(void) strcat(buf, ">\n");
				tputs(buf, sfo);
				expect(250, sfi, sfo);		/* expect OK */
				}
			setalarm(10 MINUTES, ":39:timer (%d sec) expired: response to DATA.\n");
			tputs("DATA\n", sfo);
			expect(354, sfi, sfo);
			setalarm(10 MINUTES, ":40:timer (%d sec) expired: sending mail data.\n");
			do_data(unixformat, mlfd, sfo, from, rcpts, domain);
			setalarm(1 HOURS, ":41:timer (%d sec) expired: expecting delivery ack.\n");
			expect(250, sfi, sfo);
		} else
			mailfailed = 1;	/* bomb() was called */

		from = NULL;
		catch_bombs = 0;
		if (batching) {
			char *junk;

			/* Handle current file */
			setalarm(0, "");
			/* Don't remove this file if bomb() was called */
			if (mailfailed == 0)
				rm_file();

			/* Send RSET */
			setalarm(5 MINUTES, ":42:timer (%d sec) expired: response to RSET.\n");
			tputs("RSET\n", sfo);
			expect(250, sfi, sfo);

			/* Setup new control file */
			donext(&unixformat, &from, &rcpts, &domain, mlfd, &junk, &junk);
		}
	}

	tputs("QUIT\n", sfo);
	setalarm(5 MINUTES, ":43:timer (%d sec) expired: response to QUIT.\n");
	expect(221, sfi, sfo);			/* who cares? (Some do -ches)*/
}

/*
 *  escape '.'s at the beginning of lines and turn newlines into
 *  \r\n's.
 */
static char smlastchar;

smfputs(str, fp)
	char *str;
	FILE *fp;
{
	register char *cp;

	/*
	 *  escape a leading dot
	 */
	if(smlastchar=='\n' && str[0]=='.') {
		fputc('.', fp);
		if (debug>1)
			(void) putc('.', stderr);
	}

	/*
	 *  output the line
	 */
	for(cp=str; *cp; cp++){
		if(*cp=='\n')
			putc('\r', fp);
		putc(*cp, fp);
	}
	if(cp!=str)
		smlastchar = *(cp-1);
}


/*
 * Send the data from the specified mail file out on the current smtp
 * connection.  Do the appropriate netascii conversion and hidden '.'
 * padding.  Send the <CRLF>.<CRLF> at completion.
 */
void do_data(unixformat, sfi, sfo, from, rcpts, domain)
	register FILE *sfi;		/* mail file descriptor */
	register FILE *sfo;		/* smtp files */
	char *from;
	namelist *rcpts;
	char *domain;
{
	static string *rcvr;
	char buf[4096];
	namelist *p;
	long nchars;

	/*
	 *  turn rcpts into a , list of receivers
	 */
	rcvr = s_reset(rcvr);
	for(p = rcpts; p; p = p->next){
		s_append(rcvr, p->name);
		if(p->next)
			s_append(rcvr, ", ");
	}

	/*
	 *  send data to output
	 */
	setalarm(5 MINUTES, ":44:timer (%d sec) expired: start sending mail data.\n");
	smlastchar = '\n';
	if(unixformat){
		nchars = 0;
		while(fgets(buf, sizeof(buf), sfi)!=NULL) {
			smfputs(buf, sfo);
			nchars += strlen(buf)+1;
			if (nchars>1024) {
				nchars -= 1024;
				setalarm(5 MINUTES, ":40:timer (%d sec) expired: sending mail data.\n");
				if (debug)
					putc('.', stderr);
			}
		}
	} else {
		if(to822(smfputs, sfi, sfo, from, domain, s_to_c(rcvr))<0){
#ifdef SVR4_1
			(void) pfmt(stderr, MM_ERROR, ":1:bad input file\n");
#else
			(void) fprintf(stderr, "%s: bad input file\n", progname);
#endif
			bad_input_file();
			/* Unfortunately, there is no recovering from this */
			/* as SMTP does not provide a way to abort a message */
			/* After the DATA command has been issued */
			catch_bombs = 0;
			bomb(E_DATAERR);
		}
	}

	/*
	 *  terminate the DATA command with \r\n.\r\n
	 */
	if(smlastchar != '\n'){
		fputs("\r\n", sfo);
		if(debug)
			fputs("\n", stderr);
	}
	fputs(".\r\n", sfo);

	/*
	 *  see if we screwed up
	 */
	setalarm(30 MINUTES, ":46:timer (%d sec) expired: finishing data.\n");
	fflush(sfo);
	if (ferror(sfo)) {
#ifdef SVR4_1
		pfmt(stderr, MM_ERROR, ":47:write error in smtp: %s\n", strerror(errno));
#else
		perror("write error in smtp");
#endif
		bomb(E_IOERR);
	}
	if(debug)
		fputs("\nFinished sending.\n", stderr);
}

/*
 * Expect a reply message with the specified code.  If the specified code
 * is received return TRUE; otherwise print the error message out on the
 * standard output and give up.  Note that the reply can be a multiline
 * message.
 */
void expect(code, sfi, sfo)
int	code;
FILE	*sfi, *sfo;
{
	int retcd;
	char cmdbuf[BUFSIZ];

	if (debug)
		(void) fprintf(stderr, "expect %d ", code);
	/* get whole reply */
more:
	while (tgets(cmdbuf, sizeof cmdbuf, sfi) > 0) {	/* get input line */
		if (cmdbuf[3] != '-')	/* continuation line? */
			break;		/* no, last line */
	}
	if (sscanf(cmdbuf, "%d", &retcd) !=1 ){
		int l=strlen(cmdbuf)-1;
		if (l>=0 && cmdbuf[l]=='\n')
			cmdbuf[l]='\0';
		if (debug)
			(void) fprintf(stderr, "non-numeric command reply (%s)\n", cmdbuf);
		goto more;
	}
	if (retcd == code) {
		if (debug)
			(void) fprintf(stderr, "got it\n");
		return;
	}
	if (retcd/100 == code/100) {
		if (debug)
			(void) fprintf(stderr, "got it close enough (%d)\n", retcd);
		return;
	}
	if (debug)
		(void) fprintf(stderr, "FAIL (got %d)\n", retcd);
	/* print the error line */
#ifdef SVR4_1
	(void) pfmt(stderr, MM_ERROR, ":2:<<< %s", cmdbuf);
#else
	(void) fprintf(stderr, "%s: <<< %s", progname, cmdbuf);
#endif
	tputs ("QUIT\n", sfo);
	bomb(retcd);		/* map smtp errors to mailsys errors */
}

void setalarm(limit, message)
	char *message;
{
	timelim = limit;
	timemsg = message;
	alarm(limit);
}

/* Maximum time to live elapsed.  Die right now. */
SIGRETURN
death(unused)
int unused;
{
#ifdef SVR4_1
	(void) pfmt(stderr, MM_INFO, timemsg, timelim);
#else
	/* Skip over message number (only used in SVR4ES) */
	for (timemsg += 2; timemsg[0] != ':'; )
		timemsg++;
	timemsg++;
	(void) fprintf(stderr, "%s: ", progname);
	(void) fprintf(stderr, timemsg, timelim);
#endif
	exit(1);
}
