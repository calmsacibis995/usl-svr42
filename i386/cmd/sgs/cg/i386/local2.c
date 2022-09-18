/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:i386/local2.c	1.71"
/*	local2.c - machine dependent grunge for back end
 *	i386 CG
 *              Intel iAPX386
 */

# include "mfile2.h"
# include "string.h"

typedef	long	OFFSZ;		/* should be same as that defined in
				 * mfile1.h.  This became necessary
				 * to redefine with the change from
				 * 5.3.4 to 5.3.12
				 */
# define istnode(p) ((p)->in.op==REG && istreg((p)->tn.rval))

extern int zflag;       /* true if we print comments in assembly output */
extern int edebug;      /* true if we print expression debug info       */
extern int fpdebug;
static void blockmove();
static void starput();

extern int canbereg();

int vol_opnd = 0, special_opnd = 0;	/* Attributes volatile, pos_offset auto operand */
int cur_opnd = 1;	/* current operand  */

#define CLEAN()     {vol_opnd = 0; special_opnd = 0; cur_opnd = 1;}

extern RST regstused;	/* List of registers used for structure moves */

/* The outreg array maps a number for each value of rnames[] for
 * sdb debugging purposes.  We use PUSHA order for cpu registers.
 */
int outreg[] = {
    0,      /* %eax */
    2,      /* %ecx */
    1,      /* %edx */
    9,      /* %st0 */
    3,      /* %ebx */
    6,      /* %esi */
    7,      /* %edi */
    4,      /* %esp */
    5       /* %ebp */
};

TWORD register_ok[] = {
	TINT , TCHAR , TSHORT , TLONG , TUCHAR , TUSHORT ,
	TUNSIGNED , TULONG , TPOINT , TPOINT2, 0 };



char *rnames[] = {  /* normal access register names */
    "%eax", "%edx", "%ecx", "%st(0)",		/*scratch registers*/
    "%ebx", "%esi", "%edi",			/*user registers*/
    "%esp", "%ebp"                              /*other registers*/
};

static const char * const rsnames[] = { /* register names for shorts */
    "%ax",  "%dx",  "%cx",  "ERROR",
    "%bx",  "%si",  "%di",
    "%sp",  "%bp"
};

static const char * const rcnames[] = { /* register names for chars */
    "%al",  "%dl",  "%cl",  "ERROR",
    "%bl",  "ERROR","ERROR",
    "ERROR","ERROR"
};

static const char * const ccbranches[] = {
    "je",   "jne",  "jle",  "jl",   "jge",  "jg",
		    "jna", "jnae",  "jnb",  "jnbe"
};

static const char * const usccbranches[] = {
    "je",   "jne",  "jbe",  "jb",   "jae",  "ja",
		    "jna", "jnae",  "jnb",  "jnbe"
};

/* Floating Point Stack Simulation Routines 
** 
** The stack grows in such a way that fpstack[fpsp] is always
** the current top of stack.  fpsp == -1 means the stack is empty.
*/

extern void fp_init(), fp_cleanup(), fp_push();
static void fp_fill(), fp_pop(), fp_save(), fp_xch(), fp_clear(); 
static int fp_onstack(), fp_keep(), fp_widen();
static char fp_suffix();

static struct fpstype {
	TWORD	fpop;
	OFFSZ	fpoff1;
	OFFSZ	fpoff2;
	char    *name;
	int	isqnode;
	TWORD	ftype;
}	fpstack[8];	/* current contents of FP stack */

#define NOOP		((TWORD)(~0))	
#define BAD_OFFSET	0x80000000	/* used to signify empty fpoff2 */

static int	fpsp = -1;	/* current FP stack pointer */

#define FP_ISEMPTY() (fpsp == -1||(fpsp == 0 && fpstack[ST(0)].isqnode == 1))
#define FP_ISFULL()  (fpsp == 7)

#define ST(n)	(fpsp-n)

/* Lower is true if ST(i) is lower on stack than j */
#define LOWER(i, j) (i < j)

#define CHKUFLOW	\
	if (fpsp < -1) cerror("floating point stack underflow")
#define CHKOFLOW	\
	if (fpsp > 7) cerror("floating point stack overflow")

/* Print out the stack */
#ifndef NODBG
#define FP_DEBUG(s, l, loc)	if (fpdebug) fp_print(s, l, loc);
#else
#define FP_DEBUG(s, l, loc)
#endif

#ifndef NODBG
static void
fp_print(s, l, loc)
char *s;
int l;
int loc;
{
	int i;
	fprintf(outfile, "\n/ %s\n", s);
	if (l) fprintf(outfile, "/ stin line = %d\n", l);
	if (loc) fprintf(outfile, "/ temp location = %d\n", loc);
	fprintf(outfile, "/ loc\tfpop\tname\tfpoff1\tfpoff2\tisqnode\tftype\n");
	for (i=fpsp; i>=0; i--)
		if (fpstack[i].fpoff2 == BAD_OFFSET)
			fprintf(outfile, "/ st(%d)\t%d\t%s\t%d\tEMPTY\t%d\t%o\n",
				ST(i),
				fpstack[i].fpop,
				fpstack[i].fpop == NAME ? fpstack[i].name : "",
				fpstack[i].fpoff1,
				fpstack[i].isqnode,
				fpstack[i].ftype
			);
		else
			fprintf(outfile, "/ st(%d)\t%d\t%s\t%d\t%d\t%d\t%o\n",
				ST(i),
				fpstack[i].fpop,
				fpstack[i].fpop == NAME ? fpstack[i].name : "",
				fpstack[i].fpoff1,
				fpstack[i].fpoff2,
				fpstack[i].isqnode,
				fpstack[i].ftype
			);
	putc('\n', outfile);
}
#endif

/* Init the floating point stack (At routine start) */

void
fp_init()
{
	static void fp_clear();
	register int i;

	for(i = 0; i < 8; i++) 
		fp_clear(i);
	fpsp = -1;
}

static NODE tnode;

static void
fp_remove()
/* Remove the item that is on the top of the stack. */
{
	/* Check to see if we have seen only one qnode.  If this
	** is so, this entry is from the first choice in question-
	** colon operation and is not really on the stack.  This
	** is a little black magic...
	*/
	if (fpstack[ST(0)].isqnode == 1)
		return;
	if (fpstack[ST(0)].fpop != TEMP) {
		fprintf(outfile, "\tfstp\t%%st(0)\n");
		fp_pop();
		return;
	}
	/* Top of the stack is a TEMP.  Check if we are going
	** to reference its value again.
	*/
	tnode.in.op = TEMP; 
	tnode.in.type = fpstack[ST(0)].ftype;
	tnode.in.lval = fpstack[ST(0)].fpoff1;

	if (references(&tnode) > 0) {
		fprintf(outfile,"\tfstp%c\t%d(%%ebp)\n", 
			fp_suffix(tnode.in.type), tnode.in.lval);
	}
	else {
		fprintf(outfile, "\tfstp\t%%st(0)\n");
	}
	fp_pop();
	return;
}

void
fp_cleanup()
{
#ifndef NODBG
	if (fpdebug && !FP_ISEMPTY())
		fprintf(outfile, "/\tcleaning fp stack\n");
#endif
	while (!FP_ISEMPTY())
		fp_remove();
}

static void
fp_fill(i, op, name, off1, off2, isqnode, type)
int i;
TWORD op;
char *name;
OFFSZ off1, off2;
int isqnode;
TWORD type;
{
	register struct fpstype *fp = &fpstack[i];
	fp->fpop = op;
	fp->name = name;
	fp->fpoff1 = off1;
	fp->fpoff2 = off2;
	fp->isqnode = isqnode;
	fp->ftype = type;
}

void
fp_push(p)
NODE *p;
{
	fpsp++;
	CHKOFLOW;
	fp_fill(fpsp,p->tn.op,p->tn.name,p->tn.lval,BAD_OFFSET,0,p->tn.type);
}

static void
fp_pop()
{
	fpsp--;
	CHKUFLOW;
}

static void
fp_save()
{
	/* Save ST(0) at the end of the stack.  If the stack is too
	** full, put it out to memory.
	*/

	if (FP_ISEMPTY())
		cerror("fp_save(): empty stack");
	if (!FP_ISFULL()) {
		fpsp++;
		fprintf(outfile, "\tfld\t%%st(0)\n");
		fpstack[fpsp] = fpstack[fpsp-1];
	}
	else {
		char t;
		switch(fpstack[ST(0)].ftype) {
		case TFLOAT:	t = 's';	break;
		case TDOUBLE:	t = 'l';	break;
		case TLDOUBLE:	t = 't';	break;
		}

		switch(fpstack[ST(0)].fpop) {
		case TEMP: 
			fprintf(outfile,"\tfst%c\t%d(%%ebp)\n",
					t, fpstack[ST(0)].fpoff1);
			break;
		default:
		 	cerror("fp_save: bad op: %d", fpstack[ST(0)].fpop);
		}
	}
}			
	
static void
fp_xch(i)
{
	struct fpstype t;
	if (i >= fpsp) 
		cerror("fp_xch: illegal exchange value: %d", i);
	t = fpstack[i];
	fpstack[i] = fpstack[fpsp];
	fpstack[fpsp] = t;
}
	
static void
fp_clear(i)
int i;
{
	register struct fpstype *fp = &fpstack[i];
	fp->fpop = 0;
	fp->name = 0;
	fp->fpoff1 = 0;
	fp->fpoff2 = BAD_OFFSET;
	fp->isqnode = 0;
	fp->ftype = TVOID;
}

