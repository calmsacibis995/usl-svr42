/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:svcent.c	1.4.2.3"
#ident  "$Header: svcent.c 1.2 91/06/27 $"

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <rx.h>
#include <stdlib.h>
#include <string.h>
#include "rxsvcent.h"

#define	DONTEXPAND	0

/* externally defined routines */

extern	char	**strtoargv();


/* local routines */

static int	gettok();
static int	readuntil();


static struct {
	char	*service;	/* service name to look for */
	char	*argstr;	/* from RXM_ARGS message */
	char	**argv;		/* above converted */
} Target;


void
settarget(service, argstr)
char	*service;
char	*argstr;
{
	Target.service = service;
	Target.argstr = argstr;
	Target.argv = strtoargv(argstr);
}


/*
 * skipcmnt() - read comments and any whitespace characters from an
 * open services file.
 *
 * The return value is 0 for success, -1 for error
 *
 */

int
skipcmnt(fp)
FILE	*fp;			/* an open stream file pointer */
{
	int	skip = 1;	/* skipping a comment flag */
	int	c;		/* tmp character */
	char	linebuf[RX_MAXSVCLINE];	/* comment line */

	/* skip blank and comment lines */
	while (skip) {
		c = getc(fp);
		switch(c) {
		case EOF: /* no more characters in file */
			skip = 0;
			break;

		case '#': /* comment line - skip to next line */
			(void) fgets(linebuf, RX_MAXSVCLINE, fp);
			break;

		default: /* other character - skip if whitespace */
			skip = isspace(c);
			break;
		}
	}

	if (c == EOF)
		return(-1);

	/* return the first character (success is guaranteed) */
	(void) ungetc(c, fp);

	return(0);
}


/*
 * putsvcent() - put a service entry into a file at the current position
 *
 * Return value is 0 if successfull, -1 if the write fails.
 *
 */

int
putsvcent(fp, svc)
FILE	*fp;		/* open stream file pointer */
RX_SERVICE	*svc;	/* service to write to file */
{
	if (fprintf(fp, "%s\t%s\t%s\t%s\n",
		    svc->name, svc->descr, svc->utmp, svc->def) < 0)
		return(-1);
	else
		return(0);
}


/*
 * getsvcent() - get a service entry from a file and return it in a strucuture
 *
 * This routine reads the next service definition from the (open) file.
 *
 * Return value is 0 if a service is found, -1 upon error such as EOF.
 *
 */

int
getsvcent(fp, svc, expand)
FILE	*fp;		/* open stream file pointer */
RX_SERVICE	*svc;	/* place to hold service information */
int	expand;		/* expand macros flag */
{
	int	gr, gr2;		/* gettok() return codes */
	int	ret = 0;		/* getsvcent() return code */

	/* get service name */
	gr = gettok(fp, svc->name, sizeof(svc->name), DONTEXPAND);
	switch(gr) {

	case RX_TOK_OK:		/* normal case */
		break;

	case RX_TOK_TRUNC:	/* service name was truncated */
		ret |= RXF_TRUNC_NAME;
		break;

	case RX_TOK_ERROR:	/* EOF reached */
	case RX_TOK_OK_LAST:	/* EOLN reached */
	case RX_TOK_TRUNC_LAST:	/* EOLN reached */
		return(-1);
	}

	if (expand)
		/* is this the service we are looking for? */
		expand = (strcmp(svc->name, Target.service) == 0);

	/* get description */
	gr = gettok(fp, svc->descr, sizeof(svc->descr), DONTEXPAND);
	switch(gr) {

	case RX_TOK_OK:		/* normal case */
		break;

	case RX_TOK_TRUNC:	/* service description was truncated */
		ret |= RXF_TRUNC_DESCR;
		break;

	case RX_TOK_ERROR:	/* EOF reached */
	case RX_TOK_OK_LAST:	/* EOLN reached */
	case RX_TOK_TRUNC_LAST:	/* EOLN reached */
		return(-1);
	}

	/* get utmp flag */
	gr = gettok(fp, svc->utmp, sizeof(svc->utmp), DONTEXPAND);
	switch(gr) {

	case RX_TOK_OK:		/* normal case */
		break;

	case RX_TOK_TRUNC:	/* utmp flag was truncated */
		ret |= RXF_TRUNC_UTMP;
		break;

	case RX_TOK_ERROR:	/* EOF reached */
	case RX_TOK_OK_LAST:	/* EOLN reached */
	case RX_TOK_TRUNC_LAST:	/* EOLN reached */
		return(-1);
	}

	/* get service definition */
	gr = gettok(fp, svc->def, sizeof(svc->def), expand);
	switch(gr) {

	case RX_TOK_OK_LAST:	/* EOLN reached as expected */
		break;

	case RX_TOK_TRUNC_LAST:	/* EOLN reached as expected */
		ret |= RXF_TRUNC_DEF;
		break;

	case RX_TOK_TRUNC:	/* more tokens on this line */
		ret |= RXF_TRUNC_DEF;
		/* FALLTHRU */

	case RX_TOK_OK:		/* more tokens on this line */
		do
			gr2 = gettok(fp, "", 0, DONTEXPAND);
		while (gr2 == RX_TOK_TRUNC);
		break;
		
	case RX_TOK_ERROR:	/* EOF reached */
		return(-1);
	}

	return(ret);
}


