/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/ipool.c	1.2.3.2"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/ipool.h>
#include <fs/mode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

/*
 * Ipool is designed to allow sharing of a pool of incore inodes between
 * 2 file system types- S5 and SFS/UFS. Since S5 and SFS inodes are of
 * different sizes, the SFS inode is split into a base portion and 2
 * extensions to the base portion, such that the size of the base portion
 * approximately equals the size of an incore S5 inode. The two extensions
 * for the SFS inode are the Vnode, and the secure inode, both of
 * which are allocated dynamically and referenced from the statically
 * allocated base portion. In the case that an SFS inode is allocated
 * for a UFS file, the secure inode portion is not required and is
 * either not allocated, or, is freed if it was previously allocated.
 *
 * The design does not limit such sharing to only the 2 file system
 * types mentioned above. Efficiency considerations must determine
 * whether or not the inode memory is to be shared. One of the
 * criteria is whether the inode size of a file system type that
 * may participate in sharing roughly equals that of the other
 * participating types. Paricipating file system types need to provide
 * pointers to 2 functions- (1) to update the inodes as they are
 * reallocated from the pool and (2) to perform any file system specific
 * cleanup functions on the inodes, such as freeing the block map,
 * and any other dynamically allocated resources. 
 */


/* Macros for ipool freelist insertion and removal */

#define	INS_IFREETAIL(ip) { \
	((iheader_t *)ip)->i_fb = ipool_free.i_fb; \
	(ipool_free.i_fb)->i_ff = (iheader_t *)ip; \
	ipool_free.i_fb = (iheader_t *)ip; \
	((iheader_t *)ip)->i_ff = &ipool_free; \
}

#define INS_IFREEHEAD(ip) { \
	((iheader_t *)ip)->i_ff = ipool_free.i_ff; \
	(ipool_free.i_ff)->i_fb = (iheader_t *)ip; \
	ipool_free.i_ff = (iheader_t *)ip; \
	((iheader_t *)ip)->i_fb = &ipool_free; \
}

STATIC struct ipool_data	ipooldata[NUM_FSTYP];
STATIC struct ipool_stats	ipoolstats;

STATIC char *fs_strings[] = { "s5", "sfs" };

STATIC struct i_header ipool_free = {	
			NULL, NULL, &ipool_free, &ipool_free, NULL, NULL };

STATIC caddr_t ipool_start, ipool_end;	
STATIC int inum, isiz;

/*
 * Get the next inode after the supplied inode (ip) in the common inode pool.
 * The inode is to reside on a file system which supports the supplied
 * vnode operations (vopsp).
 * This call is useful in stepping through the inodes in the pool.
 */
void *
get_ipoolnext(ip, vopsp)
	register caddr_t ip;
	register struct vnodeops *vopsp;
{
	register caddr_t ipx;
	register struct vnode *vp;

	if (ip == NULL)
		ip = ipool_start;
	else
		ip += isiz;

	for (ipx = (caddr_t)ip; ipx < ipool_end; ipx += isiz) {
		if ((vp = IHEADTOVP(ipx)) != NULL  && vp->v_op == vopsp) 
			return((void *)ipx);
	}

	return NULL;
}

/*
 * Return inode to the common inode free list.
 * If flag is set, the inode is returned to the head of the inode free
 * list; otherwise, it is returned to the tail.
 */
void
ipool_ret(ip, flag)
	register void *ip;
	int flag;
{
	if (flag) {
		INS_IFREEHEAD(ip);
	}
	else {
		INS_IFREETAIL(ip);
	}
}


STATIC int
last_ipoolinit()
{
	/* return 1 if all ipool data are initialized, else 0 */

	int i;
	for (i=0; i < NUM_FSTYP; i++) {
		if (vfs_getvfssw(fs_strings[i]) != NULL)
			if (ipooldata[i].inosize == 0) 
				return 0;
	}
	return 1;
}

