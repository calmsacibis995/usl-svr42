/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/ebboptim.c	1.8.3.2"

#include "sched.h" /*include optim.h and defs.h */
#include "optutil.h"

/* All following optimizations have to do with values in registers.
** Therefore they operate on generalized basic block. These are
** consecutive basic blocks in which the first one begins with a
** label and the next ones do not. Hence the only entry to them
** is by fall through.
*/

static void remove_register(), remove_index(), remove_base(),
			rmrdmv(), replace_registers();
static int try_forward();
static void try_backward(); 
static boolean zvtr();
int  isbase();
int fflag = 0;
extern int fp_removed;	/*declared in local.c */
extern void bldgr(), ldelin2(), chgop(), regal_ldanal();
extern unsigned indexing(), scanreg(), setreg(), uses(), sets();
extern int samereg(), isindex(), isfp();
extern char *dst();
extern unsigned int hard_uses();
void
ebboptim(opt) int opt;
/* The dirver of the optimizations. Determines the first and last
** nodes of the extended basic block and calls the specific function
** according to it's parameter.
*/
{
 BLOCK *b , *firstb;
 int found = 0;
#ifdef BBOPTIM
	bldgr(false,false);
#else
	bldgr(false);
#endif

	for (b = firstb = b0.next ; b ; b = b->next) {
		firstb = b;
		while (!( islabel(b->lastn->forw) || isuncbr(b->lastn) 
				  || b->lastn->forw == &ntail)) 
			b = b->next;
		switch (opt) {
			case ZERO_PROP:
				found |= zvtr(firstb->firstn,b->lastn); /*zero value trace*/
				break;
			case COPY_PROP:
				rmrdmv(firstb,b->lastn); /*remove redundant mov reg,reg*/
				break;
			default:
				fatal("unknown optimization for ebboptim\n");
		}
	}/*for loop*/
	if (found)
		ldanal();

}/*end ebboptim*/

/* This module keeps track of zero values in registers. If there
** is a zero value in a register and it is used as an index, then
** change the operand so that the register will not be used.
** In addition, some optimizations are possible. If the constant
** 0 is used, replace it by a register holding zero.
** If zero value is moved from one register to another, which already
** contains zero, delete the node.
*/
static unsigned int regmasx[NREGS-2] = {EAX,EDX,EBX,ECX,ESI,EDI,EBI};
extern BLOCK b0;
extern int suppress_enter_leave;
static char *registers[] =
	{ "%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi" , "%ebi" };
#define Mark_z_val(reg) zero_flags |= reg
#define Mark_non_z(reg) zero_flags &= ~(reg)
#define Have_z_val(reg) (zero_flags && (zero_flags | (reg)) == zero_flags)
#define Has_z_val(reg)	zero_flags & (reg)
#define Non_z_val(reg)	(Has_z_val(reg) == 0)
#define Non_z_have(reg) (Have_z_val(reg) == 0)

static int nregs = NREGS-2; /* Don't count ESP and CC */

/* full_reg() will return the 32 bit register that contains the 
input parameter */
static unsigned int full_reg(i) unsigned int i;
{
  switch (i) {
	case EAX: return EAX; 
	case EDX: return EDX; 
	case EBX: return EBX;
	case ECX: return ECX; 
	case ESI: return ESI; 
	case EDI: return EDI;
	case EBP: return EBP;
	case AX:  return EAX;
	case DX:  return EDX;
	case BX:  return EBX;
	case CX:  return ECX;
	case SI:  return ESI; 
	case DI:  return EDI; 
	case AH:  return EAX; 
	case DH:  return EDX; 
	case BH:  return EBX; 
	case CH:  return ECX; 
	case AL:  return EAX; 
	case DL:  return EDX; 
	case BL:  return EBX; 
	case CL:  return ECX; 
	case EBI: return EBI; 
	case BI:  return EBI; 
  /* NOTREACHED */
  }/*end switch*/
} /*end full_reg*/

/* itoreg() converts register bits to register string */
char *
itoreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "%eax";
	case EDX:  return "%edx";
	case EBX:  return "%ebx";
	case ECX:  return "%ecx";
	case ESI:  return "%esi";
	case EDI:  return "%edi";
	case EBP:  return "%ebp";
	case AX:  return "%ax";
	case DX:  return "%dx";
	case BX:  return "%bx";
	case CX:  return "%cx";
	case SI:  return "%si";
	case DI:  return "%di";
	case AH:  return "%ah";
	case DH:  return "%dh";
	case BH:  return "%bh";
	case CH:  return "%ch";
	case AL:  return "%al";
	case DL:  return "%dl";
	case BL:  return "%bl";
	case CL:  return "%cl";
	case EBI: return "%ebi";
	case BI:  return "%bi";
  /* NOTREACHED */
  }/*end switch*/
}/*end itoreg*/

char *
itohalfreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "%ax";
	case EDX:  return "%dx";
	case EBX:  return "%bx";
	case ECX:  return "%cx";
	case ESI:  return "%si";
	case EDI:  return "%di";
	case EBP:  return "%bp";
	case AX:  return "%ax";
	case DX:  return "%dx";
	case BX:  return "%bx";
	case CX:  return "%cx";
	case SI:  return "%si";
	case DI:  return "%di";
	case AH:  return "%ah";
	case DH:  return "%dh";
	case BH:  return "%bh";
	case CH:  return "%ch";
	case AL:  return "%al";
	case DL:  return "%dl";
	case BL:  return "%bl";
	case CL:  return "%cl";
	case EBI: return "%bi";
	case BI:  return "%bi";
  /* NOTREACHED */
  }/*end switch*/
}/*end itoreg*/


char *
itoqreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "%al";
	case EDX:  return "%dl";
	case EBX:  return "%bl";
	case ECX:  return "%cl";
	case AX:  return "%al";
	case DX:  return "%dl";
	case BX:  return "%bl";
	case CX:  return "%cl";
	case AH:  return "%ah";
	case DH:  return "%dh";
	case BH:  return "%bh";
	case CH:  return "%ch";
	case AL:  return "%al";
	case DL:  return "%dl";
	case BL:  return "%bl";
	case CL:  return "%cl";
	default:  return NULL;
  /* NOTREACHED */
  }/*end switch*/
}/*end itoqreg */

static unsigned int zero_flags = 0;

