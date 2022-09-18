/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/dynstructs.h	1.3"
#ident	"$Header: $"

#ifdef	_KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h> /* REQUIRED */

#endif	/* _KERNEL_HEADERS */

extern struct jpool	*sdi_get();
extern void 		sdi_free();
extern void 		sdi_poolinit();

typedef	struct	jpool {
	struct jpool	*j_ff;	/* free chain, forward */
	struct jpool	*j_fb;	/* free chain, back */
	struct jdata	*j_data;/* pointer to the pool data */
} jpool_t ;

#define	POOL_TODATA(JP)	((jpool_t *)JP)->j_data

/*
 * The jd_inuse count indicates the number of structs from this pool that
 * are in use.
 */
struct	jdata {
	struct	jdata	*jd_next;	/* pointer to the next pool */
	struct	jdata	*jd_prev;	/* pointer to the next pool */
	struct	head	*jd_head;	/* pointer to the job pool head */
	clock_t		jd_lastuse;	/* time when the pool was last used */
	short		jd_total; 	/* number of inodes in this pool */
	short		jd_inuse;	/* number of inodes in use */
};

typedef	struct jdata	jdata_t;

#define	MIN_WAITTIME	1000		/* MIN wait time before deallocation of the pool */

struct head {
	int		f_flag;		/* flags for this head */
	struct	jpool	*f_freelist;	/* freelist */
	short		f_maxpages;	/* max number of pages for each alloc */
	short		f_isize;	/* size of each job struct */
	short		f_inum;		/* number of structs to alloc per pool */
	short		f_curr;		/* number of structs currently alloced */
	short		f_frag;		/* size of each fragment in the pool */
	short		f_idmin;	/* start freeing up id when its inuse count drops */
	long		f_asize;	/* actual size of each pool */
	struct	jdata	f_jdata;	/* head of the pools */
};

/* Defination to the flags */
#define POOLWANT	0x0001		/* atleast one pool to deallocate */


/*
 * Macros for insertion of freelist pools 
 */


#define INS_POOLHEAD(freelistp, jd) { \
	((jdata_t *)jd)->jd_next = (freelistp)->jd_next; \
	(freelistp)->jd_next->jd_prev = (jdata_t *)jd; \
	(freelistp)->jd_next = (jdata_t *)jd; \
	((jdata_t *)jd)->jd_prev = (freelistp); \
}

#define	INS_POOLTAIL(freelistp, jd) { \
	((jdata_t *)jd)->jd_prev = (freelistp)->jd_prev; \
	(freelistp)->jd_prev->jd_next = (jdata_t *)jd; \
	(freelistp)->jd_prev = (jdata_t *)jd; \
	((jdata_t *)jd)->jd_next = (freelistp); \
}

#define	RM_POOL(jd) { \
	((jdata_t *)jd)->jd_next->jd_prev = ((jdata_t *)jd)->jd_prev; \
	((jdata_t *)jd)->jd_prev->jd_next = ((jdata_t *)jd)->jd_next; \
	((jdata_t *)jd)->jd_next = ((jdata_t *)jd)->jd_prev = NULL; \
}



/* Macros for jpool freelist insertion and removal */


#define INS_FREEHEAD(freelistp, jp) { \
	((jpool_t *)jp)->j_ff = (freelistp)->j_ff; \
	(freelistp)->j_ff->j_fb = (jpool_t *)jp; \
	(freelistp)->j_ff = (jpool_t *)jp; \
	((jpool_t *)jp)->j_fb = (freelistp); \
	((jpool_t *)jp)->j_data->jd_inuse--; \
}

#define	INS_FREETAIL(freelistp, jp) { \
	((jpool_t *)jp)->j_fb = (freelistp)->j_fb; \
	(freelistp)->j_fb->j_ff = (jpool_t *)jp; \
	(freelistp)->j_fb = (jpool_t *)jp; \
	((jpool_t *)jp)->j_ff = (freelistp); \
	((jpool_t *)jp)->j_data->jd_inuse--; \
}

#define	RM_FREELIST(jp) { \
	((jpool_t *)jp)->j_ff->j_fb = ((jpool_t *)jp)->j_fb; \
	((jpool_t *)jp)->j_fb->j_ff = ((jpool_t *)jp)->j_ff; \
	((jpool_t *)jp)->j_ff = ((jpool_t *)jp)->j_fb = NULL; \
	((jpool_t *)jp)->j_data->jd_inuse++; \
}
