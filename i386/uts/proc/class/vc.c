/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/class/vc.c	1.4"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/class.h>
#include <proc/class/vc.h>
#include <proc/class/vcpriocntl.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Class specific code for the VP/ix process class
 *
 * This code is derived from the time-sharing class code
 * with changes made where the peculiarities of VP/ix
 * processes render the generic time-sharing algorithms
 * inappropriate.
 */

/*
 * Extern declarations for variables defined in the vc master file
 */
extern vcdpent_t  vc_dptbl[];	/* VP/ix disp parameter table */
extern int	vc_kmdpris[];	/* array of global pris used by vc procs when */
				/*  sleeping or running in kernel after sleep */
extern short	vc_maxupri;	/* max VP/ix user priority */
extern short	vc_maxumdpri;	/* maximum user mode vc priority */
extern short	vc_maxkmdpri;	/* maximum kernel mode vc priority */

#define	VCPMEMSZ	 2048	/* request size for vcproc memory allocation */

#define	pstat		(*vcpp->vc_pstatp)
#define	ppri		(*vcpp->vc_pprip)
#define	pflag		(*vcpp->vc_pflagp)
#define	vcumdpri	(vcpp->vc_umdpri)
#define	vcmedumdpri	(vc_maxumdpri >> 1)

/*  VC_NEWUMDPRI is similar to TS_NEWUMDPRI except that we forbid both
 *  cpupri and umdpri from taking on the reserved busywait priorities
 *  that end in the digits 0, 1, 2.  We have to restrict both cpupri and
 *  umdpri because they are both used as indices into the dispatch table.
 *  VC_NEWUMDPRI should never be applied to a busywaiting process.
 */
#define	VC_NEWUMDPRI(vcpp)	\
{ \
int tmp; \
if ((tmp = (vcpp)->vc_cpupri % 10) <= 2) \
	(vcpp)->vc_cpupri += 3 - tmp; \
if (((vcpp)->vc_umdpri = (vcpp)->vc_cpupri + (vcpp)->vc_upri) > vc_maxumdpri) \
	(vcpp)->vc_umdpri = vc_maxumdpri; \
else if ((vcpp)->vc_umdpri < 0) \
	(vcpp)->vc_umdpri = 0; \
if ((tmp = (vcpp)->vc_umdpri % 10) <= 2) \
	(vcpp)->vc_umdpri += 3 - tmp; \
}


void		vc_boost(), vc_init();
STATIC int	vc_admin(), vc_enterclass(), vc_fork(), vc_getclinfo();
STATIC int	vc_nosys(), vc_parmsin(), vc_parmsout(), vc_parmsset();
STATIC int	vc_proccmp();
STATIC void	vc_exitclass(), vc_forkret(), vc_getglobpri();
STATIC void	vc_nullsys(), vc_parmsget(), vc_preempt();
STATIC void	vc_setrun(), vc_sleep(), vc_swapin(), vc_swapout();
STATIC void	vc_tick(), vc_trapret(), vc_update(), vc_wakeup();


STATIC id_t	vc_cid;		/* VP/ix class ID */
STATIC int	vc_maxglobpri;	/* maximum global priority used by vc class */
STATIC vcproc_t	vc_plisthead;	/* dummy vcproc at head of vcproc list */
STATIC caddr_t	vc_pmembase;	/* base addr of memory allocated for vcprocs */
STATIC boolean_t vc_updactive = B_FALSE;	/* vc_update activity flag */


STATIC struct classfuncs vc_classfuncs = {
	vc_admin,
	vc_enterclass,
	vc_exitclass,
	vc_fork,
	vc_forkret,
	vc_getclinfo,
	vc_getglobpri,
	vc_parmsget,
	vc_parmsin,
	vc_parmsout,
	vc_parmsset,
	vc_preempt,
	vc_proccmp,
	vc_setrun,
	vc_sleep,
	vc_nullsys,
	vc_swapin,
	vc_swapout,
	vc_tick,
	vc_trapret,
	vc_wakeup,
	vc_nullsys,
	vc_nullsys,
	vc_nosys,
	vc_nosys,
	vc_nosys,
	vc_nosys,
	vc_nosys,
	vc_nosys,
	vc_nosys,
	vc_nosys,
	vc_nosys
};


/*
 * VP/ix class initialization.  Called by dispinit() at boot time.
 * We can ignore the clparmsz argument since we know that the smallest
 * possible parameter buffer is big enough for us.
 */
/* ARGSUSED */
void
vc_init(cid, clparmsz, clfuncspp, maxglobprip)
id_t		cid;
int		clparmsz;
classfuncs_t	**clfuncspp;
int		*maxglobprip;
{
	vc_maxglobpri = max(vc_kmdpris[vc_maxkmdpri],
	    vc_dptbl[vc_maxumdpri].vc_globpri);

	vc_cid = cid;		/* Record our class ID */

	/*
	 * Initialize the vcproc list.
	 */
	vc_plisthead.vc_next = vc_plisthead.vc_prev = &vc_plisthead;

	/*
	 * We're required to return a pointer to our classfuncs
	 * structure and the highest global priority value we use.
	 */
	*clfuncspp = &vc_classfuncs;
	*maxglobprip = vc_maxglobpri;
}


/*
 * Get or reset the vc_dptbl values per the user's request.
 */
