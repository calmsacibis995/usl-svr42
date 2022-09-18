/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:common/optim.h	1.29"

/*	machine independent include file for code improver */

#include <stdio.h>
#include "defs.h"
extern int atoi(); /* from <stdlib.h> */

#ifndef MAXOPS
#define MAXOPS	1
#endif

/* booleans */

typedef int boolean;
#define false	0
#define true	1

/* predefined "opcodes" for various nodes */

#define GHOST	0		/* temporary, to prevent linking node in */
#define TAIL	0		/* end of text list */
#define MISC	1		/* miscellaneous instruction */
#define FILTER	2		/* nodes to be filtered before optim */

#ifdef LIVEDEAD
extern void ldanal();
#if LIVEDEAD - 0 < 2
#undef LIVEDEAD
#define LIVEDEAD	16
#endif
#endif

typedef struct sdependent {           /*linked list of dependent*/
	struct sdependent *next;      /*nodes, usefull for      */
	struct node *depends;         /*schedule.               */
	} tdependent;

/* structure of each text node */

typedef struct node {
	struct node *forw;	/* forward link */
	struct node *back;	/* backward link */
	tdependent  *dependent; /* dependent instructions */
	tdependent  *CCdep; 
	tdependent  *anti_dep; 
	tdependent  *agi_dep; 
	tdependent  *may_dep; 
	unsigned int sets;
	unsigned int uses;
	unsigned int idxs;
	unsigned int idus;
	int         dependents; /* # of dependent sons */
	int         chain_length; /* longest depending path */
	int         usage;               /*kind of CC usage, if any*/
	int         nparents; /* on how many instructions it depends*/
#ifdef P5
	int         pairtype;  
#endif
	int			zero_op1;
	int			ebp_offset;
	int 		esp_offset;
	char *ops[MAXOPS + 2];	/* opcode or label and operand field strings */
#ifdef IDVAL
	IDTYPE uniqid;		/* unique identification for this node */
#endif
	unsigned short op;	/* operation code */
#ifdef LIVEDEAD
	unsigned
	    nlive:LIVEDEAD,	/* registers used by instruction */
	    ndead:LIVEDEAD;	/* registers set but not used by instruction */
	unsigned
	    nrlive:LIVEDEAD,	/* registers used by instruction */
	    nrdead:LIVEDEAD;	/* registers set but not used by instruction */
#endif
#ifdef USERDATA
	USERTYPE userdata;	/* user-defined data for this node */
#endif
	int extra;		/* used in enter-leave removal, if non-negative
				    contains the stack size on execution for this instruction */
	int extra2;    /*used together with extra to count changes in index
					register values. */
	int sasm;		/* is instruction a safe asm */
} NODE;


/* values for the extra field above */
#define REMOVE -1
#define NO_REMOVE -2
#define TMPSRET -3

#define opcode	ops[0]
#define op1	ops[1]
#if MAXOPS > 1
#define op2	ops[2]
#if MAXOPS > 2
#define op3	ops[3]
#if MAXOPS > 3
#define op4	ops[4]
/* #if MAXOPS > 4 */
#define op5	ops[5]
/* #if MAXOPS > 5 */
#define op6	ops[6]
#endif
#endif
#endif
/* #endif */
/* #endif */

/* block of text */

typedef struct block {
	struct block *next;	/* pointer to textually next block */
	struct block *prev;	/* pointer to textually previous block */
	struct block *nextl;	/* pointer to next executed block if no br */
	struct block *nextr;	/* pointer to next executed block if br */
	struct block *ltest;	/* for loop termination tests */
	NODE *firstn;		/* first text node of block */
	NODE *lastn;		/* last text node of block */
	short index;		/* block index for debugging purposes */
	short length;		/* number of instructions in block */
	short indeg;		/* number of text references */
	short marked;		/* marker for various things */
	int	entry_depth;
	int exit_depth;
} BLOCK;

/* structure of non-branch text reference node */

typedef struct ref {
	char *lab;		/* label referenced */
	struct ref *nextref;	/* link to next ref */
	char *switch_table_name;
	BLOCK *switch_entry;
} REF;

typedef struct switch_tbl {
	REF *first_ref;	/* point to first label of switch in the ref list */
	REF *last_ref;	/* point to last label of switch in the ref list */
	char *switch_table_name; 
	struct switch_tbl *next_switch;
} SWITCH_TBL;

/* externals */

extern NODE n0;			/* header node of text list */
extern NODE ntail;		/* trailer node of text list */
extern NODE *lastnode;		/* pointer to last node on text list */
extern BLOCK b0;
extern REF r0;			/* header node of reference list */
extern REF *lastref;		/* pointer to last label reference */
extern SWITCH_TBL sw0;  /* header of switch table list */
extern int pic_flag;
extern int ieee_flag;
extern int dflag;		/* display live-dead info */
#ifdef STATS
extern int ndisc;		/* # of instructions discarded */
#endif
extern int ninst;		/* total # of instructions */

extern NODE *Saveop();
extern boolean same(), sameaddr();
extern char *getspace(), *xalloc();
extern void addref(), fatal(), init(), optim(), prtext(), xfree();
extern void start_switch_tbl(), end_switch_tbl();
extern void bldgr();

/* user-supplied functions or macros */

#ifndef getp
extern char *getp();
#endif
#ifndef newlab
extern char *newlab();
#endif

#define saveop(opn, str, len, op) \
	    (void) Saveop((opn), (str), (unsigned)(len), (unsigned short)(op))
#define addtail(p)		/* superfluous */
#define appinst()		/* superfluous */
#define appmisc(str, len)	saveop(0, (str), (len), MISC)
#define appfl(str, len)		saveop(0, (str), (len), FILTER)
#define applbl(str, len) \
	(setlab(Saveop(0, (str), (unsigned)(len),MISC)), --ninst)
#define ALLN(p)			p = n0.forw; p != &ntail; p = p->forw
#define PRINTF			(void) printf
#define FPRINTF			(void) fprintf
#define SPRINTF			(void) sprintf
#define PUTCHAR(c)		(void) putchar(c)
#define DELNODE(p)		((p)->back->forw = (p)->forw, \
				    (p)->forw->back = (p)->back)
#define APPNODE(p, q)		PUTNODE((p), (q), back, forw)
#define INSNODE(p, q)		PUTNODE((p), (q), forw, back)
#define PUTNODE(p, q, f, b)	((p)->f = (q), (p)->b = (q)->b, \
				    (q)->b = (q)->b->f = (p))
#define GETSTR(type)		(type *) getspace(sizeof(type))
#define COPY(str, len)	((len) != 0 ? \
	(char *)memcpy(getspace(len), str, (int)(len)) : str)
