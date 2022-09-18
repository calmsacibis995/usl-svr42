/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)wksh:xksrc/dbmalloc.c	1.1"
/*	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T    */
/*	and UNIX System Laboratories, Inc.			*/
/*	The copyright notice above does not evidence any       */
/*	actual or intended publication of such source code.    */

/**********************************************************************
	Memory management: malloc(), realloc(), free(), M_brksize.
	M_brksize: if > 0 is the maximum amount that the bottom free
		block can grow to. If not SEGMENT, GETCORE() will
		be used to return free bottom space to the OS.

	The following #define-parameters may be redefined:
	BSD:	if defined, bcopy() is used instead of memcpy() when
		data is relocated.
	SEGMENT: if defined, memory requests are assumed to be
		non-contiguous across calls of GETCORE's. SEGMENT defines
		the size of each GETCORE request.
	CORESIZE: min number of bytes to used with GETCORE.
		On a SEGMENTed machine, this should be defined
		as the size of a segment. Default is 8192.
	GETCORE: a function to get more core memory. If not SEGMENT,
		GETCORE(0) is assumed to return the next available
		address. Default is 'sbrk'.
	ERRCORE: the error code as returned by GETCORE.
		Default is ((char*)(-1)).
	MTRACE:	if defined, code is included to trace and certify that
		malloc-ed blocks are not corrupted. Available functions
		and variables are:
		int (*Mt_corrupt)(addr,size,stamp)
			The function called when a corrupted block is detected.
				addr: address of the corrupted block
				size: size of the block.
				stamp: a user/program-defined stamp (below).
			If Mt_corrupt is 0, a default function is used.
		Mt_nfree: number of free calls so far
		Mt_nmalloc: number of malloc calls so far
		Mt_nrealloc: number of realloc calls so far
		Mt_free: if not zero, free block data will be trashed immediately.
		Mt_certify: if not 0, indicates that the arena should
			be automatically certified on each call to malloc,
			realloc, or free. Default is 0.
		Mt_trace: if >= 0, is the file descriptor to write out
			a trace of all calls to malloc, free, realloc.
			Default is -1.
		Mt_stamp: default stamp for a malloc-ed block. default is NULL.
		mt_certify()
			Check all blocks to see if they are ok. If a block is
			corrupted, (*corrupt)() is called as above.
		mt_stat(int fd)
			Print arena statistics.
		mt_chkaddr(VOID *addr)
			Check to see if addr points to a block allocated
			by malloc. This function returns 0 if addr points to
			a legal malloc block, 1 if it points to a heap address
			but not a block, 2 if it points to an address below
			heap address, 3 if it points to an address higher than
			any known heap address (stack space?).

	With minor variations, the basic allocation strategy is best-fit.
	Lists of same size free blocks are kept in a splay tree.
	For results on splay trees, see:
		Self-Adjusting Binary Trees,
		DD Sleator & RE Tarjan, JACM 1985.

	The header of a block contains the size in bytes of the data part.
	Since the size of a block is 0%4, the low two bits of the header
	are free and used as follows:
		BIT0:	1 for busy (block is in use), 0 for free.
		BIT1:	if the block is busy, this bit is 1 if the
			preceding block in contiguous memory is free.
			Otherwise, it is always 0.

	Kiem-Phong Vo, AT&T Bell Laboratories
**********************************************************************/

int	M_brksize;	/* the max size for Bottom */

/* debugging macros */
#ifdef	DEBUG
#define VOID		unsigned char
#define	ASSERT(p)	((void) ((p) ? 0 : (abort(),0)))
#else
#define VOID		char
#define	ASSERT(p)
#endif /*DEBUG*/

/* system call to get more core */
#if SEGMENT > 0
#ifdef CORESIZE		/* might as well do it with segment size */
#undef CORESIZE
#endif
#define CORESIZE	SEGMENT
#else
#define CORESIZE	(8192)
#endif
#define GETCORE		sbrk
#define ERRCORE		((VOID*)(-1))
extern VOID	*GETCORE(), *malloc(), *realloc();
extern void	free();

/* function to copy data from one area to another */
#ifdef BSD
#define memcpy(to,fr,n)	bcopy(fr,to,n)
#endif

/* for conveniences */
#define reg		register
#define uchar		unsigned char
#define uint		unsigned int
#define NIL(p)		((p)0)
#define MINSIZE		((int)(sizeof(TREE)-sizeof(WORD)))
#define ROUND(x,y)	((((x)+((y)-1))/(y))*(y))

/* compute a suitable alignment for all types */
typedef union _u_
{	int	i;
	VOID*	s;
	double	d;
	VOID	big[1024];
} _ua_;
typedef struct _s_
{	VOID	c;
	_ua_	u;
} _sa_;
#define ALIGN		((int)(sizeof(_sa_) - sizeof(_ua_)))
#define WORDSIZE        ROUND((int)ALIGN,4)

