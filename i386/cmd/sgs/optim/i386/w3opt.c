/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/w3opt.c	1.1.3.11"
/* w3opt.c
**
**	Intel 386 three-instruction window improver
**
**
**
** This module contains improvements for three instruction windows,
** of which there aren't many.
*/

#include "sched.h" /*includes optim.h includes defs.h */
#include "optutil.h"
#include <stdio.h>
#ifdef P5
extern int i586_opts;
extern int jflag;
#endif

static boolean ispwrof2();

/* w3opt -- 3-instruction peephole window */

boolean					/* true if changes made */
w3opt(pf,pl)
register NODE * pf;			/* pointer to first inst. in window */
register NODE * pl;			/* pointer to last inst. in window */
{
    register NODE * pm = pf->forw;	/* point at middle node */
    char *ptr;
    int numbytes, numwords;
    int constant, ilog;
    int srcsize;
    int targsize;
    char *temp;

    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = pm->op;			/* op code number of second */
    int cop3 = pl->op;			/* ... of third */

    int src1, src2;			/* size (bytes) of source of move */
    int dst1, dst2;			/* size of destination of move */

/*Taken from w2opt, to limit it to jcc's which dont use CF and VF.
** Eliminate compares against zero if preceded by instruction that
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
**  jcc									jcc
**
**/
    if (((
	      (cop2 == CMPL || cop2 == CMPW || cop2 == CMPB)
	      &&  strcmp(pm->op1,"$0") == 0)
	     ||
	      ((cop2 == TESTL || cop2 == TESTW || cop2 == TESTB)
	      &&  samereg(pm->op1,pm->op2))
	    )
    &&  (cop1 < DIVB || cop1 > IMULL) /* flags are undefine in DIV and MUL */
    &&  (cop1 < MULB || cop1 > MULL)
  	&&  ((sets(pf) & CONCODES) == CONCODES)
	&&  strcmp(dst(pf),pm->op2) == 0
	&&  stype(cop1) == stype(cop2)
	&&  !isvolatile(pm,2)		/* non-volatile */
	&&	is_jcc_not_on_C_O(pl)
	&&	! (pl->nlive & CONCODES)
	&& ( ! (/*when shift,rotate with %cl, if %cl == 0 there is */
		(cop1 >= ROLB && cop1 <= SHRL) /*no setting of CC     */
	 && (samereg(pf->op1,"%cl"))))
	)
    {
	wchange();			/* changing window */
	lmrgin3(pf,pm,pf);		/* preserve line number info */ 
	mvlivecc(pm);		/* preserve condition code info */
	DELNODE(pm);			/* delete the compare */
	return(true);			/* announce success */
    }
/* block move improvement
**
** This improvement looks for a block move done a byte at a time, and
** attempts to do this with as many 4-byte blocks as possible, followed
** by 0 or 1 half-word moves and 0 or 1 byte moves.
*/

	if (
		cop1 == MOVL
	&&	isnumlit(pf->op1)
	&&	scanreg(pf->op2, false) == ECX
	&&	cop2 == REP
	&&	cop3 == SMOVB
	) {
		numbytes = atoi(pf->op1+1);
		if ((numwords = numbytes / 4) != 0) {
			wchange();		/* change the window */
			pf->op1 = getspace(sizeof("$2147483647"));
			sprintf(pf->op1, "$%d", numwords);
			if (numwords * 4 == numbytes)
				chgop(pl, SMOVL, "smovl");
			else {
				NODE *psmov = Saveop(0,"",0,GHOST);
				INSNODE(psmov, pl);
				psmov->nlive = pl->nlive;
				psmov->ndead = pl->ndead;
				makelive("%esi", psmov);
				makelive("%edi", psmov);
				chgop(psmov, SMOVL, "smovl");
				if (numwords * 4 + 2 >= numbytes) {
					if (numbytes % 2)
						chgop(pl, SMOVB, "smovb");
					else
						chgop(pl, SMOVW, "smovw");
				}
				else {
					NODE *psmov1 = Saveop(0,"",0,GHOST);
					INSNODE(psmov1, pl);
					psmov1->nlive = pl->nlive;
					psmov1->ndead = pl->ndead;
					makelive("%esi", psmov1);
					makelive("%edi", psmov1);
					chgop(psmov1, SMOVW, "smovw");
					chgop(pl, SMOVB, "smovb");
				}
			}
			return true;
		}
	}

/* Get move of a power of 2 into %ecx next to cltd instruction in order
     to set up peephole for signed division.
*/

   if (
	    cop1 == MOVL
	&&  cop3 == CLTD
	&&  ismove(pm,&src1,&dst1)
	&&  isnumlit(pf->op1)
	&&  ispwrof2(atoi(pf->op1+1), &ilog)
	&&  !strcmp(pf->op2, "%ecx")
	&&  !(usesreg(pm->op1, pf->op2) || usesreg(pm->op2, pf->op2))
      ) {

		wchange();
		lexchin(pf,pm);		/* preserve line number info */ 
		exchange(pf);		/* interchange instructions */
		swplivecc(pf,pm);	/* switch the live/dead info on
					   condition codes */
		makedead(pf->op2, pm);
		if (isreg(pm->op2))
			makelive(pm->op2, pf);
		return true;
   }
/* integer division improvement
**
** This improvement looks for integer division done by either 2, 4, or 8.
** If this division is done for the remainder, the optimization is only done
** if the divisor is 2.
*/
	if (
	    cop1 == MOVL
	&&  cop2 == CLTD
	&&  cop3 == IDIVL
	&&  isnumlit(pf->op1)
	&&  ispwrof2(constant = atoi(pf->op1+1), &ilog)
	&&  isreg(pf->op2)
	&&  isdead(pf->op2, pl)
	&&  !strcmp(pf->op2, pl->op1)
	) {
	    if (isdead("%edx", pl)) {
		if (constant == 2) {
			chgop(pf, CLTD, "cltd"); pf->op1 = pf->op2 = NULL;
			makelive(pm->op1 = "%edx", pf);
			chgop(pm, SUBL, "subl"); pm->op2 = pl->op2 = "%eax";
			chgop(pl, SARL, "sarl"); pl->op1 = "$1";
		}
		else {
			NODE *pn;

			pf->op1 = "%eax"; makelive(pf->op2 = "%edx", pf);

			chgop(pm, SARL, "sarl");
			pm->op1 = getspace(sizeof("$32"));
			sprintf(pm->op1, "$%d", ilog-1);
			pm->op2 = "%edx";

			chgop(pl, SHRL, "shrl");
			pl->op1 = getspace(sizeof("$32"));
			sprintf(pl->op1, "$%d", 32-ilog);
			makelive(pl->op2 = "%edx", pl);

			pn = insert(pl);
			chgop(pn, ADDL, "addl");
			pn->op1 = "%edx"; makelive(pn->op2 = "%eax", pn);

			pn = insert(pn);
			chgop(pn, SARL, "sarl");
			pn->op1 = getspace(sizeof("$32"));
			sprintf(pn->op1, "$%d", ilog);
			makelive(pn->op2 = "%eax", pn);
		}
/*
		while ((constant /= 2) >= 2) {
			NODE *pn;

			pn = insert(pl);
			chgop(pn, CLTD, "cltd");
			makelive(pm->op1, pn);
			makelive(pl->op2, pn);

			pn = insert(pn);
			chgop(pn, SUBL, "subl");
			pn->op1 = pm->op1;
			makelive(pn->op2 = pl->op2, pn);

			pn = insert(pn);
			chgop(pn, SARL, "sarl");
			pn->op1 = pl->op1;
			makelive(pn->op2 = pl->op2, pn);
		}
*/

		return true;
	    }
	    else if (isdead("%eax", pl) && constant == 2) {
		char *nlab = newlab();
    		NODE *pn, *new = Saveop(0,nlab,0,GHOST);

		pf->op1 = "%eax";
		makelive(pf->op2 = "%edx", pf);

		chgop(pm, ANDL, "andl"); pm->op1 = "$1"; pm->op2 = pf->op2;

		chgop(pl, TESTL, "testl"); pl->op1 = pl->op2 = pf->op1;
		pl->nlive |= CONCODES;

		pn = insert(pl);
		chgop(pn, JGE, "jge"); pn->op1 = nlab;
		makelive(pf->op2, pn);

		pn = insert(pn);
		chgop(pn, TESTL, "testl"); pn->op1 = pn->op2 = pf->op2;
		makelive(pf->op2, pn); pn->nlive |= CONCODES;

		pn = insert(pn);
		chgop(pn, JE, "je"); pn->op1 = nlab;
		makelive(pf->op2, pn);

		pn = insert(pn);
		chgop(pn, NEGL, "negl"); pn->op1 = pf->op2;
		makelive(pf->op2, pn);

    		APPNODE(new, pn);
		setlab(new);
		makelive(pf->op2, new);

		return true;
	    }
	}
		
/*
**	This transformation is for the following code, which
**	can be a common sequence from the rcc compiler.
**	The instruction sequence that is sought after is:
**
**		fstpl	X(ebp)	\
**		            		| fstl	X(ebp)
**		fldl	Y(ebp)		| faddl	Y(ebp)
**		faddl	X(ebp)	/
**
**	All of the operations may be converted, because there
**	are divrl, and subrl instructions non-communitive operators.
*/

    if (!ieee_flag
	&&  cop1 == FSTPL
	&&  cop2 == FLDL
	&&  ( cop3 == FADDL || cop3 == FSUBL || cop3 == FSUBRL ||
	      cop3 == FMULL || cop3 == FDIVL || cop3 == FDIVRL )
	&&  strcmp(pf->op1,pl->op1) == 0
	)
    {
	wchange();			/* change the window */
	lmrgin1(pl,pf,pf);		/* preserve line number info */
	pl->op1 = pm->op1;
	DELNODE(pm);			/* delete the extraneous moves */
	chgop (pf, FSTL, "fstl");
	if ( cop3 == FSUBL )
		chgop (pl, FSUBRL, "fsubrl");
	if ( cop3 == FDIVL )
		chgop (pl, FDIVRL, "fdivrl");
	if ( cop3 == FSUBRL )
		chgop (pl, FSUBL, "fsubl");
	if ( cop3 == FDIVRL )
		chgop (pl, FDIVL, "fdivl");
	return(true);
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

#ifdef P5
  if (! i586_opts || jflag || OpLength(pm) == WoRD)
#endif
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
	if (samereg(pm->op1,pm->op2))
		pm->op1 = pf->op1;
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
#ifdef P5
  if (! i586_opts || jflag || OpLength(pm) == WoRD)
#endif
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
	&&  ( scanreg(pm->op1, false) & (EAX|EDX|ECX) )
	&&  src1 == src2
	&&  dst1 == dst2
	&&  src1 == dst1
    	&&  (isdead(pm->op1,pl) && !(scanreg(pl->op2,false) & setreg(pm->op1)) ||
		strcmp(pm->op1,pl->op2) == 0)
	)
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
/* op= improvement
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movX	O1,R1		->	movX O1,R2
**	addX	O2,R1		->	addX O2,R2
**	movl	R1,R2
**
**	if R1 is dead after the sequence
**
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
	&&  cop3 == MOVL
	&&  isreg(pf->op2)
	&&  isreg(pm->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pm->op2) == 0
	&&  strcmp(pm->op2,pl->op1) == 0
	&&  isdead(pm->op2,pl)
	&&  pm->op3 == NULL
	&&  ! usesreg(pm->op1, pm->op2)
	&&  ! usesreg(pm->op1, pl->op2)
	)
    {
	wchange();		/* change the window */
	lmrgin1(pm,pl,pm);	/* preserve line number info */
	pf->op2 = pl->op2;
	pm->op2 = pl->op2;	/* Move fields around */
	makelive(pf->op2,pf);
	makelive(pf->op2,pm);
	makedead(pl->op1,pf);
	makedead(pl->op1,pm);
	DELNODE(pl);
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
#ifdef P5
  if (! i586_opts || jflag || OpLength(pm) == WoRD)
#endif
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
/*
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**		O1=	R  M			    R  M
**
**	movX	R1,R2	2  2			    0  0
**	addX	R3,R2	2  2	->	movX R1,O1  2  2
**	movX	R2,O1	2  2	->	addX R3,O1  2  7*
**
**	if O1 is not a register don't do it.
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
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pm->op1)
	&&  isreg(pm->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pm->op2) == 0
	&&  strcmp(pm->op2,pl->op1) == 0
	&&  isdead(pm->op2,pl)
	&&  src1 == src2
	&&  dst1 == dst2
	&&  src1 == dst1
	&&  !samereg(pm->op1,pm->op2) /* Must not have R2 == R3 */
	)
    if (
	    pm->op3 == NULL
	&&  setreg(pm->op1) != setreg(pl->op2)
	)
    {
	wchange();		/* change the window */
	pf->op2 = pl->op2;	/* Move fields around */
	pm->op2 = pl->op2;
	makelive(pl->op2,pf);
	makelive(pl->op2,pm);
	DELNODE(pl);
	return(true);
    }