/* ARGSUSED */
STATIC int
vc_admin(uaddr, reqpcid, reqpcredp)
caddr_t	uaddr;
id_t	reqpcid;
cred_t	*reqpcredp;
{
	vcadmin_t		vcadmin;
	register vcdpent_t	*tmpdpp;
	register int		userdpsz;
	register int		i;
	register int		vcdpsz;
	int			oldlvl;

	if (copyin(uaddr, (caddr_t)&vcadmin, sizeof(vcadmin_t))) {
		return(EFAULT);
	}

	vcdpsz = (vc_maxumdpri + 1) * sizeof(vcdpent_t);

	switch(vcadmin.vc_cmd) {

	case VC_GETDPSIZE:

		vcadmin.vc_ndpents = vc_maxumdpri + 1;
		if (copyout((caddr_t)&vcadmin, uaddr, sizeof(vcadmin_t)))
			return(EFAULT);
		break;

	case VC_GETDPTBL:

		userdpsz = MIN(vcadmin.vc_ndpents * sizeof(vcdpent_t), vcdpsz);
		if (copyout((caddr_t)vc_dptbl,
		    (caddr_t)vcadmin.vc_dpents, userdpsz))
			return(EFAULT);

		vcadmin.vc_ndpents = userdpsz / sizeof(vcdpent_t);
		if (copyout((caddr_t)&vcadmin, uaddr, sizeof(vcadmin_t)))
			return(EFAULT);

		break;

	case VC_SETDPTBL:

		/*
		 * We require that the requesting process have privilege.
		 * We also require that the table supplied by the user
		 * exactly match the current vc_dptbl in size.
		 */
		if (pm_denied(reqpcredp, P_SYSOPS)
		    && pm_denied(reqpcredp, P_TSHAR)) {
			return(EPERM);
		}

		if (vcadmin.vc_ndpents * sizeof(vcdpent_t) != vcdpsz) {
			return(EINVAL);
		}

		/*
		 * We read the user supplied table into a temporary buffer
		 * where it is validated before being copied over the
		 * vc_dptbl.
		 */
		tmpdpp = (vcdpent_t *)kmem_alloc(vcdpsz, KM_SLEEP);
		ASSERT(tmpdpp != NULL);
		if (copyin((caddr_t)vcadmin.vc_dpents, (caddr_t)tmpdpp,
		    vcdpsz)) {
			kmem_free(tmpdpp, vcdpsz);
			return(EFAULT);
		}
		for (i = 0; i < vcadmin.vc_ndpents; i++) {

			/*
			 * Validate the user supplied values.  All we are doing
			 * here is verifying that the values are within their
			 * allowable ranges and will not panic the system.  We
			 * make no attempt to ensure that the resulting
			 * configuration makes sense or results in reasonable
			 * performance.
			 */
			if (tmpdpp[i].vc_quantum <= 0) {
				kmem_free(tmpdpp, vcdpsz);
				return(EINVAL);
			}
			if (tmpdpp[i].vc_tqexp > vc_maxumdpri ||
			    tmpdpp[i].vc_tqexp < 0) {
				kmem_free(tmpdpp, vcdpsz);
				return(EINVAL);
			}
			if (tmpdpp[i].vc_slpret > vc_maxumdpri ||
			    tmpdpp[i].vc_slpret < 0) {
				kmem_free(tmpdpp, vcdpsz);
				return(EINVAL);
			}
			if (tmpdpp[i].vc_maxwait < 0) {
				kmem_free(tmpdpp, vcdpsz);
				return(EINVAL);
			}
			if (tmpdpp[i].vc_lwait > vc_maxumdpri ||
			    tmpdpp[i].vc_lwait < 0) {
				kmem_free(tmpdpp, vcdpsz);
				return(EINVAL);
			}
		}

		/*
		 * Copy the user supplied values over the current vc_dptbl
		 * values.  The vc_globpri member is read-only so we don't
		 * overwrite it.
		 */
		oldlvl = splhi();
		for (i = 0; i < vcadmin.vc_ndpents; i++) {
			vc_dptbl[i].vc_quantum = tmpdpp[i].vc_quantum;
			vc_dptbl[i].vc_tqexp = tmpdpp[i].vc_tqexp;
			vc_dptbl[i].vc_slpret = tmpdpp[i].vc_slpret;
			vc_dptbl[i].vc_maxwait = tmpdpp[i].vc_maxwait;
			vc_dptbl[i].vc_lwait = tmpdpp[i].vc_lwait;
		}
		splx(oldlvl);
		kmem_free(tmpdpp, vcdpsz);
		break;

	default:
		return(EINVAL);
	}
	return(0);
}

/*
 *  This routine is called from v86setint() to boost the priority of a
 *  VP/ix process in response to a pseudorupt.
 */
