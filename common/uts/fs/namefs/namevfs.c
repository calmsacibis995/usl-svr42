/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/namefs/namevfs.c	1.20.3.3"
#ident	"$Header: $"

/*
 * This file supports the vfs operations for the NAMEFS file system.
 */

#include <acc/dac/acl.h>
#include <acc/mac/cca.h>
#include <acc/mac/covert.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/namefs/namenode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/strsubr.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * The next set of lines define the bit map for obtaining 
 * unique node ids. The value chosen for the maximum node id
 * is arbitrary and should be altered to better fit the system
 * on which this file system is defined.  The maximum value can
 * not exceed the maximum value for a long, since the node id
 * must fit into a long.  (In fact, currently the routine
 * that assigns node ids restricts it to fit in a u_short.)
 * The maximum number of mounted file descriptors at a given
 * time will be limited to NMMAXID-1 (node id 0 isn't used).
 *
 * nmap    --> bitmap with one bit per node id
 * testid  --> is this node already in use?
 * setid   --> mark this node id as used
 * clearid --> mark this node id as unused
 */

#define NMMAXID		8191
#define testid(i)	((nmmap[(i)/NBBY] & (1 << ((i)%NBBY))))
#define setid(i)	((nmmap[(i)/NBBY] |= (1 << ((i)%NBBY))), (nmids++))
#define clearid(i)	((nmmap[(i)/NBBY] &= ~(1 << ((i)%NBBY))), (nmids--))

STATIC char nmmap[NMMAXID/NBBY + 1];
STATIC ushort nmids;

/* control structure for covert channel limiter */
STATIC ccevent_t cc_re_namefs = { CC_RE_NAMEFS, CCBITS_RE_NAMEFS };

/*
 * Define the routines in this file.
 */
STATIC int	nm_mount(), nm_unmount(), nm_root();
STATIC int	nm_sync(),  nm_statvfs();
struct	namenode *namefind();
void	nameinsert(), nameremove();
void	nmclearid();
int	nm_unmountall();
u_short	nmgetid();

/*
 * Define external routines and variables.
 */
extern int nm_aclstore();
extern struct vnodeops nm_vnodeops;

/*
 * Define data structures.
 */
STATIC dev_t		namedev;
STATIC struct namenode	*namealloc;
STATIC struct vfs	*namevfsp;

/*
 * Define the vfs operations vector.
 */
STATIC struct vfsops nmvfsops = {
	nm_mount,
        nm_unmount,
	nm_root,
	nm_statvfs,
	nm_sync,
	fs_nosys,    /* vget */
	fs_nosys,    /* mountroot */
	fs_nosys,    /* not used */
	fs_nosys,    /* setceiling */
	fs_nosys,    /* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

STATIC struct vfsops dummyvfsops = {
	fs_nosys,    /* mount */
        fs_nosys,    /* unmount */
	fs_nosys,    /* root */
	nm_statvfs,
	nm_sync,
	fs_nosys,    /* vget */
	fs_nosys,    /* mountroot */
	fs_nosys,    /* not used */
	fs_nosys,    /* setceiling */
	fs_nosys,    /* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

/*
 * File system initialization routine. Save the file system
 * type, establish a file system device number and initialize 
 * namealloc.
 */
int
nameinit(vswp, fstype)
	register struct vfssw *vswp;
	int fstype;
{
	register int dev;

	vswp->vsw_vfsops = &nmvfsops;
	if ((dev = getudev()) == -1) {
		cmn_err(CE_WARN, "nameinit: can't get unique device");
		dev = 0;
	}
	namedev = makedevice(dev, 0);
	namealloc = NULL;
	namevfsp = (struct vfs *)kmem_zalloc(sizeof(struct vfs), KM_SLEEP);
	namevfsp->vfs_next = NULL;
	namevfsp->vfs_op = &dummyvfsops;
	namevfsp->vfs_vnodecovered = NULL;
	namevfsp->vfs_flag = 0;
	namevfsp->vfs_bsize = NMBSIZE;
	namevfsp->vfs_fstype = fstype;
	namevfsp->vfs_fsid.val[0] = namedev;
	namevfsp->vfs_fsid.val[1] = fstype;
	namevfsp->vfs_data = NULL;
	namevfsp->vfs_dev = namedev;
	namevfsp->vfs_bcount = 0;
	return (0);
}

/*
 * Mount a file descriptor onto the node in the file system.
 * Create a new vnode, update the attributes with info from the 
 * file descriptor and the mount point.  The mask, mode, uid, gid,
 * atime, mtime and ctime are taken from the mountpt.  Link count is
 * set to one, the file system id is namedev and nodeid is unique 
 * for each mounted object.  Other attributes are taken from mount
 * point.
 * Make sure process is owner with write permissions on mount point or
 * is privileged.
 * Hash the new vnode and return 0.
 * Upon entry to this routine, the file descriptor is in the 
 * fd field of a struct namefd.  Copy that structure from user
 * space and retrieve the file descriptor.
 */
STATIC int
nm_mount(vfsp, mvp, uap, crp)
	register struct vfs *vfsp;	/* vfs for this mount */
	struct vnode *mvp;        	/* vnode of mount point */
	struct mounta *uap;		/* user arguments */
	struct cred *crp;		/* user credentials */
{
	struct namefd namefdp;
	struct vnode *filevp;		/* file descriptor vnode */
	struct file *fp;
	struct vnode *newvp;		/* vnode representing this mount */
	struct namenode *nodep;		/* namenode for this mount */
	struct vattr filevattr;		/* attributes of file dec.  */
	struct vattr *vattrp;		/* attributes of this mount */
        struct acl *tmpaclp;            /* temporary ACL buffer */
        int entries;                    /* number of ACL entries for node */
        int tmp_entries;                /* temp for return from VOP_GETACL */
        int dentries;                   /* number of default ACL entries */
	int aentries;			/* number of non-default entries */
	u_short nodeid;
	int error = 0;

	/*
	 * Get the file descriptor from user space.
	 * Make sure the file descriptor is valid and has an
	 * associated file pointer.
	 * If so, extract the vnode from the file pointer.
	 */
	if (uap->datalen != sizeof(struct namefd))
		return (EINVAL);
	if (copyin(uap->dataptr, (caddr_t) &namefdp, uap->datalen))
		return (EFAULT);
	if (error = getf(namefdp.fd, &fp))
		return (error);
	/*
	 * If the mount point already has something mounted
	 * on it, disallow this mount.  (This restriction may 
	 * be removed in a later release).
	 */
	if (mvp->v_flag & VROOT)
		return (EBUSY);

	filevp = fp->f_vnode;
	if (filevp->v_type == VDIR)
		return (EINVAL);

	/*
         * Make sure the file descriptor is not the root of some
         * file system.
         */
        if (filevp->v_flag & VROOT)
                return (EBUSY);

	/* Must have MAC write access to mount point */
	if (MAC_VACCESS(mvp, VWRITE, crp))
		return EACCES;
        MAC_ASSERT (mvp, MAC_SAME);
 
        /*
         * The mount point's level must be the same as the level of
         * the file descriptor.  This is a strict equality and no
         * privilege can be used as an override.
         */
        if (MAC_ACCESS(MACEQUAL, mvp->v_lid, filevp->v_lid))
                return EACCES;
        MAC_ASSUME (filevp, MAC_SAME);
	
        /*
         * Create a reference and allocate a namenode to represent
         * this mount request.
         */
	nodep = (struct namenode *) kmem_zalloc(sizeof(struct namenode), 
		KM_SLEEP);

	vattrp = &nodep->nm_vattr;
	if (error = VOP_GETATTR(mvp, vattrp, 0, crp))
		goto out;

	/*
	 * Populate namenode with ACL from mount point.
	 * Only do work if there are stored ACL entries on the moint point.
	 * Allocate storage to hold mount point ACL entries and get the ACL.
	 * Store non-default ACL entries on namenode.
	 * Free temporary storage.
	 */
retry:
        error = VOP_GETACLCNT(mvp, crp, &entries);
        if (error == 0 && entries > NACLBASE) {
                tmpaclp = (struct acl *)kmem_alloc(entries * sizeof(struct acl),                                                KM_SLEEP);
                error = VOP_GETACL(mvp, entries, &dentries, tmpaclp, crp,
                                	&tmp_entries);
		if (error == ENOSPC) {
			/*
			 * In between the VOP_GETACLCNT() and VOP_GETACL()
			 * calls, the ACL was modified and is now larger.
			 * Try again.
			 */
                	kmem_free((caddr_t)tmpaclp, entries * sizeof(struct acl));
			goto retry;
		}
		aentries = tmp_entries - dentries;
                if (error == 0 && aentries)
                        error = nm_aclstore(nodep, tmpaclp, aentries);
                kmem_free((caddr_t)tmpaclp, entries * sizeof(struct acl));
        }
        if (error != 0 && error != ENOSYS)
                goto out;

	if (error = VOP_GETATTR(filevp, &filevattr, 0, crp))
		goto out;
	/*
	 * Make sure the process is the owner of the mount point (or
	 * is privileged) and has write permission.
	 */
	if (vattrp->va_uid != crp->cr_uid && pm_denied(crp, P_OWNER)) {
		error = EPERM;
		goto out;
	}
	if (error = VOP_ACCESS(mvp, VWRITE, 0, crp))
		goto out;

	/*
	 * If the file descriptor has file/record locking, don't
	 * allow the mount to succeed.
	 */
	if (filevp->v_filocks) {
		error = EACCES;
		goto out;
	}
	/*
	 * Establish a unique node id to represent the mount.
	 * If can't, return error.
	 */
	if ((nodeid = nmgetid()) == 0) {
		error = ENOMEM;
		goto out;
	}

	/*
	 * Initialize the namenode.
	 */
	nodep->nm_mountpt = mvp;
	nodep->nm_filep = fp;
	nodep->nm_filevp = filevp;
	/*
	 * Hold filevp and fp to be sure they are ours until nm_inactive
	 * or nm_unmount.
	 */
	VN_HOLD(filevp);
	fp->f_count++;
	if (filevp->v_stream)
		filevp->v_stream->sd_flag |= STRMOUNT;

	/*
	 * The attributes for the mounted file descriptor were initialized 
	 * above by applying VOP_GETATTR to the mount point.  Some of
	 * the fields of the attributes structure will be overwritten 
	 * by the attributes from the file descriptor.
	 */
	vattrp->va_type    = filevattr.va_type;
	vattrp->va_fsid    = namedev;
	vattrp->va_nodeid  = nodeid;
	vattrp->va_nlink   = 1;
	vattrp->va_size    = filevattr.va_size;
	vattrp->va_rdev    = filevattr.va_rdev;
	vattrp->va_blksize = filevattr.va_blksize;
	vattrp->va_nblocks = filevattr.va_nblocks;
	vattrp->va_vcode   = filevattr.va_vcode;

	/*
	 * Initialize the new vnode structure for the mounted file
	 * descriptor.
	 */
	newvp = NMTOV(nodep);
	newvp->v_flag = filevp->v_flag | VROOT | VNOMAP | VNOSWAP;
	newvp->v_count = 1;

	/*
         * The level of the namenode is assigned the level of the
         * file descriptor node.
         */
        newvp->v_lid = filevp->v_lid;
	newvp->v_macflag |= VMAC_SUPPORT;
	if(filevp->v_macflag & VMAC_DOPEN)
		newvp->v_macflag |= VMAC_DOPEN;

	newvp->v_op = &nm_vnodeops;
	newvp->v_vfsp = vfsp;
	newvp->v_stream = filevp->v_stream;
	newvp->v_type = filevp->v_type;
	newvp->v_rdev = filevp->v_rdev;
	newvp->v_data = (caddr_t) nodep;

	/*
	 * Initialize the vfs structure.
	 */
	vfsp->vfs_bsize = NMBSIZE;
	vfsp->vfs_fsid.val[0] = namedev;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
	vfsp->vfs_data = (caddr_t) nodep;
	vfsp->vfs_dev = namedev;
	vfsp->vfs_bcount = 0;

	nameinsert(nodep);
	return (0);
out:
	if (nodep->nm_aclcnt)
		kmem_free((caddr_t)nodep->nm_aclp,
			nodep->nm_aclcnt * sizeof(struct acl));

	kmem_free((caddr_t)nodep, sizeof(struct namenode));

	return (error);
}

/*
 * Unmount a file descriptor from a node in the file system.
 * If the process is not the owner of the mount point and is not privileged,
 * the request is denied.
 * Otherwise, remove the namenode from the hash list. 
 * If the mounted file descriptor was that of a stream and this
 * was the last mount of the stream, turn off the STRMOUNT flag.
 * If the rootvp is referenced other than through the mount,
 * nm_inactive will clean up.
 */
STATIC int
nm_unmount(vfsp, crp)
	struct vfs *vfsp;
	struct cred *crp;
{
	struct namenode *nodep = (struct namenode *) vfsp->vfs_data;
	struct vnode *vp;
	struct vnode *fvp;
	struct vattr mpattr;
	int error;

	vp = NMTOV(nodep);
	fvp = nodep->nm_filevp;

	/* Must have MAC write access to name node */
	if (MAC_VACCESS(vp, VWRITE, crp))
		return EACCES;

	mpattr.va_mask = AT_UID;
	ASSERT(vfsp->vfs_vnodecovered != NULLVP);
	if ((error = VOP_GETATTR(vfsp->vfs_vnodecovered, &mpattr, 0, crp)) != 0)
		return error;
	if (mpattr.va_uid != crp->cr_uid && pm_denied(crp, P_OWNER))
		return (EPERM);

        /* delete any ACL entries */
        if (nodep->nm_aclcnt > 0)
                kmem_free((caddr_t)nodep->nm_aclp,
                        nodep->nm_aclcnt * sizeof(struct acl));

	nameremove(nodep);
	if (vp->v_count-- == 1) {
		(void)closef(nodep->nm_filep);
		VN_RELE(fvp);
		nmclearid(nodep);
		kmem_free((caddr_t) nodep, sizeof(struct namenode));
	} else {
		vp->v_flag &= ~VROOT;
		vp->v_vfsp = namevfsp;
	}
	if (namefind(fvp, NULLVP) == NULL && fvp->v_stream)
		fvp->v_stream->sd_flag &= ~STRMOUNT;
	return (0);
}

/*
 * Create a reference to the root of a mounted file descriptor.
 * This routine is called from lookupname() in the event a path
 * is being searched that has a mounted file descriptor in it.
 */
STATIC int
nm_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	struct namenode *nodep = (struct namenode *) vfsp->vfs_data;
	struct vnode *vp = NMTOV(nodep);

	VN_HOLD(vp);
	*vpp = vp;
	return (0);
}

/*
 * Return in sp the status of this file system.
 */
STATIC int
nm_statvfs(vfsp, sp)
	struct vfs *vfsp;
	register struct statvfs *sp;
{
	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize	= NMBSIZE;
	sp->f_frsize	= NMFSIZE;
	sp->f_blocks	= 0;
	sp->f_bfree	= 0;
	sp->f_bavail	= 0;
	sp->f_files	= 0;
	sp->f_ffree	= 0;
	sp->f_favail	= 0;
	sp->f_fsid	= vfsp->vfs_dev;	
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag	= vf_to_stf(vfsp->vfs_flag);	
	sp->f_namemax	= 0;
	return (0);
}

/*
 * Since this file system has no disk blocks of its own, apply
 * the VOP_FSYNC operation on the mounted file descriptor.
 */
STATIC int
nm_sync(vfsp, flag, crp)
	struct vfs *vfsp;
	short flag;
	struct cred *crp;
{
	struct namenode *nodep;

	if (vfsp == NULL)
		return (0);

	nodep = (struct namenode *) vfsp->vfs_data;
	if (flag & SYNC_CLOSE)
		return (nm_unmountall(nodep->nm_filevp, crp));

	return (VOP_FSYNC(nodep->nm_filevp, crp));
}

/*
 * Insert a namenode into the namealloc hash list.
 * namealloc is a doubly linked list that contains namenode
 * as links. Each link has a unique namenode with a unique
 * nm_mountvp field. The nm_filevp field of the namenode need not
 * be unique, since a file descriptor may be mounted to multiple
 * nodes at the same time.
 * This routine inserts a namenode link onto the front of the
 * linked list.
 */
void
nameinsert(nodep)
	struct namenode *nodep;
{
	nodep->nm_backp = NULL;
	nodep->nm_nextp = namealloc;
	namealloc = nodep;
	if (nodep->nm_nextp)
		nodep->nm_nextp->nm_backp = nodep;
}

/*
 * Search the doubly linked list, namealloc, for a namenode that
 * has a nm_filevp field of vp and a nm_mountpt of mnt.
 * If the namenode/link is found, return the address. Otherwise,
 * return NULL.
 * If mnt is NULL, return the first link with a nm_filevp of vp.
 */
struct namenode *
namefind(vp, mnt)
	struct vnode *vp;
	struct vnode *mnt;
{
	register struct namenode *tnode;

	for (tnode = namealloc; tnode; tnode = tnode->nm_nextp)
		if (tnode->nm_filevp == vp && 
			(!mnt || (mnt && tnode->nm_mountpt == mnt)))
				break;
	return (tnode);
}

/*
 * Remove a namenode from the hash table.
 * If nodep is the only node on the list, set namealloc to NULL.
 */
void
nameremove(nodep)
	struct namenode *nodep;
{
	register struct namenode *tnode;

	for (tnode = namealloc; tnode; tnode = tnode->nm_nextp)
		if (tnode == nodep) {
			if (nodep == namealloc)      /* delete first link */
				namealloc = nodep->nm_nextp;
			if (tnode->nm_nextp)
				tnode->nm_nextp->nm_backp = tnode->nm_backp;
			if (tnode->nm_backp)
				tnode->nm_backp->nm_nextp = tnode->nm_nextp;
			break;
		}
}

/*
 * Clear the bit in the bit map corresponding to the
 * nodeid in the namenode.
 */
void
nmclearid(nodep)
struct namenode *nodep;
{
	/*
	 * Safeguard against decrementing nmids when bit has
	 * already been cleared.
	 */
	if (testid(nodep->nm_vattr.va_nodeid))
		clearid(nodep->nm_vattr.va_nodeid);
}

/*
 * Attempt to establish a unique node id. Start searching
 * the bit map where the previous search stopped. If a
 * free bit is located, set the bit and keep track of
 * it because it will become the new node id.
 */
u_short
nmgetid()
{
	register u_short i;
	register u_short j;
	static u_short prev = 0;

	/*
	 * If we're concerned about covert channels, start the id search
	 * at a random place rather than where the previous search stopped.
	 * If the random() routine is stubbed out, it will return 0,
	 * in which case we want to revert to the sequential method.
	 */
	if (!mac_installed || (i = random((u_long)NMMAXID)) == 0)
		i = prev;

	for (j = NMMAXID; j--; ) {
		if (i++ >= (u_short)NMMAXID)
			i = 1;

		if (!testid(i)) {
			setid(i);
			prev = i;
			if (nmids > (u_short)(NMMAXID - RANDMINFREE))
				cc_limiter(&cc_re_namefs, u.u_cred);
			return i;
		}
	}

	cc_limiter(&cc_re_namefs, u.u_cred);
	cmn_err(CE_WARN, "nmgetid: could not establish a unique node id\n");
	return 0;
}

/*
 * Force the unmouting of a file descriptor from ALL of the nodes
 * that it was mounted to.
 * At the present time, the only usage for this routine is in the
 * event one end of a pipe was mounted. At the time the unmounted
 * end gets closed down, the mounted end is forced to be unmounted.
 *
 * This routine searches the namealloc hash list for all namenodes 
 * that have a nm_filevp field equal to vp. Each time one is found,
 * the dounmount() routine is called. This causes the nm_unmount()
 * routine to be called and thus, the file descriptor is unmounted 
 * from the node.
 *
 * At the start of this routine, the reference count for vp is
 * incremented to protect the vnode from being released in the 
 * event the mount was the only thing keeping the vnode active.
 * If that is the case, the VOP_CLOSE operation is applied to
 * the vnode, prior to it being released.
 */
int
nm_unmountall(vp, crp)
	struct vnode *vp;
	struct cred *crp;
{
	register struct vfs *vfsp;
	register struct namenode *nodep;
	register struct namenode *nextp;
	register error = 0;
	int realerr = 0;

	/*
	 * For each namenode that is associated with the file:
	 * If the v_vfsp field is not namevfsp, dounmount it.  Otherwise,
	 * it was created in nm_open() and will be released in time.
	 */
	nodep = namealloc;
	while (nodep) {
		nextp = nodep->nm_nextp;
		if (nodep->nm_filevp == vp && 
		  (vfsp = NMTOV(nodep)->v_vfsp) != NULL && vfsp != namevfsp) {
			if ((error = dounmount(vfsp, crp)) != 0)
				realerr = error;
		}
		nodep = nextp;
	}
	return (realerr);
}
