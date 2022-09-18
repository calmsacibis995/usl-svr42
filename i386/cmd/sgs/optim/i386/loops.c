/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/loops.c	1.5"

#include <stdio.h>
#include "sched.h"
#include "optutil.h"

int zflag = false; /*debugging flag*/
static int label_counter = 0; /*for generating labels*/

/*is p a conditional jump backwards*/
/*return the jump target if it is, NULL otherwise*/
static NODE *
is_back_jmp(p) NODE *p;
{
NODE *q;
	if (!iscbr(p)) return NULL;
	for (q = p; q != &n0; q = q->back) /* search for label backwards */
		if (islabel(q) && !strcmp(p->op1,q->opcode))
			return q;
	return NULL;
}/*end is_back_jmp*/

/*find whether or not there are free registers in the loop. if not return 0.
**if there are, return the first free one in regs_list[].
*/
static unsigned int
find_available_regs_in_loop(first,last) NODE *first,*last;
{
extern int suppress_enter_leave;
#define regs   ( EAX | EBX | ECX | EDX | ESI | EDI | EBI | ESP | EBP )
#define scratch_regs	( EAX | ECX | EDX )
static unsigned int regs_list[] = { EAX , EDX , ECX , EBX , ESI , EDI , EBI };
#ifdef DEBUG
static char * regs_names[] = { "eax" , "edx" , "ecx" , "ebx" , "esi" , "edi" , "ebi" };
#endif
int i;
NODE *p;
unsigned int live_in_loop = 0;
unsigned int available_regs;
	if (suppress_enter_leave) live_in_loop |= EBI;
	for (p = first; p != last->forw; p = p->forw) {
		live_in_loop |= (p->nlive | p->sets | p->uses);
		if (p->op == CALL)
			live_in_loop |= scratch_regs;
		/* are all the registers live at one single instruction? */
		/* this might be interesting for a more agressive search */
		if ((p->nlive | regs) == p->nlive) {
#ifdef DEBUG
			if (zflag)
				fprintf(stderr,"all regs live at once. lost\n");
#endif
			return 0;
		}
	}/*for loop*/
	if ((live_in_loop | regs) == live_in_loop) {
#ifdef DEBUG
		if (zflag)
			fprintf(stderr,"all live somewhere in the loop\n");
#endif
		return 0;
	} else {
		available_regs = regs & ~live_in_loop;
		for ( i = 0; i < sizeof(regs_list) / sizeof(int) ; i++ )
			if ((available_regs & regs_list[i]) == regs_list[i]) {
#ifdef DEBUG
				if (zflag)
					fprintf(stderr,"found reg %x, that's %s\n"
						,regs_list[i],regs_names[i]);
#endif
				return regs_list[i];
			}
		return 0;
	}
	/* NOTREACHED */
}/* end find_available_regs_in_loop*/

#if 0
/*level is 0 for an innermost loops, one more for each level of nesting.
**use it to save compile time, since it is not reasonable to believe
**that many outer loops will have free registers.
**currently not activated since the above is false.
*/
static int
find_level_of_loop(first,last) NODE *first,*last;
{
NODE *p;
int level = 0;
	for (p= first; p != last; p = p->forw)
		if (is_back_jmp(p))
			level++;
	if (zflag)
		fprintf(stderr,"loops inside this loop: %d\n",level);
	return level;
}/*end find_level_of_loop*/
#endif

/*if there is an fp instruction that uses the loop index as an operand,
**like fild -8(%ebp), that it is not replaceable with a register since
**fild %ecx is illegal.
*/
static boolean
fp_uses_index(first,last,increment) NODE *first,*last,*increment;
{
NODE *p;
char *index;
	if (increment->op == INCL)
		index = increment->op1;
	else
		index = increment->op2;
	for (p = first; p != last->forw; p = p->forw) {
		if (isfp(p)) {
			if (p->op1 && !strcmp(p->op1,index))
				return true;
			if (p->op2 && !strcmp(p->op2,index))
				return true;
		}
	}
	return false;
}/*end fp_uses_index*/

/*If the original memory operand is used as an eight bit operand than it
**is not replaceable with neither one of the three registers edi, esi or
**ebp. These registers are the last ones to be chosen, therefore no other
**register is available.
*/
static boolean
reg_8_probs(reg,increment,first,last)
unsigned int reg;
NODE *increment,*first,*last;
{
char *index;
NODE *p;
	if (reg != ESI && reg != EDI && reg != EBI)
		return false;
	if (increment->op == INCL)
		index = increment->op1;
	else
		index = increment->op2;
	for (p = first; p != last->forw; p = p->forw) {
		if (OpLength(p) == ByTE) {
			if (!strcmp(index,p->op1) || !strcmp(index,p->op2))
				return true;
		}
	}
	return false;
}/*end reg_8_probs*/

