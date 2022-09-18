/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:proc/class/fc.c	1.4"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/class.h>
#include <proc/class/fc.h>
#include <proc/class/fcpriocntl.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Class specific code for the time-sharing class
 */


/*
 * Extern declarations for variables defined in the fc master file
 */
extern fcdpent_t  fc_dptbl[];	/* time-sharing disp parameter table */
extern int	fc_kmdpris[];	/* array of global pris used by fc procs when */
				/*  sleeping or running in kernel after sleep */
extern short	fc_maxupri;	/* max time-sharing user priority */
extern short	fc_maxumdpri;	/* maximum user mode fc priority */
extern short	fc_maxkmdpri;	/* maximum kernel mode fc priority */

/* Audit Recording Functions */
extern	int	adt_admin();	/* defined in acc/audit/auditrec.c */
extern	int	adt_parmsset();	/* defined in acc/audit/auditrec.c */

#define	FCPMEMSZ	 1024	/* request size for fcproc memory allocation */

#define	pstat		(*fcpp->fc_pstatp)
#define	ppri		(*fcpp->fc_pprip)
#define	pflag		(*fcpp->fc_pflagp)
#define	fcumdpri	(fcpp->fc_umdpri)
#define	fcmedumdpri	(fc_maxumdpri >> 1)

#define	FC_NEWUMDPRI(fcpp)	\
{ \
if (((fcpp)->fc_umdpri = (fcpp)->fc_cpupri + (fcpp)->fc_upri) > fc_maxumdpri) \
	(fcpp)->fc_umdpri = fc_maxumdpri; \
else if ((fcpp)->fc_umdpri < 0) \
	(fcpp)->fc_umdpri = 0; \
}

#define ADT_FC_NEW		ADT_TS_NEW
#define ADT_FC_PARMSSET		ADT_TS_PARMSSET
#define ADT_FC_SETDPTBL		ADT_TS_SETDPTBL
#define ADT_SCHED_FC		ADT_SCHED_TS

#define P_FCLASS		P_TSHAR


STATIC int	fc_admin(), fc_enterclass(), fc_fork(), fc_getclinfo();
STATIC int	fc_nosys(), fc_parmsin(), fc_parmsout(), fc_parmsset();
STATIC int	fc_proccmp();
STATIC void	fc_exitclass(), fc_forkret();
STATIC void	fc_nullsys(), fc_parmsget(), fc_preempt();
STATIC void	fc_setrun(), fc_sleep(), fc_swapin(), fc_swapout(), fc_swapinpri();
STATIC void	fc_swapoutpri(), fc_tick(), fc_trapret(), fc_update(), fc_wakeup();


STATIC id_t	fc_cid;		/* time-sharing class ID */
STATIC int	fc_maxglobpri;	/* maximum global priority used by ts class */
STATIC fcproc_t	fc_plisthead;	/* dummy tsproc at head of tsproc list */
STATIC caddr_t	fc_pmembase;	/* base addr of memory allocated for tsprocs */

static int fc_swap_count;	/* number fixed class processes */
extern int number_current_processes;	/* total number processes in system */
static int current_fc_count;	/* count of this entry into swapout */

STATIC struct classfuncs fc_classfuncs = {
	fc_admin,
	fc_enterclass,
	fc_exitclass,
	fc_fork,
	fc_forkret,
	fc_getclinfo,
	fc_nullsys,
	fc_parmsget,
	fc_parmsin,
	fc_parmsout,
	fc_parmsset,
	fc_preempt,
	fc_proccmp,
	fc_setrun,
	fc_sleep,
	fc_nullsys,
	fc_swapin,
	fc_swapout,
	fc_tick,
	fc_trapret,
	fc_wakeup,
	fc_swapoutpri,
	fc_swapinpri,
	fc_nosys,
	fc_nosys,
	fc_nosys,
	fc_nosys,
	fc_nosys,
	fc_nosys,
	fc_nosys,
	fc_nosys,
	fc_nosys
};


/*
 * Time sharing class initialization.  Called by dispinit() at boot time.
 * We can ignore the clparmsz argument since we know that the smallest
 * possible parameter buffer is big enough for us.
 */
/* ARGSUSED */
void
fc_init(cid, clparmsz, clfuncspp, maxglobprip)
id_t		cid;
int		clparmsz;
classfuncs_t	**clfuncspp;
int		*maxglobprip;
{
	fc_maxglobpri = max(fc_kmdpris[fc_maxkmdpri],
	    fc_dptbl[fc_maxumdpri].fc_globpri);

	fc_cid = cid;		/* Record our class ID */

	/*
	 * Initialize the fcproc list.
	 */
	fc_plisthead.fc_next = fc_plisthead.fc_prev = &fc_plisthead;

	/*
	 * We're required to return a pointer to our classfuncs
	 * structure and the highest global priority value we use.
	 */
	*clfuncspp = &fc_classfuncs;
	*maxglobprip = fc_maxglobpri;
}


