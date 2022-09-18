/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)tr:mtr.c	1.2.1.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/tr/mtr.c,v 1.1 91/02/28 20:14:58 ccs Exp $"
#include <stdio.h>
extern int dflag, sflag, cflag;
FILE *input;
#define MAXC	256
struct string {
	long int ch[MAXC], rng[MAXC];
	int rep[MAXC], nch;
	} s1, s2;
#include <sys/euc.h>
#include <pfmt.h>
extern const char badstring[];

extern eucwidth_t WW;
char width[4], cset;
#define CH(ss,i)	ss->ch[i]
#define RNG(ss,i)	ss->rng[i]
#define REP(ss,i)	ss->rep[i]
#define NCH(ss)		ss->nch
#define CODESET(c)	(ISASCII(c) ? 0 : (ISSET2(c) ? 2 : (ISSET3(c) ? 3 : 1)))

void
mtr(string1, string2)
char *string1, *string2;
{
	register long int c, j, k;
	register int i;
	long int save=0;

	width[0] = 1;
	width[1] = WW._eucw1;
	width[2] = WW._eucw2;
	width[3] = WW._eucw3;
	if(cflag) {
		sbuild(&s2, string1);
		csbuild(&s1, &s2);
	} else
		sbuild(&s1, string1);
	sbuild(&s2, string2);
	input = stdin;
	while((c=getc(input)) != EOF ) {
		if(c == 0) continue;
		switch(cset = CODESET(c)) {
		case 2:
		case 3:	c = width[cset] ? getc(input) : 0;
		case 1: c &= 0177;
			for(i=width[cset]; i-->1; )
				c = (c<<7)+(getc(input)&0177);
			c |= cset<<28;
		default: ;
		}
		for(i=s1.nch; i-->0 && (c<s1.ch[i] || s1.ch[i]+s1.rng[i]<c); );
		if(i>=0) { /* c is specified in string1 */
			if(dflag) continue;
			j = c-s1.ch[i]+s1.rep[i];
			while(i-->0) j += s1.rep[i]+s1.rng[i];
			/* j is the character position of c in string1 */
			for(i=k=0; i<s2.nch; i++) {
				if((k += s2.rep[i]+s2.rng[i]) >= j) {
					c = s2.ch[i]+s2.rng[i];
					if(s2.rng[i]) c -= k-j;
					if(!sflag || c!=save) goto put;
					else  	 goto next;
				}
			}
			goto next;
		}
		for(i=s2.nch; i-->0 && (c<s2.ch[i] || s2.ch[i]+s2.rng[i]<c); );
		if(i<0 || !sflag || c!=save) {
		put:
			save = c;
			switch(cset = c>>28) {
			case 0:	putchar(c);
				break;
			case 2:	putchar(SS2);
				goto multi_put;
			case 3: putchar(SS3);
			default:
			multi_put:
				for(i=width[cset]; i-->0; )
					putchar(c>>7*i&0177|0200);
			}
		}
	next: ;
	}
}

sbuild(s, t)
struct string *s;
char *t;
{

	char *mesg;							/*MNLS*/

	register int i;
	long int c;
	int base, n;
#define PLACE(i,c) { CH(s,i)=getlc(c,&t); REP(s,i)=1; RNG(s,i)=0; }
	for(i=0; *t; i++) {
		if(i>MAXC) goto error;
		if(*t == '[') {
			*t++;
			c = nextbyte(&t);
			cset = CODESET(c);
			PLACE(i,c)
			switch(*t++) {
			case '-':
				c = nextbyte(&t);
				if(cset == CODESET(c)) {
					if((RNG(s,i) = getlc(c,&t)-CH(s,i)) < 0)
						goto error;
				} else {
					cset = CODESET(c);
					i++;
					PLACE(i,c)
				}
				if(*t++!=']') goto error;
				break;
			case '*':
				base = (*t=='0')?8:10;
				n = 0;
				while((c = *t)>='0' && c<'0'+base) {
					n = base*n + c - '0';
					t++;
				}
				if(*t++!=']') goto error;
				if(n==0) n = 10000;
				REP(s,i) = n;
				break;
			default:
			error:
				pfmt(stderr,MM_ERROR,badstring);
				exit(1);
			}
		} else {
			c = nextbyte(&t);
			cset = CODESET(c);
			PLACE(i,c)
		}
	}
	NCH(s) = i;
}

csbuild(s, t)
struct string *s, *t;
{
	register int i, j;
	int k, nj;
	char link[MAXC], set[4];
	long int i_y, j_y, st;
#define J	link[j]
	set[0] = 0; set[1] = 2; set[2] = 3; set[3] = 1;
	width[2]++;
	width[3]++;
	if(width[set[1]] > width[set[2]])
		{ i = set[1]; set[1] = set[2]; set[2] = i; }
	if(width[set[2]] > width[set[3]])
		{ i = set[2]; set[2] = set[3]; set[3] = i; }
	if(width[set[1]] > width[set[2]])
		{ i = set[1]; set[1] = set[2]; set[2] = i; }
	width[2]--;
	width[3]--;
	NCH(s) = 0;
	for(cset=0; cset<4; cset++) {
		for(nj=0, i=NCH(t); i-->0; ) {
			if(CH(t,i)>>28 != set[cset]) continue;
			for(j=0; j<nj && (j_y = CH(t,J)+RNG(t,J)) < CH(t,i); j++);
			if(j>=nj)
				link[nj++] = i;
			else if((i_y = CH(t,i)+RNG(t,i)) < j_y) {
				if(CH(t,i) < CH(t,J)) {
					if(i_y < CH(t,J)-1) {
						for(k=nj++; k>j; k--)
							link[k] = link[k-1];
						link[j] = i;
					} else {
						RNG(t,J) = j_y-(CH(t,J) = CH(t,i));
					}
				}
			} else if(CH(t,i) <= CH(t,J))
				link[j] = i;
			else if(i_y > j_y)
				RNG(t,J) = i_y-CH(t,J);
		}
		/* "link" has the sorted order of CH for current code set. */
		for(st=(set[cset]<<28)+1, i=NCH(s), j=0; j<nj; j++) {
			if(st<CH(t,J)) {
				RNG(s,i) = CH(t,J)-1-(CH(s,i) = st);
				REP(s,i++) = 1;
			}
			st = CH(t,J)+RNG(t,J)+1;
		}
		for(k=width[set[cset]], j_y=0; k>0; k--)
			j_y = (j_y<<7)+0177;
		if((j_y |= set[cset]<<28) >st) {
			RNG(s,i) = j_y-(CH(s,i) = st);
			REP(s,i++) = 1;
		}
		NCH(s) = i;
	}
}

getlc(c,t)
register long int c;
char **t;
{
	register int i;
	switch(cset) {
	case 0:	return(c);
	case 2:
	case 3:	c = nextbyte(t);
	default:
	case 1:	c &= 0177;
	}
	for(i=width[cset]; i>1; i--) c = (c<<7)+(nextbyte(t)&0177);
	return(c|cset<<28);
}

nextbyte(s)
register char **s;
{
	register c, i, n;
	c = *(*s)++;
	if(c=='\\') {
		i = n = 0;
		while(i<3 && (c = **s)>='0' && c<='7') {
			n = n*8 + c - '0';
			i++;
			(*s)++;
		}
		if(i>0) c = n;
		else c = *(*s)++;
	}
	if(c==0) *--(*s) = 0;
	return(c&0377);
}
