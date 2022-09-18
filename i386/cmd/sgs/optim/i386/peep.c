/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/peep.c	1.1.6.1"
/* peep.c
**
**	Intel 386 peephole improvement driver
**
**
** This module contains the driver for the Intel 386 peephole improver.
*/


#include "optim.h"
#include "optutil.h"
#include "regal.h"
#include "database.h"
#include <malloc.h>

extern boolean w1opt();
extern boolean w2opt();
extern boolean w3opt();
extern boolean w4opt();
extern boolean w6opt();
extern void window();
extern boolean lookup_regals();
extern char *mystrstr();
extern void sets_and_uses();
extern int fp_removed;

extern int dflag;			/* non-0 to display live/dead data */
extern int hflag;			/* non-0 to disable peephole entirely */
extern int tflag;			/* non-0 to disable redundant load op */
extern int Tflag;			/* non-0 to display t debugging       */

static void prld();			/* routine to print live/dead data */
static void rdfree();

void peep()
{


    if (hflag == 0)
    {
#ifdef LIVEDEAD
	if (dflag != 0)			/* if enabled */
	    prld();			/* print the world's live/dead data */
#endif

	window(6, w6opt);		/* do 6-instruction sequences */
	window(3, w3opt);		/* do 3-instruction sequences */
	window(1, w1opt);		/* do 1-instruction sequences */
	window(2, w2opt);		/* do 2-instruction sequences */
	window(3, w3opt);		/* do 3-instruction sequences */
	window(4, w4opt);		/* do 4-instruction sequences */

	window(1, w1opt);		/* now repeat to clean up stragglers */
	window(2, w2opt);
	window(1, w1opt);
	sets_and_uses();		/* update sets and uses fields of NODEs */
    }
    return;
}

void
rmrdpp()
{
  NODE *p;
  NODE *pushESI=NULL,
	   *pushEDI=NULL,
	   *pushEBX=NULL,
	   *pushEBI=NULL,
	   *popESI=NULL,
	   *popEDI=NULL,
	   *popEBX=NULL,
	   *popEBI=NULL;
  int add_stack;
  int ebi_is_used =0, ebx_is_used =0, edi_is_used = 0, esi_is_used = 0;
  int nfound = 0;
  int has_ret = 0;

  for(p = ntail.back; p != &n0; p = p->back) {
	if ((p->op == RET) || (p->op == LRET)) {
		has_ret = true;
		break;
	}
  }
  if (! has_ret)
	return;
  for (ALLN(p)) {  /* find push ebx, push esi and push edi */
	if (p->op == PUSHL)
	  switch(p->uses&~ESP) {
		case EBX:
				  if (! pushEBX) {
					pushEBX = p;
					++nfound;
				  }
				  break;
		case EDI:
				  if (! pushEDI) {
					pushEDI = p;
					++nfound;
				  }
				  break;
		case ESI:
				  if (! pushESI) {
					pushESI = p;
					++nfound;
				  }
				  break;
		case EBI:
				  if (! pushEBI) {
					pushEBI = p;
					++nfound;
				  }
				  break;
		default :
				  break;
	  }/*end switch*/
	 if (nfound == 4) break;
   }/*end for loop*/
   if (! pushEBX) ebx_is_used = 1;
   if (! pushESI) esi_is_used = 1;
   if (! pushEDI) edi_is_used = 1;
   if (! pushEBI) ebi_is_used = 1;
   nfound = 0;
   for (p=ntail.back; p != &n0; p=p->back) {
	if (p->op == POPL)
	  switch(p->sets&~ESP) {
		case EBX:
				  if (! popEBX) {
					popEBX = p;
					++nfound;
				  }
				  break;
		case EDI:
				  if (! popEDI) {
					popEDI = p;
					++nfound;
				  }
				  break;
		case ESI:
				  if (! popESI) {
					popESI = p;
					++nfound;
				  }
				  break;
		case EBI:
				  if (! popEBI) {
					popEBI = p;
					++nfound;
				  }
				  break;
		default :
				  break;
	  }/*end switch*/
	  if (nfound == 4) break;
  }/*for loop*/

  for (ALLN(p)){
	if (p == pushEBX || p == pushEDI || p == pushESI || p == pushEBI)
		continue;
	if (p->op == RET || p == popEBX || p == popEDI || p == popESI || p == popEBI)
		continue;
	if ((p->uses | p->sets) & EBX) {
	  ebx_is_used = 1;
	}
	if ((p->uses | p->sets) & EDI) {
	  edi_is_used = 1;
	}
	if ((p->uses | p->sets) & ESI) {
	  esi_is_used = 1;
	}
	if ((p->uses | p->sets) & EBI) {
	  ebi_is_used = 1;
	}
  }/*for loop*/
  add_stack = 0;
  if ( ! ebx_is_used) {
	DELNODE(pushEBX);
	DELNODE(popEBX);
	add_stack+=4;
  }
  if ( ! esi_is_used) {
	DELNODE(pushESI);
	DELNODE(popESI);
	add_stack+=4;
  }
  if ( ! edi_is_used) {
	DELNODE(pushEDI);
	DELNODE(popEDI);
	add_stack+=4;
  }
  if ( ! ebi_is_used) {
	DELNODE(pushEBI);
	DELNODE(popEBI);
	add_stack+=4;
  }
  for( ALLN( p ) ) {
	if(p->op == CALL) 
		p->opm -= add_stack;
  }
}/*end rmrdpp*/