typedef union _w_
{	/* the proto-word */
	uint		w_i;		/* an int */
	struct _t_	*w_p;		/* a pointer */
	VOID		w_a[WORDSIZE];	/* to force alignment */
} WORD;

typedef struct _t_
{	/* structure of a node in the free tree */
	WORD	t_s;	/* size of this element */
	WORD	t_n;	/* next in link list */
	WORD	t_l;	/* left child */
	WORD	t_r;	/* right child */
	WORD	t_d;	/* dummy to reserve space for self-pointer */
} TREE;

/* usable # of bytes in the block */
#define SIZE(b)		(((b)->t_s).w_i)

/* free tree pointers */
#define LEFT(b)		(((b)->t_l).w_p)
#define RIGHT(b)	(((b)->t_r).w_p)

/* links in linked lists */
#define LINK(b)		(((b)->t_n).w_p)
#define BACK(b)		(((b)->t_l).w_p)

/* set/test indicator to see if a block is in the tree or in a list */
#define SETNOTREE(b)	(RIGHT(b) = (TREE*)(~0))
#define ISNOTREE(b)	(RIGHT(b) == (TREE*)(~0))

/* functions to get information on a block */
#define DATA(b)		(((VOID*) (b)) + WORDSIZE)
#define BLOCK(d)	((TREE*) ((d) - WORDSIZE))
#define SELFP(b)	((TREE**) (((VOID*) (b)) + SIZE(b)))
#define LAST(b)		(*((TREE**) (((VOID*) (b)) - WORDSIZE)))
#define NEXT(b)		((TREE*) (((VOID*) (b)) + SIZE(b) + WORDSIZE))
#define BOTTOM(b)	((DATA(b)+SIZE(b)+WORDSIZE) == Baddr)

/* functions to set and test the lowest two bits of a word */
#define	BIT0		(01)	/* ....01 */
#define BIT1		(02)	/* ...010 */
#define BITS01		(03)	/* ...011 */
#define ISBIT0(w)	((w) & BIT0)
#define ISBIT1(w)	((w) & BIT1)
#define	SETBIT0(w)	((w) |= BIT0)
#define SETBIT1(w)	((w) |= BIT1)
#define CLRBIT0(w)	((w) &= ~BIT0)
#define CLRBIT1(w)	((w) &= ~BIT1)
#define ISBITS01(w)	((w) & BITS01)
#define SETBITS01(w)	((w) |= BITS01)
#define CLRBITS01(w)	((w) &= ~BITS01)
#define CPYBITS01(w,f)	((w) |= (f)&BITS01)

static TREE	*Root,		/* root of the free tree */
		*Bottom,	/* the last free chunk in the arena */
		*morecore(),	/* function to get more core */
		*t_search();	/* to look for a free block */

static VOID	*Baddr,		/* current high address of the arena */
		*Lfree,		/* last free block with data intact */
		*Nofree;	/* this block is not to be freed */

/* lists of small blocks */
#define LGET	16
static TREE	*List[(MINSIZE-WORDSIZE)/WORDSIZE];

/* circular queue of delayed free blocks */
#define QSIZE	(1<<8)
#define QMASK	(QSIZE-1)
static VOID	**Qfree;
static int	Qhead = -1, Qtail = 0;
#define CYCLE(n)	(n = (n+1)&QMASK)
#define REWIND(n)	(n = (n+QMASK)&QMASK)
#define QVOID()		(Qhead < 0)
#define QFULL()		(Qtail == Qhead)
#define QENDADD()	(Qhead < 0 ? (Qhead = 0) : 0)
#define QENDDEL()	(Qtail == Qhead ? (Qhead = -1, Qtail = 0) : 0)
#define ENQUEUE(x)	((Qfree[Qtail] = (x)), CYCLE(Qtail), QENDADD())
#define DEQUEUE(x)	(((x) = Qfree[Qhead]), CYCLE(Qhead), QENDDEL())
#define DESTACK(x)	(REWIND(Qtail), ((x) = Qfree[Qtail]), QENDDEL())

/*
**	Coalesce adjacent blocks and free left-over memory.
*/
#define UNLINK(u)	{ if(LINK(u)) \
				BACK(LINK(u)) = BACK(u); \
			  LINK(BACK(u)) = LINK(u); \
			}
#define FMERGE(t,n)	{ n = NEXT(t); \
			  if(!ISBIT0(SIZE(n))) \
			  {	if(n != Bottom) \
				{	if(!ISNOTREE(n)) \
						t_search(SIZE(n)); \
					else	UNLINK(n); \
				} \
				else	Bottom = NIL(TREE*); \
				SIZE(t) += SIZE(n)+WORDSIZE; \
				CLRBIT1(SIZE(NEXT(t))); \
				/**/ ASSERT(ISBIT0(SIZE(NEXT(t)))); \
			  } \
			}
