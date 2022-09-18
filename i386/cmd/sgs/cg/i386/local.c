/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:i386/local.c	1.32.1.6"
/*	local.c - machine dependent stuff for front end
 *	i386 CG
 *		Intel iAPX386
 */

#include <signal.h>
#include "mfile1.h"
#include "mfile2.h"
#include <string.h>
#include <memory.h>

/* register numbers in the compiler and their external numbers:
**
**	comp	name
**	0-2	eax,edx,ecx
**	3	fp0
**	5-7	ebx,esi,edi
*/

#ifndef TMPDIR
#define TMPDIR	"/tmp"		/* to get TMPDIR, directory for temp files */
#endif

/* bit vectors for register variables */

#define REGREGS		(RS_BIT(REG_EBX)|RS_BIT(REG_ESI)|RS_BIT(REG_EDI))
#define CREGREGS	(RS_BIT(REG_EBX))

/* *** i386 *** */
RST regstused = RS_NONE;
RST regvar = RS_NONE;
/* *** i386 *** */

int tmp_start;			/* start of temp locations on stack */
char request[TOTREGS];	/* list of reg vars used by the fct.
				   Given on either BEGF node or ENDF node */
int r_caller[]={-1};		/* no caller save register */
static int biggest_return;
static int ieeeflag = 1;

static int select();

static char *tmpfn;
static FILE *tmpfp;

FILE *fopen();
int proflag = 0;
int picflag = 0;
int gotflag;

int
ieee_fp()
{
	return ieeeflag;
}

void
p2abort()
{
	extern int unlink();
	if (tmpfp)
		(void) unlink(tmpfn);
	return;
}

void
myflags(cp)
char *cp;
{
	while (*cp) {
		switch (*cp)
		{
			case 'k':	picflag = 1; break;
			case 'i':	ieeeflag = *++cp - '0'; break;
			default:	break;
		}
		++cp;
	}
}

static char *locnames[] =
{
/*PROG*/	"	.text\n",
/*ADATA*/	"	.data\n",
/*DATA*/	"	.data\n",
/*ISTRNG*/	"	.section	.data1\n",
/*STRNG*/	"	.section	.data1\n",
/*CDATA*/	"	.section	.rodata\n",	/* read-only data */
/*CSTRNG*/	"	.section	.rodata1\n"	/* read-only strings */
};

# define FIRST_ISTR	"\t.section	.data1,\"aw\"\n"
# define FIRST_CSTR	"\t.section	.rodata1,\"a\"\n"

extern int lastalign; 

/* location counters for PROG, ADATA, DATA, ISTRNG, STRNG, CDATA, and CSTRNG */
locctr(l)		/* output the location counter */
{
	static int first_istring = 1;
	static int first_cstring = 1;
	static int lastloc = UNK;
	int retval = lastloc;		/* value to return at end */

	curloc = l;
	if (curloc != lastloc) lastalign = -1;

	switch (l)
	{
	case CURRENT:
		return ( retval );

	case PROG:
		lastalign = -1;
		/* FALLTHRU */
	case ADATA:
	case CDATA:
	case DATA:
		if (lastloc == l)
			break;
		outfile = textfile;
		if (picflag) {
		    if (curloc == CDATA) {
			curloc = DATA;
			emit_str(locnames[DATA]);
		    }
		    else
                        emit_str(locnames[l]);
                }
		else
		    emit_str(locnames[l]);
		break;

	case FORCE_LC(CDATA):
		if (lastloc == l)
			break;
		outfile = textfile;
		emit_str(locnames[CDATA]);
		break; 

	case STRNG:
	case ISTRNG:
	case CSTRNG:
		/* output string initializers to a temporary file for now
		 * don't update lastloc
		 */
		if (lastloc == l)
			break;
		outfile = textfile;
		if (curloc == CSTRNG && first_cstring)
		{
			emit_str(FIRST_CSTR);
			first_cstring = 0;
		}
		else if ( ((curloc == ISTRNG)||(curloc == STRNG)) && first_istring )
		{
			emit_str(FIRST_ISTR);
			first_istring = 0;
		}
		else
			emit_str(locnames[l]);
		break;

	case UNK:
		break;

	default:
		cerror( "illegal location counter" );
	}

	lastloc = l;
	return( retval );		/* declare previous loc. ctr. */
}