static int
fp_onstack(p)
NODE *p;
/* Return the location where p is closest to the top of the stack.
** It may be on the stack more than once.
*/
{
	int i;

	if (p->tn.op == REG)	/* Ya Gotta Believe */
		return ST(0);

	/* I do not fully understand isqnode.  I believe it is intended
	** to signify that we just assigned to QNODE, and therefore just
	** took a branch.  Is the stack empty below this point?  I do
	** not know; however, we dot save things on the stack if there
	** are branches, so maybe we are ok.  This should be heavily tested,
	** though undoubtedly it will not be heavily used.
	*/

	for (i=fpsp; i>=0; i--)

		if (fpstack[i].isqnode) 
			return -1;

		else if (optype(fpstack[i].fpop) != LTYPE)
			continue;

		else if (fpstack[i].fpop != NOOP && 			
		 	p->tn.op == fpstack[i].fpop &&		
			(p->tn.op != NAME || p->tn.name == fpstack[i].name) &&
		 	fpstack[i].ftype == p->tn.type &&		
		    		(p->tn.lval == fpstack[i].fpoff1 ||	
		     		 p->tn.lval == fpstack[i].fpoff2) )
			break;
	return i;
}

static char
fp_suffix(type)
TWORD type;
{
	switch (type) {
		case TFLOAT: 	return 's';
		case TDOUBLE:	return 'l';
		case TLDOUBLE:	return 't';
		default:	cerror("fp_suffix(): illegal type");
	}
	/* NOTREACHED */
}

static void
fp_binop(l, op, r)
NODE *l, *r;
int op;
{
	int where_left = fp_onstack(l);
	int where_right = fp_onstack(r);
	int live_left = fp_keep(l, 1);
	int live_right = fp_keep(r, 1);
	TWORD ltype = l->in.type;
	TWORD rtype = r->in.type;
	char *opstring;
	int reverse = 0;

	switch(op) {
	case PLUS:	opstring = "fadd";			break;
	case MINUS:	opstring = "fsub";	reverse = 1;	break;
	case MUL:	opstring = "fmul";			break;
	case DIV:	opstring = "fdiv";	reverse = 1;	break;
	}

	/* If an operand is extended precision, it must be
	** on the stack.
	*/
	if (ltype == TLDOUBLE && where_left == -1) {
		fprintf(outfile, "\tfldt\t");
		adrput(l);
		fprintf(outfile, "\n");
		fp_push(l);
		where_left = ST(0);
		if (r->in.op != REG && where_right == -1)
			where_right = fp_onstack(r);	/* in case l == r */
	}
	if (rtype == TLDOUBLE && where_right == -1) {
		fprintf(outfile, "\tfldt\t");
		adrput(r);
		fprintf(outfile, "\n");
		fp_push(r);
		where_right = ST(0);
		if (l->in.op != REG && where_left == -1)
			where_left = fp_onstack(l);
	}
	if (where_left == -1 && where_right == -1) {

		/* At least one must be on the stack.  Put
		** the one on that is live.  If one is a 
		** complicated expression, we do not track its
		** liveness, so load the other one.
		*/

		NODE *load = l;
		if (live_right)
			load = r;
		else if (optype(r->in.op == LTYPE)) 
			load = r;

		fprintf(outfile, "\tfld%c\t", fp_suffix(load->in.type));
		adrput(load);
		fprintf(outfile, "\n");
		fp_push(load);
		if (load == l) {
			where_left = ST(0);
			where_right = fp_onstack(r);	/* in case l == r */
		}
		else if (load == r) {
			where_right = ST(0);
			where_left = fp_onstack(l);	/* in case l == r */
		}
	}
	if (where_right != -1 && where_left == -1) {

		/* right on stack, left in memory. We want to
		** get the tree into a canonical form where
		** the left is on the stack and the right is
		** in memory.
		*/

		int tmp;
		NODE *ntmp;
		if (reverse == 1)
			reverse = 2;	/* operands have been reversed */

		/* reverse operands */
		tmp = where_left; where_left = where_right; where_right = tmp;
		tmp = live_left; live_left = live_right; live_right = tmp;
		tmp = ltype;  ltype = rtype;  rtype = tmp;
		ntmp = l; l = r; r = ntmp;
	}
	if (where_left != -1 && where_right == -1) {

		/* left on stack, right in memory */

		if (live_left) {

			/* Keep a copy on the stack */

			if (where_left == ST(0))
				fp_save();
			else {
				fprintf(outfile, "\tfld\t%%st(%d)\n", 
					ST(where_left));
				fp_push(l);
			}
		}
		else {
			/* We want left in ST(0) */

			if (where_left != ST(0)) {
				fprintf(outfile, "\tfxch\t%%st(%d)\n",
					ST(where_left));
				fp_xch(where_left);
			}
		}
		fprintf(outfile, "\t%s%s%c\t", 
			opstring, 
			reverse == 2 ? "r" : "",
			fp_suffix(rtype));
		adrput(r);
		fprintf(outfile, "\n");
		fpstack[ST(0)].fpop = NOOP;
		return;
	}
	/* both are on the stack */

	if (where_left == where_right) {
		/* same item, i.e. X + X */

		int live = fp_keep(l, 2);	/* we know about 2 refs */

		if (where_left == ST(0)) {
			if (live)
				fp_save();
		}
		else if (live) {
			fprintf(outfile, "\tfld\t%%st(%d)\n", 
				ST(where_left));
			fp_push(l);
		}
		else {
			fprintf(outfile, "\tfxch\t%%st(%d)\n",
				ST(where_left));
			fp_xch(where_left);
		}
		fprintf(outfile, "\t%s\t%%st(0),%%st\n", opstring);
		fpstack[ST(0)].fpop = NOOP;
		return;
	}
	/* Different locations on stack.  We want to get to a
	** canonical situation where the left is in ST(0).
	*/
	if (where_right == ST(0)) {
		int tmp;
		NODE *ntmp;
		if (reverse == 1)
			reverse = 2;	/* operands have been reversed */

		/* reverse operands */
		tmp = where_left; where_left = where_right; where_right = tmp;
		tmp = live_left; live_left = live_right; live_right = tmp;
		tmp = ltype;  ltype = rtype;  rtype = tmp;
		ntmp = l; l = r; r = ntmp;
	}
	else if (where_left != ST(0)) {
			fprintf(outfile, "fxch\t%%st(%d)\n",
				ST(where_left));
			fp_xch(where_left);
	}
	/* Both are now on the stack, the left is in ST(0) */

	if (live_left)
		fp_save();

	if (where_right == ST(1) && !live_right) {
		fprintf(outfile, "\t%s%s\n", opstring, reverse==2 ? "r" : "");
		fp_pop();
		fpstack[ST(0)].fpop = NOOP;
		return;
	}
	else {
		fprintf(outfile, "\t%s%s\t%%st(%d),%%st\n",
			opstring,
			reverse == 2 ? "r" : "",
			ST(where_right));
		fpstack[ST(0)].fpop = NOOP;
		return;
	}
}

static int
fp_keep(p, n)
NODE *p;
int n;
/* Should we keep p on the floating point stack?  n is the number
** of live references we already know about.  We are only interested
** in live references after the n'th.  Here are the series of criteria
** we are interested in:
**
**	1. The node must be live after the n'th use.
**	2. At least one of its live uses must not be an
**	   arithmetic operation that can just as easily access 
**	   memory.  (Should this include assigment?)
**	3. The node is live and it is long double.
**	4. The node is live and it is a TEMP
*/
{
	int i;
	NODE *t, *l, *r;
	extern int ref_entries;
	extern NODE **ref_trees;

	if (!is_live(p, n))
		return 0;	/* Criteria 1 failed */

#ifndef NODBG
	if (p->tn.op == REG)
		cerror("FP0 is live!");
#endif
	if (p->tn.type == TLDOUBLE)
		return 1;	/* Criteria 3 Passed */

	if (p->tn.op == TEMP)
		return 1;	/* Criteria 4 Passed */

	/* Check Second Criteria */

	for (i=n; i<ref_entries; i++) {
		t = ref_trees[i];
		if (t == NULL)
			return 1;	/* Just to be safe */
		switch (t->in.op) {

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:	
		case ASG PLUS:
		case ASG MINUS:
		case ASG MUL:
		case ASG DIV:	
			break;
		default:
			return 1;	/* Second Criteria Passed */
		}

		/* Here with arithmetic operation */

		/* Strip widening conversions */

		l = t->in.left;
		if (l->in.op == CONV && fp_widen(l)) 
			l = l->in.left;
		r = t->in.right;
		if (r->in.op == CONV && fp_widen(r))
			r = r->in.left;

		/* If both sides are leafs, and
		** neither is on the stack, we will
		** want to save this on the stack.
		*/
		if (	optype(r->in.type) == LTYPE &&
			optype(l->in.type) == LTYPE &&
			fp_onstack(l) == -1 &&
			fp_onstack(r) == -1
		   )
			return 1;
	}
	return 0;
}

static int 
fp_widen(p)
NODE *p;
/* return non-zero if p has a wider fp type than p->in.left */
{
	return (	p->in.type & (TFLOAT|TDOUBLE|TLDOUBLE) &&
			p->in.left->in.type & (TFLOAT|TDOUBLE|TLDOUBLE) &&
			gtsize(p->in.type) >= gtsize(p->in.left->in.type)
	);
}

			
		
static lasttype = 0;
static lastcmpe = 0;
#define NEXTZZZCHAR	( *++(*ppc) )
#define PEEKZZZCHAR	(*((*ppc)+1))

/*
 * The following codes are currently used in zzzcode:
 *		aBbCcDdEeFfGHIikLlMOoPpQRrSsTtuVWZz *+ 01234567
 */