#define BMERGE(t,n)	{ n = LAST(t); \
			  if(!ISNOTREE(n)) \
				t_search(SIZE(n)); \
			  else	UNLINK(n); \
			  SIZE(n) += SIZE(t)+WORDSIZE; \
			  t = n; \
			}
#define FREE(f)		{ if(BOTTOM(f)) \
				Bottom = f; \
			  else \
			  {	SETBIT0(SIZE(f)); /**/ ASSERT(!Lfree); \
				Lfree = DATA(f); \
			  } \
			}
/*
/*
** Trace malloc/free patterns.
*/
#ifdef MTRACE
#include	<errno.h>
#define MTFREE		1
#define MTMALLOC	2
#define MTREALLOC	3
#define MTRENEW		4
#define MTBUSY		5
#define MTIDLE		6
#define MAGIC		0333	/* 11011011 pattern */
#define FSTAMP		((VOID*)((~MAGIC << 8) | MAGIC))
#define MTSPACE		(3*WORDSIZE)
#define MTSIZE(b)	(SIZE(b)&(~BITS01))
#define MTNEXT(b)	((TREE*)(DATA(b) + MTSIZE(b)))
#define USIZE(b)	(*((uint*)(DATA(b)+MTSIZE(b)-(2*WORDSIZE))))
#define USTAMP(b)	(*((VOID**)(DATA(b)+MTSIZE(b)-WORDSIZE)))
#define MTSETFREE(b)	(USTAMP(b) = FSTAMP)
#define MTISFREE(b)	(!ISBITS01(SIZE(b)) || USTAMP(b) == FSTAMP)
#define SMAGIC(b)	((uchar*)(DATA(b)+USIZE(b)))
#define EMAGIC(b)	((uchar*)(DATA(b)+MTSIZE(b)-(2*WORDSIZE)))

static VOID	*Laddr;		/* low address of the arena */
static int	Mt_local = 0;	/* no trace output */
char	*Mt_stamp;		/* stamp for a newly created block */
int	(*Mt_corrupt)();	/* function to process corrupted blocks */
int	Mt_certify;		/* automatically certify the arena */
int	Mt_free;		/* trash free data right away */
int	Mt_trace = -1;		/* print trace of mallocs and frees */
int	Mt_nmalloc = 0;		/* number of mallocs */
int	Mt_nfree = 0;		/* number of frees */
int	Mt_nrealloc = 0;	/* number of reallocs */

static char *conv = "0123456789ABCDEF";
/* Convert an int to a string */
static itoa(i,buf)
reg uint	i;
reg char	*buf;
{
	reg int	k, c;

	buf[0] = '0';
	buf[1] = 'x';
	buf += 2;
	k = 0;
	do
	{	buf[k++] = conv[i%16];
		i /= 16;
	}	while(i != 0);
	buf[k] = '\0';
	for(i = 0; i < k/2; ++i)
	{	c = buf[i];
		buf[i] = buf[(k-i)-1];
		buf[(k-i)-1] = c;
	}
}

/* write out trace data - this handles interrupted write() calls */
static mt_write(fd,s,n)
reg int		fd;
reg char	*s;
reg int		n;
{	reg int		w;
	extern int	errno;
	while(n > 0)
	{	if((w = write(fd,s,n)) > 0)
			{n -= w; s += w;}
		else if(errno == EINTR)
			errno = 0;
		else	break;
	}
}

/* trash data for free blocks */
static mt_setfree(bp)
reg TREE	*bp;
{
	reg VOID	*s, *e;

	MTSETFREE(bp);
	if(Mt_free)
		for(s = DATA(bp), e = s+USIZE(bp); s < e; ++s)
			*s = MAGIC;
}

/* internal function for warning on corruption */
static int mt_corrupt(addr,usize,stamp)
reg VOID	*addr;
reg uint	usize;
reg VOID	*stamp;
{
	char	buf[64], *mesg;

	mesg = "corrupt:addr=";
	mt_write(2,mesg,strlen(mesg));
	itoa((uint)addr,buf);
	mt_write(2,buf,strlen(buf));

	mesg = ":size=";
	mt_write(2,mesg,strlen(mesg));
	itoa(usize,buf);
	mt_write(2,buf,strlen(buf));

	mesg = ":stamp=";
	mt_write(2,mesg,strlen(mesg));
	itoa((uint)stamp,buf);
	mt_write(2,buf,strlen(buf));

	mt_write(2,"\n",1);
}

