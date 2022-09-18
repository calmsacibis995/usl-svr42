/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/sched.h	1.3"
#include "optim.h"
#include "database.h"
#define NONE     0    /*three types of CC usage */
#define READ     1
#define WRITE    2

#define	ALONE	5    /*three ways an instruction can go*/
#define ON_X	6
#define ON_J	7

#define ANTI   	1  /* five types of dependency  */
#define MAY		2
#define DEP		3
#define AGI		4
#define CCDEP	5
#define LOST_CYCLE 6
#define different	0 /*three possible relations between memories*/
#define same		1
#define unsafe		2

#define NREGS	 9    /* eight registers and CC */
/*#define NFACTS	5     /* Number of columns in database */
#define ASIZE	1000  /* Size of memory allocate module array */

#define ByTE	1  /* lengths of instructions    */
#define WoRD	2
#define LoNG	4
#define DoBL	8
#define TEN		10
#define GREAT	28

#define tmalloc(type) (type *) malloc(sizeof(type))
#define isprefix(p)   (p->op == REP || p->op == REPZ || p->op == REPNZ\
                       || p->op == LOCK || p->op == ESC)
#define ismovs(p)	(p->op == SMOVB || p->op == SMOVW || p->op ==\
				  SMOVL)
#define ispp(p)  (p->op == PUSHL || p->op == PUSHW || p->op == POPL\
				 || p->op == POPW)
#define isshift(p)	((p->op) >= RCLB && (p->op) <= SHRL)
#define hasdisplacement(op) ((op) && (*op) != '$' && (*op) != '%'\
								  && (*op) != '(' )
#define isior(op) ((op) != NULL && ((*op) == '$' || (*op) == '%'))
#define absdiff(x,y) (x < y ? y-x : x-y)
#define max(x,y) (x < y ? y : x )
#define ismem(op)	(((op) && (*op) && (*op) != '%' && (*op) != '$' && \
					op[2] != '#') ? MEM : 0)
#define touchMEM(p)		((p->uses | p->sets) & MEM)


typedef struct clm {       /* a candidate list element */
        struct clm *next;
    	struct clm *back;
        NODE *member;
	    } clistmem;

typedef struct xlist { /* linked list of arrays for memory allocation*/
		struct xlist *next;
		tdependent   *a;
		} alist;

extern unsigned int muses(), msets();
extern int get_exe_time();
