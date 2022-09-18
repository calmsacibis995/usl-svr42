/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/fsinode.c	1.6"
#ident  "$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/fsinode.h>
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
#include <proc/disp.h>	/* XXX */
#include <proc/proc.h>	/* XXX -- needed for user-context kludge in ILOCK */
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


STATIC struct istat	fs_istat;
STATIC int fs_waittime = MIN_WAITTIME;
STATIC int fs_cleantime = MAX_CLEANTIME;

void
fs_iret(fsheadp, ip, flag)
struct	fshead	*fsheadp;
struct	ipool	*ip;
int flag;
{
	struct	ipool	*freelistp;

	freelistp = fsheadp->f_freelist;
	if (flag) {
		/*
		 * Help the inodes on the pools that are going to be free
		 * to have less of a chance of being selected for recycling.
		 */

		if ((IPOOL_TODATA(ip)->id_inuse < fsheadp->f_idmin) ||
		   (IPOOL_TODATA(ip)->id_startlbolt)) {
			INS_IFREETAIL(freelistp, ip, fsheadp);
		} else {
			INS_IFREEHEAD(freelistp, ip, fsheadp);
		}
		
	} else {
		INS_IFREETAIL(freelistp, ip, fsheadp);
	}

	if (ip->i_data->id_inuse < fsheadp->f_idmin) 
		fsheadp->f_flag |= DEALLOC;

}

fs_ipoolinit(fshead)
struct	fshead	*fshead;
{
	int	asize;
	int	isize;
	int	inum;
	int	frag;
	int	i, j;
	int	minfragpage;

	isize = fshead->f_isize;


	asize = fshead->f_maxpages * PAGESIZE;
	inum = (asize - sizeof (struct idata))/isize;
	frag = asize - (sizeof (struct idata) + inum*isize);

	/*
	 * The fshead contents should be zero'ed out in the
	 * filesystem dependent code and then initialized
	 * with filesystem specific data.
	 */
	ASSERT(fshead->f_inode_cleanup != NULL);
	fshead->f_asize = asize;
	fshead->f_inum = inum;
	fshead->f_idmin = (inum*115)/100 - inum; /* 15% */
	fshead->f_frag = frag;
	fshead->f_idata.id_next = fshead->f_idata.id_prev = &fshead->f_idata;
	fshead->f_idata.id_fshead = fshead;

	/* To prevent it from being selected for deallocation */
	fshead->f_idata.id_inuse = 0x7fff;

	ASSERT(fshead->f_frag >= 0);
#ifdef DEBUG
	printf( "freel 0x%x, isize %d, asize %d, inum %d, idmin, %d max %d, frag %d\n",
		fshead->f_freelist, fshead->f_isize, fshead->f_asize, 
		fshead->f_inum, fshead->f_idmin, fshead->f_max, fshead->f_frag);
#endif

}

STATIC
struct idata *
fs_ipoolalloc(fshead)
struct	fshead *fshead;
{
	struct	idata	*idatap;
	int		asize;
	int		inum;

	asize = fshead->f_asize;
	inum = fshead->f_inum;

	if ((idatap  = (void *)kmem_zalloc(asize, KM_NOSLEEP)) == NULL)
		return (NULL);

	idatap->id_alloclbolt = lbolt;
	idatap->id_startlbolt = 0;
	idatap->id_total = inum;
	idatap->id_inuse = inum;

	fs_istat.i_numinodes += inum;
	fs_istat.i_allocsize += asize;

	return(idatap);
}

STATIC
fs_check_idata(fsheadp)
struct	fshead	*fsheadp;
{
	struct	idata	*idatap;
	int		avail;

	avail = 0;
	idatap = fsheadp->f_idata.id_next;
	while (idatap != &fsheadp->f_idata) {
		if (idatap->id_inuse < fsheadp->f_idmin)
			avail++;
		idatap = idatap->id_next;
	}

	return(avail);
}



