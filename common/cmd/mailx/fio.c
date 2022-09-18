/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:fio.c	1.15.2.5"
#ident "@(#)fio.c	1.15 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * File I/O.
 */

static char Unknown[] = "Unknown";

static char *getencdlocale ARGS((const char *line));

/*
 * Set up the input pointers while copying the mail file into
 * /tmp.
 */

void
setptr(ibuf)
	FILE *ibuf;
{
	int n;				/* size of input line */
	int newline = 1;		/* previous header had a newline */
	int StartNewMsg = TRUE;
	int ToldUser = FALSE;
	int ctf = FALSE; 		/* header continuation flag */
	long clen = 0L;			/* value of content-length header */
	int hdr = 0;			/* type of header */
	int cflg = 0;			/* found Content-length in header */
	register char *cp;
	register int l;
	register long s;
	off_t offset;
	char linebuf[LINESIZE];
	int inhead = 1, newmail_flag, Odot;
	short flag;

	if ( !space ) {
		msgCount = 0;
		offset = 0;
		space = 32;
		newmail_flag = 0;
		message = (struct message *)malloc(space * sizeof(struct message));
		if ( message == NULL ) {
			pfmt(stderr, MM_ERROR, ":226:Not enough memory for %d messages: %s\n",
				space, strerror(errno));
			exit(1);
			/* NOTREACHED */
		}
		message[0].m_text = M_text;
		message[0].m_encoding_type = Unknown;
		message[0].m_clen = -1;
		dot = message;
	} else {
		newmail_flag = 1;
		offset = fsize(otf);
	}
	s = 0L;
	l = 0;
	flag = MUSED|MNEW;

	while ((n = getln(linebuf, sizeof linebuf, ibuf)) > 0) {
		if (!newline) {
			goto putout;
		} else if ((hdr = isheader (linebuf, &ctf)) == FALSE) {
			ctf = FALSE;	/* next line can't be cont. */
		}
		if (!hdr && cflg) {	/* nonheader, Content-length seen */
			if (clen < n) {	/* read too much */
				/* NB: this only can happen if there is a
				 * small content that is NOT \n terminated
				 * and has no leading blank line.
				 */
				if (msgCount > 0 && message[msgCount-1].m_text != M_binary) {
					message[msgCount-1].m_text = istext((unsigned char*)linebuf,clen,message[msgCount-1].m_text);
				}
				if (fwrite(linebuf,1,(int)clen,otf) != clen) {
					fclose(ibuf); fflush(otf);
				} else {
				    if (msgCount > 0 && message[msgCount-1].m_text != M_binary) {
					l += linecount(linebuf, n);
				    }
				}
				offset += clen;
				s += (long)clen;
				n -= clen;
				memcpy (linebuf, linebuf+clen, n+1);	/* copy null */
				cflg = 0;
				ctf = FALSE;
				hdr = isheader(linebuf, &ctf);
				goto headerswitch;
			}
			/* here, clen >= n */
			if (n == 1 && linebuf[0] == '\n'){	/* leading empty line */
				clen++;		/* cheat */
				inhead = 0;
			}
			offset += clen;
			s += (long)clen;
			if (msgCount > 0)
				message[msgCount-1].m_clen = (long)clen;
			for (;;) {
				if (msgCount > 0 && message[msgCount-1].m_text != M_binary) {
					message[msgCount-1].m_text = istext((unsigned char*)linebuf,(long)n,message[msgCount-1].m_text);
				}
				if (fwrite(linebuf,1,n,otf) != n) {
					fclose(ibuf); fflush(otf);
				} else {
					if (msgCount > 0) {
						l += linecount(linebuf, n);
					}
				}
				clen -= n;
				if (clen <= 0) {
					break;
				}
				n = clen < sizeof linebuf ? clen : sizeof linebuf;
				if ((n = fread (linebuf, 1, n, ibuf)) <= 0) {
				    pfmt(stderr, MM_ERROR,
					":227:Your mailfile was found to be corrupted.\n\t(Unexpected EOF).\n\tMessage #%d may be truncated.\n\n",
					msgCount);
					offset -= clen;
					s -= clen;
					clen = 0; /* stop the loop */
				}
			}
			/* All done, go to top for next message */
			cflg = 0;
			StartNewMsg = TRUE;
			continue;
		}
headerswitch:
		switch (hdr) {
		case H_FROM:
			if ( (msgCount > 0) && (!newmail_flag) ){
				message[msgCount-1].m_size = s;
				message[msgCount-1].m_lines = l;
				message[msgCount-1].m_flag = flag;
				if (message[msgCount-1].m_clen == -1)
					message[msgCount-1].m_clen = clen;
				flag = MUSED|MNEW;
			}
			/* always keep 1 extra */
			if ( (msgCount+1) >= space ) {
				/* Limit the speed at which the allocated space grows */
				if ( space < 512 )
					space = space*2;
				else
					space += 512;
				errno = 0;
				Odot = dot - &(message[0]);
				message = (struct message *)realloc((char*)message,space*(sizeof( struct message)));
				if ( message == NULL ) {
					pfmt(stderr, MM_ERROR, nomem, strerror(errno));
					exit(1);
				}
				dot = &message[Odot];
			}
			message[msgCount].m_offset = offset;
			message[msgCount].m_text = M_text;
			message[msgCount].m_encoding_type = Unknown;
			message[msgCount].m_clen = -1;
			newmail_flag = 0;
			msgCount++;
			flag = MUSED|MNEW;
			inhead = 1;
			s = 0L;
			clen = 0L;
			l = 0;
			StartNewMsg = FALSE;
			ToldUser = FALSE;
			break;

		case H_CLEN:
			if (inhead && !cflg) {
				cflg = TRUE;	/* mark for clen processing */
				clen = atol(strpbrk(linebuf, ":")+1);
			}
			break;

		case H_ENCDTYPE:
			if (msgCount > 0 && inhead && message[msgCount-1].m_encoding_type == Unknown)
				message[msgCount-1].m_encoding_type = getencdlocale(strpbrk(linebuf, ":")+1);
			break;

		case H_STATUS:
			if (inhead && ishfield(linebuf, "status")) {
				cp = hcontents(linebuf);
				if (strchr(cp, 'R'))
					flag |= MREAD;
				if (strchr(cp, 'O'))
					flag &= ~MNEW;
			}
			break;

		case 0:
			if (inhead) {
				/* If there was no blank line between the headers and */
				/* the first non-header, we must add one. */
				if (n > 1) {
					putc('\n', otf);
					clen = 1;
					offset++;
					s++;
				} else
					clen = 0;
				inhead = 0;
			}
			break;

		default:
			break;
		}
putout:
		offset += n;
		s += (long)n;
		if (!inhead)
		    clen += (long)n;
		if (msgCount > 0 && message[msgCount-1].m_text != M_binary) {
			message[msgCount-1].m_text = istext((unsigned char*)linebuf,n,message[msgCount-1].m_text);
		}
		if (fwrite(linebuf,1,n,otf) != n) {
			fclose(ibuf);
			fflush(otf);
		} else {
			l++;
		}
		if (ferror(otf)) {
			pfmt(stderr, MM_ERROR, errmsg, "/tmp", strerror(errno));
			exit(1);
		}
		if (msgCount == 0) {
			fclose(ibuf);
			fflush(otf);

		}
		if (linebuf[n-1] == '\n') {
			newline = 1;
			if (n == 1) { /* Blank line. Skip StartNewMsg */
				      /* check below                  */
				continue;
			}
		} else {
			newline = 0;
		}
		if (StartNewMsg && ToldUser) {
			pfmt(stderr, MM_ERROR,
				":228:Your mailfile was found to be corrupted\n\t(Content-length mismatch).\n\tMessage #%d may be truncated,\n\twith another message concatenated to it.\n\n",
				msgCount);
			ToldUser = TRUE;
		}
	}

	/*
		last plus 1
	*/
	message[msgCount].m_text = M_text;
	message[msgCount].m_encoding_type = Unknown;

	if (n == 0) {
		fflush(otf);
		if (msgCount > 0) {
			message[msgCount-1].m_size = s;
			message[msgCount-1].m_lines = l;
			message[msgCount-1].m_flag = flag;
			if (message[msgCount-1].m_clen == -1)
				message[msgCount-1].m_clen = clen;
		}
		flag = MUSED|MNEW;
		fclose(ibuf);
		fflush(otf);
		return;
	}
}

