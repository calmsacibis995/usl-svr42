/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/postpeep.c	1.7"
#include "sched.h"
#include "optutil.h"
#include <values.h>
static int postw2opt();
#ifdef SW_PIPE
#include "regal.h"
void sw_pipeline();
#endif
void backalign();
#ifdef P5
extern int i586_opts;
static int postw3opt();
#endif
extern int i486_opts;

extern int isbase();
extern void window();

void
postpeep()
{
#ifdef P5
 if (! i486_opts && !i586_opts)
#else
 if (! i486_opts)
#endif
	return;
#ifdef P5
 if (i586_opts) {
 	ldanal();    /*livedaed analisys */
    window(2,postw2opt);
 	window(3,postw3opt);
 } else
#endif
 {
    window(2,postw2opt);
}
}/*end postpeep*/

#ifdef P5
static char *brnames[] = { "%al", "%dl", "%cl", "%bl", 0 };
static char *srnames[] = { "%ax", "%dx", "%cx", "%bx", 0 };
static char *rnames[] =  { "%eax", "%edx", "%ecx", "%ebx", 0 };
#endif

static boolean					/* true if changes made */
postw2opt(pf,pl)
register NODE * pf;			/* first instruction node of window */
register NODE * pl;			/* second instruction node */
{
#ifdef P5
	int temp;
    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = pl->op;			/* op code number of second inst. */
#endif
    int m;				/* general integer temporary */
    char *pt;
#ifdef P5
    int src1=0, dst1=0;			/* sizes of movX source, destination */
#endif
/* Verify that all instructions in the window are doomed to run
** alone in the U pipe.
*/

#ifdef P5
if (i586_opts && (pf->usage != ALONE || pl->usage != ALONE))
	return 0;
#endif

/* Eliminate agi contentions for i486.
** If there is a contention with the base register and there is no
** scale then swap the base and index registers.
*/
	if (((m = 1, pl->ops[1] && *pl->ops[1] != '$' && *pl->ops[1] != '%')
	  || ( m = 2, pl->ops[2] && *pl->ops[2] != '%'))
	 && strchr(pl->ops[m],'(')
	 && isbase(pf->sets & ~ESP,pl->ops[m]) /* ESP can't be index */
	 && strchr(pl->ops[m],',')
	 && ! has_scale(pl->ops[m])
	 ) { char ch,*pt1;
		pt1 = getspace(strlen(pl->ops[m]) +1);
		strcpy(pt1,pl->ops[m]);
		pt = strchr(pt1,'(');
		if ( pt[3] != pt[8] || pt[4] != pt[9]) {/* (%eax,%ebx) => (%ebx,%eax) */
			ch = pt[3];
			pt[3] = pt[8];
			pt[8] = ch;
			ch = pt[4];
			pt[4] = pt[9];
			pt[9] = ch;
	 	}
	 	else { /* save one cycle: XX(%eax,%eax) =>  XX(,%eax,2) */
	 		pt[1] = ',';
	 		pt[2] = '%';
	 		pt[3] = 'e';
	 		pt[4] = pt[8];
	 		pt[5] = pt[9];
	 		pt[6] = ',';
	 		pt[7] = '2';
	 		pt[8] = ')';
	 		pt[9] = '\0';
	 	}
		pl->ops[m] = pt1;
		return(true);			/* announce success */
	 }
#ifndef P5
	return(false);			/* announce no change */
 }
#else
if (! i586_opts)
	return false;
/* Eliminate extraneous moves when the next instruction is a
** compare, and the new temporary register is dead afterwards.
**
**	Example:
**
**	movX O1,R1		->	cmpX O1,R2
**	cmpX R1,R2
**
**/

    if (
	    ismove(pf,&src1,&dst1)
	&&  (cop2 == CMPL || cop2 == CMPW || cop2 == CMPB)
	&&  src1 == dst1
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  strcmp(pl->op1,pl->op2) != 0
	&&  !isvolatile(pf, 1)
	)
    {
	wchange();
	lmrgin3(pl,pf,pl);		/* preserve line number info */ 
	pl->op1 = pf->op1;
	DELNODE(pf);			/* delete the movX */
	return(true);			/* announce success */
    }