void
zzzcode( pnode, ppc, q )        /* hard stuff escaped from templates */
register NODE *pnode;   /* tree node being expanded */
char **ppc;             /* code string, at 'Z' of macro name */
OPTAB *q;               /* template */
{
    register NODE *pl, *pr, *pn;
    register int temp1, temp2;
    static OFFSZ ctmp2;
    static OFFSZ ctmp1;
    static int save_lastcmpe;

    switch( NEXTZZZCHAR ) {

    case 'a': {
	/* Check if %ax is busy.  If so, spill, otherwise, %ax can be
	 * used - This is for fstsw instruction.
	 */
	OFFSZ tmp = 0;

	if (busy[REG_EAX]) {
		tmp = ( freetemp( 1 ) - maxboff ) / SZCHAR;
		fprintf(outfile, "\tmovl	%%eax,%ld(%%ebp)\n", tmp);
	}
	fprintf(outfile, "\tfstsw	%%ax\n\tsahf\n");
	if (tmp)
		fprintf(outfile, "\tmovl	%ld(%%ebp),%%eax\n",tmp);
	break;
    }

    case 'E':
	/* Print out the address of a free temp location.  This is
	 * used mainly in floating point conversions, where registers
	 * must be placed somewhere on the stack temporarily.
	 *
	 * ZEs: print temp for single-precision
	 * ZEd: print temp for double-precision
	 * ZEx: print temp for extended-precision
	 */
	{
		char type = NEXTZZZCHAR;
		if (ctmp2 == 0)
		{
			int words;
			switch (type) {
			case 's':       words = 1;      break;
			case 'd':       words = 2;      break;
			case 'x':	words = 3;	break;
			default:        cerror("illegal argument to ZE");
			}
			ctmp2 = ( freetemp( words ) - maxboff ) / SZCHAR;
		}
		fprintf(outfile,  "%ld(%%ebp)", ctmp2 );
	       break;
       }
    case 'e':
	/* Reset the temp location address.
	 */
	ctmp2 = 0;
	break;
    case 'k':
	/* Check that a temp really is on the stack.  If it is not,
	** push it on the stack and exchange the top two elements.
	** This is intended for uses such as an extended-precision
	** subtraction template with a Temp and FP0:  the template
	** assumes that the FP0 operand is the top of the stack, and
	** that the Temp is on the stack (unlike single- or double-
	** precision, where the Temp may be in memory.  If the tree
	** passed in is not of type TLDOUBLE, we will do nothing.
	** Thus, Zk may be called by a more generic template.
	*/ 
	pl = getadr(pnode, ppc);
	if (pl->tn.type != TLDOUBLE)
		break;
	FP_DEBUG("Zk: Ensure temp on stack", q->stinline, pl->tn.lval);

	if (fp_onstack(pl) >= 0)
		break;

	/* Push temp on stack, swap top two elements */
	fp_push(pl);
	emit_str("\tfldt\t");
	adrput(pl);
	if( zflag )
	    emit_str("\t\t/ Zk expansion");
	fp_xch(ST(1));
	emit_str("\n\tfxch\n");
	FP_DEBUG("After Zk, temp not found", 0, 0);
	break;

    case 'f':
	/* Generate the necessary floating point stack pop operand,
	 * and the necessary stack manipulation operands.  Pop the
	 * simulated stack.
	 */
	pl = getadr(pnode, ppc);
	FP_DEBUG("Before expanding Zf", q->stinline, pl->tn.lval);
	temp2 = fp_suffix(pl->in.type);
	temp1 = fp_onstack(pl);
	if (temp1 < 0) {		/* output std temp location */
		if (temp2 == 't')
			cerror("extended-precision operand must be on stack");
		fprintf(outfile, "%c\t", temp2);
		adrput(pl);
	}
	else if (temp1 == ST(0)) 
		fprintf(outfile, "\t%%st(0),%%st");
	else if (temp1 == ST(1))
		fp_pop();
	else
		fprintf(outfile, "\t%%st(%d),%%st", ST(temp1));

	fpstack[ST(0)].fpop = NOOP;	/* What should this be? */
	FP_DEBUG("After expanding Zf", 0, 0);
	break;
    case 'I':
	/* Generate the necessary stack address if it is a register
	 * and then increment the stack. if temp2 == 'N' don't
	 * generate an address
	 */
	pl = getadr(pnode, ppc);
	FP_DEBUG("Before expanding ZI", q->stinline, pl->tn.lval);
	temp2 = NEXTZZZCHAR;
	if (temp2 != 'N') {
		temp1 = fp_onstack(pl);
		if (temp1 < 0) {		/* output std temp location */
			fprintf(outfile, "%c\t", temp2);
			adrput(pl);
			break;
		}
		fprintf(outfile, "%%st(%d)", ST(temp1));
	}
	fp_push(pl);
	FP_DEBUG("After expanding ZI", 0, 0);
	break;
    case 'i':
	/* Pop a stack item.
	 */
	FP_DEBUG("Zi: pop stack", q->stinline, 0);
	fp_pop();
	FP_DEBUG("After expanding Zi", 0, 0);
	break;
    case '0':
	/* Make sure node really is in FP0 */
	pl = getadr(pnode, ppc);
	FP_DEBUG("Z0: Node must be in %%st(0)", q->stinline, pl->tn.lval);
	temp1 = fp_onstack(pl);
	if (temp1 < 0) {

		/* Not yet on stack */

		fprintf(outfile, "\tfld%c\t", fp_suffix(pl->in.type));
		adrput(pl);
		fprintf(outfile, "\n");
		fp_push(pl);

	} else if (temp1 != ST(0)) {

		/* On stack, but not in FP0 */

		fp_xch(temp1);
		fprintf(outfile, "\tfxch\t%%st(%d)\n", ST(temp1));
	}
	FP_DEBUG("After Z0", 0, 0);
	break;

    case '1':
	/* FP0 is holding a TEMP.  
	**
	**	if TEMP is not live but is referenced again:
	**	save TEMP to memory and pop the stack.  We 
	**	can do this because no claims are made that
	**	FP0 will hold the TEMP after Z1.
	**
	** If TEMP is left in FP0, paint FP0 as TEMP to avoid
	** exchanges.  Thus there will be two copies of TEMP on
	** the stack if the TEMP is still live.
	*/
	pl = getadr(pnode, ppc);
	FP_DEBUG("Z1: Keep live temp on stack", q->stinline, pl->tn.lval);

	if (!is_live(pl, 0) && references(pl) > 0) {
		fprintf(outfile,"\tfstp%c\t%d(%%ebp)\n", 
			fp_suffix(pl->in.type), pl->in.lval);
		fp_pop();
	}
	else
		fp_fill(ST(0), TEMP, NULL, pl->tn.lval, BAD_OFFSET, 0, pl->tn.type);
	FP_DEBUG("After Z1", 0, 0);
	break;

    case '2':
	/* Output correct operation for floating point binary
	** operation. 
	**
	** Z2t[i][r]
	**	t is the tree
	**	i is if we are loading from an integer
	**	r is if a sub or div must be reversed
	*/
	pl = getadr(pnode, ppc);

	/* The result will be in FP0.  If FP0 currently is holding
	** a live TEMP, we better save that TEMP.  For modularity, this
	** may want to go elsewhere, but it is here because it needs
	** go before the operation is output.
	*/
	if (fpstack[ST(0)].fpop == TEMP) {
		tnode.in.op = TEMP; 
		tnode.in.type = fpstack[ST(0)].ftype;
		tnode.in.lval = fpstack[ST(0)].fpoff1;
		if (is_live(&tnode, 0))
			fp_save();
		else if (references(&tnode) > 0) {
			fprintf(outfile,"\tfstp%c\t%d(%%ebp)\n", 
				fp_suffix(tnode.in.type), tnode.in.lval);
			fp_pop();
		}
	}
	fprintf(outfile, "\tf");
	if (PEEKZZZCHAR == 'i') {
		NEXTZZZCHAR;
		putc('i', outfile);
	}
	temp2 = PEEKZZZCHAR == 'r' ? NEXTZZZCHAR : 0;
	switch (pl->tn.op) {
		case DIV:
		case ASG DIV:
			if (temp2)
				fprintf(outfile, "divr");
			else
				fprintf(outfile, "div");
			break;
		case MINUS:
		case ASG MINUS:
			if (temp2)
				fprintf(outfile, "subr");
			else
				fprintf(outfile, "sub");
			break;
		case PLUS:
		case ASG PLUS:
			fprintf(outfile, "add");
			break;
		case MUL:
		case ASG MUL:
			fprintf(outfile, "mul");
			break;
		default:
			cerror("Bad operation in Z2 macro: %d", pl->tn.op);
	}
	break;
    case '3':
	/* Eat line if node is on stack */
	pl = getadr(pnode, ppc);
	if (fp_onstack(pl) != -1) 
		while( NEXTZZZCHAR != '\n' ) 
			;
	break;

    case '4':
	/* Generate code for a binary floating point operation.  The
	** tree passed in assumed to have the operation, with the
	** operands being the left and right children respectivley.
	** We are free to generate whatever code we want.
	*/
	pn = getadr(pnode, ppc);
	FP_DEBUG("Generating Binary expression", q->stinline, 0);
#ifndef NODBG
	if (fpdebug)	e2print(pn);
#endif
	pl = pn->in.left;
	pr = pn->in.right;
	if (pl->in.op == CONV) 
		pl = pl->in.left;
	if (pr->in.op == CONV)
		pr = pr->in.left;
	fp_binop(pl, pn->in.op, pr);
	FP_DEBUG("Done Generating Binary expression", 0, 0);
	break;

    case '5':
	/* Assigning NTMEM to TEMP.  Check if NTMEM is on stack.  If
	** it is, just paint that as the TEMP.  Otherwise, load it
	** into st(0).
	*/
	pl = getadr(pnode, ppc);
	FP_DEBUG("Loading NTMEM into TEMP", q->stinline, 0);
	pr = pl->in.right;
	if (pr->in.op == CONV)
		pr = pr->in.left;
	if ((temp1 = fp_onstack(pr)) != -1) {
		fpstack[temp1].fpop = TEMP;
		fpstack[temp1].ftype = pl->in.type;
		fpstack[temp1].fpoff1 = pl->in.left->in.lval;
	}
	else {
		fprintf(outfile,"\tfld%c\t",fp_suffix(pr->in.type));
		adrput(pr);
		fprintf(outfile, "\n");
		fp_push(pl->in.left);
	}
	FP_DEBUG("After Loading NTMEM into TEMP", 0, 0);
	break;

    case '6':
	/* Are we done with a CSE?  If so, output a 'p' (indicating
	** pop floating point stack) and pop the stack.
	*/
	FP_DEBUG("Before expanding Z6", q->stinline, 0);
	pl = getadr(pnode, ppc);
	if (pl->in.op == CSE && references(pl) == 1) {
		putc('p', outfile);
		fp_pop();
		FP_DEBUG("After expanding Z6", q->stinline, 0);
	}
	break;
    case '7':
	/* Mark ST(0) with NOOP */
	FP_DEBUG("Before expanding Z7", q->stinline, 0);
	fpstack[ST(0)].fpop = NOOP;
	FP_DEBUG("After expanding Z7", q->stinline, 0);
	break;

    case 'Q':
	/* Increment the number of Qnodes currently loaded.  This
	 * is needed when ?: statements pare used as args to subroutines
	 * passing floats (extra stack elements popped from ZG).
	 */
	FP_DEBUG("Before expanding ZQ", q->stinline, 0);
	if (fpstack[ST(1)].isqnode == 1) {
		fpstack[ST(1)].fpoff2 = fpstack[ST(0)].fpoff1;
		fpstack[ST(1)].isqnode = 2;
		fp_pop();
	}else
		fpstack[ST(0)].isqnode = 1;
	FP_DEBUG("After expanding ZQ", 0, 0);
	break;
    case 'p':
	/* Used for assigning return value.  After this routine,
	** the value passed in must be in %st(0), and no other
	** value may be on the stack.  The simulated stack will
	** be cleared.
	*/
	pl = getadr(pnode, ppc);
	FP_DEBUG("Before expanding Zp", q->stinline, pl->in.lval);

	if (pl->tn.op == REG) {
		if (fpsp > 0) {
			/* Move st(0) to st(fpsp) and pop fpsp.  
			** Then pop fpsp-1 elements off the stack.
			*/
			fprintf(outfile,"\tfstp\t%%st(%d)\n", fpsp);
			fpsp -= 2;	
			fp_cleanup();
		}
	}
	else {

		temp1 = fp_onstack(pl);

		/* Clean the stack until it is empty, or until the
		** value we want is at the top of the stack.
		*/
		while (temp1 != fpsp && !FP_ISEMPTY())
			fp_remove();

		/* Now the stack is empty, or pl is on the top of the stack */

		if (FP_ISEMPTY()) {
			fprintf(outfile, "\tfld%c\t", fp_suffix(pl->in.type));
			adrput(pl);
			putc('\n', outfile);
		} else if (fpsp != 0) {
			/* Move st(0) to st(fpsp) and pop fpsp.  
			** Then pop fpsp-1 elements off the stack.
			*/
			fprintf(outfile,"\tfstp\t%%st(%d)\n", fpsp);
			fpsp -= 2;	
			fp_cleanup();
		}
	}
	fpsp = -1;
	break;
    case 'G':
	FP_DEBUG("Before expanding ZG", q->stinline, 0);
	fp_cleanup();
	FP_DEBUG("After expanding ZG", 0, 0);
	break;

    case 'L': {
#define MAXLAB  5
	int tempval = NEXTZZZCHAR;
	switch( tempval ) {
	        static int labno[MAXLAB+1]; /* saved generated labels */
	        int i, n;
	case '.':                       /* produce labels */
	        /* generate labels from 1 to n */
	        n = NEXTZZZCHAR - '0';
	        if (n <= 0 || n > MAXLAB)
	            cerror("Bad ZL count");
	        for (i = 1; i <= n; ++i)
	            labno[i] = getlab();
	        break;
	
	default:
	        cerror("bad ZL");
		/*NOTREACHED*/
	
	/* generate selected label number */
	case '1':
	case '2':
	case '3':
	case '4':          /* must have enough cases for MAXLAB */
	case '5':
	        fprintf(outfile,".L%d", labno[tempval - '0']); 
	        break;
	}         
	break;
    }
    case 'R':
	/* Read and change the rounding mode in the 80?87 to chop; remember
	 * old value at ctmp1(%ebp).  Use Zr to restore.
	 */
	ctmp1 = ( freetemp( 1 ) - maxboff ) / SZCHAR;
	fprintf(outfile,  "\tfstcw\t%ld(%%ebp)\n\tmovw\t%ld(%%ebp),",
	    ctmp1, ctmp1 );
	temp1 = lasttype;
	lasttype = TSHORT;
	expand( pnode, FOREFF, "ZA2\n\torw\t$0x0c00,ZA2\n\tmovw\tZA2,", q );
	lasttype = temp1;
	fprintf(outfile,  "%ld(%%ebp)\n\tfldcw\t%ld(%%ebp)",
	    ctmp1+2, ctmp1+2 );
	if( zflag )
	    emit_str("\t\t/ ZR expansion");
	putc('\n',outfile );
	break;
    case 'r':
	/* restore old rounding mode */
	fprintf(outfile,  "\tfldcw\t%ld(%%ebp)", ctmp1 );
	if( zflag )
	    emit_str("\t\t/ Zr expansion");
	putc('\n',outfile );
	break;

    case 'u':
	/* Set rounding mode to -Inf for unsigned conversion magic.
	** Assume ctmp1+2 already contains the "chop" rounding mode.
	*/
	fprintf(outfile, "\txorw\t$0x800,%ld(%%ebp)\n", ctmp1+2);
	fprintf(outfile, "\tfldcw\t%ld(%%ebp)", ctmp1+2);
	if( zflag )
	    emit_str("\t\t/ Zu expansion");
	putc('\n',outfile );
	break;

    case 'c':
	/* Pop the appropriate argsize from sp */
	if (pnode->stn.argsize == SZINT)
	    fprintf(outfile, "\tpopl\t%%ecx\n");
	else
	    fprintf(outfile, "\taddl\t$%d,%%esp\n",
		    (((unsigned)pnode->stn.argsize+(SZINT-SZCHAR))/SZINT)*(SZINT/SZCHAR));
	break;

    case 'd':
	/* Output floating constant for INT_MAX or UINT_MAX:
	** Zd.i:  output constant (INT_MAX); Zdi:  output address
	** Zd.u:  output constant (UINT_MAX); Zdu:  output address
	** Zd.iu: output both constants; Zdiu:  output both addresses (sic)
	*/
    {
	static int labs[2];	/* [0] for int; [1] for uint */
	static const char * const fmts[2] = {
	    ".L%d:	.long	0x0,0x41e00000\n",	/* int */
	    ".L%d:	.long	0x0,0x41f00000\n",	/* uint */
	};
	int c;
	int gendata = 0;
	int didprefix = 0;

	if ((c = NEXTZZZCHAR) == '.') {
	    gendata = 1;
	    c = NEXTZZZCHAR;
	}

	for (;;) {
	    int index;
	    int lab;

	    switch( c ){
	    case 'i':	index = 0; break;
	    case 'u':	index = 1; break;
	    default:	cerror("bad Zd%c", c);
	    }
	    
	    lab = labs[index];
	    if (gendata) {
		if (lab == 0) {
		    labs[index] = lab = getlab();
		    if (!didprefix) {
			(void) locctr(FORCE_LC(CDATA));
			emit_str("	.align	4\n");
		    }
		    fprintf(outfile, fmts[index], lab);
		    didprefix = 1;
		}
	    }
	    else {
		if (picflag) {
		    fprintf(outfile, ".L%d@GOTOFF(%s)", lab, rnames[BASEREG]);
		    gotflag |= GOTREF;
		}
		else
		    fprintf(outfile, ".L%d", lab);
	    }
	    if (PEEKZZZCHAR == 'u')
		c = NEXTZZZCHAR;
	    else
		break;
	}
	if (didprefix)
	    (void) locctr(PROG);
	break;
    }
    case 'P':
	/* Allocate and generate the address for the appropriate amount
	 * of space for a return struct field.  This is a replacement
	 * for the 'T' macro when TMPSRET is defined.
	 */
	pl = talloc();
	pl->tn.op = TEMP;
	pl->tn.type = TSTRUCT;
	pl->tn.lval = freetemp( (pnode->stn.stsize+(SZINT-SZCHAR))/SZINT );
	pl->tn.lval /= SZCHAR;
	adrput(pl);
	tfree(pl);
	break;
    case 'B':
	/* The B and b macros set and reset the lasttype flag.  This
	 * variable is used by the T,t,A zzzcode macros to determine
	 * register name size to output.  T and t output the register
	 * names according to parameters in various subtrees, where as
	 * A prints the register names according to the last T,t,B.
	 */
	switch( *++(*ppc) ) {
	    case '1':
		if (*(*ppc + 1) == '0') {
		    lasttype = TLDOUBLE;
		    ++(*ppc);
		    break;
		}
		lasttype = TCHAR;
		break;
	    case '2':
		lasttype = TSHORT;
		break;
	    case '4':
		lasttype = TINT;
		break;
	    case '6':
		lasttype = TFLOAT;	/* can't be 4, but will probably
					 * never be used anyways... */
		break;
	    case '8':
		lasttype = TDOUBLE;
		break;
	}
	break;

    case 'b':
	/* reset the lasttype flag. */
	lasttype = 0;
	break;

    case 'T':
	/* Output the appropriate size for the given operation */
	/* 'T' Does not try to do zero/sign extend */
	pl = getadr(pnode, ppc);
	lasttype = pl->in.type;
	switch (pl->in.type) {
	    case TCHAR:
	    case TUCHAR:
		putc('b',outfile);
		break;
	    case TSHORT:
	    case TUSHORT:
		putc('w',outfile);
		break;
	    case TINT:
	    case TUNSIGNED:
	    case TLONG:
	    case TULONG:
	    case TPOINT:
		putc('l',outfile);
		break;
	    case TFLOAT:
		if (pl->in.op != REG)
		    putc('s',outfile);
		break;
	    case TDOUBLE:
		if (pl->in.op != REG)
		    putc('l',outfile);
		break;
	    case TLDOUBLE:
		if (pl->in.op != REG)
		    putc('t',outfile);
		break;
	    default:
		emit_str("ERROR");
		break;
	}
	break;
    case 't':
	/* Output the appropriate size for the given operation */
	/* 't' Does zero/sign extend  When appropriate */
	pl = getadr(pnode, ppc);
	lasttype = pl->in.type;
	if (pl->in.type != pnode->in.type) {
	    switch( pl->in.type) {
		case TCHAR:
		case TSHORT:
		    putc('s',outfile);
		    break;
		case TUCHAR:
		case TUSHORT:
		    putc('z',outfile);
		    break;
		default:
		    break;
	    }
	}
	switch (pl->in.type) {
	    case TCHAR:
	    case TUCHAR:
		putc('b',outfile);
		break;
	    case TSHORT:
	    case TUSHORT:
		putc('w',outfile);
		break;
	    case TINT:
	    case TUNSIGNED:
	    case TLONG:
	    case TULONG:
	    case TPOINT:
		putc('l',outfile);
		break;
	    default:
		emit_str("ERROR");
		break;
	}
	if (pl->in.type != pnode->in.type) {
	    switch( pnode->in.type) {
		case TSHORT:
		case TUSHORT:
		    if ((pl->in.type & (TSHORT|TUSHORT)) &&
			(pnode->in.type & (TSHORT|TUSHORT)))
			    break;
		    putc('w',outfile);
		    break;
		case TLONG:
		case TULONG:
		case TINT:
		case TUNSIGNED:
		case TPOINT:
		case TFLOAT:		/* floats doubles/ go to int regs    */
		case TDOUBLE:		/* first and the templates know this */
		case TLDOUBLE:
		    if ((pl->in.type & (TLONG|TULONG|TINT|TUNSIGNED|TPOINT)) &&
			(pnode->in.type & (TLONG|TULONG|TINT|TUNSIGNED|TPOINT)))
			    break;
		    putc('l',outfile);
		    break;
		default:
		    break;
	    }
	}
	break;

    case 'A':
	/*
	 * This is a special output mode that allows registers of
	 * a lower type (specifically char) to me used in a movl.
	 * This is used in a case such as when you want to:
	 *	movl	%edi, %al	but really must do:
	 *	movl	%edi, %eax
	 * The assembler can't handle the first case.
	 */
	pl = getadr(pnode, ppc);
	switch (lasttype) {
	    case TCHAR:
	    case TUCHAR:
		emit_str( rcnames[regno(pl)]);
		break;
	    case TSHORT:
	    case TUSHORT:
		emit_str( rsnames[regno(pl)]);
		break;
	    default:
		emit_str( rnames[regno(pl)]);
		break;
	}
	break;

    case 'F':
	/* Clobber the input line if the field passed is of size 1 and
	** the right side is an ICON equal to 1.
	*/
	pl = getlr( pnode, NEXTZZZCHAR );		/* FLD operator */
	pr = getlr( pnode, NEXTZZZCHAR );		/* ICON operator */
	if (UPKFSZ(pl->tn.rval) == 1 && pr->tn.lval == 1)
		while (NEXTZZZCHAR != '\n')
			;
	break;

    case 'H':
	/* Put out a shifted constant that has been properly
	 * modified for FLD ops.  This constant is anded (and
	 * overwritten) so it will fit into the field.  Then
	 * it is shifted for the field location and written out.
	 */
	{
	extern int fldsz, fldshf;
	/* table of masks to truncate field to size[i] */
	static unsigned long bits[33] = {
	    0x00000000, 0x00000001, 0x00000003, 0x00000007,
	    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	    (unsigned int)0xffffffff
	};


	pl = getlr( pnode, NEXTZZZCHAR );		/* FLD operator */
	pr = getlr( pnode, NEXTZZZCHAR );		/* ICON operator */
	fldsz = UPKFSZ(pl->tn.rval);
	fldshf = UPKFOFF(pl->tn.rval);
	pr->tn.lval &= bits[fldsz];		/* truncate constant to size */
	fprintf(outfile, "%ld", pr->tn.lval << fldshf);
	}
	break;

    case 'O':
	pl = getadr(pnode, ppc);
	fprintf(outfile,  "%d", 1 << pl->tn.lval );
	break;

    case 'C':
	if (lastcmpe || save_lastcmpe) {
		/* Generate a unsigned (floating point) conditional branch */
		fprintf(outfile,  "%s\t.L%d",
		    usccbranches[pnode->bn.lop - EQ], pnode->bn.label );
	} else {
		/* Generate a conditional branch */
		fprintf(outfile,  "%s\t.L%d",
		   ccbranches[pnode->bn.lop - EQ], pnode->bn.label );
	}
	if( zflag ) {
	    emit_str( "\t\t/ ZC expansion\n");
	}
	putc('\n',outfile );
	save_lastcmpe = lastcmpe = 0;
	break;

    case 'D':
	/* The next zzzcode better be a ZC or Zl or this is useless */
	lastcmpe = 1;
	return;

    case 'l':
	/* Save lastcmpe until ZC is seen. */
	if (lastcmpe) 
	    save_lastcmpe = 1;
	break;

    case 'S':
	/* STASG - structure assignment */
	fprintf(outfile,"/STASG**************:\n");
        /* If the root strat field contains a VOLATILE flag,
               move the strat field to the left and right nodes */

        if ( pnode->in.strat & VOLATILE )
        {
                pnode->in.left->in.strat |= VOLATILE;
                pnode->in.right->in.strat |= VOLATILE;
        }
        /* now call stasg for the actual structure assignemnt */
	stasg( pnode->in.left, pnode->in.right, pnode->stn.stsize, q );
	CLEAN();
	fprintf(outfile,"/End STASG^^^^^^^^^^^^^^:\n");
	break;

    case 's':
	/* STARG - structure argument */
	{   /* build a lhs to stasg to, on the stack */
	NODE px, pxl, pxr;

	px.in.op = PLUS;
	pxl.tn.type = px.in.type = pnode->in.left->in.type;
	px.in.left = &pxl;
	px.in.right = &pxr;
	px.in.strat = 0;
	pxl.tn.op = REG;
	pxl.tn.rval = REG_ESP;
	pxl.tn.strat = 0;
	pxr.tn.op = ICON;
	pxr.tn.type = TINT;
	pxr.tn.name = 0;
	pxr.tn.lval = 0;
	pxr.tn.strat = 0;

	fprintf(outfile,"/STARG**************:\n");
	fprintf(outfile,  "\tsubl\t$%d,%%esp",
		((pnode->stn.stsize+(SZINT-SZCHAR))/SZINT)*(SZINT/SZCHAR));
	if( zflag ) {
	    emit_str( "\t\t/ STARG setup\n");
	}
	putc('\n',outfile );
	if (pnode->in.strat & VOLATILE) {
	    px.tn.strat = VOLATILE;
	    pnode->in.left->in.strat |= VOLATILE;
	}
	stasg( &px, pnode->in.left, pnode->stn.stsize, q );
	CLEAN();
	fprintf(outfile,"/End STARG^^^^^^^^^^^^^^:\n");
	break;
	}

	/*
	 * This macro generates the optimal multiply by a constant
	 * sequence.  The calling sequence is:
	 *        ZM'SIZE','CONSTANT','OP','RESULT'.
	 * For instance ZML,R,L,1[,2] might generate the proper shift
	 * sequence or the sequence imulZTL $CR,AL,A1.  The break off
	 * point for generating shifts/adds/subs/moves over imulx is
	 * nine cycles.  pr is the constant's node, pl is the left
	 * side, and pt is the temp location node.  [,2] is an optional
	 * second temporary which may be useful.
	 */
    case 'M':
	{
		register NODE *pt;
		char multype;

		pr = getadr(pnode, ppc);
		switch (pr->in.type) {
			case TCHAR: case TUCHAR:
				multype = 'b';
				break;
			case TSHORT: case TUSHORT:
				multype = 'w';
				break;
			default:
				multype = 'l';
				break;
		}
		if (NEXTZZZCHAR != ',')
			cerror( "botched ZM macro");
		pr = getadr(pnode, ppc);
		if (NEXTZZZCHAR != ',')
			cerror( "botched ZM macro");
		pl = getadr(pnode, ppc);
		if (NEXTZZZCHAR != ',')
			cerror( "botched ZM macro");
		pt = getadr(pnode, ppc);
		if ( **ppc == ',' ) {
			++(*ppc);
			(void) getadr(pnode, ppc);
		}
		fprintf(outfile, "\timul%c\t", multype);
		adrput(pr);
		putc(',',outfile);
		vol_opnd_end();
		adrput(pl);
		putc(',',outfile);
		vol_opnd_end();
		adrput(pt);
		putc('\n',outfile);
		vol_instr_end();
	}
	break;			/* OUT of 'M' */

    case 'v':
        pl = getadr(pnode, ppc);
        if (pl->in.strat & VOLATILE)  vol_opnd |= cur_opnd;
	break;

    case 'W':
		fprintf(outfile,"/BLOCK MOVE*************:\n");
		blockmove(pnode, q);
                CLEAN();
		fprintf(outfile,"/END BMOVE^^^^^^^^^^^^^^:\n");
		break;

    /* Put out section directive for either .data or .rodata.
    ** Next character is 'd' for data, 't' for text.
    */
    case 'o':
		(void)locctr(NEXTZZZCHAR == 'd' ? FORCE_LC(CDATA) : PROG);
		break;

    /* Put out uninitialized data to "zero" */
    case 'z':
		zecode(pnode->tn.lval);
		break;
    /* Treat operand as address mode under * */
    case '*':
	pl = getadr( pnode, ppc );		/* operand */
	starput( pl, (CONSZ) 0 );
	break;
    
    /* Operand is ++/-- node.  Produce appropriate increment/decrement
    ** code for the left operand of the ++/-- (actually, *(R++ )).
    */
    case '+':
    {
	int isincr;

	pl = getadr(pnode, ppc)->in.left;
	switch (pl->in.op) {
	case INCR:	isincr = 1; break;
	case DECR:	isincr = 0; break;
	default:
	    cerror("Bad Z+ operator %s", opst[pl->in.op]);
	}

	if (pl->in.right->tn.lval == 1)
	    fprintf(outfile, "	%s	", isincr ? "incl" : "decl");
	else
	    fprintf(outfile, "	%s	$%d,", isincr ? "addl" : "subl",
				pl->in.right->tn.lval);
	adrput(pl->in.left);
	putc('\n', outfile);
	break;
    }
    case 'Z':
	/* Eat line if tree given is not unsigned (ZZ+) or signed (ZZ-) */
	temp1 = NEXTZZZCHAR;
	pl = getadr(pnode, ppc);
	switch (pl->in.type) {
	case TCHAR: case TSHORT: case TINT: case TLONG:
		if (temp1 == '+')
			while( NEXTZZZCHAR != '\n' ) 
				/* EMPTY */ ;
		break;
	case TUCHAR: case TUSHORT: case TUNSIGNED: case TULONG:
		if (temp1 == '-')
			while( NEXTZZZCHAR != '\n' ) 
				/* EMPTY */ ;
		break;
	default:
		cerror("Bad type passed to ZZ macro");
	}
	break;

    default:
	cerror( "botched Z macro %c", **ppc );
	/*NOTREACHED*/
    }
    lastcmpe = 0;		/* zero out if the last op wasn't a ZC */
}
void
conput( p ) 			/* name for a known */
register NODE *p; 
{
    if (p->in.strat & VOLATILE) vol_opnd |= cur_opnd;
    switch( p->in.op ) 
    {
    case ICON:
	acon( p );
	break;
    case REG:
	switch( p->tn.type )   /* get the right name for a register */
	{
	default:        /* extended register, e.g., %eax */
	    emit_str( rnames[p->tn.rval]);
	    break;
	case TSHORT:
	case TUSHORT:   /* short register, %ax */
	    emit_str( rsnames[p->tn.rval]);
	    break;
	case TCHAR:
	case TUCHAR:    /* char register, %al */
	    emit_str( rcnames[p->tn.rval]);
	    break;
	}
	break;
    case UNINIT:
        acon(p);
        break;
    case CSE:
        {
             struct cse *cs_ptr;
             if ( (cs_ptr = getcse(p->csn.id)) == NULL)
             {
                   e2print(p);
                   cerror("Uknown CSE id");
             }
	    
	     switch( p->tn.type )   /* get the right name for a register */
	     {
	     default:        /* extended register, e.g., %eax */
	         emit_str( rnames[cs_ptr->reg]);
	         break;
	     case TSHORT:
	     case TUSHORT:   /* short register, %ax */
	         emit_str( rsnames[cs_ptr->reg]);
	         break;
	     case TCHAR:
	     case TUCHAR:    /* char register, %al */
	         emit_str( rcnames[cs_ptr->reg]);
	         break;
	     }
             break;
	}
    case COPY:
    case COPYASM:
        if ( p->tn.name)
             emit_str(p->tn.name);
        break;

    default:
	cerror( "confused node in conput" );
	/*NOTREACHED*/
    }
}
/*ARGSUSED*/
void
insput( p ) NODE *p; { cerror( "insput" ); /*NOTREACHED*/ }

