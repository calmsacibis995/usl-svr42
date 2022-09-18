/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/egcanon.c	1.1"
#ident "@(#)egcanon.c	1.1 'attmail mail(1) command'"
#include	<string.h>
#include	<libc.h>
#include	<setjmp.h>
#include	"re.h"
#include	"lre.h"
#include	"re.h"
#include	"lre.h"

/* #define 	DEBUG */

static Expr **proot;

#define	PURE(e)		(!(e)->backref && !(e)->parens)
#define	PROC(kid)	if(ee = proc(kid)){ ee->parent = (kid)->parent; free((char *)kid); kid = ee; }

static Expr *
proc(e)
	Expr *e;
{
	Expr *ee;

	if(e->type == Cat){
		if(PURE(e->l)){
			if(proot){
				*proot = eg_newexpr(Cat, 0, *proot, e->l);
				return((ee = proc(e->r))? ee:e->r);
			} else
				proot = &e->l;
		} else {
			PROC(e->l)
		}
		if(PURE(e->r)){
			if(proot){
				*proot = eg_newexpr(Cat, 0, *proot, e->r);
				return(e->l);
			} else
				proot = &e->r;
		} else {
			PROC(e->r)
		}
		return(0);
	}
	proot = 0;
	switch(e->type)
	{
	case Alternate:
		PROC(e->l)
		proot = 0;
		PROC(e->r)
		break;
	case Star:
	case Plus:
	case Quest:
	case EOP:
	case Group:
		PROC(e->l)
		break;
	}
	proot = 0;
	return(0);
}

void
egcanon(e)
	Expr *e;
{
#ifdef	DEBUG
	char before[EPRINTSIZE], after[EPRINTSIZE];
#endif

#ifdef	DEBUG
	eg_epr(e, before, 0);
	if(TRACE(3)){
		PR "egcanon(%s):\n", before);
	}
#endif
	proot = 0;
	if(!PURE(e))
		proc(e);
#ifdef	DEBUG
	eg_epr(e, after, 0);
	if(TRACE(3)){
		PR "egcanon returns %s\n", after);
	}
	if(strcmp(before, after)){
		EPR "URK! egcanon did not preserve!\nbefore=%s\n after=%s\n", before, after);
		exit(1);
	}
#endif
}
