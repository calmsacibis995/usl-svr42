/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_IPOOL_H	/* wrapper symbol for kernel use */
#define _FS_IPOOL_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/ipool.h	1.3.3.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Following is the common header structure for inodes in the common
 * inode pool.
 */
typedef	struct	i_header	{
	struct i_header	*i_hf;	/* hash chain, forward */
	struct i_header	*i_hb;	/* hash chain, back */
	struct i_header	*i_ff;	/* free chain, forward */
	struct i_header	*i_fb;	/* free chain, back */
	struct vnode	*vp;	/* ptr to vnode */
	char	*secp;		/* extra memory for security data */
} iheader_t;

/*
 * File system types participating in the common inode pool scheme.
 */
#define	S5FSTYP		0
#define SFSTYP		1
#define	NUM_FSTYP	2

#define	IHEADTOVP(IP)	((iheader_t *)IP)->vp
#define IHEADTOSECP(IP)	((iheader_t *)IP)->secp

struct	ipool_stats {
	int	numinodes;
	int	inodesize;
	caddr_t	pool_start;
	caddr_t	pool_end;
	caddr_t pool_free;
};

/*
 * Following is the structure to contain type specific information
 * for file system types participating in the common inode pool scheme.
 */
struct	ipool_data {
	short	inosize; /* size, before & after ipool_init() */
	short	inonum; /* number configured- before & after ipool_init() */
	short	inuse; /* number of inodes in use */	
	short	total; /* number of inodes of this type in system */
	int	(*inode_sync)();
	void	(*inode_cleanup)();
	struct  vnodeops *v_op;	/* for quick check of vnode type */
};

#ifdef _KERNEL

#define	GETIPOOLDATAP(vp, idpp) { \
	if (ipooldata[S5FSTYP].v_op == vp->v_op) \
		*(idpp) = &ipooldata[S5FSTYP]; \
	else { \
		if (ipooldata[SFSTYP].v_op == vp->v_op) \
			*(idpp) = &ipooldata[SFSTYP]; \
		else \
			cmn_err(CE_PANIC,"GETIPOOLDATA: bad type"); \
	} \
}


extern struct 	ipool_data *ipool_init();
extern int 	ipool_get();
extern void	ipool_ret();
extern void	*get_ipoolnext();

#define	REM_IFREELIST(ip) { \
	((iheader_t *)ip)->i_ff->i_fb = ((iheader_t *)ip)->i_fb; \
	((iheader_t *)ip)->i_fb->i_ff = ((iheader_t *)ip)->i_ff; \
	((iheader_t *)ip)->i_ff = ((iheader_t *)ip)->i_fb = NULL; \
}


#define	IS_IPFREE(ip) \
	((((iheader_t *)ip)->i_ff != NULL) && \
	(((iheader_t *)ip)->i_fb != NULL))

#endif /*_KERNEL */

#endif	/* _FS_IPOOL_H */
