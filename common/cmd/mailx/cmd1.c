/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:cmd1.c	1.9.2.9"
#ident "@(#)cmd1.c	1.20 'attmail mail(1) command'"
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
 * User commands.
 */

static void	brokpipe ARGS((int));
static char	*dispname ARGS((char*hdr));
static int	doheaders ARGS((int *msgvec, int limit));
static void	print ARGS((struct message *mp, FILE *obuf, int doign, int ignore_binary));
static int	screensize ARGS((void));
static int	type1 ARGS((int *msgvec, int doign, int ignore_binary));
static int	top1 ARGS((int *msgvec, int ignore_binary));
#ifdef SVR4ES
static int	subjcount ARGS((char *subjp, int scrsize));
#endif

/*
 * Print the current active headings.
 * Don't change dot if invoker didn't give an argument.
 */

static int curscreen = 0, oldscreensize = 0;

headers(msgvec)
	int *msgvec;
{
	return doheaders(msgvec, 0);
}

Headers(msgvec)
	int *msgvec;
{
	return doheaders(msgvec, 1);
}

static int
doheaders(msgvec, unlimitedhdr)
	int *msgvec;
	int unlimitedhdr;
{
	register int n, mesg, flag;
	register struct message *mp, *eom = &message[msgCount];
	int size;

	size = screensize();
	n = msgvec[0];
	if (n != 0) {
		curscreen = (n-1)/size;
		dot = &message[n-1];
	}
	if (curscreen < 0)
		curscreen = 0;
	mp = &message[curscreen * size];
	if (mp >= eom)
		mp = &message[msgCount - size];
	if (mp < &message[0])
		mp = &message[0];
	while((mp < eom) && (mp->m_flag & MDELETED))
		mp++;
	flag = 0;
	mesg = mp - &message[0];
	if (dot >= eom) dot = eom - 1;
	if (dot <  mp)  dot = mp;
	if (Hflag)
		mp = message;
	for (; mp < eom; mp++) {
		mesg++;
		if (mp->m_flag & MDELETED)
			continue;
		if (flag++ >= size && !Hflag)
			break;
		printhead(mesg, unlimitedhdr);
		sreset();
	}
	if (flag == 0) {
		pfmt(stdout, MM_NOSTD, ":176:No more mail.\n");
		return(1);
	}
	return(0);
}

/*
    Show the given headers for a given list of messages.
*/
showheaders(str)
	char str[];
{
	int *msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	int f;
	char *hdrs = snarf2(str, &f);
	char **hdrlist;
	char **q;

	/* parse message list */
	if (f) {
		if (getmsglist(str, msgvec, 0) < 0)
			return(1);
	} else {
		msgvec[0] = first(0, MMNORM);
		if (*msgvec == NULL) {
			pfmt(stderr, MM_ERROR, ":189:No messages applicable\n");
			return(1);
		}
		msgvec[1] = NULL;
	}

	/* set pointers into the header list */
	if (!hdrs) {
		pfmt(stderr, MM_ERROR, ":498:No applicable headers\n");
		return(1);
	} else {
		char *p;
		unsigned hdrcount = 1;
		for (p = hdrs; *p; p++)
			if (isspace(*p))
				hdrcount++;
		hdrlist = (char**)salloc(hdrcount + 1);
		hdrlist[0] = hdrs;
		for (p = hdrs, q = hdrlist+1; *p; )
			if (isspace(*p)) {
				*p++ = '\0';
				p = (char*)skipspace(p);
				if (*p)
					*q++ = p;
			} else
				p++;
		*q = 0;
	}

	/* print the given headers for each message */
	for ( ; msgvec[0] != 0; msgvec++) {
		printf("Message: %d\n", msgvec[0]);
		for (q = hdrlist; *q; q++) {
			struct message *mp = &message[msgvec[0]-1];
			FILE *ibuf = setinput(mp);
			char linebuf[LINESIZE];
			long lc = mp->m_lines;
			int gfrom = strcmp(*q, ">From") == 0;

			if (lc <= 0)
				continue;

			/* skip "From " header (gethfield() can't handline it) */
			if (readline(ibuf, linebuf) < 0)
				continue;
			else if (gfrom)
				puts(linebuf);

			lc--;
			while ((lc = gethfield(ibuf, linebuf, lc)) >= 0)
				if ((gfrom && strncmp(linebuf, ">From ", 6) == 0) ||
				    ishfield(linebuf, *q))
					puts(linebuf);
		}
		putchar('\n');
	}
	return(0);
}

