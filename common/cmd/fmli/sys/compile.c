/***********************************************************************
* This file is no longer needed. We have integrated a version of
* compile(), step() and advance() that is able to handle inter-
* nationalised regular expressions. These functions are found in
* the directory ../gen.
************************************************************************
*/
#ifdef notdef
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)fmli:sys/compile.c	1.4.3.3"

#include <stdio.h>

#define GETC(x)		(*x++)
#define PEEKC(x)	(*x)
#define UNGETC(x)	(--x)

#define	CBRA	2
#define	CCHR	4
#define	CDOT	8
#define	CCL	12
#define	CXCL	16
#define	CDOL	20
#define	CCEOF	22
#define	CKET	24
#define	CBACK	36
#define NCCL	40

#define	STAR	01
#define RNGE	03

#define	NBRA	9

#define PLACE(c)	ep[c >> 3] |= bittab[c & 07]
#define ISTHERE(c)	(ep[c >> 3] & bittab[c & 07])
#define ecmp(s1, s2, n)	(!strncmp(s1, s2, n))

/* changed all chars to unsigned char.  abs s17 */

/* dmd TCB */
static int	sed;
/* the number of bracket sets in the current statement being evaluated */
int	nbra;
/* an array of pointers to each starting bracket in the current statement being evaluated */
unsigned char	*braslist[NBRA];
/* an array of pointers to each closing bracket in the current statement being evaluated */
unsigned char	*braelist[NBRA];
/* the current location in the current statement being evaluated */
unsigned char	*loc2;
/* dmd TCB */
static unsigned char	*loc1, *locs;
static int	nodelim;

/* dmd TCB */
static int	circf;
static int	low;
static int	size;

static unsigned char	bittab[] = { 1, 2, 4, 8, 16, 32, 64, 128 }; 

