/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/w2opt.c	1.1.8.1"
/* w2opt.c
**
**	Intel 386 optimizer -- two instruction peephole window
**
**
** This module contains the code that improves two-instruction
** sequences.  The general scheme is to put those improvements
** first which result in removing code, followed by the others.
**
** In some cases we play around with the live/dead information
** to convince the one-instruction window not to throw away
** code we need.  This is particularly evident when we remove
** redundant compares:  we want to make sure that the instruction
** that calculates the result indicators does not get deleted.
*/

/* Some general caveats (learned the hard way):
**
**	1.  When instructions get interchanged, we must take care that
**	    we don't alter the condition codes that would have resulted
**	    when executing the instructions in their original order.
**	2.  We can't move adds to %sp, since we may move them after the
**	    place which refers to the newly allocated space on the stack.
*/

#include "sched.h"
#include "optutil.h"
#include <values.h>

#ifdef pre50
extern boolean tflag;
#endif
extern int i486_opts;
#ifdef P5
extern int i586_opts;
extern int jflag;
#endif

/* A couple of static arrays for register name transormations */
static char *brnames[] = { "%al", "%dl", "%cl", "%bl", 0 };
static char *srnames[] = { "%ax", "%dx", "%cx", "%bx", 0 };
static char *rnames[] =  { "%eax", "%edx", "%ecx", "%ebx", 0 };

/* w2opt -- 2-instruction peephole window */

boolean					/* true if changes made */
w2opt(pf,pl)
register NODE * pf;			/* first instruction node of window */
register NODE * pl;			/* second instruction node */
{
    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = pl->op;			/* op code number of second inst. */

    int opn;				/* op code number (for istriadic) */
    char *opst, *opst2, *opst3;		/* op code string (for istriadic) */

    boolean f;				/* temporary boolean used to
					** distinguish cases below
					*/
    int temp;				/* general integer temporary */
    char *pt;
    int src1=0, dst1=0;			/* sizes of movX source, destination */
    int src2=0, dst2=0;			/* sizes of second such movX */
/* Eliminate compares against zero if preceded by instruction that
** sets result indicators.  Because some Intel 386 instructions do not
** set all result indicators, we can only discard compares after
** those instructions that set all of the relevant ones.  w3opt
** takes care of those which set a limited range of result indicators,
** but which are followed by a suitable conditional jump.
**
** None of the arithmetic operations set the "unsigned" result indicators,
** so we retain compares before any unsigned-conditional jump.
**
**	op O1,O2						->	op O1,O2
**	cmpX $0,O2 (or TESTx  R,R)
**
**/
    if (((
	      (cop2 == CMPL || cop2 == CMPW || cop2 == CMPB)
	      &&  strcmp(pl->op1,"$0") == 0)
	     ||
	      ((cop2 == TESTL || cop2 == TESTW || cop2 == TESTB)
	      &&  samereg(pl->op1,pl->op2))
	    )
	&&	is_logic(pf)
	&&  strcmp(dst(pf),pl->op2) == 0
	&&  stype(cop1) == stype(cop2)
	&&  !isvolatile(pl,2)		/* non-volatile */
	&& ( ! (/*when shift,rotate with %cl, if %cl == 0 there is */
		(cop1 >= ROLB && cop1 <= SHRL) /*no setting of CC     */
	 && (samereg(pf->op1,"%cl"))))
	)
    {
	wchange();			/* changing window */
	lmrgin3(pf,pl,pf);		/* preserve line number info */ 
	mvlivecc(pl);		/* preserve condition code info */
	DELNODE(pl);			/* delete the compare */
	return(true);			/* announce success */
    }



	if ((cop1 == CMPL || cop1 == CMPW || cop1 == CMPB)
	&&  isreg(pf->op1)		/* comparing the same register */
	&&  isreg(pf->op2)
	&&  samereg(pf->op1, pf->op2)
	&&  cop2 >= JA && cop2 <= JZ	/* followed by a conditional jump */
	&&  !(pl->nlive & CONCODES)	/* condition codes are not live */
	   ) {
		wchange();		/* changing window */
		DELNODE(pf);
		switch (cop2) {
			case JAE: case JBE: case JE: case JZ:
			case JGE: case JLE: case JNA: case JNB:
			case JNC: case JNG: case JNL:
			/* jump will occur, so change to unconditional jump */
				chgop(pl,JMP,"jmp");
				break;
			default:
				DELNODE(pl);
				break;
		}
			
		return true;
	}

	if ((cop1 == CMPL || cop1 == CMPW || cop1 == CMPB)
	&&  *pf->op2 == '$'
	&&  isnumlit(pf->op2)
	&&  cop2 >= JA && cop2 <= JZ	/* followed by a conditional jump */
	&&  !(pl->nlive & CONCODES)	/* condition codes are not live */
	   )
	   if (isnumlit(pf->op1)) {	/* pf->op2 had better be isnumlit! */
		int n1 = atoi(pf->op2+1), n2 = atoi(pf->op1+1);
		boolean willjump = true;

		wchange();		/* changing window */
		DELNODE(pf);
		switch (cop2) {
			case JA: case JNBE:
				willjump = (unsigned)n1 > (unsigned)n2;
				break;
			case JAE: case JNB:
				willjump = (unsigned)n1 >= (unsigned)n2;
				break;
			case JB: case JNAE:
				willjump = (unsigned)n1 < (unsigned)n2;
				break;
			case JBE: case JNA:
				willjump = (unsigned)n1 <= (unsigned)n2;
				break;
			case JG: case JNLE:
				willjump = n1 > n2;
				break;
			case JGE: case JNL:
				willjump = n1 >= n2;
				break;
			case JL: case JNGE:
				willjump = n1 < n2;
				break;
			case JLE: case JNG:
				willjump = n1 <= n2;
				break;
			case JE: case JZ:
				willjump = n1 == n2;
				break;
			case JNE: case JNZ:
				willjump = n1 != n2;
				break;
			default: fatal("MARC: jump is %d\n", cop2);
		}
		if (willjump)
			chgop(pl,JMP,"jmp");
		else
			DELNODE(pl);
			
		return true;
	   } else {
		/* reverse switch if not testing CF */
		switch(cop2) {
			case JAE: case JNB:
			case JB: case JNAE:
				break;
			default:
				{
				char *tmp = pf->op2;
				pf->op2 = pf->op1;
				pf->op1 = tmp;
				invbr(pl);
				}
		}
	   }

	if ((cop1 == TESTL || cop1 == TESTW || cop1 == TESTB)
	&&  *pf->op2 == '$'
	&&  isnumlit(pf->op2)
	&&  cop2 >= JA && cop2 <= JZ	/* followed by a conditional jump */
	&&  !(pl->nlive & CONCODES)	/* condition codes are not live */
	   )
	   if (isnumlit(pf->op1)) {	/* pf->op2 had better be isnumlit! */
		int n1 = atoi(pf->op2+1), n2 = atoi(pf->op1+1);
		boolean willjump = true;
		int res = n1 & n2;

		wchange();		/* changing window */
		DELNODE(pf);
		switch (cop2) {
			case JA: case JNBE:
				willjump = res != 0;
				break;
			case JAE: case JNB:
				willjump = true;
				break;
			case JB: case JNAE:
				willjump = res != 0;
				break;
			case JBE: case JNA:
				willjump = true;
				break;
			case JG: case JNLE:
				willjump = res > 0;
				break;
			case JGE: case JNL:
				willjump = res >= 0;
				break;
			case JL: case JNGE:
				willjump = res < 0;
				break;
			case JLE: case JNG:
				willjump = res <= 0;
				break;
			case JE: case JZ:
				willjump = res == 0;
				break;
			case JNE: case JNZ:
				willjump = res != 0;
				break;
			default: fatal("MARC: jump is %d\n", cop2);
		}
		if (willjump)
			chgop(pl,JMP,"jmp");
		else
			DELNODE(pl);
			
		return true;
	   } else {
		char *tmp = pf->op2;

		pf->op2 = pf->op1;
		pf->op1 = tmp;
		/* no need to invert branch, as bitwise and is associative(?) */
	   }

/* Eliminate extraneous moves when the next instruction is a
** compare, and the new temporary register is dead afterwards.
**
**	Example:
**
**	movX O1,R1		->	cmpX O1,R2
**	cmpX R1,R2
**
**/

#ifdef P5
 if (! i586_opts || jflag || cop2 == CMPW)
#endif
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
**	movX R1,R2		->	cmpX O1,R1
**	cmpX O1,R2
**
**/

    if (
	    ismove(pf,&src1,&dst1)
	&&  (cop2 == CMPL || cop2 == CMPW || cop2 == CMPB)
	&&  src1 <= dst1
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op2) == 0
	&&  isdead(pf->op2,pl)
	&&  ! usesreg(pl->op1,pf->op2)
	)
    {
	if (pf->op1[3] == 'i') goto cmp1_out;
	/* POSSIBLY UNNECESSARY */
	if (   isreg(pl->op1)
		&& pl->op1[3] == 'i'
	    && src1 < dst1
	   )
	  goto cmp1_out;		/* do nothing, these regs may not
					 * be promotable
					 */
	switch ( src1 ) {
	case 1:
	  if ( cop2 != CMPB )
	    goto cmp1_out;
	  wchange();			/* changing window */
	  if ( isreg(pl->op1) ) {
	    for (temp = 0; rnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op1,rnames[temp]) == 0 )
		pl->op1 = brnames[temp];
	    for (temp = 0; srnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op1,srnames[temp]) == 0 )
		pl->op1 = brnames[temp];
	  }
	  break;
	case 2:
	  if ( cop2 != CMPW )
	    goto cmp1_out;
	  wchange();			/* changing window */
	  if ( isreg(pl->op1) ) {
	    for (temp = 0; rnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op1,rnames[temp]) == 0 )
		pl->op1 = srnames[temp];
	  }
	  break;
	case 4:
	  if ( cop2 != CMPL )
	    goto cmp1_out;
	  wchange();
	  break;
	}
	lmrgin3(pl,pf,pl);		/* preserve line number info */ 
	pl->op2 = pf->op1;
	DELNODE(pf);			/* delete the movX */
	return(true);			/* announce success */
    }
