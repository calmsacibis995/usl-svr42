/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/egparen.c	1.1.2.2"
#ident "@(#)egparen.c	1.1 'attmail mail(1) command'"
#include	"re.h"
#include	"lre.h"

#if defined(__STDC__) || defined(c_plusplus) || defined(__cplusplus)
static int egparen(Expr *e);
#else
static int egparen();
#endif

int
re_paren(re)
	re_re *re;
{
	return egparen(re->root);
}

static int
egparen(e)
	Expr *e;
{
	if(e == 0)
		return(0);
	switch(e->type)
	{
	case Null:
	case Literal:
	case Dot:
	case Carat:
	case Dollar:
	case Backref:
	case Compcharclass:
	case Charclass:
		break;
	case Cat:
	case Alternate:
		return(egparen(e->l)+egparen(e->r));
	case Star:
	case Plus:
	case Quest:
	case EOP:
		return(egparen(e->l));
	case Group:
		return(1+egparen(e->l));
	}
	return(0);
}