/*
 * Initialize common inode pool information.
 */
struct ipool_data *
ipool_init(fstyp, fs_idata)
	char	*fstyp;	
	struct	ipool_data fs_idata;
{
	int i;
	caddr_t ipx;
	struct	ipool_data *ipooldp;

	for (i=0; i < NUM_FSTYP; i++) {
		if (strcmp(fstyp, fs_strings[i]) == 0) {
			ipooldata[i] = fs_idata; /* structure assignment */ 
			ipooldp = &ipooldata[i];
		}
	}

	if (last_ipoolinit()) {
		inum = 0;
		isiz = 0;
		for (i=0; i < NUM_FSTYP; i++) {
			if (ipooldata[i].inosize > isiz)
				isiz = ipooldata[i].inosize;
			if (ipooldata[i].inonum > inum)
				inum = ipooldata[i].inonum;
		}

		ipx = kmem_zalloc((inum * isiz), KM_SLEEP);

		ipool_start = ipx;
		ipool_end = ipx + (inum*isiz);

		for (i = 0; i < inum; i++ ) {
			((iheader_t *)ipx)->i_hf = ((iheader_t *)ipx);
			((iheader_t *)ipx)->i_hb = ((iheader_t *)ipx);
			INS_IFREETAIL(ipx);
			IHEADTOVP(ipx) = NULL;
			IHEADTOSECP(ipx) = NULL;
			ipx += isiz;
		}

		ipoolstats.numinodes = inum;
		ipoolstats.inodesize = isiz;
		ipoolstats.pool_start = ipool_start;
		ipoolstats.pool_end = ipool_end;
		ipoolstats.pool_free = (caddr_t)(&ipool_free);

		for(i=0; i < NUM_FSTYP; i++) {
			ipooldata[i].inosize = (short)isiz;
			ipooldata[i].inonum = (short)inum;
			ipooldata[i].inuse = 0;
			ipooldata[i].total = 0;
		}
	}
	return(ipooldp);
}

/*
 * Get an inode from the common inode pool.
 * idatapp will contain a pointer to file system specific information
 * on the inode's previous use, so that the inode can be "cleaned"
 * prior to re-use.
 */
int
ipool_get(idatapp, ipp)
	register struct ipool_data **idatapp;
	struct i_header  **ipp;
{
	register struct i_header *ip;
	register struct vnode *vp;
retry:
	*idatapp = NULL;
	while (ipool_free.i_ff == &ipool_free) {
		if (dnlc_purge1() == 0) { /* may have slept */
			if (ipool_free.i_ff == &ipool_free) {
				*ipp = NULL;
				return 1;
			}
			break;
		}
	}
	ip = ipool_free.i_ff;
	ASSERT(ip != &ipool_free);
	*ipp = ip;
	REM_IFREELIST(ip);
	if ((vp = IHEADTOVP(ip)) != NULL) {
		int err;
		/*
 		 * The following code checks to be 
		 * sure that putpages from the page layer
 		 * have not activated the vnode while the 
		 * inode is on the free list. If
 		 * we hit this case we put the inode back
		 *  on the tail of the free list
 		 * and try again. If there are not other 
		 * inodes on the free list then
 		 * we put the inode back and must call 
		 * preempt so that some other process
 		 * can do work to free an inode.
 		 */

		if (vp->v_count > 0) {
			if (ipool_free.i_ff == &ipool_free) {
				/*
			 	 *	Put inode back on head of freelist... 
			 	 *	only 1 inode left!
			 	 */
				INS_IFREEHEAD(ip);
				preempt();
			}
			else {
				/*
			 	 *	Put inode on end of freelist.
			 	 */
				INS_IFREETAIL(ip);
			}
			goto retry;
		}
		GETIPOOLDATAP(vp,idatapp);
	  	err = (*((*idatapp)->inode_sync))(ip);
		return err;
	}
	return 0;
}
