/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/re.c	1.2.2.2"
#ident "@(#)re.c	1.3 'attmail mail(1) command'"
#include	<libc.h>
#include	"re.h"
#include	"lre.h"

re_re *
re_recomp(b, e, map)
	char *b;
	char *e;
	unsigned char *map;
{
	return(egprep(greparse, (unsigned char *)b, (unsigned char *)e, map, 1));
}

re_reexec(pat, b, e, match)
	re_re *pat;
	char *b;
	char *e;
	char *match[10][2];
{
	unsigned char *mb[10], *me[10], **rb, **re;
	int n, i;

	if(match)
		rb = mb, re = me;
	else
		rb = re = 0;
	n = eg_match(pat, (unsigned char *)b, (unsigned char *)e, rb, re);
	if(match)
		for(i = 0; i < 10; i++){
			match[i][0] = (char *)mb[i];
			match[i][1] = (char *)me[i];
		}
	return(n);
}

static void
freeexpr(e)
	register Expr *e;
{
	switch(e->type)
	{
	case Literal:
	case Dot:
	case Carat:
	case Dollar:
		if(e->follow)
			free((char *)e->follow);
		break;
	case Compcharclass:
	case Charclass:
		free((char *)e->r);
		break;
	case Cat:
	case Alternate:
		freeexpr(e->l);
		freeexpr(e->r);
		break;
	case Star:
	case Plus:
	case Quest:
	case Group:
	case EOP:
		freeexpr(e->l);
		break;
	case Backref:
	default:
		break;
	}
}

void
re_refree(re)
	re_re *re;
{
	if(re == 0)
		return;
	if(re->posbase)
		free((char *)re->posbase);
	if(re->root)
		freeexpr(re->root);
	if(re->ptr)
		free((char *)re->ptr);
	if(re->states)
		free((char *)re->states);
	/* leave br alone for now; it is hard to get right */
	free((char *)re);
}
