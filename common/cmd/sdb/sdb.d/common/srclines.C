#ident	"@(#)sdb:sdb.d/common/srclines.C	1.5"

#include <stdio.h>
#include "utility.h"
#include <string.h>
#include "Interface.h"
#include <sys/euc.h>
#include <getwidth.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include "MBsize.h"

static int compclass(char *, int, char *);
static cclass(char *, char **, int);
extern int mbtowc(wchar_t *, const char *, size_t);

/*
** These procedures manage the source files examined by debug.
*/

/* Print the current line. */
void fprint()
{
	SrcFile *sf;
	long currline;

	sf=find_srcfile(current_src(current_process,&currline));

	printx("%ld:\t%s",currline,src_text(sf,currline));
}

/*
**	Make line `num' current.
*/
void ffind(long num)
{
	SrcFile *sf;
	char *fname;
	long currline, numlines;

	fname=current_src(current_process,&currline);
	sf=find_srcfile(fname);

	numlines=num_lines(sf);

	if(num>=1&&num<=numlines)
		set_current_src(current_process,fname,num);
	else
		printe("only %ld lines in `%s'\n",numlines,fname);
}

/* Go back n lines. */
void fback(long n)
{
	SrcFile *sf;
	char *fname;
	long currline;

	fname=current_src(current_process,&currline);
	sf=find_srcfile(fname);

	currline -= n;
	if(currline < 1)
		currline = 1;

	set_current_src(current_process,fname,currline);
}

/* Go forwards n lines. */
void fforward(long n)
{
	SrcFile *sf;
	char *fname;
	long currline;

	fname=current_src(current_process,&currline);
	sf=find_srcfile(fname);

	currline += n;
	if(currline > num_lines(sf))
		currline = num_lines(sf);

	set_current_src(current_process,fname,currline);
}

/* Print n lines. */
fprintn(int n)
{
	SrcFile *sf;
	long fline, lline;
	char *fname;

	fname=current_src(current_process,&fline);
	sf=find_srcfile(fname);

	lline=fline+n-1;

	if(lline>num_lines(sf)) {
		lline = num_lines(sf);
		n=lline-fline+1;
	}

	while (fline<=lline) {
		printx("%ld:\t%s",fline,src_text(sf,fline));
		fline++;
	}
	set_current_src(current_process,fname,lline);

	return n;
}

#define	CBRA	1
#define	CCHR	2
#define	CDOT	4
#define	CCL	6
#define	NCCL	8
#define	CDOL	10
#define	CCEOF	11	/*  CEOF --> CCEOF (conflict with <termio.h>) */
#define	CKET	12
#define	CBACK	18

#define CXCL	20
#define MCCHR	22
#define MCCL	24
#define NMCCL	36

#define	CSTAR	01

#define	LBSIZE	512
#define	ESIZE	256
#define	NBRA	9

#define ISTHERE(c)	(ep[c>>3] & bittab[c & 07])
#define chrlngth(c)	((unsigned char)(c) < 0200 ? 1 : (ISSET2(c) ? \
	wp._eucw2 + 1 : (ISSET3(c) ? wp._eucw3 + 1 : wp._eucw1)))

extern eucwidth_t wp;
static unsigned char	expbuf[ESIZE];
static int	circf;
static char	*braslist[NBRA];
static char	*braelist[NBRA];
static unsigned short	bittab[] = {
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128
};

static int ecmp(char *, char *, int);
static int advance(register char *lp, register char *ep);
static int match(char *p1);
static void compile(char *astr);

int
dore(char *nre, int redir)
{
	register SrcFile *sf;
	register char *p, *pp, *fname;
	long cline,tline,mline;
	static char re[256];

	circf = 0;

	if(
		(fname=current_src(current_process,&cline))==0
			||
		(sf=find_srcfile(fname))==0
			||
		(mline=num_lines(sf))<=0
	) {
		printe("no source file\n");
		return 0;
	}

	if(nre[0]!='\0')
		strcpy(re,nre);

	compile(re);

	tline=cline;
	do {
		if(redir) {
			if(tline==mline)
				tline=1;
			else
				tline+=1;
		}
		else {
			if(tline==1)
				tline=mline;
			else
				tline-=1;
		}
		p = src_text(sf,tline);
		pp = p;
		while (*pp++ != '\n')
			;
		*--pp = '\0';

		if (match(p)) {
			set_current_src(current_process,fname,tline);
			fprint();
			return 1;
		}
	} while (tline!=cline);
	printe("No match\n");
	return 0;
}