cmp1_out:
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
	&&  ( isreg(pf->op1) || isnumlit(pf->op1) )
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  ! usesreg(pl->op2,pf->op2)
	)
    {
	if (pf->op1[3] == 'i' && src1 < dst1)
	  goto cmp2_out;		/* do nothing, these regs may not
					 * be promotable */
	/* POSSIBLY UNNECESSARY */
	if (   isreg(pl->op2) && pl->op2[3] == 'i' && src1 < dst1)
	  goto cmp2_out;		/* do nothing, these regs may not
					 * be promotable */
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
/* This next set of improvements deals with pairs of move's. */

/* case 1:  redundant movw
**
**	movw O1,O2
**	movw O2,O1		->	movw O1,O2
** or
**
**	movw O1,O2
**	movw O1,O2		->	movw O1,O2 (second one)
**
** Note that, for the second improvement, O2 cannot be used by O1.
*/
    if (   (cop1 == MOVL && cop2 == MOVL)
	|| (cop1 == MOVW && cop2 == MOVW)
	|| (cop1 == MOVB && cop2 == MOVB))
    {
	if (    strcmp(pf->op1,pl->op2) == 0	/* first case */ 
	    &&  strcmp(pf->op2,pl->op1) == 0
	    &&  !isvolatile(pl,1)
	    &&  !isvolatile(pl,2)		/* non-volatile */
	   )
	{
	    wchange();				/* change window */
	    lmrgin3(pf,pl,pf);		/* preserve line number info */ 
	    mvlivecc(pl);	/* preserve conditions codes live info */
	    if (isreg(pl->op2))
		makelive(pl->op2, pf);
	    DELNODE(pl);			/* delete second inst. */
	    return(true);
	}
	
	if (    strcmp(pf->op1,pl->op1) == 0	/* second case */
	    &&  strcmp(pf->op2,pl->op2) == 0
	    &&  ! usesvar(pl->op1,pf->op2)
	    &&  !isvolatile(pf,1)
	    &&  !isvolatile(pf,2)		/* non-volatile */
	   )
	{
	    wchange();
	    lmrgin3(pf,pl,pl);		/* preserve line number info */ 
	    mvlivecc(pf);	/* preserve conditions codes live info */
	    DELNODE(pf);			/* delete first inst. */
	    return(true);
	}
    }
/* case:  redundant movY
**
**	mov[sz]XY O1,R1		->	mov[sz]XY O1,R2
**	movY R1,R2
**
*/
    if (
	    ismove(pf,&src1,&dst1)	/* test, get source, dest. sizes */
	&&  ismove(pl,&src2,&dst2)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  !strcmp(pf->op2, pl->op1)
	&&  dst1 == src2
	&&  dst2 == src2
	&&  isdead(pf->op2,pl)
	) {
		wchange();		/* making a change */
		pf->op2 = pl->op2;
		lmrgin3(pf,pl,pf);	/* preserve line number info */ 
		mvlivecc(pl);	/* preserve conditions codes live info */
		makelive(pf->op2,pf);	/* propage liveness of R2 */
		DELNODE(pl);		/* delete second inst. */
		return true;
	}
/*  case:
    **	Both sources are the same size, and the destination
    **  of movY is equal to the source ov movY.
    **  Examples:
    **
    **		movzbw	O1,R	->	movb O1,R	s = 1; d = 4
    **		movb	R,O2	->	movb R,O2	s = 1; d = 1
    */

    if (
	    ismove(pf,&src1,&dst1)	/* test, get source, dest. sizes */
	&&  ismove(pl,&src2,&dst2)
	&&  isreg(pf->op2)
	&&  samereg(pf->op2, pl->op1)
	&&  (
		( isdead(pl->op1,pl) &&  ! usesreg(pl->op2,pl->op1) )
	     || samereg(pl->op1,pl->op2)
	     )
	)
    {
	if (
		src1 == src2		/* source sizes equal */
	    &&  src1 < dst1
	    &&  src2 == dst2
	    )
	{
	    wchange();			/* making a change */
	    switch (src2)		/* choose correct new instruction */
	    {
	    case 1:			/* byte to byte */
		chgop(pf,MOVB,"movb");
		pf->op2 = pl->op1;
		break;
	    
	    case 2:			/* halfword to halfword */
		chgop(pf,MOVW,"movw");
		pf->op2 = pl->op1;
		break;

	    case 4:			/* word to word */
		chgop(pf,MOVL,"movl");
		pf->op2 = pl->op1;
		break;
	    }
	    return(true);
	}
    } /* end move-move merging */
/* case:  redundant movl  (case 1)
**
**	movl R1,R2
**	mov[bwl] (R2),O1	->	mov[bwl] (R1),O1
**
** Note that, for the second improvement, R2 cannot be used by O1.
*/
    if (    cop1 == MOVL
	&&  ismove(pl,&src1,&dst1)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  iszoffset(pl->op1,pf->op2)
	&&  (  ( setreg(pl->op2) & setreg(pf->op2) )
	    || ( ! usesreg(pl->op2,pf->op2) && isdead(pf->op2,pl) )
	    )
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	/* POSSIBLY UNNECESSARY */
	mvlivecc(pf);		/* preserve conditions codes live info */
	pl->op1 = getspace(NEWSIZE);
	pl->op1[0] = '(';
	pl->op1[1] = '\0';
	strcat(pl->op1,pf->op1);
	strcat(pl->op1,")");
	DELNODE(pf);		/* delete second inst. */
	return(true);
    }
/* case:  redundant movl (case 2)
**
**	movl R1,R2
**	mov[bwl] O1,(R2)	->	mov[bwl] O1,(R1)
**
** Note that, for the second improvement, R2 cannot be used by O1.
*/
    if (    cop1 == MOVL
	&&  ismove(pl,&src1,&dst1)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  iszoffset(pl->op2,pf->op2)
	&&  ! usesreg(pl->op1,pf->op2)
	&&  isdead(pf->op2,pl)
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	/* POSSIBLY UNNECESSARY */
	mvlivecc(pf);		/* preserve conditions codes live info */
	pl->op2 = getspace(NEWSIZE);
	pl->op2[0] = '(';
	pl->op2[1] = '\0';
	strcat(pl->op2,pf->op1);
	strcat(pl->op2,")");
	DELNODE(pf);		/* delete second inst. */
	return(true);
    }
/* case:  redundant mov[bwl]
**
**	mov[bwl]   R1,R2
**	mov[bwl]   R2,O1	->	mov[bwl]   R1,O1
**
**	if R2 is dead and is not used by O1
*/
    if (    (cop1 == MOVB || cop1 == MOVW || cop1 == MOVL)
	&&  cop1 == cop2
	&&  is2dyadic(pl)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  ! usesreg(pl->op2,pl->op1)
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	pl->op1 = pf->op1;
	DELNODE(pf);		/* delete first inst. */
	/* POSSIBLY UNNECESSARY */
	makelive(pl->op1,pl);	/* propage liveness of R1 */
	return(true);
    }
/* case:  redundant movl
**
**	movl	   R1,R2
**	op2	   R2,O1	->	op2   R1,O1
**
**	if R2 is dead and is not used by O1
*/
    if (    (cop1 == MOVL || cop1 == MOVW || cop1 == MOVB)
	&&  is2dyadic(pl)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  ! usesreg(pl->op2,pl->op1)
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	pl->op1 = pf->op1;
	DELNODE(pf);		/* delete first inst. */
	/* POSSIBLY UNNECESSARY */
	makelive(pl->op1,pl);	/* propage liveness of R1 */
	return(true);
    }
/* case:  redundant movl
**
**	movl	   01,R1
**	op2	   R1,R2	->	op2   01,R2
**
**	if R1 is dead and is not R2.
*/
#ifdef P5
  if (! i586_opts || jflag || cop1 == MOVW)
#endif
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

/*case: redundant movzwl
**
**  mov[sz]wl	O1,R
**  cmpl	$imm,R  -> cmpw $imm, O1
*/
  if ( (cop1 == MOVZWL || cop1 == MOVSWL)
   &&  cop2 == CMPL
   &&  isreg(pf->op2)
   &&  isreg(pl->op2)
   &&  isdead(pl->op2,pl)
   &&  strcmp(pf->op2,pl->op2) == 0
   &&  *pl->op1 == '$'
   )  {
	 int imm;
	 imm = strtol(pl->op1 +1,NULL,0);
	 if (imm < 0x8000)
	 {
	   wchange();		/* change window */
	   lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	   mvlivecc(pf);		/* preserve conditions codes live info */
	   chgop(pl,CMPW,"cmpw");
	   pl->op2 = pf->op1;
	   DELNODE(pf);
	   return true;
	 }
  }

  if ( (cop1 == MOVZBL || cop1 == MOVSBL)
   &&  cop2 == CMPL
   &&  isreg(pf->op2)
   &&  isreg(pl->op2)
   &&  isdead(pl->op2,pl)
   &&  strcmp(pf->op2,pl->op2) == 0
   &&  *pl->op1 == '$'
   )  {
	 int imm;
	 imm = strtol(pl->op1 +1,NULL,0);
	 if (imm < 0x80)
	 {
	   wchange();		/* change window */
	   lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	   mvlivecc(pf);		/* preserve conditions codes live info */
	   chgop(pl,CMPB,"cmpb");
	   pl->op2 = pf->op1;
	   DELNODE(pf);
	   return true;
	 }
  }

/* case:  combine mov and and into testl
**
**	movl	   01,R1
**	andl	   02,R1	->	testl   02,01
**
**	if R1 is dead and is not R2.
*/
    if (    (cop1 == MOVL || cop1 == MOVW || cop1 == MOVB)
	&&  (cop2 == ANDL || cop2 == ANDW || cop2 == ANDB)
	&&  isreg(pf->op2)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op2) == 0
	&&  isdead(pf->op2,pl)
	&&  !usesreg(pl->op1,pf->op2)
	&&  (isreg(pf->op1) || isreg(pl->op1) ||
	     (isnumlit(pf->op1) && isnumlit(pl->op1)) ||
	     (isnumlit(pf->op1) && *pl->op1 != '$') ||		
	     (isnumlit(pl->op1) && *pf->op1 != '$'))
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	switch ( cop2 ) {	/* change the op code */
	    case ANDL:
		chgop(pl, TESTL, "testl"); break;
	    case ANDW:
		chgop(pl, TESTW, "testw"); break;
	    case ANDB:
		chgop(pl, TESTB, "testb"); break;
	}
	pl->op2 = pf->op1;
	DELNODE(pf);		/* delete first inst. */
	return(true);
    }
