/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/nfs/nfs_export.c	1.9.2.2"
#ident	"$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

#define NFSSERVER

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <util/param.h>
#include <svc/time.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <net/transport/socket.h>
#include <svc/errno.h>
#include <io/uio.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <fs/file.h>
#include <net/transport/tiuser.h>
#include <mem/kmem.h>
#include <fs/pathname.h>
#include <util/debug.h>
#include <net/tcpip/in.h>
#include <net/rpc/types.h>
#include <net/rpc/auth.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/export.h>

#ifdef __STDC__
STATIC int	unexport(fsid_t *, struct fid *);
STATIC int	findexivp(struct exportinfo **, struct vnode *, struct vnode *);
STATIC int	loadaddrs(struct exaddrlist *);
STATIC void	freeaddrs(struct exaddrlist *);
STATIC int	loadrootnames(struct desexport *);
STATIC void	freenames(struct desexport *);
STATIC void	exportfree(struct exportinfo *);
#else /* __STDC__ */
STATIC int	unexport();
STATIC int	findexivp();
STATIC int	loadaddrs();
STATIC void	freeaddrs();
STATIC int	loadrootnames();
STATIC void	freenames();
STATIC void	exportfree();
#endif /* __STDC__ */

#define eqfsid(fsid1, fsid2)	\
	(bcmp((char *)fsid1, (char *)fsid2, (int)sizeof(fsid_t)) == 0)

#define eqfid(fid1, fid2) \
	((fid1)->fid_len == (fid2)->fid_len && \
	bcmp((char *)(fid1)->fid_data, (char *)(fid2)->fid_data,  \
	(int)(fid1)->fid_len) == 0)

#define exportmatch(exi, fsid, fid) \
	(eqfsid(&(exi)->exi_fsid, fsid) && eqfid((exi)->exi_fid, fid))

struct exportinfo *exported;	/* the list of exported filesystems */
struct exportfs_args {
	char	*dname;
	struct export	*uex;
};


/*
 * Exportfs system call
 */
exportfs(uap)
	register struct exportfs_args *uap;
{
	struct vnode *vp;
	struct export *kex;
	struct exportinfo **tail;
	struct exportinfo *exi;
	struct exportinfo *tmp;
	struct fid *fid;
	struct vfs *vfs;
	int mounted_ro;
	int error;

	if (pm_denied(u.u_cred, P_FILESYS))
		return (EPERM);

	/*
	 * Get the vfs id
	 */
	error = lookupname(uap->dname, UIO_USERSPACE, FOLLOW, 
		(struct vnode **) NULL, &vp);
	if (error)
		return (error);	
	error = VOP_FID(vp, &fid);
	vfs = vp->v_vfsp;
	mounted_ro = vp->v_vfsp->vfs_flag & VFS_RDONLY;
	VN_RELE(vp);
	if (error)
		return (error);	

	if (uap->uex == NULL) {
		error = unexport(&vfs->vfs_fsid, fid);
		freefid(fid);
		return (error);
	}
	exi = (struct exportinfo *) kmem_zalloc(sizeof(struct exportinfo),
						KM_SLEEP);
	exi->exi_fsid  = vfs->vfs_fsid;
	exi->exi_fid = fid;
	kex = &exi->exi_export;

	/*
	 * Load in everything, and do sanity checking
	 */	
	if (copyin((caddr_t) uap->uex, (caddr_t) kex, 
		(u_int) sizeof(struct export))) {
		error = EFAULT;
		goto error_return;
	}
	if (kex->ex_flags & ~EX_ALL) {
		error = EINVAL;
		goto error_return;
	}
	if (!(kex->ex_flags & EX_RDONLY) && mounted_ro) {
		error = EROFS;
		goto error_return;
	}
	if (kex->ex_flags & EX_EXCEPTIONS) {
		error = loadaddrs(&kex->ex_roaddrs);
		if (error)
			goto error_return;
		error = loadaddrs(&kex->ex_rwaddrs);
		if (error)
			goto error_return;
	}
	switch (kex->ex_auth) {
	case AUTH_UNIX:
		error = loadaddrs(&kex->ex_unix.rootaddrs);
		break;
	case AUTH_DES:
		error = loadrootnames(&kex->ex_des);
		break;
	case AUTH_ESV:
		error = loadaddrs(&kex->ex_esv.esvrootaddrs);
		break;
	default:
		error = EINVAL;
	}
	if (error) {	
		goto error_return;
	}

	/*
	 * Commit the new information to the export list, making
	 * sure to delete the old entry for the fs, if one exists.
	 */
	tail = &exported;
	while (*tail != NULL) {
		if (exportmatch(*tail, &exi->exi_fsid, exi->exi_fid)) {
			tmp = *tail;
			*tail = (*tail)->exi_next;
			exportfree(tmp);
		} else {
			tail = &(*tail)->exi_next;
		}
	}
	exi->exi_next = NULL;
	*tail = exi;
	return (0);

error_return:	
	freefid(exi->exi_fid);
	mem_free((char *) exi, sizeof(struct exportinfo));

	return (error);
}