/* Can object of type t go in a register?  rbusy is array
** of registers in use.  Return -1 if not in register,
** else return register number.  Also fill in rbusy[].
*/
/*ARGSUSED*/
int
cisreg(op, t, off, rbusy, strat)
int op;					/* not used */
TWORD t;
OFFSET off;				/* not used */
char rbusy[TOTREGS];
int strat;				/* not used */
{
	int i;

	if (picflag)
		rbusy[BASEREG] = 1;

	if ( t & ( TSHORT | TUSHORT
		 | TINT   | TUNSIGNED
		 | TLONG  | TULONG
		 | TPOINT | TPOINT2) )
	{
		/* Have a type we can put in register. */
		for (i = USRREGHI-1; i >= NRGS; --i)
			if (!rbusy[i]) break;
	
		/* If i >= NRGS, i is the register number to
		** allocate.
		*/
	
		/* If candidate is suitable number grab it, adjust rbusy[]. */
		if (i >= NRGS) {
			rbusy[i] = 1;
			regvar |= RS_BIT(i);
			return( i );
		}
	}
	else
	if ( t & ( TCHAR | TUCHAR ) )
	{
		if (! rbusy[NRGS] ) {
			rbusy[NRGS] = 1;
			return( NRGS );
		}
	}

	/* Bad type or no register to allocate. */
	return( -1 );
}

NODE *
clocal(p)			/* manipulate the parse tree for local xforms */
register NODE *p;
{
	register NODE *l, *r;

	/* make enum constants into hard ints */

	if (p->in.op == ICON && p->tn.type == ENUMTY) {
	    p->tn.type = INT;
	    return( p );
	}

	if (!asgbinop(p->in.op) && p->in.op != ASSIGN)
		return (p);
	r = p->in.right;
	if (optype(r->in.op) == LTYPE)
		return (p);
	l = r->in.left;
	if (r->in.op == QUEST ||
		(r->in.op == CONV && l->in.op == QUEST) ||
		(r->in.op == CONV && l->in.op == CONV &&
		l->in.left->in.op == QUEST))
				/* distribute assigns over colons */
	{
		register NODE *pwork;
		extern NODE * tcopy();
		NODE *pcpy = tcopy(p), *pnew;
		int astype = p->in.type;	/* remember result type of asgop */
#ifndef NODBG
		extern int xdebug;
		if (xdebug)
		{
			emit_str("Entering [op]=?: distribution\n");
			e2print(p);
		}
#endif
		pnew = pcpy->in.right;
		while (pnew->in.op != QUEST)
			pnew = pnew->in.left;
		/*
		* pnew is top of new tree
		*/

		/* type of resulting ?: will be same as original type of asgop.
		** type of : must be changed, too
		*/
		pnew->in.type = astype;
		pnew->in.right->in.type = astype;

		if ((pwork = p)->in.right->in.op == QUEST)
		{
			tfree(pwork->in.right);
			pwork->in.right = pnew->in.right->in.left;
			pnew->in.right->in.left = pwork;
			/* at this point, 1/2 distributed. Tree looks like:
			*		ASSIGN|ASGOP
			*	LVAL			QUEST
			*		EXPR1		COLON
			*			ASSIGN|ASGOP	EXPR3
			*		LVAL		EXPR2
			* pnew "holds" new tree from QUEST node
			*/
		}
		else
		{
			NODE *pholdtop = pwork;

			pwork = pwork->in.right;
			while (pwork->in.left->in.op != QUEST)
				pwork = pwork->in.left;
			tfree(pwork->in.left);
			pwork->in.left = pnew->in.right->in.left;
			pnew->in.right->in.left = pholdtop;
			/* at this point, 1/2 distributed. Tree looks like:
			*		ASSIGN|ASGOP
			*	LVAL			ANY # OF CONVs
			*			QUEST
			*		EXPR1		COLON
			*			ASSIGN|ASGOP	EXPR3
			*		LVAL		ANY # OF CONVs
			*			EXPR2
			* pnew "holds" new tree from QUEST node
			*/
		}
		if ((pwork = pcpy)->in.right->in.op == QUEST)
		{
			pwork->in.right = pnew->in.right->in.right;
			pnew->in.right->in.right = pwork;
			/*
			* done with the easy case
			*/
		}
		else
		{
			NODE *pholdtop = pwork;

			pwork = pwork->in.right;
			while (pwork->in.left->in.op != QUEST)
				pwork = pwork->in.left;
			pwork->in.left = pnew->in.right->in.right;
			pnew->in.right->in.right = pholdtop;
			/*
			* done with the CONVs case
			*/
		}
		p = pnew;
#ifndef NODBG
		if (xdebug)
		{
			emit_str("Leaving [op]=?: distribution\n");
			e2print(p);
		}
#endif
	}
	return(p);
}

