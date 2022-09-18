/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/specfs/specsec.c	1.9.2.2"
#ident	"$Header: $"
#include <acc/mac/mac.h>
#include <fs/specfs/devmac.h>
#include <fs/specfs/snode.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/* 
 * This file contains all the routines used by the
 * enhanced  security system calls and other
 * calls within SPECFS to perform security access checks.
 */


	

/*
 * This routine is called to copy the security attributes from one
 * snode to another.  It is assumed that the target sp security pointer
 * is null. This routine is called in spec_open when a new clone is created. 
 * It increments the reference count on the snode and its common.
 */
void
devsec_dcidup(sp, tsp)
	struct snode *sp;
	struct snode *tsp;

{
	struct devmac *ksecp;
	tsp->s_dstate = sp->s_dstate;
	tsp->s_dmode = sp->s_dmode;
	tsp->s_secflag = sp->s_secflag;
	if (sp->s_dsecp != NULL){
		ksecp =(struct devmac *)kmem_zalloc(sizeof(*ksecp), KM_SLEEP);
		*ksecp = *(sp->s_dsecp);
		VN_HOLD(STOV(tsp));
		VN_HOLD((tsp)->s_commonvp);
		tsp->s_dsecp = ksecp;
		tsp->s_dsecp->d_relflag = DEV_LASTCLOSE;
	}
}

/* 
 * This routine is called in spec_open to handle clone devices.
 * In addition to calling makespecvp to create a new snode,
 * the routine also copies the security attributes of a cloneable device
 * to the newly created clone.
 */

struct vnode *
devsec_cloneopen(vp, newdev, type)
	struct vnode *vp;
	register dev_t newdev;
	register vtype_t type;

{
	struct vnode *newvp;
	struct snode *sp= VTOS(vp);
	struct snode *newsp;
	struct snode *csp = VTOS((VTOS(vp))->s_commonvp);

	if ((newvp = makespecvp(newdev,type)) == NULL)
		return NULL;
	/* copy information to the newly created clone */
	newvp->v_lid = vp->v_lid;
	newsp= VTOS(newvp);
	devsec_dcidup(sp, newsp);
	return(newvp);
}

/*
 * This routine initializes the state of a device by populating
 * the snode s_dstate field.
 */

void
devset_state(sp)
	struct snode *sp;

{
	if (sp->s_secflag & D_INITPUB)	
		sp->s_dstate = DEV_PUBLIC;
	else	sp->s_dstate = DEV_PRIVATE;
}

/* 
 * This routine frees the security attributes of a device,
 * and resets the device attributes to system settings.
 * It decrements the reference count on the vnode inside
 * the snode and the vnode inside its common snode.
 * It expects sp->s_dsecp not to be  NULL.
 */
void 
devsec_dcifree(sp)
	struct snode *sp;
{
	struct devmac *locbufp;
	struct vnode *cvp=(sp)->s_commonvp;


	ASSERT(sp->s_dsecp != NULL);
	locbufp = sp->s_dsecp;
	sp->s_dsecp = NULL;
	kmem_free((caddr_t)locbufp, sizeof(*locbufp));
	devset_state(sp);
	devset_state(VTOS(cvp));
	VN_RELE(STOV(sp));
	VN_RELE(cvp);

}

/*
 * This routine is called on the last close of a device.
 * If the security attributes were set to last close,
 * then they have to be released.
 */

void
devsec_close(sp)
	struct snode *sp;
	
{
	if (REL_FLAG(sp) == DEV_LASTCLOSE) 
		devsec_dcifree(sp);
}


/* 
 * This routine creates a new device security structure and increments the
 * reference count on the snode and its common.
 * It assumes the snode security pointer is null.
 */
void
devsec_dcicreat(sp)
	struct snode *sp;

{
	struct devmac *ksecp;

	ASSERT(sp->s_dsecp == NULL);

	sp->s_dsecp =(struct devmac *)kmem_zalloc(sizeof(*ksecp), KM_SLEEP);
	VN_HOLD(STOV(sp));
	VN_HOLD((sp)->s_commonvp);
}