/* case:  combine mov and add/sub into a leal
**
**	
	movl	   R1,R2	->	leal	-n(R1),R2
	subl	   $n,R2

	movl	   R1,R2	->	leal	n(R1),R2
	addl	   $n,R2

**
*/
    if ( (cop2 == SUBL || cop2 == ADDL)
	&& cop1==MOVL
	&&  isreg(pf->op2)
	&&  isreg(pf->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op2) == 0 
	&&  isnumlit(pl->op1)
	&&  (temp=atoi(pl->op1+1) ) != -MAXLONG -1
	&&  isdeadcc(pl)	/* leal does not set cc, where sub does */
      )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pl);	/* preserve line number info */ 
	chgop(pf,LEAL,"leal");	/* first becomes a leal instruction */
	pt = pf->op1;
	pf->op1 = getspace(NEWSIZE);

	/* reformat first operand of leal */
	if (cop2 == SUBL)
		temp = -temp;
	sprintf(pf->op1,"%d(%s)",temp, pt);	

	pf->nlive = pl->nlive;

	DELNODE(pl);
	return(true);
    }
/* case:  combine mov and (dec or inc) into a leal
**
**	movl	   R1,R2
	incl	   R2		->	leal	1(R1),R2

	movl	   R1,R2	->	leal	-1(R1),R2
	decl	   R2

**
*/
    if( 
	 (cop1==MOVL && 
		    /* checks for decl or incl, sets temp depending which found */
		(  (cop2==DECL ? temp = -1:0) || (cop2==INCL ? temp = 1:0) )
	 )
	&&  isreg(pf->op2)
	&&  isreg(pf->op1)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pl->op1) == 0 
	&&  isdeadcc(pl)
     )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pl);	/* preserve line number info */ 

	chgop(pf,LEAL,"leal");	/* first becomes a leal instruction */

	pt = pf->op1;
	pf->op1 = getspace(NEWSIZE);
	/* reformat first operand of leal */

	sprintf(pf->op1,"%d(%s)",temp, pt);
	pf->nlive = pl->nlive;
	DELNODE(pl);
	return(true);
    }