/*
 * Scroll to the next/previous screen
 */

scroll(arg)
	char arg[];
{
	register int s, size;
	int cur[1];

	cur[0] = 0;
	size = screensize();
	s = curscreen;
	switch (*arg) {
	case 0:
	case '+':
		s++;
		if (s * size > msgCount) {
			pfmt(stdout, MM_NOSTD,
				":177:On last screenful of messages\n");
			return(0);
		}
		curscreen = s;
		break;

	case '-':
		if (--s < 0) {
			pfmt(stdout, MM_NOSTD,
				":178:On first screenful of messages\n");
			return(0);
		}
		curscreen = s;
		break;

	default:
		pfmt(stderr, MM_ERROR,
			":179:Unrecognized scrolling command \"%s\"\n", arg);
		return(1);
	}
	return(headers(cur));
}

/*
 * Compute what the screen size should be.
 * We use the following algorithm:
 *	If user specifies with screen option, use that.
 *	If baud rate < 1200, use  5
 *	If baud rate = 1200, use 10
 *	If baud rate > 1200, use 20
 */
static int
screensize()
{
	register char *cp;
	register int newscreensize, tmp;

	cp = value("screen");
	newscreensize = ((cp != NOSTR) && ((tmp = atoi(cp)) > 0)) ? tmp :
			(baud < B1200) ? 5 :
			(baud == B1200) ? 10 :
			20;

	/* renormalize the value of curscreen */
	if (newscreensize != oldscreensize) {
		curscreen = curscreen * oldscreensize / newscreensize;
		oldscreensize = newscreensize;
	}
	return(newscreensize);
}

/*
 * Print out the headlines for each message
 * in the passed message list.
 */

from(msgvec)
	int *msgvec;
{
	register int *ip;

	/* reset dot */
	for (ip = msgvec; *ip != NULL; ip++)
		;
	if (--ip >= msgvec)
		dot = &message[*ip - 1];
	/* print headings */
	for (ip = msgvec; *ip != NULL; ip++) {
		printhead(*ip, 0);
		sreset();
	}
	return(0);
}

/*
 * Print out the header of a specific message.
 * This is a slight improvement to the standard one.
 */