/*  HMF:  Code from fio.c. (getln)                                         */

int
getln(line, max, f)
	char *line;
	int max;
	FILE	*f;
{
	int	i,ch;
	for (i=0; i < max-1 && (ch=getc(f)) != EOF;)
		if ((line[i++] = (char)ch) == '\n') break;
	line[i] = '\0';
	return(i);
}

/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 */

readline(ibuf, linebuf)
	FILE *ibuf;
	char *linebuf;
{
	register char *cp;
	register int c;

	do {
		clearerr(ibuf);
		c = getc(ibuf);
		for (cp = linebuf; c != '\n' && c != EOF; c = getc(ibuf)) {
			if (c == 0) {
				pfmt(stderr, MM_WARNING, ":229:NUL changed to @\n");
				c = '@';
			}
			if (cp - linebuf < LINESIZE-2)
				*cp++ = (char)c;
		}
	} while (ferror(ibuf) && ibuf == stdin);
	*cp = 0;
	if (c == EOF && cp == linebuf)
		return(0);
	return(cp - linebuf + 1);
}

/*
    NAME
	getencdlocale - get the locale information from Encoding-Type: header

    SYNOPSIS
	char *getencdlocale(const char *line)

    DESCRIPTION
	Getencdlocale looks through the header for the locale information,
	such as "/locale=french". Just the "french" part is returned. Other
	/xyz fields may also be present, all prefaced with a "/".
*/