static void compile(char *astr)
{
	register c;
	register unsigned char *ep, *sp;
	char *lastep;
	int cclcnt;
	char bracket[NBRA], *bracketp;
	int closed;
	char numbra;
	char neg;
	int lc;
	int i, iflag;
	register unsigned char *mcstart;
	register int  length;
	ep = expbuf;
	sp = (unsigned char *)astr;
	lastep = 0;
	bracketp = bracket;
	closed = numbra = 0;
	if (*sp == '^') {
		circf++;
		sp++;
	}
	memset((char*)expbuf, 0, ESIZE);
	for (;;) {
		if (ep >= &expbuf[ESIZE])
			goto cerror;
		if ((c = *sp++) != '*')
			lastep = ep;
		switch (c) {

		case '\0':
			*ep++ = CCEOF;
			return;

		case '.':
			*ep++ = CDOT;
			continue;

		case '*':
			if (lastep==0 || *lastep==CBRA || *lastep==CKET)
				goto defchar;
			*lastep |= CSTAR;
			continue;

		case '$':
			if (*sp != '\0')
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			mcstart = ep + 18;
			if(&ep[17] >= &expbuf[ESIZE])
				goto cerror;
			*ep++ = CCL;
			lc = 0;
			for(i = 0; i < 16; i++)
				ep[i] = 0;
			neg = 0;
			if((c = *sp++) == '^') {
				neg = 1;
				c = *sp++;
			}
			iflag = 1;
			do {
				c &= 0377;
				if (c=='\0')
					goto cerror;
				if ((c & 0200) && iflag) {
					iflag = 0;
					if (compclass(ep,neg, &expbuf[ESIZE]) == -1)
						goto cerror;
				}
				if (c=='-' && lc != 0) {
				  if (*sp == ']')  {
					ep[c>>3] |= bittab[c&07];
					c = *sp++;
					break;
				  }
				  if ((*sp & 0200) && iflag) {
					iflag = 0;
					if (compclass(ep, neg, &expbuf[ESIZE]) == -1)
						goto cerror;
				  }
				  if (!wp._multibyte || !(lc & 0200) && !(*sp & 0200)) {
					for (c = lc; c<*((char*)sp); c++)
						ep[c>>3] |= bittab[c&07];
				  }
				  else if ((lc == *sp) ||
					 (lc >= 0200 && *sp >= 0200)) {
					 if ((lc == SS2 || *sp == SS2 ||
					      lc == SS3 || *sp == SS3)
					   && lc != *sp)
						;
					 else
						*mcstart++ = '-';
				 }
				 c = *sp++;
				}
				lc = c;
				if (!wp._multibyte || !(c & 0200))
					ep[c>>3] |= bittab[c&07];
				else {
					length = chrlngth(c);
					if (mcstart + length >= &expbuf[ESIZE])
						goto cerror;
					*mcstart++ = c;
					while( --length)
						*mcstart++ = *sp++;
				}
			} while((c = *sp++) != ']');
			if (iflag)
				iflag = 16;
			else if(!wp._multibyte)
				iflag = 32;
			else
				neg = 0;
			if(neg) {
				if (iflag == 32) {
					for(cclcnt = 0; cclcnt < iflag; cclcnt++)
						ep[cclcnt] ^= 0377;
					ep[0] &= 0376;
				} else {
					ep[-1] = NCCL;
					ep[0] |= 01;
				}
			}

			if (iflag)
				ep += iflag;
			else {
				ep += 16;
				if(mcstart - ep > 255)
					goto cerror;
				*ep = mcstart - ep;
				ep = mcstart;
			}

			continue;

		case '\\':
			if((c = *sp++) == '(') {
				if(numbra >= NBRA) {
					goto cerror;
				}
				*bracketp++ = numbra;
				*ep++ = CBRA;
				*ep++ = numbra++;
				continue;
			}
			if(c == ')') {
				if(bracketp <= bracket) {
					goto cerror;
				}
				*ep++ = CKET;
				*ep++ = *--bracketp;
				closed++;
				continue;
			}

			if(c >= '1' && c <= '9') {
				if((c -= '1') >= closed)
					goto cerror;
				*ep++ = CBACK;
				*ep++ = c;
				continue;
			}

		defchar:
		default:
			lastep = ep;
			if (wp._multibyte && (c&0200)) {
				length = chrlngth(c);
				if (ep + length >= &expbuf[ESIZE])
					goto cerror;
				*ep++ = MCCHR;
				*ep++ = c;
				while (--length)
					*ep++ = *sp++;
			} else {
				*ep++ = CCHR;
				*ep++ = c;
			}
		}
	}
    cerror:
	printe("RE error\n", (char *)NULL);
}