/*Does instruction set a register to zero.
**true only for long instructions, that set the whole register.
*/
static boolean
set_reg_to_0(p) NODE *p;
{
	if (!isreg(p->op2))
		return false;
	switch (p->op) {
		case ROLL: case RORL: case SALL:
		case ROLW: case RORW: case SALW:
		case ROLB: case RORB: case SALB:
		case SARL: case SHLL: case SHRL:
		case SARW: case SHLW: case SHRW:
		case SARB: case SHLB: case SHRB:
			if (Have_z_val(p->sets & ~CONCODES))
				return true;
			else
				return false;
		case XORL: case SUBL:
			if (samereg(p->op1,p->op2))
				return true;
			/* FALLTHRUOGH */
		case XORW: case SUBW:
		case XORB: case SUBB:
			if (Have_z_val(p->sets &~CONCODES)
			 && isreg(p->op1) && Have_z_val(p->uses))
				return true;
			if (!strcmp(p->op1,"$0")
			 && Have_z_val(p->sets & ~CONCODES))
				return true;
			return false;
		case IMULB: case IMULW: case IMULL:
			if (p->op3) {
				if (!strcmp(p->op1,"$0"))
					return true;
				if (Have_z_val(p->uses))
					return true;
				return false;
			}
			/*FALLTHROUGH*/
		case ANDL: case MULL:
			if (isreg(p->op1) && Have_z_val(p->uses))
				return true;
			if (!strcmp(p->op1,"$0"))
				return true;
			/* FALLTHROUGH */
		case ANDW: case MULW:
		case ANDB: case MULB:
			if (Have_z_val(p->sets & ~CONCODES))
				return true;
			return false;
		case ADDL: case ORL:
			if (((isreg(p->op1) && Have_z_val(p->uses))
			   || !strcmp(p->op1,"$0"))
			 && Have_z_val(p->sets))
				return true;
			else
				return false;
		case MOVB: case MOVW:
			if (! Have_z_val(p->sets))
				return false;
			/* FALLTHROUGH */
		case MOVL:
		case MOVZWL: case MOVZBL:
		case MOVSBL: case MOVSWL:
		case MOVZBW: case MOVSBW:
			if (!strcmp(p->op1,"$0"))
				return true;
			if (isreg(p->op1) && Have_z_val(p->uses))
				return true;
			return false;
		case LEAL:
			if (Have_z_val(p->uses) && !hasdisplacement(p->op1))
				return true;
			else 
				return false;
		default:
			return false;
	}/*end switch*/
/*NOTREACHED*/
}/*end set_reg_to_0*/


/*main function of the zero tracing module*/

static boolean
zvtr(firsti,lasti) NODE *firsti , *lasti;
{
unsigned int pidx;
NODE *p, *nextp = NULL; /*init to prevent lint */
int m,i,retval = false;
boolean enter;

  nregs = suppress_enter_leave ? (NREGS -3) : (NREGS -2);
  zero_flags = 0; /*no reg is known to hold zero */
  for (p = firsti ; p != lasti->forw; p = nextp) {
	if (p->op == ASMS || is_safe_asm(p)) /*asm ends the ebb, skip the rest*/
		return retval;
	nextp = p->forw;
	/*If immediate 0 is used in an instruction, we might be
	**able to replace it by a register known to hold zero.
	**do not do it for cmp, because if it is redundant, w2opt()
	**will delete it.
	*/
	if ((p->op == MOVL || p->op == PUSHL)
	  && strcmp(p->op1,"$0") == 0)
	  for(i = 0; i < nregs; i++)
		if (Have_z_val(regmasx[i])) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"make $0 a register: ");
				fprinst(p);
			}
#endif
		  p->op1 = registers[i];
		  p->uses |= regmasx[i];
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"became: ");
				fprinst(p);
			}
#endif
		  p->zero_op1 = 1;
		  retval = true;
		  break;
		}
	/*Remove useless arithmetic instructions, when the operand
	**is known to hold zero. Just change register to immediate
	**zero, and w1opt() will take care of the rest.
	*/
	if (isreg(p->op2))
		switch (p->op) {
			case LEAL: case LEAW:
				if (*p->op1 != '(' || !Have_z_val(p->uses))
					break;
				if (!Have_z_val(p->sets & ~ CONCODES)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"make lea a xor: ");
						fprinst(p);
					}
#endif
					chgop(p,XORL,"xorl");
					p->op1 = p->op2;
					p->uses = 0;
					retval = true;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"became: ");
						fprinst(p);
					}
#endif
				}
				/*FALLTHROUGH*/
			case ROLL: case RORL: case SALL:
			case ROLW: case RORW: case SALW:
			case ROLB: case RORB: case SALB:
			case SARL: case SHLL: case SHRL:
			case SARW: case SHLW: case SHRW:
			case SARB: case SHLB: case SHRB:
				if (Have_z_val(p->sets & ~CONCODES)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delete: ");
						fprinst(p);
					}
#endif
					ldelin2(p);
					DELNODE(p);
					retval = true;
				}
				break;
/*
			case IMULL: case IMULW: case IMULB:
			case MULL: case MULW: case MULB:
			Amigo finds all the cases of mull by 0  
*/
			case ANDB: case ANDW: case ANDL:
			if (p->nlive & CONCODES)
				break;
			if ((isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
			 || !strcmp(p->op1,"$0"))
				if (Have_z_val(p->sets & ~CONCODES)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delnode");
						fprinst(p);
					}
#endif
					DELNODE(p);
					retval = true;
				}
				break;
			case SUBB: case SUBW: case SUBL:
			case ADDB: case ADDW: case ADDL:
			case ORB: case ORW: case ORL:
				if (p->nlive & CONCODES)
					break;
				if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false))) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delete arithmetic ");
						fprinst(p);
					}
#endif
					DELNODE(p);
					retval = true;
				} else if ((p->op != SUBB && p->op != SUBW && p->op != SUBL)
						 && Have_z_val(p->sets & ~ CONCODES)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"change arithmetic to mov");
						fprinst(p);
					}
#endif
					switch (OpLength(p)) {
						case ByTE: chgop(p,MOVB,"movb"); break;
						case WoRD: chgop(p,MOVW,"movw"); break;
						case LoNG: chgop(p,MOVL,"movl"); break;
					}
					p->sets &= ~CONCODES;
					p->uses &= ~p->sets;
					retval = true;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"became");
						fprinst(p);
					}
#endif
				}
				break;
		}/*end switch*/
	/*If there is an operand with a register as base or index, and
	**the register is known to hold zero value, then remove
	**the register from the operand.
	*/
	if (! (pidx = scanreg(p->op1,true))) 
		pidx = scanreg(p->op2,true);
	if (pidx)
		for(i = 0; i < nregs; i++)
			if ((pidx  & regmasx[i]) && Have_z_val(regmasx[i])) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"change from ");
					fprinst(p);
				}
#endif
				remove_register(p,regmasx[i],pidx);
				p->uses = uses(p);
				pidx &=  ~regmasx[i];
				retval = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"to ");
					fprinst(p);
				}