/*
 * Remove the exported directory from the export list
 */
STATIC int
unexport(fsid, fid)
	fsid_t *fsid;
	struct fid *fid;
{
	struct exportinfo **tail;	
	struct exportinfo *exi;

	tail = &exported;
	while (*tail != NULL) {
		if (exportmatch(*tail, fsid, fid)) {
			exi = *tail;
			*tail = (*tail)->exi_next;
			exportfree(exi);
			return (0);
		} else {
			tail = &(*tail)->exi_next;
		}
	}
	return (EINVAL);
}

struct nfs_getfh_args {
	char	*fname;
	fhandle_t	*fhp;
};

/*
 * Get file handle system call.
 * Takes file name and returns a file handle for it.
 */
nfs_getfh(uap)
	register struct nfs_getfh_args *uap;
{
	fhandle_t fh;
	struct vnode *vp;
	struct vnode *dvp;
	struct exportinfo *exi;	
	int error;

	if (pm_denied(u.u_cred, P_FILESYS))
		return (EPERM);

	error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, 
			       &dvp, &vp);
	if (error == EINVAL) {
		/*
		 * if fname resolves to / we get EINVAL error
		 * since we wanted the parent vnode. Try again
		 * with NULL dvp.
		 */
		error = lookupname(uap->fname, UIO_USERSPACE,
			FOLLOW, (struct vnode **)NULL, &vp);
		dvp = NULL;
	}
	if (error == 0 && vp == NULL) {
		/*
		 * Last component of fname not found
		 */
		if (dvp) {
			VN_RELE(dvp);
		}
		error = ENOENT;
	}
	if (error)
		return (error);
	error = findexivp(&exi, dvp, vp);
	if (!error) {
		error = makefh(&fh, vp, exi);
		if (!error) {
			if (copyout((caddr_t)&fh, (caddr_t)uap->fhp, 
					sizeof(fh)))
				error = EFAULT;
		}
	}
	VN_RELE(vp);
	if (dvp != NULL) {
		VN_RELE(dvp);
	}
	return (error);
}

/*
 * Strategy: if vp is in the export list, then
 * return the associated file handle. Otherwise, ".."
 * once up the vp and try again, until the root of the
 * filesystem is reached.
 */
STATIC int
findexivp(exip, dvp, vp)
	struct exportinfo **exip;
	struct vnode *dvp;  /* parent of vnode want fhandle of */
	struct vnode *vp;   /* vnode we want fhandle of */
{
	struct fid *fid;
	int error;

