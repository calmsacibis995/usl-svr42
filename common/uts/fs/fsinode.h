/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FSINODE_H	/* wrapper symbol for kernel use */
#define _FS_FSINODE_H	/* subject to change without notice */
#ident	"@(#)uts-comm:fs/fsinode.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef	struct	ipool {
	struct ipool	*i_hf;	/* hash chain, forward */
	struct ipool	*i_hb;	/* hash chain, back */
	struct ipool	*i_ff;	/* free chain, forward */
	struct ipool	*i_fb;	/* free chain, back */
	struct vnode	*i_vp;	/* ptr to vnode */
	struct idata	*i_data;/* pointer to the pool data */
} ipool_t ;

#define	IPOOL_TOVP(IP)		((ipool_t *)IP)->i_vp
#define	IPOOL_TODATA(IP)	((ipool_t *)IP)->i_data

struct	istat {
	int	i_numinodes;	/* total number of inodes allocated */
	int	i_allocsize;	/* total size allocated */
	int	i_attempts;	/* total number of attempts */
	int	i_skips;	/* total number of skips */
	int	i_dskips;	/* total number of delay skips */ 
	int	i_secskips;	/* total number of sec skips */
	int	i_tryattempts;	/* total number of purges */
	int	i_trydskips;	/* total number of purge skips */
	int	i_dealloc;	/* total number of deallocations */
};


/*
 * The id_inuse count indicates the number of inodes from this pool that
 * on the filesystem freelist.
 * The inode could be locked via an ILOCK()/IRWLOCK()/ILOCKED.  Thus the
 * filesystem has to indicate if the inode can be removed.
 */
struct	idata {
	struct	idata	*id_next;	/* pointer to the next pool */
	struct	idata	*id_prev;	/* pointer to the next pool */
	struct	fshead	*id_fshead;	/* pointer to the filesystem head */
	clock_t		id_alloclbolt;	/* time when the block was allocated */
	clock_t		id_startlbolt;	/* started to deallocate block */
	short		id_total; 	/* number of inodes in this pool */
	short		id_inuse;	/* number of inodes in use */
	short		id_flag;	/* flags for this pool */
	struct	ipool	id_freelist;	/* freelist of inodes that will be deallocated */
};

/* Flags */
#define IDLOCKED	0x0001		/* someone has this locked */
#define IDWANT		0x0002		/* someone wants this block mostlikely unmount() */

typedef	struct idata	idata_t;
#define	MIN_WAITTIME	10000		/* MIN wait time before deallocation of the pool */
#define	MAX_CLEANTIME	10000		/* MAX time per call to sync out inodes		*/

struct fshead {
	int		f_flag;		/* flags for this filesystem head */
	struct	ipool	*f_freelist;	/* pointer to the filesystem freelist */
	int     	(*f_inode_cleanup)(); /* Cleanup function */
	short		f_reserv1;	/* reserved */
	short		f_maxpages;	/* maximum number of pages for each allocation */
	short		f_mount;	/* number of mounted filesystems */
	short		f_isize;	/* size of each inode */
	short		f_inum;		/* number of inodes to allocate per pool */
	short		f_max;		/* maximum number of inodes for this FS type */
	short		f_curr;		/* number of inodes currently allocated */
	short		f_inuse;	/* number of inodes being used  */
	short		f_fail;		/* number of times couldn't allocate an inode */
	short		f_frag;		/* size of each fragment in the pool */
	short		f_idmin;	/* start freeing up id when its inuse count drops */
	long		f_asize;	/* actual size of each pool */
	struct	idata	f_idata;	/* head of the inode pools */
};

/* Defination to the flags */
#define DEALLOC		0x0001		/* atleast one pool that can be deallocated */



#ifdef _KERNEL

/*
 * Macros for insertion of freelist pools 
 */


#define INS_POOLHEAD(freelistp, id) { \
	((idata_t *)id)->id_next = (freelistp)->id_next; \
	(freelistp)->id_next->id_prev = (idata_t *)id; \
	(freelistp)->id_next = (idata_t *)id; \
	((idata_t *)id)->id_prev = (freelistp); \
}

#define	INS_POOLTAIL(freelistp, id) { \
	((idata_t *)id)->id_prev = (freelistp)->id_prev; \
	(freelistp)->id_prev->id_next = (idata_t *)id; \
	(freelistp)->id_prev = (idata_t *)id; \
	((idata_t *)id)->id_next = (freelistp); \
}

#define	RM_POOL(id) { \
	((idata_t *)id)->id_next->id_prev = ((idata_t *)id)->id_prev; \
	((idata_t *)id)->id_prev->id_next = ((idata_t *)id)->id_next; \
	((idata_t *)id)->id_next = ((idata_t *)id)->id_prev = NULL; \
}



/* Macros for ipool freelist insertion and removal */


#define INS_IFREEHEAD(freelistp, ip, fsheadp) { \
	((ipool_t *)ip)->i_ff = (freelistp)->i_ff; \
	(freelistp)->i_ff->i_fb = (ipool_t *)ip; \
	(freelistp)->i_ff = (ipool_t *)ip; \
	((ipool_t *)ip)->i_fb = (freelistp); \
	((ipool_t *)ip)->i_data->id_inuse--; \
	(fsheadp)->f_inuse--; \
}

#define	INS_IFREETAIL(freelistp, ip, fsheadp) { \
	((ipool_t *)ip)->i_fb = (freelistp)->i_fb; \
	(freelistp)->i_fb->i_ff = (ipool_t *)ip; \
	(freelistp)->i_fb = (ipool_t *)ip; \
	((ipool_t *)ip)->i_ff = (freelistp); \
	((ipool_t *)ip)->i_data->id_inuse--; \
	(fsheadp)->f_inuse--; \
}

#define	RM_IFREELIST(ip, fsheadp) { \
	((ipool_t *)ip)->i_ff->i_fb = ((ipool_t *)ip)->i_fb; \
	((ipool_t *)ip)->i_fb->i_ff = ((ipool_t *)ip)->i_ff; \
	((ipool_t *)ip)->i_ff = ((ipool_t *)ip)->i_fb = NULL; \
	((ipool_t *)ip)->i_data->id_inuse++; \
	(fsheadp)->f_inuse++; \
}

#define	IS_IPFREE(ip) \
	((((ipool_t *)ip)->i_ff != NULL) && \
	(((ipool_t *)ip)->i_fb != NULL))

#endif /*_KERNEL */

#endif	/* _FS_FSINODE_H */