static char *getencdlocale(line)
const char *line;
{
    /* look for strings starting with "/" */
    for (line = skipspace(line); line && *line; line = strchr(line, '/'))
	/* Did we find "/locale="? */
	if (casncmp(line, "/locale=", 8) == 0) {
	    /* find the end of the locale name and copy it */
	    int localelen = strcspn(line+8, " \t\n,/");
	    char *duparea = malloc(localelen + 1);
	    if (!duparea)
		return Unknown;
	    strncpy(duparea, line+8, localelen);
	    duparea[localelen] = '\0';
	    return duparea;
	}

    return Unknown;
}

/*
 * Return a file buffer all ready to read up the
 * passed message pointer.
 */

FILE *
setinput(mp)
	register struct message *mp;
{
	fflush(otf);
	if (fseek(itf, mp->m_offset, 0) < 0) {
		pfmt(stderr, MM_ERROR, failed, "fseek", strerror(errno));
		panic(":230:Temporary file seek");
	}
	return(itf);
}


/*
 * Delete a file, but only if the file is a plain file.
 */

removefile(filename)
	char filename[];
{
	struct stat statb;

	if (stat(filename, &statb) < 0)
		return(-1);
	if ((statb.st_mode & S_IFMT) != S_IFREG) {
		errno = EISDIR;
		return(-1);
	}
	return(unlink(filename));
}

/*
 * Terminate an editing session by attempting to write out the user's
 * file from the temporary.  Save any new stuff appended to the file.
 */