void
printhead(mesg, unlimitedhdr)
	int mesg, unlimitedhdr;
{
	struct message *mp;
	FILE *ibuf;
	char headline[LINESIZE], *subjline, dispc, curind;
	char *fromline;
	char pbuf[LINESIZE];
	char username[LINESIZE];
	struct headline hl;
	register char *cp;
	int showto;

	mp = &message[mesg-1];
	ibuf = setinput(mp);
	readline(ibuf, headline);
	if ((subjline = hfield("subject", mp, addone)) == NOSTR &&
	    (subjline = hfield("subj", mp, addone)) == NOSTR &&
	    (subjline = hfield("message-status", mp, addone)) == NOSTR)
		subjline = "";

	if (istext((unsigned char*)subjline, (int)strlen(subjline), M_text) == M_binary)
		subjline = "<unprintable>";
	curind = (!Hflag && dot == mp) ? '>' : ' ';
	dispc = ' ';
	showto = 0;
	if ((mp->m_flag & (MREAD|MNEW)) == (MREAD|MNEW))
		dispc = 'R';
	if ((mp->m_flag & (MREAD|MNEW)) == MREAD)
		dispc = 'O';
	if ((mp->m_flag & (MREAD|MNEW)) == MNEW)
		dispc = 'N';
	if ((mp->m_flag & (MREAD|MNEW)) == 0)
		dispc = 'U';
	if (mp->m_flag & MSAVED)
		dispc = 'S';
	if (mp->m_flag & MPRESERVE)
		dispc = 'H';
	if (mp->m_flag & MBOX)
		dispc = 'M';
	parse(headline, &hl, pbuf);

	/*
	 * Netnews interface?
	 */

	if (newsflg) {
	    if ( (fromline=hfield("newsgroups",mp,addone)) == NOSTR && 	/* A */
		 (fromline=hfield("article-id",mp,addone)) == NOSTR ) 	/* B */
		  fromline = "<>";
	    else
		  for(cp=fromline; *cp; cp++) {		/* limit length */
			if( any(*cp, " ,\n")){
			      *cp = '\0';
			      break;
			}
		  }
	/*
	 * else regular.
	 */

	} else {
		fromline = nameof(mp);
		if (value("showto")) {
			char *skinned = skin(fromline);
			if ((samebody(mydomname, skinned) || samebody(mylocalname, skinned)) &&
				((cp = hfield("to", mp, addto)) != 0)) {
				showto = 1;
				fromline = username;
				yankword(cp, fromline, docomma(cp));
			}
		}
		fromline = dispname(fromline);
	}
	if (unlimitedhdr)
		printf("Message: %c%c%3d\n", curind, dispc, mesg);
	else
		printf("%c%c%3d ", curind, dispc, mesg);
	if (showto)
		pfmt(stdout, MM_NOSTD, ":180:To %-15.15s ", fromline);
	else if (unlimitedhdr)
		printf("From: %s", fromline);
	else
		printf("%-18.18s ", fromline);
	if (unlimitedhdr) {
		printf("\n");
		if (mp->m_text != M_binary) {
			printf("Date: %s\nSize: %ld/%ld\nSubject: %s\n\n",
				hl.l_date, mp->m_lines, mp->m_size, subjline);
		} else {
			printf("Date: %s\nSize: binary/%ld\nSubject: %s\n\n",
				hl.l_date, mp->m_size, subjline);
		}
	} else {
#ifdef SVR4ES
		if (mp->m_text != M_binary) {
			pfmt(stdout, MM_NOSTD, ":483:%16.16s %6ld/%-5ld %-.*s\n",
				hl.l_date, mp->m_lines, mp->m_size,
				subjcount(subjline, 23), subjline);
		} else {
			pfmt(stdout, MM_NOSTD, ":182:%16.16s binary/%-5ld %-.*s\n",
				hl.l_date, mp->m_size, subjcount(subjline, 23),
				subjline);
		}
#else
		if (mp->m_text != M_binary) {
			pfmt(stdout, MM_NOSTD, ":509:%16.16s %6ld/%-5ld %-.23s\n",
				hl.l_date, mp->m_lines, mp->m_size, subjline);
		} else {
			pfmt(stdout, MM_NOSTD, ":510:%16.16s binary/%-5ld %-.23s\n",
				hl.l_date, mp->m_size, subjline);
		}
#endif
	}
}

/*
 * Return the full name from an RFC-822 header line
 * or the last two (or one) component of the address.
 */

static char *
dispname(hdr)
	char *hdr;
{
	char *cp, *cp2;

	if (hdr == 0)
		return 0;
	if (((cp = strchr(hdr, '<')) != 0) && (cp > hdr)) {
		*cp = 0;
		if ((*hdr == '"') && ((cp = strrchr(++hdr, '"')) != 0))
			*cp = 0;
		return hdr;
	} else if ((cp = strchr(hdr, '(')) != 0) {
		hdr = ++cp;
		if ((cp = strchr(hdr, '+')) != 0)
			*cp = 0;
		if ((cp = strrchr(hdr, ')')) != 0)
			*cp = 0;
		return hdr;
	}
	cp = skin(hdr);
	if ((cp2 = strrchr(cp, '!')) != 0) {
		while (cp2 >= cp && *--cp2 != '!');
		cp = ++cp2;
	}
	return cp;
}

/*
 * Print out the value of dot.
 */

pdot()
{
	printf("%d\n", dot - &message[0] + 1);
	return(0);
}