#endif
			}


	/* if this inst is a cmp and the next one is a jump then try to */
	/* eliminate them.                                              */
	enter = true;
	if ((p->op == CMPL || p->op == CMPW || p->op == CMPB)
	&&  p->forw->op >= JA && p->forw->op <=JZ /*followed by a conditional jump*/
	&&  !(p->forw->nlive & CONCODES)	/* condition codes are not live */
	) { int n1,n2;
	   if (isnumlit(p->op1))
		n2 = atoi(p->op1+1);
	   else if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
		n2 = 0;
	   else enter = false;
	   if (isnumlit(p->op2+1))
		n1 = atoi(p->op2);
	   else if (isreg(p->op2) && Have_z_val(scanreg(p->op2,false)))
		n1 = 0;
	   else enter = false;
	   if (enter) {
		boolean willjump = true;

		DELNODE(p);
		switch (p->forw->op) {
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
			default: fatal("MARC: jump is %d\n", p->forw->op);
		}
		if (willjump)
			chgop(p->forw,JMP,"jmp");
		else
			DELNODE(p->forw);
			
		retval = true;
	   }
	}

	/*same as above for test - jcc */

	enter = true;
	if ((p->op == TESTL || p->op == TESTW || p->op == TESTB)
	&& p->forw->op >= JA && p->forw->op <= JZ /*followed by a conditional jump*/
	&&  !(p->forw->nlive & CONCODES)	/* condition codes are not live */
	) { int n1,n2;
	   if (isnumlit(p->op1))
		n2 = atoi(p->op1+1);
	   else if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
		n2 = 0;
	   else 
		enter = false;
	   if (isnumlit(p->op2))
		n1 = atoi(p->op2+1);
	   else if (isreg(p->op2) && Have_z_val(scanreg(p->op2,false)))
		n1 = 0;
	   else
		enter = false;
	   if (enter) {
		boolean willjump = true;
		int res = n1 & n2;

		DELNODE(p);
		switch (p->forw->op) {
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
			default: fatal("MARC: jump is %d\n", p->forw->op);
		}
		if (willjump)
			chgop(p->forw,JMP,"jmp");
		else
			DELNODE(p->forw);
			
		retval = true;
	   }
	}
/* END OF TREATMENT OF CMP, TEST */

	/*Find out that an instruction sets a register to zero
	**and mark it.
	**If possible, change the zero setting to be of the form
	** movl %eax,%ebx , where %eax is known to hold zero.
	**rmrdmv() will prevent this assignment if possible.
	*/
	if (set_reg_to_0(p)) {
		if (!(p->nlive & CONCODES) && (OpLength(p) == LoNG)) {
			for(i = 0; i < nregs; i++)
				if (Have_z_val(regmasx[i])
				   && regmasx[i] != (p->sets & ~CONCODES))
					break;
			if (i < nregs)  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"reduce to mov: ");
					fprinst(p);
				}
#endif
				chgop(p,MOVL,"movl");
				p->op1 = registers[i];
				p->uses = regmasx[i];
				p->sets &= ~CONCODES;
				if (p->op3) {
					p->op2 = p->op3;
					p->op3 = NULL;
				}
				retval = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"became: ");
					fprinst(p);
				}
#endif
			}/*endif i < nregs*/
		} /* endif */
		Mark_z_val(p->sets & ~CONCODES);
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"Mark z reg op2 ");
			fprinst(p);
		}
#endif
	} else {
	/*Find out that an instruction sets a register to non zero
	**and mark it.
	*/
		for(i = 0; i < nregs; i++)
			if (p->sets & regmasx[i]) {
				Mark_non_z(regmasx[i]);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"Mark non z reg op2 ");
				fprinst(p);
			}
#endif
		}
	}
  }/*for all nodes*/
  return retval;
}/*end zvtr*/

/*do string operations to remove a register from the operand. activated
**if the register is used as either a base or index and is known to hold zero.
*/
static void
remove_register(p,reg,pidx) NODE *p; unsigned int reg; unsigned int pidx;
{
 int m;
 int length;
 char *tmpop;
 char *t;
	if (isreg(p->op1) || *p->op1 == '$' )
		m=2;
	else
		m=1;
	if (reg == pidx) {  /*if only one register in the indexing,*/
		t = p->ops[m];    /*then leave only the displacement.    */
		length = 0;
		while(*t != '(') {
			t++;
			length++;
		}
		tmpop = getspace((unsigned)length);
		(void) strncpy(tmpop,p->ops[m],length+1);
		tmpop[length] = (char) 0;
		p->ops[m] = tmpop;
	} else                       /*there are both base and index*/
	 /*this code assumes that the zero holding register appears
	 **only once in the operand.                             */
		if (isbase(reg,p->ops[m]))
			remove_base(&(p->ops[m]));
		else
			remove_index(&(p->ops[m]));
		if (*p->ops[m] == '\0') /* reference a null pointer!*/
			p->ops[m] = "0";
}/*end remove_register*/

int
isbase(reg,op) unsigned int reg; char *op; {
	return (reg & setreg(strchr(op,'(') + 1)); /*base register*/
}/*end isbase*/

static void
remove_index(op) char **op;
{
char *tmpop;
char *t;
int length;
  t = strchr(*op,',');
  length = t - *op +2;
  tmpop = getspace((unsigned)length);
  (void) strncpy(tmpop,*op,length-2);
  if (tmpop[length -3] == '(')
	tmpop[length-3] = '\0';
  else {
	tmpop[length-2] = ')';
	tmpop[length-1] = '\0';
  }
  *op = tmpop;
}/*end remove_index*/

static void
remove_base(op) char **op;
{
int scale;
char *tmpop,*t,*s;
	t = strchr(*op,')');
	t--;
	if (isdigit(*t))
		scale = 1;
	else
		scale = 0;
	tmpop = getspace(strlen(*op) - 4);
	t = tmpop;
	s = *op;
	while (*s != '%')
		*t++ = *s++;
	s+= 5-scale;
	while (*t++ = *s++);
	if (*(t - 2 ) == '(' )
		*(t -2 ) = '\0';	
	*op = tmpop;
}/*end remove_base*/

/* fix_live() perform live/dead analysis over a block.  */
static void
fix_live(first,last,reg_set,reg_reset)  NODE *first, *last; 
unsigned int reg_set,reg_reset; {
unsigned int live;
	if (last->back->forw != last)
		live =  last->nlive | last->uses; /* from try_backward() */
	else
		live =  (last->nlive | last->uses) & ( ~last->sets | last->uses);
	for (last = last->back; first->back != last; last = last->back) {
		last->nlive &=  ~(reg_reset | reg_set); /* clear old live */
		last->nlive |= ( reg_set & live); /* set new live */
		live = (last->nlive | last->uses) & ( ~last->sets | last->uses);
	}
}/*end fix_live*/
/* is_new_value() will return 1 if the dest register is set and not used in 
the instruction p. It will return 2 if we can convert the instruction to less
good instruction that sets the dest register without using it. 
for every other instruction it will return 0 
*/
static int
is_new_value(p,reg) NODE *p; unsigned int reg;
{	char *t;
	switch(p->op) {
		case MOVB:	case MOVW:	case MOVL:
		case MOVZBW: case MOVZBL: case MOVZWL:
		case MOVSBW: case MOVSBL: case MOVSWL:
		case CALL: case LCALL:
				return 1;
		case LEAW: case LEAL:
			if (! isdeadcc(p)) /* can't replace the LEAL */ 
				return 1;
			t = strchr(p->op1,')') -1;
			if (*t == 2 || *t == 4 || *t == 8) /* has scale */
				return 1;
			if  (p->uses == reg)  /* add offset to reg only */
				return 1;
			if (*p->op1 == '(') /* no offset */
				return 1;
			return 2;
		case ADDL:
			if (! isdeadcc(p))
				return 0;
			if ( isreg(p->op1)  /* can be leal [%reg1,%reg2],%reg2 */
			 || *p->op1 == '$')  /* 'addl $val,%reg' => 'leal val[%reg],%reg' */
				return 2;
			return 0;
		case SUBL: 
			if (! isdeadcc(p))
				return 0;
			if (*p->op1 == '$' && isdigit(p->op1[1]))
				return 2;  /* 'subl $val,%reg' => 'leal -val[%reg],%reg' */
			return 0;
		case DECL: case INCL:
			return isdeadcc(p) ?  2 : 0;
		case SHLL:
			if (*p->op1 == '$' && p->op1[2] == 0 && 
				(p->op1[1] == '1' || p->op1[1] == '2' || p->op1[1] == '3'))
			return isdeadcc(p) ?  2 : 0;
				  
		default:
			return 0;
	}/*end switch*/
	/*NOTREACHED*/
}/*end is_new_value*/