/*Find the instruction that increments the loop index. Return a pointer
**to the instruction if found, NULL pointer otherwise.
**Consider it found only if it is useful: it is not allready a register,
**and it is in a memory location where it is replaceable by a register:
**either a regal, or disambiguated from all other memory locations in the
**loop. This uses seems_same() from sched.c. That one used to assume that
**it gets both it's argumwnts from the same basic block. This no longer holds
**there, for the correcness of the usage here.
*/
static NODE *
find_increment(compare,first,last) NODE *compare,*first,*last;
{
NODE *p,*q;
int x;
NODE *non_regal_inc = NULL;
char *index;
	for (p = compare; p != first->back; p = p->back) {
		if (p->op == INCL && !isreg(p->op1) &&
			(!strcmp(p->op1,compare->op1) || !strcmp(p->op1,compare->op2))) {
			x = atoi(p->op1);
			if (isregal(x)) {
#ifdef DEBUG
				if (zflag > 1) {
					fprintf(stderr,"increment is ");
					fprinst(p);
				}
#endif
				return p;
			}
			else if (!isvolatile(p,1)) {
				non_regal_inc = p; /*this is the one, non regal, save and*/
				break;			   /*disambiguate after the loop*/
			}
		}
		if (p->op == ADDL && !isreg(p->op2) &&
			(!strcmp(p->op2,compare->op1) || !strcmp(p->op2,compare->op2))) {
			x = atoi(p->op2);
			if (isregal(x)) {
#ifdef DEBUG
				if (zflag > 1) {
					fprintf(stderr,"increment is ");
					fprinst(p);
				}
#endif
				return p;
			}
			else if (!isvolatile(p,2)) {
				non_regal_inc = p;
				break;
			}
		}/*endif ADDL*/
	}/*end for loop*/
	/*it may be that the increment is not a regal, and waits in non_regal_inc.
	**verify that it is different from any other memory reference inside the
	**loop. Compare instructions that use the index with instructions that
	**set MEMory.
	*/
	if (!non_regal_inc) return NULL;
	if (non_regal_inc->op == INCL)
		index = non_regal_inc->op1;
	else
		index = non_regal_inc->op2;
	for (p = first ; p != last; p = p->forw) { /*add memory knowledge*/
		p->uses |= muses(p);
		p->sets = msets(p);
		p->idus = indexing(p);
	}/*for loop*/
	for (p = first ; p != last; p = p->forw) {
		if ((p->op1 && !strcmp(p->op1,index))
			|| (p->op2 && !strcmp(p->op2,index))) {
		/*p uses the index.*/
		/*q has to run in two loops. one until p and one from p to last
		**because seems_same counts on that it's first parameter is
		**after it's second parameter in the code sequence
		*/
			for (q = first; q != p; q = q->forw) {
				if ((q->sets | q->uses) & MEM) {
					if (!strcmp(q->op1,index) || !strcmp(q->op2,index))
						continue;
					else if (seems_same(q,p))
						return NULL;
				}
			}
			for (q = p->forw; q != last->forw; q = q->forw) {
				if ((q->sets | q->uses) & MEM) {
					if (!strcmp(q->op1,index) || !strcmp(q->op2,index))
						continue;
					else if (seems_same(p,q))
						return NULL;
				}
			}
		}/*endif*/
	}/*for loop*/
	for (p = first ; p != last; p = p->forw) { /*remove memory knowledge*/
		p->uses &= ~MEM;
		p->sets &= ~MEM;
	}/*for loop*/
	return non_regal_inc;
}/*end find_increment*/