/* Print live/dead data for all instruction nodes */

#ifdef LIVEDEAD


	static void
prld()
{
    register NODE * p;

    for (ALLN(p))			/* for all instruction nodes... */
    {
	PUTCHAR(CC);			/* write comment char */

	PRINTF("(live: 0x%7.7x)", p->nlive); /* print live/dead data */
	prinst(p);			/* print instruction */
    }
    return;
}

#endif
/*
 * This section of code, pays attention to register loads,
 * and tries to do redundant load removal.  This is done,
 * by watching mov instructions, and keeping track of what
 * values are stored in the actual register.  If the mov
 * is reloading a register with the same contents, then the
 * mov is deleted.
 */
#define NUMVALS 20

static struct	rld {
	char	*rname;			/* register name */
	int	rnum;			/* register number    */
	int	rsrc;			/* size of the mov source   */
	int	rdst;			/* size of the mov dest reg */
	int	mvtype;			/* type of move instruction */
	char	*ropnd;			/* Actual operand */
} rreg[NUMVALS];

static NODE *ccodes;

static int numcache;	/* number of real values currently in cache */

#define FORALL(rp,n) \
	for (rp = rreg, n = numcache; n > 0; ++rp) \
		if (rp->ropnd != NULL ? n-- : 0)


	static void
insertcache(rname, srcsize, dstsize, movetype, ropnd)
char *rname, *ropnd;
int srcsize, dstsize, movetype;
{
	register struct rld *rp;
	int n;

	if (!strcmp(rname, ropnd)) return;

	FORALL(rp,n)
		if (!strcmp(rp->rname, rname) && !strcmp(rp->ropnd, ropnd)
		&&  rp->rsrc == srcsize && rp->rdst == dstsize
		&&  rp->mvtype == movetype)
			return;
	for (rp = rreg, n = NUMVALS; n > 0; ++rp, --n)
		if (rp->ropnd == NULL) {
			rp->rname = rname;
			rp->rnum = scanreg(rname, false);
			rp->rsrc = srcsize;
			rp->rdst = dstsize;
			rp->mvtype = movetype;
			rp->ropnd = ropnd;
			++numcache;
			return;
		}
}


	static void
rdfree(arg)
struct rld * arg;
{
	register struct rld *rp;