static int	toplab;
static int	botlab;
static int	piclab;
static int	picpc;		/* stack temp for pc */
static int	efoff[TOTREGS];
void
efcode(p)			/* wind up a function */
NODE *p;
{
	extern int strftn;	/* non-zero if function is structure function,
				** contains label number of local static value
				*/
	register int i;
	int stack;

	if (p->in.name)
		memcpy(request, p->in.name, sizeof(request));
	if (picflag)
                request[BASEREG] = 1;

	stack = -(p->tn.lval) * SZCHAR;

	deflab(retlab);

	for (i = REG_EDI; i >= REG_EBX; i--)
		if (request[i]) {
			stack += SZINT;
			efoff[i] = -(stack/SZCHAR);
		}

	stack = -(p->tn.lval) * SZCHAR;
	SETOFF(stack, ALSTACK);

        /* Restore old 386 register variables */
        /* Must be reverse order of the register saves */

	/* %ebx not saved if picflag is set but there was not GOT reference */
        if (request[REG_EBX] && !(picflag && gotflag == NOGOTREF))
                emit_str("\tpopl\t%ebx\n");
        if (request[REG_ESI] || (regstused & RS_BIT(REG_ESI)) )
                emit_str("\tpopl\t%esi\n");
        if (request[REG_EDI] || (regstused & RS_BIT(REG_EDI)) )
                emit_str("\tpopl\t%edi\n");

	fprintf(outfile,"\tleave\n\tret/%d\n",biggest_return);

	/*
	 * The entry sequence according to the 387 spec sheet is always
	 * faster (by 4 cycles!) to do a push/movl/sub rather than an
	 * enter and occasionally a sub also.  Fastest enter is 10 cycles
	 * versus 2/2/2...
	 */
	deflab(botlab);
#ifndef	OLDTMPSRET
	if (strftn)
		emit_str("\tpopl\t%eax\n\txchgl\t%eax,0(%esp)\n");
#endif
	emit_str("\tpushl\t%ebp\n\tmovl\t%esp,%ebp\n");
	if (stack/SZCHAR > 0) {
		if (stack / SZCHAR == 4)
			emit_str("\tpushl\t%eax\n");
		else
			fprintf(outfile, "\tsubl\t$%d,%%esp\n", stack/SZCHAR);
	} 

	/* Save old 386 register variables */
	if (request[REG_EDI] || (regstused & RS_BIT(REG_EDI)) )
		emit_str("\tpushl\t%edi\n");
	if (request[REG_ESI] || (regstused & RS_BIT(REG_ESI)) )
		emit_str("\tpushl\t%esi\n");
	/* %ebx not saved if picflag is set but there was not GOT reference */
        if (request[REG_EBX] && !(picflag && gotflag == NOGOTREF))
		emit_str("\tpushl\t%ebx\n");

	regstused = RS_NONE;
	regvar = RS_NONE;
	if (gotflag != NOGOTREF)
	{
	    fprintf(outfile,"\tcall	.L%d\n", piclab);
	    fprintf(outfile,".L%d:\n", piclab);
	    fprintf(outfile,"\tpopl	%s\n", rnames[BASEREG]);
	    if (gotflag & GOTSWREF)
		fprintf(outfile,"\tmovl    %s,%d(%%ebp)\n", rnames[BASEREG],picpc);
	    fprintf(outfile,"\taddl	$_GLOBAL_OFFSET_TABLE_+[.-.L%d],%s\n",
				piclab, rnames[BASEREG]);
	}
	jmplab(toplab);
}
void
bfcode(p)			/* begin function code. a is array of n stab */
NODE * p;
{
	extern void fp_init();
	retlab = getlab();	/* common return point */
	toplab = getlab();
	botlab = getlab();
	gotflag = NOGOTREF;
	if (picflag) {
		piclab = getlab();
		picpc  = freetemp(1) / SZCHAR;	/* stack temp for pc */
	}
	jmplab(botlab);
	deflab(toplab);
	if (p->in.type == TSTRUCT)
	{
		/*Save place for structure return on stack*/
		strftn = 1;
		fprintf(outfile,"	movl	%s,%d(%%ebp)\n",rnames[AUXREG],str_spot);
	}
	if (proflag)
	{
	        int temp;

		emit_str("/ASM\n");
		emit_str("	.data\n");
		temp = getlab();
		emit_str("	.align	4\n");
		deflab(temp);
		emit_str("	.long	0\n	.text\n");
		if (picflag) {
		    fprintf(outfile,"	leal	.L%d@GOTOFF(%s),%%edx\n",temp, rnames[BASEREG]);
		    fprintf(outfile,"	call	*_mcount@GOT(%s)\n", rnames[BASEREG]);
		    gotflag |= GOTREF;
		}
		else {
		    fprintf(outfile,"	movl	$.L%d,%%edx\n",temp);
		    emit_str("	call	_mcount\n");
		}
		emit_str("/ASMEND\n");
	}

	fp_init();			/* Init the floating point stack */
}