/* case:  split inc MEM to inc reg and mov
**
	incl	   MEM		->	MOV MEM,R1
	movl	   MEM,R1		INC	R1
							MOV	R1,MEM

**
*/
    if((cop1== INCL || cop1 == DECL || cop1 == NEGL)
	&& (cop2 == MOVL) 
	&& (! isreg(pf->op1))
	&& ! isvolatile(pf,1)
	&&  (strcmp(pf->op1,pl->op1) == 0) 
	&&  isreg(pl->op2)
	&&  ! usesreg(pl->op1,pl->op2)
     )
    {
    NODE * pnew, *prepend();

	pnew = prepend(pf, pf->op2);
	chgop(pnew, MOVL, "movl");
	pnew->op1 =  pl->op1;
	pnew->op2 =  pl->op2;
	pnew->nlive = pf->nlive = pl->nlive;
	pf->op1 = pl->op2;
	pl->op2 = pl->op1;
	pl->op1 = pf->op1;
	wchange();		/* change window */
	lexchin(pf,pnew);		/* preserve line number info */ 
	return(true);
    }
/* case:  combine lea and mov into one instruction
**
**	lea[wl]	   O,R1
**	mov[wl]	   R1,R2	->	lea[wl]	O,R2
**
*/
    if ( ((cop1 == LEAL && cop2 == MOVL) || (cop1 == LEAW  && cop2 == MOVW) )
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  (!strcmp(pf->op2,pl->op1))
	&&  isdead(pl->op1,pl)
	)
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pl);	/* preserve line number info */ 
	pf->op2 = pl->op2;
	pf->nlive = pl->nlive;
	DELNODE(pl);		/* delete first inst. */
	return(true);
    }
/* case:  combine imull and movl into one instruction
**
**	imull	   O1,O2,R1	->	imull	O1,O2,R2
**	movl	   R1,R2
**
*/
    if (    cop1 == IMULL && cop2 == MOVL
	&&  isreg(pf->op3)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  (!strcmp(pf->op3,pl->op1))
	&&  isdead(pl->op1,pl)
	)
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pl);	/* preserve line number info */ 
	pf->op3 = pl->op2;
	pf->nlive = pl->nlive;
	DELNODE(pl);		/* delete last inst. */
	return(true);
    }
/* case:  mov and push combined into push
**
**	mov[wl]	   R1,R2
**	push[wl]   R2	->	push[wl]	R1
**
*/
    if ( ( (cop1 == MOVL && cop2 == PUSHL) || (cop1 == MOVW && cop2 == PUSHW) )
	&&  isreg(pf->op2)
	&&  isreg(pf->op1)
	&&  isreg(pl->op1)
	&&  !strcmp(pf->op2,pl->op1)
	&&  isdead(pl->op1,pl)
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	pl->op1 = pf->op1;
	DELNODE(pf);		/* delete first inst. */
	return(true);
    }
/* case:  redundant movl
**
**	movl R1,R2
**	op   addr(R2),O1	->	op	addr(R1),O1
*/
    if (    cop1 == MOVL
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  !isreg(pl->op1)
	&&  usesvar(pl->op1,pf->op2)
	&&  (  ( ((f = isdead(pf->op2,pl)) != 0) && pl->op2 == NULL )
	    || ( f && ! usesreg(pl->op2,pf->op2) )
	    || ( setreg(pl->op2) & setreg(pf->op2) )
	    )
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	opst = getspace((unsigned)strlen(pl->op1));
	opst2 = pl->op1;
	pl->op1 = opst;
	temp = strlen(pf->op2);
	while(*opst2 != '\0') {
		if (*opst2 == pf->op2[0] && strncmp(opst2,pf->op2,temp) == 0) {
			for(opst3 = pf->op1; *opst3;)
				*opst++ = *opst3++;
			opst2 += temp;
			continue;
		}
		*opst++ = *opst2++;
	}
	*opst = '\0';
	DELNODE(pf);		/* delete second inst. */
	return(true);
    }
