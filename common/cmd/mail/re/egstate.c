/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/egstate.c	1.2.2.2"
#ident "@(#)egstate.c	1.3 'attmail mail(1) command'"
#include	<libc.h>
#include	<string.h>
#include	"re.h"
#include	"lre.h"

#ifndef	MINSTATE
#define	MINSTATE	32
#endif

void
eg_stateinit(r)
	re_re *r;
{
	r->statelim = MINSTATE;
	r->states = 0;
	r->threshhold = 2;
}

void
eg_clrstates(r)
	re_re *r;
{
	r->nstates = 0;
	if(r->states == 0){
		r->states = (State *)egmalloc(r->statelim*sizeof(State), "states");
		if (!r->states)
			return;
	}
}

void
eg_savestate(r, s)
	re_re *r;
	State *s;
{
	r->initialstate = s-r->states;
	r->istate = r->states[r->initialstate];	/* save for reset */
	r->istate.init = 1;
	r->flushed = 0;
}

State *
eg_startstate(r)
	re_re *r;
{
	register i;

	if(r->flushed > r->threshhold){
		int slim = r->statelim*2;
		if(slim > 512)
			slim = 512;
		if(slim > r->statelim){
			for(i = 0; i < r->statelim; i++)
				memset((char *)r->states[i].tab, 0, sizeof r->states[i].tab);
			r->states = (State *)egrealloc((char *)r->states,
				(r->statelim = slim)*sizeof(State), "states");
			if (!r->states)
				return 0;
		}
		r->flushed = 0;
		r->threshhold++;
		r->states[r->initialstate] = r->istate;
		r->nstates = r->initialstate+1;
	}
	return(r->states+r->initialstate);
}

eg_getstate(r)
	register re_re *r;
{
	if(r->nstates >= r->statelim){
		r->nstates = r->initialstate+1;
		r->states[r->initialstate] = r->istate;
		(void)eg_posalloc(r, -1);
		r->flushed++;
	}
	r->states[r->nstates].init = 0;
	return(r->nstates++);
}

State *
eg_stateof(r, ps)
	re_re *r;
	register Positionset *ps;
{
	register State *s;
	register i;
	register *p, *e;

	for(i = 0, s = r->states; i < r->nstates; i++, s++){
		if(s->npos == ps->count){
			for(p = s->pos+r->posbase, e = p+s->npos; p < e;)
				if(ps->base[*p++] == 0){
					goto next;
				}
			return(s);
		}
	next:;
	}
	return(0);
}
