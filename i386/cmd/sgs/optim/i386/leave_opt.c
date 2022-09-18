/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/leave_opt.c	1.3"
/* leave_opt.c
**
**	Intel 386 optimizer:  for rewriting leave instruction
**
*/

/* #include "defs.h" -- optim.h takes care of this */
#include "optim.h"
#include "optutil.h"

/* leave_opt -- leave instruction optimizer
**
** This routine will attempt to rewrite leave instructions to take
** advantage of the 486.
*/

void
leave_opt()
{

/*
**	leave	->	movl	%ebp,%esp
**			popl	%ebp
**
**	Note that for this improvement, we must be optimizing for a 486,
**	as this sequence is slower on a 386.
*/
    NODE *p;

    for (ALLN(p))
    if (p->op == LEAVE)
    {
	NODE * pnew, * prepend();


	pnew = prepend(p, "%esp");
	chgop(pnew, MOVL, "movl");
	chgop(p, POPL, "popl");
	pnew->op1 = p->op1 = "%ebp";
	pnew->op2 = "%esp";
	pnew->uses = EBP;
	pnew->sets = ESP;
	p->uses = ESP;
	p->sets = EBP | ESP;
	
	lexchin(p,pnew);		/* preserve line number info */ 
    }
}