/* Generate an addressing mode that has an implied STAR on top. */

static int baseregno;			/* base register number */
static int indexregno;			/* index register number */
static int scale;			/* scale factor */
static CONSZ offset;			/* current offset */
static char *name;			/* for named constant */
static CONSZ findstar();

static void
starput( p, inoff )
NODE *p; 
int inoff;				/* additional offset (bytes) */
{
    int flag = 0;
    /* enter with node below STAR */
    baseregno = -1;
    indexregno = -1;
    scale = 1;
    offset = inoff;
    name = (char *) 0;

    /* Find the pieces, then generate output. */
    offset += findstar(p,0,&flag);

    /* Prefer base register to index register. */
    if (indexregno >= 0 && baseregno < 0 && scale == 1) {
	baseregno = indexregno;
	indexregno = -1;
    }

    if (name) {
	emit_str(name);
	if (picflag) {
	    char *outstring = "";
	    if (flag & PIC_GOT) {
		gotflag |= GOTREF;
		if (flag & (NI_FLSTAT|NI_BKSTAT))
		    outstring = "@GOTOFF"; 
		else if (offset)
		    cerror("starput: illegal offset in ICON node: %s", name);
		else 
		    outstring = "@GOT";
	    } 
	    else if (flag & PIC_PLT) {
		gotflag |= GOTREF;
		outstring = "@PLT";
	    }
	    fprintf(outfile, outstring);
	}
	if (offset > 0)
	    putc('+',outfile);
    }
    if (offset)
	fprintf(outfile, "%ld", offset);
    else if (baseregno == REG_ESP)
	putc('0',outfile );
    

    putc('(',outfile);
    if (baseregno >= 0)
	fprintf(outfile, "%s", rnames[baseregno]);
    if (indexregno >= 0) {
	fprintf(outfile, ",%s", rnames[indexregno]);
	if (scale > 1)
	    fprintf(outfile, ",%d", scale);
    }
    putc(')',outfile );
    sideff = 0;
    return;
}


