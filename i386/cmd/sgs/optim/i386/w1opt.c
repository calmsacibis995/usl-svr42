/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/w1opt.c	1.1.4.1"
/* w1opt.c
**
**	Intel 386 optimizer:  for one-instruction window
**
*/

#include "sched.h"
#include "optutil.h"

extern int i486_opts;
#ifdef P5
extern int i586_opts;
extern int jflag;
#endif

static int isaligned();

/*	D A N G E R
**
** This definition selects the highest numbered register that we
** can arbitrarily choose as a temporary in the multiply strength
** reduction that is performed below.  It should equal the highest
** numbered temporary register used by the compiler (1 less than
** lowest numbered register used for register variables.
*/

/* w1opt -- one-instruction optimizer
**
** This routine handles the single-instruction optimization window.
** See individual comments below about what's going on.
** In some cases (which are noted), the optimizations are ordered.
*/

struct RI { unsigned int reg_number;
			char * regname[3];
		  };
static struct RI reginfo[] = {
					 { EAX, { "%eax", "%ax", "%al"}},
						 { EDX, { "%edx", "%dx", "%dl"}},
						 { ECX, { "%ecx", "%cx", "%cl"}},
						 { EBX, { "%ebx", "%bx", "%bl"}},
						 { ESI, { "%esi", "%si", NULL }},
						 { EDI, { "%edi", "%di", NULL }},
						 { EBI, { "%ebi", "%bi", NULL }}
						};
static int regorder[] = { 1,2,0,3,4,5,6 };
static int nregs = sizeof(regorder) / sizeof(int);
extern NODE n0;
#ifdef STATISTICS
extern int first_run;
int try_peep = 0, done_peep = 0;
#endif
extern int suppress_enter_leave;

boolean					/* true if we make any changes */
/* ARGSUSED */
w1opt(pf,pl)
register NODE * pf;			/* pointer to first instruction in
					** window (and last)
					*/