fs_cleanone(fsheadp, start_idatap, delay)
struct	fshead	*fsheadp;
struct	idata	*start_idatap;
int		delay;
{
	struct	idata	*idatap;
	struct	idata	*end_idatap;
	char		*ip;
	int		skip;
	int		second;


	if (start_idatap == NULL) {
		end_idatap = idatap = fsheadp->f_idata.id_next;
		ASSERT(idatap->id_prev == &fsheadp->f_idata);
	} else {
		end_idatap = idatap = start_idatap;
	}
	
	if (idatap == &fsheadp->f_idata)
		return(0);

	/*
	 * Here we want to select only one pool for deallocation.
	 * During our selection we do not sleep.  Thus all the
	 * pointers (forward and back) that we are currently looking 
	 * at are valid.
	 */

	second = 0;
	do {
		
		if ((idatap->id_inuse < fsheadp->f_idmin) &&
		   ((idatap->id_alloclbolt + fs_waittime) < lbolt) &&
		   ((idatap->id_flag & IDLOCKED) == 0)) {
			/*
			 * See if we can convince the inodes in this pool
			 * to be released
			 */
			fs_tryfree(idatap, delay);
		}

		if ((idatap->id_inuse == 0) &&
		   ((idatap->id_alloclbolt + fs_waittime) < lbolt) &&
		   ((idatap->id_flag & IDLOCKED) == 0)) {

			/*
			 * If we did not deallocate the pool by the second try
			 * we should remove this block and put it on the tail
			 * eg use startlbolt.
			 * We need to do this so that it does not get selected 
			 * immediately 
			 */
			if (idatap->id_startlbolt)
				second++;
			skip = fs_ipoolfree(idatap, delay);

			/*
			 * If we were not able to deallocate the pool on our
			 * second try, put on the tail so that we can find
			 * another ipool -- May be later.
			 */
			if (skip && second) {
				fs_istat.i_secskips++;
			}
			break;
		}

		idatap = idatap->id_next;
	} while (idatap != end_idatap);


	if (fs_check_idata(fsheadp))
		fsheadp->f_flag |= DEALLOC;
	else 
		fsheadp->f_flag &= ~DEALLOC;
}

fs_cleanall(fsheadp)
struct	fshead	*fsheadp;
{
	struct	idata	*idatap;
	struct	idata	*nidatap;
	int		skip;


	skip = 0;
	idatap = fsheadp->f_idata.id_next;
	if (idatap == &fsheadp->f_idata)
		return (0);

	do {
		while (idatap->id_flag & IDLOCKED) {
			idatap->id_flag |= IDWANT;
			sleep((caddr_t)idatap, PINOD);
		}

		if (fs_ipoolfree(idatap, 0)) {
			/*
			 * This should not happen -- But ???
			 */
			cmn_err(CE_WARN, 
			    "Cannot deallocate fshead 0x%x, idatap 0x%x resources\n",
			    fsheadp, idatap);
			skip++;
			idatap = idatap->id_next;
		} else
			idatap = fsheadp->f_idata.id_next;
			
	} while (idatap != &fsheadp->f_idata);

	/*
	 * Let the caller know if were able to free all all of the 
	 * pools
	 */
	return(skip);
}

fs_ipoolfree(idatap, delay)
struct	idata	*idatap;
int		delay;
{
	struct	fshead	*fsheadp;
	char		*nip;
	int		skip;
	int		i;
	clock_t		starttime;

	fsheadp = idatap->id_fshead;

	ASSERT(idatap != &(idatap->id_fshead)->f_idata);
	ASSERT(!(idatap->id_flag & IDLOCKED));

	if (idatap == &(idatap->id_fshead)->f_idata)
		return(0);

	idatap->id_flag |= IDLOCKED;
	starttime = lbolt;
	fs_istat.i_attempts++;

	if (!idatap->id_startlbolt)
		idatap->id_startlbolt = lbolt;
		
	skip = 0;
	nip = (char *)idatap + sizeof (struct idata);
	for (i = 0; i < idatap->id_total; i++) {
		if (delay && ((lbolt - starttime) > delay)) {
			fs_istat.i_dskips++;
			skip++;
			break;
		}
				
		/*
		 * Skip inodes that have been removed from the freelist
		 */
		if (IPOOL_TODATA(nip) == NULL) {
			nip += fsheadp->f_isize;
			continue;
		}

		/* 
		 * The cleanup function will indicate that we 
		 * cannot release this inode yet because 
		 * it is in use.
		 */
		if ((*fsheadp->f_inode_cleanup)(nip)) {
			skip++;
			nip += fsheadp->f_isize;
			continue;
		}

		/*
		 * Cleanup function will remove it from the filesystem
		 * free and hash lists
		 */


		INS_IFREEHEAD(&idatap->id_freelist, nip, fsheadp);
		IPOOL_TODATA(nip) = NULL;
		nip += fsheadp->f_isize;
	}

	idatap->id_flag &= ~IDLOCKED;
	if (idatap->id_flag & IDWANT) {
		idatap->id_flag &= ~IDWANT;
		wakeprocs((caddr_t)idatap, PRMPT);
	}

	if (!skip) {
		/*
		 * Remove the pool from the filesytems linked list of pools
		 */
		RM_POOL(idatap);
		kmem_free(idatap, fsheadp->f_asize);
		fsheadp->f_curr -= fsheadp->f_inum;
		fs_istat.i_numinodes -= fsheadp->f_inum;
		fs_istat.i_allocsize -= fsheadp->f_asize;
		fs_istat.i_dealloc++;
	} else {
		fs_istat.i_skips++;
	}

	return(skip);
}