static int compclass(register char *ep, int neg, register char *endbuf)
{
	register int i;
	if (wp._multibyte) {
		if(neg) {
			ep[-1] = NMCCL;
			ep[0] |= 01;
		} else
			ep[-1] = MCCL;
	} else {
		if(&ep[32] >= endbuf)
			return(-1);
		ep[-1] = CXCL;
		for (i = 16; i < 32; i++)
			ep[i] = 0;
	}
	return 0;
}

static int match(char *p1)
{
	register char *p2;
	register c;
		p2 = expbuf;
		if (circf) {
			if (advance(p1, p2))
				goto found;
			goto nfound;
		}
		/* fast check for first character */
		if (*p2==CCHR) {
			c = p2[1];
			do {
				if (*p1!=c)
					continue;
				if (advance(p1, p2))
					goto found;
			} while (*p1++);
			goto nfound;
		}
		/* regular algorithm */
		do {
			if (advance(p1, p2))
				goto found;
			if (*p1)
				p1 += chrlngth(*p1);
			else
				goto nfound;
		} while (1);
	nfound:
		return(0);
	found:
		return(1);
}

static int advance(register char *lp, register char *ep)
{
	char *cp;
	int neg, length;
	wchar_t c, d;
	int llp, lep;
	register char *curlp;
	char *bbeg;
	int ct;

	for (;;) {
	neg = 0;
	switch (*ep++) {
	case CCHR:
		if (*ep++ == *lp++)
			continue;
		return(0);

	case MCCHR:
		length = chrlngth(*ep);
		while(length--)
			if(*ep++ != *lp++)
				return(0);
		continue;
	case CDOT:
		if (c = *lp) {
			lp += chrlngth(c);
			continue;
		}
		return(0);

	case CDOL:
		if (*lp=='\0')
			continue;
		return(0);

	case CCEOF:
		return(1);

	case CXCL:
		c = (unsigned char)*lp++;
		if(ISTHERE(c)) {
			ep += 32;
			continue;
		}
		return(0);

	case NCCL:
		c = *lp;
		if((c & 0200) || !ISTHERE(c)) {
			ep += 16;
			lp += chrlngth(c);
			continue;
		}
		return(0);

	case CCL:
		c = (unsigned char)*lp++;
		if((c < 0200) && ISTHERE(c)) {
			ep += 16;
			continue;
		}
		return(0);
	case NMCCL:
		neg = 1;

	case MCCL:
		cp = lp;
		if(!cclass(ep, &cp, neg))
			return(0);
		ep += *(ep + 16) + 16;
		lp = cp;
		continue;

	case CBRA:
		braslist[*ep++] = lp;
		continue;

	case CKET:
		braelist[*ep++] = lp;
		continue;

	case CBACK:
		bbeg = braslist[*ep];
		if (braelist[*ep]==0)
			return(0);
		ct = braelist[*ep++] - bbeg;
		if(ecmp(bbeg, lp, ct)) {
			lp += ct;
			continue;
		}
		return(0);

	case CBACK|CSTAR:
		bbeg = braslist[*ep];
		if (braelist[*ep]==0)
			return(0);
		ct = braelist[*ep++] - bbeg;
		curlp = lp;
		while(ecmp(bbeg, lp, ct))
			lp += ct;
		while(lp >= curlp) {
			if(advance(lp, ep))	return(1);
			lp -= ct;
		}
		return(0);


	case CDOT|CSTAR:
		curlp = lp;
		while (*lp++);
		goto mstar;

	case CCHR|CSTAR:
		curlp = lp;
		while (*lp++ == *ep);
		ep++;
		goto star;

	case MCCHR | CSTAR:
		curlp = lp;
		if ((lep = mbtowc(&c, ep, MB_LEN_MAX)) == -1) {
			fprintf(stdout, "Bad Multibyte Character\n");
			exit(1);
		} else {
		ep += (lep ? lep : 1);
		}
		do {
			if ((llp = mbtowc(&d, lp, MB_LEN_MAX)) == -1) {
				fprintf(stdout, "Bad Multibyte Character\n");
				exit(1);
			} else {
			lp += (llp ? llp : 1);
			}
		} while(d == c);
		goto mstar;

	case NMCCL | CSTAR:
		neg = 1;

	case MCCL | CSTAR:
		curlp = cp = lp;
		while(cclass(ep, &cp, neg));
		lp = cp;
		ep += *(ep + 16) +16;
		goto mstar;

	case CXCL |CSTAR:
		curlp = lp;
		do {
			c = (unsigned char)*lp++;
		} while(ISTHERE(c));
		ep += 32;
		goto star;

	case NCCL | CSTAR:
		curlp = lp;
		do {
			c = (unsigned char)*lp++;
		} while(c >= 0200 || !ISTHERE(c));
		ep += 16;
		goto mstar;


	case CCL | CSTAR:
		curlp = lp;
		do {
			c = (unsigned char)*lp++;
		} while(c < 0200 && ISTHERE(c));
		ep += 16;
		goto star;

	mstar:
		if(!wp._multibyte)
			goto star;
		do {
			register char *p1, *p2;
			lp--;
			p1 = lp - wp._eucw2;
			p2 = lp - wp._eucw3;
			if((unsigned char)*lp >= 0200) {
				if(p1 >= curlp && (unsigned char)*p1 == SS2)
					lp = p1;
				else if(p2 >= curlp && (unsigned char)*p2 == SS3)
					lp = p2;
				else
					lp = lp - wp._eucw1 + 1;
			}
			if(advance(lp,ep))
				return(1);
		} while (lp > curlp);
		return(0);


	star:
		if(--lp == curlp) {
			continue;
		}

		if(*ep == CCHR) {
			c = ep[1];
			do {
				if(*lp != c)
					continue;
				if(advance(lp, ep))
					return(1);
			} while(lp-- > curlp);
			return(0);
		}

		do {
			if (advance(lp, ep))
				return(1);
		} while (lp-- > curlp);
		return(0);

	default:
		printe("RE botch\n");
	}
	}
}

