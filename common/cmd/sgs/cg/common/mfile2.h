/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:common/mfile2.h	1.28"
 
#ifndef	MACDEFS_H			/* guard against double include */
# include "macdefs.h"
#define	MACDEFS_H
#endif
# include "manifest.h"

/*  code generation goals, used in table entries */

# define FOREFF 01 /* compute for effects only */
# define INREG 02 /* compute into a register */
# define FORCC 04 /* compute for condition codes only */

	/* types of operators and shapes; optab.tyop, optab.ltype optab.rtype */
# define TCHAR		01
# define TUCHAR		0100
# define TSHORT		02
# define TUSHORT	0200
# define TLONG		010
# define TULONG		01000
                        /* Macros to sign/unsign types:  special CG/Nail hack */
#define MKUNSIGN(t) ( (t) << 6 )
#define MKSIGN(t) ( (t) >> 6)

/* make INT match SHORT, LONG, or both depending on NOSHORT, NOLONG */
# define urTINT		04
# define urTUNSIGNED	0400

# ifdef NOSHORT
	/* shorts == ints */
#	ifdef	NOLONG
		/* longs == ints */
#		define TINT		(urTINT|TSHORT|TLONG)
#		define TUNSIGNED	(urTUNSIGNED|TUSHORT|TULONG)
#	else
		/* longs != ints, and shorts == ints */
#		define TINT		(urTINT|TSHORT)
#		define TUNSIGNED	(urTUNSIGNED|TUSHORT)
#	endif
# else
	/* shorts != ints */
#	ifdef NOLONG
		/* longs == ints */
#		define TINT		(urTINT|TLONG)
#		define TUNSIGNED	(urTUNSIGNED|TULONG)
#	else
		/* longs, shorts, and ints all distinct */
#		define TINT		urTINT
#		define TUNSIGNED	urTUNSIGNED
#	endif
# endif

# define urTDOUBLE	040

/* Note that with the advent of TLDOUBLE, types no longer fit in
** 16 bits.  This might reveal some latent problems in existing code.
*/
# define TLDOUBLE	0100000
# define TFLOAT		020
# ifdef ONEFLOAT
#	ifdef NOLDOUBLE
	/* only one floating point size */
#		define TDOUBLE		(urTDOUBLE|TFLOAT|TLDOUBLE)
# 	else
#		define TDOUBLE		(urTDOUBLE|TFLOAT)
#	endif
# else
#	ifdef NOLDOUBLE	
#		define TDOUBLE		(urTDOUBLE|TLDOUBLE)
#	else
#		define TDOUBLE		urTDOUBLE
#	endif
# endif

# define TSTRUCT      02000   /* structure or union */
# define TPOINT	      04000
# define TPOINT2      010000
# define TVOID	      020000
# define TFPTR	      040000	/* CG frame pointers */
# define TANY	      0177777  /* matches anything within reason */
	/* 0200000 (SPTYPE) is reserved for matches */

	/* reclamation cookies */

# define RLEFT 01
# define RRIGHT 02
# define RESC1 04
# define RESC2 010
# define RESC3 020
# define RESCC 040
# define RNOP 0100   /* DANGER: can cause loops.. */
# define RNULL 0200    /* clobber result */
# define REITHER 0400	/* take result from L or R, whichever is "better" */

	/* needs */

# define NREG 01
# define NCOUNT 017
# define NMASK 0777
# define LSHARE 020 /* share left register */
# define RSHARE 040 /* share right register */
# define NPAIR 0100 /* allocate registers in pairs */
# define LPREF 0200 /* prefer left register, if any */
# define RPREF 0400 /* prefer right register, if any */

/* These bits are set by sty to force exact match of sub-trees.
** Note that they are not included under NMASK
*/

# define LMATCH 010000        /* left sub-tree must match exactly */
# define RMATCH 020000        /* right sub-tree must match exactly */
# define NO_IGNORE 01000	/*This template cannot ignore exceptions*/
# define NO_HONOR 02000		/*This template cannot honor exceptions*/
 
	/* register allocation */

extern int busy[];

# define INFINITY 10000

typedef struct shape SHAPE;

	/* special shapes (enforced by making costs high) */