/* case:  redundant movl
**
**	movl R1,R2
**	op   O1,addr(R2)	->	op	O1,addr(R1)
*/
    if (    cop1 == MOVL
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  !isreg(pl->op2)
	&&  usesvar(pl->op2,pf->op2)
	&&  ! usesreg(pl->op1,pf->op2)
	&&  isdead(pf->op2,pl)
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	opst = getspace((unsigned)strlen(pl->op2));
	opst2 = pl->op2;
	pl->op2 = opst;
	temp = strlen(pf->op2);
	while(*opst2 != '\0') {
		if (*opst2 == pf->op2[0] && strncmp(opst2,pf->op2,temp) == 0) {
			for(opst3 = pf->op1; *opst3;)
				*opst++ = *opst3++;
			opst2 += temp;
			continue;
		}
		*opst++ = *opst2++;
	}
	*opst = '\0';
	DELNODE(pf);		/* delete second inst. */
	return(true);
    }
/* case:  redundant leal
**
**	leal n(R1),R2
**	op   (R2...),O1	->	op	n(R1...),O1
*/
    if (    cop1 == LEAL
	&&  strchr(pf->op1,'(') != NULL	/* For 1st operand, */
	&&  strchr(pf->op1,',') == NULL	/* we want only "n(%rx)" */
	&&  isreg(pf->op2)
	&&  !isreg(pl->op1)
	&&  pl->op1 && *pl->op1 == '('
	&&  usesvar(pl->op1,pf->op2)

	    /* don't want op O1,m(x,R2,c) */
	&&  ! ((pt = strchr(pl->op1,',')) != 0
	      && strncmp(pf->op2,pt+1,strlen(pf->op2)) == 0)

	&&  ((((f = isdead(pf->op2,pl)) != 0) && pl->op2 == NULL )
	    || ( f && ! usesreg(pl->op2,pf->op2) )
	    || ( setreg(pl->op2) & setreg(pf->op2) )
	    )
       )
    {
	char *p;

	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	p = getspace((unsigned)(strlen(pf->op1) + strlen(pl->op1)));
				/* get space for new operand */
	if (pt == NULL)		/* find first comma or */
		pt = strchr(pl->op1,')');/* find right paren */
	pl->op1 = p;		/* point to new space */
	strcpy(p,pf->op1);	/* copy first operand of leal */
	p = strchr(p,')');	/* find right paren */
	strcpy(p,pt);		/* append rest */
	DELNODE(pf);		/* delete second inst. */
	return(true);
    }
/* case:  redundant leal
**
**	leal n(R1),R2
**	op   O1,(R2...)	->	op	O1,n(R1...)
*/
    if (    cop1 == LEAL
	&&  strchr(pf->op1,'(') != NULL	/* For 1st operand, */
	&&  strchr(pf->op1,',') == NULL	/* we want only "n(%rx)" */
	&&  isreg(pf->op2)
	&&  !isreg(pl->op2)
	&&  (pl->op2 && *pl->op2 == '(')
	&&  usesvar(pl->op2,pf->op2)

	    /* don't want op O1,m(x,R2,c) */
	&&  ! ((pt = strchr(pl->op2,',')) != 0
	      && strncmp(pf->op2,pt+1,strlen(pf->op2)) == 0)

	&&  isdead(pf->op2,pl)
	&& ! usesreg(pl->op1,pf->op2) 
       )
    {
	char *p;
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	p = getspace((unsigned)(strlen(pf->op1) + strlen(pl->op2)));
				/* get space for new operand */
	if (pt == NULL)		/* find first comma or */
		pt = strchr(pl->op2,')');/* find right paren */
	pl->op2 = p;		/* point to new space */
	strcpy(p,pf->op1);	/* copy first operand of leal */
	p = strchr(p,')');	/* find right paren */
	strcpy(p,pt);		/* append rest */
	DELNODE(pf);		/* delete second inst. */
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
	&&	! hasdisplacement(pl->op2)
	&&  strcmp(pf->op1,pf->op2) == 0
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pl->op1,pl)
	&&  (! (scanreg(pl->op1,false) & scanreg(pl->op2,true)))
       )
    {
	wchange();		/* change window */
	lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	mvlivecc(pf);		/* preserve conditions codes live info */
	pl->op1 = "$0";		/* first operand is immed zero */
	DELNODE(pf);		/* delete first inst. */
	return(true);
    }
