/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/eglit.c	1.1.2.2"
#ident "@(#)eglit.c	1.2 'attmail mail(1) command'"
#include	<string.h>
#include	<libc.h>
#include	"re.h"
#include	"lre.h"

#if defined(__STDC__) || defined(c_plusplus) || defined(__cplusplus)
static void traverse(Expr *);
#else
static void traverse();
#endif

#define	MAXLIT	256	/* is plenty big enough */
static unsigned char tmp[MAXLIT], best[MAXLIT];
static unsigned char *p;
static int bestlen;
#define	START	{ p = tmp ; }
#define	ADD(c)	{ if(p >= &tmp[MAXLIT]) p--; *p++ = c; }
#define	FINISH	{ ADD(0) if((p-tmp) > bestlen) memmove((char *)best, (char *)tmp, bestlen = p-tmp); }

re_lit(r, b, e)
	re_re *r;
	unsigned char **b;
	unsigned char **e;
{
	bestlen = 0;
	START
	traverse(r->root);
	FINISH
	if(bestlen < 3)
		return(0);
	*b = best;
	*e = best+bestlen-1;
	return(1);
}

static void
traverse(e)
	register Expr *e;
{
	switch(e->type)
	{
	case Literal:
		ADD(e->lit)
		break;
	case Charclass:
		if((int)e->l == 1)
			ADD(*(char *)e->r)
		else {
			FINISH
			START
		}
		break;
	case Cat:
		traverse(e->l);
		traverse(e->r);
		break;
	case Plus:
		traverse(e->l);
		FINISH	/* can't go on past a + */
		START	/* but we can start with one! */
		traverse(e->l);
		break;
	case EOP:
		FINISH
		START
		traverse(e->l);
		break;
	default:
		FINISH
		START
		break;
	}
}