void
begf(p) /*called for BEGF nodes*/
NODE *p;
{
                        /*save the used registers*/
	if (p->in.name)
        	memcpy(request, p->in.name, sizeof(request));
	else
		memset(request, 0, sizeof(request));
	if (picflag)
		request[BASEREG] = 1;

        (void) locctr(PROG);
        strftn = 0;
	biggest_return = 0;
}

void
myret_type(t)
TWORD t;
{
	if (! (t & (TVOID | TFLOAT | TDOUBLE | TLDOUBLE | TFPTR)) )
	{
		if (!biggest_return)
			biggest_return = 1;
	}
}

void		
sw_direct(p, n, range)
register struct sw *p;
int n;
unsigned long range;
{

	register int i;
	register CONSZ j;
	register int dlab, swlab;

	dlab = (p->slab >= 0) ? p->slab : getlab();
	swlab = getlab();
	if (p[1].sval)
		fprintf(outfile,"\tsubl\t$%ld,%%eax\n", p[1].sval);
	fprintf(outfile,"\tcmpl\t$%ld,%%eax\n\tja\t.L%d\n", range, dlab);
	if (picflag) {
	    fprintf(outfile, "\tleal	.L%d@GOTOFF(%s),%%edx\n",swlab, rnames[BASEREG]);
	    fprintf(outfile, "\tmovl	(%%edx,%%eax,4),%%eax\n");
	    fprintf(outfile, "\taddl	%d(%%ebp),%%eax\n", picpc);
	    fprintf(outfile, "\tjmp	*%%eax\n");
	    gotflag |= GOTSWREF;;	
	}
	else
	    fprintf(outfile,"\tjmp\t*.L%d(,%%eax,4)\n", swlab);
	(void) locctr(FORCE_LC(CDATA));
	defalign(ALPOINT);
	emit_str("/SWBEG\n");
	deflab(swlab);
	for (i = 1, j = p[1].sval; i <= n; ++j)
	     if (picflag)
		fprintf(outfile,"	.long	.L%d-.L%d\n",
		    (j == p[i].sval) ? p[i++].slab : dlab, piclab);
	     else
		fprintf(outfile,"	.long	.L%d\n",
		    (j == p[i].sval) ? p[i++].slab : dlab );
	emit_str("/SWEND\n");
	(void) locctr(PROG);
	if (p->slab < 0)
		deflab(dlab);
}

p2done()
{
        char buf[BUFSIZ];
        int m;
	if (tmpfp)
	{
        	fclose(tmpfp);
        	if (!(tmpfp = fopen(tmpfn, "r")))
                	cerror("string file disappeared???");
#ifndef RODATA
		(void) locctr(DATA);
#endif
        	while (m = fread(buf, 1, BUFSIZ, tmpfp))
                	fwrite(buf, 1, m, outfile);
        	(void) unlink(tmpfn);
	}
	return( ferror(outfile) );
}
