/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/namefs/namevno.c	1.18.3.2"
#ident	"$Header: $"
/*
 * This file defines the vnode operations for mounted file descriptors.
 * The routines in this file act as a layer between the NAMEFS file 
 * system and SPECFS/FIFOFS.  With the exception of nm_open(), nm_setattr(),
 * nm_getattr() and nm_access(), the routines simply apply the VOP 
 * operation to the vnode representing the file descriptor.  This switches 
 * control to the underlying file system to which the file descriptor 
 * belongs.
 */

#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/namefs/namenode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/poll.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Define the routines within this file.
 */
STATIC int	nm_open(),     nm_close(),   nm_read(),    nm_write();
STATIC int	nm_ioctl(),    nm_getattr(), nm_setattr(), nm_access();
STATIC int	nm_link(),     nm_fsync(),   nm_fid(),     nm_seek();
STATIC int	nm_realvp(),   nm_poll(),    nm_create();
STATIC void	nm_inactive(), nm_rwlock(),  nm_rwunlock();
STATIC int	nm_getacl(),   nm_setacl(),  nm_getaclcnt();
STATIC mode_t	nm_daccess();

int	nm_aclstore();


/*
 * Define external routines.
 */
extern	u_short	nmgetid();
extern	int	cleanlocks();
extern	void	nmclearid(),   nameinsert(), nameremove();
extern	struct	namenode *namefind();

struct vnodeops nm_vnodeops = {
	nm_open,
	nm_close,
	nm_read,
	nm_write,
	nm_ioctl,
	fs_setfl,
	nm_getattr,
	nm_setattr,
	nm_access,
	fs_nosys,	/* lookup */
	nm_create,
	fs_nosys,	/* remove */
	nm_link,
	fs_nosys,	/* rename */
	fs_nosys,	/* mkdir */
	fs_nosys,	/* rmdir */
	fs_nosys,	/* readdir */
	fs_nosys,	/* symlink */
	fs_nosys,	/* readlink */
	nm_fsync,
	nm_inactive,
	nm_fid,
	nm_rwlock,
	nm_rwunlock,
	nm_seek,
	fs_cmp,
	fs_frlock,
	fs_nosys,	/* space */
	nm_realvp,
	fs_nosys,	/* getpages */
	fs_nosys,	/* putpages */
	fs_nosys,	/* map */
	fs_nosys,	/* addmap */
	fs_nosys,	/* delmap */
	nm_poll,
	fs_nosys,	/* not used */
	fs_pathconf,
	fs_nosys,	/* allocstore */
	nm_getacl,
	nm_getaclcnt,
	nm_setacl,
	fs_nosys,	/* setlevel */
	fs_nosys,	/* getdvstat */
	fs_nosys,	/* setdvstat */
	fs_nosys,	/* makemld */
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};
 
/*
 * Create a reference to the vnode representing the file descriptor.
 * Then, apply the VOP_OPEN operation to that vnode.
 *
 * The vnode for the file descriptor may be switched under you.
 * If it is, search the hash list for an nodep - nodep->nm_filevp
 * pair. If it exists, return that nodep to the user.
 * If it does not exist, create a new namenode to attach
 * to the nodep->nm_filevp then place the pair on the hash list.
 *
 * Newly created objects are like children/nodes in the mounted
 * file system, with the parent being the initial mount.
 */
STATIC int
nm_open(vpp, flag, crp)
	struct vnode **vpp;
	int flag;
	struct cred *crp;
{
	register struct namenode *nodep = VTONM(*vpp);
	register int error = 0;
	register struct namenode *newnamep;
	struct vnode *newvp;
	struct vnode *infilevp;
	struct vnode *outfilevp;

	ASSERT(nodep->nm_filevp);
	/*
	 * If the vnode is switched under us, the corresponding
	 * VN_RELE for this VN_HOLD will be done by the file system
	 * performing the switch. Otherwise, the corresponding
	 * VN_RELE will be done by nm_close().
	 */
	VN_HOLD(nodep->nm_filevp);
	infilevp = outfilevp = nodep->nm_filevp;

