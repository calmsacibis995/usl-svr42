/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:collect.c	1.20.2.9"
#ident "@(#)collect.c	1.22 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Collect input from standard input, handling
 * ~ escapes.
 */

#include "rcv.h"

#ifdef SIGCONT
static void	collcont ARGS((int));
static void	(*sigrset ARGS((int,void (*)(int))))ARGS((int));
#endif
static void	collrub ARGS((int));
static void	cpout ARGS((char *, FILE *));
static int	exwrite ARGS((char[], FILE *));
static int	forward ARGS((char[], FILE *, int));
static int	forwardmsgs ARGS((int*, FILE *, int, int));
static void	intack ARGS((int));
static FILE	*mesedit ARGS((FILE *, FILE *, int, struct header *));
static FILE	*mespipe ARGS((FILE *, FILE *, char[]));
static void	resetsigs ARGS((int));
static int	stripnulls ARGS((char *, int));
static void	xhalt ARGS((void));
static char	**Xaddone ARGS((char **, char[]));

/*
 * Read a message from standard output and return a read file to it
 * or NULL on error.
 */

/*
 * The following hokiness with global variables is so that on
 * receipt of an interrupt signal, the partial message can be salted
 * away on dead.letter.  The output file must be available to flush,
 * and the input to read.  Several open files could be saved all through
 * mailx if stdio allowed simultaneous read/write access.
 */

static void		(*savesig)();	/* Previous SIGINT value */
static void		(*savehup)();	/* Previous SIGHUP value */
#ifdef SIGCONT
static void		(*savecont)();	/* Previous SIGCONT value */
#endif
static FILE		*newi;		/* File for saving away */
static FILE		*newo;		/* Output side of same */
static int		ig_ints;	/* Ignore interrupts */
static int		hadintr;	/* Have seen one SIGINT so far */
static struct header	*savehp;
static jmp_buf		coljmp;		/* To get back to work */