/* *p++ improvement
**
**	movl	R1,R2
**	incl	R1		->	opX  O1,(R1)
**	opX	O1,(R2)		->	incl R1
**
** or
**	movl	R1,R2
**	addl	$imm,R1		->	opX  O1,(R1)
**	opX	O1,(R2)		->	addl $imm,R1

**	if R2 is dead after sequence
*/

    if (
	    cop1 == MOVL
	&&  isdyadic(pl)
	&&  isreg(pf->op1)
	&&  (  ( cop2 == INCL && isreg(pm->op1) && 
		 strcmp(pf->op1,pm->op1)==0 )
	    || ( cop2 == ADDL && isnumlit(pm->op1) && isreg(pm->op2) && 
	         strcmp(pf->op1,pm->op2)==0 )
	    )
	&&  isreg(pf->op2)
	&&  iszoffset(pl->op2,pf->op2)
	&&  ! strcmp(pf->op1,pf->op2) == 0
	&&  isdead(pf->op2,pl)
	&&  isdeadcc( pl )
	&&  ! usesreg(pl->op1,pf->op1)
	&&  ! usesreg(pl->op1,pf->op2)
	)
    {
	wchange();			/* change the window */
	lexchin(pm,pl);		/* preserve line number info */
	ldelin(pf);
	pl->op2 = getspace(NEWSIZE);
	pl->op2[0] = '(';
	pl->op2[1] = '\0';
	strcat(pl->op2,pf->op1);
	strcat(pl->op2,")");
	makelive(pf->op1,pl);	/* "R1" is now live in "op" node */
	makedead(pf->op2,pm);	/* "R2" is now dead in "incl" node */
	exchange(pm);		/* exchange the last two nodes */
	swplivecc(pm,pl);       /* swap live/dead info on condtion codes */
	DELNODE(pf);		/* delete first node */
	return(true);
    }