	if ((error = VOP_OPEN(&outfilevp, flag | FNMFS, crp)) != 0) {
		VN_RELE(nodep->nm_filevp);
		return (error);
	}
	if (infilevp != outfilevp) {
		/*
		 * See if the new filevp (outfilevp) is already associated
		 * with the mount point. If it is, then it already has a
		 * namenode associated with it.
		 */
		if ((newnamep = namefind(outfilevp, nodep->nm_mountpt)) != NULL)
			goto gotit;

		/*
		 * Create a new namenode. 
		 */
		newnamep =
			(struct namenode *)kmem_zalloc (sizeof(struct namenode),
				KM_SLEEP);

		/*
		 * Initialize the fields of the new vnode/namenode.
		 * Then overwrite the fields that should not be copied.
		 */
		*newnamep = *nodep;

		newvp = NMTOV(newnamep);
		newvp->v_flag &= ~VROOT;  	/* new objects are not roots */
		newvp->v_flag |= VNOMAP|VNOSWAP;
		newvp->v_count = 0;        	/* bumped down below */
		newvp->v_vfsmountedhere = NULL;
		newvp->v_vfsp = (*vpp)->v_vfsp;
		newvp->v_stream = outfilevp->v_stream;
		newvp->v_pages = NULL;
		newvp->v_type = outfilevp->v_type;
		newvp->v_rdev = outfilevp->v_rdev;
		newvp->v_lid = outfilevp->v_lid;
		/* set VMAC_DOPEN if it is set in outfilevp */
		if(outfilevp->v_macflag & VMAC_DOPEN)
			newvp->v_macflag |= VMAC_DOPEN;
		newvp->v_macflag |= VMAC_SUPPORT;
		newvp->v_data = (caddr_t) newnamep;
		newvp->v_filocks = NULL;
		newnamep->nm_vattr.va_type = outfilevp->v_type;
		newnamep->nm_vattr.va_nodeid = nmgetid();
		newnamep->nm_vattr.va_size = 0;
		newnamep->nm_vattr.va_rdev = outfilevp->v_rdev;
		newnamep->nm_flag = 0;
		newnamep->nm_filevp = outfilevp;
		/*
		 * Hold the associated file structure and the new vnode to
		 * be sure they are ours until nm_inactive or nm_unmount.
		 */
		newnamep->nm_filep->f_count++;
		VN_HOLD(outfilevp);
		newnamep->nm_mountpt = nodep->nm_mountpt;
		newnamep->nm_backp = newnamep->nm_nextp = NULL;

		/*
		 * Insert the new namenode into the hash list.
		 */
		nameinsert(newnamep);
gotit:		
		/*
		 * Release the above reference to the infilevp, the reference 
		 * to the NAMEFS vnode, create a reference to the new vnode
		 * and return the new vnode to the user.
		 */
		VN_HOLD(NMTOV(newnamep));
		VN_RELE(*vpp);
		*vpp = NMTOV(newnamep);
	}
	return (0);
}

/*
 * Close a mounted file descriptor.
 * Remove any locks and apply the VOP_CLOSE operation to the vnode for
 * the file descriptor.
 */
STATIC int
nm_close(vp, flag, count, offset, crp)
	struct vnode *vp;
	int flag;
	int count;
	off_t offset;
	struct cred *crp;
{
	register struct namenode *nodep = VTONM(vp);
	register int error = 0;

	ASSERT (nodep->nm_filevp);
	cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
	error = VOP_CLOSE(nodep->nm_filevp, flag, count, offset, crp);
	if ((unsigned) count == 1) {
		(void) nm_fsync(vp, crp);
		VN_RELE(nodep->nm_filevp);
	}
	return (error);
}

/*
 * Read from a mounted file descriptor.
 */
STATIC int
nm_read(vp, uiop, ioflag, crp)
	struct vnode *vp;
	struct uio *uiop;
	int ioflag;
	struct cred *crp;
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_READ (VTONM(vp)->nm_filevp, uiop, ioflag, crp));
}