/* Print trace information */
static mt_trace(addr,type)
reg VOID	*addr;
reg int		type;
{
	char	*mesg, buf[64];

	if(Mt_local || Mt_trace < 0)
		return;

	mesg =	type == MTMALLOC ? "malloc" : type == MTFREE ? "free" :
		type == MTREALLOC ? "realloc" : type == MTRENEW ? "renew" :
		type == MTBUSY ? "busy" : "idle";
	mt_write(Mt_trace,mesg,strlen(mesg));

	mesg = ":addr=";
	mt_write(Mt_trace,mesg,strlen(mesg));
	itoa((uint)addr,buf);
	mt_write(Mt_trace,buf,strlen(buf));

	mesg = ":size=";
	mt_write(Mt_trace,mesg,strlen(mesg));
	if(type == MTIDLE)
		itoa(SIZE(BLOCK(addr))&~03,buf);
	else	itoa(USIZE(BLOCK(addr)),buf);
	mt_write(Mt_trace,buf,strlen(buf));

	mesg = ":stamp=";
	mt_write(Mt_trace,mesg,strlen(mesg));
	if(type == MTIDLE)
		itoa(0,buf);
	else	itoa((uint)USTAMP(BLOCK(addr)),buf);
	mt_write(Mt_trace,buf,strlen(buf));

	mt_write(Mt_trace,"\n",1);
}

/* Print a warning */
static mt_didfree(addr,type)
reg VOID	*addr;
reg int		type;
{
	char	*mesg, buf[64];

	mesg = type == MTFREE ? "free:addr=" : "realloc:addr=";
	mt_write(2,mesg,strlen(mesg));
	itoa((uint)addr,buf);
	mt_write(2,buf,strlen(buf));
	mesg = ":already freed\n";
	mt_write(2,mesg,strlen(mesg));
	if(Mt_corrupt && Mt_corrupt != mt_corrupt)
		(*Mt_corrupt)(addr,0,NIL(VOID*));
}

static mt_isblock(addr,type)
reg VOID	*addr;
reg int		type;
{
	char	*mesg, buf[64];
	reg int		rv;

	/* see if it's a good address */
	if((rv = mt_chkaddr(addr)) == 0)
		return;

	mesg = type == MTFREE ? "free:addr=" : "realloc:addr=";
	mt_write(2,mesg,strlen(mesg));
	itoa((uint)addr,buf);
	mt_write(2,buf,strlen(buf));
	if(rv == 1)
		mesg = ":heap address but not a block\n";
	else if(rv == 2)
		mesg = ":lower than heap address\n";
	else	mesg = ":higher than heap address\n";
	
	mt_write(2,mesg,strlen(mesg));
	if(Mt_corrupt && Mt_corrupt != mt_corrupt)
		(*Mt_corrupt)(addr,0,NIL(VOID*));
}

/* Set trace info for a block */
static mt_setinfo(bp,usize,type)
reg TREE	*bp;
reg uint	usize;
int		type;
{
	reg uchar	*magic, *emagic;

	USIZE(bp) = usize;
	USTAMP(bp) = NIL(VOID*);
	for(magic = SMAGIC(bp), emagic = EMAGIC(bp); magic < emagic; ++magic)
		*magic = MAGIC;

	USTAMP(bp) = Mt_stamp;

	if(Mt_trace >= 0)
		mt_trace(DATA(bp),type);
}

/* see if an address points to a malloc block */
mt_chkaddr(addr)
reg VOID	*addr;
{
	reg TREE	*bp, *endb;

	if(addr >= Baddr)
		return 3;
	if(addr < Laddr)
		return 2;

#ifndef SEGMENT
	for(bp = (TREE*)Laddr, endb = (TREE*) Baddr; bp < endb; bp = MTNEXT(bp))
		if(DATA(bp) == addr)
			return 0;
#endif
	return 1;
}

static mt_chktree(t)
reg TREE	*t;
{
	reg TREE	*n;

	for(n = t; n; n = LINK(n))
		if(mt_chkaddr(DATA(n)) != 0)
		{	char *mesg = "Free tree is corrupted\n";
			mt_write(2,mesg,strlen(mesg));
			return;
		}
	if(n = LEFT(t))
		mt_chktree(n);
	if(n = RIGHT(t))
		mt_chktree(n);
}

/* Certify that no data block has been corrupted */
mt_certify()
{
#ifndef SEGMENT
	reg TREE	*bp, *endb;
	reg uchar	*magic, *endm;

	if(!Mt_corrupt)
		Mt_corrupt = mt_corrupt;

	for(bp = (TREE*)Laddr, endb = (TREE*) Baddr; bp < endb; bp = MTNEXT(bp))
	{
		if(MTISFREE(bp) || MTSIZE(bp) == 0)
			continue;
		if(USIZE(bp) >= MTSIZE(bp))
			(*Mt_corrupt)(DATA(bp),USIZE(bp),USTAMP(bp));
		else for(magic = SMAGIC(bp), endm = EMAGIC(bp); magic < endm; ++magic)
			if(*magic != MAGIC)
			{	(*Mt_corrupt)(DATA(bp),USIZE(bp),USTAMP(bp));
				break;
			}
	}
#endif
	if(Root)
		mt_chktree(Root);
}

