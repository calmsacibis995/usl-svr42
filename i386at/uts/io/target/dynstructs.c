/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/dynstructs.c	1.6"
#ident  "$Header: $"

#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/param.h>
#include <proc/cred.h>
#include <io/ddi.h>
#include <io/target/dynstructs.h>


int waittime = MIN_WAITTIME;

void
sdi_free(headp, jp)
struct	head	*headp;
struct	jpool	*jp;
{
	struct	jpool	*freelistp;
	int	prevpl;

	freelistp = headp->f_freelist;

	prevpl = spl5();
	if (POOL_TODATA(jp)->jd_inuse < headp->f_idmin) {
		INS_FREETAIL(freelistp, jp);
	} else {
		INS_FREEHEAD(freelistp, jp);
	}
	splx(prevpl);

	drv_getparm(LBOLT, (ulong *)&POOL_TODATA(jp)->jd_lastuse);
	if (headp->f_flag & POOLWANT) {
		wakeup((caddr_t)headp);
		headp->f_flag &= ~POOLWANT;
	}
	else {
		prevpl = spl5();
		/* If multiple clusters allocated see if we can free any */
		if (headp->f_curr > headp->f_inum)
			cleanone(headp, jp->j_data);
		splx(prevpl);
	}

}

void
sdi_poolinit(headp)
struct	head	*headp;
{
	int	asize;
	int	isize;
	int	inum;
	int	frag;

	isize = headp->f_isize;
	asize = headp->f_maxpages * PAGESIZE;
	inum = (asize - sizeof (struct jdata))/ isize;
	frag = asize - (sizeof (struct jdata) + inum * isize);

	/*
	 * The head contents should be zero'ed out in the
	 * filesystem dependent code and then initialized
	 * with filesystem specific data.
	 */
	headp->f_asize = asize;
	headp->f_inum = inum;
	headp->f_idmin = (inum*115)/100 - inum; /* 15% */
	headp->f_frag = frag;
	headp->f_jdata.jd_next = headp->f_jdata.jd_prev = &headp->f_jdata;
	headp->f_jdata.jd_head = headp;

	/* To prevent it from being selected for deallocation */
	headp->f_jdata.jd_inuse = 0x7fff;

	ASSERT(headp->f_frag >= 0);
#ifdef DEBUG
	printf( "freel 0x%x, isize %d, asize %d, inum %d, idmin, %d frag %d\n",
		headp->f_freelist, headp->f_isize, headp->f_asize, 
		headp->f_inum, headp->f_idmin, headp->f_frag);
#endif

}

struct jdata *
poolalloc(headp)
struct	head *headp;
{
	struct	jdata	*jdatap;
	int		asize;
	int		inum;

	asize = headp->f_asize;
	inum = headp->f_inum;

	if ((jdatap  = (void *)kmem_zalloc(asize, KM_NOSLEEP)) == NULL) {
		return (NULL);
	}

	drv_getparm(LBOLT, (ulong *)&jdatap->jd_lastuse);
	jdatap->jd_total = inum;
	/* jd_inuse is set to inum since it gets decremented as structs */
	/* added to freelist.						*/
	jdatap->jd_inuse = inum;

	return(jdatap);
}

/* cleanone looks for a pool which can be deallocated. The pool must have */
/* been around longer than deallocate time and have no structs in use.    */
/* This routine is protected by its caller 				  */
cleanone(headp, start_jdatap)
struct	head	*headp;
struct	jdata	*start_jdatap;
{
	struct	jdata	*jdatap;
	struct	jdata	*end_jdatap;
	char		*jp;
	clock_t		curtime;

	if (start_jdatap == NULL) {
		end_jdatap = jdatap = headp->f_jdata.jd_next;
		ASSERT(jdatap->jd_prev == &headp->f_jdata);
	} else {
		end_jdatap = jdatap = start_jdatap;
	}
	
	if (jdatap == &headp->f_jdata)
		return(0);

	/*
	 * Here we want to select only one pool for deallocation.
	 * During our selection we do not sleep.  Thus all the
	 * pointers (forward and back) that we are currently looking 
	 * at are valid.
	 */

	drv_getparm(LBOLT, (ulong *)&curtime);
	do {
		if ((jdatap->jd_inuse == 0) &&
		   ((jdatap->jd_lastuse + waittime) < curtime)) {
			poolfree(jdatap);
			break;
		}
		jdatap = jdatap->jd_next;
	} while (jdatap != end_jdatap);
}

poolfree(jdatap)
struct	jdata	*jdatap;
{
	struct	head	*headp;
	char		*njp;
	int 		i;

	headp = jdatap->jd_head;
	ASSERT(jdatap != &(jdatap->jd_head)->f_jdata);
	if (jdatap == &(jdatap->jd_head)->f_jdata)
		return(0);

	/* take structs off freelist */
	njp = (char *)jdatap + sizeof (struct jdata);
	for (i = 0; i < jdatap->jd_total; i++) {
		RM_FREELIST(njp);
		njp += headp->f_isize;
	}
	/*
	 * Remove the pool from the linked list of pools
	 */
	RM_POOL(jdatap);
	kmem_free(jdatap, headp->f_asize);
	headp->f_curr -= headp->f_inum;
	return(0);
}

struct jpool *
sdi_get(headp, flag)
struct	head	*headp;
int		flag;
{
	
	struct	jpool	*freep;
	struct	jpool	*jp;
	struct	jdata	*save_jdata;
	char		*njp;
	struct	jdata	*jdatap;
	struct	jdata	*njdatap;
	struct vnode 	*vp;
	int		prevpl;
	int		i;

	prevpl = spl5();
	freep = headp->f_freelist;
retry:
	if ((jp = freep->j_ff) != freep) {
		ASSERT(jp->j_fb->j_ff == jp);
		ASSERT(jp->j_ff->j_fb == jp);
		RM_FREELIST(jp);
		splx(prevpl);
		save_jdata = jp->j_data;
		bzero((caddr_t)jp, (size_t)headp->f_isize);
		jp->j_data = save_jdata;
		return(jp);
	}

	/* Allocate space for an additional set of inodes */
	njdatap = poolalloc(headp);
	if (njdatap == NULL) {
		headp->f_flag |= POOLWANT;
		sleep((caddr_t)headp,PRIBIO);
		goto retry;
	}

	/* Link the inode pool to the filesystem head */
	njdatap->jd_head = headp;
	INS_POOLHEAD(&headp->f_jdata, njdatap);
	headp->f_curr += njdatap->jd_total;

	/* Update inuse count so the INS_FREEHEAD() macro can decrement later */
	njp = (char *)njdatap + sizeof (struct jdata);

	for (i = 0; i < njdatap->jd_total; i++) {
		POOL_TODATA(njp) = njdatap;
		INS_FREEHEAD(freep, njp);
		njp += headp->f_isize;
	}
	goto retry;
}

#ifdef DEBUG
pooldump(headp)
struct head	*headp;
{

	struct	jdata	*jdata;
	
	printf( "freel 0x%x, isize %d, asize %d, inum %d, idmin %d curr %d frag %d\n",
		headp->f_freelist, headp->f_isize, headp->f_asize, 
		headp->f_inum, headp->f_idmin, headp->f_curr, headp->f_frag);

	jdata = headp->f_jdata.jd_next;
	while (jdata != &headp->f_jdata) {
		printf( "jdata 0x%x, inuse %d\n", jdata, jdata->jd_inuse);
		jdata = jdata->jd_next;
	}
}
#endif