/*
 * gettok() - get the next token
 *
 * This routine reads the next token from an (open) file.
 * If a single or double quote character is encountered, the routine
 * keeps reading the token disregarding any special characters until
 * a matching single or double quote character is found.
 * If a backslash character is encountered, the next character is
 * read literally and not interpreted.
 * A tab or new line character terminates the token (unless it occurs
 * between quotes or is preceded by a backslash).  If the maximum token
 * length has been read in, the rest of the token will be discarded and
 * the last character in the token being returned will always be NULL
 * (unless, of course, maxlen is 0).
 *
 * RX_TOK_ERROR is returned if EOF was encountered before the end of a token
 * RX_TOK_TRUNC is returned if the token read was too big, so the token
 * being returned had to be truncated
 * RX_TOK_TRUNC_LAST means the same as RX_TOK_TRUNC, except the truncated
 * token was also the last token on the line
 * RX_TOK_OK is returned if a token was read successfully and it was
 * delimited by a tab character
 * RX_TOK_OK_LAST is returned if a token was read successfully and 
 * it was delimited by a new line character
 *
 */

static int
gettok(fp, tokstr, maxlen, expand)
FILE	*fp;		/* open stream file pointer */
char	*tokstr;	/* where to put the token */
int	maxlen;		/* maximum length of the token */
int	expand;		/* expand macros flag */
{
	int	r;	/* readuntil() return code */

	r = readuntil(fp, &tokstr, 0, &maxlen, expand);

	if (r & RX_SCAN_ERROR)
		return(RX_TOK_ERROR);

	if (r & RX_SCAN_LAST)
		if (maxlen < 0)
			return(RX_TOK_TRUNC_LAST);
		else
			return(RX_TOK_OK_LAST);
	else
		if (maxlen < 0)
			return(RX_TOK_TRUNC);
		else
			return(RX_TOK_OK);
}


/*
 * getmacro() returns the string representing a macro value
 *
 *	%m - client machine network address
 *	%t - transport provider
 *	%s - login shell of mapped user
 *	%u - user id of user on client machine  (not supported)
 *	%g - group id of user on client machine (not supported)
 *	%l - label id of user on client machine (not supported)
 *	%# - argument number from the argument string
 *	%* - all arguments from the argument string
 *
 * For example, if a "listfiles" service defined to be "/bin/ls -l %*" is
 * invoked with arguments ". /bin /usr/bin", upon exit, argv would contain:
 *	argv[0] = "/bin/ls"
 *	argv[1] = "-l"
 *	argv[2] = "."
 *	argv[3] = "/bin"
 *	argv[4] = "/usr/bin"
 *	argv[5] = NULL
 *
 */