int
edstop()
{
	register int gotcha, c;
	register struct message *mp;
	FILE *obuf, *ibuf, *tbuf = 0, *readstat;
	struct stat statb;
	char tempname[30], *id;

	if (readonly)
		return(0);
	holdsigs();
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (mp = &message[0], gotcha = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MODIFY|MDELETED|MSTATUS))
			gotcha++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			if ((id = hfield("article-id", mp, addone)) != NOSTR)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (!gotcha || Tflag != NOSTR)
		goto done;
	if ((ibuf = fopen(editfile, "r+")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, editfile, strerror(errno));
		relsesigs();
		longjmp(srbuf, 1);
	}
	lock(ibuf, "r+", 1);
	if (fstat(fileno(ibuf), &statb) >= 0 && statb.st_size > mailsize) {
		strcpy(tempname, "/tmp/mboxXXXXXX");
		(void) mktemp(tempname);
		if ((obuf = fopen(tempname, "w")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				tempname, strerror(errno));
			fclose(ibuf);
			relsesigs();
			longjmp(srbuf, 1);
		}
		fseek(ibuf, mailsize, 0);
		copystream(ibuf, obuf);
		fclose(obuf);
		if ((tbuf = fopen(tempname, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				tempname, strerror(errno));
			fclose(ibuf);
			removefile(tempname);
			relsesigs();
			longjmp(srbuf, 1);
		}
		removefile(tempname);
	}
	flush();
	if ((obuf = fopen(editfile, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, editfile, strerror(errno));
		fclose(ibuf);
		if (tbuf)
			fclose(tbuf);
		relsesigs();
		longjmp(srbuf, 1);
	}
	c = 0;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if ((mp->m_flag & MDELETED) != 0)
			continue;
		c++;
		if (send(mp, obuf, 0, 0, 1) < 0) {
			pfmt(stderr, MM_ERROR, badwrite,
				editfile, strerror(errno));
			fclose(ibuf);
			fclose(obuf);
			if (tbuf)
				fclose(tbuf);
			relsesigs();
			longjmp(srbuf, 1);
		}
	}
	if (tbuf != NULL) {
		copystream(tbuf, obuf);
		fclose(tbuf);
	}
	fflush(obuf);
	if (ferror(obuf)) {
		pfmt(stderr, MM_ERROR, badwrite, editfile, strerror(errno));
		fclose(ibuf);
		fclose(obuf);
		relsesigs();
		longjmp(srbuf, 1);
	}
	if (c == 0 && !value("keep")) {
		removefile(editfile);
		pfmt(stdout, MM_NOSTD, ":231:\"%s\" removed.\n", editfile);
	} else
		pfmt(stdout, MM_NOSTD, ":232:\"%s\" updated.\n", editfile);
	fclose(ibuf);
	fclose(obuf);
	flush();

done:
	relsesigs();
	return(1);
}

/*
 * Hold signals SIGHUP - SIGQUIT.
 */
void
holdsigs()
{
	sighold(SIGHUP);
	sighold(SIGINT);
	sighold(SIGQUIT);
}

/*
 * Release signals SIGHUP - SIGQUIT
 */
void
relsesigs()
{
	sigrelse(SIGHUP);
	sigrelse(SIGINT);
	sigrelse(SIGQUIT);
}

/*
 * Flush the standard output.
 */

void
flush()
{
	fflush(stdout);
	fflush(stderr);
}

/*
 * Determine the size of the file possessed by
 * the passed buffer.
 */

off_t
fsize(iob)
	FILE *iob;
{
	register int f;
	struct stat sbuf;

	f = fileno(iob);
	if (fstat(f, &sbuf) < 0)
		return(0);
	return(sbuf.st_size);
}

/*
 * Take a file name, possibly with shell meta characters
 * in it and expand it by using "sh -c echo filename"
 * Return the file name as a dynamic string.
 * If the name cannot be expanded (for whatever reason)
 * return NULL.
 */