	VN_HOLD(vp);
	if (dvp != NULL) {
		VN_HOLD(dvp);
	}
	for (;;) {
		error = VOP_FID(vp, &fid);
		if (error) {
			break;
		}
		*exip = findexport(&vp->v_vfsp->vfs_fsid, fid); 
		freefid(fid);
		if (*exip != NULL) {
			/*
			 * Found the export info
			 */
			error = 0;
			break;
		}

		/*
		 * We have just failed finding a matching export.
		 * If we're at the root of this filesystem, then
		 * it's time to stop (with failure).
		 */
		if (vp->v_flag & VROOT) {
			error = EINVAL;
			break;	
		}

		/*
		 * Now, do a ".." up vp. If dvp is supplied, use it,
	 	 * otherwise, look it up.
		 */
		if (dvp == NULL) {
			error = VOP_LOOKUP(vp, "..", &dvp,
					(struct pathname *)NULL, 0,
					(struct vnode *) 0,	/* XXX - unused? */
					u.u_cred);
			if (error) {
				break;
			}
		}
		VN_RELE(vp);
		vp = dvp;
		dvp = NULL;
	}
	VN_RELE(vp);
	if (dvp != NULL) {
		VN_RELE(dvp);
	}
	return (error);
}

/*
 * Make an fhandle from a vnode
 */
int
makefh(fh, vp, exi)
	register fhandle_t *fh;
	struct vnode *vp;
	struct exportinfo *exi;
{
	struct fid *fidp;
	int error;

	error = VOP_FID(vp, &fidp);
	if (error || fidp == NULL) {
		/*
		 * Should be something other than EREMOTE
		 */
		return (EREMOTE);
	}
	if (fidp->fid_len + exi->exi_fid->fid_len + sizeof(fsid_t) 
		> NFS_FHSIZE) {
		freefid(fidp);
		return (EREMOTE);
	}
	bzero((caddr_t) fh, sizeof(*fh));
	fh->fh_fsid.val[0] = vp->v_vfsp->vfs_fsid.val[0];
	fh->fh_fsid.val[1] = vp->v_vfsp->vfs_fsid.val[1];
	fh->fh_len = fidp->fid_len;
	bcopy(fidp->fid_data, fh->fh_data, fidp->fid_len);
	fh->fh_xlen = exi->exi_fid->fid_len;
	bcopy(exi->exi_fid->fid_data, fh->fh_xdata, fh->fh_xlen);
	NFSLOG(0x40, "makefh: vp %x fsid %x ", vp, fh->fh_fsid.val[0]);
	NFSLOG(0x40, "%x len %d\n", fh->fh_fsid.val[1], fh->fh_len);
	/* these are at halfword offsets - data alignment panic */
	/* NFSLOG(0x40, "data %d %d\n",
		*(int *)fh->fh_data, *(int *)&fh->fh_data[sizeof(int)]); */
	freefid(fidp);
	return (0);
}

/*
 * Find the export structure associated with the given filesystem
 */
struct exportinfo *
findexport(fsid, fid)
	fsid_t *fsid;	
	struct fid *fid;
{
	struct exportinfo *exi;

	for (exi = exported; exi != NULL; exi = exi->exi_next) {
		if (exportmatch(exi, fsid, fid)) {
			return (exi);
		}
	}
	return (NULL);
}

/*
 * Load from user space a list of exception addresses and masks
 */
STATIC int
loadaddrs(addrs)
	struct exaddrlist *addrs;
{
	char *tmp;
	int allocsize;
	register int i;
	struct netbuf *uaddrs;
	struct netbuf *umasks;

	if (addrs->naddrs > EXMAXADDRS)
		return (EINVAL);
	if (addrs->naddrs == 0)
		return (0);

	allocsize = addrs->naddrs * sizeof(struct netbuf);
	uaddrs = addrs->addrvec;
	umasks = addrs->addrmask;

	addrs->addrvec = (struct netbuf *) mem_alloc(allocsize);
	if (copyin((caddr_t)uaddrs, (caddr_t)addrs->addrvec, (u_int)allocsize)) {
		mem_free((char *)addrs->addrvec, allocsize);
		return (EFAULT);
	}