/* Find the pieces for address modes under *.  Return
** the numeric part of any constant found.  If "scaled"
** set, any register must be an index register.
*/
static CONSZ
findstar(p,scaled,flag)
NODE * p;
int scaled;
int *flag;
{
    switch( p->in.op ){
    case CSE:
        {
             struct cse *cs_ptr;
             if ( (cs_ptr = getcse(p->csn.id)) == NULL)
             {
                   e2print(p);
                   cerror("Uknown CSE id");
             }
	     if (scaled) {
	     	    /* Double-indexed with address constant case. */
		    if (indexregno >= 0)
		    	baseregno = indexregno;
		    indexregno = cs_ptr->reg;
	     }
	     else if (baseregno < 0)
		    baseregno = cs_ptr->reg;
	     else
	            indexregno = cs_ptr->reg;
	     return( 0 );
        }
    case REG:
	/* Base register has pointer type, index is other.
	** With double indexing and an address constant, there
	** may be two index registers.  Be careful not to overwrite
	** the index register, and make sure the index register is
	** the one that gets scaled, if necessary.
	*/
	if (scaled) {
	    /* Double-indexed with address constant case. */
	    if (indexregno >= 0)
		baseregno = indexregno;
	    indexregno = p->tn.rval;
	}
	else if (baseregno < 0)
	    baseregno = p->tn.rval;
	else
	    indexregno = p->tn.rval;
	return( 0 );
    case ICON:
	name = p->tn.name;
	*flag = p->tn.strat | (p->tn.rval & (NI_BKSTAT|NI_FLSTAT));
	return( p->tn.lval );
    {
	CONSZ temp;
    /* Force specific ordering of tree walk. */
    case PLUS:
	temp = findstar(p->in.left,0,flag);
	return(temp + findstar(p->in.right,0,flag));
    case MINUS:
	temp = findstar(p->in.left,0,flag);
	return(temp - findstar(p->in.right,0,flag));
    }
    case LS:
	/* Assume ICON on right. */
	scale = 1 << p->in.right->tn.lval;
	return(findstar(p->in.left,1,flag));
    case UNARY AND:
	p = p->in.left;
	if (p->in.op != VAUTO)
	    break;
	if (baseregno >= 0)
	    indexregno = baseregno;
	baseregno = REG_EBP;
	return( p->tn.lval );
    }
    cerror("confused findstar, op %s", opst[p->in.op]);
    /* NOTREACHED */
}