/*
 * Print out all the possible commands.
 */

pcmdlist()
{
	register struct cmd *cp;
	register int cc;

	pfmt(stdout, MM_NOSTD, ":185:Commands are:\n");
	for (cc = 0, cp = cmdtab; cp->c_name != NULL; cp++) {
		cc += strlen(cp->c_name) + 2;
		if (cc > 72) {
			printf("\n");
			cc = strlen(cp->c_name) + 2;
		}
		if ((cp+1)->c_name != NOSTR)
			printf("%s, ", cp->c_name);
		else
			printf("%s\n", cp->c_name);
	}
	return(0);
}

/*
 * Type out messages, honor ignored fields.
 */
type(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 1, 0));
}

/*
 * Type out messages, even printing ignored fields.
 */
Type(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 0, 0));
}

/*
 * Type out messages, honor ignored fields, ignore binary content flag.
 */
btype(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 1, 1));
}

/*
 * Type out messages, even printing ignored fields, ignore binary content flag.
 */
Btype(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 0, 1));
}

/*
 * Type out the messages requested.
 */
static jmp_buf	pipestop;

static int
type1(msgvec, doign, ignore_binary)
	int *msgvec;
{
	register *ip;
	register struct message *mp;
	register int mesg;
	register char *cp;
	long nlines;
	FILE *obuf;
	void (*sigint)(), (*sigpipe)();
	int setsigs = 0;

	obuf = stdout;
	if (setjmp(pipestop)) {
		if (obuf != stdout) {
			pipef = NULL;
			npclose(obuf);
		}
		goto ret0;
	}
	if (intty && outtty && (cp = value("crt")) != NOSTR) {
		for (ip = msgvec, nlines = 0; *ip && ip-msgvec < msgCount; ip++)
			nlines += message[*ip - 1].m_lines;
		if (nlines > atoi(cp)) {
			const char *pg = pager();
			if ((obuf = npopen(pg, "w")) == NULL) {
				pfmt(stderr, MM_ERROR, failed, pg, strerror(errno));
				obuf = stdout;
			} else {
				pipef = obuf;
				sigint = sigset(SIGINT, SIG_IGN);
				sigpipe = sigset(SIGPIPE, brokpipe);
				setsigs++;
			}
		}
	}
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;
		print(mp, obuf, doign, ignore_binary);
	}
	if (obuf != stdout) {
		pipef = NULL;
		npclose(obuf);
	}
ret0:
	if (setsigs) {
		sigset(SIGPIPE, sigpipe);
		sigset(SIGINT, sigint);
	}
	return(0);
}

/*
 * Respond to a broken pipe signal --
 * probably caused by user quitting pg.
 */
/* ARGSUSED */
static void
brokpipe(unused)
int unused;
{
	sigrelse(SIGPIPE);
	longjmp(pipestop, 1);
}

static const char localemsg[] =
	":525:\n*** Message locale '%s' does not match current locale '%s' ***\n*** Use \"bprint\", \"Bprint\", or save to a file ***\n";
static const char localemsg2[] =
	":526:\n*** Message locale '%s' does not match current locale '%s' ***\n*** Use \"btop\" to see top of message ***\n";


/*
 * Print the indicated message on standard output.
 */

static void
print(mp, obuf, doign, ignore_binary)
	register struct message *mp;
	FILE *obuf;
{
	if (!doign || !isign("message")) {
		pfmt(obuf, MM_NOSTD, ":186:Message %2d:\n", mp - &message[0] + 1);
		fflush(obuf);
	}
	touch(mp - &message[0] + 1);
	if (mp->m_text == M_text || ignore_binary) {
		send(mp, obuf, doign, 0, 1);
	} else if (mp->m_text == M_gtext) {
		char *locale = getenv("LC_CTYPE");
		char *letlocale = mp->m_encoding_type;
		if (!locale)
		    locale = getenv("LANG");
		if (!locale || (strcmp(letlocale, locale) != 0)) {
			pfmt(obuf, MM_NOSTD, localemsg, letlocale,
			    locale ? locale : "Unknown");
			fflush(obuf);
		} else
			send(mp, obuf, doign, 0, 1);
	} else {
		putc('\n', obuf);
		pfmt(obuf, MM_NOSTD, ":522:*** Message content is not printable ***\n*** Use \"bprint\", \"Bprint\", or save to a file ***\n");
		fflush(obuf);
	}
}