/* Print block statistics */
mt_stat(fd,type)
int	fd;
int	type;	/* 0: summary only, 1: busy space only, 2: all blocks */
{
	reg TREE	*bp, *endb;
	reg uint	nfree, nbusy, sfree, sbusy;
	char		buf[64], *mesg;
	int		save_trace;

	nfree = nbusy = sbusy = 0;
#ifndef SEGMENT
	save_trace = Mt_trace;
	Mt_trace = fd;
	for(bp = (TREE*)Laddr, endb = (TREE*) Baddr; bp < endb; bp = MTNEXT(bp))
	{
		if(MTISFREE(bp) || MTSIZE(bp) == 0)
		{	nfree += 1;
			sfree += SIZE(bp);
			if(type > 1)
				mt_trace(DATA(bp),MTIDLE);
		}
		else
		{	nbusy += 1;
			sbusy += USIZE(bp);
			if(type > 0)
				mt_trace(DATA(bp),MTBUSY);
		}
	}
	Mt_trace = save_trace;
#endif
	mesg="#malloc=";
	mt_write(fd,mesg,strlen(mesg));
	itoa(Mt_nmalloc,buf);
	mt_write(fd,buf,strlen(buf));

	mesg="\n#free=";
	mt_write(fd,mesg,strlen(mesg));
	itoa(Mt_nfree,buf);
	mt_write(fd,buf,strlen(buf));

	mesg="\n#realloc=";
	mt_write(fd,mesg,strlen(mesg));
	itoa(Mt_nrealloc,buf);
	mt_write(fd,buf,strlen(buf));

	mesg="\nfree_blocks=";
	mt_write(fd,mesg,strlen(mesg));
	itoa(nfree,buf);
	mt_write(fd,buf,strlen(buf));

	mesg="\nfree_space=";
	mt_write(fd,mesg,strlen(mesg));
	itoa(sbusy,buf);
	mt_write(fd,buf,strlen(buf));

	mesg="\nbusy_blocks=";
	mt_write(fd,mesg,strlen(mesg));
	itoa(nbusy,buf);
	mt_write(fd,buf,strlen(buf));

	mesg="\nbusy_space=";
	mt_write(fd,mesg,strlen(mesg));
	itoa(sbusy,buf);
	mt_write(fd,buf,strlen(buf));

	mesg="\narena_size=";
	mt_write(fd,mesg,strlen(mesg));
	itoa(Baddr-Laddr,buf);
	mt_write(fd,buf,strlen(buf));

	mt_write(fd,"\n",1);
}
#endif	/* MTRACE */

/*
**	malloc().
*/
VOID *malloc(size)
reg uint	size;
{
	reg TREE	*tp, *np, *fp;
	reg int		n, i;
#ifdef MTRACE
	/* save true size and make size large enough to hold our data */
	reg uint	mtsize = size;
	size = size <= (MINSIZE-MTSPACE) ? MINSIZE : size + MTSPACE;
	if(Mt_certify)
		mt_certify();
	if(!Mt_local)
		Mt_nmalloc += 1;
	Mt_local += 1;
#endif

	size = size == 0 ? WORDSIZE : ROUND(size,WORDSIZE);
	tp = NIL(TREE*);
	if(Lfree)
	{	/* see if the last free block can be used */
		fp = BLOCK(Lfree);
		Lfree = NIL(VOID*);
		n = SIZE(fp);
		CLRBITS01(SIZE(fp));
		if(SIZE(fp) == size)
		{	/* exact match, use it as is */
			SIZE(fp) = n;
			if(!QVOID())
				DESTACK(Lfree);
#ifdef MTRACE
			Mt_local -= 1;
			mt_setinfo(fp,mtsize,MTMALLOC);
#endif
			return DATA(fp);
		}
		else if(n >= MINSIZE && size >= MINSIZE && !ISBIT1(n))
		{	/* see if good enough */
			FMERGE(fp,np);
			if(!BOTTOM(fp) && SIZE(fp) >= size)
				tp = fp;
		}
		else	SIZE(fp) = n;
		if(!tp)
			free(Lfree = DATA(fp));
	}

	if(size < MINSIZE)
	{	/**/ ASSERT(!Lfree && QVOID());
		n = size/WORDSIZE - 1;
		if(List[n] == NIL(TREE*))
		{	/* get a bunch of these small blocks */
			if(!(tp = (TREE*) malloc((size+WORDSIZE)*LGET)))
			{
#ifdef MTRACE
				Mt_local -= 1;
#endif
				return NIL(VOID*);
			}
			List[n] = tp;
			for(i = LGET-1; i > 0; --i)
			{	SIZE(tp) = size;
				tp = LINK(tp) = NEXT(tp);
			}
			SIZE(tp) = size;
			LINK(tp) = NIL(TREE*);
		}
		tp = List[n];
		List[n] = LINK(tp);
#ifdef MTRACE
		Mt_local -= 1;
#endif
		return DATA(tp);
	}

	if(!tp)
	{	/* normal malloc requests */
		if(Root && (tp = t_search(size)) != NIL(TREE*))
			CLRBIT1(SIZE(NEXT(tp)));
		else if((tp = Bottom) != NIL(TREE*) && SIZE(tp) >= size)
			Bottom = NIL(TREE*);
		else if((tp = morecore(size)) == NIL(TREE*))
		{
#ifdef MTRACE
			Mt_local -= 1;
#endif
			return NIL(VOID*);
		}
	}	/**/ ASSERT(tp && !ISBITS01(SIZE(tp)));

	if((n = SIZE(tp)-size) >= (MINSIZE+WORDSIZE))
	{	/* the leftover is enough for a new free piece */
		SIZE(tp) = size;
		np = NEXT(tp);
		SIZE(np) = n-WORDSIZE;
		FREE(np);
#ifdef MTRACE
		mt_setinfo(np,n-(WORDSIZE+MTSPACE),MTMALLOC);
		mt_setfree(np);
#endif
	}

	/* set busy the allocated space */
	SETBIT0(SIZE(tp));

	if(!Lfree && !QVOID())
		DESTACK(Lfree);

#ifdef MTRACE
	Mt_local -= 1;
	mt_setinfo(tp,mtsize,MTMALLOC);
#endif
	return DATA(tp);
}