/* *p++ improvement
**
**	movl	R1,R2
**	incl	R1		->	opX  (R1),O1
**	opX	(R2),O1		->	incl R1
**
** or
**	movl	R1,R2
**	addl	$imm,R1		->	opX  (R1),O1
**	opX	(R1),O1		->	addl $imm,R1

**	if R2 is dead after sequence
**	opX can also be a PUSHW or PUSHL, with no 2nd operand
*/

    if (
	    cop1 == MOVL
	&&  isreg(pf->op1)
	&&  (  ( cop2 == INCL && isreg(pm->op1) && 
		 strcmp(pf->op1,pm->op1)==0 )
	    || ( cop2 == ADDL && isnumlit(pm->op1) && isreg(pm->op2) && 
	         strcmp(pf->op1,pm->op2)==0 )
	    )
	&&  isreg(pf->op2)
	&&  (  ( isdyadic(pl) &&  ! usesreg(pl->op2,pf->op1) &&
		 ! usesreg(pl->op2,pf->op2) )
	    || ( cop3 == PUSHW || cop3 == PUSHL )
	    )
	&&  iszoffset(pl->op1,pf->op2)
	&&  ! strcmp(pf->op1,pf->op2) == 0
	&&  isdead(pf->op2,pl)
	&&  isdeadcc( pl )
	)
    {
	wchange();			/* change the window */
	lexchin(pm,pl);		/* preserve line number info */
	ldelin(pf);
	pl->op1 = getspace(NEWSIZE);
	pl->op1[0] = '(';
	pl->op1[1] = '\0';
	strcat(pl->op1,pf->op1);
	strcat(pl->op1,")");
	makelive(pf->op1,pl);	/* "R1" is now live in "op" node */
	makedead(pf->op2,pm);	/* "R2" is now dead in "incl" node */
	exchange(pm);		/* exchange the last two nodes */
	swplivecc(pm,pl);       /* swap live/dead info on condtion codes */
	DELNODE(pf);		/* delete first node */
	return(true);
    }