# define SPECIAL	0270000
# define SPTYPE		0200000
# define STMASK 	 070000
# define SVAL		 010000	/* positive constant value */
# define SNVAL 		 020000	/* negative constant value */
# define SRANGE0 	 030000 /* positive range [0, 2**mm - 1] */
# define SSRANGE 	 040000	/* signed range [-2**mm, 2**mm] */
# define NACON		 050000	/* nameless contant */
# define SUSER		 070000	/* user's cost function */
# define SVMASK		  07777	/* shape value mask */

struct shape {
	int	op;		/* operator */
	SHAPE *sl;	/* pointers to left- and right-hand shape */
	SHAPE *sr;
	int sh;			/* flags for special shape and type matches */
	RST sregset;		/* register set for this shape (REG shape only) */
};

typedef struct optab OPTAB;

typedef SHAPE *SH_PTR;
typedef SH_PTR *SH_PTR_PTR;

struct optab {
	int	op;	/* operator */
	int	tyop;	/* type of the operator node itself */
	OPTAB	*nextop;
	SH_PTR_PTR lshape;	/* pointer to pointer to left shape */
	int	ltype;		/* type of left shape */
	SH_PTR_PTR rshape;	/* likewise for the right shape */
	int	rtype;
	int	needs;
	short	rneeds;		/* offset into rsbits[] */
	int	rewrite;
	char	*cstring;
	int	stinline;	/* line number in stin file */
};

extern OPTAB
	*match(),
	* const ophead[];

extern NODE resc[];

extern int tmpoff;
extern int maxboff;
extern int maxtemp;
extern int maxarg;
extern int ftnno;
extern int sideff;

extern NODE
	*talloc(),
/* special hack for Nifty/C++, which uses tcopy():  check arg type */
#ifndef	NODEPTR
#define	NODEPTR
#endif
	*tcopy(NODEPTR),
	*getadr(),
	*getlr();

extern int
	argsize(),
	freetemp(),
	iseff(),
	lhsok(),
	rewass(),
	rewsto(),
	rs_rnum(),
	tnumbers(),
	special(),
	pre_ex(),
	p2nail(),
	semilp(),
	rewsemi(),
	ushare();

extern RST rs_reclaim();

extern void
	allo0(),
	allo(),
	allchk(),
	codgen(),
	expand(),
	fcons(),
	insprt(),
	mkdope(),
	qsort(),
	reallo(),
	reclaim(),
	rewcom(),
	reweop(),
	regrcl(),
	tinit(),
	unorder(),
	commute(),
	typecheck(),
	uncomma(),
#ifdef  MYRET_TYPE
	myret_type(),
#endif
	exit(),
	e2print(),
	e22print(),
	e222print();

# define getlt(p,t) ((t)==LTYPE?p:p->in.left)
# define getlo(p,o) (getlt(p,optype(o)))
# define getl(p) (getlo(p,p->tn.op))
# define getrt(p,t) ((t)==BITYPE?p->in.right:p)
# define getro(p,o) (getrt(p,optype(o)))
# define getr(p) (getro(p,p->tn.op))

extern char *rnames[];

extern int lineno;
extern char ftitle[];
extern int fldshf, fldsz;
extern int fast;  /* try to make the compiler run faster */
extern int lflag, udebug, e2debug, odebug, rdebug, /*radebug,*/ sdebug;

#ifndef callchk
#define callchk(x) allchk()
#endif
#ifndef callreg
	/* try to number so results returned in 0 */
#define callreg(x) 0
#endif
#ifndef szty
	/* it would be nice if number of registers to hold type t were 1 */
	/* on most machines, this will be overridden in macdefs.h */
# define szty(t) 1
#endif


#ifndef PUTCHAR
# define PUTCHAR(x) putchar(x)
#endif

# define CEFF (NRGS+1)
# define CTEMP (NRGS+2)
# define CCC (NRGS+3)
	/* the assumption is that registers 0 through NRGS-1 are scratch */
	/* the others are real */
	/* some are used for args and temps, etc... */

# define istreg(r) ((r)<NRGS)

typedef struct st_inst INST;

struct st_inst {
	NODE	*p;
	OPTAB	*q;
	int	goal;
	/* set of registers where result is desired to be */
	RST	rs_want;
	/* Set of available registers:  cfix() may need this to
	** choose scratch registers.
	*/
	RST	rs_avail;
};

#undef NINS
extern struct td td_inst;
#define inst ((INST *)(td_inst.td_start))
#define nins (td_inst.td_used)
#define NINS (td_inst.td_allo)


	/* definitions of strategies legal for ops */