/*
**	realloc().
*/
VOID *realloc(old,size)
VOID		*old;
reg uint	size;
{
	reg TREE	*tp, *np;
	reg int		n, ts;
	reg VOID	*new;
#ifdef MTRACE
	reg uint	mtsize = size;
	if(old)
	{
		size = size <= (MINSIZE-MTSPACE) ? MINSIZE : size + MTSPACE;
		mt_isblock(old,MTREALLOC);
		if(Mt_certify)
			mt_certify();
		if(MTISFREE(BLOCK(old)))
			mt_didfree(old,MTREALLOC);
		if(Mt_trace >= 0)
			mt_trace(old,MTREALLOC);
		Mt_nrealloc += 1;
		Mt_local += 1;
	}
#endif
	if(!old)
		return malloc(size);

	if(Lfree)
	{	/* free everything except old */
		Nofree = old;
		free(Lfree);
		Nofree = NIL(VOID*);
	}

	size = size == 0 ? WORDSIZE : ROUND(size,WORDSIZE);
	tp = BLOCK(old);
	ts = SIZE(tp);
	if(size >= MINSIZE && ts >= MINSIZE)
	{
		CLRBITS01(SIZE(tp));
		if((n = SIZE(tp)-size) < 0)
		{	/* growing, try forward merging */
			FMERGE(tp,np);
			n = SIZE(tp) - size;
#ifndef SEGMENT
			if(n < 0 && BOTTOM(tp) && GETCORE(0) == Baddr)
			{	/* try extending core */
				Bottom = tp;
				if((tp = morecore(size)) != NIL(TREE*))
					n = SIZE(tp) - size;
				else	tp = Bottom;
			}
#endif /*!SEGMENT*/
		}
		if(n >= (MINSIZE+WORDSIZE))
		{	/* left over is enough for a new piece */
			SIZE(tp) = size;
			np = NEXT(tp);
			SIZE(np) = (n-WORDSIZE);
			FREE(np);
#ifdef MTRACE
			mt_setinfo(np,n-(WORDSIZE+MTSPACE),MTMALLOC);
			mt_setfree(np);
#endif
		}
		CPYBITS01(SIZE(tp),ts);
		if(n >= 0)
		{	/* got it */
#ifdef MTRACE
			Mt_local -= 1;
			mt_setinfo(tp,mtsize,MTRENEW);
#endif
			return old;
		}
	}

	/* call malloc to get a new block */
	n = ts & ~BITS01;
#ifdef MTRACE
	mt_setinfo(tp,n-MTSPACE,MTRENEW);
#endif
	if(size <= SIZE(tp))
		new = DATA(tp);
	else if((new = malloc(size)) != NIL(VOID*))
	{	/* copy data and reclaim all free space */
		memcpy(new,old,n < size ? n : size);
		if(Lfree)
			ENQUEUE(Lfree);
		Lfree = old;
#ifdef MTRACE
		mt_setfree(tp);
#endif
	}
#ifdef MTRACE
	Mt_local -= 1;
	if(new)
		mt_setinfo(BLOCK(new),mtsize,MTRENEW);
#endif
	return new;
}

