/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/egcw.c	1.1.2.2"
#ident "@(#)egcw.c	1.2 'attmail mail(1) command'"
#include	<string.h>
#include	"re.h"
#include	"lre.h"

#if defined(__STDC__) || defined(c_plusplus) || defined(__cplusplus)
static altlist(Expr*, unsigned char *);
static word(Expr*, unsigned char*);
#else
static altlist();
static word();
#endif
static re_cw *pat;

re_cw *
re_recw(r, map)
	re_re *r;
	unsigned char *map;
{
	unsigned char buf[20000];
	register Expr *e, *root = r->root;

	if(root->type != EOP)
		return(0);
	if(root->l->type != Cat)
		return(0);
	if(root->l->l->type != Star)
		return(0);
	if(root->l->l->l->type != Dot)
		return(0);
	e = root->l->r;
	pat = re_cwinit(map);
	if(altlist(e, buf) == 0)
		return(0);
	re_cwcomp(pat);
	return(pat);
}

static
altlist(e, buf)
	Expr *e;
	unsigned char *buf;
{
	if(e->type == Alternate)
		return(altlist(e->l, buf) && altlist(e->r, buf));
	return(word(e, buf));
}

static unsigned char *p;

static
word(e, buf)
	Expr *e;
	unsigned char *buf;
{
	if(buf)
		p = buf;
	if(e->type == Cat){
		if(word(e->l, (unsigned char *)0) == 0)
			return(0);
		if(word(e->r, (unsigned char *)0) == 0)
			return(0);
	} else if(e->type == Literal)
		*p++ = e->lit;
	else
		return(0);
	if(buf)
		re_cwadd(pat, buf, p);
	return(1);
}