/* Eliminate extraneous moves when the next instruction is a
** compare, and the new temporary register is dead afterwards.
**
**	Example:
**
**	movX R1,R2		->	cmpX R1,O1
**	cmpX R2,O1
**
**  or
**	movX $imm,R2		->	cmpX $imm,O1
**	cmpX R2,O1
**
**/

    if (
	    ismove(pf,&src1,&dst1)
	&&  (cop2 == CMPL || cop2 == CMPW || cop2 == CMPB)
	&&  src1 <= dst1
	&&  ( isreg(pf->op1) || isnumlit(pf->op1))
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  ! usesreg(pl->op2,pf->op2)
	)
    {
	if (pf->op1[3] == 'i' && src1 < dst1)
	  goto cmp2_out;		/* do nothing, these regs may not
					 * be promotable
					 */
	/* POSSIBLY UNNECESSARY */
	if (   isreg(pl->op2)
		&& pl->op2[3] == 'i'
	    && src1 < dst1
	   )
	  goto cmp2_out;		/* do nothing, these regs may not
					 * be promotable
					 */
	switch ( src1 ) {
	case 1:
	  if ( cop2 != CMPB )
	    goto cmp2_out;
	  wchange();			/* changing window */
	  if ( isreg(pl->op2) ) {
	    for (temp = 0; rnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op2,rnames[temp]) == 0 )
		pl->op2 = brnames[temp];
	    for (temp = 0; srnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op2,srnames[temp]) == 0 )
		pl->op2 = brnames[temp];
	  }
	  break;
	case 2:
	  if ( cop2 != CMPW )
	    goto cmp2_out;
	  wchange();			/* changing window */
	  if ( isreg(pl->op2) ) {
	    for (temp = 0; rnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op2,rnames[temp]) == 0 )
		pl->op2 = srnames[temp];
	  }
	  break;
	case 4:
	  if ( cop2 != CMPL )
	    goto cmp2_out;
	  wchange();
	  break;
	}
	lmrgin3(pl,pf,pl);		/* preserve line number info */ 
	pl->op1 = pf->op1;
	DELNODE(pf);			/* delete the movX */
	return(true);			/* announce success */
    }
cmp2_out:

/* case:  redundant movl
**
**	movl	   01,R1
**	op2	   R1,R2	->	op2   01,R2
**
**	if R1 is dead and is not R2.
*/
    if (    (cop1 == MOVL || cop1 == MOVW || cop1 == MOVB)
	&&  is2dyadic(pl)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  setreg(pf->op2) != setreg(pl->op2)
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	pl->op1 = pf->op1;
	DELNODE(pf);		/* delete first inst. */
	return(true);
    }
/* case:  clean up xorl left by one-instr peephole
**
**	xorl	R1,R1
**	op2	R1, ...	->	op2   $0, ...
**
**	if R1 is dead
*/
    if (    cop1 == XORL
	&&  (is2dyadic(pl) || cop2==PUSHW || cop2==PUSHL)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op1,pf->op2) == 0
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pl->op1,pl)
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	pl->op1 = "$0";		/* first operand is immed zero */
	DELNODE(pf);		/* delete first inst. */
	return(true);
    }

/*case:
**  mov O1,R2
**  test R2,R2  ->  cmp $0,O1
**
**  if R2 is dead after the sequence
*/
    if (cop1 == MOVL
	 && cop2 == TESTL
	 && ! isconst(pf->op1)
	 && isreg(pf->op2)
	 && isreg(pl->op1)
	 && isreg(pl->op2)
	 && isdead(pl->op2,pl)
	 && strcmp(pf->op2,pl->op2) == 0
	 && strcmp(pl->op1,pl->op2) == 0
	)  {
	 wchange();		/* change window */
	 lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	 mvlivecc(pf);		/* preserve conditions codes live info */
	 chgop(pl,CMPL,"cmpl");
	 pl->op1 = "$0";
	 pl->op2 = pf->op1;
	 DELNODE(pf);		/* delete first inst. */
	 return(true);
    }

/*case:
**  mov O1,R2
**  cmp R3,R2  ->  cmp $0,O1
**
**  if R2 is dead after the sequence
**  if it is known that R3 holds zero (from zero value trace).
*/
    if (cop1 == MOVL
	 && cop2 == CMPL
	 && isreg(pf->op2)
	 && isreg(pl->op1)
	 && isreg(pl->op2)
	 && isdead(pl->op2,pl)
	 && pl->zero_op1
	 && strcmp(pf->op2,pl->op2) == 0
	)  {
	 wchange();		/* change window */
	 lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	 mvlivecc(pf);		/* preserve conditions codes live info */
	 chgop(pl,CMPL,"cmpl");
	 pl->op1 = "$0";
	 pl->op2 = pf->op1;
	 DELNODE(pf);		/* delete first inst. */
	 return(true);
    }
	/*Un RISC the FP ops back to FI?? */
	if (pf->op == FILD || pf->op == FILDL) {
		if (!strcmp(pl->op2,"%st(1)")) {
			switch (pl->op) {
				case FADDP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FIADD,"fiadd");
							else
								chgop(pl,FIADDL,"fiaddl");
							break;
				case FSUBP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FISUBR,"fisubr");
							else
								chgop(pl,FISUBRL,"fisubrl");
							break;
				case FMULP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FIMUL,"fimul");
							else
								chgop(pl,FIMULL,"fimull");
							break;
				case FDIVP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FIDIVR,"fidivr");
							else
								chgop(pl,FIDIVRL,"fidivrl");
							break;
				case FDIVRP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FIDIV,"fidiv");
							else
								chgop(pl,FIDIVL,"fidivl");
							break;
				case FSUBRP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FISUB,"fisub");
							else
								chgop(pl,FISUBL,"fisubl");
							break;
				case FCOMP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FICOM,"ficom");
							else
								chgop(pl,FICOML,"ficoml");
							break;
				default:
					return false; /*make no change */
			}/*end switch*/
			pl->op1 = pf->op1;
			pl->op2 = NULL;
			DELNODE(pf);
		}/*second operand*/
	}/*end un risk fp*/
	return false; /* made no opts. */

}
 