static FILE *
readmailfromfile(fbuf, hp, zapheaders)
	FILE *fbuf;
	struct header *hp;
	int zapheaders;
{
	FILE *ibuf, *obuf;
	char hdr[LINESIZE];

	if (zapheaders) {
		hp->h_to = hp->h_subject = hp->h_cc = hp->h_bcc = hp->h_defopt = hp->h_encodingtype = NOSTR;
		hp->h_others = NOSTRPTR;
		hp->h_seq = 0;
	}
	while (gethfield(fbuf, hdr, 9999L) > 0) {
		if (ishfield(hdr, "to"))
			hp->h_to = addto(hp->h_to, hcontents(hdr));
		else if (ishfield(hdr, "subject"))
			hp->h_subject = addone(hp->h_subject, hcontents(hdr));
		else if (ishfield(hdr, "cc"))
			hp->h_cc = addto(hp->h_cc, hcontents(hdr));
		else if (ishfield(hdr, "bcc"))
			hp->h_bcc = addto(hp->h_bcc, hcontents(hdr));
		else if (ishfield(hdr, "default-options"))
			hp->h_defopt = addone(hp->h_defopt, hcontents(hdr));
		else if (ishfield(hdr, "encoding-type"))
			hp->h_encodingtype = addone(hp->h_encodingtype, hcontents(hdr));
		else if (ishfield(hdr, "from"))
			assign("postmark", hcontents(hdr));
		else if (ishfield(hdr, "content-length"))
			/* EMPTY */;
		else
			hp->h_others = Xaddone(hp->h_others, hdr);
		hp->h_seq++;
	}
	if ((obuf = fopen(tempMail, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, strerror(errno));
		fclose(fbuf);
		return (FILE*)NULL;
	}
	if ((ibuf = fopen(tempMail, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, strerror(errno));
		removefile(tempMail);
		fclose(fbuf);
		fclose(obuf);
		return (FILE*)NULL;
	}
	removefile(tempMail);
	if (strlen(hdr) > 0) {
		fputs(hdr, obuf);
		putc('\n', obuf);
	}
	copystream(fbuf, obuf);
	fclose(newo);
	fclose(newi);
	newo = obuf;
	newi = ibuf;
	return newo;
}

FILE *
collect(hp, forwardvec)
	struct header *hp;
	int *forwardvec;
{
	FILE *ibuf, *fbuf, *obuf;
	int escape, eof;
	long lc, cc;
	register int c, t;
	char linebuf[LINESIZE+1], *cp;
	char *iprompt;
	void (*sigpipe)(), (*sigint)();

	noreset++;
	if (tflag) {
		hadintr = 1;
		if ((savesig = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
			sigset(SIGINT, collrub);
		if ((savehup = sigset(SIGHUP, SIG_IGN)) != SIG_IGN)
			sigset(SIGHUP, collrub);
#ifdef SIGCONT
# ifdef preSVr4
		savecont = sigset(SIGCONT, collcont);
# else
		savecont = sigrset(SIGCONT, collcont);
# endif
#endif
		(void) readmailfromfile(stdin, hp, 1);
		ibuf = newi;
		obuf = newo;
		goto eofl;
	}

	ibuf = obuf = NULL;
	newi = newo = NULL;
	if ((obuf = fopen(tempMail, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, strerror(errno));
		goto err;
	}
	newo = obuf;
	if ((ibuf = fopen(tempMail, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, strerror(errno));
		newo = NULL;
		fclose(obuf);
		goto err;
	}
	newi = ibuf;
	removefile(tempMail);

	ig_ints = !!value("ignore");
	hadintr = 1;
	savehp = hp;
	if ((savesig = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
		if (ig_ints) {
			sigset(SIGINT, intack);
		} else {
			sigset(SIGINT, collrub);
		}
	if ((savehup = sigset(SIGHUP, SIG_IGN)) != SIG_IGN)
		sigset(SIGHUP, collrub);
#ifdef SIGCONT
# ifdef preSVr4
	savecont = sigset(SIGCONT, collcont);
# else
	savecont = sigrset(SIGCONT, collcont);
# endif
#endif
	/*
	 * If we are going to prompt for subject/cc/bcc,
	 * refrain from printing a newline after
	 * the headers (since some people mind).
	 */

	if (hp->h_subject == NOSTR) {
		hp->h_subject = sflag;
		sflag = NOSTR;
	}
	t = GMASK;
	c = 0;
	if (intty) {
		if (hp->h_to == NOSTR && forwardvec)
			c |= GTO;
		if (hp->h_subject == NOSTR && value("asksub"))
			c |= GSUBJECT;
		if (!value("askatend")) {
			if (hp->h_cc == NOSTR && value("askcc"))
				c |= GCC;
			if (hp->h_bcc == NOSTR && value("askbcc"))
				c |= GBCC;
		}
		if (c)
			t &= ~GNL;
	}
	if (hp->h_seq != 0) {
		puthead(hp, stdout, t, (FILE*)0);
		fflush(stdout);
	}
	if (setjmp(coljmp))
		goto err;
	if (c)
		grabh(hp, c, 1);
	escape = SENDESC;
	if ((cp = value("escape")) != NOSTR)
		escape = *cp;
	if (escape == '\0')	/* if escape=nothing, disable escape processing */
		escape = -1;
	eof = 0;
	if ((cp = value("MAILX_HEAD")) != NOSTR) {
	      cpout( cp, obuf);
	      if (isatty(fileno(stdin)))
		    cpout( cp, stdout);
	}
	iprompt = value("iprompt");
	fflush(obuf);
	hadintr = 0;

	if (intty && (value("autovedit") || value("autoedit"))) {
		if (forwardvec) {
			if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
				goto err;
			forwardvec = 0;
		}
		if ((obuf = mesedit(ibuf, obuf, value("autovedit") ? 'v' : 'e', hp)) == NULL)
			goto err;
		newo = obuf;
		ibuf = newi;
		pfmt(stdout, MM_NOSTD, usercontinue);
	}

	for (;;) {
		int nread, hasnulls;
		setjmp(coljmp);
		sigrelse(SIGINT);
		sigrelse(SIGHUP);
		if (intty && outtty && iprompt)
			fputs(iprompt, stdout);
		flush();
		if ((nread = getline(linebuf,LINESIZE,stdin,&hasnulls)) == NULL) {
			if (intty && value("ignoreeof") != NOSTR) {
				if (++eof > 35)
					break;
				pfmt(stdout, MM_NOSTD, 
					":212:Use \".\" to terminate letter\n");
				continue;
			}
			break;
		}
		eof = 0;
		hadintr = 0;
		if (intty && equal(".\n", linebuf) &&
		    (value("dot") != NOSTR || value("ignoreeof") != NOSTR))
			break;
		if ((linebuf[0] != escape) || (escape == -1) || (!intty && !escapeokay)) {
			if (write(fileno(obuf),linebuf,nread) != nread)
				goto err;
			continue;
		}
		/*
		 * On double escape, just send the single one.
		 */
		if ((nread > 1) && (linebuf[1] == escape)) {
			if (write(fileno(obuf),linebuf+1,nread-1) != (nread-1))
				goto err;
			continue;
		}
		if (hasnulls)
			nread = stripnulls(linebuf, nread);
		c = linebuf[1];
		linebuf[nread - 1] = '\0';
		switch (c) {
		default:
			/*
			 * Otherwise, it's an error.
			 */
			pfmt(stdout, MM_ERROR, ":213:Unknown tilde escape.\n");
			break;

		case 'a':
		case 'A':
			/*
			 * autograph; sign the letter.
			 */

			if ((cp = value(c=='a' ? "sign":"Sign")) != 0) {
			      cpout( cp, obuf);
			      if (isatty(fileno(stdin)))
				    cpout( cp, stdout);
			}

			break;

		case 'i':
			/*
			 * insert string
			 */
			for (cp = &linebuf[2]; any(*cp, " \t"); cp++)
				;
			if (*cp)
				cp = value(cp);
			if (cp != NOSTR) {
				cpout(cp, obuf);
				if (isatty(fileno(stdout)))
					cpout(cp, stdout);
			}
			break;

		case '!':
			/*
			 * Shell escape, send the balance of the
			 * line to sh -c.
			 */

			shell(&linebuf[2]);
			break;

		case ':':
		case '_':
			/*
			 * Escape to command mode, but be nice!
			 */

			execute(&linebuf[2], 1);
			iprompt = value("iprompt");
			if ((cp = value("escape")) != 0)
				escape = *cp;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case '.':
			/*
			 * Simulate end of file on input.
			 */
			goto eofl;

		case 'q':
		case 'Q':
			/*
			 * Force a quit of sending mail.
			 * Act like an interrupt happened.
			 */

			hadintr++;
			collrub(SIGINT);
			exit(1);
			/* NOTREACHED */

		case 'x':
			xhalt();
			break; 	/* not reached */

		case 'h':
			/*
			 * Grab a bunch of headers.
			 */
			if (!intty || !outtty) {
				pfmt(stdout, MM_ERROR, ":214:~h: Not a tty\n");
				break;
			}
			grabh(hp, GMASK, 0);
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case 't':
			/*
			 * Add to the To list.
			 */

			hp->h_to = addto(hp->h_to, &linebuf[2]);
			hp->h_seq++;
			break;

		case 's':
			/*
			 * Set the Subject list.
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			hp->h_subject = savestr(cp);
			hp->h_seq++;
			break;

		case 'c':
			/*
			 * Add to the CC list.
			 */

			hp->h_cc = addto(hp->h_cc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'b':
			/*
			 * Add stuff to blind carbon copies list.
			 */
			hp->h_bcc = addto(hp->h_bcc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'R':
			hp->h_defopt = addone(hp->h_defopt, "/receipt");
			hp->h_seq++;
			pfmt(stderr, MM_INFO, ":215:Return receipt marked.\n");
			break;

		case 'd':
			copy(Getf("DEAD"), &linebuf[2]);
			/* FALLTHROUGH */

		case '<':
		case 'r': {
			int	ispip;
			/*
			 * Invoke a file:
			 * Search for the file name,
			 * then open it and copy the contents to obuf.
			 *
			 * if name begins with '!', read from a command
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\0') {
				pfmt(stdout, MM_ERROR, 
					":216:Interpolate what file?\n");
				break;
			}
			if (*cp=='!') {
				/* take input from a command */
				ispip = 1;
				if ((fbuf = npopen(++cp, "r"))==NULL) {
					pfmt(stderr, MM_ERROR, failed, "pipe",
						strerror(errno));
					break;
				}
				sigint = sigset(SIGINT, SIG_IGN);
			} else {
				ispip = 0;
				cp = expand(cp);
				if (cp == NOSTR)
					break;
				if (isdir(cp)) {
					pfmt(stdout, MM_ERROR, 
						":217:%s: directory\n", cp);
					break;
				}
				if ((fbuf = fopen(cp, "r")) == NULL) {
					pfmt(stderr, MM_ERROR, badopen, 
						cp, strerror(errno));
					break;
				}
			}
			printf("\"%s\" ", cp);
			flush();
			lc = cc = 0;
			while ((t = getc(fbuf)) != EOF) {
				if (t == '\n')
					lc++;
				if (putc(t, obuf) == EOF) {
					if (ispip) {
						npclose(fbuf);
						sigset(SIGINT, sigint);
					} else
						fclose(fbuf);
					goto err;
				}
				cc++;
			}
			if (ispip) {
				npclose(fbuf);
				sigset(SIGINT, sigint);
			} else
				fclose(fbuf);
			printf("%ld/%ld\n", lc, cc);
			fflush(obuf);
			break;
			}

		case 'w':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
				forwardvec = 0;
			}

			/*
			 * Write the message on a file.
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\0') {
				pfmt(stderr, MM_ERROR, 
					":218:Write what file!?\n");
				break;
			}
			if ((cp = expand(cp)) == NOSTR)
				break;
			fflush(obuf);
			rewind(ibuf);
			exwrite(cp, ibuf);
			break;

		case 'm':
		case 'f':
		case 'M':
		case 'F':
			/*
			 * Interpolate the named messages, if we
			 * are in receiving mail mode.  Does the
			 * standard list processing garbage.
			 * If ~f is given, we don't shift over.
			 */

			if (!rcvmode) {
				pfmt(stdout, MM_ERROR, 
					":219:No messages to send from!?!\n");
				break;
			}
			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (forward(cp, obuf, c) < 0)
				goto err;
			fflush(obuf);
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case '?':
			(void) pghelpfile(THELPFILE);
			break;

		case 'p':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
				forwardvec = 0;
			}

			/*
			 * Print out the current state of the
			 * message without altering anything.
			 */

			fflush(obuf);
			rewind(ibuf);
			if (value("crt")) {
				const char *pg = pager();
				if ((fbuf = npopen(pg, "w")) != 0) {
					sigint = sigset(SIGINT, SIG_IGN);
					sigpipe = sigset(SIGPIPE, SIG_IGN);
				} else
					fbuf = stdout;
			} else
				fbuf = stdout;
			pfmt(fbuf, MM_NOSTD, ":220:-------\nMessage contains:\n");
			puthead(hp, fbuf, GMASK, (FILE*)0);
			copystream(ibuf, fbuf);
			if (fbuf != stdout) {
				npclose(fbuf);
				sigset(SIGPIPE, sigpipe);
				sigset(SIGINT, sigint);
			}
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case '^':
		case '|':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
				forwardvec = 0;
			}

			/*
			 * Pipe message through command.
			 * Collect output as new message.
			 */

			obuf = mespipe(ibuf, obuf, &linebuf[2]);
			newo = obuf;
			ibuf = newi;
			newi = ibuf;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case 'v':
		case 'e':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
				forwardvec = 0;
			}

			/*
			 * Edit the current message.
			 * 'e' means to use EDITOR
			 * 'v' means to use VISUAL
			 */

			if ((obuf = mesedit(ibuf, obuf, c, hp)) == NULL)
				goto err;
			newo = obuf;
			ibuf = newi;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;
		}
		fflush(obuf);
	}
eofl:
	/*
	 * simulate a ~a/~A autograph and sign the letter.
	 */
	if ((cp = value("autosign")) != 0) {
	      cpout( cp, obuf);
	      if (isatty(fileno(stdin)))
		    cpout( cp, stdout);
	}
	if ((cp = value("autoSign")) != 0) {
	      cpout( cp, obuf);
	}

	/* first make certain the forwarded messages are copied */
	if (forwardvec) {
		if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
			goto err;
		forwardvec = 0;
	}

	if ((cp = value("MAILX_TAIL")) != NOSTR) {
	      cpout( cp, obuf);
	      if (isatty(fileno(stdin)))
		    cpout( cp, stdout);
	}
	if (intty && value("askatend")) {
		if (hp->h_cc == NOSTR && value("askcc"))
			grabh(hp, GCC, 0);
		if (hp->h_bcc == NOSTR && value("askbcc"))
			grabh(hp, GBCC, 0);
	}
	fflush(obuf);
	fclose(obuf);
	rewind(ibuf);
	resetsigs(0);
	noreset = 0;
	return(ibuf);

err:
	if (ibuf != NULL)
		fclose(ibuf);
	if (obuf != NULL)
		fclose(obuf);
	resetsigs(0);
	noreset = 0;
	return(NULL);
}

static void resetsigs(resethup)
int resethup;
{
	(void) sigset(SIGINT, savesig);
	if (resethup)
		(void) sigset(SIGHUP, savehup);
#ifdef SIGCONT
# ifdef preSVr4
	(void) sigset(SIGCONT, savecont);
# else
	(void) sigrset(SIGCONT, savecont);
# endif
#endif
}

/*
 * Write a file ex-like.
 */

static int
exwrite(fname, ibuf)
	char fname[];
	FILE *ibuf;
{
	register FILE *of;
	struct stat junk;
	void (*sigint)(), (*sigpipe)();
	int pi = (*fname == '!');

	if (!pi && stat(fname, &junk) >= 0
	 && (junk.st_mode & S_IFMT) == S_IFREG) {
		pfmt(stderr, MM_ERROR, filedothexist, fname);
		return(-1);
	}
	if ((of = pi ? npopen(++fname, "w") : fopen(fname, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, errmsg, fname, strerror(errno));
		return(-1);
	}
	if (pi) {
		sigint = sigset(SIGINT, SIG_IGN);
		sigpipe = sigset(SIGPIPE, SIG_IGN);
	}
	lcwrite(fname, ibuf, of);
	pi ? npclose(of) : fclose(of);
	if (pi) {
		sigset(SIGPIPE, sigpipe);
		sigset(SIGINT, sigint);
	}
	return(0);
}

void
lcwrite(fn, fi, fo)
char *fn;
FILE *fi, *fo;
{
	register int c;
	long lc, cc;

	printf("\"%s\" ", fn);
	fflush(stdout);
	lc = cc = 0;
	while ((c = getc(fi)) != EOF) {
		cc++;
		if (putc(c, fo) == '\n')
			lc++;
		if (ferror(fo)) {
			pfmt(stderr, MM_ERROR, badwrite1, strerror(errno));
			return;
		}
	}
	printf("%ld/%ld\n", lc, cc);
	fflush(stdout);
}

/*
 * Edit the message being collected on ibuf and obuf.
 * Write the message out onto some poorly-named temp file
 * and point an editor at it.
 *
 * On return, make the edit file the new temp file.
 */

static FILE *
mesedit(ibuf, obuf, c, hp)
	FILE *ibuf, *obuf;
	struct header *hp;
{
	pid_t pid;
	FILE *fbuf;
	void (*sigint)();
	struct stat sbuf;
	register char *editor_value;

	if (stat(tempEdit, &sbuf) >= 0) {
		pfmt(stdout, MM_ERROR, filedothexist, tempEdit);
		goto out;
	}
	close(creat(tempEdit, TEMPPERM));
	if ((fbuf = fopen(tempEdit, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempEdit, strerror(errno));
		goto out;
	}
	fflush(obuf);
	rewind(ibuf);
	if (value("editheaders"))
		puthead(hp, fbuf, GMASK, (FILE*)0);
	copystream(ibuf, fbuf);
	fflush(fbuf);
	if (ferror(fbuf)) {
		pfmt(stderr, MM_ERROR, badwrite, tempEdit, strerror(errno));
		removefile(tempEdit);
		goto out;
	}
	fclose(fbuf);
	if (((editor_value = value(c == 'e' ? "EDITOR" : "VISUAL")) == NOSTR) || (*editor_value == '\0'))
		editor_value = c == 'e' ? EDITOR : VISUAL;
	editor_value = safeexpand(editor_value);

	/*
	 * Fork/execlp the editor on the edit file
	*/

	pid = vfork();
	if ( pid == (pid_t)-1 ) {
		pfmt(stderr, MM_ERROR, failed, "fork", strerror(errno));
		removefile(tempEdit);
		goto out;
	}
	if ( pid == 0 ) {
		sigchild();
		execlp(editor_value, editor_value, tempEdit, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, editor_value, strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	sigint = sigset(SIGINT, SIG_IGN);
	while (wait((int *)0) != pid)
		;
	sigset(SIGINT, sigint);
	/*
	 * Now switch to new file.
	 */

	if ((fbuf = fopen(tempEdit, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempEdit, strerror(errno));
		removefile(tempEdit);
		goto out;
	}
	removefile(tempEdit);
	newo = readmailfromfile(fbuf, hp, (value("editheaders") != 0));
	fclose(fbuf);
out:
	return(newo);
}

/*
 * Pipe the message through the command.
 * Old message is on stdin of command;
 * New message collected from stdout.
 * Sh -c must return 0 to accept the new message.
 */

static FILE *
mespipe(ibuf, obuf, cmd)
	FILE *ibuf, *obuf;
	char cmd[];
{
	register FILE *ni, *no;
	pid_t pid;
	int s;
	void (*sigint)();
	char *Shell;

	newi = ibuf;
	if ((no = fopen(tempEdit, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempEdit, strerror(errno));
		return(obuf);
	}
	if ((ni = fopen(tempEdit, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempEdit, strerror(errno));
		fclose(no);
		removefile(tempEdit);
		return(obuf);
	}
	removefile(tempEdit);
	fflush(obuf);
	rewind(ibuf);
	if ((Shell = value("SHELL")) == NULL || *Shell=='\0')
		Shell = SHELL;
	if ((pid = vfork()) == (pid_t)-1) {
		pfmt(stderr, MM_ERROR, failed, "fork", strerror(errno));
		goto err;
	}
	if (pid == 0) {
		/*
		 * stdin = current message.
		 * stdout = new message.
		 */

		sigchild();
		close(0);
		dup(fileno(ibuf));
		close(1);
		dup(fileno(no));
		for (s = 4; s < 15; s++)
			close(s);
		execlp(Shell, Shell, "-c", cmd, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, Shell, strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	sigint = sigset(SIGINT, SIG_IGN);
	while (wait(&s) != pid)
		;
	sigset(SIGINT, sigint);
	if (s != 0 || pid == (pid_t)-1) {
		pfmt(stderr, MM_ERROR, cmdfailed, cmd);
		goto err;
	}
	if (fsize(ni) == 0) {
		pfmt(stderr, MM_ERROR, ":221:No bytes from \"%s\" !?\n", cmd);
		goto err;
	}

	/*
	 * Take new files.
	 */

	newi = ni;
	fclose(ibuf);
	fclose(obuf);
	return(no);

err:
	fclose(no);
	fclose(ni);
	return(obuf);
}

/*
 * Interpolate the named messages into the current
 * message, preceding each line with a tab.
 * Return 0 on success, or -1 if an error is encountered writing
 * the message temporary.  The flag argument is 'm' if we
 * should shift over and 'f' if not.
 */
static int
forward(ms, obuf, f)
	char ms[];
	FILE *obuf;
{
	register int *msgvec;

	msgvec = (int *) salloc((msgCount+1) * sizeof *msgvec);
	if (getmsglist(ms, msgvec, 0) < 0)
		return(0);
	if (*msgvec == NULL) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			pfmt(stdout, MM_ERROR, ":222:No appropriate messages\n");
			return(0);
		}
		msgvec[1] = NULL;
	}
	pfmt(stdout, MM_NOSTD, ":223:Interpolating:");
	if (forwardmsgs(msgvec, obuf, f, 1) < 0)
		return(-1);
	printf("\n");
	return(0);
}

/*
 * Interpolate the named messages into the current
 * message, preceding each line with a tab.
 * Return 0 on success, or -1 if an error is encountered writing
 * the message temporary.  The flag argument is:
 *	'm','M'		we should shift over with "mprefix"
 *	'f','F'		we should not
 *
 *	'm','f'		ignore headers
 *	'M','F'		do not ignore headers
 *
 * If value("forwardbracket")
 *	'f','F'		Bracket with "forwardbegin"/"forwardend" messages & shift with "forwardprefix"
 * 			Use defaults if those are not set.
 */
static int
forwardmsgs(msgvec, obuf, flag, printnum)
	int *msgvec;
	FILE *obuf;
	int flag;
	int printnum;
{
	register int *ip;
	int doign = islower(flag);

	if ((tolower(flag) == 'f') && value("forwardbracket"))
		flag = 'B';

	if (flag == 'B') {
		char *cp = value("forwardbegin");
		if (cp) cpout(cp, obuf);
		else cpout(gettxt(forwardbeginid, forwardbegin), obuf);
	}

	for (ip = msgvec; *ip != NULL; ip++) {
		if (printnum)
			printf(" %d", *ip);
		if (send(&message[*ip-1], obuf, doign, flag, 1) < 0) {
			pfmt(stderr, MM_ERROR, badwrite,
				tempMail, strerror(errno));
			return(-1);
		}
		touch(*ip);
	}

	if (flag == 'B') {
		char *cp = value("forwardend");
		if (cp) cpout(cp, obuf);
		else cpout(gettxt(forwardendid, forwardend), obuf);
	}
	return(0);
}

/*
 * Print (continue) when continued after ^Z.
 */
#ifdef SIGCONT
/*ARGSUSED*/
static void
collcont(s)
{
	pfmt(stdout, MM_NOSTD, usercontinue);
	fflush(stdout);
}

# ifndef preSVr4
static void (*sigrset(sig, sigfunc))(int)
int sig;
void (*sigfunc)(int);
{
	struct sigaction nsig, osig;
	nsig.sa_handler = sigfunc;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = SA_RESTART;
	(void) sigaction(sig, &nsig, &osig);
	return osig.sa_handler;
}
# endif /* preSVr4 */
#endif /* SIGCONT */

/*
 * On interrupt, go here to save the partial
 * message on ~/dead.letter.
 * Then restore signals and execute the normal
 * signal routine.  We only come here if signals
 * were previously set anyway.
 */
static void
collrub(s)
{
	if (s == SIGHUP)
		sigignore(SIGHUP);
	if (s == SIGINT && hadintr == 0) {
		hadintr++;
		putchar('\n');
		pfmt(stdout, MM_NOSTD,
			":224:(Interrupt -- one more to kill letter)\n");
		sigrelse(s);
		longjmp(coljmp, 1);
	}
	fclose(newo);
	rewind(newi);
	if (fsize(newi) > 0 && (value("save") || s != SIGINT))
		savemail(Getf("DEAD"), DEADPERM, savehp,
			newi, (off_t)0, s == SIGINT);
	fclose(newi);
	resetsigs(1);
	if (rcvmode) {
		if (s == SIGHUP)
			hangup(s);
		else
			stop(s);
	} else
		exit(1);
}

/*
 * Acknowledge an interrupt signal from the tty by typing an @
 */
/* ARGSUSED */
static void
intack(s)
{
	puts("@");
	fflush(stdout);
	clearerr(stdin);
	longjmp(coljmp,1);
}

/* Read line from stdin, noting any NULL characters.
   Return the number of characters read. Note that the buffer
   passed must be 1 larger than "size" for the trailing NUL byte.
 */
int
getline(line,size,f,hasnulls)
char *line;
int size;
FILE *f;
int *hasnulls;
{
	register int i, ch;
	for (i = 0; (i < size) && ((ch=getc(f)) != EOF); ) {
		if ( ch == '\0' )
			*hasnulls = 1;
		if ((line[i++] = (char)ch) == '\n') break;
	}
	line[i] = '\0';
	return(i);
}

/* ARGSUSED */
void
savedead(s)
{
	collrub(SIGINT);
	exit(1);
	/* NOTREACHED */
}

/*
 * Add a list of addresses to the end of a header entry field.
 */
char *
addto(hf, news)
	char hf[], news[];
{
	char fname[LINESIZE];
	int comma = docomma(news);

	while ((news = yankword(news, fname, comma)) != 0) {
		strcat(fname, ", ");
		hf = addone(hf, fname);
	}
	return hf;
}

/*
 * Add a string to the end of a header entry field.
 */
char *
addone(hf, news)
	char hf[], news[];
{
	register char *cp, *cp2, *linebuf;

	if (hf == NOSTR)
		hf = "";
	if (*news == '\0')
		return(hf);
	linebuf = salloc((unsigned)(strlen(hf) + strlen(news) + 2));
	for (cp = hf; any(*cp, " \t"); cp++)
		;
	for (cp2 = linebuf; *cp;)
		*cp2++ = *cp++;
	if (cp2 > linebuf && cp2[-1] != ' ')
		*cp2++ = ' ';
	for (cp = news; any(*cp, " \t"); cp++)
		;
	while (*cp != '\0')
		*cp2++ = *cp++;
	*cp2 = '\0';
	return(linebuf);
}

static int nptrs(hf)
char **hf;
{
	register int i;
	if (!hf)
		return 0;
	for (i = 0; *hf; hf++)
		i++;
	return i;
}

/*
 * Add a non-standard header to the end of the non-standard headers.
 */
static char **
Xaddone(hf, news)
	char **hf, news[];
{
	register char *linebuf;
	int nhf = nptrs(hf);

	if (hf == NOSTRPTR)
		hf = (char**)salloc(sizeof(char*) * 2);
	else
		hf = (char**)srealloc((char*)hf, sizeof(char*) * (nhf + 2));
	linebuf = salloc((unsigned)(strlen(news) + 1));
	strcpy(linebuf, news);
	hf[nhf++] = linebuf;
	hf[nhf] = NOSTR;
	return(hf);
}

static void
cpout( str, ofd )
char *str;
FILE *ofd;
{
      register char *cp = str;

      while ( *cp ) {
	    if ( *cp == '\\' ) {
		  switch ( *(cp+1) ) {
			case 'n':
			      putc('\n',ofd);
			      cp++;
			      break;
			case 't':
			      putc('\t',ofd);
			      cp++;
			      break;
			default:
			      putc('\\',ofd);
		  }
	    }
	    else {
		  putc(*cp,ofd);
	    }
	    cp++;
      }
      putc('\n',ofd);
      fflush(ofd);
}

static void
xhalt()
{
	fclose(newo);
	fclose(newi);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
	if (rcvmode)
		stop(0);
	exit(1);
	/* NOTREACHED */
}

/*
	Strip the nulls from a buffer of length n
*/
static int
stripnulls(linebuf, nread)
register char *linebuf;
register int nread;
{
	register int i, j;
	for (i = 0; i < nread; i++)
		if (linebuf[i] == '\0')
			break;
	for (j = i; j < nread; j++)
		if (linebuf[j] != '\0')
			linebuf[i++] = linebuf[j];
	linebuf[i] = '\0';
	return i;
}