char *
getmacro(macro)
int	macro;		/* macro character */
{
	int	argc = 0;		/* definition argument counter */
	int	cin, cout;		/* in and out character counts */
	int	argnum;			/* argument number */
	char	*macval;		/* macro string value */
	struct passwd *passwd;		/* passwd struct for getpwuid() */
	static char	buf[RX_MAXARGSZ]; /* static buffer for %* macro */

	switch(macro) {

	case 'm': /* client machine network address */
		if ((macval = getenv("NLSADDR")) == NULL)
			return("");
		else
			return(macval);
		break;

	case 't': /* transport provider */
		if ((macval = getenv("NLSPROVIDER")) == NULL)
			return("");
		else
			return(macval);
		break;

	case 's': /* shell */
		if ((passwd = getpwuid(getuid())) != (struct passwd *) NULL) {
			macval = passwd->pw_shell;
			if (strcmp(macval, "") != 0) {
				endpwent();
				return(macval);
			}
		}
		endpwent();
		return("/bin/sh");
		break;

	case '*': /* pass all the service parameters */
		argnum = 0;
		cout = 0;
		while(Target.argv[argnum] != NULL) {
			strcpy(buf + cout, Target.argv[argnum]);
			cout += strlen(Target.argv[argnum]);
			strcpy(buf + cout, " ");
			cout++;
			argnum++;
		}
		if (argnum > 0)
			cout--; /* ignore blank at end */
		buf[cout] = '\0';
		return(buf);
		break;

	case '0': /* argument 0 is the service name */
		return(Target.service);
		break;

	default: /* rexec command arguments or unknown macros */
		if ((macro >= '1') && (macro <= '9')) {
			argnum = macro - '1';
			return(Target.argv[argnum]);
		} else {
			buf[0] = '%';
			buf[1] = (char) macro;
			buf[2] = '\0';
			return(buf);
		}
		break;

	} /* switch */
}


int
readuntil(fp, bufp, endchar, buflen, expand)
FILE	*fp;		/* file pointer to read from */
char	**bufp;		/* modifiable buffer pointer to read into */
int	endchar;	/* character to read until */
int	*buflen;	/* modifiable maximum buffer length */
int	expand;		/* expand macros flag */
{
	int	c,c2;	/* character being read in */
	int	r;	/* readuntil() return value */
	char	*macvalue;	/* expanded macro value */
	int	maclen;		/* length of expanded macro */

	while(1) {
		c = getc(fp);

		if ((endchar != 0) && (c == endchar))
			return(0);

		switch(c) {
		default:
			if (*buflen > 1) {
				**bufp = (char) c;
				(*bufp)++;
			}

			/*
			 *	the following statement has this effect:
			 *
			 *	if buflen > 1,
			 *		decrement buflen to reflect char copied
			 *
			 *	if buflen == 1,
			 *		decrement buflen to reflect NULL which
			 *		will be copied at EOF, EOLN, or TAB
			 *
			 *	if buflen <= 0,
			 *		decrement buflen to flag truncation
			 *
			 */

			(*buflen)--;
			break;

		case EOF:
			**bufp = '\0';
			if (endchar == 0)
				return(RX_SCAN_EOF | RX_SCAN_LAST);
			else
				return(RX_SCAN_ERROR | RX_SCAN_EOF |
				       RX_SCAN_LAST);

		case EOLN:
			if (endchar == 0) {
				**bufp = '\0';
				return(RX_SCAN_LAST);
			}

			if (*buflen > 1) {
				**bufp = (char) c;
				(*bufp)++;
				(*buflen)--;
			}
			break;

		case TAB:
			if (endchar == 0) {
				**bufp = '\0';
				return(0);
			}

			if (*buflen > 1) {
				**bufp = (char) c;
				(*bufp)++;
				(*buflen)--;
			}
			break;

		case QUOTE1:
		case QUOTE2:
			if ((r = readuntil(fp, bufp, c, buflen, DONTEXPAND)) != 0)
				return(r);
			break;

		case BACKSLASH:
			c2 = getc(fp);

			if (c2 == EOF) {
				**bufp = '\0';
				return(RX_SCAN_EOF | RX_SCAN_LAST);
			}

			if (*buflen > 1) {
				**bufp = (char) c2;
				(*bufp)++;
				(*buflen)--;
			}
			break;

		case PERCENT:
			if (!expand) {
				if (*buflen > 1) {
					**bufp = (char) c;
					(*bufp)++;
					(*buflen)--;
				}
			} else {
				c2 = getc(fp);

				if (c2 == EOF) {
					**bufp = '\0';
					return(RX_SCAN_EOF | RX_SCAN_LAST);
				}

				macvalue = getmacro(c2);
				maclen = strlen(macvalue);
				(void) strncpy(*bufp, macvalue, *buflen - 1);
				if (maclen > (*buflen - 1)) {
					*bufp += (*buflen - 1);
					**bufp = '\0';
					*buflen = 0;
				} else {
					*bufp += maclen;
					*buflen -= maclen;
				}
			}
			break;

		} /* switch(c) */
	} /* while(1) */
}
