/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/procfs/prvfsops.c	1.7.3.5"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/fs_subr.h>
#include <fs/procfs/prdata.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

struct vfs *procvfs;		/* points to /proc vfs entry */
dev_t procdev;

KSTATIC struct prnode prrootnode;	
STATIC int prmounted = 0;	/* set to 1 if /proc is mounted */

/*
 * /proc VFS operations vector.
 */
STATIC int prmount(), prunmount(), prroot(), prstatvfs();

STATIC struct vfsops prvfsops = {
	prmount,
	prunmount,
	prroot,
	prstatvfs,
	fs_sync,
	fs_nosys,	/* vget */
	fs_nosys,	/* mountroot */
	fs_nosys,	/* not used */
	fs_nosys,	/* setceiling */
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

/*
 * One-time initialization.
 */

int
prinit(vswp, fstype)
	register struct vfssw *vswp;
	int fstype;
{
	register int dev;

	/*
	 * Associate VFS ops vector with this fstype.
	 */
	vswp->vsw_vfsops = &prvfsops;

	/*
	 * Assign a unique "device" number (used by stat(2)).
	 */
	if ((dev = getudev()) == -1) {
		cmn_err(CE_WARN, "prinit: can't get unique device number");
		dev = 0;
	}
	procdev = makedevice(dev, 0);
	return 0;
}

/* ARGSUSED */
STATIC int
prmount(vfsp, mvp, uap, cr)
	struct vfs *vfsp;
	struct vnode *mvp;
	struct mounta *uap;
	struct cred *cr;
{
	register struct vnode *vp;
	register struct prnode *pnp;

	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	if (mvp->v_type != VDIR)
		return ENOTDIR;
	if (mvp->v_count > 1 || (mvp->v_flag & VROOT))
		return EBUSY;
	/*
	 * Prevent duplicate mount.
	 */
	if (prmounted)
		return EBUSY;
	pnp = &prrootnode;
	vp = &pnp->pr_vnode;
	vp->v_vfsp = vfsp;
	vp->v_vfsmountedhere = NULL;
	vp->v_op = &prvnodeops;
	vp->v_count = 1;
	vp->v_lid = mvp->v_lid;
	vp->v_macflag |= VMAC_SUPPORT;
	vp->v_type = VDIR;
	vp->v_data = (caddr_t) pnp;
	vp->v_flag |= VROOT;
	pnp->pr_mode = 0555;	/* read and search permissions */
	pnp->pr_vnext = NULL;
	pnp->pr_free = NULL;
	pnp->pr_proc = NULL;
	pnp->pr_opens = 0;
	pnp->pr_writers = 0;
	pnp->pr_flags = 0;
	vfsp->vfs_data = NULL;
	vfsp->vfs_dev = procdev;
	vfsp->vfs_fsid.val[0] = procdev;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
	vfsp->vfs_bsize = PRBSIZE;
	prmounted = 1;
	procvfs = vfsp;
	return 0;
}

/* ARGSUSED */
STATIC int
prunmount(vfsp, cr)
	struct vfs *vfsp;
	struct cred *cr;
{
	register proc_t *p;

	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	/*
	 * Ensure that no /proc vnodes are in use.
	 */
	if (prrootnode.pr_vnode.v_count > 1)
		return EBUSY;

	for (p = practive; p != NULL; p = p->p_next)
		if (p->p_trace != NULL)
			return EBUSY;

	VN_RELE(&prrootnode.pr_vnode);
	prmounted = 0;
	procvfs = NULL;
	return 0;
}

/* ARGSUSED */
STATIC int
prroot(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	struct vnode *vp = &prrootnode.pr_vnode;

	VN_HOLD(vp);
	*vpp = vp;
	return 0;
}

STATIC int
prstatvfs(vfsp, sp)
	struct vfs *vfsp;
	register struct statvfs *sp;
{
	register int i, n;

        for (n = v.v_proc, i = 0; i < v.v_proc; i++)
		if (pid_entry(i) != NULL)
                        n--;

	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize	= PRBSIZE;
	sp->f_frsize	= PRFSIZE;
	sp->f_blocks	= 0;
	sp->f_bfree	= 0;
	sp->f_bavail	= 0;
	sp->f_files	= v.v_proc + 2;
	sp->f_ffree	= n;
	sp->f_favail	= n;
	sp->f_fsid	= vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = PNSIZ;
	strcpy(sp->f_fstr, "/proc");
	strcpy(&sp->f_fstr[6], "/proc");
	return 0;
}