unsigned char *
compile(instring, ep, endbuf, seof, errfunc)
register unsigned char *ep;
unsigned char *instring, *endbuf;
void (*errfunc)();
{
    register unsigned char *sp = instring;
    register c;
    register eof = seof;
    unsigned char *lastep = instring;
    int cclcnt;
    unsigned char bracket[NBRA], *bracketp;
    int closed;
    int neg;
    int lc;
    int i, cflg;
    int iflag;			/* used for non-ascii characters in brackets */

    /*
      (re)initialize globals (necessary so that if compile()
      is called more than once, globals will be reset ... see
      fmlexpr() which references several of these globals)
      */
    sed = nbra = 0;
    braslist[0] = braelist[0] = NULL;
    loc1 = loc2 = locs = NULL;

    lastep = 0;
    if((c = GETC(sp)) == eof || c == '\n') {
	if(c == '\n') {
	    UNGETC(sp);
	    nodelim = 1;
	}
	if(*ep == 0 && !sed)
	    (*errfunc)(41);
	return(ep);
    }
    bracketp = bracket;
    circf = closed = nbra = 0;
    if(c == '^')
	circf++;
    else
	UNGETC(sp);
    while(1) {
	if(ep >= endbuf)
	    (*errfunc)(50);
	c = GETC(sp);
	if(c != '*' && ((c != '\\') || (PEEKC(sp) != '{')))
	    lastep = ep;
	if(c == eof) {
	    *ep++ = CCEOF;
	    if (bracketp != bracket)
		(*errfunc)(42);
	    return(ep);
	}
	switch(c) {

	case '.':
	    *ep++ = CDOT;
	    continue;

	case '\n':
	    if(!sed) {
		UNGETC(sp);
		*ep++ = CCEOF;
		nodelim = 1;
		if(bracketp != bracket)
		    (*errfunc)(42);
		return(ep);
	    }
	    else (*errfunc)(36);
	case '*':
	    if(lastep == 0 || *lastep == CBRA || *lastep == CKET)
		goto defchar;
	    *lastep |= STAR;
	    continue;

	case '$':
	    if(PEEKC(sp) != eof && PEEKC(sp) != '\n')
		goto defchar;
	    *ep++ = CDOL;
	    continue;

	case '[':
	    if(&ep[17] >= endbuf)
		(*errfunc)(50);

	    *ep++ = CCL;
	    lc = 0;
	    for(i = 0; i < 16; i++)
		ep[i] = 0;

	    neg = 0;
	    if((c = GETC(sp)) == '^') {
		neg = 1;
		c = GETC(sp);
	    }
	    iflag = 1;
	    do {
		c &= 0377;
		if(c == '\0' || c == '\n')
		    (*errfunc)(49);
		if((c & 0200) && iflag) {
		    iflag = 0;
		    if(&ep[32] >= endbuf)
			(*errfunc)(50);
		    ep[-1] = CXCL;
		    for(i = 16; i < 32; i++)
			ep[i] = 0;
		}
		if(c == '-' && lc != 0) {
		    if((c = GETC(sp)) == ']') {
			PLACE('-');
			break;
		    }
		    if((c & 0200) && iflag) {
			iflag = 0;
			if(&ep[32] >= endbuf)
			    (*errfunc)(50);
			ep[-1] = CXCL;
			for(i = 16; i < 32; i++)
			    ep[i] = 0;
		    }
		    while(lc < c ) {
			PLACE(lc);
			lc++;
		    }
		}
		lc = c;
		PLACE(c);
	    } while((c = GETC(sp)) != ']');
			
	    if(iflag)
		iflag = 16;
	    else
		iflag = 32;
			
	    if(neg) {
		if(iflag == 32) {
		    for(cclcnt = 0; cclcnt < iflag; cclcnt++)
			ep[cclcnt] ^= 0377;
		    ep[0] &= 0376;
		} else {
		    ep[-1] = NCCL;
		    /* make nulls match so test fails */
		    ep[0] |= 01;
		}
	    }

	    ep += iflag;

	    continue;

	case '\\':
	    switch(c = GETC(sp)) {

	    case '(':
		if(nbra >= NBRA)
		    (*errfunc)(43);
		*bracketp++ = nbra;
		*ep++ = CBRA;
		*ep++ = nbra++;
		continue;

	    case ')':
		if(bracketp <= bracket) 
		    (*errfunc)(42);
		*ep++ = CKET;
		*ep++ = *--bracketp;
		closed++;
		continue;

	    case '{':
		if(lastep == (unsigned char *) 0)
		    goto defchar;
		*lastep |= RNGE;
		cflg = 0;
	    nlim:
		c = GETC(sp);
		i = 0;
		do {
		    if('0' <= c && c <= '9')
			i = 10 * i + c - '0';
		    else
			(*errfunc)(16);
		} while(((c = GETC(sp)) != '\\') && (c != ','));
		if(i >= 255)
		    (*errfunc)(11);
		*ep++ = i;
		if(c == ',') {
		    if(cflg++)
			(*errfunc)(44);
		    if((c = GETC(sp)) == '\\')
			*ep++ = 255;
		    else {
			UNGETC(sp);
			goto nlim;
			/* get 2'nd number */
		    }
		}
		if(GETC(sp) != '}')
		    (*errfunc)(45);
		if(!cflg)	/* one number */
		    *ep++ = i;
		else if((unsigned char)(ep[-1] & (unsigned char)0377) < 
			(unsigned char)(ep[-2] & (unsigned char)0377))
		    (*errfunc)(46);
		continue;

	    case '\n':
		(*errfunc)(36);

	    case 'n':
		c = '\n';
		goto defchar;

	    default:
		if(c >= '1' && c <= '9') {
		    if((c -= '1') >= closed)
			(*errfunc)(25);
		    *ep++ = CBACK;
		    *ep++ = c;
		    continue;
		}
	    }
	    /* Drop through to default to use \ to turn off special chars */

	defchar:
	default:
	    lastep = ep;
	    *ep++ = CCHR;
	    *ep++ = c;
	}
    }
}

int step(p1, p2)
register unsigned char *p1, *p2; 
{
	register c;


	if(circf) {
		loc1 = p1;
		return(advance(p1, p2));
	}
	/* fast check for first character */
	if(*p2 == CCHR) {
		c = p2[1];
		do {
			if(*p1 != c)
				continue;
			if(advance(p1, p2)) {
				loc1 = p1;
				return(1);
			}
		} while(*p1++);
		return(0);
	}
		/* regular algorithm */
	do {
		if(advance(p1, p2)) {
			loc1 = p1;
			return(1);
		}
	} while(*p1++);
	return(0);
}