/* case: rewrite addition or subtraction of constant from a register
**
**	
	movs[bw]l	R1,R2		->	mov[bw]	R1,R3
	incl	   	R2 		->	inc[bw]	R3
	mov[bw]		R2,R3

	movs[bw]l	R1,R2		->	mov[bw]	R1,R3
	leal	   	$n(R2),R2 	->	add[bw]	$n,R3
	mov[bw]		R2,R3
	
**
*/
    if ((cop1==MOVSBL ? srcsize = 1 : cop1 == MOVSWL ? srcsize = 2 : 0)
    	&& (cop3==MOVB ? targsize = 1 : cop3 == MOVW ? targsize = 2 : cop3 == MOVL ?
	    targsize= 3 : 0)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  isdead(pf->op2,pl)
	&&  ( 
	        ( 
		    (cop2 == INCL) ? (constant = 1, temp = pm->op1)  : 0
		)
					||
	        ( 
		    (cop2 == DECL ) ? (constant = -1, temp = pm->op1) : 0
		)
	    				|| 
		(
		    cop2 == LEAL  
		    && isdeadcc(pm)
 		    && (constant=strtol(pm->op1,&ptr,10))
	  	    && ptr[0] == '(' 
		    && ptr[5] == ')' 
		    && ( (strncmp(ptr+1,pm->op2,4) == 0) ? temp = pm->op2 : 0)
		)
	    				|| 
		(
		    cop2 == SUBL 
		    && pm->op1[0] == '$' 
		    && ( (constant= atoi(pm->op1+1)) ? temp = pm->op2 : 0 )
		)
	    )
        &&  isreg(temp)
        &&  scanreg(temp,0)&(EAX|ECX|EDX)
	&&  strcmp(pf->op2,temp) == 0
	/* check register match between second and third instructions */
	&& (targsize == 3 ? strcmp(temp+2,pl->op1+2) 
	   : targsize == 2 ? strcmp(temp+2,pl->op1+1) == 0 
	   : temp[2]==pl->op1[1] && pl->op1[2] == 'l')
	)
{
	wchange();
	makedead(pf->op2,pf);
	pf->op2 = pl->op2;
	makelive(pl->op2,pf);
	pm->nlive = pl->nlive;
	DELNODE(pl);
	
	if (srcsize == 2 && targsize == 3)
		chgop(pf,MOVSWL,"movswl");
	else if (srcsize == 2 && targsize == 2)
		chgop(pf,MOVW,"movw");
	else if (srcsize == 2 && targsize == 1)
		chgop(pf,MOVB,"movb");
	else if (srcsize == 1 && targsize == 3)
		chgop(pf,MOVSWL,"movswl");
	else if (srcsize == 1 && targsize == 2)
		chgop(pf,MOVSBW,"movsbw");
	else if (srcsize == 1 && targsize == 1)
		chgop(pf,MOVB,"movb");
	if (constant == 1) {
		if (targsize == 1)
			chgop(pm,INCB,"incb");
		else if (targsize == 2) 
			chgop(pm,INCW,"incw");
		else 
			chgop(pm,INCL,"incl");
		pm->op1 = pf->op2;
	}
	else if (constant == -1) {
		if (targsize == 1)
			chgop(pm,DECB,"decb");
		else if (targsize == 2) 
			chgop(pm,DECW,"decw");
		else 
			chgop(pm,DECL,"decl");
		pm->op1 = pf->op2;
	}
	else {
		if (targsize == 1) {
			if (cop2 == LEAL)
				chgop(pm,ADDB,"addb");
			else
				chgop(pm,SUBB,"subb");
		}
		else if (targsize == 2) {
			if (cop2 == LEAL)
				chgop(pm,ADDW,"addw");
			else
				chgop(pm,SUBW,"subw");
		}
		else {
			if (cop2 == LEAL)
				chgop(pm,ADDL,"addl");
			else
				chgop(pm,SUBL,"subl");
		}
		pm->op2 = pf->op2;
		pm->op1 = getspace(NEWSIZE);
		sprintf(pm->op1,"$%d", constant);
	}
	makelive(pl->op2,pl);
	pm->nlive = pl->nlive;
	return (true);
}
/* *p++ improvement
**
** This improvement rearranges things to facilitate a later 2-instruction
** improvement.  We're looking for the kind of code the compiler generates
** (naively) for *p++.  We want to make an indirect reference possible:
**
**	movl	O1,R		->	movl O1,R
**	incl	O1		->	movX (R),O2
**	movX	(R),O2		->	incl O1
**
** or
**	movl	O1,R		->	movl O1,R
**	incl	O1		->	movX O2,(R)
**	movX	O2,(R)		->	incl O1
**
**	if O1 is not a register used by O2
**	if instruction following movX is not conditional branch, since
**	  we're setting different condition codes than before
**	in the first case, O2 cannot be a register used by O1.
**
** Note that this transformation is always correct because:
**
**	1.  O1 could not use R, or the first 2 instructions wouldn't work.
**	2.  O2 can use or set R without problems.
*/

    if (
	    cop1 == MOVL
	&&  cop2 == INCL
	&&  ismove(pl,&src1,&dst1)
	&&  isreg(pf->op2)
	&&  strcmp(pf->op1,pm->op1) == 0
	&&  isdeadcc( pl )
	&&  ! usesvar(pm->op1,pl->op1)	/* O1 can't use O2 */
	&&  ! usesvar(pm->op1,pl->op2)	/* O1 can't use O2 */
	&&  ! usesvar(pl->op2,pm->op1)	/* O2 can't use O1 */
	&&  ! usesvar(pl->op1,pm->op1)	/* O2 can't use O1 */
	)
    {
	char * R = pf->op2;		/* point at register string */
	char * O1 = pf->op1;		/* point at first operand */
	char * O2;			/* second operand */

	if (
		(   ( O2 = pl->op2, iszoffset(pl->op1,R) ) /* test (R) */
		||  ( O2 = pl->op1, iszoffset(pl->op2,R) )
		)
	    &&  ! usesvar(O2,O1)	/* O2 can't use O1 */

	    &&  ! ( isvolatile(pm,1) 
		   && ( isvolatile(pl,1) || isvolatile(pl,2)))
		/* O1 and O2 must not both be volatile, */ 
		/* and O1 and (R) must not both be volatile. */
	    )
	{
	    wchange();			/* change the window */
	    lexchin(pm,pl);		/* preserve line number info */
	    exchange(pm);		/* exchange the last two nodes */
	    swplivecc(pm,pl);       /* swap live/dead info on condtion codes */
	    makelive(R,pf);
	    return(true);
	}
    }
