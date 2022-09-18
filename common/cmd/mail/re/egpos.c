/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/egpos.c	1.2.2.2"
#ident "@(#)egpos.c	1.3 'attmail mail(1) command'"
#include	<string.h>
#include	"re.h"
#include	"lre.h"

#ifndef	POSSTEP
#define		POSSTEP		(8*1024)
#endif

void
eg_posinit(r)
	re_re *r;
{
	if(r->nposalloc <= 0)
		r->nposalloc = POSSTEP;
	r->posbase = (int *)egmalloc(r->nposalloc*sizeof(int), "posbase");
	if (!r->posbase)
		return;
	r->posnext = 0;
}

void
eg_posset(r)
	re_re *r;
{
	r->posreset = r->posnext;
}

eg_posalloc(r, n)
	re_re *r;
	int n;
{
	register j;

	if(n < 0){
		r->posnext = r->posreset;
		return(-1);
	}
	j = r->posnext;
	r->posnext += n;
	if(r->posnext >= r->nposalloc){
		while((r->nposalloc < r->posnext) && (r->nposalloc < 256*1024))
			r->nposalloc *= 2;
		if(r->nposalloc < r->posnext){
			r->nposalloc = (r->posnext+POSSTEP-1)/POSSTEP;
			r->nposalloc *= POSSTEP;
		}
		r->posbase = (int *)egrealloc((char *)r->posbase, r->nposalloc*sizeof(int), "posbase");
		if (!r->posbase)
			return(-1);
	}
	return(j);
}