/*
 * Apply the VOP_WRITE operation on the file descriptor's vnode.
 */
STATIC int
nm_write(vp, uiop, ioflag, crp)
	struct vnode *vp;
	struct uio *uiop;
	int ioflag;
	struct cred *crp;
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_WRITE(VTONM(vp)->nm_filevp, uiop, ioflag, crp));
}

/*
 * Apply the VOP_IOCTL operation on the file descriptor's vnode.
 */
STATIC int
nm_ioctl(vp, cmd, arg, mode, cr, rvalp)
	register struct vnode *vp;
	int cmd;
	int arg;
	int mode;
	struct cred *cr;
	int *rvalp;
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_IOCTL(VTONM(vp)->nm_filevp, cmd, arg, mode, cr, rvalp));
}

/*
 * Return in vap the attributes that are stored in the namenode
 * structure. In addition, overwrite the va_mask field with 0;
 */
STATIC int
nm_getattr(vp, vap, flags, crp)
	struct vnode *vp;
	struct vattr *vap;
	int flags;
	struct cred *crp;
{
	register struct namenode *nodep = VTONM(vp);
	struct vattr va;
	register int error;

        /*
         * Must have MAC read access to the real vnode since some
         * of its information is returned as well.
         * Of course, this check need only be done if the namenode's
         * level does not match the real vnode's level.
         * Since the namenode's level can never change, the real
         * vnode's level must have changed.  This can only happen
         * for file system types which allow level changes while
         * the file is open (e.g. device special files).
         */
        if (vp->v_lid != nodep->nm_filevp->v_lid
        &&  (error = MAC_VACCESS(nodep->nm_filevp, VREAD, crp)))
                return(error);

	if (error = VOP_GETATTR(nodep->nm_filevp, &va, flags, crp))
		return (error);

	*vap = nodep->nm_vattr;
	vap->va_mask = 0;
	vap->va_size = va.va_size;
	return (0);
}

/*
 * Set the attributes of the namenode from the attributes in vap.
 */
/* ARGSUSED */
STATIC int
nm_setattr(vp, vap, flags, crp)
	struct vnode *vp;
	register struct vattr *vap;
	int flags;
	struct cred *crp;
{
	register struct namenode *nodep = VTONM(vp);
	register struct vattr *nmvap = &nodep->nm_vattr;
	register long mask = vap->va_mask;
	int error = 0;

	/*
	 * Cannot set these attributes.
	 */
	if (mask & (AT_NOSET|AT_SIZE))
		return (EINVAL);

	nm_rwlock(vp);

	/*
	 * Change ownership/group/time/access mode of mounted file
	 * descriptor.  Must be owner or privileged.
	 */
	if (crp->cr_uid != nmvap->va_uid && pm_denied(crp, P_OWNER)) {
		error = EPERM;
		goto out;
	}
	/*
	 * If request to change mode, copy new
	 * mode into existing attribute structure.
	 */
	if (mask & AT_MODE) {
		nmvap->va_mode = vap->va_mode & ~VSVTX;
		if (!groupmember(nmvap->va_gid, crp) && pm_denied(crp, P_OWNER))
			nmvap->va_mode &= ~VSGID;
	}
	/*
	 * If request was to change user or group, turn off suid and sgid
	 * bits.
	 * If the system was configured with the "rstchown" option, the 
	 * owner is not permitted to give away the file, and can change 
	 * the group id only to a group of which he or she is a member.
	 */
	if (mask & (AT_UID|AT_GID)) {
		int checksu = 0;

		if (rstchown) {
			if (((mask & AT_UID) && vap->va_uid != nmvap->va_uid)
			  || ((mask & AT_GID)
			    && !groupmember(vap->va_gid, crp)))
				checksu = 1;
		} else if (crp->cr_uid != nmvap->va_uid)
			checksu = 1;

		if (checksu && pm_denied(crp, P_OWNER)) {
			error = EPERM;
			goto out;
		}
		if ((nmvap->va_mode & (VSUID|VSGID)) && pm_denied(crp, P_OWNER))
			nmvap->va_mode &= ~(VSUID|VSGID);
		if (mask & AT_UID)
			nmvap->va_uid = vap->va_uid;
		if (mask & AT_GID)
			nmvap->va_gid = vap->va_gid;
	}
	/*
	 * If request is to modify times, make sure user has write 
	 * permissions on the file.
	 */
	if (mask & (AT_ATIME|AT_MTIME)) {
		if (!(nmvap->va_mode & VWRITE) && pm_denied(crp, P_DACWRITE)) {
			error = EACCES;
			goto out;
		}
		if (mask & AT_ATIME)
			nmvap->va_atime = vap->va_atime;
		if (mask & AT_MTIME) {
			nmvap->va_mtime = vap->va_mtime;
			nmvap->va_ctime = hrestime;
		}
	}
out:
	nm_rwunlock(vp);
	return (error);
}