void
vc_boost(pp, urgent)
proc_t		*pp;
boolean_t	urgent;
{
	register vcproc_t	*vcpp;
	register boolean_t	wasonq;
	short			newpri;

	/* Do nothing if not a VP/ix process */
	if (pp->p_cid != vc_cid)
		return;

	/* Find the class-specific process structure */
	vcpp = pp->p_clproc;

	/* Remember that we boosted the process */
	vcpp->vc_flags |= VCBOOST;

	/* Turn off any busywait indication */
	vcpp->vc_flags &= ~VCBSYWT;

	/* Calculate the priority we are planning to give the process.
	 * For urgent events use the maximum class priority.  For others
	 * use the next highest.
	 */
	newpri = (urgent ? vc_maxumdpri : vc_maxumdpri - 1);

	/*  If process is at kernel priority it already has higher
	 *  priority than newpri.  Set the user mode priority for
	 *  use when returning to user mode.
	 */
	if (vcpp->vc_flags & VCKPRI) {
		vcpp->vc_umdpri = newpri;
		return;
	}

	/* Return immediately if priority would not change.  Otherwise
	 * process would move to the back of the queue it is already on.
	 */
	if (newpri == vcpp->vc_umdpri)
		return;

	/* Remove process from dispatcher queue if it is on one.
	 * Remember whether it was.
	 */
	wasonq = dispdeq(pp);

	/* Assign the new priority.
	 */
	vcpp->vc_umdpri = newpri;
	*vcpp->vc_pprip = vc_dptbl[newpri].vc_globpri;
	if (pp == curproc)
		curpri = newpri;

	/* Put process back on a queue if we got it off one */
	if (wasonq)
		setbackdq(pp);

	/*  If current process is lower priority than the newly
	 *  boosted one, request a process switch.
	 */
	if (curpri < newpri)
		runrun++;
}


vc_busywait(pp, level)
proc_t		*pp;
int		level;
{
	register vcproc_t	*vcpp;
	register boolean_t	wasonq;
	short			newpri;

	ASSERT(pp->p_cid == vc_cid);

	/* Find the class-specific process structure */
	vcpp = pp->p_clproc;

	if (level) {
		/*  ECT has indicated that process is busywaiting.
		 *  Turn on busywait flag.  If we are already at the
		 *  appropriate busywait level, do nothing.
		 */
		vcpp->vc_flags |= VCBSYWT;
		if (vcpp->vc_umdpri % 10 == level - 1)
			return;
		newpri = level - 1;
	}
	else {
		if ((vcpp->vc_flags & VCBSYWT) == 0)
			return;
		vcpp->vc_flags &= ~VCBSYWT;
	}

	/* Remove process from dispatcher queue if it is on one.
	 * Remember whether it was.
	 */
	wasonq = dispdeq(pp);

	/* Assign the new priority.
	 */
	if (vcpp->vc_flags & VCBSYWT)
		vcpp->vc_umdpri = newpri;
	else {
		VC_NEWUMDPRI(vcpp);
		newpri = vcpp->vc_umdpri;
	}

	*vcpp->vc_pprip = vc_dptbl[newpri].vc_globpri;
	if (pp == curproc)
		curpri = newpri;

	/* Put process back on a queue if we got it off one */
	if (wasonq)
		setbackdq(pp);
}


/*
 * Allocate a VP/ix class specific proc structure and
 * initialize it with the parameters supplied. Also move process
 * to suitable VP/ix priority.
 */
/* ARGSUSED */
STATIC int
vc_enterclass(vcparmsp, pp, pstatp, pprip, pflagp, pcredpp, vcprocpp,
							reqpcid, reqpcredp)
vcparms_t	*vcparmsp;
proc_t		*pp;
char		*pstatp;
int		*pprip;
uint		*pflagp;
cred_t		**pcredpp;
vcproc_t	**vcprocpp;
id_t		reqpcid;
cred_t		*reqpcredp;
{
	register vcproc_t	*vcpp;
	register short		reqvcuprilim;
	register short		reqvcupri;
	register int 		oldlvl;
	register boolean_t	wasonq;

	/*  Although VP/ix class processes have some aspects of real
	 *  time processes, unlike the real time process class we do
	 *  not force the process to be loaded at this point.
	 *  Typically it will be loaded because we are running v86init()
	 *  in the process itself.  Even if future development introduces
	 *  a new way to create a VP/ix process, it would probably still
	 *  not be necessary to force the process in.
	 */


	if ((vcpp = (vcproc_t *)kmem_fast_alloc(&vc_pmembase, sizeof(vcproc_t),
	    VCPMEMSZ / sizeof(vcproc_t), KM_NOSLEEP)) == NULL) {
		return(ENOMEM);
	}


	/*
	 * Initialize the vcproc structure.
	 */
	if (vcparmsp == NULL) {
		/*
		 * Use default values.
		 */
		vcpp->vc_uprilim = vcpp->vc_upri = 0;
		vcpp->vc_nice = 20;
		vcpp->vc_umdpri = vcpp->vc_cpupri = vcmedumdpri;
	} else {
		/*
		 * Use supplied values.
		 */
		if (vcparmsp->vc_uprilim == VC_NOCHANGE)
				reqvcuprilim = 0;
		else
			reqvcuprilim = vcparmsp->vc_uprilim;

		/*
		 * In order to set an initial upri limit greater than
		 * zero the requesting process must be privileged.
		 * This may have been checked previously but if our
		 * caller passed us a credential structure we assume
		 * it hasn't and we check it here.
		 */
		if (reqpcredp != NULL && reqvcuprilim > 0 &&
		    pm_denied(reqpcredp, P_TSHAR)) {
			kmem_fast_free(&vc_pmembase, (caddr_t)vcpp);
			return(EPERM);
		}

		if (vcparmsp->vc_upri == VC_NOCHANGE) {
			reqvcupri = reqvcuprilim;
		} else {
			/*
			 * Set the user priority to the requested value
			 * or the upri limit, whichever is lower.
			 */
			reqvcupri = vcparmsp->vc_upri;
			if (reqvcupri > reqvcuprilim)
				reqvcupri = reqvcuprilim;
		}

		vcpp->vc_uprilim = reqvcuprilim;
		vcpp->vc_upri = reqvcupri;
		vcpp->vc_nice = 20 - (20 * reqvcupri) / vc_maxupri;
		vcpp->vc_cpupri = vcmedumdpri;
		VC_NEWUMDPRI(vcpp);
	}

	vcpp->vc_dispwait = 0;
	vcpp->vc_flags = 0;
	vcpp->vc_procp = pp;
	vcpp->vc_pstatp = pstatp;
	vcpp->vc_pprip = pprip;
	vcpp->vc_pflagp = pflagp;


	/*  If the VP/ix priority update daemon is not running, start it */
	if (vc_updactive == B_FALSE) {
		vc_updactive = B_TRUE;
		(void)timeout(vc_update, 0, HZ);
	}

	/*
	 * Link new structure into vcproc list.
	 */
	vcpp->vc_next = vc_plisthead.vc_next;
	vcpp->vc_prev = &vc_plisthead;
	vc_plisthead.vc_next->vc_prev = vcpp;
	vc_plisthead.vc_next = vcpp;


	/*
	 * Reset priority. Process goes to a "user mode" priority
	 * here regardless of whether or not it has slept since
	 * entering the kernel.
	 */
	oldlvl = splhi();

	if (vcpp->vc_procp == curproc) {
		if ((curpri = *vcpp->vc_pprip =
		    vc_dptbl[vcumdpri].vc_globpri) > maxrunpri)
			vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
		else {
			vcpp->vc_flags |= VCBACKQ;
			runrun++;
		}
	} else {
		wasonq = dispdeq(pp);
		*vcpp->vc_pprip = vc_dptbl[vcumdpri].vc_globpri;
		if (wasonq == B_TRUE) {
			if (*vcpp->vc_pprip > curpri)
				runrun++;
			vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
			setbackdq(pp);
		} else
			vcpp->vc_flags |= VCBACKQ;
	}
	*vcprocpp = vcpp;
	splx(oldlvl);

	return(0);
}