	if (arg) {
		arg->ropnd = NULL;
		--numcache;
	}
	else {
		FORALL(rp,numcache)
			rp->ropnd = NULL;
		ccodes = NULL;
	}
}


	static void
rdfreemem(dop, dsize)
char *dop;
int dsize;
{
	register int offs1, offs2;
	int n;
	register struct rld *rp;
	boolean onstack = isiros(dop), aliased = !lookup_regals(dop,
								first_regals);

	FORALL(rp,n)
		if (!isreg(rp->ropnd) && *rp->ropnd != '$')
#if 0
			if (dop == NULL)  /* for CALL: needs improvement */
				rdfree(rp);
			else
#endif
			if (onstack) {
				if (aliased)
					if (!isiros(rp->ropnd) ||
					    !mystrstr(rp->ropnd, "(%ebp)") ||
					    !mystrstr(dop, "(%ebp)"))
						rdfree(rp);
					else {
						offs1 = atoi(dop);
						offs2 = atoi(rp->ropnd);
						if (offs1 < offs2) {
							if (dsize == -1 ||
							    offs1 + dsize >=
							    offs2)
							rdfree(rp);
						} else if (offs2 + rp->rdst >=
							   offs1)
							rdfree(rp);
					}
			}
			else if (isiros(rp->ropnd)) {
				if (!lookup_regals(rp->ropnd, first_regals))
					rdfree(rp);
			}
			else if (isdigit(*rp->ropnd) || *rp->ropnd == '-' ||
				 *rp->ropnd == '(')
				rdfree(rp);
			else if (dop == NULL || isdigit(*dop) || *dop == '-' ||
				*dop == '(')
				rdfree(rp);
			else {
				char *p1, *p2, t1, t2;

				p1 = rp->ropnd; p2 = dop;
				while (*p1 && (isalnum(*p1) || *p1 == '_' ||
						*p1 == '.'))
					++p1;
				while (*p2 && (isalnum(*p2) || *p2 == '_' ||
						*p2 == '.'))
					++p2;
				t1 = *p1, t2 = *p2;
				*p1 = '\0'; *p2 = '\0';
				if (!strcmp(rp->ropnd, dop))
					rdfree(rp);
				*p1 = t1; *p2 = t2;
			}
}


static char *
other_set_inst(n, srcsize, dstsize)
NODE *n;
int  *srcsize, *dstsize;
{
	if ((n->op == XORB || n->op == XORW || n->op == XORL)
	 && isreg(n->op1)
	 && isreg(n->op2)
	 && !strcmp(n->op1, n->op2)
	) {
		if (n->op == XORB)
			*srcsize = 1, *dstsize = 1;
		else if (n->op == XORW)
			*srcsize = 2, *dstsize = 2;
		else
			*srcsize = 4, *dstsize = 4;
		return "$0";
	}
	else if (n->op == LEAL) {
		char *tmp = (char *)malloc(strlen(n->op1)+2);
		*tmp = '&';
		strcpy(tmp+1, n->op1);
		*srcsize = *dstsize = 4;
		return tmp;
	}

	return NULL;
}