/*
 * Check mode permission on the namenode.  If the namenode bits deny the
 * privilege is checked.  In addition an access check is performed
 * on the mounted file. Finally, if the file was opened without the
 * requested access at mount time, deny the access.
 */
/* ARGSUSED */
STATIC int
nm_access(vp, mode, flags, crp)
	struct vnode *vp;
	int mode;
	int flags;
	struct cred *crp;
{
	register struct namenode *nodep = VTONM(vp);
	register mode_t denied_mode;
	int error, fmode;

	if ((denied_mode = nm_daccess(nodep, mode, crp)) != 0) {
                if ((denied_mode & (VREAD | VEXEC)) 
				&& pm_denied(crp, P_DACREAD))
                        return (EACCES);
                if ((denied_mode & VWRITE) && pm_denied(crp, P_DACWRITE))
                        return (EACCES);
        }

	if (error = VOP_ACCESS(nodep->nm_filevp, mode, flags, crp))
		return (error);
	/*
	 * Last stand.  Regardless of the requestor's credentials, don't
	 * grant a permission that wasn't granted at the time the mounted
	 * file was originally opened.
	 */
	fmode = nodep->nm_filep->f_flag;
	if (((mode & VWRITE) && (fmode & FWRITE) == 0)
	  || ((mode & VREAD) && (fmode & FREAD) == 0))
		return (EACCES);
	return (0);
}

/*
 * Do discretionary access check on the name node.  This includes
 * checking the mode bits, and any ACL entries.
 * Any denied mode returned must be shifted to the leftmost octal permissions
 * digit, so that nm_access (above) will be able to do the correct
 * privilege check based on VREAD, VEXEC, or VWRITE.
 */
STATIC mode_t
nm_daccess(nodep, mode, crp)
        struct namenode *nodep;
        mode_t mode;
        struct cred *crp;
{
        struct acl *aclp;
        long    idmatch = 0;
        mode_t  workmode = 0;
        mode_t  reqmode;
        mode_t  basemode;
	int	i;

        ASSERT(nodep->nm_aclcnt >= 0);

        /* check if effective uid == owner of name node */
        if (crp->cr_uid == nodep->nm_vattr.va_uid) {
                if ((workmode = (nodep->nm_vattr.va_mode & mode)) == mode)
                        return (0);
                else
                        return (mode & ~workmode);
        }
        mode >>= 3;
        /*
         * If there's no ACL, check only the group and
         * other permissions.
         */
        if (nodep->nm_aclcnt == 0) {
                if (groupmember(nodep->nm_vattr.va_gid, crp)) {
                        if ((workmode = (nodep->nm_vattr.va_mode & mode)) 
				== mode)
                                return (0);
                        else
                                return ((mode & ~workmode) << 3);
                } else
                        goto other_ret;
        }