/*
 * Free vcproc structure of process.
 */
STATIC void
vc_exitclass(vcprocp)
vcproc_t	*vcprocp;
{
	register int 		oldlvl;

	oldlvl = splhi();
	vcprocp->vc_prev->vc_next = vcprocp->vc_next;
	vcprocp->vc_next->vc_prev = vcprocp->vc_prev;
	splx(oldlvl);

	kmem_fast_free(&vc_pmembase, (caddr_t)vcprocp);
}


/*
 * Allocate and initialize VP/ix class specific
 * proc structure for child.
 */
/* ARGSUSED */
STATIC int
vc_fork(pvcpp, cprocp, cpstatp, cpprip, cpflagp, cpcredpp, vcprocpp)
register vcproc_t	*pvcpp;
proc_t			*cprocp;
char			*cpstatp;
int			*cpprip;
uint			*cpflagp;
cred_t			**cpcredpp;
vcproc_t		**vcprocpp;
{
	register vcproc_t	*cvcpp; /* ptr to child's vcproc structure */


	cvcpp = (vcproc_t *)kmem_fast_alloc(&vc_pmembase, sizeof(vcproc_t),
	    VCPMEMSZ / sizeof(vcproc_t), KM_SLEEP);
	ASSERT(cvcpp != NULL);


	/*
	 * Initialize child's vcproc structure.
	 */
	cvcpp->vc_timeleft = vc_dptbl[pvcpp->vc_umdpri].vc_quantum;
	cvcpp->vc_umdpri = pvcpp->vc_umdpri;
	cvcpp->vc_cpupri = pvcpp->vc_cpupri;
	cvcpp->vc_uprilim = pvcpp->vc_uprilim;
	cvcpp->vc_upri = pvcpp->vc_upri;
	cvcpp->vc_nice = pvcpp->vc_nice;
	cvcpp->vc_dispwait = 0;
	cvcpp->vc_flags = pvcpp->vc_flags & ~VCBACKQ;
	cvcpp->vc_procp = cprocp;
	cvcpp->vc_pstatp = cpstatp;
	cvcpp->vc_pprip = cpprip;
	cvcpp->vc_pflagp = cpflagp;

	/*
	 * Link structure into vcproc list.
	 */
	cvcpp->vc_next = vc_plisthead.vc_next;
	cvcpp->vc_prev = &vc_plisthead;
	vc_plisthead.vc_next->vc_prev = cvcpp;
	vc_plisthead.vc_next = cvcpp;
	
	*vcprocpp = cvcpp;
	return(0);
}


/*
 * Child is placed at back of dispatcher queue and parent gives
 * up processor so that the child runs first after the fork.
 * This allows the child immediately execing to break the multiple
 * use of copy on write pages with no disk home. The parent will
 * get to steal them back rather than uselessly copying them.
 */