static void
replace_operands(p)
register NODE *p;
{
	register struct rld *rp;
	int n;
	boolean nodice = false;

	switch (p->op) {

		case CMPB:  case CMPW:  case CMPL:
		case TESTB: case TESTW: case TESTL:

		{
		register char *r1, *r2, *c1, *c2;
		boolean reg1, reg2;

		c1 = isnumlit(p->op1) ? p->op1 : NULL;
		r1 = isreg(p->op1) ? p->op1 : NULL;
		c2 = isnumlit(p->op2) ? p->op2 : NULL;
		r2 = isreg(p->op2) ? p->op2 : NULL;

		if (!isvolatile(p,1))
		   FORALL(rp,n)
			if (rp->rsrc == rp->rdst
			&&  OpLength(p) == rp->rsrc) {
				if (!strcmp(p->op1, rp->ropnd)) {
					if (!c1 && isnumlit(rp->rname))
						c1 = rp->rname;
					else if (!r1)
						r1 = rp->rname;
				}
				else if (!strcmp(p->op1, rp->rname) && !c1 
						&& isnumlit(rp->ropnd))
					c1 = rp->ropnd;
				if (c1 && r1)
					break;
			}
		if (!isvolatile(p,2))
		   FORALL(rp,n)
			if (rp->rsrc == rp->rdst
			&&  OpLength(p) == rp->rsrc) {
				if (!strcmp(p->op2, rp->ropnd)) {
					if (!c2 && isnumlit(rp->rname))
						c2 = rp->rname;
					else if (!r2 && !isconst(rp->rname))
						r2 = rp->rname;
				}
				else if (!strcmp(p->op2, rp->rname) && !c2 
						&& isnumlit(rp->ropnd))
					c2 = rp->ropnd;
				if (c2 && r2)
					break;
			}

		reg1 = isreg(r1), reg2 = isreg(r2);
		if (reg1 && reg2)
			p->op1 = r1, p->op2 = r2;
		else if (c1 && reg2)
			p->op1 = c1, p->op2 = r2;
		else if (reg1 || reg2) {
			if (r1)
				p->op1 = r1;
			if (r2)
				p->op2 = r2;
		}
		}

		break;

					
		case IMULB: case IMULW: case IMULL:
		if (!p->op2) nodice = true;

		if (p->op3) {
			if (!isreg(p->op2))
				FORALL(rp,n)
					if (!strcmp(p->op2, rp->ropnd)
					 && !isvolatile(p,2)
					 && rp->rsrc == rp->rdst
					 && OpLength(p) == rp->rsrc
					 && isreg(rp->rname))
						{ /* found an operand to replace */
						p->op2 = /*strdup*/(rp->rname);
						break;
						}
			break;
		}
		goto cont;


		case MOVSBW: case MOVSBL: case MOVSWL:
		case MOVZBW: case MOVZBL: case MOVZWL:
		case MULB: case MULW: case MULL:
		case DIVB: case DIVW: case DIVL:
		case IDIVB: case IDIVW: case IDIVL:
			nodice = true;
			/* FALLTHROUGH */

		case ORB:  case ORW:  case ORL:
		case ADCB: case ADCW: case ADCL:
		case ADDB: case ADDW: case ADDL:
		case ANDB: case ANDW: case ANDL:
		case MOVB: case MOVW: case MOVL:
		case SBBB: case SBBW: case SBBL:
		case SUBB: case SUBW: case SUBL:
		case XORB: case XORW: case XORL:
		case PUSHW: case PUSHL:

cont:		
		{
			char *cst = NULL, *reg = NULL;

			FORALL(rp,n)
				if (!strcmp(p->op1, rp->ropnd)
				 && !isvolatile(p,1)
				 && rp->rsrc == rp->rdst
				 && OpLength(p) == rp->rsrc
				 && (!nodice || *rp->rname != '$'))
					{ /* found an operand to replace */
					if (isreg(rp->rname)) {
						if (!reg || !p->op2 ||
						    !strcmp(rp->rname, p->op2))
							reg = rp->rname;
					} else
						cst = rp->rname;
					}
			if (cst && p->op != PUSHW && p->op != PUSHL && !isvolatile(p,1) &&
			    !strcmp(cst, "$0")) {
				p->op1 = cst;
			}
			else if (reg && !isvolatile(p,1))
				p->op1 = reg;
			else if (cst && !isreg(p->op1) && !isvolatile(p,1))
				p->op1 = cst;
			break;
		}
	}
}