static boolean					/* true if changes made */
postw3opt(pf,pl)
register NODE * pf;			/* pointer to first inst. in window */
register NODE * pl;			/* pointer to last inst. in window */
{
    register NODE * pm = pf->forw;	/* point at middle node */
    int cop1 = pf->op;			/* op code number of first */
    int cop2 = pm->op;			/* op code number of second */
    int cop3 = pl->op;			/* op code number of third */
    int src1, src2;			/* size (bytes) of source of move */
    int dst1, dst2;			/* size of destination of move */

/*Verify that all three instructions are doomed to rum alone
** in the U pipe. */
	if (! i586_opts || (pf->usage != ALONE
	 || pm->usage != ALONE || pl->usage != ALONE))
	  return (false);
/*Unrisc FP instructions - the case for FCOM is with three instructions: 
**FILD
**FXCH -> FICOM
**FCOM
*/
	if ((cop1 == FILD || cop1 == FILDL)
	 && cop2 == FXCH
	 && cop3 == FCOM
	 ) {
		if (OpLength(pf) == WoRD)
			chgop(pf,FICOM,"ficom");
		else
			chgop(pf,FICOML,"ficoml");
		DELNODE(pm);
		DELNODE(pl);
	 }
/* op= improvement
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op
** instruction sequence.  An example:
**
**	movX	R1,R2
**	addX	O3,R2		->	addX O3,R1
**	movX	R2,R1
**
** This is useful if R2 is then dead afterwards.
*/

    if (
	    ismove(pf,&src1,&dst1)
	&&  (  cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
	    || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
	    || cop2 == XORB || cop2 == XORW || cop2 == XORL
	    || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
	    || cop2 == MULB || cop2 == MULW || cop2 == MULL
	    || cop2 == IMULB || cop2 == IMULW || cop2 == IMULL
	    || cop2 == SUBB || cop2 == SUBW || cop2 == SUBL
	    )
	&&  ismove(pl,&src2,&dst2)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pm->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pm->op2) == 0
	&&  strcmp(pm->op2,pl->op1) == 0
	&&  strcmp(pf->op1,pl->op2) == 0
	&&  isdead(pm->op2,pl)
	&&  pm->op3 == NULL
	)
    {
	wchange();		/* change the window */
	lmrgin1(pf,pm,pm);	/* preserve line number info */
	pm->op2 = pf->op1;	/* Move fields around */
	makelive(pm->op2,pm);
	DELNODE(pf);
	DELNODE(pl);
	return(true);
    }
/* op= improvement
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movX	O1,R2
**	addX	R1,R2		->	addX O1,R1
**	movX	R2,O2		->	movX R1,O2
**
**	if O1, and O2 are not registers.
**
** Note that this transformation is is only correct for
** commutative operators.
*/

    if (
	    ismove(pf,&src1,&dst1)
	&&  (  cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
	    || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
	    || cop2 == XORB || cop2 == XORW || cop2 == XORL
	    || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
	    || cop2 == MULB || cop2 == MULW || cop2 == MULL
	    || cop2 == IMULB || cop2 == IMULW || cop2 == IMULL
	    )
	&&  ismove(pl,&src2,&dst2)
	&&  isreg(pf->op2)
	&&  isreg(pm->op1)
	&&  isreg(pm->op2)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pm->op2) == 0
	&&  strcmp(pm->op2,pl->op1) == 0
	&&  strcmp(pm->op1,pm->op2) != 0
	&&  isdead(pm->op2,pl)
	&&  pm->op3 == NULL
/*	&&  ( scanreg(pm->op1, false) & (EAX|EDX|ECX) ) */
	&&  src1 == src2
	&&  dst1 == dst2
	&&  src1 == dst1
	)
    if  (isdead(pm->op1,pl) || strcmp(pm->op1,pl->op2) == 0)
    {
	wchange();		/* change the window */
	lmrgin1(pf,pm,pm);	/* preserve line number info */
	pl->op1 = pm->op1;
	pm->op2 = pm->op1;	/* Move fields around */
	pm->op1 = pf->op1;
	makelive(pm->op2,pm);
	DELNODE(pf);
	return(true);
    }