void
adrput( p ) 			/* output address from p */
NODE *p; 
{      

    sideff = 0;

again:
    while( p->in.op == FLD || p->in.op == CONV )
	p = p->in.left;
    if (p->in.strat & VOLATILE)  vol_opnd |= cur_opnd;

    switch( p->in.op ) 
    {
    case ICON:               /* value of the constant */
	putc('$',outfile );
	/*FALLTHRU*/
    case NAME:
	acon( p );
	break;
    case CSE:
    case REG:
	conput( p );
	break;
    case STAR:
	if( p->in.left->in.op == UNARY AND ) {
	    p = p->in.left->in.left;
	    goto again;
	}
	starput( p->in.left, 0 );
	break;
    case UNARY AND:
	switch( p->in.left->in.op) {
	case STAR:
	    p = p->in.left->in.left;
	    goto again;
	case TEMP:
	case VAUTO:     /* these appear as part of STARG and STASG */
	case VPARAM:
	    p = p->in.left;
	    goto again;
	case NAME:
	    p = p->in.left;
	    p->in.op = ICON;
	    goto again;
	default:
	    cerror( "adrput:  U& over nonaddressable node" );
	    /*NOTREACHED*/
	}
	break;
    case TEMP:
	fprintf(outfile,  "%ld(%%ebp)", p->tn.lval /*- ( maxboff / SZCHAR )*/ );
	break;
    case VAUTO:
	if(p->tn.lval >= 0)
		special_opnd |= cur_opnd;
    case VPARAM:
	fprintf(outfile,  "%ld(%%ebp)", p->tn.lval );
	break;
    default:
	cerror( "adrput: illegal address" );
	/* NOTREACHED */
    }
}

    /* Output address of n words after p.  Used for double moves. */
