/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:svc/uadmin.c	1.5"
#ident	"$Header: $"
#ident "$Header: uadmin.c 1.1 91/03/08 $"

#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/statvfs.h>
#include <fs/ustat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/faultcatch.h>
#include <mem/seg.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/ucontext.h>
#include <proc/unistd.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/hrtcntl.h>
#include <svc/resource.h>
#include <svc/sysconfig.h>
#include <svc/systeminfo.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/uadmin.h>
#include <svc/ulimit.h>
#include <svc/utsname.h>
#include <svc/utssys.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

#define UADMIN_SYNC 0
#define UADMIN_UMOUNT 1

void	shutdown();

/* sync/umount root filesystem */
STATIC void
dis_vfs(op)
	int op;
{
 	register struct vfs *pvfsp, *cvfsp, *ovfsp;

        pvfsp = rootvfs;
	cvfsp = pvfsp->vfs_next;

        while (cvfsp != NULL) {
		ovfsp = cvfsp;

                switch (op) {
		case UADMIN_SYNC:
			(void)VFS_SYNC(cvfsp, SYNC_CLOSE, u.u_cred);
			break;
		case UADMIN_UMOUNT:
			(void)dounmount(cvfsp, u.u_cred);
			break;
		default:
			break;
		}

		cvfsp = pvfsp->vfs_next;
		if (cvfsp == ovfsp) {
			pvfsp = cvfsp;
			cvfsp = cvfsp->vfs_next;
		}
	}
}

/*
 * Administrivia system call.
 */
struct uadmina {
	int	cmd;
	int	fcn;
	int	mdep;
};

/* ARGSUSED */
int
uadmin(uap, rvp)
	register struct uadmina *uap;
	rval_t *rvp;
{
	static ualock;
	int error = 0;

	/*
	 * check cmd arg (fcn is system dependent & defaulted in
	 * mdboot() if wrong
	 */
	switch (uap->cmd) {
	case A_SHUTDOWN:
	case A_REBOOT:
	case A_REMOUNT:
	case A_SWAPCTL:
	case A_CLOCK:
	case A_SETCONFIG:
		break;
	default:
		return EINVAL;
	}
	
	if (uap->cmd != A_SWAPCTL) {
		if (pm_denied(u.u_cred, P_SYSOPS))
			return EPERM;
		if (ualock)
			return 0;
		ualock = 1;
	}

	switch (uap->cmd) {
	case A_SHUTDOWN:
	{
		shutdown();
		/* FALLTHROUGH */
	}

	case A_REBOOT:
	{
		extern void mdboot();
		mdboot(uap->fcn, uap->mdep);
		/* no return expected */
		break;
	}

	case A_REMOUNT:
		/* remount root file system */
		error = VFS_MOUNTROOT(rootvfs, ROOT_REMOUNT);
		break;

	case A_SWAPCTL:
	{
		extern int swapctl();
		error = swapctl(uap, rvp);
		break;
	}

	case A_CLOCK:
		error = ConfigureTimeCorrection(uap->fcn);
		break;

	case A_SETCONFIG:
		error = (uap->fcn != AD_PANICBOOT) ? EINVAL :
			ConfigurePanic(uap->mdep);
		break;

	default:
		error = EINVAL;
	}

	if (uap->cmd != A_SWAPCTL)
		ualock = 0;

	return error;
}

#define TIME_TO_DIE	(15 * HZ)	/* allow process 15 seconds grace */

STATIC void
shutdown_proc(p)
	register struct proc	*p;
{
	register struct user	*pu;
	register int		id;
	register clock_t	t;

	psignal(p, SIGKILL);
	t = lbolt;
	id = timeout(wakeup, (caddr_t)p, TIME_TO_DIE);
	(void) sleep((caddr_t)p, PWAIT);
	untimeout(id);

	if (lbolt < t + TIME_TO_DIE) {
		/* Full timeout didn't elapse.
		   Either it died properly, or
		   we woke up prematurely;
		   in either case, just check
		   again later. */
		return;
	}
	if (p->p_exec != NULLVP
	 && p->p_stat != SZOMB) {
		/* Process still alive after timeout;
		   force it to go away */
		VN_RELE(p->p_exec);
		p->p_exec = NULLVP;
		/* Assume process is hung for good;
		   release its current and root dirs */
		pu = PTOU(p);
		CATCH_FAULTS(CATCH_SEGU_FAULT) {
			VN_RELE(pu->u_cdir);
			pu->u_cdir = rootdir;
			if (pu->u_rdir) {
				VN_RELE(pu->u_rdir);
				pu->u_rdir = NULLVP;
			}
		}
		END_CATCH();
	}
}

/*
 * common routine called by uadmin() and adt_shutdown().
 */
void
shutdown()
{
	register struct proc *p;

	/* 
	 * Hold all signals so we don't die.
	 */
	sigfillset(&u.u_procp->p_hold);	

	if (proc_init->p_stat != SZOMB)
		shutdown_proc(proc_init);
checkagain:
	for (p = practive; p != NULL; p = p->p_next) {
		if (p->p_exec != NULLVP	/* kernel daemons */
		  && p->p_stat != SZOMB
		  && p != u.u_procp) {
			shutdown_proc(p);
			goto checkagain;
		}
	}
	dis_vfs(UADMIN_SYNC);
	dis_vfs(UADMIN_UMOUNT);

	(void) VFS_MOUNTROOT(rootvfs, ROOT_UNMOUNT);
	/* FALLTHROUGH */
}