        /* set up requested & base permissions */
        reqmode = (mode >>= 3) & 07;
        basemode = (nodep->nm_vattr.va_mode >> 3) & 07;
        for (i = nodep->nm_aclcnt, aclp = nodep->nm_aclp; i > 0; i--, aclp++) {
                switch (aclp->a_type) {
                case USER:
                        if (crp->cr_uid == aclp->a_id) {
                                if ((workmode = ((aclp->a_perm & reqmode) & 
					basemode)) == reqmode)
                                        /*
                                         * Matching USER found;
                                         * access granted.
                                         */
                                        return (0);
                                else
                                        /*
                                         * Matching USER found;
                                         * access denied.
                                         */
                                        return ((reqmode & ~workmode) << 6);
                        }
                        break;

                case GROUP_OBJ:
                        if (groupmember(nodep->nm_vattr.va_gid, crp)) {
                                if ((workmode |= (aclp->a_perm & reqmode)) ==
                                        reqmode)
                                        goto match;
                                else
                                        idmatch++;
                        }
                        break;

                case GROUP:
                        if (groupmember(aclp->a_id, crp)) {
                                if ((workmode |= (aclp->a_perm & reqmode)) ==
                                        reqmode)
                                        goto match;
                                else
                                        idmatch++;
                        }
                        break;
                }       /* end switch statement */
        }       /* end for statement */
        if (idmatch)
                /*
                 * Matching GROUP or GROUP_OBJ entries
                 * were found, but did not grant the access.
                 */
                return ((reqmode & ~workmode) << 6);

other_ret:
        /*
         * Not the file owner, and either
         * no ACL, or no match in ACL.
         * Now, check the file other permissions.
         */
        mode >>= 3;
        if ((workmode = (nodep->nm_vattr.va_mode & mode)) == mode) 
		return (0);
        else
                return ((mode & ~workmode) << 6);

match:
        /*
         * Access granted by ACL entry or entries.
         * File Group Class bits mask access, so
         * determine whether matched entries
         * should really grant access.
         */
        if ((workmode & basemode) == reqmode)
                return (0);
        else
                return ((reqmode & ~(workmode & basemode)) << 6);
}

/*
 * Dummy op so that creats and opens with O_CREAT
 * of mounted streams will work.
 */
/*ARGSUSED*/
STATIC int
nm_create(dvp, name, vap, excl, mode, vpp, cr)
	struct vnode *dvp;
	char *name;
	struct vattr *vap;
	enum vcexcl excl;
	int mode;
	struct vnode **vpp;
	struct cred *cr;
{
	register int error = 0;

	if (*name == '\0') {
		/*
		 * Null component name refers to the root.
		 */
		if ((error = nm_access(dvp, mode, 0, cr)) == 0) {
			VN_HOLD(dvp);
			*vpp = dvp;
		}
	} else {
		error = ENOSYS;
	}
	return (error);
}

/*
 * Links are not allowed on mounted file descriptors.
 */
/*ARGSUSED*/
STATIC int
nm_link(tdvp, vp, tnm, crp)
	register struct vnode *tdvp;
	struct vnode *vp;
	char *tnm;
	struct cred *crp;

{
	return (EXDEV);
}

/*
 * Apply the VOP_FSYNC operation on the file descriptor's vnode.
 */
STATIC int
nm_fsync(vp, crp)
	struct vnode *vp;
	struct cred *crp;
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_FSYNC (VTONM(vp)->nm_filevp, crp));
}

/*
 * Inactivate a vnode/namenode by...
 * clearing its unique node id, removing it from the hash list
 * and freeing the memory allocated for it.
 */
/* ARGSUSED */
STATIC void
nm_inactive(vp, crp)
	struct vnode *vp;
	struct cred *crp;
{
	register struct namenode *nodep = VTONM(vp);

	/*
	 * Note:  Maintain this ordering since closef and VN_RELE may sleep.
	 */
	nmclearid(nodep);
	nameremove(nodep);
	closef(nodep->nm_filep);
	VN_RELE(nodep->nm_filevp);
	kmem_free((caddr_t) nodep, sizeof(struct namenode));
}

/*
 * Apply the VOP_FID operation on the file descriptor's vnode.
 */
STATIC int
nm_fid(vp, fidnodep)
	struct vnode *vp;
	struct fid **fidnodep;
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_FID(VTONM(vp)->nm_filevp, fidnodep));
}