/* new_leal() will convert the instruction to an leal instruction. It will 
be used to convert instruction that uses and sets the same register to an 
instruction that uses some registers and sets other register. 
*/ 
static void
new_leal(p) NODE *p; {
	char *tmp;
	switch(p->op) {
		case ADDL:
			if ( isreg(p->op1))  { /* can be leal (%reg1,%reg2),%reg2 */
				tmp = getspace(12);
				sprintf(tmp,"(%s,%s)",p->op1,p->op2);
				p->op1 = tmp;
			} else  { 
				tmp = getspace(strlen(p->op1) + 8);
				sprintf(tmp,"%s(%s)",&p->op1[1],p->op2);
				p->op1 = tmp;
			}
			break;
		case SUBL: 
			tmp = getspace(strlen(p->op1) + 8);
			if (p->op1[1] == '-')
				sprintf(tmp,"%s(%s)",&p->op1[2],p->op2);
			else 
				sprintf(tmp,"-%s(%s)",&p->op1[1],p->op2);
			p->op1 = tmp;
			break;
		case INCL:
			p->op2 = p->op1;
			p->op1 = getspace(8);
			sprintf(p->op1,"1(%s)",p->op2);
			break;				
		case DECL: 
			p->op2 = p->op1;
			p->op1 = getspace(9);
			sprintf(p->op1,"-1(%s)",p->op2);
			break;				
		case SHLL:
			tmp = getspace(10);
			sprintf(tmp,"(,%s,%d)",p->op2, 1 << (p->op1[1] - '0'));
			p->op1 = tmp;
			break; 
	}
	chgop(p, LEAL, "leal");
}

#define is8bit(cp)  (cp && *cp == '%' && (cp[2] == 'l'|| cp[2] == 'h'))

/* This module eliminates redundant moves from register to
**  register. If there is a move from register r1 to register r2
**  and then a series of uses of r2 then remove the mov instruction
**  and change all uses of r2 to use r1.
**  Some conditions may disable this optimization. They are looked for
**  and explained in comments.
*/
static void
try_backward(cb,movinst,firsti,lasti) BLOCK *cb; NODE *firsti, *lasti, *movinst;
{
  NODE *q,*new_set_src = NULL, *firstset = NULL, *may_new_src = NULL;
  NODE *jtarget;
  BLOCK *cb1;
  char *tmp;
  int not8adr,new_set;
  unsigned int srcreg,dstreg,use_target;
  boolean   give_up = 0;
  int movsize = OpLength(movinst);
	if (movinst == firsti) /*  Can't go backward from the first instruction */
		return;
#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"%c move inst ",CC);
		fprinst(movinst);
	}
#endif
	srcreg = movinst->uses;
	dstreg = movinst->sets;
	not8adr = (dstreg & (ESI|EDI|EBI)) && (srcreg & (EAX|EBX|ECX|EDX));
	for (cb1 = cb, q = movinst->back; q != firsti->back; q = q->back) {
		if (q->op == ASMS || is_safe_asm(q))
			return;
		if (q == cb1->firstn->back)
			cb1 = cb1->prev;
		if ((q->uses & srcreg) && /*dont mess with implicit uses of registers*/
			((isshift(q) && (srcreg & CL) && (isreg(q->op1)))
			|| (hard_uses(q) & srcreg)
			|| (! ((scanreg(q->op1,false) & srcreg) 
			|| (scanreg(q->op2,false) & srcreg))))) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c funny uses, give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
		if ((q->sets & srcreg) /*dont mess with implicit set of registers*/ 
		&& ( setreg(dst(q)) != (q->sets & ~CONCODES)
		) 
		) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"implicit set. give up.");
				fprinst(q);
			}
#endif
			break;
		}
		if (OpLength(q) == ByTE
		  && not8adr  /* Can't replace %esi with %eax if %al is used */
		  && (  (is8bit(q->op1) && (setreg(q->op1) & srcreg)) 
			  || (is8bit(q->op2) && (setreg(q->op2) & srcreg)) 
			 ) 
		) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"esi edi vs byte. give up.");
				fprinst(q);
			}
#endif
			break;
		}

		if (isbr(q)) {
			if (cb1->nextr) {
				jtarget = cb1->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
				use_target = jtarget->uses | jtarget->nlive;
				if ((jtarget->sets & srcreg) && ! (jtarget->uses & srcreg))
					use_target &= ~srcreg;
			} else {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"Jump to undef location or end of block: ");
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			}
			if	(!isuncbr(q) /*otherwise jtarget is undefined*/
				&& ( ( q->nlive & srcreg && use_target & srcreg)
				|| (q->nlive & dstreg && use_target & dstreg))
				) {
				give_up = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"src live at branch, give up.");
					fprinst(q);
				}
#endif
				break;
			}
		}
		if ((q->sets | q->uses | q->nlive) & dstreg) {
		   /*The old dst reg is set or used. */
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c dst is used or set. give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
/* If we can separate between the set and the use of the src we may do it.
   We can use the src register and sets the destination register
   for example "inc %src" can be "leal 1(%src),%dest" 
   If by doing it we will get less good instruction we will do it only
   if we can't do the mov reg,reg any other way */
		if(( q->uses & q->sets & srcreg) &&  
		   ((new_set = is_new_value(q,srcreg)) != 0)){
				if ( (! may_new_src) && new_set == 2 && movsize == LoNG) {
					may_new_src = q;
			} else if (new_set == 1 && movsize == LoNG) {
				firstset = new_set_src = q;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"separate ");
					fprinst(q);
				}
#endif
				break;
			}
		}
/* Chack if we used full register and moved less then full register */
		if (q->uses & srcreg) {
			if ((movsize < LoNG) && 
				((((int) OpLength(q)) > movsize) || (indexing(q) & srcreg))
				 )  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c big use of src. give up.\n",CC);
					fprinst(q);
					}