/* Address arithmetic:
**
** This transformation merges instructions of the form
**
**	addw2 &m,R
**	or
**	subw2 &m,R
**
** with operands of the form
**
**	n(R)
**	  or
**	*n(R)
**
** yielding a replacement operand of the form
**
**	m+n(R)
**	  or
**	*m+n(R)
**
** We do this in a general way, transforming all of the operands in the
** instruction that follows the add/sub that can be transformed.
** The transformation cannot be performed if the second instruction uses
** R in any other way in a source.  Note that dyadic instructions like
**
**	addw2 a,R
**
** use R implicitly:  a + R -> R.
** However, for moves, triadics, and field insert/extract
** the last operand is only a destination,
** not a source operand, so having R as the last operand does not impede our
** transformation.
**
** If the second node kills R or R is dead after the second node,
** we delete the add/sub node.  Otherwise, we simply move the first node
** past the second after transforming the second nodes operands.
** Thus we can propagate the add/sub through a series of offsets:
**
**	addw2 &4,%r0
**	movw 0(%r0),0(%r1)
**	movw 4(%r0),4(%r1)
**
**		|
**		V
**	movw 4(%r0),0(%r1)
**	addw2 &4,%r0
**	movw 4(%r0),4(%r1)
**
**		|
**		V
**	movw 4(%r0),0(%r1)
**	movw 8(%r0),4(%r1)
**	addw2 &4,%r0
**
** and the addw2 can be discarded if %r0 is dead now.  Notice that no
** particular performance penalty is incurred by doing this transformation
** if we fail to discard the add/sub. But we will win if we can throw it out.  
**
** NOTE, however, that we cannot delete the add/sub if the condition
** codes following the add/sub are live and that we cannot move the add/sub if
** the condition codes live following the second instruction are live,
** since we will provide different condition codes from the original sequence.
*/
/* Begin address arithmetic transformation */

    if (    (   ( 	(cop1 == ADDL || cop1 == SUBL) 
			&& isreg(pf->op2) 
			&& strcmp(pf->op2,"%esp") != 0 /* not stack pointer */
		)
	    )
	    &&  isnumlit(pf->op1)
	    &&  ! isbr(pl)		/* don't move add/sub past branch! */
	    &&  ! ( cop2 == OTHER || cop2 == CALL )

	    /* abort if second node is also an add/sub to prevent looping */
	    &&  ! (    ( 	( 	(cop2 == ADDL || cop2 == SUBL) 
					&& isreg(pl->op2) 
				)
			)
			&&  isnumlit(pl->op1)
		  )
	)
    {
	char * srcreg = pf->op2;	/* point at source register name (R) */
	char * reg = dst(pf);		/* point at destination reg name */
	int destreg = 0;		/* number of destination operand,
					** zero if none
					*/
	int idxmask = 0;		/* bit mask of operands to change */
	long idxval[MAXOPS+1];		/* current indices for each (indexed
					** 1 to MAXOPS, not 0 to MAXOPS-1)
					*/
	int i;				/* loop index */
	long m =  atol(pf->op1+1);	/* literal in add/sub instruction */
	NODE * nextinst = pl->forw;	/* pointer to inst. after window */
	char *dl = dst(pl);
#ifdef IMPADDR
	char *df = reg;
#endif /* IMPADDR */


/* Check for live registers and condition codes.
*/

	f = ( isdead(reg,pl) || samereg(reg, dl)  )
#ifdef IMPADDR
		&& isdeadcc(pf)
#endif /* IMPADDR */
		;
					/* Conditions underwhich it is okay
					 * to delete, i.e., (a) reg is dead 
					 * after or is killed by the second 
					 * instruction and (b) condition codes
					 * of add/sub aren't needed. */
	if ( (! f) 
		&& (
#ifdef IMPADDR
			strcmp(pf->op2,dl) == 0
			|| ( strcmp(df,dl) == 0
				&& ! isdead(reg,dl)
			     )
			||
#endif /* IMPADDR */
			   ( ! isdeadcc(pl) 
#ifndef IMPADDR
				|| iscompare(nextinst)
#endif /* IMPADDR */
			   ) 
		   )
	   )
	    goto noindex;		/* It is not okay to move, i.e., 
					 * (a) second instruction changes 
					 * a source of the add/sub or
					 * (b) the add/sub would overwrite the
					 * needed destination of the second
					 * instruction or (c) condition 
					 * codes of second instruction
					 * are needed. */


	
/* Compute offset */
	if ( cop1 == SUBL )
	    m = -m;			/* negate literal if subtract */
	
/* Determine in which instructions we allow ourselves to see R as an
** operand (destination) and remember which operand number it is.
*/

	if (istriadic(pl,&opn,&opst))
	    destreg = 3;		/* in triadics, is third operand */
	else if (ismove(pl,&src1,&dst1))
	    destreg = 2;		/* in moves, is second */
	/* in all others, not allowed */

/* Now we loop through all possible operands in an instruction, looking
** for indexed uses of R (i.e., n(R) or *n(R) ).  If we find, instead, a use of
** R which is not in the "destreg" position, we cannot do the transform.
*/

	for (i = MAXOPS; i > 0; i--)
	{
	    char * t = pl->ops[i];	/* point at new operand */

	    if (t == NULL)
		continue;		/* disregard null operands */

	    if (isindex(t,reg))		/* this is what we seek */
	    {
		idxmask |= (1<<i);	/* remember where we saw it */
		if(*t == '*')
			t++;
		idxval[i] = atol(t);	/* remember current index value
					** (i.e., n from n(R)
					*/
	    }
	    else if (samereg(t,reg) && i == destreg)
		continue;		/* no transformation needed */
	    else if (usesreg(t,reg))
		goto noindex;		/* instruction uses register in
					** non-transformable way
					*/
	}
/* We now know there have been only valid uses of R in the second
** instruction.  If there are any uses of R at all, transform the
** second instruction.
*/

#ifndef IMPADDR
	if (idxmask == 0)
	    goto noindex;		/* nothing to do if no uses of R */
#endif /* IMPADDR */

	wchange();			/* we may just move/delete the add */
	for (i = MAXOPS; i>0; i--)	/* find operands to alter */
	{
	    char * t;
	    if ((idxmask & (1<<i)) == 0)
		continue;		/* ignore this operand:  not n(R) */

	    t = pl->ops[i];
	    pl->ops[i] = getspace(NEWSIZE);
					/* get space for new operand */
	    if(*t == '*')
		(void) sprintf(pl->ops[i],"*%d(%s)",(int)(m+idxval[i]),srcreg);
	    else
		(void) sprintf(pl->ops[i],"%d(%s)",(int)(m+idxval[i]),srcreg);
					/* build new operand string */
	}

/* Discard add/sub if (a) condition codes permit and (b) R is dead after 
** second instruction or that instruction sets a new value.
*/

	if (f)				/* conditions permit deleting */
	    { 
	     ldelin(pf);		/* preserve line number info */ 
	     mvlivecc(pf);	/* preserve conditions codes live info */
	     DELNODE(pf); }		/* delete add/sub */
	else {	
	    lexchin(pf,pl);		/* preserve line number info */ 
	    exchange(pf);		/* otherwise exchange add/sub and
					** second inst.
					*/
	    swplivecc(pf,pl);	/* swaping the condition codes live/dead info */
	}
	return(true);
    }
noindex:


/********************************************************************
**
**	Begin improvements that alter code, rather than deleting it.
**
***********************************************************************
*/

/* Use register in pushl if possible
**
**	movl R,O		->	movl R,O
**	pushl O			->	pushl R
**
** if O is not a register.
*/

    if (
	     cop1 == MOVL
	&&   cop2 == PUSHL
	&&   strcmp(pf->op2,pl->op1) == 0
	&&   isreg(pf->op1)
	&&   !isreg(pl->op1)
	&&   !isvolatile(pl,1)		/* non-volatile */
	)
    {
	wchange();
	makelive((pl->op1 = pf->op1),pl); /* propagate liveness of R */
	return(true);
    }
/* Use register in pushw if possible
**
**	movw R,O		->	movw R,O
**	pushw O			->	pushw R
**
** if O is not a register.
*/

    if (
	     cop1 == MOVW
	&&   cop2 == PUSHW
	&&   strcmp(pf->op2,pl->op1) == 0
	&&   isreg(pf->op1)
	&&   !isreg(pl->op1)
	&&   !isvolatile(pl,1)		/* non-volatile */
	)
    {
	wchange();
	makelive((pl->op1 = pf->op1),pl); /* propagate liveness of R */
	return(true);
    }