/*
 * Get or reset the fc_dptbl values per the user's request.
 */
/* ARGSUSED */
STATIC int
fc_admin(uaddr, reqpcid, reqpcredp)
caddr_t	uaddr;
id_t	reqpcid;
cred_t	*reqpcredp;
{
	fcadmin_t		fcadmin;
	register fcdpent_t	*tmpdpp;
	register int		userdpsz;
	register int		i;
	register int		fcdpsz;
	int			oldlvl;

	if (copyin(uaddr, (caddr_t)&fcadmin, sizeof(fcadmin_t))) {
		ADT_ADMIN(ADT_SCHED_FC,EFAULT,ADT_FC_SETDPTBL,0,0,0,0,0,0);
		return(EFAULT);
	}

	fcdpsz = (fc_maxumdpri + 1) * sizeof(fcdpent_t);

	switch(fcadmin.fc_cmd) {

	case FC_GETDPSIZE:

		fcadmin.fc_ndpents = fc_maxumdpri + 1;
		if (copyout((caddr_t)&fcadmin, uaddr, sizeof(fcadmin_t)))
			return(EFAULT);
		break;

	case FC_GETDPTBL:

		userdpsz = MIN(fcadmin.fc_ndpents * sizeof(fcdpent_t), fcdpsz);
		if (copyout((caddr_t)fc_dptbl,
		    (caddr_t)fcadmin.fc_dpents, userdpsz))
			return(EFAULT);

		fcadmin.fc_ndpents = userdpsz / sizeof(fcdpent_t);
		if (copyout((caddr_t)&fcadmin, uaddr, sizeof(fcadmin_t)))
			return(EFAULT);

		break;

	case FC_SETDPTBL:

		/*
		 * We require that the requesting process have privilege.
		 * We also require that the table supplied by the user
		 * exactly match the current fc_dptbl in size.
		 */
		if (pm_denied(reqpcredp, P_SYSOPS)
		    && pm_denied(reqpcredp, P_FCLASS)) {
			ADT_ADMIN(ADT_SCHED_FC,EPERM,ADT_FC_SETDPTBL,0,0,0,0,0,0);
			return(EPERM);
		}
		if (fcadmin.fc_ndpents * sizeof(fcdpent_t) != fcdpsz) {
			ADT_ADMIN(ADT_SCHED_FC,EINVAL,ADT_FC_SETDPTBL,0,0,0,0,0,0);
			return(EINVAL);
		}

		/*
		 * We read the user supplied table into a temporary buffer
		 * where it is validated before being copied over the
		 * fc_dptbl.
		 */
		tmpdpp = (fcdpent_t *)kmem_alloc(fcdpsz, KM_SLEEP);
		ASSERT(tmpdpp != NULL);
		if (copyin((caddr_t)fcadmin.fc_dpents, (caddr_t)tmpdpp,
		    fcdpsz)) {
			kmem_free(tmpdpp, fcdpsz);
			ADT_ADMIN(ADT_SCHED_FC,EFAULT,ADT_FC_SETDPTBL,0,0,0,0,0,0);
			return(EFAULT);
		}
		for (i = 0; i < fcadmin.fc_ndpents; i++) {

			/*
			 * Validate the user supplied values.  All we are doing
			 * here is verifying that the values are within their
			 * allowable ranges and will not panic the system.  We
			 * make no attempt to ensure that the resulting
			 * configuration makes sense or results in reasonable
			 * performance.
			 */
			if (tmpdpp[i].fc_quantum <= 0) {
				ADT_ADMIN(ADT_SCHED_FC,EINVAL,
						ADT_FC_SETDPTBL,
						tmpdpp[i].fc_globpri,
						tmpdpp[i].fc_quantum,0,0,0,0);
				kmem_free(tmpdpp, fcdpsz);
				return(EINVAL);
			}
		}

		/*
		 * Copy the user supplied values over the current fc_dptbl
		 * values.  The fc_globpri member is read-only so we don't
		 * overwrite it.
		 */
		oldlvl = splhi();
		for (i = 0; i < fcadmin.fc_ndpents; i++) {
			fc_dptbl[i].fc_quantum = tmpdpp[i].fc_quantum;
			/*
			* Write audit record
			*/
			ADT_ADMIN(ADT_SCHED_FC, 0, ADT_FC_SETDPTBL,
					tmpdpp[i].fc_globpri, 
					tmpdpp[i].fc_quantum, 0,0,0,0);
		}
		splx(oldlvl);

		kmem_free(tmpdpp, fcdpsz);
		break;

	default:
		return(EINVAL);
	}
	return(0);
}