STATIC
fs_tryfree(idatap, delay)
struct	idata	*idatap;
int		delay;
{
	struct	fshead	*fsheadp;
	char		*nip;
	clock_t		starttime;
	struct vnode	*vp;
	int		i;

	fsheadp = idatap->id_fshead;

	ASSERT(idatap != &(idatap->id_fshead)->f_idata);
	ASSERT(!(idatap->id_flag & IDLOCKED));

	if (idatap == &(idatap->id_fshead)->f_idata)
		return(0);

	idatap->id_flag |= IDLOCKED;
	starttime = lbolt;
	fs_istat.i_tryattempts++;
			
	nip = (char *)idatap + sizeof (struct idata);
	for (i = 0; i < idatap->id_total; i++) {
		if (delay && ((lbolt - starttime) > delay)) {
			fs_istat.i_trydskips++;
			break;
		}
				
		/*
		 * Skip inodes that have been removed from the freelist
		 */
		if (IPOOL_TODATA(nip) == NULL) {
			nip += fsheadp->f_isize;
			continue;
		}

		vp = IPOOL_TOVP(nip);
		if (vp == NULL || vp->v_count != 1) {
			nip += fsheadp->f_isize;
			continue;
		}

		dnlc_purge_vp(vp);
		nip += fsheadp->f_isize;
	}

	idatap->id_flag &= ~IDLOCKED;
	if (idatap->id_flag & IDWANT) {
		idatap->id_flag &= ~IDWANT;
		wakeprocs((caddr_t)idatap, PRMPT);
	}
}


struct ipool *
fs_iget(fshead, vfsp)
struct	fshead	*fshead;
struct	vfs	*vfsp;
{
	
	struct	ipool	*freep;
	struct	ipool	*ip;
	char		*nip;
	struct	idata	*idatap;
	struct	idata	*nidatap;
	struct vnode 	*vp;
	int		inum;
	int		i;
	int		skipped;