STATIC void
vc_forkret(cvcpp, pvcpp)
vcproc_t		*cvcpp;
register vcproc_t	*pvcpp;
{
	id_t			tsclass;
	caddr_t			clprocp;
	register int 		oldlvl;
	register proc_t		*cp = cvcpp->vc_procp;

	/*  If time-sharing process class is properly configured and
	 *  we can successfully enroll this process in it, switch it
	 *  to time-sharing.  Then run the time-sharing equivalent of
	 *  this routine.  We know that the time-sharing routine will
	 *  not try to switch us back to VP/ix, so infinite recursion
	 *  cannot occur.
	 */
	if (getcid("TS", &tsclass) == 0 && tsclass > 0 &&
			CL_ENTERCLASS(&class[tsclass], NULL, cp, &cp->p_stat,
			&cp->p_pri, &cp->p_flag, &cp->p_cred,
			&clprocp, NULL, NULL) == 0) {

		/* Withdraw process from old class and update proc table */
		oldlvl = splhi();
		CL_EXITCLASS(cp, cp->p_clproc);
		cp->p_cid = tsclass;
		cp->p_clfuncs = class[tsclass].cl_funcs;
		cp->p_clproc = clprocp;
		splx(oldlvl);
		CL_FORKRET(cp, cp->p_clproc, NULL);
		return;
	}

	/*  For some reason we were unable to switch the process back
	 *  to time-sharing.  There is no provision for failure from this
	 *  routine, so just let the process remain as a VP/ix one.  We
	 *  do not really expect this to happen, so maybe a warning would
	 *  be appropriate.
	 */  

	cmn_err(CE_WARN, "vc_forkret: failed to enter a proc inthe TS class");

	setbackdq(cvcpp->vc_procp);

	if (pvcpp != NULL) {
		pvcpp->vc_flags |= (VCBACKQ|VCFORK);
		runrun++;
	}
}


/*
 * Get information about the VP/ix class into the buffer
 * pointed to by vcinfop. The maximum configured user priority
 * is the only information we supply.  We ignore the class and
 * credential arguments because anyone can have this information.
 */
/* ARGSUSED */
STATIC int
vc_getclinfo(vcinfop, reqpcid, reqpcredp)
vcinfo_t	*vcinfop;
id_t		reqpcid;
cred_t		*reqpcredp;
{
	vcinfop->vc_maxupri = vc_maxupri;
	return(0);
}


/*
 * Return the global scheduling priority that would be assigned
 * to a process entering the VP/ix class with the vc_upri
 * value specified in the vcparms buffer.
 */
STATIC void
vc_getglobpri(vcparmsp, globprip)
vcparms_t	*vcparmsp;
int		*globprip;
{
	register int	vcpri;

	vcpri = vcmedumdpri + vcparmsp->vc_upri;
	if (vcpri > vc_maxumdpri)
		vcpri = vc_maxumdpri;
	else if (vcpri < 0)
		vcpri = 0;
	*globprip = vc_dptbl[vcpri].vc_globpri;
}


STATIC int
vc_nosys()
{
	return(ENOSYS);
}


STATIC void
vc_nullsys()
{
}


/*
 * Get the VP/ix parameters of the process pointed to by
 * vcprocp into the buffer pointed to by vcparmsp.
 */
STATIC void
vc_parmsget(vcprocp, vcparmsp)
vcproc_t	*vcprocp;
vcparms_t	*vcparmsp;
{
	vcparmsp->vc_uprilim = vcprocp->vc_uprilim;
	vcparmsp->vc_upri = vcprocp->vc_upri;
}


/*
 * Check the validity of the VP/ix parameters in the buffer
 * pointed to by vcparmsp. If our caller passes us a non-NULL
 * reqpcredp pointer we also verify that the requesting process
 * (whose credentials are pointed to by reqpcredp) has the necessary
 * permissions to set these parameters for the target process.
 */
/* ARGSUSED */
STATIC int
vc_parmsin(vcparmsp, reqpcid, reqpcredp, targpcid, targpcredp, vcpp)
register vcparms_t	*vcparmsp;
id_t			reqpcid;
cred_t			*reqpcredp;
id_t			targpcid;
cred_t			*targpcredp;
vcproc_t		*vcpp;
{
	/*
	 * Check validity of parameters.
	 */
	if ((vcparmsp->vc_uprilim > vc_maxupri ||
	    vcparmsp->vc_uprilim < -vc_maxupri) &&
	    vcparmsp->vc_uprilim != VC_NOCHANGE)
		return(EINVAL);

	if ((vcparmsp->vc_upri > vc_maxupri || vcparmsp->vc_upri < -vc_maxupri) &&
	    vcparmsp->vc_upri != VC_NOCHANGE)
		return(EINVAL);

	if (reqpcredp == NULL || vcparmsp->vc_uprilim == VC_NOCHANGE)
		return(0);

	/* 
	 * Our caller passed us non-NULL credential pointers so
	 * we are being asked to check permissions as well as
	 * the validity of the parameters.  The basic rules are
	 * that the calling process must be privileged in order
	 * to raise the target process' upri limit above its
	 * current value.  If the target process is not currently
	 * VP/ix, the calling process must be privileged in
	 * order to set a upri limit greater than zero.
	 */
	if (targpcid == vc_cid) {
		if (vcparmsp->vc_uprilim > vcpp->vc_uprilim &&
		    pm_denied(reqpcredp, P_TSHAR))
			return(EPERM);
	} else {
		if (vcparmsp->vc_uprilim > 0 && pm_denied(reqpcredp, P_TSHAR))
			return(EPERM);
	}

	return(0);
}


/*
 * Nothing to do here but return success.
 */
STATIC int
vc_parmsout()
{
	return(0);
}


/*
 * Set the scheduling parameters of the process pointed to by vcprocp
 * to those specified in the buffer pointed to by vcparmsp.
 */