/*
 * Allocate a time-sharing class specific proc structure and
 * initialize it with the parameters supplied. Also move process
 * to specified time-sharing priority.
 */
/* ARGSUSED */
STATIC int
fc_enterclass(fcparmsp, pp, pstatp, pprip, pflagp, pcredpp, fcprocpp,
							reqpcid, reqpcredp)
fcparms_t	*fcparmsp;
proc_t		*pp;
char		*pstatp;
int		*pprip;
uint		*pflagp;
cred_t		**pcredpp;
fcproc_t	**fcprocpp;
id_t		reqpcid;
cred_t		*reqpcredp;
{
	register fcproc_t	*fcpp;
	register short		reqfcuprilim;
	register short		reqfcupri;
	register int 		oldlvl;
	register boolean_t	wasonq;
	static int		fcpexists = 0;	/* set on first occurence of */
						/*   a time-sharing process */


	if ((fcpp = (fcproc_t *)kmem_fast_alloc(&fc_pmembase, sizeof(fcproc_t),
	    FCPMEMSZ / sizeof(fcproc_t), KM_NOSLEEP)) == NULL) {
		ADT_PARMSSET(ADT_SCHED_FC,ENOMEM,ADT_FC_NEW,pp->p_pid,0,0,0);
		return(ENOMEM);
	}


	/*
	 * Initialize the fcproc structure.
	 */
	if (fcparmsp == NULL) {
		/*
		 * Use default values.
		 */
		fcpp->fc_uprilim = fcpp->fc_upri = 0;
		fcpp->fc_nice = 20;
		fcpp->fc_umdpri = fcpp->fc_cpupri = fcmedumdpri;
	} else {
		/*
		 * Use supplied values.
		 */
		if (fcparmsp->fc_uprilim == FC_NOCHANGE)
				reqfcuprilim = 0;
		else
			reqfcuprilim = fcparmsp->fc_uprilim;

		/*
		 * In order to set an initial upri limit greater than
		 * zero the requesting process must be privileged.
		 * This may have been checked previously but if our
		 * caller passed us a credential structure we assume
		 * it hasn't and we check it here.
		 */
		if (reqpcredp != NULL && reqfcuprilim > 0 &&
		    pm_denied(reqpcredp, P_FCLASS)) {
			kmem_fast_free(&fc_pmembase, (caddr_t)fcpp);
			ADT_PARMSSET(ADT_SCHED_FC,EPERM,ADT_FC_NEW,pp->p_pid,reqfcupri,reqfcuprilim,0);
			return(EPERM);
		}

		if (fcparmsp->fc_upri == FC_NOCHANGE) {
			reqfcupri = reqfcuprilim;
		} else {
			/*
			 * Set the user priority to the requested value
			 * or the upri limit, whichever is lower.
			 */
			reqfcupri = fcparmsp->fc_upri;
			if (reqfcupri > reqfcuprilim)
				reqfcupri = reqfcuprilim;
		}

		fcpp->fc_uprilim = reqfcuprilim;
		fcpp->fc_upri = reqfcupri;
		fcpp->fc_nice = 20 - (20 * reqfcupri) / fc_maxupri;
		fcpp->fc_cpupri = fcmedumdpri;
		FC_NEWUMDPRI(fcpp);
	}

	fcpp->fc_dispwait = 0;
	fcpp->fc_flags = 0;
	fcpp->fc_procp = pp;
	fcpp->fc_pstatp = pstatp;
	fcpp->fc_pprip = pprip;
	fcpp->fc_pflagp = pflagp;

	/*
	 * Link new structure into fcproc list.
	 */
	fcpp->fc_next = fc_plisthead.fc_next;
	fcpp->fc_prev = &fc_plisthead;
	fc_plisthead.fc_next->fc_prev = fcpp;
	fc_plisthead.fc_next = fcpp;

	/*
         * Write audit record
	 */
	ADT_PARMSSET(ADT_SCHED_FC,0,ADT_FC_NEW,pp->p_pid,fcpp->fc_upri,fcpp->fc_uprilim,0);
 
	/*
	 * Reset priority. Process goes to a "user mode" priority
	 * here regardless of whether or not it has slept since
	 * entering the kernel.
	 */
	oldlvl = splhi();
	if (fcpp->fc_procp == curproc) {
		if ((curpri = *fcpp->fc_pprip =
		    fc_dptbl[fcumdpri].fc_globpri) > maxrunpri)
			fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
		else {
			fcpp->fc_flags |= FCBACKQ;
			runrun++;
		}
	} else {
		wasonq = dispdeq(pp);
		*fcpp->fc_pprip = fc_dptbl[fcumdpri].fc_globpri;
		if (wasonq == B_TRUE) {
			if (*fcpp->fc_pprip > curpri)
				runrun++;
			fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
			setbackdq(pp);
		} else
			fcpp->fc_flags |= FCBACKQ;
	}
	*fcprocpp = fcpp;
	splx(oldlvl);

	/*
	 * If this is the first time-sharing process to occur since
	 * boot we set up the initial call to fc_update() here.
	 */
	if (fcpexists == 0) {
		(void)timeout(fc_update, 0, HZ);
		fcpexists++;
	}
	return(0);
}


