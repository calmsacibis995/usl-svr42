/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fgrep:fgrep.c	1.13.1.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fgrep/fgrep.c,v 1.1 91/02/28 17:08:50 ccs Exp $"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */
/*
 * fgrep -- print all lines containing any of a set of keywords
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>

#include	<sys/euc.h>
#include	<getwidth.h>
#define	Wgetwidth()	{getwidth(&WW); WW._eucw2++; WW._eucw3++;}
eucwidth_t WW;
#define WIDTH1	WW._eucw1
#define WIDTH2	WW._eucw2
#define WIDTH3	WW._eucw3
#define MULTI_BYTE	WW._multibyte
#define GETONE(lc, p)	cw = ISASCII(lc = *p++) ? 1 :			\
			     (ISSET2(lc) ? WIDTH2 :			\
			     (ISSET3(lc) ? WIDTH3 : WIDTH1 ));		\
			if(--cw > --ccount) {				\
				cw -= ccount;				\
				while(ccount--)				\
					lc = (lc << 7) | ((*p++) & 0177); \
				if (p == &buf[2*BUFSIZ]) p = buf;	\
				if (p > &buf[BUFSIZ]) {			\
					if ((ccount = fread(p, sizeof(char), &buf[2*BUFSIZ] - p, fptr)) <= 0) break; \
				}					\
				else if ((ccount = fread(p, sizeof(char), BUFSIZ, fptr)) <= 0) break; \
				blkno += (long)ccount;			\
			}						\
			ccount -= cw;					\
			while(cw--)					\
				lc = (lc << 7) | ((*p++) & 0177)

/* The same() macro and letter() function were inserted to allow for the -i option */

#define letter(a) (isupper(a) ? tolower(a) : (a))
#define same(pat, src) ((pat == src) || (iflag && (!MULTI_BYTE || ISASCII(pat)) && ((pat ^ src) == ' ') && letter(pat) == letter(src)))

#define	MAXSIZ 6000
#define QSIZE 400
struct words {
	unsigned long inp;
	char	out;
	struct	words *nst;
	struct	words *link;
	struct	words *fail;
} w[MAXSIZ], *smax, *q;

FILE *fptr;
long	lnum;
int	bflag, cflag, lflag, fflag, nflag, vflag, xflag, eflag;
int	hflag, iflag;
int	retcode = 0;
int	nfile;
long	blkno;
int	nsucc;
long	tln;
FILE	*wordf;
char	*argptr;
extern	char *optarg;
extern	int optind;
void exit();

static char badopen[] = ":92:Cannot open %s: %s\n";

main(argc, argv)
char **argv;
{
	register c;
	char *usage, *usageid;
	int errflg = 0;
	usage = "[ -bchilnvx ] [ -e exp ] [ -f file ] [ strings ] [ file ] ...";
	usageid = ":190";

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:fgrep");

	while(( c = getopt(argc, argv, "hybcie:f:lnvx")) != EOF)
		switch(c) {

		case 'h':
			hflag++;
			continue;
		case 'b':
			bflag++;
			continue;

		case 'i':
		case 'y':
			iflag++;
			continue;

		case 'c':
			cflag++;
			continue;

		case 'e':
			eflag++;
			argptr = optarg;
			continue;

		case 'f':
			fflag++;
			wordf = fopen(optarg, "r");
			if (wordf==NULL) {
				pfmt(stderr, MM_ERROR, badopen, optarg,
					strerror(errno));
				exit(2);
			}
			continue;

		case 'l':
			lflag++;
			continue;

		case 'n':
			nflag++;
			continue;

		case 'v':
			vflag++;
			continue;

		case 'x':
			xflag++;
			continue;

		case '?':
			errflg++;
	}

	argc -= optind;
	if (errflg || ((argc <= 0) && !fflag && !eflag)) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":191:Usage:\n\tfgrep %s\n",
			gettxt(usageid, usage));
		exit(2);
	}
	if ( !eflag  && !fflag ) {
		argptr = argv[optind];
		optind++;
		argc--;
	}
	Wgetwidth();
	cgotofn();
	cfail();
	nfile = argc;
	argv = &argv[optind];
	if (argc<=0) {
		if (lflag) exit(1);
		execute((char *)NULL);
	}
	else
		while ( --argc >= 0 ) {
			execute(*argv);
			argv++;
		}
	exit(retcode != 0 ? retcode : nsucc == 0);
}