#if 0
/* bit field improvement:
**
**  This improvement will optimize expressions involving bit fields checked
**  for zero or non-zero.
**
**	movl	OP,R1
**	shrl	$C1,R1
**	andl	$C2,R1	->	testl	$(C2<<C1),OP
**
**  R1 must be dead afterwards
*/
	if (cop1 == MOVL
	&&  cop2 == SHRL
	&&  cop3 == ANDL
	&&  isreg(pf->op2)
	&&  isreg(pm->op2)
	&&  isreg(pl->op2)
	&&  !strcmp(pf->op2, pm->op2)
	&&  !strcmp(pf->op2, pl->op2)
	&&  isdead(pf->op2, pl)
	&&  isnumlit(pm->op1)
	&&  isnumlit(pl->op1)
	&&  *pf->op1 != '$'
	) {
		wchange();
		lmrgin1(pf,pl,pl);	/* preserve line number info */
		constant = atoi(pl->op1+1) << atoi(pm->op1+1);
		chgop(pl, TESTL, "testl");
		pl->op1 = getspace(sizeof("$-2147483647"));
		sprintf(pl->op1, "$%d", constant);
		pl->op2 = pf->op1;
		DELNODE(pf);
		DELNODE(pm);
		return true;
	}
#endif

    return(false);
}

static boolean
ispwrof2(num, ilog)
int num, *ilog;
{
	if (num < 2)
		return false;
	else {
		int tmp;

		*ilog = 0;
		while ((tmp = num / 2) != 0) {
			++*ilog;
			if (num % 2)
				return false;
			num = tmp;
		}
		return true;
	}
}
