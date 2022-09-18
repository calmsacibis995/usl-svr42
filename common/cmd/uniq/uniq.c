/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)uniq:uniq.c	1.4.1.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/uniq/uniq.c,v 1.1 91/02/28 20:17:02 ccs Exp $"
/*
 * Deal with duplicated lines in a file
 */

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <sys/euc.h>
#include <getwidth.h>

int	fields = 0;
int	letters = 0;
int	linec;
char	mode;
int	uniq;
char	*skip();

eucwidth_t	wp;

main(argc, argv)
int argc;
char *argv[];
{
	static char b1[1000], b2[1000];
	FILE *temp;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:uniq");

	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;

	while(argc > 1) {
		if(*argv[1] == '-') {
			if (isdigit(argv[1][1]))
				fields = atoi(&argv[1][1]);
			else mode = argv[1][1];
			argc--;
			argv++;
			continue;
		}
		if(*argv[1] == '+') {
			letters = atoi(&argv[1][1]);
			argc--;
			argv++;
			continue;
		}
		if ( (temp = fopen(argv[1], "r")) == NULL)
			printe(":3:Cannot open %s: %s\n", argv[1],
				strerror(errno));
		else {  fclose(temp);
			freopen(argv[1], "r", stdin);
		     }
		break;
	}
	if(argc > 2 && freopen(argv[2], "w", stdout) == NULL)
		printe(":12:Cannot create %s: %s\n", argv[2],
			strerror(errno));

	if(gline(b1))
		exit(0);
	for(;;) {
		linec++;
		if(gline(b2)) {
			pline(b1);
			exit(0);
		}
		if(!equal(b1, b2)) {
			pline(b1);
			linec = 0;
			do {
				linec++;
				if(gline(b1)) {
					pline(b2);
					exit(0);
				}
			} while(equal(b1, b2));
			pline(b2);
			linec = 0;
		}
	}
}

gline(buf)
register char buf[];
{
	register c;

	while((c = getchar()) != '\n') {
		if(c == EOF)
			return(1);
		*buf++ = c;
	}
	*buf = 0;
	return(0);
}

pline(buf)
register char buf[];
{

	switch(mode) {

	case 'u':
		if(uniq) {
			uniq = 0;
			return;
		}
		break;

	case 'd':
		if(uniq) break;
		return;

	case 'c':
		printf("%4d ", linec);
	}
	uniq = 0;
	fputs(buf, stdout);
	putchar('\n');
}

equal(b1, b2)
register char b1[], b2[];
{
	register char c;

	b1 = skip(b1);
	b2 = skip(b2);
	while((c = *b1++) != 0)
		if(c != *b2++) return(0);
	if(*b2 != 0)
		return(0);
	uniq++;
	return(1);
}

char *
skip(s)
register char *s;
{
	register nf, nl;
	register int i, j;

	nf = nl = 0;
	while(nf++ < fields) {
		while(*s == ' ' || *s == '\t')
			s++;
		while( !(*s == ' ' || *s == '\t' || *s == 0) ) 
			s++;
	}
	while(nl++ < letters && *s != 0) 
	{
		if (ISASCII(*s))
			s++;
		else {
			if (ISSET2(*s)) {
				i = wp._eucw2;
				j = wp._scrw2;
			} else if (ISSET3(*s)) {
				i = wp._eucw3;
				j = wp._scrw3;
			} else {
				i = wp._eucw1;
				j = wp._scrw1;
			}

			nl += (--j);

			if (nl <= letters)
				s += i;
		}
	}
	return(s);
}

printe(p,s, s2)
char *p,*s, *s2;
{
	pfmt(stderr, MM_ERROR, p, s, s2);
	exit(1);
}
