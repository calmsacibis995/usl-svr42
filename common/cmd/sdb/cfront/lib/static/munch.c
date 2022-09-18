/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/lib/static/munch.c	1.3"
/*
	scan nm output and detect constructors and destructors for static objects.
	the name on an nm output line is expected to be in the right hand margin.
	the name is expected to be of the form _STD*_ or _STI*_.
	nm output lines are assumed to be less than BUFSIZ characters long.
	constructors found are called by _main() called from main().
	destructors found are called by exit().
	return 0 if no constructor or destructor is found otherwise.
	The output is redirected by CC into _ctdt.c
	
	
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>

struct sbuf {
	struct sbuf *next;
	char *str;
};
typedef struct sbuf	Ssbuf;

static Ssbuf *new_sbuf();

static Ssbuf *new_sbuf(l, p)
Ssbuf *l;
char *p;
{
	Ssbuf *this;

	this = (Ssbuf *)_new((long)sizeof(Ssbuf));
	this->next= l;
	this->str = (char *)_new((long)(strlen(p)+1));
	(void) strcpy( this->str, p );
	return this;
}

Ssbuf *dtor = 0;	/* list of constructors */
Ssbuf *ctor = 0;	/* list of destructors */

main ()
{
	char buf[BUFSIZ];
	register char *p;
	register char *p2;

	_main();
	while (gets(buf) != NULL)
	{
		p = buf + strlen(buf) - 1;
		while (*p != '|' && !isspace(*p) && p != buf)
			--p;
		if (*++p != '_')
			continue;
		if ( *(p2 = p + 1) == '_')   /* accept _ST and __ST */
			p2++;

		if (!strncmp(p2, "STI", 3))
			ctor = new_sbuf(ctor,p);
		else if (!strncmp(p2, "STD", 3))
			dtor = new_sbuf(dtor,p);
	}

	if (!dtor && !ctor)
		exit(0);

	printf("typedef int (*PFV)();\n");	/* "int" to dodge bsd4.2 bug */
	if (ctor) {
		Ssbuf *p, *q;
		for (p = ctor; p; p=p->next) printf("int %s();\n",p->str);
		printf("extern PFV _ctors[];\nPFV _ctors[] = {\n");
		for (q = ctor; q; q=q->next) printf("\t%s,\n",q->str);
		printf("\t0\n};\n");
	}

	if (dtor) {
		Ssbuf *p, *q;
		for (p = dtor; p; p=p->next) printf("int %s();\n",p->str);
		printf("extern PFV _dtors[];\nPFV _dtors[] = {\n");
		for (q = dtor; q; q=q->next) printf("\t%s,\n",q->str);
		printf("\t0\n};\n");
	}

	exit(1);
}