/*
**	free().
*/
void free(old)
reg VOID	*old;
{
	reg int		size;
	reg TREE	*tp, *np, *sp;
	reg VOID	*dequeue;
#ifdef MTRACE
	if(old != Lfree)
	{	/* not an internal call */
		mt_isblock(old,MTFREE);
		if(Mt_certify)
			mt_certify();
		if(Mt_trace >= 0)
			mt_trace(old,MTFREE);
		if(MTISFREE(BLOCK(old)))
			mt_didfree(old,MTFREE);
		else	mt_setfree(BLOCK(old));
	}
	if(!Mt_local)
		Mt_nfree += 1;
	Mt_local += 1;
#endif

	if(!old)
		old = Lfree;

	dequeue = NIL(VOID*);
	if(Lfree != old)
	{	/* this is a normal free call */
		if(Lfree)
		{	/* make queue space for current Lfree */
			if(QFULL())
				DEQUEUE(dequeue);	/**/ ASSERT(!QFULL());
			ENQUEUE(Lfree);
		}
		Lfree = old;
		old = dequeue;
	}
	else	Lfree = NIL(VOID*);

	while(old)
	{	/* being realloc-ed, don't free it */
		if(old == Nofree)
			goto next;

		tp = BLOCK(old);
		if((size = SIZE(tp)) < MINSIZE)
		{	/* small block */
			size = size/WORDSIZE - 1;
			LINK(tp) = List[size];
			List[size] = tp;
			goto next;
		}

		/* merge adjacent free blocks */
		CLRBITS01(SIZE(tp));
		FMERGE(tp,np);
		if(ISBIT1(size))
			BMERGE(tp,np);

		if(BOTTOM(tp))
		{
#ifndef SEGMENT		/* bottom block, may return some space to the OS */
			reg int	brksize = M_brksize;
			if(brksize > 0 && GETCORE(0) == Baddr)
			{	brksize = ROUND(brksize,CORESIZE);
				if((size = SIZE(tp)-MINSIZE) >= brksize)
				{
					size = (size/CORESIZE)*CORESIZE;
					GETCORE(-size);
					if((old = GETCORE(0)) != Baddr)
					{	Baddr = old;
						SIZE(tp) = (old-WORDSIZE) - DATA(tp);
						SIZE(NEXT(tp)) = BIT0;
					}
				}
			}
#endif
			Bottom = tp;
			goto next;
		}

		/* tell next block that this one is free */
		SETBIT1(SIZE(NEXT(tp)));	/**/ ASSERT(ISBIT0(SIZE(NEXT(tp))));

		/* leaf insert into the free tree */
		LEFT(tp) = RIGHT(tp) = LINK(tp) = NIL(TREE*);
		*(SELFP(tp)) = tp;

		if(!Root)
		{	Root = tp;
			goto next;
		}

		np = Root;
		size = SIZE(tp);
		while(1)
		{	if(SIZE(np) > size)
			{	if((sp = LEFT(np)) != NIL(TREE*))
					np = sp;
				else
				{	LEFT(np) = tp;
					break;
				}
			}
			else if(SIZE(np) < size)
			{	if((sp = RIGHT(np)) != NIL(TREE*))
					np = sp;
				else
				{	RIGHT(np) = tp;
					break;
				}
			}
			else /* SIZE(np) == size */
			{	if((sp = LINK(np)) != NIL(TREE*))
				{	LINK(tp) = sp;
					BACK(sp) = tp;
				}
				LINK(np) = tp;
				BACK(tp) = np;
				SETNOTREE(tp);
				break;
			}
		}
	next:
		if(dequeue || QVOID())
			old = NIL(VOID*);
		else	DEQUEUE(old);
	}
#ifdef MTRACE
	if(Lfree)
		free(Lfree);
	Mt_local -= 1;
#endif
}

/*
**	Get more core. Gaps in memory are noted as busy blocks.
*/
static TREE *morecore(size)
reg uint	size;
{
	reg TREE	*tp, *bp;
	reg VOID	*addr;

	/* space for queue of delayed free blocks */
	if(!Qfree)
		size += QSIZE*sizeof(Qfree[0]);

	/* determine memory request size and get it */
	size += 2*WORDSIZE;
#ifdef SEGMENT
	if(size > CORESIZE)
		return NIL(TREE*);
	else	size = CORESIZE;
#else
	size = ROUND(size,CORESIZE);
#endif
	if((addr = GETCORE(size)) == ERRCORE)
		return NIL(TREE*);

	/* merging with bottom block */
	bp = Bottom;
	if(addr == Baddr)
	{	/* contiguous memory, merge with previous bottom */
		if(bp)
		{	addr = ((VOID*)bp);
			size += SIZE(bp) + 2*WORDSIZE;
		}
		else
		{	addr = Baddr-WORDSIZE;
			size += WORDSIZE;
		}
	}
	else
	{
#ifndef SEGMENT
		reg int	n;
		if((n = ((uint)addr)%ALIGN) != 0)
		{	/* make sure alignment is correct */
			n = ALIGN - n;
			if(GETCORE(n) == ERRCORE)
				return NIL(TREE*);
			addr += n;
		}
#endif
		if(!Qfree)
		{	/* space for the free queue */
			Qfree = (VOID**) addr;
			addr += QSIZE*sizeof(Qfree[0]);
			size -= QSIZE*sizeof(Qfree[0]);
#ifdef MTRACE
			if(!Laddr)
				Laddr = addr;
#endif
		}
		else
		{	/* make the gap a busy block */
			tp = BLOCK(Baddr);
			SIZE(tp) = (addr-Baddr)|BIT0;
		}
	}

	/* new bottom address */
	Bottom = NIL(TREE*);
	Baddr = addr + size;

	/* new bottom block */
	tp = (TREE*)addr;
	SIZE(tp) = size - 2*WORDSIZE;	/**/ASSERT((SIZE(tp)%WORDSIZE) == 0);

	/* reserved the last word to head any noncontiguous memory */
	SIZE(NEXT(tp)) = BIT0;

	if(bp && bp != tp)
	{	/* non-contiguous memory, free old bottom block */
		/**/ ASSERT(!Lfree && QVOID());
		SETBIT0(SIZE(bp));
		ENQUEUE(DATA(bp));
	}

	return tp;
}