NODE * pl;				/* pointer to last instruction in
					** window (= pf)
					*/
{

    register int cop = pf->op;		/* single instruction's op code # */
    boolean retval = false;		/* return value:  in some cases
					** we fall through after completing
					** an optimization because it could
					** lead into others.  This variable
					** contains the return state for the
					** end.
					*/
    register char *dp, *tmp;
    register int len, num;
    register unsigned int regs_set;

if (suppress_enter_leave) nregs=6;
/* eliminate dead code:
**
**	op ...
**   where the destination operand and all registers set are dead
*/

	if (!((regs_set=sets(pf)) & pf->nlive)	/* all regs set dead? */
	&&  regs_set != 0		/* leave instructions that don't
					** set any regs alone */
	&&  !(regs_set & (FP0 | FP1 | FP2 | FP3 | FP4 | FP5 | FP6 |FP7))
					/* don't mess with fp instr */
	&&  ! isbr(pf)			/* some branches set variables
					** and jump:  keep them */
	&&  (isdead(dp=dst(pf),pf) ||	/* are the destination and ... */
	     (!*dp && (
	      pf->op == TESTL || pf->op == CMPL || pf->op == TESTW ||
	      pf->op == TESTB || pf->op == CMPW || pf->op == CMPB ||
	      pf->op == SAHF)))  /* maybe SETcc when implemented? */
	&&  (pf->op1 == NULL || !isvolatile( pf,1 )) /* are operands non- */
	&&  (pf->op2 == NULL || !isvolatile( pf,2 )) /* volatile? */
	&&  (pf->op3 == NULL || !isvolatile( pf,3 ))
	)
    {
	wchange();			/* Note we're changing the window */
	ldelin2(pf);			/* preserve line number info */
	mvlivecc(pf);			/* preserve condition codes line info */
	DELNODE(pf);			/* discard instruction */
	return(true);			/* announce success */
    }

/*
**	leal num(reg),reg	->	addl	$num,reg
*/


    if (cop == LEAL
	&&  (*pf->op1 != '(' )  
	&&  scanreg(pf->op1, false) == scanreg(pf->op2, false)
	&&  (tmp = strchr(pf->op1, '('))
	&&  !strncmp(tmp+1, pf->op2, len = strlen(pf->op2))
	&&  *(tmp+1+len) == ')'
	&&  isdeadcc(pf)
	)

    {
		len = tmp - pf->op1;
		wchange();			/* note change */
		chgop(pf, ADDL, "addl");
		tmp = pf->op1;
		pf->op1 = getspace((unsigned)len+2);/* 1 for '$' and 1 for '\0' */
		sprintf(pf->op1, "$%.*s", len, tmp);
		return(true);			/* made a change */
    }

/*
**	mov[bw]	mem,reg		->	movl	mem,reg'
**
**	(if mem is 4-byte aligned)
*/


    if  (
	    i486_opts
	&&  isreg(pf->op2)
	&&  ((cop == MOVB && pf->op2[2] == 'l' && !isreg(pf->op1) &&
	      *pf->op1 != '$' && isaligned(pf->op1)) ||
	     (cop == MOVW && (isreg(pf->op1) || *pf->op1 == '$' ||
	      isaligned(pf->op1))))
	)
    {
	if (isreg(pf->op1)) switch (pf->op1[1]) {
		case 'a':
			pf->op1 = "%eax";
			break;
		case 'b':
			if (pf->op1[2] == 'i')
				pf->op1 = "%ebi";
			else
				pf->op1 = "%ebx";
			break;
		case 'c':
			pf->op1 = "%ecx";
			break;
		case 'd':
			if (pf->op1[2] == 'i')
				pf->op1 = "%edi";
			else
				pf->op1 = "%edx";
			break;
		case 's':
			pf->op1 = "%esi";
			break;
		default:
			goto cont;
	}

	switch (pf->op2[1]) {
		case 'a':
			pf->op2 = "%eax";
			pf->nlive |= EAX;
			break;
		case 'b':
			if (pf->op2[2] == 'i') {
				pf->op2 = "%ebi";
				pf->nlive |= EBI;
			} else {
				pf->op2 = "%ebx";
				pf->nlive |= EBX;
			}
			break;
		case 'c':
			pf->op2 = "%ecx";
			pf->nlive |= ECX;
			break;
		case 'd':
			if (pf->op2[2] == 'i') {
				pf->op2 = "%edi";
				pf->nlive |= EDI;
			} else {
				pf->op2 = "%edx";
				pf->nlive |= EDX;
			}
			break;
		case 's':
			pf->op2 = "%esi";
			pf->nlive |= ESI;
			break;
		default:
			goto cont;
	}
	wchange();			/* note change */
	chgop(pf, MOVL, "movl");
	return true;			/* made a change */
    }
cont:

/*
**	leal (,reg,num),reg	->	shll	$num',reg
*/


    if (cop == LEAL
	&&  *(tmp = pf->op1) == '(' && tmp[1] == ','
	&&  isreg(pf->op2)
	&&  !strncmp(tmp+2, pf->op2, strlen(pf->op2))
	&&  tmp[6] == ','
	&&  tmp[8] == ')'
	&&  ((num = tmp[7] - '0') == 2 || num == 4 || num == 8)
	&&  isdeadcc(pf)
	)
    {
	num = num == 4 ? 2 : num == 8 ? 3 : 1;

	wchange();			/* note change */
	chgop(pf, SHLL, "shll");
	pf->op1 = getspace(3);	/* 1 for '$', 1 for [248], and 1 for '\0' */
	sprintf(pf->op1, "$%d", num);
	return true;			/* made a change */
    }

/*
**	movzbl op,reg	->	xorl	reg,reg
**				movb	op,reg'
**			OR
**
**			->	movb	op,reg
**				andl	$255,reg
*/


    if (    i486_opts
	&&  cop == MOVZBL
	&&  !usesreg(pf->op1, pf->op2)
	)
    {
	NODE * pnew, *prepend();

	num = scanreg(pf->op2, false);

	if ((num & (EAX | EBX | ECX | EDX)) &&
	    (isreg(pf->op1) || !isaligned(pf->op1))) {
		wchange();			/* note change */

		pnew = prepend(pf, pf->op2);
		chgop(pnew, XORL, "xorl");
		pnew->op1 = pnew->op2 = pf->op2;
		pnew->nlive = pf->nlive;
		chgop(pf, MOVB, "movb");
		pf->op2 = getspace(4);		/* "%[abcd]l\0" */
		sprintf(pf->op2, "%%%cl", pnew->op1[2]);

		lexchin(pf,pnew);		/* preserve line number info */ 
		return(true);			/* made a change */
	}
	else if ((num & (EAX | EBX | ECX | EDX)) ||
		 (isreg(pf->op1) && pf->op1[2] == 'l') ||
		 isaligned(pf->op1)) {
		wchange();			/* note change */

		pnew = prepend(pf, pf->op2);
		chgop(pnew, MOVL, "movl");
		pnew->op2 = pf->op2;
		pnew->nlive = pf->nlive;
		if (isreg(pf->op1)) {
			tmp = pf->op1;
			pf->op1 = getspace(5);	/* "%e[abcd]x\0" */
			sprintf(pf->op1, "%%e%cx", tmp[1]);
		}
		pnew->op1 = pf->op1;

		chgop(pf, ANDL, "andl");
		pf->op1 = "$255";

		lexchin(pf,pnew);		/* preserve line number info */ 
		return(true);			/* made a change */
	}
    }

/*
**	pushl mem	->	movl	mem,%eax
**				pushl	%eax
**
**	Note that for this improvement, we must be optimizing for a 486,
**	as this sequence is slower on a 386;  also, at least one scratch
**	register must not be live after this instruction.
*/


    if (    i486_opts
	&&  cop == PUSHL
	&&  *pf->op1 != '%'
	&&  *pf->op1 != '$'
	)

    {
	NODE * pnew, *prepend();
	int i;
	for (i=0; i < nregs; i++)
	  if (! (pf->nlive & reginfo[regorder[i]].reg_number)) {
		tmp = reginfo[regorder[i]].regname[0];
		break;
	  }

    if (i==nregs)  return false;

	wchange();			/* note change */

#ifdef P5
	if (i586_opts) rotate(regorder,3);
#endif
	pnew = prepend(pf, tmp);
	chgop(pnew, MOVL, "movl");
	pnew->op1 = pf->op1;
	pf->op1 = pnew->op2 = tmp;
	pnew->userdata = pf->userdata; /* preserve volatile info */
	
	lexchin(pf,pnew);		/* preserve line number info */ 
	return(true);			/* made a change */
    }

#ifdef P5
  if (i586_opts && ! jflag) {
/* change any dyadic op on memory to move memory to register and
** then op. example:
** addl X,R1 -> movl X,R2
**              addl R2,R1
*/
    if ((ISRISCY(pf) || Istest(pf))
	&& *pf->op1 != '%'
	&& *pf->op1 != '$'
	)
    {
	NODE * pnew, *prepend();
	int i;
#ifdef STATISTICS
	if (first_run) {
	  ++try_peep;
	}
#endif
	for (i=0; i<nregs; i++) {
	  if (OpLength(pf) == ByTE &&
			 reginfo[regorder[i]].regname[0][3] == 'i') continue;
	  if (! (pf->nlive & reginfo[regorder[i]].reg_number)
		 && ! ((scanreg(pf->op1,0) | scanreg(pf->op2,0))
		 & reginfo[regorder[i]].reg_number)
		 ) {
		tmp = reginfo[regorder[i]].regname[0];
		break;
	  }
	}/*for loop*/
	if (i==nregs) return false;

#ifdef DEBUGW1
	fprintf(stderr,"was: "); fprinst(pf);
#endif

	switch(OpLength(pf)) {
	  case ByTE:
				 tmp = reginfo[regorder[i]].regname[2];
		     pnew = prepend(pf, tmp);
		     chgop(pnew, MOVB, "movb");
				 break;
	  case LoNG:
		     pnew = prepend(pf, tmp);
		     chgop(pnew, MOVL, "movl");
				 break;
	  default  : 
			     return false;
	}/*end switch*/
	
#ifdef STATISTICS
	if (first_run) ++done_peep;
#endif
	wchange();			/* note change */
#ifdef P5
	if (i586_opts)	rotate(regorder,3);
#endif
	pnew->op1 = pf->op1;
	pf->op1 = tmp;
	pnew->nlive |= scanreg(pnew->op1,false);
	makelive(pf->op2,pnew);
#ifdef DEBUGW1
	fprintf(stderr,"became: "); fprinst(pnew); fprinst(pf);
#endif
	lexchin(pf,pnew);		/* preserve line number info */ 
	return(true);			/* made a change */
    }
	/*
	** The other direction:
	** op reg1,X -> mov X,reg2
	**              op  reg1,reg2
	**              mov reg2,X
	*/
	if (ISRISCY(pf) && *pf->op2 != '%')
	  {
	    NODE *pnew, *prepend();
	int i;
		int top;
		char *topc;
#ifdef STATISTICS
	    if (first_run) {
		  ++try_peep;
		}
#endif
		 for (i=0; i<nregs; i++) {
		   if (OpLength(pf) == ByTE &&
				   reginfo[regorder[i]].regname[0][3] == 'i') continue;
		   if (! (pf->nlive & reginfo[regorder[i]].reg_number)
			  && ! ((scanreg(pf->op2,0) | scanreg(pf->op1,0))
			   & reginfo[regorder[i]].reg_number)
			  ) {
			 tmp = reginfo[regorder[i]].regname[0];
			 break;
		   }
		 }/*for loop*/
		 if (i==nregs)  return false;
	    switch(OpLength(pf)) {
		  case ByTE:
					tmp = reginfo[regorder[i]].regname[2];
					top = MOVB;
					topc = "movb";
				    break;
  
		  case LoNG:
					  top = MOVL;
					  topc = "movl";
					  break;
		  default:
					  return false;
	}/*end switch*/
#ifdef DEBUGW1
		fprintf(stderr,"was "); fprinst(pf);
#endif
	    wchange();
#ifdef P5
		if (i586_opts)	rotate(regorder,3);
#endif
	    pnew = prepend(pf,tmp);
	    chgop(pnew,top,topc);
	    pnew->op1 = pf->op2;
		pnew->nlive |= scanreg(pnew->op1,false);
		makelive(pf->op1,pnew);
		pf->op2 = tmp;
		makelive(pf->op2,pf);
		pf->nlive |= scanreg(pnew->op1,false);
	    lexchin(pf,pnew);
		pnew = insert(pf);
		chgop(pnew,top,topc);
		pnew->op1 = tmp;
		pnew->op2 = getspace(strlen(pf->back->op1));
		(void) strcpy(pnew->op2,pf->back->op1);
		pnew->nlive = pf->nlive;
#ifdef STATISTICS
	    if (first_run) ++done_peep;
#endif
#ifdef DEBUGW1
fprintf(stderr,"became "); fprinst(pf->back); fprinst(pf); fprinst(pf->forw);
#endif
	return(true);
	 }
   /* The other direction for compare, test:
   ** cmp reg1,X -> mov X,reg2
   **               cmp reg1,reg2
   */
	if (Istest(pf)
	&& *pf->op2 != '%'
	&& *pf->op2 != '$' )
	 { NODE *pnew, *prepend();
	   int i;
	   int top;
	   char *topc;
#ifdef STATISTICS
	   if (first_run) {
		 ++try_peep;
	   }
#endif
#ifdef DEBUGW1
	fprintf(stderr,"was: "); fprinst(pf);
#endif
	   for (i=0; i<nregs; i++) {
		 if (OpLength(pf) == ByTE &&
			 reginfo[regorder[i]].regname[0][3] == 'i') continue;
		 if ((! (pf->nlive & reginfo[regorder[i]].reg_number))
			&& (!((scanreg(pf->op2,0) | scanreg(pf->op1,0))
			& reginfo[regorder[i]].reg_number))
			) {
		   tmp = reginfo[regorder[i]].regname[0];
		   break;
		 }
	   }/*for loop*/
	   if (i == nregs)  return false;

#ifdef DEGUB
	   fprintf(stderr,"cmp: %s",tmp); plive(pf->nlive,0); fprinst(pf);
#endif
	   switch(OpLength(pf)) {
	     case ByTE:
					tmp = reginfo[regorder[i]].regname[2];
					top = MOVB;
					topc = "movb";
					break;
		 case LoNG:
					top = MOVL;
					topc = "movl";
					break;
		 default:
					return false;
	   }/*end switch*/
#ifdef STATISTICS
	   if (first_run) ++done_peep;
#endif
	   wchange();
#ifdef P5
	   if(i586_opts)	rotate(regorder,3);
#endif
	   pnew = prepend(pf,tmp);
	   chgop(pnew,top,topc);
	   pnew->op1 = pf->op2;
	   makelive(pf->op1,pnew);
	   pf->op2 = tmp;
	   lexchin(pf,pnew);
#ifdef DEBUGW1
	   fprintf(stderr,"became: "); fprinst(pnew); fprinst(pf);
#endif
       return(true);
	}
	/* The same as above for INC DEC NOT and NEG */
    if (Isreflexive(pf) &&  *pf->op1 != '%' )
    {
	NODE * pnew, *prepend();
	int i;
	int top;
	char *topc;
#ifdef DEBUGW1
fprintf(stderr,"was "); fprinst(pf);
#endif
#ifdef STATISTICS
	if (first_run) {
	  ++try_peep;
	}
#endif
	for (i=0; i<nregs; i++) {
      if (OpLength(pf) == ByTE &&
			 reginfo[regorder[i]].regname[0][3] == 'i') continue;
	  if (! (pf->nlive & reginfo[regorder[i]].reg_number)
	   && (! (scanreg(pf->op1,false) & reginfo[regorder[i]].reg_number))
		) {
		   tmp = reginfo[regorder[i]].regname[0];
		   break;
		 }
	}/*for loop*/
	if (i==nregs)  return false;

	switch (OpLength(pf)) {
	  case ByTE: 
				 tmp = reginfo[regorder[i]].regname[2];
				 top = MOVB;
				 topc = "movb";
				 break;
      case LoNG:
				 top = MOVL;
				 topc = "movl";
				 break;
	  default:
				 return false;
    }/*end switch*/

#ifdef STATISTICS
	if (first_run) ++done_peep;
#endif
	wchange();			/* note change */
#ifdef P5
	if (i586_opts)	rotate(regorder,3);
#endif
	pf->nlive |= scanreg(pf->op1,false);
	pnew = prepend(pf, tmp);
	pnew->nlive |= scanreg(pf->op1,false);
	chgop(pnew, top, topc);
	pnew->op1 = pf->op1;
	lexchin(pf,pnew);		/* preserve line number info */ 

	pf->op1 = tmp;
	makelive(pf->op1,pf);
	
	pnew = insert(pf);
	chgop(pnew, top, topc);
	pnew->op1 = tmp;
	pnew->op2 = getspace(strlen(pf->back->op1));
	(void) strcpy(pnew->op2,pf->back->op1);
	pnew->nlive = pf->nlive;
#ifdef DEBUGW1
fprintf(stderr,"became "); fprinst(pf->back); fprinst(pf); fprinst(pf->forw);
#endif
	return(true);			/* made a change */
    }
/*
** op $imm,disp(r1) -> mov $imm,r2
**	     			   op r2,disp(r1)
*/
    if ((ISRISCY(pf) || Istest(pf))
	 && *pf->op1 == '$'
	 && hasdisplacement(pf->op2)
	 )
	 {
	  NODE * pnew, *prepend();
	  int i;
	  int top;
	  char *topc;
#ifdef DEBUGW1
fprintf(stderr,"was 11"); fprinst(pf);
#endif
#ifdef STATISTICS
	if (first_run) {
	  ++try_peep;
	}
#endif
	for (i=0; i<nregs; i++) {
	  if (! (pf->nlive & reginfo[regorder[i]].reg_number) && 
		 (! (scanreg(pf->op1,false) & reginfo[regorder[i]].reg_number))
		) {
		   tmp = reginfo[regorder[i]].regname[0];
		   break;
		 }
	}/*for loop*/
	if (i==nregs)  return false;
#ifdef STATISTICS
	   if (first_run) ++done_peep;
#endif
	   if (OpLength(pf) == ByTE) {
		 tmp = reginfo[regorder[i]].regname[2];
		 top = MOVB;
		 topc = "movb";
	   } else {
		 top = MOVL;
		 topc = "movl";
	   }
	   wchange();
#ifdef P5
	   if (i586_opts)	rotate(regorder,3);
#endif
	   pnew = prepend(pf,tmp);
	   chgop(pnew,top,topc);
	   pnew->op1 = pf->op1;
	   pf->op1 = tmp;
	   lexchin(pf,pnew);
#ifdef DEBUGW1
	   fprintf(stderr,"became: "); fprinst(pnew); fprinst(pf);
#endif
       return(true);
	}
}
#endif

/*
**	cmpl $imm,mem	->	movl	mem,%eax
**				cmpl	$imm,%eax
**
**	This will only be done if a displacement is in the memory operand.
**	Note that for this improvement, we must be optimizing for a 486,
**	as this sequence is slower on a 386;  also, at least one scratch
**	register must not be live after this instruction.
*/


    if (    i486_opts
	&&  (cop == CMPL || cop == TESTL || cop == CMPB ||
	     cop == TESTB || cop == CMPW || cop == TESTW)
	&&  *pf->op1 == '$'
	&&  *pf->op2 != '%'
	&&  *pf->op2 != '$'
	&&  *pf->op2 != '('
	)
    {
	NODE * pnew, *prepend();
	int i;

	for (i=0; i<nregs; i++) {
      if (OpLength(pf) == ByTE &&
			 reginfo[regorder[i]].regname[0][3] == 'i') continue;
	  if (! (pf->nlive & reginfo[regorder[i]].reg_number)
	   && (! (scanreg(pf->op1,false) & reginfo[regorder[i]].reg_number))
		) {
		   tmp = reginfo[regorder[i]].regname[0];
		   break;
		 }
	}/*for loop*/
	if (i==nregs) 
		return false;


	wchange();			/* note change */

	switch(OpLength(pf)) {
		case ByTE:	tmp = reginfo[regorder[i]].regname[2];
					pnew = prepend(pf,tmp);
					chgop(pnew,MOVB,"movb");
					pnew->op1 = pf->op2;
					pf->op2 = pnew->op2;
					break;
		case WoRD:  if (!isaligned(pf->op2)) {
						tmp = reginfo[regorder[i]].regname[1];
						pnew = prepend(pf,tmp);
						chgop(pnew,MOVW,"movw");
						pnew->op1 = pf->op2;
						pnew->op2 = pf->op2 = tmp;
						break;
					}
					/*FALLTHRU*/
		default:  	pnew = prepend(pf,tmp);
					chgop(pnew,MOVL,"movl");
					pnew->op1 = pf->op2;
					pnew->op2 = pf->op2 = tmp;
					if (OpLength(pf) == WoRD)
						pf->op2 = reginfo[regorder[i]].regname[1]; 
	}/*end switch*/
	pnew->nlive &= ~CONCODES;

#ifdef P5
	if (i586_opts)	rotate(regorder,3);
#endif
	lexchin(pf,pnew);		/* preserve line number info */ 
	return(true);			/* made a change */
    }

/*
**	cmp[bwl] $0,R	->	test[bwl] R,R
*/


    if (
	    (
	    cop == CMPL || cop == CMPW || cop == CMPB
	    )
	&&  strcmp(pf->op1,"$0") == 0
	&&  isreg(pf->op2)
	)

    {
	wchange();			/* note change */
	switch ( cop ) {		/* change the op code */
	    case CMPL:
		chgop(pf,TESTL,"testl"); break;
	    case CMPW:
		chgop(pf,TESTW,"testw"); break;
	    case CMPB:
		chgop(pf,TESTB,"testb"); break;
	}
	pf->op1 = pf->op2;		/* both operands point at R */
	makelive(pf->op2,pf);		/* make register appear to be live
	pf->nlive |= CONCODES;		** hereafter so "compare" isn't thrown
					** away.  (Otherwise R might be dead
					** and we would eliminate the inst.)
					*/
	return(true);			/* made a change */
    }

/*
**	addl $1,O1	->	incl O1
**
**	Also for dec[bwl] and inc[bw]
*/


    if (
	    (
	    cop == ADDL || cop == ADDW || cop == ADDB ||
	    cop == SUBL || cop == SUBW || cop == SUBB
	    )
	&&  strcmp(pf->op1,"$1") == 0
	)

    {
	wchange();			/* note change */
	switch ( cop ) {		/* change the op code */
	    case ADDL:
		chgop(pf,INCL,"incl"); break;
	    case ADDW:
		chgop(pf,INCW,"incw"); break;
	    case ADDB:
		chgop(pf,INCB,"incb"); break;
	    case SUBL:
		chgop(pf,DECL,"decl"); break;
	    case SUBW:
		chgop(pf,DECW,"decw"); break;
	    case SUBB:
		chgop(pf,DECB,"decb"); break;
	}
	pf->op1 = pf->op2;		/* both operands point at R */
	pf->op2 = NULL;
	makelive(pf->op1,pf);		/* make register appear to be live
					** hereafter so "compare" isn't thrown
					** away.  (Otherwise R might be dead
					** and we would eliminate the inst.)
					*/
	return(true);			/* made a change */
    }
/*
**	mov[bwl] $0,R1	->	xor[bwl] R1,R1
**
**	This is the same speed, but more compact.
*/


    if (
	    (
	    cop == MOVL || cop == MOVW || cop == MOVB
	    )
	&&  strcmp(pf->op1,"$0") == 0
	&&  isreg(pf->op2)
	)

    {
	wchange();			/* note change */
	switch ( cop ) {		/* change the op code */
	    case MOVL:
		chgop(pf,XORL,"xorl"); break;
	    case MOVW:
		chgop(pf,XORW,"xorw"); break;
	    case MOVB:
		chgop(pf,XORB,"xorb"); break;
	}
	pf->op1 = pf->op2;		/* both operands point at R */
	makelive(pf->op1,pf);		/* make register appear to be live
					** hereafter so "compare" isn't thrown
					** away.  (Otherwise R might be dead
					** and we would eliminate the inst.)
					*/
	return(true);			/* made a change */
    }

/* get rid of useless arithmetic
**
**	addl	$0,O		->	deleted  or  cmpl O,$0
**	subl	$0,O		->	deleted  or  cmpl O,$0
**	orl	$0,O		->	deleted  or  cmpl O,$0
**	xorl	$0,O		->	deleted  or  cmpl O,$0
**	sall	$0,O		->	deleted  or  cmpl O,$0
**	sarl	$0,O		->	deleted  or  cmpl O,$0
**	shll	$0,O		->	deleted  or  cmpl O,$0
**	shrl	$0,O		->	deleted  or  cmpl O,$0
**	mull	$1,O		->	deleted  or  cmpl O,$0
**	imull	$1,O		->	deleted  or  cmpl O,$0
**	divl	$1,O		->	deleted  or  cmpl O,$0
**	idivl	$1,O		->	deleted  or  cmpl O,$0
**	andl	$-1,O		->	deleted  or  cmpl O,$0

**	mull	$0,O		->	movl $0,O
**	imull	$0,O		->	movl $0,O
**	andl	$0,O		->	movl $0,O
**
** Note that since we've already gotten rid of dead code, we won't
** check whether O (O2) is live.  However, we must be careful to
** preserve the sense of result indicators if a conditional branch
** follows some of these changes.
*/

/* Define types of changes we will make.... */

#define	UA_NOP		1		/* no change */
#define UA_DELL		2		/* delete instruction */
#define UA_DELW		3		/* delete instruction */
#define UA_DELB		4		/* delete instruction */
#define UA_MOVZL	5		/* change to move zero to ... */
#define UA_MOVZW	6		/* change to move zero to ... */
#define UA_MOVZB	7		/* change to move zero to ... */
/* We must have a literal as the first operand. */
    if ( isnumlit(pf->op1) )
    {
	int ultype = UA_NOP;		/* initial type of change = none */

	switch(atol(pf->op1+1))			/* branch on literal */
	{
	case 0:				/* handle all instructions with &0
					** as first operand
					*/
	    switch (cop)
	    {
	    case ADDL:
	    case SUBL:
	    case ORL:
	    case XORL:
	    case SALL:
	    case SHLL:
	    case SARL:
	    case SHRL:
		ultype = UA_DELL;
		break;
	    
	    case ADDW:
	    case SUBW:
	    case ORW:
	    case XORW:
	    case SALW:
	    case SHLW:
	    case SARW:
	    case SHRW:
		ultype = UA_DELW;
		break;
	    
	    case ADDB:
	    case SUBB:
	    case ORB:
	    case XORB:
	    case SALB:
	    case SHLB:
	    case SARB:
	    case SHRB:
		ultype = UA_DELB;
		break;
	    
	    case MULL:
	    case IMULL:
	    case ANDL:
		ultype = UA_MOVZL;	/* convert to move zero */
		break;

	    case MULW:
	    case IMULW:
	    case ANDW:
		ultype = UA_MOVZW;	/* convert to move zero */
		break;
	    
	    case MULB:
	    case IMULB:
	    case ANDB:
		ultype = UA_MOVZB;	/* convert to move zero */
		break;
	    }
	    break;			/* done $0 case */

	case 1:				/* &1 case */
	    switch( cop )		/* branch on op code */
	    {
	    case DIVL:
	    case IDIVL:
	    case MULL:
	    case IMULL:
		ultype = UA_DELL;	
		break;
	    
	    case DIVW:
	    case IDIVW:
	    case MULW:
	    case IMULW:
		ultype = UA_DELW;	
		break;
	    
	    case DIVB:
	    case IDIVB:
	    case MULB:
	    case IMULB:
		ultype = UA_DELB;	
		break;
	    }
	    break;			/* done $1 case */
	
	case -1:			/* $-1 case */
	    switch ( cop )		/* branch on op code */
	    {
	    case ANDL:
		ultype = UA_DELL;	
		break;
	    
	    case ANDW:
		ultype = UA_DELW;	
		break;
	    
	    case ANDB:
		ultype = UA_DELB;	
		break;
	    }
	    break;			/* end $-1 case */
	} /* end switch on immediate value */
/* Now do something, based on selections made above */

	switch ( ultype )
	{
	case UA_MOVZL:			/* change to move zero to operand */
	case UA_MOVZW:			/* change to move zero to operand */
	case UA_MOVZB:			/* change to move zero to operand */
	    if (isvolatile(pf,2))	/* if dest is volatile, don't */
		break;			/* touch this. */
	    wchange();
		if (isdeadcc(pf)) {
		pf->op1 = "$0";		/* first operand is zero */
		pf->op2 = dst(pf);		/* second is ultimate destination */
		pf->op3 = NULL;		/* clean out if there was one */
		switch ( ultype ) {
				case UA_MOVZL:
				chgop(pf,MOVL,"movl");	/* change op code */
				break;
				case UA_MOVZW:
				chgop(pf,MOVW,"movw");	/* change op code */
				break;
				case UA_MOVZB:
				chgop(pf,MOVB,"movb");	/* change op code */
				break;
		}
		} else if (cop != ANDL && cop != ANDW && cop != ANDB) {
		switch ( ultype ) {
				case UA_MOVZL:
				chgop(pf,ANDL,"andl");	/* change op code */
				break;
				case UA_MOVZW:
				chgop(pf,ANDW,"andw");	/* change op code */
				break;
				case UA_MOVZB:
				chgop(pf,ANDB,"andb");	/* change op code */
				break;
		}
		}
	    retval = true;		/* made a change */
	    break;
	
/* For this case we must be careful:  if a following instruction is a
** conditional branch, it is clearly depending on the result of the
** arithmetic, so we must put in a compare against zero instead of deleting
** the instruction.
*/

	case UA_DELL:			/* delete instruction */
	case UA_DELW:			/* delete instruction */
	case UA_DELB:			/* delete instruction */

	    if (isvolatile(pf,2))	/* if dest is volatile, don't */
		break;			/* touch this. */
	    wchange();			/* we will make a change */

	    if ( ! isdeadcc(pf) )
	    {
			if (isreg(pf->op2)) {
				switch ( ultype ) {
				case UA_DELL:
					chgop(pf,TESTL,"testl");
					break;
				case UA_DELW:
					chgop(pf,TESTW,"testw");
					break;
				case UA_DELB:
					chgop(pf,TESTB,"testb");
					break;
				}
				pf->op1 = pf->op2;	/* always test second operand */
				pf->op3 = NULL;		/* for completeness */
				retval = true;		/* made a change */
			} else {
				switch ( ultype ) {
				case UA_DELL:
					chgop(pf,CMPL,"cmpl");
					break;
				case UA_DELW:
					chgop(pf,CMPW,"cmpw");
					break;
				case UA_DELB:
					chgop(pf,CMPB,"cmpb");
					break;
				}
				retval = true;
				pf->op1 = "$0";
			}
	    }
	    else
	    {
		ldelin2(pf);		/* preserve line number info */
		mvlivecc(pf);		/* preserve cond. codes line info */
		DELNODE(pf);		/* not conditional; delete node */
		return(true);		/* say we changed something */
	    }
	    break;
	} /* end case that decides what to do */
	
	cop = pf->op;			/* reset current op for changed inst. */

    } /* end useless arithmetic removal */
/* discard useless mov's
**
**	movw	O,O		->	deleted
** Don't worry about condition codes, because mov's don't change
** them anyways.
*/

    if ( (  pf->op == MOVB
	 || pf->op == MOVW
	 || pf->op == MOVL
	 )
	&&  strcmp(pf->op1,pf->op2) == 0
	&&  !isvolatile(pf,1)		/* non-volatile */
	)
    {
	wchange();			/* changing the window */
	ldelin2(pf);			/* preserve line number info */
	mvlivecc(pf);			/* preserve condition codes line info */
	DELNODE(pf);			/* delete the movw */
	return(true);
    }


/* For Intel 386, a shift by one bit is more efficiently
** done as an add.
**
**	shll $1,R		->	addl R,R
**
*/

    {
	if( ( pf->op == SHLL ||
	      pf->op == SHLW ||
	      pf->op == SHLB )
	    && strcmp( pf->op1, "$1" ) == 0 
	    && isreg(pf->op2)		/* safe from mmio */
	    ) {
		if( pf->op == SHLL ) {
			chgop( pf, ADDL, "addl" );
			pf->op1 = pf->op2;
			return( true );
		}
		if( pf->op == SHLW ) {
			chgop( pf, ADDW, "addw" );
			pf->op1 = pf->op2;
			return( true );
		}
		if( pf->op == SHLB ) {
			chgop( pf, ADDB, "addb" );
			pf->op1 = pf->op2;
			return( true );
		}
	}
    }

    return(retval);			/* indicate whether anything changed */
}
extern int auto_elim;

static boolean
isaligned(op)
char *op;
{
char *mystrstr();
	int regs = scanreg(op, false);
	unsigned int frame_pointer = auto_elim ? ESP : EBP ;

	if (regs == frame_pointer)
		return atoi(op) % 4 ? false : true;
	else if (regs & frame_pointer)
		return (mystrstr(op, ",4") || mystrstr(op, ",8") ) ? true : false;
	else if (regs == ESP)
		return atoi(op) % 4 ? false : true;
	else if (regs & ESP)
		return (mystrstr(op, ",4") || mystrstr(op, ",8") ) ? true : false;
	else
		return false;
}