void
rmrdld()
{
	register NODE *p;
	int n;
	register struct rld *rp;
	char *dop, *sop;
	unsigned rnum;
	int src1, dst1, movetype;

	if (tflag)
		return;
	rdfree((struct rld*)NULL);
	for (ALLN(p))			/* for all instruction nodes... */
	{
		if (Tflag) {
			PRINTF("%c rmrdld: ", CC);
			prinst(p);	/* print instruction */
		}
		if (isuncbr(p) || islabel(p) || p->op == ASMS || is_safe_asm(p))
			rdfree((struct rld*)NULL);	/* free up the list between breaks */
		else if (p->op == CALL || p->op == LCALL) {
			rnum = EAX | EDX | ECX;
			FORALL(rp,n) {
				if (rnum & rp->rnum
				 || rnum & scanreg(rp->ropnd, false))
					rdfree(rp);
			}
			ccodes = NULL;
			rdfreemem((char *)NULL, -1);
		} else if ((sop = p->op1, ismove(p, &src1, &dst1)) ||
			   (sop = other_set_inst(p, &src1, &dst1))) {
			ccodes = NULL;
			if (!isreg(p->op2)) {
				dop = p->op2;
				FORALL(rp,n) {
					if (!strcmp(p->op1, rp->rname)
					 && !strcmp(dop, rp->ropnd)
					 && rp->rsrc == src1
					 && rp->rdst == dst1
					 && !isvolatile(p,2)
					   ) {
						/* found a redundant load */
						if (Tflag) {
							PRINTF("%c removed",CC);
							prinst(p);
						}
						ldelin2(p);
						DELNODE(p);
						dop = NULL;
						break;
					}
				}
				if (dop == NULL) continue;

				if (!isreg(p->op1)) {
					replace_operands(p);
					new_sets_uses(p);
				}
				rnum = 0;
				FORALL(rp,n)
					if (!strcmp(dop, rp->ropnd)) {
						rnum |= rp->rnum;
						rdfree(rp);
					}
				if (rnum)
				   FORALL(rp,n)
					if (rnum & scanreg(rp->ropnd, false))
						rdfree(rp);
				rdfreemem(dop, dst1);
				/*else*/ /* if (src1 == dst1) */
				insertcache(p->op1, src1, dst1, 'l', dop);
				if (isreg(p->op1))
				   FORALL(rp,n) {
					if (!strcmp(p->op1, rp->rname)
					&&  (isreg(rp->ropnd) ||
					     *rp->ropnd == '$')
					&&  rp->rsrc == src1
					&&  rp->rdst == dst1
					   )
					   insertcache(rp->ropnd, src1, dst1,
						       'l', dop);
					}
				else	/* literal ? */
				   FORALL(rp,n)
					if (!strcmp(p->op1, rp->ropnd)
					&&  rp->rsrc == src1
					&&  rp->rdst == dst1
					   )
					   insertcache(rp->rname, src1, dst1,
						       'l', dop);
			} else {
				movetype = src1 == dst1 ? 'l' : p->opcode[3];
				FORALL(rp,n) {
					if (!strcmp(p->op2, rp->rname)
					 && !strcmp(sop, rp->ropnd)
					 && rp->rsrc == src1
					 && rp->rdst == dst1
					 && rp->mvtype == movetype
					 && !isvolatile(p,1)
					   ) { /* found a redundant load */
						if (Tflag) {
							PRINTF("%c removed",CC);
							prinst(p);
						}
						ldelin2(p);
						DELNODE(p);
						sop = NULL;
						break;
					}
				}
				if (sop == NULL) continue;

				if (!isreg(p->op1)) {
					replace_operands(p);
					new_sets_uses(p);
				}
				/* time to change the move */
				rnum = scanreg(p->op2, false);
				FORALL(rp,n) {
					if (rnum & rp->rnum
					 || rnum & scanreg(rp->ropnd, false))
						rdfree(rp);
				}
				if (!(rnum & scanreg(sop, false))) {
					insertcache(p->op2, src1, dst1,
						    movetype, sop);
					if (isreg(sop)) {
					   FORALL(rp,n)
						if (!strcmp(sop, rp->rname)) {
						  if (rp->rdst == rp->rsrc &&
						      src1 == dst1)
						    insertcache(p->op2,
								rp->rsrc, dst1,
								'l', rp->ropnd);
						  else if (*rp->ropnd != '$')
						     if (rp->rdst == rp->rsrc)
							insertcache(p->op2,
								rp->rsrc, dst1,
								movetype,
								rp->ropnd);
						     else if (src1 == dst1)
							insertcache(p->op2,
								rp->rsrc, dst1,
								rp->mvtype,
								rp->ropnd);
						}
					   if (src1 == dst1)
						  insertcache(sop, dst1, src1,
							'l', p->op2);
					}
					else if (*sop == '$') {
					   FORALL(rp,n)
						if (!strcmp(sop, rp->ropnd)
						&&  rp->rdst == src1)
						   insertcache(p->op2, src1,
							dst1, 'l', rp->rname);
						else if (!strcmp(sop, rp->rname)
						     &&  rp->rdst == src1)
						   insertcache(p->op2, src1,
							dst1, 'l', rp->ropnd);
					}
					else {
					   FORALL(rp,n)
						if (!strcmp(sop, rp->ropnd)
						&&  rp->rsrc == src1
						&&  rp->rdst == dst1
						&&  rp->mvtype == movetype)
						   insertcache(p->op2,
							rp->rdst, dst1,
							movetype, rp->rname);
					}
				}
			}
		} else {
			rnum = p->sets;
			dop = dst(p);		/* dst doesn't return NULL */
			if (isreg(dop) || *dop == '\0')
				dop = NULL;
			replace_operands(p);
			new_sets_uses(p);
			if (rnum)
			   FORALL(rp,n)
				if (rnum & rp->rnum
				 || rnum & scanreg(rp->ropnd, false))
					rdfree(rp);
			if (dop != NULL) {
			   FORALL(rp,n)
				if (!strcmp(dop, rp->ropnd))
					rdfree(rp);
			   rdfreemem(dop, -1);
			}

			if (rnum == CONCODES && !dop)
				if (ccodes)
					if (same(p, ccodes)) {
						ldelin2(p);
						DELNODE(p);
					}
					else
						ccodes = p;
				else
					ccodes = p;
			else if (dop || rnum)
				ccodes = NULL;
		}
	}
}


