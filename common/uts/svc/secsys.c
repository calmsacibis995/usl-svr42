/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:svc/secsys.c	1.10.3.1"
#ident	"$Header: $"

#include <acc/mac/covert.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <fs/vnode.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/secsys.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/types.h>
#include <util/var.h>

#if defined(__STDC__)

extern int mac_openlid(char *);
extern int mac_rootlid(lid_t);
extern int mac_adtlid(lid_t);
extern int pm_secsys(int, rval_t *, caddr_t);
extern int tp_getmajor(major_t *);
STATIC int mac_syslid(lid_t);

#else

extern int mac_openlid();
extern int mac_rootlid();
extern int mac_adtlid();
extern int pm_secsys();
extern int tp_getmajor();
STATIC int mac_syslid();

#endif

/*
 * Enhanced Security System Call.
 */

struct secsysa {
	int	cmd;
	caddr_t	arg;
};

int
secsys(uap, rvp)
	register struct secsysa *uap;
	rval_t *rvp;
{
	int error = 0;
	lid_t lid;
	major_t tpmajor;

	switch (uap->cmd) {
	case ES_MACOPENLID:	/* open LID file for kernel use */
		if (pm_denied(u.u_cred, P_SYSOPS)) {
                        error = EPERM;
                        break;
		}
		error = mac_openlid((char *)uap->arg);
		break;

	case ES_MACSYSLID:	/* assign LID to system processes */
		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)uap->arg, (caddr_t)&lid, sizeof(lid_t))) {
			error = EFAULT;
			break;
		}
		error = mac_syslid(lid);
		break;

	case ES_MACROOTLID:	/* assign LID to root fs root vnode */
		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)uap->arg, (caddr_t)&lid, sizeof(lid_t))) {
			error = EFAULT;
			break;
		}
		error = mac_rootlid(lid);
		break;

	case ES_PRVID:		/* get kernel privileged ID information */
	case ES_PRVINFO:	/* get kernel privilege information */
	case ES_PRVSETS:	/* get kernel privilege set information */
	case ES_PRVSETCNT:	/* get kernel privilege set count */
		error = pm_secsys(uap->cmd, rvp, uap->arg);
		break;

	case ES_MACADTLID:	/* assign LID to audit daemon */
		if (pm_denied(u.u_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)uap->arg, (caddr_t)&lid, sizeof(lid_t))){
			error = EFAULT;
			break;
		}
		error = mac_adtlid(lid);
		break;

	case ES_TPGETMAJOR:	/* Get Trusted Path major device number */
		if (tp_getmajor(&tpmajor) == 1){
			if(copyout((caddr_t)&tpmajor, (caddr_t)uap->arg,
			 sizeof(major_t))){
				error = EFAULT;
			}
		}else{ /* return value from stubs */
			error = ENODEV;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * secadvise	- enhanced security advisory system call
 *
 * This system call is used to provide advisory
 * information to trusted user level processes.
 */

#define	sec_macaccess(op, sub, obj, priv) \
	(MAC_ACCESS(op, (sub)->cr_lid, (obj)->lid) && \
	 pm_denied((sub), priv))

/* control structure for covert channel limiter */
STATIC ccevent_t cc_cache_macsec = { CC_CACHE_MACSEC, CCBITS_CACHE_MACSEC };


#define	TST_GROUP	3
#define	TST_OTHER	6

STATIC int
sec_dacaccess(objp, mode, crp)
	struct obj_attr *objp;
	int mode;
	struct cred *crp;
{
	int i;
	int lshift;
	int denied_mode;

	if (crp->cr_uid == objp->uid)
		lshift = 0;			/* TST OWNER */
	else if (groupmember(objp->gid, crp)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	} else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	if ((i = (objp->mode & mode)) == mode)
		return 0;

	denied_mode = (mode & (~i));
	denied_mode <<= lshift;
	if (((denied_mode & (VREAD | VEXEC)) && pm_denied(crp, P_DACREAD))
	||  ((denied_mode & VWRITE) && pm_denied(crp, P_DACWRITE)))
		return EACCES;

	return 0;
}

/*
 * secadvise
 *
 * This system call provides enhanced security advisory
 * information to trusted application programs.
 */

struct secadvisea {
	struct obj_attr *obj;
	int cmd;
	struct sub_attr *sub;
};

int
secadvise(uap, rvp)
	struct secadvisea *uap;
	rval_t *rvp;
{
	struct obj_attr obj;
	struct cred *subp;
	static size_t credsize = 0;
	int mode;
	int op;
	int priv;
	int error = 0;
	
	if (credsize == 0)
		credsize = crgetsize();

	/* if cmd is to return the sub_attr size, just return it */
	if (uap->cmd == SA_SUBSIZE) {
		rvp->r_val1 = credsize;
		return 0;
	}

	subp = (struct cred *)kmem_alloc(credsize, KM_SLEEP);

	if (copyin((caddr_t)uap->obj, (caddr_t)&obj, sizeof(obj))
	||  copyin((caddr_t)uap->sub, (caddr_t)subp, credsize)) {
		error = EFAULT;
		goto out;
	}
	
	switch (uap->cmd) {
	case SA_READ:
		mode = VREAD;
		op = MACDOM;
		priv = P_MACREAD;
		break;

	case SA_WRITE:
		mode = VWRITE;
		op = MACEQUAL;
		priv = P_MACWRITE;
		break;

	case SA_EXEC:
		mode = VEXEC;
		op = MACDOM;
		priv = P_MACREAD;
		break;
	}

	cc_limiter(&cc_cache_macsec, u.u_cred);

	if (sec_macaccess(op, subp, &obj, priv)
	||  sec_dacaccess(&obj, mode, subp))
		error = EACCES;

out:
	kmem_free(subp, credsize);
	return error;
}
/*
 * mac_syslid() assigns a LID to the system processes.
 *
 * Notes:
 *	1. This is a one shot call, i.e., once assigned, the level
 *	   of system processes cannot be altered.  An alternative
 *	   is to allow only higher levels.
 *	2. The level of all existing processes at the time of
 *	   calling this routines are set to the specified lid.
 *	   It is therefore very important that this routine be
 *	   called at initialization time.
 */

STATIC int
mac_syslid(lid)
	register lid_t lid;
{
	static int completed = 0;
	register proc_t *p;
	register int i;

	if (completed)
		return EBUSY;

	if (mac_valid(lid))
		return EINVAL;

	for (i = 0; i < v.v_proc; i++) {
		if ((p = pid_entry(i)) != (proc_t *)NULL
		&&  p->p_cred->cr_lid == (lid_t)0)
			p->p_cred->cr_lid = lid;
	}

	completed = 1;

	return 0;
}