/* op= improvement (For inc/dec only
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movX	O1,R1
**	incX	R1		->	incX O1
**	movX	R1,O1
**
*/

    if (
	    ismove(pf,&src1,&dst1)
	&&  (  cop2 == INCB || cop2 == INCW || cop2 == INCL
	    || cop2 == DECB || cop2 == DECW || cop2 == DECL
	    || cop2 == NEGB || cop2 == NEGW || cop2 == NEGL
	    || cop2 == NOTB || cop2 == NOTW || cop2 == NOTL
	    )
	&&  ismove(pl,&src2,&dst2)
	&&  isreg(pf->op2)
	&&  isreg(pm->op1)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pm->op1) == 0
	&&  strcmp(pm->op1,pl->op1) == 0
	&&  strcmp(pf->op1,pl->op2) == 0
	&&  isdead(pm->op1,pl)
#ifndef pre50
	&&  !isvolatile(pl,2)		/* non-volatile */
#endif
	)
    {
	wchange();		/* change the window */
	lmrgin1(pf,pm,pm);	/* preserve line number info */
	pm->op1 = pf->op1;	/* Move fields around */
	makelive(pm->op1,pm);
	DELNODE(pf);
	DELNODE(pl);
	return(true);
    }
 return (false);
}
#endif
void backalign() {
 BLOCK *b;
 NODE *p,*new;
 static int blabel = 0;
	for (b = b0.next ; b ; b = b->next) {
		if (lookup_regals(b->firstn->opcode, first_label)) {
			for (p = b->firstn->back; p != b0.next->firstn ; p = p->back) {
				if ( isbr(p) || islabel(p) || (p->uses & CONCODES))
					break;
				if (p->usage == LOST_CYCLE && ! isprefix(p)) {
					new = Saveop(0,"",0,GHOST);/* create isolated node, init. */
					APPNODE(new, p);		/* append new node after current */
					blabel++;
					new->opcode = getspace( 9 );
					b->firstn->op1 = new->opcode;
					sprintf( new->opcode, ".B%d\0", blabel );
					new->op = LABEL;
					break;
				}
			}
		}
	}
}
#ifdef SW_PIPE
/* In loops we should let loop's control (mainly the jump)
execute concurrently with floating point operations of the loop.
L:                              jmp   L:
.                           S1:
.                               fstp   O2
.                           L:
.                              ...
	fXXXl   O1                  fXXXl   O1                
	fstp    O2         =>       jne		L:	
	jne		L:	                fstpl   O2

*/
void
sw_pipeline() {
 BLOCK *b;
 NODE *p,*q,*new;
 static int slabel = 0;
 unsigned count;
#ifdef BBOPTIM
	bldgr(false,false);
#else
	bldgr(false);
#endif

	for (b = b0.next ; b ; b = b->next) {
		p = b->lastn->back;
		if (FP(p->op) && (! CONCURRENT(p->op))
		  && ( b->nextr  && b->nextr->firstn->op == LABEL  )
		  && ( strcmp(p->forw->op1,b->nextr->firstn->opcode) == 0 )
		  && lookup_regals(b->nextr->firstn->opcode, first_label)
		  ) {	
			count = 0;
			for (q = p->back ; q != b->firstn && !FP(q->op) ; q = q->back)
				count += get_exe_time(q);
#ifdef P5
			if (i586_opts && (count > 2 || q->op == FXCH))
				continue;
			else if ((! FP(q->op)) || (CONCURRENT(q->op) < (count + 3)))
#else
			if ((! FP(q->op)) || (CONCURRENT(q->op) < (count + 3)))
#endif
				continue;
			q = b->nextr->firstn;
			while (islabel(q))
				q = q->back;
			new = Saveop(0,"",0,GHOST);/* create isolated node, init. */
			APPNODE(new, q);		/* append new node after current */
			chgop(new,JMP,"jmp");
			new->op1 = p->forw->op1;
			kill_label(p->forw->op1);
			q = new;
			new = Saveop(0,"",0,GHOST);/* create isolated node, init. */
			APPNODE(new, q);		/* append new node after current */
			slabel++;
			new->opcode = getspace( 9 );
			sprintf( new->opcode, ".S%d\0", slabel );
			new->op = LABEL;
			q = new;
			new = Saveop(0,"",0,GHOST);/* create isolated node, init. */
			APPNODE(new, q);		/* append new node after current */
			new->opcode = p->opcode;
			new->op = p->op;
			new->op1 = p->op1; 
			new->op2 = p->op2; 
			p->forw->op1 = q->opcode;
			q = p->forw;
			DELNODE(p);
			APPNODE(p,q);		/* append new node after current */
		}
	}/*for loop*/
}/*end sw_pipeline*/
#endif