static unsigned int
tmpsets(p)
NODE *p;
{
	unsigned int regs = p->uses | p->sets, tmpregs = 0;;

	if (regs & EAX)
		tmpregs = EAX;
	if (regs & ECX)
		tmpregs |= ECX;
	if (regs & EDX)
		tmpregs |= EDX;

	return tmpregs;
}


static boolean
gettmpreg(reg, p)
unsigned *reg;
NODE *p;
{
	if (*reg)
		*reg &= ~(p->nlive | tmpsets(p));
	else {
		if (!(p->nlive & EAX) && !(p->uses & EAX) && !(p->sets & EAX))
			*reg |= EAX;
		if (!(p->nlive & ECX) && !(p->uses & ECX) && !(p->sets & ECX))
			*reg |= ECX;
		if (!(p->nlive & EDX) && !(p->uses & EDX) && !(p->sets & EDX))
			*reg |= EDX;
	}

	return *reg != 0;
}

static char
*tmp_reg_size(reg, inst_size)
unsigned *reg;
int inst_size;	/* better be 'b', 'w', or 'l' */
{
	if (*reg & ECX) {
		*reg = ECX;
		if (inst_size == 'b')
			return "%cl";
		else if (inst_size == 'w')
			return "%cx";
		else
			return "%ecx";
	}
	else if (*reg & EDX) {
		*reg = EDX;
		if (inst_size == 'b')
			return "%dl";
		else if (inst_size == 'w')
			return "%dx";
		else
			return "%edx";
	}
	else if (*reg & EAX) {
		*reg = EAX;
		if (inst_size == 'b')
			return "%al";
		else if (inst_size == 'w')
			return "%ax";
		else
			return "%eax";
	}
	/* NOTREACHED */
}