/* ARGSUSED */
STATIC int
vc_parmsset(vcparmsp, vcpp, reqpcid, reqpcredp)
register vcparms_t	*vcparmsp;
register vcproc_t	*vcpp;
id_t			reqpcid;
cred_t			*reqpcredp;
{
	register int		oldlvl;
	boolean_t		wasonq;
	register char		nice;
	register short		reqvcuprilim;
	register short		reqvcupri;

	if (vcparmsp->vc_uprilim == VC_NOCHANGE)
			reqvcuprilim = vcpp->vc_uprilim;
	else
		reqvcuprilim = vcparmsp->vc_uprilim;

	if (vcparmsp->vc_upri == VC_NOCHANGE)
		reqvcupri = vcpp->vc_upri;
	else
		reqvcupri = vcparmsp->vc_upri;

	/*
	 * Make sure the user priority doesn't exceed the upri limit.
	 */
	if (reqvcupri > reqvcuprilim)
		reqvcupri = reqvcuprilim;

	/*
	 * Basic permissions enforced by generic kernel code
	 * for all classes require that a process attempting
	 * to change the scheduling parameters of a target
	 * process be privileged or have a real or effective
	 * UID matching that of the target process. We are not
	 * called unless these basic permission checks have
	 * already passed. The VP/ix class requires in
	 * addition that the calling process be privileged if it
	 * is attempting to raise the upri limit above its current
	 * value This may have been checked previously but if our
	 * caller passed us a non-NULL credential pointer we assume
	 * it hasn't and we check it here.
	 */
	if (reqpcredp != NULL) {
		if (reqvcuprilim > vcpp->vc_uprilim && pm_denied(reqpcredp, P_TSHAR)) {
			return(EPERM);
		}
	}

	oldlvl = splhi();
	vcpp->vc_uprilim = reqvcuprilim;
	vcpp->vc_upri = reqvcupri;

	/*  Recalculate priority unless process is busywaiting.  If it is,
	 *  the new parameters may still have some influence when the
	 *  process stops busywaiting.
	 */
	if ((vcpp->vc_flags & VCBSYWT) == 0)
		VC_NEWUMDPRI(vcpp);

	/*
	 * Set vc_nice to the nice value corresponding to the user
	 * priority we are setting.
	 */
	nice = 20 - (vcparmsp->vc_upri * 20) / vc_maxupri;
	if (nice == 40)
		nice = 39;
	vcpp->vc_nice = nice;

	if ((vcpp->vc_flags & VCKPRI) != 0) {
		splx(oldlvl);
		return(0);
	}

	vcpp->vc_dispwait = 0;
	if (vcpp->vc_procp == curproc) {
		if ((curpri = *vcpp->vc_pprip =
		    vc_dptbl[vcumdpri].vc_globpri) >= maxrunpri)
			vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
		else {
			vcpp->vc_flags |= VCBACKQ;
			runrun++;
		}
	} else {
		wasonq = dispdeq(vcpp->vc_procp);
		*vcpp->vc_pprip = vc_dptbl[vcumdpri].vc_globpri;
		if (wasonq == B_TRUE) {
			if (*vcpp->vc_pprip > curpri)
				runrun++;
			vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
			setbackdq(vcpp->vc_procp);
		} else
			vcpp->vc_flags |= VCBACKQ;
	}

	splx(oldlvl);
	return(0);
}


/*
 * Arrange for process to be placed in appropriate location
 * on dispatcher queue.  Runs at splhi() since the clock
 * interrupt can cause VCBACKQ to be set.
 */
STATIC void
vc_preempt(vcpp)
vcproc_t	*vcpp;
{
	register int	oldlvl;

	oldlvl = splhi();

	switch (vcpp->vc_flags & (VCBACKQ|VCKPRI|VCFORK)) {
		case VCBACKQ:
			vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
			vcpp->vc_dispwait = 0;
			vcpp->vc_flags &= ~VCBACKQ;
			setbackdq(vcpp->vc_procp);
			break;
		case (VCBACKQ|VCKPRI):
			vcpp->vc_flags &= ~VCBACKQ;
			setbackdq(vcpp->vc_procp);
			break;
		case (VCBACKQ|VCFORK):
			vcpp->vc_dispwait = 0;
			vcpp->vc_flags &= ~(VCBACKQ|VCFORK);
			setbackdq(vcpp->vc_procp);
			break;
		default:
			setfrontdq(vcpp->vc_procp);
	}

	splx(oldlvl);
}


/*
 * vc_proccmp() is part of the implementation of the PC_GETPARMS
 * command of the priocntl system call. When the user specifies
 * multiple processes to priocntl PC_GETPARMS the criteria
 * for selecting a process from the set is class specific. The
 * criteria used by the VP/ix class is the upri value
 * of the process. vc_proccmp() simply compares two processes based
 * on their upri values.  All the ugly work of looping through the 
 * processes in the set is done by higher level (class independent)
 * functions.
 */
STATIC int
vc_proccmp(vcproc1p, vcproc2p)
vcproc_t	*vcproc1p;
vcproc_t	*vcproc2p;
{
	return(vcproc1p->vc_upri - vcproc2p->vc_upri);
}


STATIC void
vc_setrun(vcpp)
vcproc_t	*vcpp;
{
	if ((vcpp->vc_flags & VCKPRI) == 0) {
		vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
		vcpp->vc_dispwait = 0;
	}
	vcpp->vc_flags &= ~(VCBACKQ|VCFORK);
	setbackdq(vcpp->vc_procp);
}


/*
 * Prepare process for sleep. We reset the process priority so it will
 * run at the requested priority (as specified by the disp argument)
 * when it wakes up.
 */