/*
 * Lock the namenode associated with vp.
 */
STATIC void
nm_rwlock(vp)
	struct vnode *vp;
{
	register struct namenode *nodep = VTONM(vp);

	VOP_RWLOCK(nodep->nm_filevp);
}

/*
 * Unlock the namenode associated with vp.
 */
STATIC void
nm_rwunlock(vp)
	struct vnode *vp;
{
	register struct namenode *nodep = VTONM(vp);

	VOP_RWUNLOCK(nodep->nm_filevp);
}

/*
 * Apply the VOP_SEEK operation on the file descriptor's vnode.
 */
STATIC int
nm_seek(vp, ooff, noffp)
	struct vnode *vp;
	off_t ooff;
	off_t *noffp;
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_SEEK(VTONM(vp)->nm_filevp, ooff, noffp));
}

/*
 * Return the vnode representing the file descriptor in vpp.
 */
STATIC int
nm_realvp(vp, vpp)
	register struct vnode *vp;
	register struct vnode **vpp;
{
	register struct vnode *fvp = VTONM(vp)->nm_filevp;
	struct vnode *rvp;

	ASSERT(fvp);
	vp = fvp;
	if (VOP_REALVP(vp, &rvp) == 0)
		vp = rvp;
	*vpp = vp;
	return (0);
}

/*
 * Apply VOP_POLL to the vnode representing the mounted file descriptor.
 */
STATIC int
nm_poll(vp, events, anyyet, reventsp, phpp)
	vnode_t *vp;
	register short events;
	int anyyet;
	register short *reventsp;
	struct pollhead **phpp;
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_POLL(VTONM(vp)->nm_filevp, events, anyyet, reventsp, phpp));
}

/*
 * Apply VOP_GETACL on the namenode.
 *
 * Note that default ACLs are not supported for NAMEFS.
 */
/* ARGSUSED */
STATIC int
nm_getacl(vp, nentries, dentriesp, aclbufp, cr, rvalp)
        register struct vnode *vp;
        register long nentries;
        register long *dentriesp;
        register struct acl *aclbufp;
        struct cred *cr;
        int *rvalp;
{
        register struct namenode *nodep = VTONM(vp);
        struct acl base_user = {USER_OBJ, (uid_t) 0, (ushort) 0};
        struct acl base_group = {GROUP_OBJ, (uid_t) 0, (ushort) 0};
        struct acl base_class = {CLASS_OBJ, (uid_t) 0, (ushort) 0};
        struct acl base_other = {OTHER_OBJ, (uid_t) 0, (ushort) 0};
        struct acl *tgt_aclp;
	int rval;

	rval = nodep->nm_aclcnt ? nodep->nm_aclcnt + NACLBASE - 1 : NACLBASE;
	if (rval > nentries)
		return (ENOSPC);

        /*
         * Get USER_OBJ, CLASS_OBJ, & OTHER_OBJ entry permissions from file
         * owner class, file group class, and file other permission bits.
         */
        base_user.a_perm = (nodep->nm_vattr.va_mode >> 6) & 07;
        base_class.a_perm = (nodep->nm_vattr.va_mode >> 3) & 07;
        base_other.a_perm = nodep->nm_vattr.va_mode & 07;

	tgt_aclp = aclbufp;

        /* copy USER_OBJ entry into caller's buffer */
        bcopy((caddr_t)&base_user, (caddr_t)tgt_aclp, sizeof(struct acl));
        tgt_aclp++;

        if (nodep->nm_aclcnt == 0) {
        	/* copy GROUP_OBJ entry into caller's buffer */
                base_group.a_perm = base_class.a_perm;
                bcopy((caddr_t)&base_group, (caddr_t)tgt_aclp,
                        sizeof(struct acl));
                tgt_aclp++;
	} else {
        	/* copy USER, GROUP_OBJ, & GROUP entries into caller's buffer */
        	bcopy((caddr_t)nodep->nm_aclp, (caddr_t)tgt_aclp,
                        nodep->nm_aclcnt * sizeof(struct acl));
		tgt_aclp += nodep->nm_aclcnt;
	}

        /* copy CLASS_OBJ & OTHER_OBJ entries  into caller's buffer */
        bcopy((caddr_t)&base_class, (caddr_t)tgt_aclp, sizeof(struct acl));
        tgt_aclp++;
        bcopy((caddr_t)&base_other, (caddr_t)tgt_aclp, sizeof(struct acl));
	*rvalp = rval;
	*dentriesp = 0;	/* no defaults */
	return (0);
}