int
replace_consts(run)
boolean run;
{
	register NODE *p, *first = NULL;
	boolean looking = true, isone,found = false;
	char *literal;
	char ch;
#define NONE 0
#define OUT  3
	int strikes = NONE;
	unsigned tmpreg = 0;

	for (ALLN(p))			/* for all instruction nodes... */
	{
		if (isuncbr(p) || islabel(p) || is_safe_asm(p) ||
		    p->op == ASMS || p->op == CALL || p->op == LCALL) {
			looking = true;
			first = NULL;
			strikes = NONE;
			tmpreg = 0;
		}
		else if (!p->op1 || *p->op1 != '$') {
			if (!looking && (++strikes == OUT ||
					 !(tmpreg &= ~tmpsets(p)))) {
				looking = true;
				first = NULL;
				strikes = NONE;
				tmpreg = 0;
			}
		} else {
			switch (p->op) {
			   case ADDL:  case ADDW:  case ADDB:
			   case ANDL:  case ANDW:  case ANDB:
			   case CMPL:  case CMPW:  case CMPB:
			   case MOVL:  case MOVW:  case MOVB:
			   case ORL:   case ORW:   case ORB:
			   case PUSHL: case PUSHW:
			   case SUBL:  case SUBW:  case SUBB:
			   case TESTL: case TESTW: case TESTB:
			   case XORL:  case XORW:  case XORB:
				isone = true;
				break;
			   default:
				isone = false;
				break;
			}
			if (
			   isone
			   && (!p->op2 || strcmp(p->op2, "%esp") || !looking)
			   && gettmpreg(&tmpreg, p)
			  ) {
				if (looking) {
					looking = false;
					first = p;
					literal = p->op1;
				}
				else if (strcmp(p->op1, literal) == 0) {
					/* found a consecutive instruction with the 
					same literal */
					if (first) {
						NODE *prepend(), *pnew;
						char *dest;
						dest = tmp_reg_size(&tmpreg, 'l');
						pnew = prepend(first, dest);
						if (!strcmp(literal, "$0")) {
							chgop(pnew, XORL, "xorl");
							pnew->op1 = pnew->op2 = dest;
							pnew->uses =0;
							pnew->sets = tmpreg | CONCODES;
						} else {
							chgop(pnew, MOVL, "movl");
							pnew->op1 = literal;
							pnew->op2 = dest;
							pnew->uses =0;
							pnew->sets = tmpreg;
						}
					
						first->op1 = tmp_reg_size(&tmpreg,
						first->opcode[strlen(first->opcode)-1]);
						lexchin(first,pnew);
						new_sets_uses(first);
						if (( first->op == MOVL || first->op == MOVB 
													|| first->op == MOVW )
							&& isreg(p->op2) /* movX %reg,%reg. */ 
						) 
						found = true; /* rmrdmv() will try to remove it */
						first = NULL;
					}
					p->op1 = tmp_reg_size(&tmpreg,
						ch = p->opcode[strlen(p->opcode)-1]);
					if (ch == 'b')
						p->uses |= L2B(tmpreg);
					else if (ch == 'w')
						p->uses |= L2W(tmpreg);
					else
						p->uses |= tmpreg;
					if ( (! found)
						&& ( p->op == MOVL || p->op == MOVB || p->op == MOVW )
						&& isreg(p->op2)
						&& !samereg(p->op1,p->op2) /* movX %reg,%reg. */
					   ) 
					found = true; /* rmrdmv() will try to remove it */
				}
				else if (run) {
					literal = p->op1;
					first = p;
					strikes = NONE;
					tmpreg = 0;
					if (!gettmpreg(&tmpreg, p))
						looking = true, first = NULL;
				}
				else if (!looking && ++strikes == OUT) {
					looking = true;
					first = NULL;
					strikes = NONE;
					tmpreg = 0;
				}
			}
		}
	}
	return found;
}