#endif
				give_up = true;
				break;
			}
		} else if (q->sets & srcreg) {
			if ((q->sets & srcreg) != srcreg) { /* set less than reg */
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c set only part of src. \n",CC);
					fprinst(q);
				}
#endif
				continue;
			}
			if (((int) OpLength(q)) > movsize) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c set more then src. \n",CC);
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			} else {
				firstset = q; /* A good targeting set of register */
				break; /* no further questions */
			}
		}
	}/*end inner loop*/
	if (give_up) {
		if (may_new_src) { /* Go back to separate.  */
			firstset = new_set_src = may_new_src;
		}
		else {
#ifdef DEBUG
			if (fflag) {
					fprintf(stderr,"check point 1, have to give up.\n");
			}
#endif
			return;
		}
	} else 
		may_new_src = NULL;
	if ( ! firstset || (q == firsti->back)) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, no set in block.\n");
		}
#endif
		return;
	}
	if ( firstset->nlive & dstreg) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, srcreg is set and dstreg is needed.\n");
		}
#endif
		return;
	}
	if ( srcreg & movinst->nlive){ /* Replace forward and backward */
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c try change forward",CC);
				fprinst(q);
			}
#endif
		tmp = movinst->op1;
		movinst->op1 = movinst->op2;
		movinst->op2 = tmp;
		movinst->uses = dstreg;
		movinst->sets = srcreg;
		if (! try_forward(cb,movinst,lasti)) {
			movinst->uses = srcreg;
			movinst->sets = dstreg;
			movinst->op2 = movinst->op1;
			movinst->op1 = tmp;
			return;
		}
	} else
		tmp = NULL;
	if (may_new_src && may_new_src->op != LEAL)
		new_leal(firstset);
	for (q = movinst->back; q != firstset->back; q = q->back) {
		if (q->uses & srcreg || q->sets & srcreg) { 
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c finally change ",CC);
				fprinst(q);
			}
#endif
			if (q == new_set_src)
				replace_registers(q,srcreg,dstreg,2);
			else
				replace_registers(q,srcreg,dstreg,3);
			new_sets_uses(q);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c to ",CC);
				fprinst(q);
			}
#endif
		}/*endif q->uses & srcreg*/
	}/*for loop*/
	if (! tmp) {
		ldelin2(movinst);
		DELNODE(movinst);
	} 
	fix_live(firstset,movinst,dstreg,srcreg);
	return;
}/*end try_backward*/

/*Test the conditions under which it is ok to remove the copy instruction.
**This is either if there is no instruction that make it illegal, and then
**the give_up flag is set, or if an alternative copy is met, and then the
**testing is stopped.
*/
static int
try_forward(cb,movinst,lasti)  BLOCK *cb; NODE *lasti, *movinst;
{
  NODE *q,*new_set_dst = NULL, *lastuse = NULL, *may_new_dest = NULL;
  NODE *jtarget;
  NODE *srcset = NULL;
  BLOCK *cb1;
  int not8adr,new_set;
  unsigned int srcreg,dstreg,use_target;
  boolean dst_is_changed = 0, srcset_now, give_up = 0;
  int movsize = OpLength(movinst);

#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"%c move inst ",CC);
		fprinst(movinst);
	}
#endif
	srcreg = movinst->uses;
	dstreg = movinst->sets;
	not8adr = (srcreg & (ESI|EDI|EBI)) && (dstreg & (EAX|EBX|ECX|EDX));
	/* go from the copy inst. to the end of the ebb, and do the checks */
	for (cb1 = cb, q = movinst->forw; q != lasti->forw; q = q->forw) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check ");
			fprinst(q);
		}
#endif
		if (q->op == ASMS || is_safe_asm(q))  /*disable */
			return 0;
		srcset_now = false; /*init*/
		if (q == cb1->lastn->forw)
			cb1 = cb1->next;
		/*dont mess with implicit uses of registers*/
		/*these uses can not be changed to use the second register*/
		if ((q->uses & dstreg) &&
			((isshift(q) && (dstreg & CL) && (isreg(q->op1)))
			|| (hard_uses(q) & dstreg)
			|| (! ((scanreg(q->op1,false) & dstreg) 
			|| (scanreg(q->op2,false) & dstreg))))) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c funny uses, give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
		/*If there is a usage of the destination register as a byte register*/
		/*and the source register is esi, edi or ebp then can not replace them*/
		if (OpLength(q) == ByTE
		  && not8adr
		  && (  (is8bit(q->op1) && (setreg(q->op1) & dstreg)) 
			  || (is8bit(q->op2) && (setreg(q->op2) & dstreg))) 
		) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"esi edi vs byte. give up.");
				fprinst(q);
			}
#endif
			break;
		}

		/*Check if the copy is needed at the destination of a jump*/
		if (isbr(q)) {
			/*find the destination of the jump, go beyond the label(s)*/
			/*and find what registers are live there. l/d anal needs a*/
			/*little correction here, add use bits.*/
			if (cb1->nextr) {
				jtarget = cb1->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
				use_target = jtarget->uses | jtarget->nlive;
				/*In the following case, the register is marked, but it's*/
				/*previous value is irrelevant.                          */
				if ((jtarget->sets & dstreg) && ! (jtarget->uses & dstreg))
					use_target &= ~dstreg;
			} else { /*nextr == NULL -> jtarget == NULL */
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"Jump to undef location or end of block: ");
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			}
			/*Two cases here not to be able to eliminate the copy: */
			/*1. If the dest register is live at a branch target, or */
			/*2. If the dest reg is changed and the source reg is needed */
			if	(!isuncbr(q) /*otherwise jtarget is undefined*/
			  && ((q->nlive & dstreg && use_target & dstreg)
			  || (dst_is_changed && q->nlive & srcreg
			  && use_target & srcreg))
			) {
				give_up = true;
#ifdef DEBUG
				if (fflag) {
					if (q->nlive & dstreg && use_target & dstreg)
						fprintf(stderr,"dstreg used at target.\n");
					if (dst_is_changed && q->nlive & srcreg
						&& use_target & srcreg)
						fprintf(stderr,"dst changed, src used at target\n");
					fprinst(jtarget);
					fprintf(stderr,"dst live at branch, give up.");
					fprinst(q);
				}
#endif
				break;
			}
		}/*endif isbr*/
/* If we can separate between the set and the use of the src we may do it.
   We can use the src register and sets the destination register
   for example "inc %src" can be "leal 1(%src),%dest" 
   If by doing it we will get less good instruction we will do it only
   if we can't do the mov reg,reg any other way */
		if(( q->uses & q->sets & dstreg) && 
		   ((new_set = is_new_value(q,dstreg)) != 0)){
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"uses & sets dst, new val ");
				fprinst(q);
			}
#endif
			if ( ! srcset) {
				if ( (! may_new_dest) && new_set == 2 && movsize == LoNG) {
					may_new_dest = q;
				} else if (new_set == 1 && movsize == LoNG) {
					lastuse = new_set_dst = q;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"separate, nfq ");
						fprinst(q);
					}
#endif
					break;
				}
			} else {
				give_up = true; /*give up*/
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"sets & uses dst, src is set, give up.");
					fprinst(q);
				}
#endif
				break; /* no further questions */
			}
		}/*endif q sets dest a new value*/
		if (q->sets & srcreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q sets src ");
				fprinst(q);
			}
#endif
		   /*mark the state that src reg is set. */
			if (!srcset)
				srcset_now = true;
			srcset = q;
				
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c set src ",CC);
				fprinst(srcset);
			}