/*
 * Apply VOP_GETACLCNT on the namenode.
 */
/* ARGSUSED */
STATIC int
nm_getaclcnt(vp, cr, rvalp)
        register struct vnode   *vp;
        struct  cred            *cr;
        int                     *rvalp;
{
        register struct namenode   *nodep = VTONM(vp);

	/*
	 * If there are no stored ACL entries, return the number
	 * of base entries (NACLBASE).  Otherwise, return the stored
	 * ACL entries plus (NACLBASE-1).  In the latter case,
	 * GROUP_OBJ is part of the stored entries.
	 */
	*rvalp = nodep->nm_aclcnt ? nodep->nm_aclcnt + NACLBASE - 1 : NACLBASE;
        return 0;
}

/*
 * nm_setacl - Set a mounted file descriptor's ACL
 *
 * Input:  	Pointer to the file's namenode
 *		Pointer to user's ACL entries
 *		Number of ACL entries to save
 *		Pointer to number of default ACL entries
 *
 * Note that NAMEFS does not support the mounting of a
 * directory fd; therefore NAMEFS does not support default
 * ACLs.
 */

STATIC int
nm_setacl(vp, nentries, dentries, aclbufp, cr)
        register struct vnode   *vp;
        register long           nentries;
        register long           dentries;
        register struct acl     *aclbufp;
        struct  cred            *cr;
{
        register struct namenode *nodep = VTONM(vp);

        if (cr->cr_uid != nodep->nm_vattr.va_uid && pm_denied(cr, P_OWNER))
                return (EPERM);

	ASSERT(dentries == 0);

        /* go store the entries on the file */
        return(nm_aclstore(nodep, aclbufp, nentries));
}

/*
 * Store ACL entries on namenode.
 */
int
nm_aclstore(nodep, aclbufp, nentries)
        register struct namenode *nodep;
        register struct acl     *aclbufp;
        register long           nentries;
{
        register struct acl     *src_aclp;
        struct acl              *tmpaclp = NULL;
	long			entries = 0;
        mode_t                  mode;

	ASSERT(nentries >= NACLBASE);

        mode = (aclbufp->a_perm & 07) << 6;     /* save owner perms */
        src_aclp = aclbufp + nentries - 1;      /* point at OTHER_OBJ */
        mode |= src_aclp->a_perm & 07;          /* save other perms */
        src_aclp--;                             /* point at CLASS_OBJ */
        mode |= (src_aclp->a_perm & 07) << 3;   /* save file group class perms */


	if (nentries > NACLBASE) {
		entries = nentries - (NACLBASE - 1);
        	tmpaclp = (struct acl *)
			kmem_alloc(entries * sizeof(struct acl), KM_NOSLEEP);
		if (tmpaclp == (struct acl *)NULL)
			return (ENOSPC);
                src_aclp = aclbufp + 1; /* point past USER_OBJ */
        	bcopy((caddr_t)src_aclp, (caddr_t)tmpaclp,
                                        entries * sizeof(struct acl));
	}

	if (nodep->nm_aclcnt)
		kmem_free((caddr_t)nodep->nm_aclp,
			nodep->nm_aclcnt * sizeof(struct acl));

	nodep->nm_aclcnt = entries;
	nodep->nm_aclp = tmpaclp;
        nodep->nm_vattr.va_mode &= ~(ushort)PERMMASK;
        nodep->nm_vattr.va_mode |= mode;
        return (0);
}