advance(lp, ep)
register unsigned char *lp, *ep;
{
	register unsigned char *curlp;
	int c;
	unsigned char *bbeg; 
	register unsigned char neg;
	int ct;

	while(1) {
		neg = 0;
		switch(*ep++) {

		case CCHR:
			if(*ep++ == *lp++)
				continue;
			return(0);
	
		case CDOT:
			if(*lp++)
				continue;
			return(0);
	
		case CDOL:
			if(*lp == 0)
				continue;
			return(0);
	
		case CCEOF:
			loc2 = lp;
			return(1);
	
		case CXCL: 
			c = (unsigned char)*lp++;
			if(ISTHERE(c)) {
				ep += 32;
				continue;
			}
			return(0);
		
		case NCCL:	
			neg = 1;

		case CCL: 
			c = *lp++;
			if(((c & 0200) == 0 && ISTHERE(c)) ^ neg) {
				ep += 16;
				continue;
			}
			return(0);
		
		case CBRA:
			braslist[*ep++] = lp;
			continue;
	
		case CKET:
			braelist[*ep++] = lp;
			continue;
	
		case CCHR | RNGE:
			c = *ep++;
			getrnge(ep);
			while(low--)
				if(*lp++ != c)
					return(0);
			curlp = lp;
			while(size--) 
				if(*lp++ != c)
					break;
			if(size < 0)
				lp++;
			ep += 2;
			goto star;
	
		case CDOT | RNGE:
			getrnge(ep);
			while(low--)
				if(*lp++ == '\0')
					return(0);
			curlp = lp;
			while(size--)
				if(*lp++ == '\0')
					break;
			if(size < 0)
				lp++;
			ep += 2;
			goto star;
	
		case CXCL | RNGE:
			getrnge(ep + 32);
			while(low--) {
				c = (unsigned char)*lp++;
				if(!ISTHERE(c))
					return(0);
			}
			curlp = lp;
			while(size--) {
				c = (unsigned char)*lp++;
				if(!ISTHERE(c))
					break;
			}
			if(size < 0)
				lp++;
			ep += 34;		/* 32 + 2 */
			goto star;
		
		case NCCL | RNGE:
			neg = 1;
		
		case CCL | RNGE:
			getrnge(ep + 16);
			while(low--) {
				c = *lp++;
				if(((c & 0200) || !ISTHERE(c)) ^ neg)
					return(0);
			}
			curlp = lp;
			while(size--) {
				c = *lp++;
				if(((c & 0200) || !ISTHERE(c)) ^ neg)
					break;
			}
			if(size < 0)
				lp++;
			ep += 18; 		/* 16 + 2 */
			goto star;
	
		case CBACK:
			bbeg = braslist[*ep];
			ct = braelist[*ep++] - bbeg;
	
			if(ecmp(bbeg, lp, ct)) {
				lp += ct;
				continue;
			}
			return(0);
	
		case CBACK | STAR:
			bbeg = braslist[*ep];
			ct = braelist[*ep++] - bbeg;
			curlp = lp;
			while(ecmp(bbeg, lp, ct))
				lp += ct;
	
			while(lp >= curlp) {
				if(advance(lp, ep))	return(1);
				lp -= ct;
			}
			return(0);
	
	
		case CDOT | STAR:
			curlp = lp;
			while(*lp++);
			goto star;
	
		case CCHR | STAR:
			curlp = lp;
			while(*lp++ == *ep);
			ep++;
			goto star;
	
		case CXCL | STAR:
			curlp = lp;
			do {
				c = (unsigned char)*lp++;
			} while(ISTHERE(c));
			ep += 32;
			goto star;
		
		case NCCL | STAR:
			neg = 1;

		case CCL | STAR:
			curlp = lp;
			do {
				c = *lp++;
			} while(((c & 0200) == 0 && ISTHERE(c)) ^ neg);
			ep += 16;
			goto star;
	
		star:
			do {
				if(--lp == locs)
					break;
				if(advance(lp, ep))
					return(1);
			} while(lp > curlp);
			return(0);

		}
	}
}

static
getrnge(str)
register unsigned char *str;
{
	low = *str++ & 0377;
	size = ((*str & 0377) == 255)? 20000: (*str &0377) - low;
}

#endif