execute(file)
char *file;
{
	register unsigned char *p;
	register struct words *c;
	register ccount;
	unsigned char buf[2*BUFSIZ];
	int failed;
	unsigned char *nlp;
	register unsigned long lc;
	int cw;
	static const char *pref_s, *pref_d, *pref_ld;

	if (file) {
		if ((fptr = fopen(file, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,	file, strerror(errno));
			retcode = 2;
			return;
		}
	}
	else
		fptr = stdin;
	ccount = 0;
	failed = 0;
	lnum = 1;
	tln = 0;
	blkno = 0;
	p = buf;
	nlp = p;
	c = w;
	for (;;) {
		if (ccount <= 0) {
			if (p == &buf[2*BUFSIZ]) p = buf;
			if (p > &buf[BUFSIZ]) {
				if ((ccount = fread(p, sizeof(char), &buf[2*BUFSIZ] - p, fptr)) <= 0) break;
			}
			else if ((ccount = fread(p, sizeof(char), BUFSIZ, fptr)) <= 0) break;
			blkno += (long)ccount;
		}
			GETONE(lc, p);
		nstate:
			if (same(c->inp,lc)) {
				c = c->nst;
			}
			else if (c->link != 0) {
				c = c->link;
				goto nstate;
			}
			else {
				c = c->fail;
				failed = 1;
				if (c==0) {
					c = w;
					istate:
					if (same(c->inp,lc)) {
						c = c->nst;
					}
					else if (c->link != 0) {
						c = c->link;
						goto istate;
					}
				}
				else goto nstate;
			}
		if (c->out) {
			while (lc != '\n') {
				if (ccount <= 0) {
					if (p == &buf[2*BUFSIZ]) p = buf;
					if (p > &buf[BUFSIZ]) {
						if ((ccount = fread(p, sizeof(char), &buf[2*BUFSIZ] - p, fptr)) <= 0) break;
					}
					else if ((ccount = fread(p, sizeof(char), BUFSIZ, fptr)) <= 0) break;
					blkno += (long)ccount;
				   }
				GETONE(lc, p);
			}
			if ( (vflag && (failed == 0 || xflag == 0)) || (vflag == 0 && xflag && failed) )
				goto nomatch;
	succeed:	nsucc = 1;
			if (cflag) tln++;
			else if (lflag) {
				printf("%s\n", file);
				fclose(fptr);
				return;
			}
			else {
				if (nfile > 1 && !hflag)
					printf(pref_s ? pref_s :
						(pref_s = gettxt(":188", "%s:")),
						file);
				if (bflag)
					printf(pref_d ? pref_d :
						(pref_d = gettxt(":192", "%d:")),
						(blkno-(long)(ccount-1))/BUFSIZ);
				if (nflag)
					printf(pref_ld ? pref_ld :
						(pref_ld = gettxt(":189", "%ld:")),
						lnum);
				if (p <= nlp) {
					while (nlp < &buf[2*BUFSIZ]) putchar(*nlp++);
					nlp = buf;
				}
				while (nlp < p) putchar(*nlp++);
			}
	nomatch:	lnum++;
			nlp = p;
			c = w;
			failed = 0;
			continue;
		}
		if (lc == '\n')
			if (vflag) goto succeed;
			else {
				lnum++;
				nlp = p;
				c = w;
				failed = 0;
			}
	}
	fclose(fptr);
	if (cflag) {
		if ((nfile > 1) && !hflag)
			printf(pref_s ? pref_s : (pref_s = gettxt(":188", "%s:")),
				file);
		printf("%ld\n", tln);
	}
}

unsigned long
getargc()
{
			/* appends a newline to shell quoted argument list so */
			/* the list looks like it came from an ed style file  */
	register unsigned long c;
	int cw;
	static int endflg;
	if (wordf) {
		cw = ISASCII(c = getc(wordf)) ? 1 : (ISSET2(c) ? WIDTH2: (ISSET3(c) ? WIDTH3 : WIDTH1 ));
		while(--cw)
			c = (c << 7) | ((fgetc(wordf)) & 0177);
		return(c);
	}
	if (endflg) 
		return(EOF);
	cw = ISASCII(c = (unsigned char)*argptr++) ? 1 : (ISSET2(c) ? WIDTH2 : (ISSET3(c) ? WIDTH3 : WIDTH1 ));
	while(--cw)
		c = (c << 7) | (((unsigned char)*argptr++) & 0177);
	if (c == '\0') {
		endflg++;
		return('\n');
	}
	return iflag ? letter(c) : c;
}

cgotofn() {
	register unsigned long c;
	register struct words *s;

	s = smax = w;
nword:	for(;;) {
		c = getargc();
		if (c==EOF)
			return;
		if (c == '\n') {
			if (xflag) {
				for(;;) {
					if (s->inp == c) {
						s = s->nst;
						break;
					}
					if (s->inp == 0) goto nenter;
					if (s->link == 0) {
						if (smax >= &w[MAXSIZ -1]) overflo();
						s->link = ++smax;
						s = smax;
						goto nenter;
					}
					s = s->link;
				}
			}
			s->out = 1;
			s = w;
		} else {
		loop:	if (s->inp == c) {
				s = s->nst;
				continue;
			}
			if (s->inp == 0) goto enter;
			if (s->link == 0) {
				if (smax >= &w[MAXSIZ - 1]) overflo();
				s->link = ++smax;
				s = smax;
				goto enter;
			}
			s = s->link;
			goto loop;
		}
	}

	enter:
	do {
		s->inp = c;
		if (smax >= &w[MAXSIZ - 1]) overflo();
		s->nst = ++smax;
		s = smax;
	} while ((c = getargc()) != '\n' && c!=EOF);
	if (xflag) {
	nenter:	s->inp = '\n';
		if (smax >= &w[MAXSIZ -1]) overflo();
		s->nst = ++smax;
	}
	smax->out = 1;
	s = w;
	if (c != EOF)
		goto nword;
}

overflo() {
	pfmt(stderr, MM_ERROR, ":193:Wordlist too large\n");
	exit(2);
}
cfail() {
	struct words *queue[QSIZE];
	struct words **front, **rear;
	struct words *state;
	register unsigned long c;
	register struct words *s;
	s = w;
	front = rear = queue;
init:	if ((s->inp) != 0) {
		*rear++ = s->nst;
		if (rear >= &queue[QSIZE - 1]) overflo();
	}
	if ((s = s->link) != 0) {
		goto init;
	}

	while (rear!=front) {
		s = *front;
		if (front == &queue[QSIZE-1])
			front = queue;
		else front++;
	cloop:	if ((c = s->inp) != 0) {
			*rear = (q = s->nst);
			if (front < rear)
				if (rear >= &queue[QSIZE-1])
					if (front == queue) overflo();
					else rear = queue;
				else rear++;
			else
				if (++rear == front) overflo();
			state = s->fail;
		floop:	if (state == 0) state = w;
			if (state->inp == c) {
			qloop:	q->fail = state->nst;
				if ((state->nst)->out == 1) q->out = 1;
				if ((q = q->link) != 0) goto qloop;
			}
			else if ((state = state->link) != 0)
				goto floop;
		}
		if ((s = s->link) != 0)
			goto cloop;
	}
}