void
upput( p, n ) 
register NODE *p; 
int n;
{
    register NODE *psav = p;

    while( p->in.op == FLD || p->in.op == CONV ) {
	p = p->in.left;
    }
    if ((p->in.strat & VOLATILE) || (psav->in.strat & VOLATILE))
	vol_opnd |= cur_opnd;

recurse:
    switch( p->in.op ) {
    case NAME:
    case VAUTO:
    case VPARAM:
	p->tn.lval += n * SZINT/SZCHAR;
	adrput( psav );
	p->tn.lval -= n * SZINT/SZCHAR;
	break;
    case REG:
    case CSE:
	fprintf(outfile,  "%d", n * SZINT/SZCHAR );
	adrput( psav );
	break;
    case TEMP:
	p->tn.lval += n * SZINT/SZCHAR;
	adrput( psav );
	p->tn.lval -= n * SZINT/SZCHAR;
	break;
    case STAR:
	starput( p->in.left, n * SZINT/SZCHAR );
	break;
    case UNARY AND:
	p = p->in.left;
	goto recurse;

    default:
	cerror( "upput:  confused addressing mode" );
	/*NOTREACHED*/
    }
}
void
acon( p ) 			/* print out a constant */
register NODE *p; 
{       
    register OFFSZ off;

    if( p->in.name == (char *) 0 ) 	/* constant only */
    {            
	if( p->in.op == ICON &&
		( p->in.type == TCHAR || p->in.type == TUCHAR ) ) {
	    p->tn.lval &= 0xff;
	}
	fprintf(outfile,  "%ld", p->tn.lval );
    } 
    else {
	char *outstring = "";
	if (picflag) {
	    register flag = p->tn.strat;
	    if (flag & PIC_GOT) {
		gotflag |= GOTREF;
		if (p->tn.rval & (NI_FLSTAT|NI_BKSTAT))
		    outstring = "@GOTOFF";
		else if (p->tn.lval)
		    cerror("acon: illegal offset in ICON node: %s", p->in.name);
		else
		    outstring = "@GOT";
	    } 
	    else if (flag & PIC_PLT) {
		gotflag |= GOTREF;
		outstring = "@PLT";
	    }
	}
	if( ( off = p->tn.lval ) == 0 )     /* name only */
	    fprintf(outfile,  "%s%s", p->in.name, outstring );
	else if( off > 0 )                       /* name + offset */
	    fprintf(outfile,  "%s%s+%ld", p->in.name, outstring, off );
	else                                     /* name - offset */
	    fprintf(outfile,  "%s%s%ld", p->in.name, outstring, off );
    }
}

void
p2cleanup()
{
	/* must cleanup floating point stack after every statement.  This
	** could be done after every basic block, but I believe the cost
	** is the same.  This keeps the stack cleaner and allows for
	** potentially more temporaries in use to be kept on the stack.
	*/
	fp_cleanup();
}

/*ARGSUSED*/
special( sc, p )
NODE *p;
{
    cerror( "special shape used" );
    /* NOTREACHED */
}

/* Move an intermediate result in a register to a different register. */
void 
rs_move( pnode, newregno ) 
NODE * pnode; int newregno; 
{
    register int type = pnode->tn.type;
    int r = regno(pnode);

    if( type & (TINT|TUNSIGNED|TPOINT) ) 
	fprintf(outfile,  "\tmovl\t%s,%s", rnames[r], rnames[newregno] );
    else if( type & (TSHORT|TUSHORT) )
	fprintf(outfile, "\tmovw\t%s,%s", rsnames[r], rsnames[newregno] );
    else if( type & (TCHAR|TUCHAR) ) 
	fprintf(outfile, "\tmovb\t%s,%s", rcnames[r], rcnames[newregno] );
    else 
	cerror( "bad rs_move" );
    
    if( zflag ) {           /* if commenting on source of lines */
	emit_str( "\t\t/ RS_MOVE\n");
    }
    putc('\n',outfile );
}

static int
is_address_mode(p)
NODE *p;
/* Return non-zero if p can be evaluated by an address mode */
{

	NODE *l, *r;
	NODE *ll, *lr, *rl, *rr;

	/* Strip off top level STAR.  If no STAR, we can use leal */

	if (p->tn.op == STAR)
		p = p->tn.left;

	l = p->tn.left;
	r = p->tn.right;
	if (p->tn.op == MINUS)
		goto minus;	/* special case */
	if (p->tn.op != PLUS)
		return 0;

	/* Pick up indirect without base 
	**
	**	(Rsreg[iuip]+C[iuip])
	**	((Rsreg[iui]<<C)+C[iuip])	first C is 1,2,3
	*/
#	define SHIFT_CON(p)	(p->tn.op == ICON && \
				p->tn.lval >= 1 && p->tn.lval <= 3)

	if (r->tn.op == ICON && (r->tn.type & (TINT|TUNSIGNED|TPOINT))) {
		
		/* right side ok, check left side */

		if (l->tn.op == REG && (l->tn.type & (TINT|TUNSIGNED|TPOINT)))
			return 1;

		if (l->tn.op == LS) {
			ll = l->tn.left;
			lr = l->tn.right;

			if (	ll->tn.op == REG && 
			    	(ll->tn.type & (TINT|TUNSIGNED)) &&
				SHIFT_CON(lr)
			   )
				return 1;
		}
	}
	/* Double indexing with REG and implied stack pointer 
	**
	** 1.   &A + Rsreg[iui]
	** 2.	&A + (Rsreg[iui]<<C)
	** 3.	(&A+Rsreg[iui]) + C
	** 4.	(&A+(Rsreg[iui]<<C)) + C
	*/
