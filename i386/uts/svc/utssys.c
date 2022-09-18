/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:svc/utssys.c	1.7"
#ident	"$Header: $"

#include <acc/mac/mac.h>
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

/*
 * utssys()
 */
 
struct utssysa {
	union {
		char *cbuf;
		struct ustat *ubuf;
	} ub;
	union {
		int	mv;		/* for USTAT */
		int	flags;		/* for FUSERS */
	} un;
	int	type;
	char	*outbp;			/* for FUSERS */
};

int
utssys(uap, rvp)
	register struct utssysa *uap;
	rval_t *rvp;
{
	register int error = 0;

	switch (uap->type) {

	case UTS_UNAME:
	{
		char *buf = uap->ub.cbuf;

		/* XENIX Support */
		if (VIRTUAL_XOUT) {
			/* return the XENIX utsname structure */
			if (copyout((caddr_t)&xutsname, buf,
				sizeof(struct xutsname))) {
				error = EFAULT;
				break;
			}
			break;
		}
		/* End XENIX Support */
		if (copyout(utsname.sysname, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;
		if (copyout(utsname.nodename, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;
		if (copyout(utsname.release, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;
		if (copyout(utsname.version, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;
		if (copyout(utsname.machine, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		rvp->r_val1 = 1;
		break;
	}

	case UTS_USTAT:
	{
		register struct vfs *vfsp;
		struct ustat ust;
		struct statvfs stvfs;
		char *cp, *cp2;
		int i;
		extern int rf_ustat();

		/* NFA ustat hook */
		if (error = nfc_ustat())
			return(error);
		/* end NFA */

		/*
		 * RFS HOOK.
		 */
		if (uap->un.mv < 0)
			return rf_ustat((dev_t)uap->un.mv, uap->ub.ubuf);
		/*
		 * Search vfs list for user-specified device.
		 */
		for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next)
			if (vfsp->vfs_dev == uap->un.mv || 
				cmpdev(vfsp->vfs_dev) == uap->un.mv)

				break;
		if (vfsp == NULL) {
			error = EINVAL;
			break;
		}
		if (error = VFS_STATVFS(vfsp, &stvfs))
			break;
		if (stvfs.f_ffree > USHRT_MAX) {
			error = EOVERFLOW;
			break;
		}

                /* If the level of the calling process does not dominate the
                 * file system level ceiling, zero out blocks free and files
                 * free to prevent a covert channel.  If the process has
                 * P_FSYSRANGE or P_COMPAT, don't bother.
                 */

                if (MAC_ACCESS(MACDOM, u.u_cred->cr_lid, vfsp->vfs_macceiling)
                &&  pm_denied(u.u_cred, P_FSYSRANGE)
                &&  pm_denied(u.u_cred, P_COMPAT)) {
                        ust.f_tfree = 0;
                        ust.f_tinode = 0;
                } else {
			/* XENIX Support */
			if (VIRTUAL_XOUT) {
				/*
				 * Note that ustat() converts f_bfree
				 * to 512 byte blocks. XENIX binaries
				 * expect to receive the number of
				 * free logical blocks, not the number
				 * of 512 byte free blocks. We need to
				 * undo the conversion of 512 byte
				 * blocks, for XENIX compatibility.
				 */

				ust.f_tfree = (daddr_t) stvfs.f_bfree;
			/* End XENIX Support */

			} else {
                        	ust.f_tfree = (daddr_t)
                                (stvfs.f_bfree * (stvfs.f_frsize/512));
			}
                        ust.f_tinode = (o_ino_t)stvfs.f_ffree;
                }

		cp = stvfs.f_fstr;
		cp2 = ust.f_fname;
		i = 0;
		while (i++ < sizeof(ust.f_fname))
			if (*cp != '\0')
				*cp2++ = *cp++;
			else
				*cp2++ = '\0';
		while (*cp != '\0'
		  && (i++ < sizeof(stvfs.f_fstr) - sizeof(ust.f_fpack)))
			cp++;
		cp++;
		cp2 = ust.f_fpack;
		i = 0;
		while (i++ < sizeof(ust.f_fpack))
			if (*cp != '\0')
				*cp2++ = *cp++;
			else
				*cp2++ = '\0';
		if (copyout((caddr_t)&ust, uap->ub.cbuf, sizeof(ust)))
			error = EFAULT;
		break;
	}

	case UTS_FUSERS:
	{
		STATIC int uts_fusers();
		return uts_fusers(uap->ub.cbuf, uap->un.flags, uap->outbp, rvp);
	}

	default:
		error = EINVAL;		/* ? */
		break;
	}

	return error;
}

/*
 * Determine the ways in which processes are using a named file or mounted
 * file system (path).  Normally return 0 with rvp->rval1 set to the number of 
 * processes found to be using it.  For each of these, fill a f_user_t to
 * describe the process and its useage.  When successful, copy this list
 * of structures to the user supplied buffer (outbp).
 *
 * In error cases, clean up and return the appropriate errno.
 */
STATIC int
uts_fusers(path, flags, outbp, rvp)
	char *path;
	int flags;
	char *outbp;
	rval_t *rvp;
{
	vnode_t *fvp = NULL;
	int error;
	extern int lookupname();
        STATIC int dofusers();

	if (pm_denied(u.u_cred, P_COMPAT))
                return EPERM;

        if ((error = lookupname(path, UIO_USERSPACE, FOLLOW, NULLVPP, &fvp))
	  != 0) {
		return error;
	}
	ASSERT(fvp);
	error = dofusers(fvp, flags, outbp, rvp);
	VN_RELE(fvp);
	return error;
}

STATIC int
dofusers(fvp, flags, outbp, rvp)
	vnode_t *fvp;
	int flags;
	char *outbp;
	rval_t *rvp;
{
	register proc_t *prp;
	register int pcnt = 0;		/* number of f_user_t's copied out */
	int error = 0;
	register int contained = (flags == F_CONTAINED);
	register vfs_t *cvfsp;
	register int use_flag = 0;
	register int fd;
	register struct ufchunk *ufp;
	file_t *fp;
	f_user_t *fuentry, *fubuf;	/* accumulate results here */

	if ((fuentry = fubuf = 
	  (f_user_t *)kmem_alloc(v.v_proc * sizeof(f_user_t), 0)) == NULL) {
		return ENOMEM;
	}
	if (contained && !(fvp->v_flag & VROOT)) {
		error = EINVAL;
		goto out;
	}
	if (fvp->v_count == 1) {	/* no other active references */
		goto out;
	}
	cvfsp = fvp->v_vfsp;
	ASSERT(cvfsp);
	for (prp = practive; prp != NULL; prp = prp->p_next) {
		register user_t *up;

		if (prp->p_stat == SZOMB || prp->p_stat == SIDL)
			continue;

		up = (user_t *)KUSER(prp->p_segu);
		CATCH_FAULTS(CATCH_SEGU_FAULT) {
			if (up->u_cdir && (VN_CMP(fvp, up->u_cdir) ||
			  contained && up->u_cdir->v_vfsp == cvfsp)) {
				use_flag |= F_CDIR;
			}
			if (up->u_rdir && (VN_CMP(fvp, up->u_rdir) ||
			  contained && up->u_rdir->v_vfsp == cvfsp)) {
				use_flag |= F_RDIR;
			}
			if (prp->p_exec && (VN_CMP(fvp, prp->p_exec) ||
			  contained && prp->p_exec->v_vfsp == cvfsp)) {
				use_flag |= F_TEXT;
			}
			if (prp->p_trace && (VN_CMP(fvp, prp->p_trace) ||
			  contained && prp->p_trace->v_vfsp == cvfsp)) {
				use_flag |= F_TRACE;
			}
			/* In this code the vnodes associated with the
			 * same special file do not equal each other.
			 * One way to check if both refer to the same
			 * special file is to look at the snode associated
			 * with the file and from the snode figure out
			 * the common vnode.
			 */
			/*if (prp->p_sessp && (VN_CMP(fvp,prp->p_sessp->s_vp) ||
			  contained && prp->p_sessp->s_vp && 
			  prp->p_sessp->s_vp->v_vfsp == cvfsp)) {
				use_flag |= F_TTY;
			}*/
			ufp = &(up->u_flist);
			for (fd = 0; fd < up->u_nofiles; fd++) {
				fp = ufp->uf_ofile[fd % NFPCHUNK];
				if (fp != NULLFP && fp->f_vnode && 
				  (VN_CMP(fvp, fp->f_vnode) ||
				  contained && fp->f_vnode->v_vfsp == cvfsp)) {
					use_flag |= F_OPEN;
					break;		/* we don't count fds */
				}
				if ((fd + 1) % NFPCHUNK == 0) {
					ufp = ufp->uf_next;
				}
			}
			/*
			 * mmap usage??
			 */
		}
		END_CATCH();
		if (use_flag) {
			fuentry->fu_pid = prp->p_pid;
			fuentry->fu_flags = use_flag;
			fuentry->fu_uid = (uid_t) prp->p_uid;
			fuentry++;
			pcnt++;
			use_flag = 0;
		}
	}
	if (copyout((caddr_t)fubuf, outbp, pcnt * sizeof(f_user_t))) {
		error = EFAULT;
	}
out:
	kmem_free(fubuf, v.v_proc * sizeof(f_user_t));
	rvp->r_val1 = pcnt;
	return error;
}