/*A loop is considered simple if it has only one entery point,
**and either only one exit (by fallthrough), or some jmps, all
**to the same target.
**Three possible return values: 0 for non optimizable, 1 if only
**one exit, 2 if there are "legal" jumps out of the loop.
**in case of several jumps to one target, which is out of the loop,
**return via jtarget, the name of the label.
**A bug in this test: the jumps may be to the same block but to 
**some different but succesive labels. Currently this will return 0.
*/
static int /*not boolean*/
is_simple_loop(first,last,jtarget) NODE *first, *last; char **jtarget;
{
extern SWITCH_TBL *get_base_label();
NODE *p,*q;
boolean label_found = false;
boolean first_label = true;
char *target = NULL;
SWITCH_TBL *sw;
REF *r;
BLOCK *b;
	/*Special case: if there is a wild jump in the function, of the form
	**jmp	*%eax ; then no loop is simple.
	*/
	for (ALLN(p)) {
		if (p->op == JMP && *(p->op1) == '*' 
		 && !(p->op1[1] == '.' || isalpha(p->op1[1]) || p->op1[1] == '_')) {
			return 0;
		}
	}
	/*first test that jump from inside the loop stay inside */
	/*in particular, if there is a switch in the loop, verify that
	**all of the targets are inside the loop.
	*/
	for (p = first; p != last->forw; p = p->forw) {
		if (is_jmp_ind(p)) {
			sw = get_base_label(p->op1);
			for (r = sw->first_ref ; r != sw->last_ref; r = r->nextref) {
				b = r->switch_entry;
				label_found = false;
				for (q = first; q != last; q = q->forw) {
					if (q == b->firstn) label_found = true;
				}
				if (!label_found) return false;
			}
		} else if (isbr(p)) {
			label_found = false;
			for (q = first; q != last->forw; q = q->forw) {
				if (islabel(q) && !strcmp(p->op1,q->opcode)) {
					label_found = true;
					break;
				}
			}
			if (!label_found) { /*the jmp target is ! inside the loop*/
				if (first_label) {
					first_label = false;
					target = p->op1; /*save the first such jmp target*/
				} else { /*if this target != first target, it's ! simple*/
					if (strcmp(target,p->op1)) {
						return 0;
					}
				}
			}
		}
	}
	/*second test that jumps from before the loop dont enter*/
	for (p = n0.forw; p != first; p = p->forw) {
		if (isbr(p)) {
			label_found = false;
			for (q = first; q != last->forw; q = q->forw) {
				if (islabel(q) && !strcmp(p->op1,q->opcode)) {
					return 0;
				}
			}
		}
	}
	/*third test that jumps from after the loop dont enter*/
	for (p = last->forw; p != &ntail; p = p->forw) {
		if (isbr(p)) {
			label_found = false;
			for (q = first; q != last->forw; q = q->forw) {
				if (islabel(q) && !strcmp(p->op1,q->opcode)) {
					return 0;
				}
			}
		}
	}
	*jtarget = target;
	return target == NULL ? 1 : 2;
}/*end is_simple_loop*/

/*do the work. insert a movl prev_index,free_reg before the loop, a
**movl free_reg,prev_index after the loop, and replace all occurrences
**of the previous index by the register.
**If the loop has jump to a location out of the loop, than in addition:
**find that location, which is a label like .L105: and make it like 
**the sequence:
**	jmp .L105:
**R7:
**movl	%ecx,-4(%ebp)
**.L105:
*/
static void
place_index_in_reg(first,increment,compare,last,regnum,jtarget)
NODE *first,*increment,*compare,*last;
unsigned int regnum;
char *jtarget;
{
extern char *itoreg(),*itohalfreg(),*itoqreg();
extern NODE *prepend();
NODE *before,*after;
char *prev_index;
char *reg = itoreg(regnum);
char *halfreg = itohalfreg(regnum);
char *qreg = itoqreg(regnum);
BLOCK *b,*target_block;
NODE *target_label;
NODE *p;
boolean found_label = false;
char *new_label;
#ifdef DEBUG
	if (zflag) {
		fprintf(stderr,"do something");
		fprinst(first);
		if (zflag > 1) {
			for (p = first; p != last->forw; p = p->forw) {
				fprintf(stderr,"b4 ");
				fprinst(p);
			}
		}
	}
#endif
	if (jtarget) {
		new_label = getspace(LABELSIZE);
		sprintf(new_label,".R%d",++label_counter); /*prepare a new label*/
		/*find the block and node which are the target of the jump*/
		for (b = b0.next; b; b = b->next) {
			p = b->firstn;
			while (islabel(p)) {
				if (!strcmp(p->opcode,jtarget)) {
					target_block = b;
					target_label = p;
					found_label = true;
					break;
				}
				p = p->forw;
			}
			if (found_label)
				break;
		}/*for loop*/
	}/*endif jtarget*/
	if (increment->op == INCL) {
		prev_index = increment->op1;
		increment->op1 = reg;
	} else {
		prev_index = increment->op2;
		increment->op2 = reg;
	}
#ifdef DEBUG
	if (zflag) {
		fprintf(stderr,"prev index %s at",prev_index);
		fprinst(increment);
	}
#endif
	new_sets_uses(increment);

	if (!strcmp(compare->op1,prev_index))
		compare->op1 = reg;
	else 
		compare->op2 = reg;
	compare->uses = uses(compare);

	before = prepend(first,reg);
	chgop(before,MOVL,"movl");
	before->op1 = prev_index;
	before->op2 = reg;
	new_sets_uses(before);

	after = insert(last);
	chgop(after,MOVL,"movl");
	after->op1 = reg;
	after->op2 = prev_index;
	new_sets_uses(after);

	for (p = first; p != last->forw; p = p->forw) {
		if (p->op1 && !strcmp(p->op1,prev_index)) {
#ifdef DEBUG
			if (zflag) {
				fprintf(stderr,"prev to reg ");
				fprinst(p);
			}
#endif
			switch (OpLength(p)) {
				case LoNG: p->op1 = reg; break;
				case WoRD: p->op1 = halfreg; break;
				case ByTE: p->op1 = qreg; break;
				default: fatal("loop index: unexpected instruction size\n");
			}/*end switch*/
			new_sets_uses(p);
#ifdef DEBUG
			if (zflag)
				fprinst(p);
#endif
		}
		if (p->op2 && !strcmp(p->op2,prev_index)) {
#ifdef DEBUG
			if (zflag) {
				fprintf(stderr,"prev to reg ");
				fprinst(p);
			}
#endif
			switch (OpLength(p)) {
				case LoNG: p->op2 = reg; break;
				case WoRD: p->op2 = halfreg; break;
				case ByTE: p->op2 = qreg; break;
				default: fatal("loop index: unexpected instruction size\n");
			}/*end switch*/
			new_sets_uses(p);
#ifdef DEBUG
			if (zflag)
				fprinst(p);
#endif
		}
		if (jtarget && isbr(p) && !strcmp(jtarget,p->op1)) {
			p->op1 = new_label;
		}
	}/*for all N in the loop*/
	if (jtarget) { /*add some nodes for break treatment */
		p = prepend(target_block->firstn,NULL); /*add a node before jmp target*/
		chgop(p,JMP,"jmp"); /*make it a jmp to target */
		p->op1 = target_label->opcode; /*address the jmp to the label*/
		new_sets_uses(p);

		p = insert(p); /*add a node after the jmp*/
		chgop(p,LABEL,new_label); /*make it the new label*/

		p = insert(p); /*add a node for the move instruction*/
		chgop(p,MOVL,"movl");
		p->op1 = reg;
		p->op2 = prev_index;
		new_sets_uses(p);
	}/*endif*/
#ifdef DEBUG
	if (zflag > 1) {
		for (p = first; p != last->forw; p = p->forw) {
			fprintf(stderr,"c5 ");
			fprinst(p);
		}
		fprintf(stderr,"before ");
		fprinst(before);
		fprintf(stderr,"after ");
		fprinst(after);
	}
#endif
}/*end place_index_in_reg*/