/* ARGSUSED */
STATIC void
vc_sleep(vcprocp, chan, disp)
register vcproc_t	*vcprocp;
caddr_t			chan;
int			disp;
{
	vcprocp->vc_flags |= VCKPRI;
	*vcprocp->vc_pprip = vc_kmdpris[vc_maxkmdpri - (disp & PMASK)];
}


/*
 * Nominate a process for sched to "swap in".  Choose
 * the highest priority runnable process which is unloaded.
 * We do not allow VP/ix processes to be swapped out, but
 * we allow for the possibility that a process was out
 * when it entered the VP/ix class.
 */
/* ARGSUSED */
STATIC void
vc_swapin(fm, procpp, runflagp)
int	fm;
proc_t	**procpp;
int	*runflagp;
{
	register vcproc_t	*vcpp;
	register vcproc_t	*retpp;
	register int		maxpri;
	register int		oldlvl;

	maxpri = -1;
	oldlvl = splhi();
	*runflagp = 0;
	for (vcpp = vc_plisthead.vc_next; vcpp != &vc_plisthead;
	    vcpp = vcpp->vc_next) {
		if (pflag & SUSWAP)
			continue;
		if (pstat == SRUN) {
			if ((pflag & SLOAD) == 0) {
				if (ppri > maxpri) {
					retpp = vcpp;
					maxpri = ppri;
				}
			} else {
				*runflagp = 1;
			}
		}
	}
	splx(oldlvl);
	if (maxpri == -1)
		*procpp = NULL;
	else
		*procpp = retpp->vc_procp;
}


/*
 * Nominate a process for sched to "swap out".  We set *unloadokp to
 * B_FALSE so sched() will try to swap out the pages of the process
 * we nominate but won't swap out the u-block or mark the process
 * unloaded.
 */
/* ARGSUSED */
STATIC void
vc_swapout(fm, jl, procpp, unloadokp)
int		fm;
proc_t		*jl;
proc_t		**procpp;
boolean_t	*unloadokp;
{
	register vcproc_t	*vcpp;
	register vcproc_t	*retpp;
	register int		minspri;
	register int		minrpri;
	register int		oldlvl;

	retpp = NULL;
	minspri = minrpri = vc_maxglobpri + 1;
	oldlvl = splhi();
	for (vcpp = vc_plisthead.vc_next; vcpp != &vc_plisthead;
	    vcpp = vcpp->vc_next) {
		if (pstat == SZOMB)
			continue;

		/*  Skip processes which need to stay in or are already
		 *  out or being swapped in or out.
		 */
		if ((pflag & (SLOAD|SSYS|SLOCK|SUSWAP|SPROCIO|SSWLOCKS))
		    != SLOAD)
			continue;

		/*  Skip the last process just loaded */
		if (vcpp->vc_procp == jl)
			continue;

		/*  Look for the lowest priority sleeping or stopped
		 *  process or, if there aren't any, the lowest priority
		 *  runnable one.
		 */
		if ((pstat == SSLEEP || pstat == SSTOP) && ppri < minspri) {
			retpp = vcpp;
			minspri = ppri;
		} else {
			if (retpp == NULL && pstat == SRUN && ppri < minrpri) {
				retpp = vcpp;
				minrpri = ppri;
			}
		}
	}
	splx(oldlvl);
	if (retpp == NULL) {
		*procpp = NULL;
	} else {
		*procpp = retpp->vc_procp;
		*unloadokp = B_TRUE;
	}
}


/*
 * Check for time slice expiration.  If time slice has expired
 * move proc to priority specified in vcdptbl for time slice expiration
 * and set runrun to cause preemption.
 */

STATIC void
vc_tick(vcpp)
register vcproc_t	*vcpp;
{
	register boolean_t	wasonq;

	if ((vcpp->vc_flags & VCKPRI) != 0)
		/*
		 * No time slicing of procs at kernel mode priorities.
		 */
		return;

	if (--vcpp->vc_timeleft == 0) {
		wasonq = dispdeq(vcpp->vc_procp);

		/*  If process is busywaiting, new user mode priority
		 *  is taken directly from the table.  Otherwise it
		 *  is computed as for time-sharing.
		 */
		if (vcpp->vc_flags & VCBSYWT) {
			vcpp->vc_umdpri = vc_dptbl[vcpp->vc_umdpri].vc_tqexp;
		}
		else {
			vcpp->vc_cpupri = vc_dptbl[vcpp->vc_cpupri].vc_tqexp;
			VC_NEWUMDPRI(vcpp);
		}
		curpri = *vcpp->vc_pprip = vc_dptbl[vcumdpri].vc_globpri;
		vcpp->vc_dispwait = 0;
		if (wasonq == B_TRUE) {
			vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
			setbackdq(vcpp->vc_procp);
		} else
			vcpp->vc_flags |= VCBACKQ;
		runrun++;
	}
}


/*
 * If process is currently at a kernel mode priority (has slept)
 * we assign it the appropriate user mode priority and time quantum
 * here.  If we are lowering the process' priority below that of
 * other runnable processes we will normally set runrun here to
 * cause preemption.  We don't do this, however, if a non-preemptive
 * wakeup has occurred since we were switched in because doing so
 * would defeat the non-preemptive wakeup mechanism.
 */