	addrs->addrmask = (struct netbuf *) mem_alloc(allocsize);
	if (copyin((caddr_t)umasks, (caddr_t)addrs->addrmask, (u_int)allocsize)) {
		mem_free((char *)addrs->addrmask, allocsize);
		mem_free((char *)addrs->addrvec, allocsize);
		return (EFAULT);
	}

	for (i = 0; i < addrs->naddrs; i++) {
		tmp = (char *) mem_alloc(addrs->addrvec[i].len);
		if (copyin(addrs->addrvec[i].buf, tmp, (u_int) addrs->addrvec[i].len)) {
			register int j;

			for (j = 0; j < i; j++)
				mem_free((char *) addrs->addrvec[j].buf, addrs->addrvec[j].len);
			mem_free(tmp, addrs->addrvec[i].len);
			mem_free((char *)addrs->addrmask, allocsize);
			mem_free((char *)addrs->addrvec, allocsize);
			return (EFAULT);
		}
		else
			addrs->addrvec[i].buf = tmp;
	}

	for (i = 0; i < addrs->naddrs; i++) {
		tmp = (char *) mem_alloc(addrs->addrmask[i].len);
		if (copyin(addrs->addrmask[i].buf, tmp, (u_int) addrs->addrmask[i].len)) {
			register int j;

			for (j = 0; j < i; j++)
				mem_free((char *) addrs->addrmask[j].buf, addrs->addrmask[j].len);
			mem_free(tmp, addrs->addrmask[i].len);
			for (j = 0; j < addrs->naddrs; j++)
				mem_free((char *) addrs->addrvec[j].buf, addrs->addrvec[j].len);
			mem_free((char *)addrs->addrmask, allocsize);
			mem_free((char *)addrs->addrvec, allocsize);
			return (EFAULT);
		}
		else
			addrs->addrmask[i].buf = tmp;
	}
	return (0);
}

/*
 * free an exaddrlist struct
 */
STATIC void
freeaddrs(addrs)
	struct exaddrlist *addrs;
{
	int i;
	for (i = 0; i < addrs->naddrs; i++) {
		mem_free(addrs->addrvec[i].buf,
			 addrs->addrvec[i].len);
		mem_free(addrs->addrmask[i].buf,
			 addrs->addrmask[i].len);
	}
	mem_free((char *)addrs->addrvec,
		 addrs->naddrs * sizeof(struct netbuf));
	mem_free((char *)addrs->addrmask,
		 addrs->naddrs * sizeof(struct netbuf));
}

/*
 * Load from user space the root user names into kernel space
 * (AUTH_DES only)
 */
STATIC int
loadrootnames(dex)
	struct desexport *dex;
{
	int error;
	char *exnames[EXMAXROOTNAMES];
	int i;
	u_int len;
	char netname[MAXNETNAMELEN+1];
	u_int allocsize;

	if (dex->nnames > EXMAXROOTNAMES)
		return (EINVAL);
	if (dex->nnames == 0)
		return (0);

	/*
	 * Get list of names from user space
	 */
	allocsize =  dex->nnames * sizeof(char *);
	if (copyin((char *)dex->rootnames, (char *)exnames, allocsize))
		return (EFAULT);
	dex->rootnames = (char **) mem_alloc(allocsize);
	bzero((char *) dex->rootnames, allocsize);

	/*
	 * And now copy each individual name
	 */
	for (i = 0; i < dex->nnames; i++) {
		error = copyinstr(exnames[i], netname, sizeof(netname), &len);
		if (error) {
			goto freeup;
		}
		dex->rootnames[i] = mem_alloc(len + 1);
		bcopy(netname, dex->rootnames[i], len);
		dex->rootnames[i][len] = 0;
	}
	return (0);

freeup:
	freenames(dex);
	return (error);
}

/*
 * Figure out everything we allocated in a root user name list in
 * order to free it up. (AUTH_DES only)
 */