#endif
		}
		if (q->uses & dstreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q uses dst ");
				fprinst(q);
			}
#endif
			if ((movsize < LoNG) && 
				((((int) OpLength(q)) > movsize) || (indexing(q) & dstreg))
				 )  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c big use of dst. give up.\n",CC);
					fprinst(q);
					}
#endif
				 give_up = true;
				 break;
			}
			lastuse = q;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"/ lastuse "); fprinst(lastuse);
			}
#endif
			if (q->sets & dstreg) { 
				if (q->nlive & srcreg) {
					give_up = true; /*give up*/
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"src live and dst set, give up ");
						fprinst(q);
					}
#endif
					break; /* no further questions */
				}
				dst_is_changed = 1;
			}
			if (((!srcset) || srcset_now ) && !(q->nlive & dstreg)
				 && (!(q->uses & srcreg) || !dst_is_changed)  ) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"src not set, dst not live, nfq");
					fprinst(q);
				}
#endif
				break; /* no further questions */
			}
			if (srcset) {
				/* src reg was changed, now dst is used, give up*/
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c uses dstreg and src set, give up.\n",CC);
					fprintf(stderr,"%c uses:",CC); fprinst(q);
				}
#endif
				give_up = true; /*give up*/
				break;
			}
		} else if (q->sets & dstreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q no uses dst, q sets dst");
				fprinst(q);
			}
#endif
			if ((q->sets & dstreg) != dstreg) { /* set less than reg */
				dst_is_changed = 1;
				lastuse = q;
			} else {
				if (!lastuse)
					lastuse = q->back;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"q ! use dst, q set all dst ");
					fprinst(q);
				}
#endif
				break; /* no further questions */
			}
		}
#ifdef DEBUG
		else {
			if (fflag) {
				fprintf(stderr,"q no uses no sets dst ");
				fprinst(q);
			}
		}
#endif
		if (q->uses & srcreg && dst_is_changed) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"src needed after dst is set.\n");
				fprinst(q);
			}
#endif
			break;
		}/*end if*/
	}/*end inner loop*/
	if (give_up) {
		/*try to recover from the give up: */
		if (may_new_dest) {
			lastuse = new_set_dst = may_new_dest;
			if (lastuse->op != LEAL)
				new_leal(lastuse);
		} else {

#ifdef DEBUG
			if (fflag) {
					fprintf(stderr,"check point 2, have to give up.\n");
			}
#endif
			return 0;
		}
	}/*endif giveup*/
	/*giveup was not set, but other conditions may prevent the optimization*/
	if ( ! lastuse) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, no last use last change.\n");
		}
#endif
		return 0;
	}
	if ((q == lasti->forw) && (lasti->nlive & dstreg)) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, dest live in the end.\n");
		}
#endif
		return 0;
	}
#ifdef DEBUG
	else if (fflag) {
		if (q != lasti->forw)
			fprintf(stderr,"iner loop broke before end.\n");
		if (!(lasti->nlive & dstreg))
			fprintf(stderr,"dest reg dont live in the end.\n");
	}
#endif
	if ( dst_is_changed && lastuse->nlive & srcreg) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, dstreg is changed and srcreg is needed.\n");
		}
#endif
		return 0;
	}
	for (q = movinst; q != lastuse->forw; q = q->forw) {
		if (q->uses & dstreg || q->sets & dstreg) { 
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c finally change ",CC);
				fprinst(q);
			}
#endif
			if (q == new_set_dst)
				replace_registers(q,dstreg,srcreg,1);
			else
				replace_registers(q,dstreg,srcreg,3);
			new_sets_uses(q);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c to ",CC);
				fprinst(q);
			}
#endif
		}/*endif q->uses & dstreg*/
	}/*for loop*/
	fix_live(movinst,lastuse,srcreg,dstreg);
	ldelin2(movinst);
	DELNODE(movinst);
	return true;
}/*end try_forward*/

static void
rmrdmv(cb,lasti) BLOCK *cb; NODE *lasti;
{
  NODE *p;
  NODE *firsti = cb->firstn;
#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"new ebb ");
		fprinst(cb->firstn);
		fprintf(stderr,"until ");
		fprinst(lasti);
	}
#endif
	for (p = firsti; p != lasti->forw; p = p->forw) {  /* find the copy inst. */
		if (p->op == ASMS)  /* disable for all the ebb if found an asm */
			return;
		if (p == cb->lastn->forw)  /* update current basic block */
			cb = cb->next;
		if (( p->op == MOVL || p->op == MOVB || p->op == MOVW )
		   && isreg(p->op1)
		   && isreg(p->op2)
		   && !samereg(p->op1,p->op2) /*dont do it for same register*/
		   && ! (p->sets == EBP) /*save time for move esp,ebp, it fails anyway*/
		 ) 
			if ( ! try_forward(cb,p,lasti))  /* copy propagation */
				try_backward(cb,p,firsti,lasti);  /* targeting */
	}/*main for loop*/
}/*end rmrdmv*/