STATIC void
vc_trapret(vcpp)
vcproc_t	*vcpp;
{
	register boolean_t	was_boosted;

	/*  Note whether process priority had been boosted, then
	 *  turn off the flag.
	 */
	was_boosted = (vcpp->vc_flags & VCBOOST) ? B_TRUE : B_FALSE;
	vcpp->vc_flags &= ~VCBOOST;

	/*  If process is already at user priority just leave it unchanged.
	 */
	if ((vcpp->vc_flags & VCKPRI) == 0)
		return;

	vcpp->vc_cpupri = vc_dptbl[vcpp->vc_cpupri].vc_slpret;

	/*  If priority was not boosted and is not busywaiting,
	 *  calculate a suitable new priority.  If it was boosted,
	 *  vc_boost() already did so.  If it is busywaiting, it
	 *  will receive a new priority when it receives or misses
	 *  its timeslice or when it stops busywaiting.
	 */
	if (was_boosted == B_FALSE && (vcpp->vc_flags & VCBSYWT) == 0) {
		VC_NEWUMDPRI(vcpp);
	}
	vcpp->vc_flags &= ~VCKPRI;
	curpri = *vcpp->vc_pprip = vc_dptbl[vcumdpri].vc_globpri;
	vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
	vcpp->vc_dispwait = 0;
	if (npwakecnt == 0 && curpri < maxrunpri)
	    /* If process priority had been boosted, do not set runrun
	     * because we want user mode to run for a while to process
	     * the event for which we were boosted.
	     */
	    if (was_boosted == B_FALSE)
		runrun++;

	/* If process was boosted we want to run it in user mode
	 * for a while.  Turn off runrun to prevent anyone stealing
	 * the cpu.  That is really too cavalier, and we should
	 * remember whether it was on and turn it back on later if
	 * so.  But when would that be?  Ideally we want to run
	 * long enough to echo a character.
	 */
	if (was_boosted == B_TRUE)
		runrun = 0;
}


/*
 *  Update the vc_dispwait values of all VP/ix processes that
 *  are currently runnable at a user mode priority and bump the priority
 *  if vc_dispwait exceeds vc_maxwait.  Called once per second via
 *  timeout which we reset here if any VP/ix processes exist.
 *
 *  In the time-sharing class, the equivalent routine is called
 *  every second whereas the time limits for processes to receive
 *  their quanta are all 5 seconds.  Processes exceeding their
 *  time limits will tend to do so at different invocations of the
 *  update routine.
 *
 *  In the VP/ix class, the time limits are all 1 second and the
 *  update routine runs once per second.  Processes exceeding their
 *  time limits will tend to do so in lock step.  The intent is that
 *  they will update on-screen clocks at much the same time.  If this
 *  strategy turns out to be inappropriate, the correct answer is
 *  probably to express the time limits in clock ticks and run vc_update
 *  more frequently, perhaps with the frequency depending on the number
 *  of VP/ix processes (perhaps vpixprocs >= 10 ? HZ/10 : HZ / vpixprocs).
 */
STATIC void
vc_update()
{
	register vcproc_t	*vcpp;
	register int		oldlvl;
	register boolean_t	wasonq;

	oldlvl = splhi();
	for (vcpp = vc_plisthead.vc_next; vcpp != &vc_plisthead;
	    vcpp = vcpp->vc_next) {
		if ((vcpp->vc_flags & VCKPRI) != 0)
			continue;
		if ((pstat != SRUN && pstat != SONPROC) || (pflag & SPROCIO))
			continue;
		vcpp->vc_dispwait++;
		if (vcpp->vc_dispwait <= vc_dptbl[vcumdpri].vc_maxwait)
			continue;

		/*  If process is busywaiting, get its new priority directly
		 *  from the table.  Otherwise set cpupri from the table
		 *  and recalculate.
		 */
		if (vcpp->vc_flags & VCBSYWT) {
			vcpp->vc_umdpri = vc_dptbl[vcpp->vc_umdpri].vc_lwait;
		}
		else {
			vcpp->vc_cpupri = vc_dptbl[vcpp->vc_cpupri].vc_lwait;
			VC_NEWUMDPRI(vcpp);
		}

		vcpp->vc_dispwait = 0;
		wasonq = dispdeq(vcpp->vc_procp);
		if (vcpp->vc_procp == curproc)
			curpri = *vcpp->vc_pprip =
			    vc_dptbl[vcumdpri].vc_globpri;
		else
			*vcpp->vc_pprip = vc_dptbl[vcumdpri].vc_globpri;
		if (wasonq == B_TRUE) {
			vcpp->vc_timeleft = vc_dptbl[vcumdpri].vc_quantum;
			setbackdq(vcpp->vc_procp);
		} else
			vcpp->vc_flags |= VCBACKQ;
	}
	runrun++;
	splx(oldlvl);

	/*  Reschedule myself unless there are no VP/ix class processes */
	if (vc_plisthead.vc_next == &vc_plisthead)
		vc_updactive = B_FALSE;
	else
		(void)timeout(vc_update, 0, HZ);
}


/*
 * Processes waking up go to the back of their queue.  We don't
 * need to assign a time quantum here because process is still
 * at a kernel mode priority and the time slicing is not done
 * for processes running in the kernel after sleeping.  The proper
 * time quantum will be assigned by vc_trapret before the process
 * returns to user mode.
 * Note that we ignore the preemption flag (we permit non-preemptive
 * wakeups).
 */
/* ARGSUSED */
STATIC void
vc_wakeup(vcprocp, preemptflg)
vcproc_t	*vcprocp;
int		preemptflg;
{
	vcprocp->vc_flags &= ~(VCBACKQ|VCFORK);
	setbackdq(vcprocp->vc_procp);
}