char *
expand(filename)
	char filename[];
{
	char xname[BUFSIZ];
	char cmdbuf[BUFSIZ];
	register pid_t pid;
	register int l;
	register char *cp;
	int s, pivec[2];
	struct stat sbuf;
	char *Shell;

	if (debug) fprintf(stderr, "expand(%s)=", filename);
	if (filename[0] == '+') {
		cp = safeexpand(++filename);
		if (*cp != '/' && getfold(cmdbuf) >= 0) {
			sprintf(xname, "%s/%s", cmdbuf, cp);
			cp = savestr(xname);
		}
		if (debug) fprintf(stderr, "%s\n", cp);
		return cp;
	}
	if (!anyof(filename, "~{[*?$`'\"\\ \t")) {
		if (debug) fprintf(stderr, "%s\n", filename);
		return(filename);
	}
	if (pipe(pivec) < 0) {
		pfmt(stderr, MM_ERROR, failed, "pipe", strerror(errno));
		return(savestr(filename));
	}
	sprintf(cmdbuf, "echo %s", filename);
	if ((pid = vfork()) == 0) {
		sigchild();
		close(pivec[0]);
		close(1);
		dup(pivec[1]);
		close(pivec[1]);
		if ((Shell = value("SHELL")) == NOSTR || *Shell=='\0')
			Shell = SHELL;
		execlp(Shell, Shell, "-c", cmdbuf, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, cmdbuf, strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	if (pid == (pid_t)-1) {
		pfmt(stderr, MM_ERROR, failed, "fork", strerror(errno));
		close(pivec[0]);
		close(pivec[1]);
		return(NOSTR);
	}
	close(pivec[1]);
	l = read(pivec[0], xname, BUFSIZ);
	close(pivec[0]);
	while (wait(&s) != pid);
		;
	s &= 0377;
	if (s != 0 && s != SIGPIPE) {
		pfmt(stderr, MM_ERROR, cmdfailed, "Echo");
		goto err;
	}
	if (l < 0) {
		pfmt(stderr, MM_ERROR, badread1, strerror(errno));
		goto err;
	}
	if (l == 0) {
		pfmt(stderr, MM_ERROR, ":233:\"%s\": No match\n", filename);
		goto err;
	}
	if (l == BUFSIZ) {
		pfmt(stderr, MM_ERROR, ":234:Buffer overflow expanding \"%s\"\n",
			filename);
		goto err;
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	*++cp = '\0';
	if (any(' ', xname) && stat(xname, &sbuf) < 0) {
		pfmt(stderr, MM_ERROR, ":235:\"%s\": Ambiguous\n", filename);
		goto err;
	}
	if (debug) fprintf(stderr, "%s\n", xname);
	return(savestr(xname));

err:
	printf("\n");
	return(NOSTR);
}

/*
 * Take a file name, possibly with shell meta characters
 * in it and expand it by using "sh -c echo filename"
 * Return the file name as a dynamic string.
 * If the name cannot be expanded (for whatever reason)
 * return the original file name.
 */

char *
safeexpand(filename)
	char filename[];
{
	char *t = expand(filename);
	return t ? t : savestr(filename);
}

/*
 * Determine the current folder directory name.
 */
getfold(foldername)
	char *foldername;
{
	char *folder;

	if ((folder = value("folder")) == NOSTR) {
		strcpy(foldername, homedir);
		return(0);
	}
	if ((folder = expand(folder)) == NOSTR)
		return(-1);
	if (*folder == '/')
		strcpy(foldername, folder);
	else
		sprintf(foldername, "%s/%s", homedir, folder);
	return(0);
}

/*
 * A nicer version of Fdopen, which allows us to fclose
 * without losing the open file.
 */

FILE *
Fdopen(fildes, mode)
	char *mode;
{
	register int f;

	f = dup(fildes);
	if (f < 0) {
		pfmt(stderr, MM_ERROR, failed, "dup", strerror(errno));
		return(NULL);
	}
	return(fdopen(f, mode));
}

/*
 * return the filename associated with "s".  This function always
 * returns a non-null string (no error checking is done on the receiving end)
 */
char *
Getf(s)
register char *s;
{
	register char *cp;
	static char defbuf[PATHSIZE];

	if (((cp = value(s)) != 0) && *cp) {
		return safeexpand(cp);
	} else if (strcmp(s, "MBOX")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "mbox");
		if (isdir(defbuf))
			strcat(defbuf, "/mbox");
		return(defbuf);
	} else if (strcmp(s, "DEAD")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "dead.letter");
		return(defbuf);
	} else if (strcmp(s, "MAILRC")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, ".mailrc");
		return(defbuf);
	} else if (strcmp(s, "HOME")==0) {
		/* no recursion allowed! */
		return(".");
	}
	return("DEAD");	/* "cannot happen" */
}