static void
rm_dead_insts()
{
NODE *pf;
unsigned int regs_set;
char *dp;


	for (ALLN(pf)) {
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
			ldelin2(pf);			/* preserve line number info */
			mvlivecc(pf);			/* preserve condition codes line info */
			DELNODE(pf);			/* discard instruction */
    	}
	}
}

/*driver. find the loop, test if it is optimizable and do it.*/
void
loop_index()
{
NODE *pf,*pm,*pl;
NODE *first, *p;
unsigned int reg = 0;
char *jtarget = NULL;
#ifdef DEBUG
	if (zflag > 4) return;
#endif
	ldanal(); /*count on it in find dead regs in the loop*/
	/*
	** Next call ifdef'd out for d12.
	*/
#if 0
	rm_dead_insts();
#endif
	for (ALLN(pm)) {
		jtarget = NULL;
		pl = pm->forw; if (!pl || pl == &ntail) return;
		/*is it a cmp - jcc */
		if ((pm->op == CMPL) && (iscbr(pl))) {
			if ((first = is_back_jmp(pl)) == NULL) {
#ifdef DEBUG
			if (zflag > 1) {
				fprintf(stderr,"not a back jmp");
				fprinst(pl);
			}
#endif
				continue;
			}
			if ((pf = find_increment(pm,first,pl)) == NULL) {
#ifdef DEBUG
			if (zflag > 1) {
				fprintf(stderr,"didnt find increment for ");
				fprinst(first);
			}
#endif
				continue;
			}
			if (is_simple_loop(first,pl,&jtarget) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(stderr,"loop not simple: ");
					fprinst(first);
				}
#endif
				continue;
			}
			if (fp_uses_index(first,pl,pm)) {
#ifdef DEBUG
				if (zflag) {
					fprintf(stderr,"fp uses index ");
					fprinst(first);
				}
#endif
				continue;
			}
			reg = find_available_regs_in_loop(first,pl);
			if (reg_8_probs(reg,pf,first,pl)) {
#ifdef DEBUG
				if (zflag) {
					fprintf(stderr,"only non 8 bit reg, and counter used\n");
				}
#endif
				continue;
			}
			if (reg) {
#ifdef DEBUG
				if (zflag) {
					fprintf(stderr,"do something ");
					fprinst(first);
				}
#endif
				place_index_in_reg(first,pf,pm,pl,reg,jtarget);
				ldanal();
			}
		}/*endif cmp-jcc*/
	}/*end for loop*/
}/*end loop_index*/


