/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)expr:common/cmd/expr/expr.c	1.21.1.5"
#ident "$Header: expr.c 1.2 91/08/13 $"

#include  <stdlib.h>
#include  <regexpr.h>
#include  <locale.h>
#include  <pfmt.h>
#include  "expr.h"


long atol();
char *ltoa(), *strcpy(), *strncpy();
char	*expres();
void 	exit();
char	**Av;
static	char 	*buf;
int	Ac;
int	Argi;
int noarg;
int paren;
/*	
 *	Array used to store subexpressions in regular expressions
 *	Only one subexpression allowed per regular expression currently 
 */
static	char 	Mstring[1][MSIZE];


const char badsyn[] = ":233:Syntax error\n";
const char zerodiv[] = ":234:Division by zero\n";

char *rel(oper, r1, r2) register char *r1, *r2; 
{
	long i;

	if(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$"))
		i = atol(r1) - atol(r2);
	else
		i = strcmp(r1, r2);
	switch(oper) {
	case EQ: 
		i = i==0; 
		break;
	case GT: 
		i = i>0; 
		break;
	case GEQ: 
		i = i>=0; 
		break;
	case LT: 
		i = i<0; 
		break;
	case LEQ: 
		i = i<=0; 
		break;
	case NEQ: 
		i = i!=0; 
		break;
	}
	return i? "1": "0";
}

char *arith(oper, r1, r2) char *r1, *r2; 
{
	long i1, i2;
	register char *rv;

	if(!(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$")))
		yyerror(":235:Non-numeric argument\n");
	i1 = atol(r1);
	i2 = atol(r2);

	switch(oper) {
	case ADD: 
		i1 = i1 + i2; 
		break;
	case SUBT: 
		i1 = i1 - i2; 
		break;
	case MULT: 
		i1 = i1 * i2; 
		break;
	case DIV: 
		if (i2 == 0)
			yyerror(zerodiv);
		i1 = i1 / i2; 
		break;
	case REM: 
		if (i2 == 0)
			yyerror(zerodiv);
		i1 = i1 % i2; 
		break;
	}
	rv = malloc(16);
	(void) strcpy(rv, ltoa(i1));
	return rv;
}
char *conj(oper, r1, r2) char *r1, *r2; 
{
	register char *rv;

	switch(oper) {

	case OR:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			if(EQL(r2, "0")
			    || EQL(r2, ""))
				rv = "0";
			else
				rv = r2;
		else
			rv = r1;
		break;
	case AND:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			rv = "0";
		else if(EQL(r2, "0")
		    || EQL(r2, ""))
			rv = "0";
		else
			rv = r1;
		break;
	}
	return rv;
}

char *match(s, p)
char *s, *p;
{
	register char *rv;

	(void) strcpy(rv=malloc(8), ltoa((long)ematch(s, p)));
	if(nbra) {
		rv = malloc((unsigned) strlen(Mstring[0]) + 1);
		(void) strcpy(rv, Mstring[0]);
	}
	return rv;
}

ematch(s, p)
char *s;
register char *p;
{
	static char *expbuf;
	char *nexpbuf;
	register num;
	extern char *braslist[], *braelist[], *loc2;

	nexpbuf = compile(p, (char *)0, (char *)0);
	if(nbra > 1)
		yyerror(":236:Too many `\\(' s\n");
	if(regerrno) {
		if (regerrno != 41 || expbuf == NULL)
			errxx();
	} else {
		if (expbuf)
			free(expbuf);
		expbuf = nexpbuf;
	}
	if(advance(s, expbuf)) {
		if(nbra == 1) {
			p = braslist[0];
			num = braelist[0] - p;
			if ((num > MSIZE - 1) || (num < 0))
				yyerror(":237:string too long\n");
			(void) strncpy(Mstring[0], p, num);
			Mstring[0][num] = '\0';
		}
		return(loc2-s);
	}
	return(0);
}

errxx()
{
	yyerror(":238:RE error\n");
}

yyerror(s)
char *s;
{
	pfmt(stderr, MM_ERROR, s);
	exit(2);
}

char *ltoa(l)
long l;
{
	static str[20];
	register char *sp = (char *) &str[18];	/*u370*/
	register i;
	register neg = 0;

	if(l == 0x80000000L)
		return "-2147483648";
	if(l < 0)
		++neg, l = -l;
	str[19] = '\0';
	do {
		i = l % 10;
		*sp-- = '0' + i;
		l /= 10;
	} 
	while(l);
	if(neg)
		*sp-- = '-';
	return ++sp;
}

main(argc, argv) char **argv; 
{
	Ac = argc;
	Argi = 1;
	noarg = 0;
	paren = 0;
	Av = argv;
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:expr");

	buf = expres(0,1);
	if(Ac != Argi || paren != 0) {
		yyerror(badsyn);
	}
	(void) write(1, buf, (unsigned) strlen(buf));
	(void) write(1, "\n", 1);
	exit((!strcmp(buf, "0") || !buf[0])? 1: 0);
	/* NOTREACHED */
}
