/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:svc/isc.c	1.7"
#ident	"$Header: $"

/* Enhanced Application Compatibility Support */

#include <util/types.h>
#include <svc/systm.h>
#include <proc/user.h>
#include <proc/reg.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>
#include <svc/sysconfig.h>
#include <proc/unistd.h>
#include <svc/umem.h>
#include <proc/procset.h>
#include <proc/procset.h>
#include <proc/wait.h>

#include <svc/isc.h>

extern int	nosys();
extern int	ngroups_max;

#define K_FAULT_ERR	"%s: generated bad user address (%#x)\n"

typedef long		isc_sigset_t;
typedef unsigned short	isc_uid_t;
typedef unsigned short	isc_gid_t;
typedef short		isc_pid_t;

int	__setostype(), rename_isc(), sigaction_isc(), sigprocmask_isc(),
	sigpending_isc(), getgroups_isc(), setgroups_isc(), pathconf_isc(),
	fpathconf_isc(), sysconf_isc(), fsync_isc(),
	waitpid_isc(), setsid_isc(), setpgid_isc();

struct sysent iscentry[] = {
        0, 0, nosys,			/* 0 = UNUSED */
	1, 0, __setostype,		/* 1 = __setostype */
	2, 0, rename_isc,		/* 2 = ISC rename */
	3, 0, sigaction_isc,		/* 3 = ISC sigaction */
	3, 0, sigprocmask_isc,		/* 4 = ISC sigprocmask */
	1, 0, sigpending_isc,		/* 5 = ISC sigpending */
	2, 0, getgroups_isc,		/* 6 = ISC getgroups */
	2, 0, setgroups_isc,		/* 7 = ISC setgroups */
	2, 0, pathconf_isc,		/* 8 = ISC pathconf */
	2, 0, fpathconf_isc,		/* 9 = ISC fpathconf */
	1, 0, sysconf_isc,		/* 10 = ISC sysconf */
	3, 0, waitpid_isc,		/* 11 = wait on PID */
	0, 0, setsid_isc,		/* 12 = create new session */
	2, 0, setpgid_isc,		/* 13 = set process group id */
	0, 0, nosys,			/* 14 */
	0, 0, nosys,			/* 15 */
	1, 0, fsync_isc			/* 16 = ISC fsync */
};

/* number of sysisc subfunctions */	
int niscentry = sizeof iscentry/sizeof(struct sysent);

/*
 *      sysisc - ISC custom system call dispatcher
 */

/* ARGSUSED */
int
sysisc(uap, rvp)
char *uap;
rval_t *rvp;
{
	register int subfunc;
	register struct user *uptr = &u;
	register struct sysent *callp;
	register int *ap;
	register u_int i;
	int ret;

	ap = (int *)u.u_ar0[UESP];
	ap++;			/* ap points to the return addr on the user's
				 * stack. bump it up to the actual args.  */
	subfunc = lfuword(ap++);	/* First argument is subfunction */
	if (subfunc <= 0 || subfunc >= niscentry)
		return EINVAL;
	callp = &iscentry[subfunc];

	/* get sysisc arguments in U block */
	for (i = 0; i < callp->sy_narg; i++){
		uptr->u_arg[i] = lfuword(ap++);
	}

	uptr->u_ap = uptr->u_arg;

	/* do the system call */
	ret = (*callp->sy_call)(uptr->u_ap, rvp);
	return ret;
}

#define ISC_SYSV_SEMANTICS	0
#define ISC_POSIX_SEMANTICS	1

struct isc_setostypea {
	int	type;
};

int
__setostype(uap, rvp)
	struct isc_setostypea	*uap;
	rval_t			*rvp;
{
	if (uap->type == ISC_POSIX_SEMANTICS)
		u.u_renv2 |= ISC_POSIX;
	rvp->r_val1 = 0;
	return 0;
}

/*
** Support for ISC implementation of rename(2)
*/
struct isc_renamea {
	char	*from;
	char	*to;
};

int
rename_isc(uap, rvp)
	struct isc_renamea	*uap;
	rval_t			*rvp;
{
	return rename(uap, rvp);
}