/*
**	Tree rotation functions
*/
#define RROTATE(t,r)	(t = LEFT(r), LEFT(r) = RIGHT(t), RIGHT(t) = r, r = t)
#define LROTATE(t,r)	(t = RIGHT(r), RIGHT(r) = LEFT(t), LEFT(t) = r, r = t)
#define RLINK(r,s,x)	(r ? (s = LEFT(s) = x) : (r = s = x))
#define LLINK(r,s,x)	(r ? (s = RIGHT(s) = x) : (r = s = x))
#define RTWICE(t,r)	(LEFT(LEFT(r)) = RIGHT(t), RIGHT(t) = r, r = t)
#define LTWICE(t,r)	(RIGHT(RIGHT(r)) = LEFT(t), LEFT(t) = r, r = t)

/*
**	Look up a suitable element in the tree. If found, delete it from
**	the tree and return its address.
**	This uses the top-down splay strategy.
*/
static TREE *t_search(size)
reg int		size;
{
	reg int		cmp;
	reg TREE	*t, *del, *left, *right, *lroot, *rroot;

	/* find the right one to delete */
	del = Root;
	lroot = rroot = NIL(TREE*);
	while(del)
	{	/**/ ASSERT(!ISBITS01(size) && !ISBITS01(SIZE(del)));
		if((cmp = size - SIZE(del)) == 0)
			break;
		if(cmp < 0)
		{	if((t = LEFT(del)) == NIL(TREE*))
			{	RLINK(rroot,right,del);
				del = NIL(TREE*);
			}
			else if((cmp = size - SIZE(t)) <= 0)
			{	/* left,left case */
				RROTATE(t,del);
				if(cmp == 0)
					break;
				RLINK(rroot,right,del);
				del = LEFT(del);
			}
			else
			{	/* left, right case */
				RLINK(rroot,right,del);
				LLINK(lroot,left,t);
				del = RIGHT(t);
			}
		}
		else
		{	if((t = RIGHT(del)) == NIL(TREE*))
			{	LLINK(lroot,left,del);
				del = NIL(TREE*);
			}
			else if((cmp = size - SIZE(t)) >= 0)
			{	/* right, right case */
				LROTATE(t,del);
				if(cmp == 0)
					break;
				LLINK(lroot,left,t);
				del = RIGHT(del);
			}
			else
			{	/* right, left case */
				LLINK(lroot,left,del);
				RLINK(rroot,right,t);
				del = LEFT(t);
			}
		}
	}

	if(del)
	{	if(lroot)
		{	RIGHT(left) = LEFT(del);
			LEFT(del) = lroot;
		}
		if(rroot)
		{	LEFT(right) = RIGHT(del);
			RIGHT(del) = rroot;
		}
	}
	else
	{	if(lroot)
			RIGHT(left) = NIL(TREE*);
		if(rroot)
		{	/* get least one > size */
			LEFT(right) = NIL(TREE*);
			while(LEFT(rroot))
			{	/* left zig-zig case */
				if((t = LEFT(LEFT(rroot))) != NIL(TREE*))
					RTWICE(t,rroot);
				else	RROTATE(t,rroot);
			}
			LEFT(rroot) = lroot;
			del = rroot;
		}
		else
		{	Root = lroot;
			return NIL(TREE*);
		}
	}

	if((t = LINK(del)) != NIL(TREE*))
	{	/* start of a non-singleton list */
		LEFT(t) = LEFT(del);
		RIGHT(t) = RIGHT(del);
		Root = t;	/**/ ASSERT(!ISNOTREE(t));
	}
	else if((right = RIGHT(del)) != NIL(TREE*))
	{	/* make least elt of right tree the root */
		while(LEFT(right))
		{	/* left zig-zig case */
			if((t = LEFT(LEFT(right))) != NIL(TREE*))
				RTWICE(t,right);
			else	RROTATE(t,right);
		}
		LEFT(right) = LEFT(del);
		Root = right;
	}
	else	Root = LEFT(del);

	return del;
}