/*
 * Print the top so many lines of each desired message.
 * The number of lines is taken from the variable "toplines"
 * and defaults to 5.
 */

top(msgvec)
	int *msgvec;
{
	return top1(msgvec, 0);
}

btop(msgvec)
	int *msgvec;
{
	return top1(msgvec, 1);
}

static top1(msgvec, ignore_binary)
	int *msgvec;
{
	register int *ip;
	register struct message *mp;
	register int mesg;
	int topl, lineb;
	long c, lines;
	char *valtop, linebuf[LINESIZE];
	FILE *ibuf;

	topl = 5;
	valtop = value("toplines");
	if (valtop != NOSTR) {
		topl = atoi(valtop);
		if (topl <= 0 || topl > 10000)
			topl = 5;
	}
	lineb = 1;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		int doprint = 0;
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;
		if (value("quiet") == NOSTR)
			pfmt(stdout, MM_NOSTD, ":187:Message %d:\n", mesg);
		if (mp->m_text == M_text || ignore_binary)  {
			doprint = 1;
		} else if (mp->m_text == M_gtext) {
			char *locale = getenv("LC_CTYPE");
			char *letlocale = mp->m_encoding_type;
			if (!locale)
			    locale = getenv("LANG");
			if (!locale || (strcmp(letlocale, locale) != 0)) {
			    pfmt(stderr, MM_NOSTD, localemsg2, letlocale,
				locale ? locale : "Unknown");
			} else
			    doprint = 1;
		} else {
			putc('\n',stderr);
			pfmt(stderr, MM_NOSTD, ":523:*** Message content is not printable ***\n*** Use \"btop\" to see top of message ***\n");
			putc('\n',stderr);
		}
		if (doprint) {
			ibuf = setinput(mp);
			c = mp->m_lines;
			if (!lineb)
				printf("\n");
			for (lines = 0; lines < c && lines < topl; lines++) {
				if (readline(ibuf, linebuf) <= 0)
					break;
				puts(linebuf);
				lineb = blankline(linebuf);
			}
		}
	}
	return(0);
}

/*
 * Touch all the given messages so that they will
 * get mboxed.
 */

stouch(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * Pretend all the given messages have never been seen before (are "new").
 */

New(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag = MUSED|MNEW;
	}
	return(0);
}

/*
 * Make sure all passed messages get mboxed.
 */

mboxit(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH|MBOX;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * List the folders the user currently has.
 */
folders()
{
	char dirname[BUFSIZ], cmd[BUFSIZ];

	if (getfold(dirname) < 0) {
		pfmt(stderr, MM_ERROR, ":188:No value set for \"folder\"\n");
		return(-1);
	}
	sprintf(cmd, "%s %s", lister(), dirname);
	return(system(cmd));
}

/* Count the number of bytes to make up "scrsize" characters, taking
   into consideration the international character sets. */
#ifdef SVR4ES
static int
subjcount(subjp, scrsize)
register char *subjp;
int scrsize;
{
	register int eucw=0, scrw=0;
	register int neucw, nscrw;

	while (*subjp != '\0') {
		if (ISASCII(*subjp)) {
			neucw = nscrw = 1;
		} else {
			if (ISSET2(*subjp)) {
				neucw = wp._eucw2;
				nscrw = wp._scrw2;
			} else if (ISSET3(*subjp)) {
				neucw = wp._eucw3;
				nscrw = wp._scrw3;
			} else {
				neucw = wp._eucw1;
				nscrw = wp._scrw1;
			}
		}

		if (scrw + nscrw > scrsize)
			break;

		eucw += neucw;
		scrw += nscrw;
		subjp += neucw;
	}

	return(eucw);
}
#endif