STATIC void
freenames(dex)
	register struct desexport *dex;
{
	register int i;

	for (i = 0; i < dex->nnames; i++) {
		if (dex->rootnames[i] != NULL) {
			mem_free((char *) dex->rootnames[i],
				strlen(dex->rootnames[i]) + 1);
		}
	}	
	mem_free((char *) dex->rootnames, dex->nnames * sizeof(char *));
}

/*
 * Free an entire export list node
 */
STATIC void
exportfree(exi)
	struct exportinfo *exi;
{
	struct export *ex;

	ex = &exi->exi_export;
	switch (ex->ex_auth) {
	case AUTH_UNIX:
		freeaddrs(&ex->ex_unix.rootaddrs);
		break;
	case AUTH_DES:
		freenames(&ex->ex_des);
		break;
	case AUTH_ESV:
		freeaddrs(&ex->ex_esv.esvrootaddrs);
		break;
	}
	if (ex->ex_flags & EX_EXCEPTIONS) {
		freeaddrs(&ex->ex_roaddrs);
		freeaddrs(&ex->ex_rwaddrs);
	}
	freefid(exi->exi_fid);
	mem_free(exi, sizeof(struct exportinfo));
}

/*
 * setnfslp(): store a new nfslpbuf list for use in all future NFS requests.
 */

STATIC	u_int		nfslpcount = 0;	/* number of entries */
STATIC	struct nfslpbuf *nfslpbuf;	/* encoded /etc/lid_and_priv */
STATIC	struct nfslpbuf	defnfslpbuf = {
	NULL, NULL, NULL, 0, 0
};					/* global server defaults */

