/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmalloc:common/mallint.h	1.1.1.1"

#include "machdep.h"

/*
	template for a small block
*/
struct lblk  {
	union {
		struct lblk *nextfree;  /* the next free little block in this
					   holding block.  This field is used
					   when the block is free */
		struct holdblk *holder; /* the holding block containing this
					   little block.  This field is used
					   when the block is allocated */
	}  header;
	char byte;		    /* There is no telling how big this
					   field freally is.  */
};

/*
	The following manipulate the free queue

		DELFREEQ will remove x from the free queue
		ADDFREEQ will add an element to the head
			 of the free queue.
		MOVEHEAD will move the free pointers so that
			 x is at the front of the queue
*/
#define ADDFREEQ(x)       (x)->prevfree = &(freeptr[0]);\
				(x)->nextfree = freeptr[0].nextfree;\
				freeptr[0].nextfree->prevfree = (x);\
				freeptr[0].nextfree = (x);\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
#define DELFREEQ(x)       (x)->prevfree->nextfree = (x)->nextfree;\
				(x)->nextfree->prevfree = (x)->prevfree;\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
#define MOVEHEAD(x)       freeptr[1].prevfree->nextfree = \
					freeptr[0].nextfree;\
				freeptr[0].nextfree->prevfree = \
					freeptr[1].prevfree;\
				(x)->prevfree->nextfree = &(freeptr[1]);\
				freeptr[1].prevfree = (x)->prevfree;\
				(x)->prevfree = &(freeptr[0]);\
				freeptr[0].nextfree = (x);\
				assert((x)->nextfree != (x));\
				assert((x)->prevfree != (x));
/*
	The following manipulate the busy flag
*/
#define BUSY	1
#define SETBUSY(x)      ((struct header *)((int)(x) | BUSY))
#define CLRBUSY(x)      ((struct header *)((int)(x) & ~BUSY))
#define TESTBUSY(x)     ((int)(x) & BUSY)
/*
	The following manipulate the small block flag
*/
#define SMAL	2
#define SETSMAL(x)      ((struct lblk *)((int)(x) | SMAL))
#define CLRSMAL(x)      ((struct lblk *)((int)(x) & ~SMAL))
#define TESTSMAL(x)     ((int)(x) & SMAL)
/*
	The following manipulate both flags.  They must be 
	type coerced
*/
#define SETALL(x)       ((int)(x) | (SMAL | BUSY))
#define CLRALL(x)       ((int)(x) & ~(SMAL | BUSY))
/*
	Other useful constants
*/
#define TRUE    1
#define FALSE   0
#define BLOCKSZ 2048		/* memory is gotten from sbrk in
				   multiples of BLOCKSZ */
#define GROUND  (struct header *)0
#define LGROUND (struct lblk *)0	/* ground for a queue within a holding
					   block	*/
#define HGROUND (struct holdblk *)0     /* ground for the holding block queue */
/*
	number of bytes to align to  (must be at least 4, because lower 2 bits
	are used for flags.
	See machdep.h for different ALIGNSZ (if there is)
*/
#ifndef ALIGNSZ
#define ALIGNSZ 4
#endif
#define HOLDHEADSZ  ( (sizeof(struct holdblk) % ALIGNSZ == 0) ? sizeof(struct holdblk)\
		: (ALIGNSZ % sizeof(struct holdblk) == 0) ? ALIGNSZ\
		: sizeof(struct holdblk) * ALIGNSZ )

#define HEADSZ  ( (sizeof(struct header) % ALIGNSZ == 0) ? sizeof(struct header)\
		: (ALIGNSZ % sizeof(struct header) == 0) ? ALIGNSZ\
		: sizeof(struct header) * ALIGNSZ )

/*
 	min. block size must as big as HEADSZ
*/
#define MINBLKSZ ( (HEADSZ > 12) ? HEADSZ : 12 )
/*
	MINHEAD is the minimum size of an allocated block header.
	See machdep.h for different MINHEAD (if there is)
*/
#ifndef MINHEAD

#define HOLDMINHEAD ( (sizeof(struct holdblk) % ALIGNSZ == 0)\
		? sizeof(struct holdblk)\
		: (ALIGNSZ % sizeof(struct holdblk ) == 0) ? ALIGNSZ\
		: sizeof(struct holdblk) * ALIGNSZ )

#define MINHEAD ( (sizeof(struct header *) % ALIGNSZ == 0) ? sizeof(struct header *)\
		: (ALIGNSZ % sizeof(struct header *) == 0) ? ALIGNSZ\
		: sizeof(struct header *) * ALIGNSZ )
#endif
#ifndef NULL
#define NULL    (char *)0
#endif
/*
	Structures and constants describing the holding blocks
*/
#define NUMLBLKS  100   /* default number of small blocks per holding block */
/* size of a holding block with small blocks of size blksz */
#define HOLDSZ(blksz)  (HOLDMINHEAD + blksz*numlblks)
#define FASTCT    6	  /* default number of block sizes that can be
				   allocated quickly */
#define MAXFAST   ALIGNSZ*FASTCT  /* default maximum size block for fast
				     allocation */


#ifdef debug
#define CHECKQ  checkq();
static
checkq()
{
	register struct header *p;

	p = &(freeptr[0]);
	/* check forward */
	while(p != &(freeptr[1]))       {
		p = p->nextfree;
		assert(p->prevfree->nextfree == p);
	}
	/* check backward */
	while(p != &(freeptr[0]))       {
		p = p->prevfree;
		assert(p->nextfree->prevfree == p);
	}
}
#else
#define CHECKQ
#endif