	freep = fshead->f_freelist;
	skipped = 0;
retry:
	if ((ip = freep->i_ff) != freep) {

		ASSERT(ip->i_fb->i_ff == ip);
		ASSERT(ip->i_ff->i_fb == ip);

		RM_IFREELIST(ip, fshead);

		vp = IPOOL_TOVP(ip);

		/*
		 * Check if we have to recycle this inode
		 * Conditions to skip recycling.
		 *
		 * 1. This is not our first attempt of skipping.
		 * 	1. Inode pool was scheduled for deallocation
		 *	2. The vnode has pages on it
		 */
		if (!skipped ) {
			if (IPOOL_TODATA(ip)->id_startlbolt) {
				INS_IFREETAIL(freep, ip, fshead);
				skipped++;
				goto retry;
			}

			if ((vp != NULL) && 
			    (vp->v_pages != NULL) &&
			    (fshead->f_curr < fshead->f_max)) {
				/*
				 * Put inode on the end of the freeplist
				 */
				INS_IFREETAIL(freep, ip, fshead);
				skipped++;
				goto alloc;
			}
		}

		/*
		 * The following code checks to be sure that putpages from the 
		 * page layer have not activated the vnode while the inode is 
		 * on the free list. If we hit this case we check if we can
		 * allocate additional inodes, otherwise, we put the inode back 
		 * on the tail of the free list and try again.
		 * If there are inodes on the free list we put it back 
		 * on the tail of the freelist and try again.  If there
		 * is only one inode on the freelist we attempt to 
		 * purge the DNLC.  If that does not work we fail the
		 * the request.
		 */
		if ((vp != NULL) && (vp->v_count > 0)) {
			if (fshead->f_curr < fshead->f_max) {
				/*
				 * Put inode on the end of the freeplist
				 */
				INS_IFREETAIL(freep, ip, fshead);
				goto alloc;
			}
			if (freep->i_ff  == freep) {
				/* Only one inode left
				 * Try purging inodes for the selected mounted
				 * filesystem.  PREEMPT() does not work if you 
				 * are the highest priority process this results
				 * in an infinite loop.
				 * Put inode back on the tail of the freeplist
				 * after we have attempted to purge the DNLC
				 */

				dnlc_purge_vfsp(vfsp, 10);
				while ((freep->i_ff == freep) && 
				       (dnlc_purge1() == 1))
					;
				if (freep->i_ff == freep) {
					INS_IFREEHEAD(freep, ip, fshead);
					fshead->f_fail++;
					return(NULL);
				}
				INS_IFREETAIL(freep, ip, fshead);
			} else {
				/*
				 * Put inode on the end of the freeplist
				 * and search for another one.
				 */
				INS_IFREETAIL(freep, ip, fshead);
			}
			goto retry;
		}
		return(ip);
	}
alloc:
	 if (fshead->f_curr > fshead->f_max) {

		/*
		 * Try purging inodes for the selected mounted filesystem
		 */
		dnlc_purge_vfsp(vfsp, 10);
		while ((freep->i_ff == freep) && (dnlc_purge1() == 1))
			;

		if (freep->i_ff == freep) {
			fshead->f_fail++;
			return(NULL);
		}
		goto retry;
	}

	/* Allocate space for an additional set of inodes */

	nidatap = fs_ipoolalloc(fshead);
	if (nidatap == NULL) {

		/*
		 * Try purging inodes for the selected mounted filesystem
		 */
		dnlc_purge_vfsp(vfsp, 10);
		while ((freep->i_ff == freep) && (dnlc_purge1() == 1))
			;

		if (freep->i_ff == freep) {
			fshead->f_fail++;
			return(NULL);
		}
		goto retry;
	}

	/* Link the inode pool to the filesystem head */
	nidatap->id_fshead = fshead;
	INS_POOLHEAD(&fshead->f_idata, nidatap);

	/* Prepare  the pool for the de-allocate list */
	nidatap->id_freelist.i_ff =  nidatap->id_freelist.i_fb =  &nidatap->id_freelist;

	fshead->f_curr += nidatap->id_total;

	/* Update inuse count so the INS_FREEHEAD() macro can decrement later */
	fshead->f_inuse += nidatap->id_total;
	nip = (char *)nidatap + sizeof (struct idata);

	for (i = 0; i < nidatap->id_total; i++) {
		/* Indicate that this is a new inode */
		IPOOL_TOVP(nip) = NULL;

		IPOOL_TODATA(nip) = nidatap;
		INS_IFREEHEAD(freep, nip, fshead);
		nip += fshead->f_isize;
	}
	goto retry;
}


fs_ipooldump(fshead)
struct fshead	*fshead;
{

	struct	idata	*idata;
	
	printf( "freel 0x%x, isize %d, asize %d, inum %d, idmin %d max %d, curr %d frag %d\n",
		fshead->f_freelist, fshead->f_isize, fshead->f_asize, 
		fshead->f_inum, fshead->f_idmin, fshead->f_max, fshead->f_curr, 
		fshead->f_frag);

	idata = fshead->f_idata.id_next;
	while (idata != &fshead->f_idata) {
		printf( "idata 0x%x, inuse %d\n", idata, idata->id_inuse);
		idata = idata->id_next;
	}
}