/*
** Support for ISC implementation of sigaction(2)
*/
#define ISC_SA_NOCLDSTOP	1	/* ISC's value for SA_NOCLDSTOP */

struct isc_sigaction {			/* ISC version of struct sigaction */
	void		(*sa_handler)();
	isc_sigset_t	sa_mask;
	int		sa_flags;
};

struct isc_sigactiona {
	int			sig;
	struct isc_sigaction	*act;
	struct isc_sigaction	*oact;
};

struct sigactiona {
	int			sig;
	struct sigaction	*act;
	struct sigaction	*oact;
};

/*
** Since the ISC and USL definitions of struct sigaction differ, this
** function has to do translation before and after calling
** scalls.c:sigaction().  The basic steps are (assuming `act' and `oact'
** are both non-NULL):
**	1. Copy in ISC-style structure.
**	2. Allocate *user* memory to hold USL-style sigaction struct.
**	3. Translate ISC format to USL.
**	4. Call real sigaction.
**	5. Translate old sigaction structure to ISC format.
**	6. Copy out ISC-style structure.
*/

/* ARGSUSED */
int
sigaction_isc(uap, rvp)
	struct isc_sigactiona	*uap;
	rval_t			*rvp;
{
	struct isc_sigaction	isc_act;
	struct sigaction	usl_act;
	struct sigactiona	nuap;
	int			error = 0;
	int			where;

	switch (uap->sig) {
	case ISC_SIGCONT:
		nuap.sig = SIGCONT;
		break;
	case ISC_SIGSTOP:
		nuap.sig = SIGSTOP;
		break;
	case ISC_SIGTSTP:
		nuap.sig = SIGTSTP;
		break;
	default:
		nuap.sig = uap->sig;
		break;
	}

	nuap.act = NULL;
	nuap.oact = NULL;

	if (uap->act != NULL) {
		if (copyin((caddr_t)uap->act,
			   (caddr_t)&isc_act, sizeof isc_act)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.act = (struct sigaction *)umem_alloc(sizeof *nuap.act,
							  &where);
		if (nuap.act == NULL) {
			error = ENOMEM;
			goto cleanup;
		}
		
		/* Translate ISC sigaction structure to the USL format */
		usl_act.sa_flags = isc_act.sa_flags & ISC_SA_NOCLDSTOP ?
					SA_NOCLDSTOP : 0;
		usl_act.sa_handler = isc_act.sa_handler;
		sigktou(&isc_act.sa_mask, &usl_act.sa_mask);

		if (copyout((caddr_t)&usl_act,
			    (caddr_t)nuap.act, sizeof usl_act)) {
			cmn_err(CE_WARN, K_FAULT_ERR, "sigaction", nuap.act);
			error = EFAULT;
			goto cleanup;
		}
	}

	/*
	** Only allocate space for one sigaction structure.  If we didn't
	** allocate one above and we need one for translation of `oact',
	** then allocate one now.  If we did allocate one above, just use
	** it, since sigaction(2) handles the case where `act' and `oact'
	** point to the same address.
	*/
	if (uap->oact != NULL)
		if (nuap.act == NULL) {
			nuap.oact =
			    (struct sigaction *)umem_alloc(sizeof *nuap.oact,
							   &where);
			if (nuap.oact == NULL) {
				error = ENOMEM;
				goto cleanup;
			}
		} else
			nuap.oact = nuap.act;

	if (error = sigaction(&nuap, rvp))
		goto cleanup;

	/*
	** The ISC implementation of sigaction(2) expects the old method of
	** returning from a handler, so we set it up here.
	*/
	u.u_sigreturn = (void (*)())u.u_ar0[EDX];
	sigaddset(&u.u_oldsig, nuap.sig);

	if (nuap.oact != NULL) {
		if (copyin((caddr_t)nuap.oact,
			   (caddr_t)&usl_act, sizeof usl_act)) {
			cmn_err(CE_WARN, K_FAULT_ERR, "sigaction", nuap.oact);
			error = EFAULT;
			goto cleanup;
		}

		/* Translate USL sigaction structre to the ISC format */
		isc_act.sa_flags = usl_act.sa_flags & SA_NOCLDSTOP ?
					ISC_SA_NOCLDSTOP : 0;
		isc_act.sa_handler = usl_act.sa_handler;
		sigutok(&usl_act.sa_mask, &isc_act.sa_mask);
		
		if (copyout((caddr_t)&isc_act,
			    (caddr_t)uap->oact, sizeof isc_act))
			error = EFAULT;
	}

 cleanup:
	if (nuap.act != NULL)
		umem_free((_VOID *)nuap.act, sizeof *nuap.act, where);
	else if (nuap.oact != NULL)
		umem_free((_VOID *)nuap.oact, sizeof *nuap.oact, where);

	return error;
}

/*
** Support for ISC implementation of sigprocmask(2)
*/
/* Converts ISC sigprocmask `how' arg. to USL value */
static int procmask_conv[] = {SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK};

struct isc_sigprocmaska {
	int		how;
	isc_sigset_t	*set;
	isc_sigset_t	*oset;
};

struct sigprocmaska {
	int		how;
	sigset_t	*set;
	sigset_t	*oset;
};

/* ARGSUSED */
int
sigprocmask_isc(uap, rvp)
	struct isc_sigprocmaska	*uap;
	rval_t			*rvp;
{
	isc_sigset_t		isc_set;
	isc_sigset_t		temp_isc_set;
	sigset_t		usl_set;
	struct sigprocmaska	nuap;
	int			error = 0;
	int			where;

	/* USL and ISC have different definitions of SIG_* */
	if (uap->how < 0 || uap->how > 2)
		return EINVAL;
	nuap.how = procmask_conv[uap->how];
	nuap.set = NULL;
	nuap.oset = NULL;

	if (uap->set != NULL) {
		if (copyin((caddr_t)uap->set,
			   (caddr_t)&isc_set, sizeof isc_set)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.set = (sigset_t *)umem_alloc(sizeof *nuap.set, &where);
		if (nuap.set == NULL) {
			error = ENOMEM;
			goto cleanup;
		}
		
		/* Structure Copy */
		temp_isc_set = isc_set;

		/* Convert 
		 * ISC_SIGCONT to SIGCONT, 
		 * ISC_SIGSTOP to SIGSTOP 
		 * ISC_SIGTSTP to SIGTSTP
		 */
	
		sigdelset(&isc_set, ISC_SIGCONT);
		sigdelset(&isc_set, ISC_SIGSTOP);
		sigdelset(&isc_set, ISC_SIGTSTP);

		if (sigismember(&temp_isc_set, ISC_SIGCONT))
			sigaddset(&isc_set, SIGCONT);
		if (sigismember(&temp_isc_set, ISC_SIGSTOP))
			sigaddset(&isc_set, SIGSTOP);
		if (sigismember(&temp_isc_set, ISC_SIGTSTP))
			sigaddset(&isc_set, SIGTSTP);

		/* Convert ISC sigset to the USL format */
		sigktou(&isc_set, &usl_set);

		if (copyout((caddr_t)&usl_set,
			    (caddr_t)nuap.set, sizeof usl_set)) {
			cmn_err(CE_WARN, K_FAULT_ERR, "sigprocmask", nuap.set);
			error = EFAULT;
			goto cleanup;
		}
	}

	/*
	** Only allocate space for one sigset.  If we didn't
	** allocate one above and we need one for conversion of `oset',
	** then allocate one now.  If we did allocate one above, just use
	** it, since sigprocmask(2) handles the case where `set' and `oset'
	** point to the same address.
	*/
	if (uap->oset != NULL)
		if (nuap.set == NULL) {
			nuap.oset = (sigset_t *)umem_alloc(sizeof *nuap.oset,
							   &where);
			if (nuap.oset == NULL) {
				error = ENOMEM;
				goto cleanup;
			}
		} else
			nuap.oset = nuap.set;

	if (error = sigprocmask(&nuap, rvp))
		goto cleanup;

	if (nuap.oset != NULL) {
		if (copyin((caddr_t)nuap.oset,
			   (caddr_t)&usl_set, sizeof usl_set)) {
			cmn_err(CE_WARN, K_FAULT_ERR, "sigprocmask",nuap.oset);
			error = EFAULT;
			goto cleanup;
		}

		/* Convert USL sigset to the ISC format */
		sigutok(&usl_set, &isc_set);
		sigutok(&usl_set, &temp_isc_set);
		
		/* Convert 
		 * SIGCONT to ISC_SIGCONT, 
		 * SIGSTOP to ISC_SIGSTOP 
		 * SIGTSTP to ISC_SIGTSTP
		 */
	
		sigdelset(&isc_set, SIGCONT);
		sigdelset(&isc_set, SIGSTOP);
		sigdelset(&isc_set, SIGTSTP);

		if (sigismember(&temp_isc_set, SIGCONT))
			sigaddset(&isc_set, ISC_SIGCONT);
		if (sigismember(&temp_isc_set, SIGSTOP))
			sigaddset(&isc_set, ISC_SIGSTOP);
		if (sigismember(&temp_isc_set, SIGTSTP))
			sigaddset(&isc_set, ISC_SIGTSTP);

		if (copyout((caddr_t)&isc_set,
			    (caddr_t)uap->oset, sizeof isc_set))
			error = EFAULT;
	}

 cleanup:
	if (nuap.set != NULL)
		umem_free((_VOID *)nuap.set, sizeof *nuap.set, where);
	else if (nuap.oset != NULL)
		umem_free((_VOID *)nuap.oset, sizeof *nuap.oset, where);

	return error;
}

/*
** Support for ISC implementation of sigpending(2)
*/
struct isc_sigpendinga {
	isc_sigset_t	*set;
};

struct sigpendinga {
	int		flag;
	sigset_t	*set;
};

/* ARGSUSED */
int
sigpending_isc(uap, rvp)
	struct isc_sigpendinga	*uap;
	rval_t			*rvp;
{
	isc_sigset_t		isc_set;
	isc_sigset_t		temp_isc_set;
	sigset_t		usl_set;
	struct sigpendinga	nuap;
	int			error = 0;
	int			where;

	nuap.flag = 1;			/* Ask for sigpending, not sigfillset*/
	nuap.set = (sigset_t *)umem_alloc(sizeof *nuap.set, &where);
	if (nuap.set == NULL) {
		error = ENOMEM;
		goto cleanup;
	}
		
	if (error = sigpending(&nuap, rvp))
		goto cleanup;;

	if (copyin((caddr_t)nuap.set, (caddr_t)&usl_set, sizeof usl_set)) {
		cmn_err(CE_WARN, K_FAULT_ERR, "sigpending", nuap.set);
		error = EFAULT;
		goto cleanup;
	}

	/* Convert USL sigset to the ISC format */
	sigutok(&usl_set, &isc_set);
	sigutok(&usl_set, &temp_isc_set);

	/* Convert 
	 * SIGCONT to ISC_SIGCONT, 
	 * SIGSTOP to ISC_SIGSTOP 
	 * SIGTSTP to ISC_SIGTSTP
	 */

	sigdelset(&isc_set, SIGCONT);
	sigdelset(&isc_set, SIGSTOP);
	sigdelset(&isc_set, SIGTSTP);

	if (sigismember(&temp_isc_set, SIGCONT))
		sigaddset(&isc_set, ISC_SIGCONT);
	if (sigismember(&temp_isc_set, SIGSTOP))
		sigaddset(&isc_set, ISC_SIGSTOP);
	if (sigismember(&temp_isc_set, SIGTSTP))
		sigaddset(&isc_set, ISC_SIGTSTP);

	if (copyout((caddr_t)&isc_set, (caddr_t)uap->set, sizeof isc_set))
		error = EFAULT;

 cleanup:
	if (nuap.set != NULL)
		umem_free((_VOID *)nuap.set, sizeof *nuap.set, where);

	return error;
}

/*
** Support for ISC implementation of getgroups(2)
*/
struct isc_getgroupsa {
	int		gidsetsize;
	isc_gid_t	*gidset;
};

struct getgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

int
getgroups_isc(uap, rvp)
	struct isc_getgroupsa	*uap;
	rval_t			*rvp;
{
	register int		gidsetsize = uap->gidsetsize;
	size_t			isc_size = gidsetsize * sizeof(isc_gid_t);
	isc_gid_t		*isc_set = NULL;
	size_t			usl_size = gidsetsize * sizeof(gid_t);
	gid_t			*usl_set = NULL;
	struct getgroupsa	nuap;
	int			error = 0;
	int			where;

	nuap.gidsetsize = gidsetsize;

	if (gidsetsize > 0) {
		nuap.gidset = (gid_t *)umem_alloc(usl_size, &where);
		if ( nuap.gidset == NULL) {
			error = ENOMEM;
			goto cleanup;
		}
	}

	if (error = getgroups(&nuap, rvp))
		goto cleanup;

	if (gidsetsize > 0) {
		register isc_gid_t	*isc_setp;
		register gid_t		*usl_setp;
		register int		i;

		usl_set = (gid_t *)kmem_alloc(usl_size, KM_SLEEP);
		usl_setp = usl_set;

		if (copyin((caddr_t)nuap.gidset, (caddr_t)usl_set, usl_size)) {
			cmn_err(CE_WARN, K_FAULT_ERR, "getgroups",nuap.gidset);
			error = EFAULT;
			goto cleanup;
		}

		isc_set = (isc_gid_t *)kmem_alloc(isc_size, KM_SLEEP);
		isc_setp = isc_set;

		for (i = 0; i < gidsetsize; ++i)
			*isc_setp++ = *usl_setp++;

		if (copyout((caddr_t)isc_set, (caddr_t)uap->gidset, isc_size))
			error = EFAULT;
	}

 cleanup:
	if (nuap.gidset != NULL)
		umem_free((_VOID *)nuap.gidset, usl_size, where);

	if (isc_set != NULL)
		kmem_free((_VOID *)isc_set, isc_size);

	if (usl_set != NULL)
		kmem_free((_VOID *)usl_set, usl_size);

	return error;
}

/*
** Support for ISC implementation of setgroups(2)
*/
struct isc_setgroupsa {
	int		gidsetsize;
	isc_gid_t	*gidset;
};

struct setgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

int
setgroups_isc(uap, rvp)
	struct isc_setgroupsa	*uap;
	rval_t			*rvp;
{
	register int		gidsetsize = uap->gidsetsize;
	size_t			isc_size = gidsetsize * sizeof(isc_gid_t);
	isc_gid_t		*isc_set = NULL;
	size_t			usl_size = gidsetsize * sizeof(gid_t);
	gid_t			*usl_set = NULL;
	struct setgroupsa	nuap;
	int			error = 0;
	int			where;

	if (gidsetsize > ngroups_max)
		return EINVAL;

	nuap.gidsetsize = gidsetsize;

	if (gidsetsize > 0) {
		register isc_gid_t	*isc_setp;
		register gid_t		*usl_setp;
		register int i;

		isc_set = (isc_gid_t *)kmem_alloc(isc_size, KM_SLEEP);
		isc_setp = isc_set;

		if (copyin((caddr_t)uap->gidset, (caddr_t)isc_set, isc_size)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.gidset = (gid_t *)umem_alloc(usl_size, &where);
		if (nuap.gidset == NULL) {
			error = ENOMEM;
			goto cleanup;
		}

		usl_set = (gid_t *)kmem_alloc(usl_size, KM_SLEEP);
		usl_setp = usl_set;

		for (i = 0; i < gidsetsize; ++i)
			*usl_setp++ = *isc_setp++;

		if (copyout((caddr_t)usl_set, (caddr_t)nuap.gidset, usl_size)){
			cmn_err(CE_WARN, K_FAULT_ERR, "setgroups",nuap.gidset);
			error = EFAULT;
			goto cleanup;
		}
	}

	error = setgroups(&nuap, rvp);

 cleanup:
	if (nuap.gidset != NULL)
		umem_free((_VOID *)nuap.gidset, usl_size, where);

	if (isc_set != NULL)
		kmem_free((_VOID *)isc_set, isc_size);

	if (usl_set != NULL)
		kmem_free((_VOID *)usl_set, usl_size);

	return error;
}

/*
** Support for ISC implementation of sysconf(2)
*/
#define ISC_SC_ARG_MAX		1
#define ISC_SC_CHILD_MAX	2
#define ISC_SC_CLK_TCK		3
#define ISC_SC_NGROUPS_MAX	4
#define ISC_SC_OPEN_MAX		5
#define ISC_SC_JOB_CONTROL	6
#define ISC_SC_SAVED_IDS	7
#define ISC_SC_VERSION		8

#define ISC_POSIX_JOB_CONTROL	1
#define ISC_POSIX_SAVED_IDS	1

struct isc_sysconfa {			/* The USL and ISC argument */
	int	which;			/* structures are the same */
};

int
sysconf_isc(uap, rvp)
	struct isc_sysconfa	*uap;
	rval_t			*rvp;
{
	extern int	exec_ncargs;	/* Value of ARG_MAX tunable */
	int		error = 0;

	switch(uap->which) {
		case ISC_SC_ARG_MAX:
			rvp->r_val1 = exec_ncargs;
			break;

		case ISC_SC_CHILD_MAX:
			uap->which = _CONFIG_CHILD_MAX;
			error = sysconfig(uap, rvp);
			break;

		case ISC_SC_CLK_TCK:
			uap->which = _CONFIG_CLK_TCK;
			error = sysconfig(uap, rvp);
			break;

		case ISC_SC_NGROUPS_MAX:
			uap->which = _CONFIG_NGROUPS;
			error = sysconfig(uap, rvp);
			break;

		case ISC_SC_OPEN_MAX:
			uap->which = _CONFIG_OPEN_FILES;
			error = sysconfig(uap, rvp);
			break;

		case ISC_SC_JOB_CONTROL:
			rvp->r_val1 = ISC_POSIX_JOB_CONTROL;
			break;

		case ISC_SC_SAVED_IDS:
			rvp->r_val1 = ISC_POSIX_SAVED_IDS;
			break;

		case ISC_SC_VERSION:
			uap->which = _CONFIG_POSIX_VER;
			error = sysconfig(uap, rvp);
			break;

		default:
			error = EINVAL;
			break;
	}

	return error;
}

/*
** Support for ISC implementation of pathconf(2)
*/

/* Table to convert ISC pathconf args to USL values */
static int	pathconf_conv[] = {
	-1,				/* 0 - Illegal value */
	_PC_LINK_MAX,			/* 1 - No conversion */
	_PC_MAX_CANON,			/* 2 - No conversion */
	_PC_MAX_INPUT,			/* 3 - No conversion */
	_PC_NAME_MAX,			/* 4 - No conversion */
	_PC_PATH_MAX,			/* 5 - No conversion */
	_PC_PIPE_BUF,			/* 6 - No conversion */
	_PC_CHOWN_RESTRICTED,		/* 7 */
	_PC_NO_TRUNC,			/* 8 */
	_PC_VDISABLE			/* 9 */
};

static size_t	pathconf_conv_size = sizeof pathconf_conv / sizeof(int);

struct isc_pathconfa {			/* The USL and ISC argument */
	char*	fname;			/* structures are the same */
	int	name;
};

int
pathconf_isc(uap, rvp)
	struct isc_pathconfa	*uap;
	rval_t			*rvp;
{
	if (uap->name < 0 || uap->name >= pathconf_conv_size)
		return EINVAL;

	uap->name = pathconf_conv[uap->name];
	return pathconf(uap, rvp);
}

struct isc_fpathconfa {			/* The USL and ISC argument */
	int	fdes;			/* structures are the same */
	int	name;
};

int
fpathconf_isc(uap, rvp)
	struct isc_fpathconfa	*uap;
	rval_t			*rvp;
{
	if (uap->name < 0 || uap->name >= pathconf_conv_size)
		return EINVAL;

	uap->name = pathconf_conv[uap->name];
	return fpathconf(uap, rvp);
}

/*
** Support for ISC implementation of fsync(2)
*/
struct isc_fsynca {
	int	fd;
};

int
fsync_isc(uap, rvp)
	struct isc_fsynca	*uap;
	rval_t			*rvp;
{
	return fsync(uap, rvp);
}

struct	isc_waitpida {
	int     pid;
	int    *stat_loc;
	int     options;
};

struct svr4_waitsysa {
	idtype_t idtype;
	id_t	 id;
	siginfo_t *infop;
	int	 options;
};

waitpid_isc(uap, rvp)
	struct isc_waitpida	*uap;
	rval_t			*rvp;
{
	int			options;
	int			pid;

	struct svr4_waitsysa	nuap;
	idtype_t		idtype;
	id_t			id;
	siginfo_t		*infop;
	int			where;
	int			rval;
	

	/* Validate options: only these bits may be set */
	if (uap->options & ~(ISC_WNOHANG | ISC_WUNTRACED))
		return(EINVAL);

	nuap.options = (WEXITED|WTRAPPED);
	options = uap->options;
	if (options & ISC_WNOHANG)
		nuap.options |= WNOHANG;
	if (options & ISC_WUNTRACED)
		nuap.options |= WUNTRACED;

	pid = uap->pid;
	if (pid > 0) {
		idtype = P_PID;
		id = pid;
	} else if (pid < -1) {
		idtype = P_PGID;
		id = -pid;
	} else if (pid == -1) {
		idtype = P_ALL;
		id = 0;
	} else {
		idtype = P_PGID;
		id = u.u_procp->p_pgrp;
	}
	
	infop = (siginfo_t *)umem_alloc(sizeof *infop, &where);
	if (infop == NULL)
		return(ENOMEM);

	nuap.idtype = idtype;
	nuap.id = id;
	nuap.infop = infop;
	rval = waitsys(&nuap, rvp);

	if (!rval) {
		rvp->r_val1 = infop->si_pid;
		rvp->r_val2 = wstat_isc(infop->si_code, infop->si_status);
	
		if (uap->stat_loc) {
			if (suword(uap->stat_loc, rvp->r_val2) == -1)
				rval = EFAULT;
		
		}
	}

	umem_free((_VOID *)infop, sizeof *infop, where);
	return(rval);
}


/*
 * convert code/data pair into old style wait status
 */

int
wstat_isc(code, data)
{
	register stat = (data & 0377);

	switch (code) {
		case CLD_EXITED:
			stat <<= 8;
			break;
		case CLD_DUMPED:
			stat |= WCOREFLG;
			break;
		case CLD_KILLED:
			break;
		case CLD_TRAPPED:
		case CLD_STOPPED:
			stat <<= 8;
			stat |= WSTOPFLG;
			break;
		case CLD_CONTINUED:
			stat = WCONTFLG;
			break;
	}
	return stat;
}

struct svr4_setsida {
	int	flag;
	int	pid;
	int	pgid;
};

int
setsid_isc(uap, rvp)
	int			*uap;
	rval_t			*rvp;
{
	struct svr4_setsida	nuap;

	/* Want the Base Code (setpgrp()) to be able to detect
	 * that call is not a SCO request. 
	 */

	if (!ISC_USES_POSIX)
		return(EINVAL);

	nuap.flag = 3;	/* SVR4 setsid() */

	return (setpgrp(&nuap, rvp));
}

struct isc_setpgida {
	int	pid;
	int	pgid;
};

struct svr4_setpgida {
	int	flag;
	int	pid;
	int	pgid;
};

int
setpgid_isc(uap, rvp)
	struct isc_setpgida	*uap;
	rval_t			*rvp;
{
	struct svr4_setpgida	nuap;

	/* Want the Base Code (setpgrp()) to be able to detect
	 * that call is not a SCO request. 
	 */

	if (!ISC_USES_POSIX)
		return(EINVAL);

	nuap.flag = 5;	/* SVR4 setpgid() */
	nuap.pid = uap->pid;
	nuap.pgid = uap->pgid;

	return (setpgrp(&nuap, rvp));
}

/* End Enhanced Application Compatibility Support */