#	define AAUTO(p)	(p->tn.op == UNARY AND && p->tn.left->tn.op == VAUTO)

	if (AAUTO(l)) {
		/* Case 1, 2 */
		if (r->tn.op == REG && (r->tn.type & (TINT|TUNSIGNED)))
			return 1;	/* case 1 */
		rl = r->tn.left;
		rr = r->tn.right;
		if (	rl->tn.op == REG && 
			(rl->tn.type & (TINT|TUNSIGNED)) &&
			SHIFT_CON(rr)
		   )
			return 1;	/* Case 2 */
	}
	ll = l->tn.left;
	if (l->tn.op == PLUS && AAUTO(ll) && r->tn.op == ICON) {
		/* Case 3, 4 */
		lr = l->tn.right;
		if (lr->tn.op == REG && (lr->tn.type & (TINT|TUNSIGNED)))
			return 1;	/* Case 3 */
		if (	lr->tn.op == LS &&
			lr->tn.left->tn.op == REG &&
			(lr->tn.left->tn.type & (TINT|TUNSIGNED)) &&
			SHIFT_CON(lr->tn.right)
		   )
			return 1;	/* Case 4 */
	}
	/* Double Indexing with two explicit registers 
	**
	** 1.	Rsreg[iuip] + Rsreg[iuip]
	** 2.	Rsreg[iuip] + (Rsreg[iui]<<C)
	** 3.	(Rsreg[iuip]+Rsreg[iuip]) + C
	** 4.	(Rsreg[iuip]+(Rsreg[iui]<<C1)) + C 
	*/

	if (l->tn.op == REG && (l->tn.type & (TINT|TUNSIGNED|TPOINT))) {
		/* Case 1, 2 */
		if (r->tn.op == REG && (r->tn.type & (TINT|TUNSIGNED|TPOINT))) 
			return 1;	/* case 1 */
		rl = r->tn.left;
		rr = r->tn.right;

		if (	r->tn.op == LS &&
			rl->tn.op == REG &&
			(rl->tn.type & (TINT|TUNSIGNED)) &&
			SHIFT_CON(rr)
		   )
			return 1;	/* case 2 */
	}
	if (r->tn.op == ICON && l->tn.op == PLUS) {
		/* Case 3, 4 */
		ll = l->tn.left;
		lr = l->tn.right;
		if (	ll->tn.op == REG  &&	
			(ll->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			lr->tn.op == REG  &&
			(lr->tn.type & (TINT|TUNSIGNED|TPOINT))
		   )
			return 1;	/* case 3 */
		if (	ll->tn.op == REG  &&    
                        (ll->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			lr->tn.op == LS &&
			lr->tn.left->tn.op == REG &&
			(lr->tn.left->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			SHIFT_CON(lr->tn.right)
		   )
			return 1;	/* case 4 */
	}
	
	return 0;

	/* Double Indexing with two explicit registers, special case
	** with - as root of tree.
	**
	** 5.   (Rsreg[iuip]+Rsreg[iuip]) - C[!p]
	** 6.	(Rsreg[iuip]+(Rsreg[iui]<<C)) - C[!p]
	*/
minus:
	if (l->tn.op == PLUS && r->tn.op == ICON && (!(r->tn.type & TPOINT))) {
		ll = l->tn.left;
		lr = l->tn.right;
		if (	ll->tn.op == REG  &&	
			(ll->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			lr->tn.op == REG  &&
			(lr->tn.type & (TINT|TUNSIGNED|TPOINT))
		   )
			return 1;	/* case 5 */

		if (	ll->tn.op == REG  &&    
                        (ll->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			lr->tn.op == LS &&
			lr->tn.left->tn.op == REG &&
			(lr->tn.left->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			SHIFT_CON(lr->tn.right)
		   )
			return 1;	/* case 6 */
	}
	return 0;
}

NODE *
suggest_spill(p)
NODE *p;
/* For now, this routine will only be concerened with finding
** the proper side of a floating point operation to spill.
*/
{
	int op = p->tn.op;
	TWORD type = p->tn.type;
	TWORD ltype, rtype;
	NODE *l = p->tn.left;
	NODE *r = p->tn.right;

	if (optype(op) != BITYPE)
		return NULL;		/* no need to make suggestion */

	if (asgop(op))
		return NULL;		/* not prepared to handle this yet */

	ltype = l->tn.type;
	rtype = r->tn.type;
	
	if ((ltype|rtype|type) & (~(TLDOUBLE|TDOUBLE|TFLOAT)))
		return NULL;

	/* Spill one side if other side is a leaf */

	if (optype(l->tn.op) == LTYPE)
		return r;	
	if (optype(r->tn.op) == LTYPE)
		return l;	

	if (is_address_mode(l))
		return r;
	if (is_address_mode(r))
		return l;
	return NULL;
}

static void
blockmove(pnode, q)
NODE *pnode;
OPTAB *q;
{
	/* Do a block move.  from and to are scratch
	** registers, count is either a scratch register
	** or a constant.
	*/
	int bk_vol_opnd = (pnode->in.strat & VOLATILE) ? VOL_OPND2 : 0;
	NODE *pcount, *pfrom, *pto;	/*node pointers */

	/* This can be either a block move or VLRETURN node.
	** For block moves, the count may or may not be a 
	** scratch register; if not, a1 is available for a copy.
	** For VLRETURN, the count must be in a scratch reg;
	** a1 contains the "to" adress (%fp)
	*/

	switch ( pnode->in.op)
	{
	case BMOVE:
		/* Do a stasg() so that the code need only be maintained
		** in one place.  If the move count is not a constant, though,
		** we cannot do a stasg().
		*/
		pcount = pnode->in.left;
		pfrom = pnode->in.right->in.right;
		pto = pnode->in.right->in.left;
		if (pcount->tn.op != ICON || pcount->tn.name != 0)
			break;
		/* If the root strat field contains a VOLATILE flag,
		** move the strat field to the left and right nodes 
		*/

		if ( bk_vol_opnd ) {
			pfrom->in.strat |= VOLATILE;
			pto->in.strat |= VOLATILE;
		}
		stasg(pto, pfrom, pcount->tn.lval * SZCHAR, q);
                CLEAN();
		return;

	case BMOVEO:
		cerror("blockmove: BMOVEO unsupported");
		/*NOTREACHED*/
	default:
		cerror("Bad node passed to ZM macro");
		/*NOTREACHED*/
	}
	/* Variable length block moves are not used by C.
	** Furthermore, we did them incorrectly in the past.  
	** For this reason, we will exit with a cerror at this point.
	*/
	cerror("blockmove: Variable length moves unsupported");
	/*NOTREACHED*/
}

void
end_icommon()
{
	(void) locctr(locctr(UNK));
}

costex()
{
	cerror("costex(): not ported for i386 CG\n");
	/*NOTREACHED*/
}
/* output volatile operand information at the end of an instruction */
void
vol_instr_end()
{
        int opnd;
        int first = 1;

        for (opnd=0; vol_opnd; ++opnd)
        {
            if (vol_opnd & (1<<opnd))
            {
                   /* first time output the information for the instruction */
                   if ( first )
                   {
                        PUTS("/VOL_OPND ");
                        first = 0;
                   }
                   else
                        PUTCHAR(',');
                   vol_opnd &= ~(1<<opnd);      /* clean up the checked operand bit */
                   fprintf(outfile, "%d", opnd+1);
            }
        }

        if ( !first ) PUTCHAR('\n');

	first = 1;

        for (opnd=0; special_opnd; ++opnd)
        {
            if (special_opnd & (1<<opnd))
            {
                   /* first time output the information for the instruction */
                   if ( first )
                   {
                        PUTS("/POS_OFFSET ");
                        first = 0;
                   }
                   else
                        PUTCHAR(',');
                   special_opnd &= ~(1<<opnd);      /* clean up the checked operand bit */
                   fprintf(outfile, "%d", opnd+1);
            }
        }

        if ( !first ) PUTCHAR('\n');

	CLEAN();    /* reset the initial values for bookkeeping variables */
}

/* Routines to support HALO optimizer. */

#ifdef	OPTIM_SUPPORT

#ifndef	INI_OIBSIZE
#define	INI_OIBSIZE 100
#endif

static char oi_buf_init[INI_OIBSIZE];	/* initial buffer */
static
TD_INIT(td_oibuf, INI_OIBSIZE, sizeof(char), 0, oi_buf_init, "optim support buf");

#define	OI_NEED(n) if (td_oibuf.td_used + (n) > td_oibuf.td_allo) \
			td_enlarge(&td_oibuf, td_oibuf.td_used+(n)) ;
#define	OI_BUF ((char *)(td_oibuf.td_start))
#define	OI_USED (td_oibuf.td_used)

/* Produce #REGAL information for a local temp. */
static int temp_set = 0;
#define MAX_TEMPS  64
 /* optim will not use more then 32 temps */
struct temp_loc {
      OFFSET off;
      OFFSET last_off;
};
static struct temp_loc temp_array[MAX_TEMPS];

/*  oi_temp() will save temps created by freetemp(). If the temp size is more
** then one word this temp can't be in register.
**
**  oi_temp() will return true if temp that can't be in register overlap
** with /REGAL or if /REGAL overlap with long temp. In this case freetemp
** will try another offset.
*/

int
oi_temp(offset,size)
OFFSET offset;
int size;
{
	int i,last_offset;

	last_offset = offset + (size - 1) * 4;
	if (temp_set == MAX_TEMPS )
		return 0; /* too many temps. */
	if (size > 1) {
		for (i = 0; i < temp_set; i++) {
			if (    temp_array[i].off == temp_array[i].last_off
			     && temp_array[i].off >= offset
			     && temp_array[i].off <= last_offset
			)
				return 1; /* Not a REGAL overlap with REGAL */
			else if (   temp_array[i].off == offset
				 && temp_array[i].last_off == last_offset)
				return 0; /* identical to an old temp */ 
		}
	}
	else {
		for (i = 0; i < temp_set; i++) {
			if (   temp_array[i].off != temp_array[i].last_off
			    && temp_array[i].off <= offset
			    && temp_array[i].last_off >= offset
			)
				return 1; /* REGAL overlap with no REGAL */
			else if (   temp_array[i].off == offset )
				return 0; /* identical to an old temp */
		}
	}
	temp_array[temp_set].off = offset;
	temp_array[temp_set++].last_off = last_offset;
	return 0;
}

void
oi_temp_end(size)
OFFSET size;            /* size is byte size */
{
	int i;
	if (temp_set == MAX_TEMPS)
		temp_set = 0;	/* too many temps */
	else if (temp_set) {
		for (i = 0; i < temp_set; i++) {
			if (	temp_array[i].off== temp_array[i].last_off 
			  && 	temp_array[i].off < size)
				fprintf(outfile, "/REGAL\t0\tAUTO\t%ld(%%ebp)\t4\n",
						temp_array[i].off);
		}
		temp_set =0;
	}
}

/* Produce comment for loop code. */

char *
oi_loop(code)
int code;
{
    char * s;

    switch( code ) {
    case OI_LSTART:	s = "/LOOP	BEG\n"; break;
    case OI_LBODY:	s = "/LOOP	HDR\n"; break;
    case OI_LCOND:	s = "/LOOP	COND\n"; break;
    case OI_LEND:	s = "/LOOP	END\n"; break;
    default:
	cerror("bad op_loop code %d", code);
    }
    return( s );
}


/* Analog of adrput, but this one takes limited address modes (leaves
** only) and writes to a buffer.  It returns a pointer to just past the
** end of the buffer.
*/
static void
sadrput(p)
NODE * p;				/* node to produce output for */
{
    int n;

    /* Assume need space for auto/param at a minimum. */
    /*      % n ( % ebp) NUL */
    OI_NEED(1+8+1+1+ 3+1+1);

    switch( p->tn.op ){
    case VAUTO:
    case VPARAM:	OI_USED += sprintf(OI_BUF+OI_USED, "%ld(%%ebp)",
								p->tn.lval);
			break;
    case NAME:		n = strlen(p->tn.name);
			OI_NEED(n+1);
			(void) strcpy(OI_BUF+OI_USED, p->tn.name);
			OI_USED += n;
			if (p->tn.lval != 0) {
			    OI_NEED(1+8+1);
			    OI_USED += sprintf(OI_BUF+OI_USED, "%s%ld",
					(p->tn.lval > 0 ? "+" : ""),
					p->tn.lval);
			}
			if (   picflag
			    && (p->tn.rval & (NI_GLOBAL))) {
			    OI_NEED(4+1);
			    (void) strcpy(OI_BUF+OI_USED, "@GOT");
			    OI_USED += 4;
			}
			else if (   picflag
			    && (p->tn.rval & (NI_FLSTAT|NI_BKSTAT))) {
			    OI_NEED(7+1);
			    (void) strcpy(OI_BUF+OI_USED, "@GOTOFF");
			    OI_USED += 7;
			}
			break;
    default:
	cerror("bad op %d in sadrput()", p->in.op);
    }
    return;
}

/* Note that the address of an object was taken. */

char *
oi_alias(p)
NODE * p;
{
    BITOFF size = (p->tn.type & (TVOID|TSTRUCT)) ? 0 : gtsize(p->tn.type);

    OI_USED = 0;			/* start buffer */
    /*	    /ALIAS\t	*/
    OI_NEED(1+   5+1+1);
    OI_USED += sprintf(OI_BUF, "/ALIAS	");
    sadrput(p);
    /*	    \t% n\tFP\n */
    OI_NEED(1+1+8+1+2+1+1);
    (void) sprintf(OI_BUF+OI_USED, "	%ld%s\n", size/SZCHAR,
		(long)(p->tn.type & (TFLOAT|TDOUBLE|TLDOUBLE)) ? "	FP" : "");
    return( OI_BUF );
}

/* Produce #REGAL information for a symbol. */

char *
oi_symbol(p, class)
NODE * p;
int class;
{
    char * s_class;

    switch( class ) {
    case OI_AUTO:	s_class = "AUTO"; break;
    case OI_PARAM:	s_class = "PARAM"; break;
    case OI_EXTERN:	s_class = "EXTERN"; break;
    case OI_EXTDEF:	s_class = "EXTDEF"; break;
    case OI_STATEXT:	s_class = "STATEXT"; break;
    case OI_STATLOC:	s_class = "STATLOC"; break;
    default:
	cerror("bad class %d in op_symbol", class);
    }

    OI_USED = 0;			/* initialize */
    /*		/REGAL\t 0\tSTATLOC\t	*/
    OI_NEED(	1+   5+1+1+1+     7+1+1 );
    OI_USED += sprintf(OI_BUF, "/REGAL	0	%s	", s_class);
    sadrput(p);
    /*	    \t% n\tFP\n */
    OI_NEED(1+1+8+1+2+1+1);
    (void) sprintf(OI_BUF+OI_USED, "	%d%s\n", gtsize(p->tn.type)/SZCHAR,
		(p->tn.type & (TFLOAT|TDOUBLE|TLDOUBLE)) ? "	FP" : "");
    return( OI_BUF );
}

#endif	/* def OPTIM_SUPPORT */