/* Use register if possible in op2 or op3
**
**	mov[bwl] R,O1		->	mov[bwl] R,O1
**	op2 O1,O2		->	op2 R,O2
**
**	if O1 not a register
*/

    if (
	    ( cop1 == MOVL || cop1 == MOVW || cop1 == MOVB )
	&&  isreg(pf->op1)
	&&  ! isreg(pf->op2)
	)
    {
	if (
	       (
		    isdyadic(pl)
		&&  (f = (strcmp(pf->op2,pl->op1) == 0))
		&&  ( ismove( pl, &src2, &dst2 ) && src2 == stype(cop1)
		||   !ismove( pl, &src2, &dst2 ) && stype(cop2) == stype(cop1)
		    )
		&&   !isvolatile(pl,1)		/* non-volatile */
		)
	    )
	{
	    wchange();
	    if (f)			/* f true if we modify first operand */
		pl->op1 = pf->op1;
	    else
		pl->op2 = pf->op1;
	    makelive(pf->op2,pl);	/* propagate liveness into op2/3 */
	    makelive(pf->op1,pf);	/* propagate liveness into op2/3 */
	    return(true);
	}
    }
/* Use register in second of two moves, if possible.
**
** This improvement attempts to propagate a register use, if possible.
** We do this in a general way for pairs of moves.  Here's the case
** we're considering:
**
**		movX R,O1	->	movX R,O1
**		movY O1,O2	->	movY R,O2
**
** We don't bother with this improvement if O1 is already a register.
**
** We can make the identical improvement if "R" is really a nibble.
**
** We can do this when (using earlier terminology) the destination
** of movX and the source of movY are the same size.  There are three
** sub-cases:
**
**	1.  destination1 == source2 == destination2.
**	2.  source1 >= destination1 == source2 .
**		(remember, source1 is register or nibble)
**	3.  source1 >= destination2.
**
** Examples:
**
**		movbbh R,O1				s = 1; d = 2
**		movthb O1,O2	->	movb R,O2	s = 2; d = 1
**
**		movbbh R,O1				s = 1; d = 2
**		movh O1,O2	->	movbbh R,O2	s = 2; d = 2
**
*/

    if (
	    ismove(pf, &src1, &dst1)
	&&  ismove(pl, &src2, &dst2)
	&&  src2 == dst1
	&&  ( isreg(pf->op1) )		/* R can be reg. */
	&&  ! isreg(pf->op2)		/* this is "don't bother" test */
	&&  strcmp(pf->op2, pl->op1) == 0
	&&  !isvolatile(pl,1)		/* non-volatile */
	)
    /* fall through to sub-cases */
    {

    /* sub-case 1:  source2 and destination2 same */

	if (src2 == dst2)
	{
	    wchange();
	    chgop(pl,(int)pf->op,pf->opcode); /* copy instruction of first node */
	    pl->op1 = pf->op1;		/* propagate register/nibble use */
	    makelive(pl->op1,pl);	/* mark it as live */
	    return(true);
	}

    /* sub-case 2:  source1 >= destination1 */

	if (src1 >= dst1)
	{
	    wchange();
	    pl->op1 = pf->op1;		/* propagate register/nibble */
	    makelive(pl->op1,pl);	/* make register live after pl */
	    return(true);
	}

    /* sub-case 3:  source1 >= destination2 */

	if (src1 >= dst2)
	{
	    wchange();
	    switch (dst2)		/* choose correct new instruction */
	    {
	    case 1:
		chgop(pl,MOVB,"movb"); break;
	    case 2:
		chgop(pl,MOVW,"movw"); break;
	    case 4:
		chgop(pl,MOVL,"movl"); break;
	    }
	    pl->op1 = pf->op1;		/* propagate register/nibble use */
	    makelive(pl->op1,pl);	/* make this register live here */
	    return(true);
	}
    }
/* More paired move improvements
**
** This improvement propagates the use of a register, if possible from
** one move to a following one.  It also has the potential of killing
** off the liveness of a register.  Example:
**
**	movX O,R1		->	movX O,R1
**	movY O,R2		->	movY R1,R2
**
**	Since we are trying to kill registers off early, these special
**	conditions apply if O is a register:
**		1)  O must be dead after movY or be the same as R2.
**		2)  R1 must be live after movY.
**	If O is a register, it must be live after movY or be the same
**	register as R2.
**	O may not use R1.
**
**	The size of source1 must be the same as source2,
**	and destination1 must be at least as large as source2.
*/

    if (
	    ismove(pf,&src1,&dst1)
	&&  ismove(pl,&src2,&dst2)
	&&  strcmp(pf->op1,pl->op1) == 0
	&&  !isnumlit(pf->op1)	/* these are done by replace_consts() */
	&&  isreg(pf->op2)
	&&  isreg(pl->op2)
	&&  ! usesreg(pl->op1,pf->op2)
	&&  src1 >= src2
	&&  src2 == dst1
	&&  (
		! isreg(pl->op1)		/* O not a register */
	    ||  (
		    (
			samereg(pl->op1,pl->op2)/* O same as R2 */
		    ||  isdead(pl->op1,pl)	/* O dead after movY */
		    )
		
		&&  ! isdead(pf->op2,pl)	/* R1 live after movY */
		)
	    )
	&&  !isvolatile(pl,1)		/* non-volatile */
	)
    {
	wchange();
	makelive(pl->op1 = pf->op2,pf); /* copy operand, make register live
					** after movX
					*/
	return(true);
    }

/* Re-order and change pairs of movs to make better use of
**  registers.
**
**  mov $x,O1  ->  mov $x,R
**  mov O1,R   ->  mov R,O1
**
** do it also if the sourse of the first move is a register.
*/
   if (cop1 == MOVL
    && cop2 == MOVL
	&& ! isreg(pf->op2)
	&& ! isvolatile(pf,2)
	&& (isreg(pf->op1) || isconst(pf->op1))
	&& isreg(pl->op2)
	&& ! isreg(pl->op1)  /*avoid unnecessary strcmp */
	&& strcmp(pl->op1,pf->op2) == 0
	&& ! (scanreg(pl->op1,false) & scanreg(pl->op2,false))
   )
   {
	 wchange();
	 lexchin(pf,pl);
	 pf->op2 = pl->op2;
	 pl->op2 = pl->op1;
	 pl->op1 = pf->op2;
	 makelive(pf->op2,pf);
	 return true;
   }