static void
replace_registers(p,old,new,limit) 
NODE *p;
unsigned int old,new;
int limit;
{
  int m;
  int length;
  unsigned int net_dst;
  char *oldname, *newname;
  char *opst,*opst2,*opst3;
  for (m = (limit == 2 ) ? 2: 1 ; m <= limit; m++) {
	  if (p->ops[m] && (net_dst = scanreg(p->ops[m],false) & old)) {
		if (net_dst == old) {
			oldname = itoreg(old);
			newname = itoreg(new);
		}
		else if (net_dst == L2W(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(L2W(new));
		}
		else if (net_dst == L2B(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(L2B(new));
		} 
		else if ( net_dst ==  full_reg(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(full_reg(new));
		}
		opst = getspace(strlen(p->ops[m]));
		opst2 = p->ops[m];
		p->ops[m] = opst;
		length = strlen(newname);
		while(*opst2 != '\0') {
		  if (*opst2 == *oldname
		   && strncmp(opst2,oldname,length) == 0) {
			  for(opst3 = newname; *opst3;)
				  *opst++ = *opst3++;
			  opst2 += length;
			  continue;
		  }
		  *opst++ = *opst2++;
		}
		*opst = '\0';
	  }/*endif*/
	}/*for loop*/
}/*end replace_registers*/

/* This group of functions  are for spilling elimination. A temp is a variable
that for every basic block is set before used. If there is a free register
the temp an be reallocated to this register. In this case rmrdmv() that should
come after this optimization will sometime alienate the temp reg completely.
If is a specific basic block temp is referenced only once (set only) it will
be removed.
*/  
/* next function manage book keeping of ebp - offsets parsed
** as regal comment but not as alias comments by the compiler.
*/
typedef struct auto_reg
{
  int	offset;	/* offset of regal in the stack */
  struct auto_reg *reg_hash_next;	/* next outo reg in hash chain */
  unsigned bits; /* the live/dead register bit */
} AUTO_REG;

AUTO_REG *regals[0x80];
static unsigned reg_bits = 1;


void
init_regals() {
	int i;
	reg_bits = 1;
	for (i=0; i < 0x80; i++)
		regals[i] = NULL;
}/*end init_regals*/

void
save_regal(s) char *s; { /* Get regal from regal.c and 
				initialize it and add to the hash table */
	AUTO_REG *r;
	int entry;

	r = GETSTR(AUTO_REG);
	r->offset = atoi((*s == '*') ? &s[1] : s); /* get offset from %ebp */
	entry = ((- r->offset) >> 2) & 0x7f; /* hash function is (-x/4)%128 */
	r->reg_hash_next = regals[entry]; /* link to the hash table */
	regals[entry] = r; 
	r->bits = 0;
}

int
isregal(x) int x; { /* look for regal in hash table x == offset from %ebp */
  int entry;
  AUTO_REG *r;

  if (x == 0) 
	return false;
  entry = ((-x) >> 2) & 0x7f; /* hash function is (-x/4)%128 */
  for (r= regals[entry]; r != NULL ; r = r->reg_hash_next)
	if (r->offset == x) /* If regal was found */
	  return true;
  return false;
}

static AUTO_REG *
getregal(p) NODE *p; { /* look for regal in hash table, one op
	 has the form "[*]num(%e[bs]p)". If found return the reg structure*/
	int entry;
	AUTO_REG *r;
	int x;
	char *s = NULL;

	if (p->op1 && isindex(p->op1,"%ebp"))
		s = p->op1;
	else if (p->op2 && isindex(p->op2,"%ebp"))
		s = p->op2;
	else return NULL;
	x =  (*s == '*') ? atoi(&s[1]) : atoi(s); 
	entry = ((-x) >> 2) & 0x7f; /* hash function is (-x/4)%128 */
	for (r= regals[entry]; r != NULL ; r = r->reg_hash_next)
		if ((r->offset == x ) && (r->bits != 3)) /* regal found return it */
			return r;
	return NULL;
}



void
set_regal_bits(p) NODE *p; { /* If temp is used in instruction p return it's
		 structure    */  	


	AUTO_REG *reg = NULL;
	unsigned bits;
	if (p->op == ASMS) {
		p->nrlive = (unsigned) ~0;
		p->nrdead = 0;
		return;
	}
	if (!(reg = getregal(p))) {
		p->nrlive = 0;
		p->nrdead = 0;
		return;
	}
	if ( p->op == LEAL || p->op == LEAW || !reg_bits)
	{	reg->bits = 3; /* Mark this register as not valid */
#ifdef DEBUG
		if (fflag) 
			fprintf(stderr,"kill regal offset = %d\n", reg->offset);
#endif
		p->nrlive = 0;
		p->nrdead = 0;
		return;
	}
	if (reg->bits)
		bits= reg->bits;
	else if (reg_bits)
	{	bits = reg->bits = reg_bits;
#ifdef DEBUG
		if (fflag) 
			fprintf(stderr,"regal offset = %d, bit is %8.8x\n",reg->offset,bits);
#endif
		reg_bits <<= 1;
	}
	if ( MEM & muses(p)) {
		p->nrlive = bits;
		p->nrdead = 0;
	}
	else if (MEM & msets(p)) {
		p->nrlive = 0;
		p->nrdead = bits;
	}
	return;	
}
static unsigned used_first;
extern int asmflag;
unsigned use_first() {
 BLOCK *b = b0.next;
 NODE *firsti = n0.forw , *lasti = n0.forw,*p;
 unsigned use,set;
	if (asmflag)
		return used_first = ((unsigned) ~0);
	used_first = 0;
	while (lasti != &ntail) {
		firsti = b->firstn;
		do
			b = b->next;
		while (b && ! islabel(b->firstn));
		lasti = b ? b->firstn : &ntail;
		for (set = use = 0, p = firsti; p != lasti; p = p->forw) {
			if (p->nrlive & ~set) 
				use |= p->nrlive;
			else if (p->nrdead & ~use)
				set |= p->nrdead;
		}
		used_first |= use; 
	}/*while loop*/
	return used_first;
}/*end use_first */


/* use *%reg and not %reg in case ao function call */
static char *
itostarreg(i) unsigned int i; { /* *%reg is used by indirect call */
  switch (i) {
	case EAX:  return "*%eax";
	case EDX:  return "*%edx";
	case EBX:  return "*%ebx";
	case ECX:  return "*%ecx";
	case ESI:  return "*%esi";
	case EDI:  return "*%edi";
	case EBI:  return "*%ebi";
  }/*end switch*/
  /* NOTREACHED */
}/*end itoreg*/


static int
find_free_reg(p,b,reg_bit) NODE *p;BLOCK *b;
						unsigned reg_bit; {	/* find reg that is free from p
		 to last_p instruction block  */

	int first_reg = 0,last_reg = nregs;
	int i;
	AUTO_REG *r;
	unsigned live = EAX | EDX | ECX | EBX | ESI | EDI | EBI;
	static int regs[] = { EAX,EDX,ECX,EBX,ESI,EDI,EBI};
	NODE *jtarget;
	if (suppress_enter_leave) nregs=6 , last_reg=6;
	if (! (p->nrdead & reg_bit)) /* /REGAL  was live before */
		return 0;
	if (OpLength(p) == ByTE)
		last_reg = 4; /* no ESI or EDI */
	if (isuncbr(p) || islabel(p->forw)) /* last instruction should have deleted if not used */
		return 0;
	for(p = p->forw ; p != &ntail ; p = p->forw) {	
		if (p->op == ASMS)
			return 0; 
		live &= ~(p->nlive | p->uses | p->sets);
		if (p->op == CALL || p->op == LCALL)
			first_reg = 3; 
		if ((reg_bit & used_first) && isbr(p)) {
			if (isuncbr(p) || (b->nextr == NULL && (b->nextl == NULL ||
				( !isuncbr(b->lastn))))) /* a return, or an unconditional indexed
										jump, or a switch. */
				return 0;
			if (b->nextr) {
				jtarget = b->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
			}
			if ( r = getregal(jtarget)) {
				if (((r->bits | jtarget->nrlive) & reg_bit) & ~jtarget->nrdead)
					return 0; /* used (and not killed) or live in jump target */
			}
			else if (reg_bit & jtarget->nrlive)
				return 0; /* live in jump target */
		}
		if (p == b->lastn) {
			if (isuncbr(p) || islabel(p->forw)) {
				return 0;
			} 
			b = b->next;
		} 
		r = getregal(p);
		if (r && (r->bits == reg_bit)) {	
			if (isfp(p))
				return 0;
			if (OpLength(p) == ByTE) /* If byte register is needed */
				last_reg = 4; /* no ESI or EDI */
		}
		if (!(reg_bit & p->nrlive))
			break;
	}
	for (i = first_reg ; i < last_reg; i++)
		if ((regs[i] & live) == regs[i]){
#ifdef DEBUG
			if (fflag) { 
				fprintf(stderr,"last checked: ");
				fprinst(p);
			}
#endif
			return regs[i];
		}
	return 0;
}
	  
void
rm_tmpvars()  { /* Link all temp in the
block and then try to eliminate them one by one */
	NODE *p,*q;
	AUTO_REG *reg;
	unsigned int r;
	char *r_str,*r_str1;
	BLOCK *b;
#ifdef BBOPTIM
	bldgr(false,false);
#else
	bldgr(false);
#endif
	b = b0.next;
	regal_ldanal();
	for (p = n0.forw; p != &ntail; p = p->forw) {
		if (p == b->lastn->forw)
			b = b->next;
		reg = getregal(p); 
		if (! reg) /* does p uses regal */
			continue;
		if ((! (reg->bits & p->nrlive)) && /* /REGAL is dead after */ 
			(! (p->sets & p->nlive)) && /*if register was set it was not used */
			(! isfp(p)) &&       /* don't remove FP code */
			(p->op !=CALL) && (p->op != LCALL)) { /* Not /REGAL MEM was set */ 
#ifdef DEBUG
				if (fflag) { 
					fprintf(stderr," deleted reg->bits = %x ",reg->bits); 
					fprintf(stderr," p->nrlive = %8.8x \n",p->nrlive);
					fprinst(p);
				}
#endif
				ldelin2(p); /* temp was set and never used. */
				DELNODE(p); /* Remove it */
				continue;
		}
		if (r = find_free_reg(p,b,reg->bits) ) { /* reg was found */ 
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"found  r = %s \n",itoreg(r));
				fprinst(p);
			}
#endif
			r_str = itoreg(r);
			for(q = p ; ; ) {
				if( reg == getregal(q)) { /* replace temp by reg */
					switch(OpLength(q)) {
						case ByTE:
							r_str1 = itoreg(L2B(r));
							break;
						case WoRD:
							r_str1 = itoreg(L2W(r));
							break;
						default:
							r_str1 = r_str;
					}
					if (isindex(q->op1,"%ebp")) {
						if (q->op == MOVSBW || q->op == MOVSBL || 
										q->op == MOVZBW || q->op == MOVZBL)
							q->op1 = itoreg(L2B(r));
						else if (q->op == MOVSWL || q->op == MOVZWL)
							q->op1 = itoreg(L2W(r));
						else if (q->op == CALL)
							q->op1 = itostarreg(r);
						else 
							q->op1 = r_str1;
					} else
						q->op2 = r_str1;
					new_sets_uses(q);
				}
				if (! (q->nrlive & reg->bits))
					break;
				else 
					q->nrlive &= ~reg->bits;
				q->nlive |= r; /* Mark reg as live */
				q= q->forw;
			}
		}
	}
}

#define MAX_LABEL_LENGTH	100
/* new_offset will get a str for operand. Register name that is used as base 
 or index in str and the register content value in val. It will remove the 
 register from the operand and add the val to the displacement.
*/
static char *
new_offset(str,reg,val)
char **str;
unsigned int reg;
int val;
{
int change = 0;
char name[MAX_LABEL_LENGTH];
int x,scale;
unsigned int base,index;
char *t,*rand,*fptr;
char sign;

	fptr = (**str == '*') ? "*" : ""; /*  function pointer call */
	t = strchr(*str,'(');
	base = setreg(1+t); /*base register*/
	t = strchr(t,',');
	if (t)
		index = setreg(t+1); /*index register*/
	else
		index = 0;
	(void) decompose(*str,&x,name,&scale); /*rest components*/
	if (index == reg) { /* Do index first. If not the index will become base */
		change = val * scale;
		remove_index(str);
	}
	if (base == reg) { /* The register is used as base */
		change += val;
		remove_base(str);
	}
	rand = getspace(strlen(*str) + 12);
	change +=x;
	t = strchr(*str,'(');
	if ( change == 0) { 
		if ( name[0] || t ) { 
			if (t)
				sprintf(rand,"%s%s%s",fptr,name,t);
			else
				sprintf(rand,"%s%s",fptr,name);
		} else 
			rand = "0";
	}
	else if (name[0]) {
		if (change > 0) sign = '+';
		else { 
			sign = '-';
			change = -change;
		}
		if (t)
			sprintf(rand,"%s%s%c%d%s",fptr,name,sign,change,t);
		else
			sprintf(rand,"%s%s%c%d",fptr,name,sign,change);
	} else {
		if (t)
			sprintf(rand,"%s%d%s",fptr,change,t);
		else
			sprintf(rand,"%s%d",fptr,change);
	}
	*str = rand;
}/*end new_offsets*/


/* const_index will find base/index registers with constant value and will 
remove the register and add the value to the displacement */
const_index() {
	NODE *p;
	unsigned int pidx;
	int i,i1,i2,m;
	int const_val[NREGS -2]; /*  */
	static int regmasx[] = { EAX,EDX,ECX,EBX,ESI,EDI,EBI};

	for (p = n0.forw; p != &ntail; p = p->forw) {
		if (isuncbr(p) || islabel(p) || p->op == ASMS || is_safe_asm(p)) {
			for(i = 0; i < (NREGS -2); i++) /* new EBB, reset all values */
				const_val[i] = 0;
			continue;
		}
		if (! (pidx = scanreg(p->op1,true))) { 
			pidx = scanreg(p->op2,true);
			m = 2;
		} else
			m = 1;
		if (pidx) {
			for(i = 0; i < (NREGS -2); i++) {
				if ((pidx  & regmasx[i]) && const_val[i]) { 
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"found reg %d with %d in: ",i,const_val[i]);
						fprintf(stderr," was p->ops[%d] = %s \n",m,p->ops[m]);
					}
#endif
					new_offset(&p->ops[m],regmasx[i],const_val[i]);
					new_sets_uses(p);
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr," now p->ops[%d] = %s \n",m,p->ops[m]);
						fprinst(p);
					}
#endif
				}
			}
		}
		if (p->op == MOVL && isconst(p->op1)
			&& ( p->op1[1] == '-' || isdigit(p->op1[1])) 
			&& p->op2[0] == '%' ) { /* found movl $val,%reg */
			for ( i = 0 ; i < (NREGS -2) ; i++)
				if (p->sets == regmasx[i])
					const_val[i] = atoi(&p->op1[1]); /* save val */
		} else if (p->op == LEAL  && (! strchr(p->op1,'('))
			&& ( p->op1[0] == '-' || isdigit(p->op1[0]))  ) {
			for ( i = 0 ; i < (NREGS -2) ; i++) /* found leal val,%reg */
				if (p->sets == regmasx[i])
					const_val[i] = atoi(&p->op1[0]); /* save val */
		} else if (p->op == MOVL && p->op1[0] == '%'&& p->op2[0] == '%'  ) {
			for ( i1 = i2 = i = 0 ; i < (NREGS -2) ; i++) { /* found movl %reg,%reg */
				if (p->uses == regmasx[i])
					i1 = i; /*move from reg i1 */
				else if (p->sets == regmasx[i])
					i2 = i; /* to reg i2 */
			}
			const_val[i2] = const_val[i1];
		} else { /* kill value for set registers */
			for ( i = 0 ; i < (NREGS -2) ; i++)
				if (p->sets & regmasx[i])
					const_val[i] = 0;
		}
	}
}