static int ecmp(register char *a, register char *b, register int cc)
{
	while(cc--)
		if(*a++ != *b++)	return(0);
	return(1);
}

static cclass(register char *ep, register char **cp, int neg)
{
	register char *lp;
	wchar_t c, d, f;
	int llp, lep;

	lp = *cp;
	f = (wchar_t)0;
	c = (unsigned char)*lp;
	if((c & 0200) == 0) {
		*cp = lp + 1;
		if(ISTHERE(c) && neg || !ISTHERE(c) && !neg)
			return(0);
	} else {
		ep += 17;
		if ((llp = mbtowc(&c, lp, MB_LEN_MAX)) == -1) {
			fprintf(stdout, "Bad Multibyte Character\n");
			exit(1);
		} else {
			lp += (llp ? llp : 1);
		}
		if ((lep = mbtowc(&d, ep, MB_LEN_MAX)) == -1) {
			fprintf(stdout, "Bad Multibyte Character\n");
			exit(1);
		} else {
			ep += (lep ? lep : 1);
		}
		*cp = lp;
		while(1) {
			if (d < 0200) {
				if(d != '-') {
					if(!neg)
						return(0);
					break;
				}
				if ((lep = mbtowc(&d, ep, MB_LEN_MAX)) == -1) {
					fprintf(stdout,"Bad Multibyte Character\n");
					exit(1);
				} else {
					ep += (lep ? lep : 1);
				}
				if(f <= c && c <= d) {
					if(neg)
						return(0);
					break;
				}
			}
			if(d == c) {
				if(neg)
					return(0);
				break;
			}
			f = d;
			if ((lep = mbtowc(&d, ep, MB_LEN_MAX)) == -1) {
				fprintf(stdout,"Bad Multibyte Character\n");
				exit(1);
			} else {
				ep += (lep ? lep : 1);
			}
		}
	}
	return(1);
}