# define STORE 01
# define LTOR 02
# define RTOL 04
# define EITHER (LTOR|RTOL)

#define CONTAINS_CALL 01/* function contains call */
# define PAREN 010	/* do entire op at once*/
# define COPYOK 040	/* on LET node: OK to copy CSE's */
# define EXHONOR 0100	/* Must honor numeric exceptions*/
# define EXIGNORE 0200	/* Must ignore numeric exceptions*/
# define DOEXACT 0400	/* Must honor exact semantics (e.g. volatile)*/
# define WASCSE 01000	/* On TEMPS -- this node used to be a CSE */
# define OCOPY 02000	/* ordered copy: 1 = must make a copy of this value*/
# define VOLATILE 04000	/* treat node according to volatile conditions */

# define PIC_GOT 010000 /* for PIC "global offset table" flag */
# define PIC_PLT 020000 /* for PIC "procedure linkage table" flag */
# define PIC_PC  040000 /* for PIC "program counter" flag */
# define FULL_OPT 0100000 /* fully optimizable asm */
# define PART_OPT 0200000 /* partially optimizable asm */

#define VOL_OPND1 01	/* operands which are volatile objects */
#define VOL_OPND2 02
#define VOL_OPND3 04
#define VOL_OPND4 010

#define RS_FAIL	(RS_BIT(TOTREGS)) /* special flag for match fail */
#ifndef RS_FORTYPE
#define RS_FORTYPE(type) (RS_NRGS) /* any scratch reg for any type */
#endif
/* flags for insout/bprt */
#define REWROTE (RS_FAIL + 1)	/* tree rewritten */
#define OUTOFREG (RS_FAIL + 2)	/* ran out of regs, couldn't gen. code */
extern RST insout();
#define INSOUT(p,g) insout(p,g,NRGS,RS_NRGS,g==NRGS?RS_NRGS:RS_NONE)
					/* add reg.set. as avail. */
/* definitions for templates */
extern RST rsbits[];		/* array of register set bits */
#define RSB(n)	(&rsbits[(n)])	/* address of bit vector n */
#ifndef RS_MOVE			/* default is cerror() */
#define RS_MOVE(p,q) cerror("RS_MOVE called!!")
#endif

#ifndef	NODBG
extern OPTAB const table[];
extern SH_PTR const pshape[];
extern SHAPE const shapes[];
#endif

/* definitions of INSOUT() for use at "top-level", when all registers
** are available.
*/

        /*flags for CG DEFNAM node*/
# define EXTNAM 01
# define FUNCT  02
# define COMMON 04
# define DEFINE 010
# define DEFNOI 020
# define ICOMMON 040
# define LCOMMON 0100


/*Structure for common subexpressions*/
#define MAXCSE 100	/*max # of cse's*/
struct cse {
	int id;	/*number for this CSE*/
	int reg;/*reg that holds the cse*/
};

extern struct cse cse_list[MAXCSE];
extern struct cse *cse_ptr;
struct cse *getcse();

#ifdef	IN_LINE
/* Declaration for expanding asm's */
extern void as_gencode();
#endif

/* Types - T2_types make available to the front
 * end for using either RCC or PCC back ends.
*/
#define T2_CHAR	TCHAR
#define T2_SHORT	TSHORT
#define T2_INT		TINT
#define T2_LONG	TLONG
#define T2_FLOAT	TFLOAT
#define T2_DOUBLE	TDOUBLE
#define T2_LDOUBLE	TLDOUBLE
#define T2_UCHAR	TUCHAR
#define T2_USHORT	TUSHORT
#define T2_UNSIGNED	TUNSIGNED
#define T2_ULONG	TULONG
#define T2_STRUCT	TSTRUCT
#define T2_VOID	TVOID

#define T2_ADDPTR(t)	TPOINT
#define T2_ADDFTN(t)	(t)
#define T2_ADDARY(t)	(t)

/* parameter flags */
#define REGPARAM	01
#define VARPARAM	02
#define VOLPARAM	04

/* name node information */
#define NI_FUNCT	01
#define NI_OBJCT	02
#define NI_GLOBAL	04
#define NI_FLSTAT	010
#define NI_BKSTAT	020

#ifndef FORCE_LC
#define	FORCE_LC(lc)	(lc)
#endif