/*
 * Free fcproc structure of process.
 */
STATIC void
fc_exitclass(fcprocp)
fcproc_t	*fcprocp;
{
	fcprocp->fc_prev->fc_next = fcprocp->fc_next;
	fcprocp->fc_next->fc_prev = fcprocp->fc_prev;

	kmem_fast_free(&fc_pmembase, (caddr_t)fcprocp);
}


/*
 * Allocate and initialize time-sharing class specific
 * proc structure for child.
 */
/* ARGSUSED */
STATIC int
fc_fork(pfcpp, cprocp, cpstatp, cpprip, cpflagp, cpcredpp, fcprocpp)
register fcproc_t	*pfcpp;
proc_t			*cprocp;
char			*cpstatp;
int			*cpprip;
uint			*cpflagp;
cred_t			**cpcredpp;
fcproc_t		**fcprocpp;
{
	register fcproc_t	*cfcpp; /* ptr to child's fcproc structure */


	cfcpp = (fcproc_t *)kmem_fast_alloc(&fc_pmembase, sizeof(fcproc_t),
	    FCPMEMSZ / sizeof(fcproc_t), KM_SLEEP);
	ASSERT(cfcpp != NULL);


	/*
	 * Initialize child's fcproc structure.
	 */
	cfcpp->fc_timeleft = fc_dptbl[pfcpp->fc_umdpri].fc_quantum;
	cfcpp->fc_umdpri = pfcpp->fc_umdpri;
	cfcpp->fc_cpupri = pfcpp->fc_cpupri;
	cfcpp->fc_uprilim = pfcpp->fc_uprilim;
	cfcpp->fc_upri = pfcpp->fc_upri;
	cfcpp->fc_nice = pfcpp->fc_nice;
	cfcpp->fc_dispwait = 0;
	cfcpp->fc_flags = pfcpp->fc_flags & ~FCBACKQ;
	cfcpp->fc_procp = cprocp;
	cfcpp->fc_pstatp = cpstatp;
	cfcpp->fc_pprip = cpprip;
	cfcpp->fc_pflagp = cpflagp;

	/*
	 * Link structure into fcproc list.
	 */
	cfcpp->fc_next = fc_plisthead.fc_next;
	cfcpp->fc_prev = &fc_plisthead;
	fc_plisthead.fc_next->fc_prev = cfcpp;
	fc_plisthead.fc_next = cfcpp;
	
	*fcprocpp = cfcpp;
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
fc_forkret(cfcpp, pfcpp)
register fcproc_t	*cfcpp;
register fcproc_t	*pfcpp;
{
	setbackdq(cfcpp->fc_procp);

	if (pfcpp != NULL) {
		pfcpp->fc_flags |= (FCBACKQ|FCFORK);
		runrun++;
	}
}


/*
 * Get information about the time-sharing class into the buffer
 * pointed to by fcinfop. The maximum configured user priority
 * is the only information we supply.  We ignore the class and
 * credential arguments because anyone can have this information.
 */
/* ARGSUSED */
STATIC int
fc_getclinfo(fcinfop, reqpcid, reqpcredp)
fcinfo_t	*fcinfop;
id_t		reqpcid;
cred_t		*reqpcredp;
{
	fcinfop->fc_maxupri = fc_maxupri;
	return(0);
}


STATIC int
fc_nosys()
{
	return(ENOSYS);
}


STATIC void
fc_nullsys()
{
}


/*
 * Get the time-sharing parameters of the process pointed to by
 * fcprocp into the buffer pointed to by fcparmsp.
 */
STATIC void
fc_parmsget(fcprocp, fcparmsp)
fcproc_t	*fcprocp;
fcparms_t	*fcparmsp;
{
	fcparmsp->fc_uprilim = fcprocp->fc_uprilim;
	fcparmsp->fc_upri = fcprocp->fc_upri;
	fcparmsp->fc_timeleft = fcprocp->fc_timeleft;
	fcparmsp->fc_cpupri = fcprocp->fc_cpupri;
	fcparmsp->fc_umdpri = fcprocp->fc_umdpri;
}


/*
 * Check the validity of the time-sharing parameters in the buffer
 * pointed to by fcparmsp. If our caller passes us a non-NULL
 * reqpcredp pointer we also verify that the requesting process
 * (whose credentials are pointed to by reqpcredp) has the necessary
 * permissions to set these parameters for the target process.
 */
/* ARGSUSED */
STATIC int
fc_parmsin(fcparmsp, reqpcid, reqpcredp, targpcid, targpcredp, fcpp)
register fcparms_t	*fcparmsp;
id_t			reqpcid;
cred_t			*reqpcredp;
id_t			targpcid;
cred_t			*targpcredp;
fcproc_t		*fcpp;
{
	/*
	 * Check validity of parameters.
	 */
	if ((fcparmsp->fc_uprilim > fc_maxupri ||
	    fcparmsp->fc_uprilim < -fc_maxupri) &&
	    fcparmsp->fc_uprilim != FC_NOCHANGE)
		return(EINVAL);

	if ((fcparmsp->fc_upri > fc_maxupri || fcparmsp->fc_upri < -fc_maxupri) &&
	    fcparmsp->fc_upri != FC_NOCHANGE)
		return(EINVAL);

	if (reqpcredp == NULL || fcparmsp->fc_uprilim == FC_NOCHANGE)
		return(0);

	/* 
	 * Our caller passed us non-NULL credential pointers so
	 * we are being asked to check permissions as well as
	 * the validity of the parameters.  The basic rules are
	 * that the calling process must be privileged in order
	 * to raise the target process' upri limit above its
	 * current value.  If the target process is not currently
	 * time-sharing, the calling process must be privileged in
	 * order to set a upri limit greater than zero.
	 */
	if (targpcid == fc_cid) {
		if (fcparmsp->fc_uprilim > fcpp->fc_uprilim &&
		    pm_denied(reqpcredp, P_FCLASS)) 
			return(EPERM);
	} else {
		if (fcparmsp->fc_uprilim > 0 && pm_denied(reqpcredp, P_FCLASS))
			return(EPERM);
	}
	return(0);
}


/*
 * Nothing to do here but return success.
 */
STATIC int
fc_parmsout()
{
	return(0);
}


/*
 * Set the scheduling parameters of the process pointed to by fcprocp
 * to those specified in the buffer pointed to by fcparmsp.
 */
/* ARGSUSED */
STATIC int
fc_parmsset(fcparmsp, fcpp, reqpcid, reqpcredp)
register fcparms_t	*fcparmsp;
register fcproc_t	*fcpp;
id_t			reqpcid;
cred_t			*reqpcredp;
{
	register int		oldlvl;
	boolean_t		wasonq;
	register char		nice;
	register short		reqfcuprilim;
	register short		reqfcupri;

	if (fcparmsp->fc_uprilim == FC_NOCHANGE)
			reqfcuprilim = fcpp->fc_uprilim;
	else
		reqfcuprilim = fcparmsp->fc_uprilim;

	if (fcparmsp->fc_upri == FC_NOCHANGE)
		reqfcupri = fcpp->fc_upri;
	else
		reqfcupri = fcparmsp->fc_upri;

	/*
	 * Make sure the user priority doesn't exceed the upri limit.
	 */
	if (reqfcupri > reqfcuprilim)
		reqfcupri = reqfcuprilim;

	/*
	 * Basic permissions enforced by generic kernel code
	 * for all classes require that a process attempting
	 * to change the scheduling parameters of a target
	 * process be privileged or have a real or effective
	 * UID matching that of the target process. We are not
	 * called unless these basic permission checks have
	 * already passed. The time-sharing class requires in
	 * addition that the calling process be privileged if it
	 * is attempting to raise the upri limit above its current
	 * value This may have been checked previously but if our
	 * caller passed us a non-NULL credential pointer we assume
	 * it hasn't and we check it here.
	 */
	if (reqpcredp != NULL) {
		if (reqfcuprilim > fcpp->fc_uprilim && pm_denied(reqpcredp, P_FCLASS)) {
			ADT_PARMSSET(ADT_SCHED_FC,EPERM,ADT_FC_PARMSSET,fcpp->fc_procp->p_pid,fcparmsp->fc_upri,fcparmsp->fc_uprilim,0);
			return(EPERM);
		}
	}

	oldlvl = splhi();
	fcpp->fc_uprilim = reqfcuprilim;
	fcpp->fc_upri = reqfcupri;
	FC_NEWUMDPRI(fcpp);

	/*
	 * Set fc_nice to the nice value corresponding to the user
	 * priority we are setting.
	 */
	nice = 20 - (fcparmsp->fc_upri * 20) / fc_maxupri;
	if (nice == 40)
		nice = 39;
	fcpp->fc_nice = nice;

	if ((fcpp->fc_flags & FCKPRI) != 0) {
		/*
         	* Write audit record
	 	*/
		ADT_PARMSSET(ADT_SCHED_FC,0,ADT_FC_PARMSSET,fcpp->fc_procp->p_pid,fcparmsp->fc_upri,fcparmsp->fc_uprilim,0);

		splx(oldlvl);
		return(0);
	}

	fcpp->fc_dispwait = 0;
	if (fcpp->fc_procp == curproc) {
		if ((curpri = *fcpp->fc_pprip =
		    fc_dptbl[fcumdpri].fc_globpri) > maxrunpri)
			fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
		else {
			fcpp->fc_flags |= FCBACKQ;
			runrun++;
		}
	} else {
		wasonq = dispdeq(fcpp->fc_procp);
		*fcpp->fc_pprip = fc_dptbl[fcumdpri].fc_globpri;
		if (wasonq == B_TRUE) {
			if (*fcpp->fc_pprip > curpri)
				runrun++;
			fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
			setbackdq(fcpp->fc_procp);
		} else
			fcpp->fc_flags |= FCBACKQ;
	}
	/*
         * Write audit record
	 */
	ADT_PARMSSET(ADT_SCHED_FC,0,ADT_FC_PARMSSET,fcpp->fc_procp->p_pid,fcparmsp->fc_upri,fcparmsp->fc_uprilim,0);
				
	splx(oldlvl);
	return(0);
}


/*
 * Arrange for process to be placed in appropriate location
 * on dispatcher queue.  Runs at splhi() since the clock
 * interrupt can cause FCBACKQ to be set.
 */
STATIC void
fc_preempt(fcpp)
fcproc_t	*fcpp;
{
	register int	oldlvl;

	oldlvl = splhi();

	switch (fcpp->fc_flags & (FCBACKQ|FCKPRI|FCFORK)) {
		case FCBACKQ:
			fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
			fcpp->fc_dispwait = 0;
			fcpp->fc_flags &= ~FCBACKQ;
			setbackdq(fcpp->fc_procp);
			break;
		case (FCBACKQ|FCKPRI):
			fcpp->fc_flags &= ~FCBACKQ;
			setbackdq(fcpp->fc_procp);
			break;
		case (FCBACKQ|FCFORK):
			fcpp->fc_dispwait = 0;
			fcpp->fc_flags &= ~(FCBACKQ|FCFORK);
			setbackdq(fcpp->fc_procp);
			break;
		default:
			setfrontdq(fcpp->fc_procp);
	}


	splx(oldlvl);
}


/*
 * fc_proccmp() is part of the implementation of the PC_GETPARMS
 * command of the priocntl system call. When the user specifies
 * multiple processes to priocntl PC_GETPARMS the criteria
 * for selecting a process from the set is class specific. The
 * criteria used by the time-sharing class is the upri value
 * of the process. fc_proccmp() simply compares two processes based
 * on their upri values.  All the ugly work of looping through the 
 * processes in the set is done by higher level (class independent)
 * functions.
 */
STATIC int
fc_proccmp(fcproc1p, fcproc2p)
fcproc_t	*fcproc1p;
fcproc_t	*fcproc2p;
{
	return(fcproc1p->fc_upri - fcproc2p->fc_upri);
}


STATIC void
fc_setrun(fcpp)
fcproc_t	*fcpp;
{
	if ((fcpp->fc_flags & FCKPRI) == 0) {
		fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
		fcpp->fc_dispwait = 0;
	}
	fcpp->fc_flags &= ~(FCBACKQ|FCFORK);
	setbackdq(fcpp->fc_procp);
}


/*
 * Prepare process for sleep. We reset the process priority so it will
 * run at the requested priority (as specified by the disp argument)
 * when it wakes up.
 */
/* ARGSUSED */
STATIC void
fc_sleep(fcprocp, chan, disp)
register fcproc_t	*fcprocp;
caddr_t			chan;
int			disp;
{
	fcprocp->fc_flags |= FCKPRI;
	*fcprocp->fc_pprip = fc_kmdpris[fc_maxkmdpri - (disp & PMASK)];
}


/*
 * Nominate a process for sched to "swap in".  Choose
 * the highest priority runnable process which is unloaded.
 */
/* ARGSUSED */
STATIC void
fc_swapin(fm, procpp, runflagp)
int	fm;
proc_t	**procpp;
int	*runflagp;
{
	register fcproc_t	*fcpp;
	register fcproc_t	*retpp;
	register int		maxpri;
	register int		oldlvl;

	maxpri = -1;
	oldlvl = splhi();
	*runflagp = 0;
	for (fcpp = fc_plisthead.fc_next; fcpp != &fc_plisthead;
	    fcpp = fcpp->fc_next) {
		if (pflag & SUSWAP)
			continue;
		if (pstat == SRUN) {
			if ((pflag & SLOAD) == 0) {
				if (ppri > maxpri) {
					retpp = fcpp;
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
		*procpp = retpp->fc_procp;
}


/*
 * Nominate a process for sched to swap out. Nominate the lowest
 * priority sleeping or stopped process, or if none, nominate
 * the lowest priority runnable process.
 */
/* ARGSUSED */
STATIC void
fc_swapout(fm, jl, procpp, unloadokp)
int		fm;
proc_t		*jl;
proc_t		**procpp;
boolean_t	*unloadokp;
{
	register fcproc_t	*fcpp;
	register fcproc_t	*retpp;
	register int		minspri;
	register int		minrpri;
	register int		oldlvl;
	int foundsleeper = 0;

	if ((++current_fc_count) < fc_swap_count) {
		*procpp = NULL;
		return;
	}

	current_fc_count = 0;
	retpp = NULL;
	minspri = minrpri = fc_maxglobpri + 1;
	oldlvl = splhi();

	for (fcpp = fc_plisthead.fc_next; fcpp != &fc_plisthead;
	    fcpp = fcpp->fc_next) {
		if (pstat == SZOMB)
			continue;
		if ((pflag & (SLOAD|SSYS|SLOCK|SUSWAP|SPROCIO|SSWLOCKS))
		    != SLOAD)
			continue;
		if (fcpp->fc_procp == jl)
			continue;
		if ((pstat == SSLEEP || pstat == SSTOP) && ppri < minspri) {
			retpp = fcpp;
			minspri = ppri;
			/*
			 * Note that a sleeping candidate has been found
			 */
			foundsleeper++;
		} else {
			/* 
			 * If there are no sleeping candidates, choose the
			 * lowest priority runnable process. The original code
			 * always chooses the first runnable.
			 */
			if (!foundsleeper && pstat == SRUN && ppri < minrpri)
			{
				retpp = fcpp;
				minrpri = ppri;
			}
		}
	}
	splx(oldlvl);
	if (retpp == NULL) {
		*procpp = NULL;
	} else {
		*procpp = retpp->fc_procp;
		*unloadokp = B_FALSE;
	}
}


/*
 * Increase the priority level for processes that are being swapped in if their priority
 * levels were decreased by fc_swapoutpri.
 */
STATIC void
fc_swapinpri(fcpp)
fcproc_t	*fcpp;
{

#ifdef XXX
        register boolean_t      wasonq;

	wasonq = dispdeq(fcpp->fc_procp);
 	*fcpp->fc_pprip = *fcpp->fc_pprip * 5/3;
	if (*fcpp->fc_pprip > fc_kmdpris[fc_maxkmdpri])
	    *fcpp->fc_pprip = fc_kmdpris[fc_maxkmdpri];
        if (fcpp->fc_procp->p_pidp->pid_id == curproc->p_pidp->pid_id)
	    curpri = *fcpp->fc_pprip;	
        if (wasonq == B_TRUE) {
	    fcpp->fc_cpupri = fcpp->fc_cpupri * 5/3;
	    if (fcpp->fc_cpupri > fc_maxumdpri)
	         fcpp->fc_cpupri = fc_maxumdpri;
	    FC_NEWUMDPRI(fcpp);
            fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
            setbackdq(fcpp->fc_procp);
	}
	else
	    fcpp->fc_flags |= FCBACKQ;
#endif


}

/*
 * Lower the priority level for swapped out processes if its memory claim is more than SWPINDLY.
 */

STATIC void
fc_swapoutpri(fcpp)
fcproc_t	*fcpp;
{

#ifdef XXX
        register boolean_t      wasonq;

	wasonq = dispdeq(fcpp->fc_procp);
	/*  lower the priority level such that the processes that are on the same level will 
	    get a crack at the cpu one more time  before the swapped out process. */

 	*fcpp->fc_pprip = *fcpp->fc_pprip * 2/3; 
        if (fcpp->fc_procp->p_pidp->pid_id == curproc->p_pidp->pid_id)
	    curpri = curpri;
            if (wasonq == B_TRUE) {
	        fcpp->fc_cpupri = fcpp->fc_cpupri * 2/3;
		FC_NEWUMDPRI(fcpp);
                setbackdq(fcpp->fc_procp);
		fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
	    }
	    else
	        fcpp->fc_flags |= FCBACKQ;
#endif
}


/*
 * Check for time slice expiration.  If time slice has expired
 * move proc to priority specified in fcdptbl for time slice expiration
 * and set runrun to cause preemption.
 */

STATIC void
fc_tick(fcpp)
register fcproc_t	*fcpp;
{
	register boolean_t	wasonq;

	if ((fcpp->fc_flags & FCKPRI) != 0)
		/*
		 * No time slicing of procs at kernel mode priorities.
		 */
		return;


	/*
	 * Do not have to re-calculate the curpri because it is fixed.
	 */
	if (--fcpp->fc_timeleft <= 0) {
		wasonq = dispdeq(fcpp->fc_procp);
		fcpp->fc_dispwait = 0;
		if (wasonq == B_TRUE) {
			fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
			setbackdq(fcpp->fc_procp);
		} else
			fcpp->fc_flags |= FCBACKQ;
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
fc_trapret(fcpp)
fcproc_t	*fcpp;
{
	if ((fcpp->fc_flags & FCKPRI) == 0)
		return;

	fcpp->fc_flags &= ~FCKPRI;
	curpri = *fcpp->fc_pprip = fc_dptbl[fcumdpri].fc_globpri;
	/*
	 * proc must have slept at least 1 second
	 * penalize process that goes in and out of kernel
	 */
	if (fcpp->fc_dispwait) {

		fcpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
		fcpp->fc_dispwait = 0;

	} else if(fcpp->fc_timeleft > 0)
		fcpp->fc_timeleft--;
	if (npwakecnt == 0 && curpri < maxrunpri)
		runrun++;
}


/*
 * Update the fc_dispwait values of all Fixed Class processes that
 * are currently runnable at a user mode priority.
 * Called once per second via timeout which we reset here.
 */
STATIC void
fc_update()
{
	register fcproc_t	*fcpp;
	register int		oldlvl;
	register boolean_t	wasonq;
	int 			number_fixed_class;

	oldlvl = splhi();
	number_fixed_class = 0;
	for (fcpp = fc_plisthead.fc_next; fcpp != &fc_plisthead;
	    fcpp = fcpp->fc_next) {
	    	number_fixed_class++;
		if ((fcpp->fc_flags & FCKPRI) != 0)
			continue;
		if ((pstat != SRUN && pstat != SONPROC) || (pflag & SPROCIO))
			continue;
		fcpp->fc_dispwait++;

	}
	runrun++;
	if (number_fixed_class)
		fc_swap_count = number_current_processes / number_fixed_class;
	else
		fc_swap_count = 0;
	splx(oldlvl);
	(void)timeout(fc_update, 0, HZ);
}


/*
 * Processes waking up go to the back of their queue.  We don't
 * need to assign a time quantum here because process is still
 * at a kernel mode priority and the time slicing is not done
 * for processes running in the kernel after sleeping.  The proper
 * time quantum will be assigned by fc_trapret before the process
 * returns to user mode.
 * Note that we ignore the preemption flag (we permit non-preemptive
 * wakeups).
 */
/* ARGSUSED */
STATIC void
fc_wakeup(fcprocp, preemptflg)
fcproc_t	*fcprocp;
int		preemptflg;
{
	fcprocp->fc_flags &= ~(FCBACKQ|FCFORK);
	setbackdq(fcprocp->fc_procp);
}


/*
 * Increment the nice value of the specified process by incr and
 * return the new value in *retvalp.
 */
void
fc_donice(fcpp, incr, retvalp)
fcproc_t	*fcpp;
int		incr;
int		*retvalp;
{
	int		newnice;
	fcparms_t	fcparms;

	/*
	 * Specifying a nice increment greater than the upper limit of
	 * 2 * NZERO - 1 will result in the process's nice value being
	 * set to the upper limit.  We check for this before computing
	 * the new value because otherwise we could get overflow 
	 * if a privileged process specified some ridiculous increment.
	 */
	if (incr > 2 * NZERO - 1)
		incr = 2 * NZERO - 1;

	newnice = fcpp->fc_nice + incr;
	if (newnice >= 2 * NZERO)
		newnice = 2 * NZERO - 1;
	else if (newnice < 0)
		newnice = 0;

	fcparms.fc_uprilim = fcparms.fc_upri =
	    -((newnice - NZERO) * fc_maxupri) / NZERO;

	/*
	 * Reset the uprilim and upri values of the process.
	 */
	(void)fc_parmsset(&fcparms, fcpp, (id_t)0, (cred_t *)NULL);

	/*
	 * Although fc_parmsset already reset fc_nice it may
	 * not have been set to precisely the value calculated above
	 * because fc_parmsset determines the nice value from the
	 * user priority and we may have truncated during the integer
	 * conversion from nice value to user priority and back.
	 * We reset fc_nice to the value we calculated above.
	 */
	fcpp->fc_nice = (char)newnice;

	if (retvalp)
		*retvalp = newnice - NZERO;
}