/* Re-order pairs of instructions to make better use of registers.
**
** This improvement reverses a dyadic and a movw to make better use of
** registers.  The canonical sequence is:
**
**	op2 O1,O2		->	movw O1,R
**	movw O1,R		->	op2 R,O2
**
**	if O1 and O2 don't use R
**	if R live after movw
**	if instruction following movw is not a conditional branch
**	(since we're changing the instruction that sets codes)
**
**	op2 can also be CMPL, but cannot be a shift, consider the case:
**
**		shll	$12,%eax	->	movl	$12,%edx
**		movl	$12,%edx	->	shll	%edx,%eax
*/

    if (
	    (     cop1 == CMPL
	     ||  (   isdyadic(pf) 
		  && ! ismove(pf,&src1,&dst1)
		  &&   cop1 != SHLL  &&  cop1 != SHLW  &&  cop1 != SHLB
		  &&   cop1 != SALL  &&  cop1 != SALW  &&  cop1 != SALB
		  &&   cop1 != SHRL  &&  cop1 != SHRW  &&  cop1 != SHRB
		  &&   cop1 != SARL  &&  cop1 != SARW  &&  cop1 != SARB
		 )
	    )
				/* since moves are handled elsewhere
				** and this would tend to undo them
				*/
	&&  (cop2 == MOVL || cop2 == MOVW || cop2 == MOVB)
	&&  ismove(pl,&src2,&dst2)
	&&  strcmp(pf->op1,pl->op1) == 0
	&&  isreg(pl->op2)
	&&  ! isdead(pl->op2,pl)
	&&  ! usesreg(pf->op1,pl->op2)
	&&  ! usesreg(pf->op2,pl->op2)
	&&  ! usesvar(pf->op1,pf->op2)	/* O1 cannot use O2 in any way */
	&&  isdeadcc( pl )
	&&  !isvolatile(pf,1)		/* non-volatile */
	&&  is_legal_op1(dst2, pf)
	)
    {
	wchange();
	pf->op1 = pl->op2;		/* copy operand first */
	lexchin(pf,pl);		/* preserve line number info */ 
	exchange(pf);			/* switch the two instructions */
	swplivecc(pf,pl);    /* switch the live/dead info on condition codes */
	makelive(pl->op2,pf);		/* show register as live after op2 */
	return(true);
    }
/* Reverse adds/subtracts of literals and other adds/subtracts.
**
** This transformation facilitates other improvements by bubbling
** adds/subtracts of literals downward and other adds/subtracts upwards.
**
**	addw2	&n,R		->	addw2	O,R
**	addw2	O,R		->	addw2	&n,R
**
**	if R not used in O, O not a literal.
*/

    if (
	    (cop1 == ADDL || cop1 == SUBL)
	&&  (cop2 == ADDL || cop2 == SUBL)
	&&  isreg(pf->op2)
	&&  samereg(pf->op2,pl->op2)
	&&  *pf->op1 == '$'
	&&  ! usesreg(pl->op1,pl->op2)
	&&  *pl->op1 != '$'
	)
    {
	wchange();
	lexchin(pf,pl);		/* preserve line number info */ 
	exchange(pf);			/* just exchange instructions */
	swplivecc(pf,pl);    /* switch the live/dead info on condition codes */
	return(true);
    }
/* Reverse adds/subtracts of literals and moves.
**
** This transformation facilitates address arithmetic transformations.
**
**	addw2 &n,R		->	movX O1,O2
**	movX O1,O2		->	addw2 &n,R
**
**	if neither O1 nor O2 uses R
**	if instruction following movX is not conditional branch, since we
**	  will set different condition codes
*/

    if (
	    (cop1 == ADDL || cop1 == SUBL)
	&&  isreg(pf->op2)
	&&  strcmp(pf->op2,"%esp") != 0	/* not stack pointer */
	&&  isnumlit(pf->op1)		/* only useful with numeric lits. */
	&&  ismove(pl,&src1,&dst1)
	&&  ! (usesreg(pl->op1,pf->op2) || usesreg(pl->op2,pf->op2))
	&&  isdeadcc( pl )
	)
    {
	wchange();
	lexchin(pf,pl);			/* preserve line number info */ 
	exchange(pf);			/* interchange instructions */
	swplivecc(pf,pl);    /* switch the live/dead info on condition codes */
	if (isreg(pf->op2))
		makelive(pf->op2,pl);	/* show register as live after op2 */
	if (isreg(pl->op2))
		makelive(pl->op2,pf);
	return(true);
    }
/*
**	This code improvement looks for side by side push/pop
**	instructions to the same location, and removes
**	them.
**		pushl	r1
**		popl	r1
*/
    if (    cop1 == PUSHL
	&&  cop2 == POPL
	&&  strcmp(pf->op1,pl->op1) == 0
       )
    {
	wchange();
	lmrgin3(pf,pl,pl);	/* preserve line number info */ 
	DELNODE(pf);		/* delete first inst. */
	DELNODE(pl);		/* delete second inst. */
	return(true);
    }
/*
**	This code improvement looks for two side by side floating
**	point exchange instructions to the same location, and removes
**	them.  This is the same a redundant move:
**		fmove	r1,r2
**		fmove	r2,r1...
*/
    if (    cop1 == FXCH
	&&  cop2 == FXCH
	&&  strcmp(pf->op1,pl->op1) == 0
       )
    {
	wchange();
	lmrgin3(pf,pl,pl);	/* preserve line number info */ 
	DELNODE(pf);		/* delete first inst. */
	DELNODE(pl);		/* delete second inst. */
	return(true);
    }
/*
**	This code improvement looks for a fstl, followed by a fstp
**	and changes this into one fstpl instruction.
**		fstl	xxx
**		fstp	%st(0)
*/
    if (    cop1 == FSTL
	&&  cop2 == FSTP
	&&  strcmp(pl->op1,"%st(0)") == 0
       )
    {
	wchange();
	lmrgin3(pf,pl,pl);	/* preserve line number info */ 
	chgop(pf,FSTPL,"fstpl");
	DELNODE(pl);		/* delete second inst. */
	return(true);
    }
/*
** This improvement looks for a specific class of mov, op
** instructions that may be converted to the shorter op
** instruction sequence.  An example:
**
**	movX	R1,R2
**	addX	R2,R3		->	addX R1,R3
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
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  ( scanreg(pl->op1, false) & (EAX|EDX|ECX) )
	&&  src1 == dst1
	&&  setreg(pf->op2) != setreg(pl->op2)
	)
    {
	wchange();		/* change the window */
	lmrgin1(pf,pl,pl);	/* preserve line number info */
	pl->op1 = pf->op1;
	DELNODE(pf);
	return(true);
    }
/*
**
**	movl	R1,R2
**	addX	R3,R2		->	leal (R1,R3),R2
**
*/

    if (
	    !i486_opts
	&&  cop1 == MOVL
	&&  cop2 == ADDL
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op2) == 0
	&&  strcmp(pf->op2,pl->op1) != 0 /* MOVL must not kill R3 */
	&&  isdeadcc(pl)
	)
    {
	wchange();		/* change the window */
	lmrgin3(pf,pl,pl);	/* preserve line number info */
	chgop(pf, LEAL,"leal");	/* first becomes a leal instruction */

	pt = getspace(NEWSIZE);
	/* reformat first operand of leal */

	sprintf(pt, "(%s,%s)", pf->op1, pl->op1);
	pf->op1 = pt;
	pf->nlive = pl->nlive;
	DELNODE(pl);
	return(true);
    }
    return(false);
}