int
setnfslp(ubuf, size, lid, priv)
	struct nfslpbuf *ubuf;
	u_int size;
	lid_t lid;
	pvec_t priv;
{
	struct nfslpbuf *tmplp, *flp;
	int i, j;
	struct netbuf *tbuf;
	char *tc;

	if (pm_denied(u.u_cred, P_FILESYS))
		return (EPERM);

	if (nfslpcount) {
		for (tmplp = nfslpbuf, i = 0; i < nfslpcount; tmplp++, i++) {
			kmem_free(tmplp->addr->buf, tmplp->addr->maxlen);
			kmem_free(tmplp->mask->buf, tmplp->mask->maxlen);
			kmem_free(tmplp->addr, sizeof (struct netbuf));
			kmem_free(tmplp->mask, sizeof (struct netbuf));
		}
		kmem_free(nfslpbuf, nfslpcount * sizeof(struct nfslpbuf));
	}
	nfslpbuf = (struct nfslpbuf *)kmem_alloc(size, KM_SLEEP);
	if (copyin((caddr_t)ubuf, (caddr_t)nfslpbuf, size)) {
		kmem_free(nfslpbuf, size);
		nfslpcount = 0;
		return (EFAULT);
	} else
		nfslpcount = size / sizeof(struct nfslpbuf);
	for (tmplp = nfslpbuf, i = 0; i < nfslpcount; tmplp++, i++) {
		/* restrict the privs we're giving away to what we have */
		tmplp->priv &= u.u_cred->cr_maxpriv;

		/* copyin everything, checking for errors along the way */
		tbuf = tmplp->addr;
		tmplp->addr = (struct netbuf *)kmem_alloc(sizeof(struct netbuf), KM_SLEEP);
		if (copyin((caddr_t)tbuf, (caddr_t)tmplp->addr, sizeof(struct netbuf))) {
			for (flp = nfslpbuf, j = 0; j < i; flp++, j++) {
				kmem_free(flp->addr->buf, flp->addr->maxlen);
				kmem_free(flp->mask->buf, flp->mask->maxlen);
				kmem_free(flp->addr, sizeof(struct netbuf));
				kmem_free(flp->mask, sizeof(struct netbuf));
			}
			kmem_free(tmplp->addr, sizeof(struct netbuf));
			kmem_free(nfslpbuf, size);
			nfslpcount = 0;
			return (EFAULT);
		}
		tc = tmplp->addr->buf;
		tmplp->addr->buf = kmem_alloc(tmplp->addr->maxlen, KM_SLEEP);
		if (copyin((caddr_t)tc, (caddr_t)tmplp->addr->buf, tmplp->addr->maxlen)) {
			for (flp = nfslpbuf, j = 0; j < i; flp++, j++) {
				kmem_free(flp->addr->buf, flp->addr->maxlen);
				kmem_free(flp->mask->buf, flp->mask->maxlen);
				kmem_free(flp->addr, sizeof(struct netbuf));
				kmem_free(flp->mask, sizeof(struct netbuf));
			}
			kmem_free(tmplp->addr->buf, tmplp->addr->maxlen);
			kmem_free(tmplp->addr, sizeof(struct netbuf));
			kmem_free(nfslpbuf, size);
			nfslpcount = 0;
			return (EFAULT);
		}
		tbuf = tmplp->mask;
		tmplp->mask = (struct netbuf *)kmem_alloc(sizeof(struct netbuf), KM_SLEEP);
		if (copyin((caddr_t)tbuf, (caddr_t)tmplp->mask, sizeof(struct netbuf))) {
			for (flp = nfslpbuf, j = 0; j < i; flp++, j++) {
				kmem_free(flp->addr->buf, flp->addr->maxlen);
				kmem_free(flp->mask->buf, flp->mask->maxlen);
				kmem_free(flp->addr, sizeof(struct netbuf));
				kmem_free(flp->mask, sizeof(struct netbuf));
			}
			kmem_free(tmplp->addr->buf, tmplp->addr->maxlen);
			kmem_free(tmplp->addr, sizeof(struct netbuf));
			kmem_free(tmplp->mask, sizeof(struct netbuf));
			kmem_free(nfslpbuf, size);
			nfslpcount = 0;
			return (EFAULT);
		}
		tc = tmplp->mask->buf;
		tmplp->mask->buf = kmem_alloc(tmplp->mask->maxlen, KM_SLEEP);
		if (copyin((caddr_t)tc, (caddr_t)tmplp->mask->buf, tmplp->mask->maxlen)) {
			for (flp = nfslpbuf, j = 0; j <= i; flp++, j++) {
				kmem_free(flp->addr->buf, flp->addr->maxlen);
				kmem_free(flp->mask->buf, flp->mask->maxlen);
				kmem_free(flp->addr, sizeof(struct netbuf));
				kmem_free(flp->mask, sizeof(struct netbuf));
			}
			kmem_free(nfslpbuf, size);
			nfslpcount = 0;
			return (EFAULT);
		}
	}
	defnfslpbuf.lid = lid;
	defnfslpbuf.priv = (priv & u.u_cred->cr_maxpriv);
	defnfslpbuf.addr = defnfslpbuf.mask = defnfslpbuf.dummy = NULL;
	return (0);
}

/*
 * applynfslp(): find the nfslpbuf entry for the host at 'addr' (or the default
 * entry if no specific one). Filter the priv vector in 'cred' with the
 * priv filter, and, if flag is set, set cred->cr_lid.
 * After this routine returns the privs and lid in the cred will be set
 * correctly for this host.
 */
void
applynfslp(addr, cred, flag)
	struct netbuf *addr;
	struct cred *cred;
	u_int flag;
{
	struct nfslpbuf *tmplp;
	int i;

	for (tmplp = nfslpbuf, i = 0; i < nfslpcount; tmplp++, i++)
		if (eqaddr(addr, tmplp->addr, tmplp->mask)) {
			cred->cr_wkgpriv &= tmplp->priv;
			cred->cr_maxpriv &= tmplp->priv;
			if (flag)
				cred->cr_lid = tmplp->lid;
			return;
		}
	cred->cr_wkgpriv &= defnfslpbuf.priv;
	cred->cr_maxpriv &= defnfslpbuf.priv;
	if (flag)
		cred->cr_lid = defnfslpbuf.lid;
	return;
}
