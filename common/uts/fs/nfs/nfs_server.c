/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/nfs/nfs_server.c	1.17.2.6"
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
 *  		  All rights reserved.
 */

#define NFSSERVER

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <acc/dac/acl.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <fs/buf.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <io/uio.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <svc/errno.h>
#include <net/transport/socket.h>
#include <util/sysmacros.h>
#include <proc/siginfo.h>
#include <net/tcpip/in.h>
#include <net/transport/tiuser.h>
#include <fs/statvfs.h>
#include <net/ktli/t_kuser.h>
#include <mem/kmem.h>
#include <net/rpc/types.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_unix.h>
#include <net/rpc/auth_des.h>
#include <net/rpc/auth_esv.h>
#include <net/rpc/svc.h>
#include <net/rpc/xdr.h>
#include <net/rpc/token.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/export.h>
#include <fs/dirent.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <mem/hat.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_kmem.h>

extern struct vnodeops nfs_vnodeops;

struct rfsspace {
	struct rfsspace *rs_next;
	caddr_t		rs_dummy;
};

struct rfsspace *rfsfreesp = NULL;

int rfssize = 0;

/* NFS server procedures */
STATIC	void	rfs_getattr();
STATIC	void	rfs_setattr();
STATIC	void	rfs_lookup();
STATIC	void	rfs_readlink();
STATIC	void	rfs_read();
STATIC	void	rfs_write();
STATIC	void	rfs_create();
STATIC	void	rfs_remove();
STATIC	void	rfs_rename();
STATIC	void	rfs_link();
STATIC	void	rfs_symlink();
STATIC	void	rfs_mkdir();
STATIC	void	rfs_rmdir();
STATIC	void	rfs_readdir();
STATIC	void	rfs_statfs();
STATIC	void	rfs_access();
STATIC	void	rfs_null();
STATIC	void	rfs_error();

STATIC	void	rfs_success();
STATIC	void	rfs_diropres();

#ifdef __STDC__
/* server procedure result data free procs */
STATIC	void	rfs_rlfree(struct nfsrdlnres *);
STATIC	void	rfs_rdfree(struct nfsrdresult *);
STATIC	void	rfs_esvrdfree(struct nfsesvrdresult *);
STATIC	void	rfs_rddirfree(struct nfsrddirres *);
STATIC	void	rfs_esvrddirfree(struct nfsesvrddirres *);
STATIC	void	nullfree(void);

/* arg/result data area mgmt */
STATIC	caddr_t	rfsget(void);
STATIC	void	rfsput(struct rfsspace *);

/* utilities */
STATIC	int	get_esv_attrs(struct vnode *, struct nfsesvfattr *, struct netbuf *);
STATIC	struct vnode	*fhtovp(fhandle_t *, struct exportinfo *);
STATIC	void	rfs_dispatch(struct svc_req *, SVCXPRT *);
STATIC	int	hostinlist(struct netbuf *, struct exaddrlist *);
STATIC	int	rootname(struct export *, char *);
STATIC	int	checkauth(struct exportinfo *, struct svc_req *, struct cred *);
#else /* __STDC__ */
/* server procedure result data free procs */
STATIC	void	rfs_rlfree();
STATIC	void	rfs_rdfree();
STATIC	void	rfs_esvrdfree();
STATIC	void	rfs_rddirfree();
STATIC	void	rfs_esvrddirfree();
STATIC	void	nullfree();

/* arg/result data area mgmt */
STATIC	caddr_t	rfsget();
STATIC	void	rfsput();

/* utilities */
STATIC	int	get_esv_attrs();
STATIC	struct vnode	*fhtovp();
STATIC	void	rfs_dispatch();
STATIC	int	hostinlist();
STATIC	int	rootname();
STATIC	int	checkauth();
#endif /* __STDC__ */

/*
 * rpc service program version range supported
 * NOTE: We really support NFS version 2 and ESV NFS version 1.
 */
#define	VERSIONMIN	2
#define	VERSIONMAX	3
#define	ESVVERSIONMIN	1

#define IS_ESV(req)	((req)->rq_prog == NFS_ESVPROG && (req)->rq_vers == NFS_ESVVERS)
#define IS_V2(req)	((req)->rq_prog == NFS_PROGRAM && (req)->rq_vers == NFS_VERSION)
#define printnfsprog(req)	((req)->rq_prog == NFS_ESVPROG ? "ESV" : ((req)->rq_prog == NFS_PROGRAM ? "V2" : "unknown"))

/*
 *	Returns true iff exported filesystem is read-only to the given host.
 *	This can happen in two ways:  first, if the default export mode is
 *	read only, and the host's address isn't in the rw list;  second,
 *	if the default export mode is read-write but the host's address
 *	is in the ro list.  The optimization (checking for EX_EXCEPTIONS)
 *	is to allow us to skip the calls to hostinlist if no host exception
 *	lists were loaded.
 *
 *	Note:  this macro should be as fast as possible since it's called
 *	on each NFS modification request.
 */
#define	rdonly(exi, req)						\
	(								\
	  (								\
	    ((exi)->exi_export.ex_flags & EX_RDONLY)			\
	    &&								\
	    (								\
	      (((exi)->exi_export.ex_flags & EX_EXCEPTIONS) == 0) ||	\
	      !hostinlist(svc_getrpccaller((req)->rq_xprt), 		\
	 	  &(exi)->exi_export.ex_rwaddrs)			\
	    )								\
	  )								\
	  ||								\
	  (								\
	    ((exi)->exi_export.ex_flags & EX_RDWR)			\
	    &&								\
	    (								\
	      ((exi)->exi_export.ex_flags & EX_EXCEPTIONS) &&		\
	      hostinlist(svc_getrpccaller((req)->rq_xprt),		\
	 	 &(exi)->exi_export.ex_roaddrs)				\
	    )								\
	  )								\
	)

struct {
	int	ncalls;		/* number of calls received */
	int	nbadcalls;	/* calls that failed */
	int	reqs[32];	/* count for each request */
} svstat;

struct nfs_svc_args {
	int fd;
};
int nfs_server_count = 0;

/*
 * NFS Server system call.
 * Does all of the work of running a NFS server.
 * uap->fd is the fd of an open transport provider
 */
nfs_svc(uap)
	struct nfs_svc_args *uap;
{
	struct file *fp;
	SVCXPRT *xprt;
	u_long vers;
	register int	error;

	if (getf(uap->fd, &fp)) {
		return EBADF;
	}

	/*
	 * Now, release client memory; we never return back to user.
	 * On some implementations, relvm() may not actually free
	 * the address space. Hence the flag ISADAEMON.
	 * 
	 */
	relvm(u.u_procp, ISADAEMON);

	/*
	 * Create a transport handle.
	 */
	if ((error = svc_tli_kcreate(fp, 0, &xprt)) != 0) {
		NFSLOG(1, "nfs_svc: svc_tli_kcreate failed %d, exiting\n",
			error, 0);
		exit(CLD_EXITED, 0);
	}

	/*
	 * register the services
	 */
	(void)svc_register(xprt, NFS_PROGRAM, NFS_VERSION, rfs_dispatch, FALSE);
	(void)svc_register(xprt, NFS_ESVPROG, NFS_ESVVERS, rfs_dispatch, FALSE);

	nfs_server_count++;
	/*
	 * enter rpc poll loop
	 */
	svc_run(xprt);

	if (--nfs_server_count == 0) {
		/*
		 * no more daemons so unregister all services
		 */
		for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
			svc_unregister(NFS_PROGRAM, vers);
		}
		svc_unregister(NFS_ESVPROG, NFS_ESVVERS);
	}

	/*
	 * next statement not needed but it may help debugging
	 */
	u.u_error = EINTR;

	/*
	 * remove all pending signals
	 */
	{
		struct proc *p = u.u_procp;
		p->p_cursig = 0;
		if (p->p_curinfo) {
			kmem_free((caddr_t)p->p_curinfo,
				sizeof(*p->p_curinfo));
			p->p_curinfo = NULL;
		}
	}

	exit(CLD_EXITED, 0);
}

/*
 * Utility routine to retrieve the level of a vnode. This is called from
 * fhtovp() and various other operations, as VOP_LOOKUP won't always set the
 * LID of the new vp.
 *
 * Use the level of the root vnode with VFS_ROOT to set the level.
 *
 */
#define NFSRV_GETLID(vp, cred)					\
{								\
	struct vnode *rtvp;					\
	if (VFS_ROOT((vp)->v_vfsp, &rtvp) == 0) {		\
		(vp)->v_lid = rtvp->v_lid;			\
		VN_RELE(rtvp);					\
	}							\
}

/*
 * Utility to set an ACL. Assumes all its arguments are set (except dacl,
 * which will be set by acl_valid).
 */
#define NFSRV_SETACL(vp, nacl, dacl, dirflag, aclp, cred)		      \
{									      \
	if (mac_installed && (acl_valid(aclp, nacl, dirflag, &(dacl)) == 0))  \
		(void)VOP_SETACL((vp), (nacl), (dacl), (aclp), (cred));       \
}

/*
 * nfsrv_setlevel: perform the MAC checks necessary to allow a process to set
 * a file level. If all checks pass, issue the VOP_SETLEVEL. The checks are
 * taken from clvlfile() in acc/mac/vnmac.c
 */
STATIC int
nfsrv_setlevel(vp, level, cred)
	struct vnode *vp;
	lid_t level;
	struct cred *cred;
{
	struct vattr va;
	int error;

	va.va_mask = AT_UID;
	error = VOP_GETATTR(vp, &va, 0, cred);

	if (!error)
		error = MAC_VACCESS(vp, VWRITE, cred);

	if (!error && va.va_uid != cred->cr_uid)
		error = pm_denied(cred, P_OWNER);

	if (!error && (error = pm_denied(cred, P_SETFLEVEL))) {
		if (MAC_ACCESS(MACDOM, level, vp->v_lid) == 0)
			error = pm_denied(cred, P_MACUPGRADE);
	}
	if (!error)
		error = VOP_SETLEVEL(vp, level, cred);
	return (error);
}

/*
 * Utility routine to get all the attributes for a vnode and store
 * in a struct nfsesvfattr. This will retrieve the regular attributes,
 * the LID, the ACL, and the MLD indication.
 * Returns the return code from the VOP_GETATTR (assuming error from
 * VOP_GETACL simply mean there is no ACL 
 * Errors from the token mapper are also ignored).
 */
STATIC int
get_esv_attrs(vp, attr, addr)
	struct vnode *vp;
	struct nfsesvfattr *attr;
	struct netbuf *addr;
{
	struct acl *aclp;
	int nacl, dacl;
	struct vattr va;
	int error;

	va.va_mask = AT_ALL;	/* we want all the attributes */
	error = VOP_GETATTR(vp, &va, 0, u.u_cred);

	if (!error) {
		aclp = (struct acl *)kmem_alloc(acl_getmax() * sizeof (struct acl),
						KM_SLEEP);
		if (!VOP_GETACL(vp, acl_getmax(), &dacl, aclp, u.u_cred, &nacl))
			nacl = 0;

		if (!(vp->v_macflag & VMAC_SUPPORT)) {
			NFSRV_GETLID(vp, u.u_cred);
			if (vp->v_lid != 0)
				NFSLOG(0x40, "get_esv_attrs: LID changed to %d\n", vp->v_lid, 0);
			else
				NFSLOG(0x40, "get_esv_attrs: LID 0\n", 0, 0);
		} else
			NFSLOG(0x40, "get_esv_attrs: vattr_to_esvnattr: LID %d\n", vp->v_lid, 0);
		vattr_to_esvnattr(&va, attr, addr,
				  &vp->v_lid, aclp, nacl * sizeof(struct acl));
		kmem_free(aclp, acl_getmax() * sizeof(struct acl));
	}

	if (!error && (vp->v_macflag & VMAC_ISMLD)) {
		NFSLOG(0x40, "get_esv_attrs: encoding an MLD\n", 0, 0);
		NA_SETMLD(attr);
	}
	
	return (error);
}


/*
 * These are the interface routines for the server side of the
 * Network File System.  See the NFS protocol specification
 * for a description of this interface.
 */


/*
 * Get file attributes.
 * Returns the current attributes of the file with the given fhandle.
 */
STATIC void
rfs_getattr(fhp, res, exi, req)
	fhandle_t *fhp;
	register caddr_t *res;	/* result struct - cast to either v.2 or ESV */
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct nfsattrstat *ns = (struct nfsattrstat *)res;
	register struct nfsesvattrstat *cns = (struct nfsesvattrstat *)res;
	register struct vnode *vp;
	struct vattr va;

	NFSLOG(0x10, "rfs_getattr fh %x %x ",
		fhp->fh_fsid.val[0], fhp->fh_fsid.val[1]);
	NFSLOG(0x10, "%d: %s prog\n", fhp->fh_len, printnfsprog(req));
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		if (IS_V2(req))
			ns->ns_status = NFSERR_STALE;
		else if (IS_ESV(req))
			cns->ns_status = NFSERR_STALE;
		return;
	}
	/*
	 * Must have MAC read access to the vnode.
	 */
	error = MAC_VACCESS(vp, VREAD, u.u_cred);

	if (IS_V2(req)) {
		va.va_mask = AT_ALL;	/* we want all the attributes */
		if (!error)
			error = VOP_GETATTR(vp, &va, 0, u.u_cred);
		if (!error) {
			vattr_to_nattr(&va, &ns->ns_attr);
		}
		ns->ns_status = puterrno(error);
	} else if (IS_ESV(req)) {
		if (!error)
			error = get_esv_attrs(vp, &cns->ns_attr, svc_getrpccaller(req->rq_xprt));
		cns->ns_status = puterrno(error);
	}
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_getattr: returning %d\n", error, 0);
}

/*
 * Set file attributes.
 * Sets the attributes of the file with the given fhandle.  Returns
 * the new attributes.
 */
STATIC void
rfs_setattr(args, res, exi, req)
	register caddr_t *args;
	register caddr_t *res;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfssaargs *saarg = (struct nfssaargs *)args;
	struct nfsesvsaargs *csaarg = (struct nfsesvsaargs *)args;
	struct nfsattrstat *ns = (struct nfsattrstat *)res;
	struct nfsesvattrstat *cns = (struct nfsesvattrstat *)res;
	int error = 0;
	int flag;
	register struct vnode *vp;
	struct vattr va;
	lid_t tmplid;
	struct acl *aclp;
	u_int nacl, dacl;

	NFSLOG(0x10, "rfs_setattr fh %x %x ",
		saarg->saa_fh.fh_fsid.val[0], saarg->saa_fh.fh_fsid.val[1]);
	NFSLOG(0x10, "%d: %s prog\n", saarg->saa_fh.fh_len, printnfsprog(req));
	/* Both saarg and csaarg have fhandle as the first element */
	vp = fhtovp(&saarg->saa_fh, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req) || (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;
	} else {
		if (IS_V2(req))
			sattr_to_vattr(&saarg->saa_sa, &va);
		else if (IS_ESV(req)) {
			nacl = acl_getmax();
			aclp = (struct acl *)kmem_alloc(nacl*sizeof(struct acl),
							KM_SLEEP);
			esvsattr_to_vattr(&csaarg->saa_sa, &va, &tmplid, aclp, &nacl);
		}

		/*
		 * Allow System V-compatible option to set access and
		 * modified times if root, owner, or write access.
		 *
		 * XXX - Until an NFS Protocol Revision, this may be
		 *       simulated by setting the client time in the
		 *       tv_sec field of the access and modified times
		 *       and setting the tv_nsec field of the modified
		 *       time to an invalid value (1,000,000).  This
		 *       may be detected by servers modified to do the
		 *       right thing, but will not be disastrous on
		 *       unmodified servers.
		 * XXX - 1,000,000 is actually a valid tv_nsec value,
		 *	 so we must look in the pre-converted nfssaargs
		 *	 structure instead.
		 * XXX - For now, va_mtime.tv_nsec == -1 flags this in
		 *	 VOP_SETATTR(), but not all file system setattrs
		 *	 respect this convention (for example, s5setattr).
		 */
		if (IS_V2(req))
			if ((saarg->saa_sa.sa_mtime.tv_sec != (u_long)-1) &&
			    (saarg->saa_sa.sa_mtime.tv_usec == 1000000))
				flag = 0;
			else
				flag = ATTR_UTIME;
		else if (IS_ESV(req))
			if ((csaarg->saa_sa.sa_mtime.tv_sec != (u_long)-1) &&
			    (csaarg->saa_sa.sa_mtime.tv_usec == 1000000))
				flag = 0;
			else
				flag = ATTR_UTIME;

		/*
		 * Must have MAC write access to the vnode.
		 */
		error = MAC_VACCESS(vp, VWRITE, u.u_cred);

		/* don't do size changes with VOP_SETATTR */
		va.va_mask &= ~AT_SIZE;

		if (!error)
			error = VOP_SETATTR(vp, &va, flag, u.u_cred);

		if (((IS_V2(req) && saarg->saa_sa.sa_size != -1) ||
		     (IS_ESV(req) && csaarg->saa_sa.sa_size != -1)) &&
		    error == 0) {
			struct flock fl;

			fl.l_whence = 0;
			fl.l_start= (IS_V2(req)) ? saarg->saa_sa.sa_size :
						   csaarg->saa_sa.sa_size;
			fl.l_type = F_WRLCK;
			fl.l_len = 0;
			error = VOP_SPACE(vp, F_FREESP, &fl, 0, 0, u.u_cred);
		}

		/* set the LID if requested */
		if (!error && IS_ESV(req) && csaarg->saa_sa.sa_sens != 0)
			error = nfsrv_setlevel(vp, tmplid, u.u_cred);

		/* set the ACL if requested */
		if(!error && IS_ESV(req) && csaarg->saa_sa.sa_acl != (s_token)0)
			NFSRV_SETACL(vp, nacl, dacl, (vp->v_type == VDIR)?1:0,
				     aclp, u.u_cred);

		if (IS_ESV(req))
			kmem_free(aclp, acl_getmax() * sizeof(struct acl));
		/* get attrs to return */
		if (!error) {
			if (IS_V2(req)) {
				va.va_mask = AT_ALL;	/* get everything */
				error = VOP_GETATTR(vp, &va, 0, u.u_cred);
				if (!error) {
					vattr_to_nattr(&va, &ns->ns_attr);
				}
			} else if (IS_ESV(req)) {
				error = get_esv_attrs(vp, &cns->ns_attr,
						svc_getrpccaller(req->rq_xprt));
			}
		}
	}
	ns->ns_status = puterrno(error);
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_setattr: returning %d\n", error, 0);
}

/*
 * Directory lookup.
 * Returns an fhandle and file attributes for file name in a directory.
 */
STATIC void
rfs_lookup(da, res, exi, req)
	struct nfsdiropargs *da;
	register caddr_t *res;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfsdiropres *dr = (struct nfsdiropres *)res;
	struct nfsesvdiropres *cdr = (struct nfsesvdiropres *)res;
	int error;
	register struct vnode *dvp;
	struct vnode *vp;
	struct vattr va;

	NFSLOG(0x10, "rfs_lookup %s fh %x ",
		da->da_name, da->da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d",
		da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);
	NFSLOG(0x10, ": %s prog\n", printnfsprog(req), 0);
	/*
	 *	Disallow NULL paths
	 */
	if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
		dr->dr_status = NFSERR_ACCES;
		cdr->dr_status = NFSERR_ACCES;
		return;
	}

	dvp = fhtovp(&da->da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		cdr->dr_status = NFSERR_STALE;
		return;
	}
	/*
	 * need MAC exec access to the parent directory
	 */
	error = MAC_VACCESS(dvp, VEXEC, u.u_cred);

	/*
	 * do lookup.
	 */
	if (!error)
		error = VOP_LOOKUP(dvp, da->da_name, &vp,
				(struct pathname *) NULL, 0, (struct vnode *) 0,
				u.u_cred);

	/* MLD deflection if needed since local filesystem doesn't any more */
	if (!error && (vp->v_macflag & VMAC_ISMLD) &&
	    !(u.u_cred->cr_flags & CR_MLDREAL) &&	/* V2 protocol only */
	    (dvp != vp)) {
		char *tcomp = NULL;
		char eff_dirname[MLD_SZ];

		NFSLOG(0x40, "rfs_lookup: deflecting through MLD link %s at LID %d\n", da->da_name, u.u_cred->cr_lid);

		if (strcmp(da->da_name, "..") == 0) {
			tcomp = da->da_name;
		} else {
			fs_itoh(u.u_cred->cr_lid, eff_dirname);
			tcomp = eff_dirname;
		}
		VN_RELE(dvp);
		if (error = MAC_VACCESS(vp, VEXEC, u.u_cred)) {
			VN_RELE(vp);
		} else {
			dvp = vp;
			if (tcomp == da->da_name) {
				/*
				 * Looking up "..". Note that we don't back up
				 * the mount point hierarchy as is done in
				 * lookuppn(), as NFS does not cross mount
				 * points (in either direction).
				 */
				if (VN_CMP(dvp, u.u_rdir) ||
				    VN_CMP(dvp, rootdir)) {
					goto skip;
				}
				error = VOP_LOOKUP(dvp, tcomp, &vp,
						   (struct pathname *)NULL, 0,
						   (struct vnode *)NULL, u.u_cred);
				if (!error) {
					VN_RELE(dvp);
					if (!(vp->v_macflag & VMAC_SUPPORT) &&
					    vp->v_vfsp)
						vp->v_lid = vp->v_vfsp->vfs_macfloor;
				}
			} else {
				error = VOP_LOOKUP(dvp, tcomp, &vp,
						   (struct pathname *)NULL, 0,
						   (struct vnode *)NULL, u.u_cred);
				if (!error) {
					VN_RELE(dvp);
					if (vp->v_type != VDIR) {
						error = ENOTDIR;
						VN_RELE(vp);
					}
				} else if (error == ENOENT) {
					struct cred *tmpcr = crdup(u.u_cred);
					NFSLOG(0x40, "making new subdirectory %s\n", tcomp, 0);
					error = VOP_GETATTR(dvp, &va, 0, u.u_cred);
					if (error) {
						VN_RELE(dvp);
						crfree(tmpcr);
						goto skip;
					}
					va.va_mask = AT_TYPE|AT_MODE;
					va.va_type = VDIR;
					va.va_mode &= MODEMASK;
					tmpcr->cr_wkgpriv |= pm_privbit(P_MACWRITE);
					tmpcr->cr_wkgpriv |= pm_privbit(P_DACWRITE);
					/* create at same uid,gid as parent */
					tmpcr->cr_uid = tmpcr->cr_ruid = va.va_uid;
					tmpcr->cr_gid = tmpcr->cr_rgid = va.va_gid;
					error = VOP_MKDIR(dvp, tcomp, &va, &vp, tmpcr);
					crfree(tmpcr);
					if (error) {
						VN_RELE(dvp);
						goto skip;
					}
					dnlc_enter(dvp, tcomp, vp, (struct cred *)NULL);
					VN_RELE(dvp);
					if (!(vp->v_macflag & VMAC_SUPPORT) &&
					    vp->v_vfsp)
						vp->v_lid = vp->v_vfsp->vfs_macfloor;
					if (vp->v_lid != u.u_cred->cr_lid) {
						error = EINVAL;
						VN_RELE(vp);
					}
				}
			}
		}
	} else
		VN_RELE(dvp);
skip:
	/* at this point vp is our target, and only it is VN_HELD, only once */
	if (error) {
		vp = (struct vnode *)NULL;
	} else {
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;	/* we want everything */
			error = VOP_GETATTR(vp, &va, 0, u.u_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
				error = makefh(&dr->dr_fhandle, vp, exi);
			}
		} else if (IS_ESV(req)) {
			error = get_esv_attrs(vp, &cdr->dr_attr,
					svc_getrpccaller(req->rq_xprt));
			if (!error)
				error = makefh(&cdr->dr_fhandle, vp, exi);
		}
	}
	dr->dr_status = puterrno(error);
	cdr->dr_status = puterrno(error);
	if (vp) {
		VN_RELE(vp);
	}
	NFSLOG(0x20, "rfs_lookup: returning %d\n", error, 0);
}

/*
 * Read symbolic link.
 * Returns the string in the symbolic link at the given fhandle.
 */
STATIC void
rfs_readlink(fhp, res, exi, req)
	fhandle_t *fhp;
	register caddr_t *res;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfsrdlnres *rl = (struct nfsrdlnres *)res;
	struct nfsesvrdlnres *crl = (struct nfsesvrdlnres *)res;
	int error;
	struct iovec iov;
	struct uio uio;
	struct vnode *vp;

	NFSLOG(0x10, "rfs_readlink fh %x %x ",
		fhp->fh_fsid.val[0], fhp->fh_fsid.val[1]);
	NFSLOG(0x10, "%d: %s prog\n", fhp->fh_len, printnfsprog(req));
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		rl->rl_status = NFSERR_STALE;
		crl->rl_status = NFSERR_STALE;
		return;
	}

	/*
	 * Allocate data for pathname.  This will be freed by rfs_rlfree.
	 * Note that both results structs have their data, count, and status
	 * fields at the same offsets.
	 */
	rl->rl_data = (char *)kmem_alloc((u_int)MAXPATHLEN, KM_SLEEP);

	/*
	 * Set up io vector to read sym link data
	 */
	iov.iov_base = rl->rl_data;
	iov.iov_len = NFS_MAXPATHLEN;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = 0;
	uio.uio_resid = NFS_MAXPATHLEN;

	/*
	 * Must have MAC read access to the symlink
	 */
	error = MAC_VACCESS(vp, VREAD, u.u_cred);

	/*
	 * read link
	 */
	if (!error)
		error = VOP_READLINK(vp, &uio, u.u_cred);

	/* ESV readlink returns attributes */
	if (!error && IS_ESV(req))
		error = get_esv_attrs(vp, &crl->rl_attr,
				      svc_getrpccaller(req->rq_xprt));
	/*
	 * Clean up
	 */
	if (error) {
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
		rl->rl_count = 0;
		rl->rl_data = NULL;
	} else {
		rl->rl_count = NFS_MAXPATHLEN - uio.uio_resid;
	}
	rl->rl_status = puterrno(error);
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_readlink: returning '%s' %d\n", rl->rl_data, error);
}

/*
 * Free data allocated by rfs_readlink
 */
STATIC void
rfs_rlfree(rl)
	struct nfsrdlnres *rl;
{
	if (rl->rl_data) {
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
	}
}

int nfsreadmap = 1;

/*
 * Read data.
 * Returns some data read from the file at the given fhandle.
 */
STATIC void
rfs_read(ra, res, exi, req)
	register struct nfsreadargs *ra;
	register caddr_t *res;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfsrdresult *rr = (struct nfsrdresult *)res;
	struct nfsesvrdresult *crr = (struct nfsesvrdresult *)res;
	register struct vnode *vp;
	register int error, closerr;
	struct vattr va;
	struct iovec iov;
	struct uio uio;
	int offset;
	char *savedatap;
	int opened;
	struct vnode *avp;	/* addressable */
	/* Result structure field addresses for transparent code */
	u_long *rdcount = ((IS_V2(req)) ? &rr->rr_count : &crr->rr_count);
	char **rddata = ((IS_V2(req)) ? &rr->rr_data : &crr->rr_data);
	char **rdmap = ((IS_V2(req)) ? &rr->rr_map : &crr->rr_map);
	struct vnode **rdvp = ((IS_V2(req)) ? &rr->rr_vp : &crr->rr_vp);

	opened = 0;
	NFSLOG(0x10, "rfs_read %d from fh %x ",
		ra->ra_count, ra->ra_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d",
		ra->ra_fhandle.fh_fsid.val[1], ra->ra_fhandle.fh_len);
	NFSLOG(0x10, ": %s prog\n", printnfsprog(req), 0);
	*rddata = NULL;
	*rdcount = 0;
	vp = fhtovp(&ra->ra_fhandle, exi);
	if (vp == NULL) {
		rr->rr_status = NFSERR_STALE;
		return;
	}
	if (vp->v_type != VREG) {
		NFSLOG(1, "rfs_read: attempt to read from non-file\n", 0, 0);
		error = EISDIR;
	} else {
		VOP_RWLOCK(vp);
		va.va_mask = (IS_V2(req) ? AT_ALL : AT_SIZE|AT_UID);
		error = VOP_GETATTR(vp, &va, 0, u.u_cred);
	}
	if (error) {
		goto bad;
	}

	/*
	 * need MAC read access
	 */
	error = MAC_VACCESS(vp, VREAD, u.u_cred);
	if (error)
		goto bad;

	/*
	 * This is a kludge to allow reading of files created
	 * with no read permission.  The owner of the file
	 * is always allowed to read it.
	 */
	if (u.u_cred->cr_uid != va.va_uid) {
		error = VOP_ACCESS(vp, VREAD, 0, u.u_cred);
		if (error) {
			/*
			 * Exec is the same as read over the net because
			 * of demand loading.
			 */
			error = VOP_ACCESS(vp, VEXEC, 0, u.u_cred);
		}
		if (error) {
			goto bad;
		}
	}

	avp = vp;
	error = VOP_OPEN(&avp, FREAD, u.u_cred);
	vp = avp;
	if (error) {
		goto bad;
	}
	opened = 1;

	if (ra->ra_offset >= va.va_size) {
		*rdcount = 0;
		if (IS_V2(req))
			vattr_to_nattr(&va, &rr->rr_attr);
		else if (IS_ESV(req))
			error = get_esv_attrs(vp, &crr->rr_attr,
					      svc_getrpccaller(req->rq_xprt));
		goto done;			/* hit EOF */
	}

	/*
	 * Check whether we can do this with segmap,
	 * which would save the copy through the uio.
	 */
	offset = ra->ra_offset & MAXBOFFSET;
#ifdef	VNOCACHE
	if (nfsreadmap && (offset + ra->ra_count <= MAXBSIZE) &&
	    (vp->v_flag & VNOCACHE) == 0)
#else
	if (nfsreadmap && (offset + ra->ra_count <= MAXBSIZE))
#endif
	{
		faultcode_t fault_error;
		struct vnode *rvp;

		/*
		 * fix for vnode aliasing
		 */
		if (VOP_REALVP(vp, &rvp) !=0)
			rvp = vp;
		*rdmap = segmap_getmap(segkmap, vp,
		    (u_int)(ra->ra_offset & MAXBMASK));
		*rddata = *rdmap + offset;
		*rdcount = MIN(va.va_size - ra->ra_offset, ra->ra_count);
		/*
		 * Fault in and lock down the pages.
		 */
		fault_error = as_fault(&kas, *rddata, (u_int)*rdcount,
				       F_SOFTLOCK, S_READ);
		if (fault_error == 0) {
			VN_HOLD(vp);
			*rdvp = vp;
			if (IS_V2(req))
				vattr_to_nattr(&va, &rr->rr_attr);
			else if (IS_ESV(req))
				error = get_esv_attrs(vp, &crr->rr_attr,
						svc_getrpccaller(req->rq_xprt));
			goto done;
		} else {
			if (FC_CODE(fault_error) == FC_OBJERR)
				error = FC_ERRNO(fault_error);
			else
				error = EIO;
			NFSLOG(0x40, "rfs_read: map failed, error = %d\n", error, 0);
			(void) segmap_release(segkmap, *rdmap, 0);
			/* Fall through and try just doing a read */
		}
	}
	*rdmap = NULL;

	/*
	 * Allocate space for data.  This will be freed by xdr_rdresult
	 * when it is called with x_op = XDR_FREE.
	 */
	*rddata = kmem_alloc((u_int)ra->ra_count, KM_SLEEP);

	/*
	 * Set up io vector
	 */
	iov.iov_base = *rddata;
	iov.iov_len = ra->ra_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = ra->ra_offset;
	uio.uio_resid = ra->ra_count;
	uio.uio_fmode |= FNDELAY;
	/*
	 * For now we assume no append mode and
	 * ignore totcount (read ahead)
	 */
	error = VOP_READ(vp, &uio, IO_SYNC, u.u_cred);
	if (error) {
		goto bad;
	} else {
		/*
		 * Get attributes again so we can send the latest access
		 * time to the client side for his cache.
		 */
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(vp, &va, 0, u.u_cred);
			if (error)
				goto bad;
			vattr_to_nattr(&va, &rr->rr_attr);
		} else if (IS_ESV(req))
			error = get_esv_attrs(vp, &crr->rr_attr,
					      svc_getrpccaller(req->rq_xprt));
			if (error)
				goto bad;
	}
	*rdcount = ra->ra_count - uio.uio_resid;
	/*
	 * free the unused part of the data allocated
	 */
	if (uio.uio_resid) {
		savedatap = *rddata;
		*rddata = kmem_alloc ((u_int)*rdcount, KM_SLEEP);
		bcopy (savedatap, *rddata, *rdcount);
		kmem_free(savedatap, (u_int)ra->ra_count);
	}
bad:
	if (error && *rddata != NULL) {
		kmem_free(*rddata, (u_int)ra->ra_count);
		*rddata = NULL;
		*rdcount = 0;
	}
done:
	if (opened)
		closerr = VOP_CLOSE(vp, FREAD, 1, 0, u.u_cred);
	else
		closerr = 0;
	if (!error)
		error = closerr;
	rr->rr_status = puterrno(error);
	if (vp->v_type == VREG)
		VOP_RWUNLOCK(vp);
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_read returning %d, count = %d\n",
	    error, *rdcount);
}

/*
 * Free data allocated by rfs_read.
 */
STATIC void
rfs_rdfree(rr)
	struct nfsrdresult *rr;
{
	if (rr->rr_map == NULL && rr->rr_data != NULL) {
		kmem_free(rr->rr_data, (u_int)rr->rr_count);
	}
}

/*
 * Free data allocated by rfs_read - extended protocol
 */
STATIC void
rfs_esvrdfree(rr)
	struct nfsesvrdresult *rr;
{
	if (rr->rr_map == NULL && rr->rr_data != NULL) {
		kmem_free(rr->rr_data, (u_int)rr->rr_count);
	}
}

/*
 * Write data to file.
 * Returns attributes of a file after writing some data to it.
 */
STATIC void
rfs_write(wa, res, exi, req)
	struct nfswriteargs *wa;
	caddr_t *res;
	register struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfsattrstat *ns = (struct nfsattrstat *)res;
	struct nfsesvattrstat *cns = (struct nfsesvattrstat *)res;
	register int error, closerr;
	register struct vnode *vp;
	struct vattr va;
	struct iovec iov;
	struct uio uio;
	int opened;
	struct vnode *avp;	/* addressable */

	opened = 0;
	NFSLOG(0x10, "rfs_write: %d bytes fh %x ",
		wa->wa_count, wa->wa_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d",
		wa->wa_fhandle.fh_fsid.val[1], wa->wa_fhandle.fh_len);
	NFSLOG(0x10, ": %s prog\n", printnfsprog(req), 0);
	vp = fhtovp(&wa->wa_fhandle, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req) || (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;
	} else if (vp->v_type != VREG) {
		NFSLOG(1, "rfs_write: attempt to write to non-file\n", 0, 0);
		error = EISDIR;
	} else {
		VOP_RWLOCK(vp);
		va.va_mask = AT_UID;	/* all we need is the uid */
		error = VOP_GETATTR(vp, &va, 0, u.u_cred);
	}
	if (!error) {
		/*
		 * need MAC write access
		 */
		error = MAC_VACCESS(vp, VWRITE, u.u_cred);
		if (!error && u.u_cred->cr_uid != va.va_uid) {
			/*
			 * This is a kludge to allow writes of files created
			 * with read only permission.  The owner of the file
			 * is always allowed to write it.
			 */
			error = VOP_ACCESS(vp, VWRITE, 0, u.u_cred);
		}
		if (!error) {
			avp = vp;
			error = VOP_OPEN(&avp, FWRITE, u.u_cred);
			vp = avp;
		}
		if (!error) {
			opened = 1;
			if (wa->wa_data) {
				iov.iov_base = wa->wa_data;
				iov.iov_len = wa->wa_count;
				uio.uio_iov = &iov;
				uio.uio_iovcnt = 1;
				uio.uio_segflg = UIO_SYSSPACE;
				uio.uio_offset = wa->wa_offset;
				uio.uio_resid = wa->wa_count;
				/*
				 * We really need a mechanism for the client to
				 * tell the server its current file size limit.
				 * The NFS protocol doesn't provide one, so all
				 * we can use is the current ulimit.
				 */
				uio.uio_limit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
				uio.uio_fmode |= FNDELAY;
				/*
				 * for now we assume no append mode
				 * note: dupreq cache processing is done at the
				 * dispatch level.
				 */
				error = VOP_WRITE(vp, &uio, IO_SYNC, u.u_cred);

				/*
				 * Catch partial writes - if uio.uio_resid > 0:
				 * If there is still room in the filesystem and
				 * we're at the rlimit then return EFBIG. If the
				 * filesystem is out of space return ENOSPC.
				 *
				 * This points out another protocol problem:
				 * there is no way for the server to tell the
				 * client that a write partially succeeded. It's
				 * all or nothing, and we may leave extraneous
				 * data lying around (the partial write).
				 */
				if (uio.uio_resid > 0) {
					struct statvfs svfs;
					int terr;

					terr = VFS_STATVFS(vp->v_vfsp, &svfs);
					if (terr == 0 &&
					    svfs.f_bfree > 0 &&
					    uio.uio_offset == u.u_rlimit[RLIMIT_FSIZE].rlim_cur)
						error = EFBIG;
					else
						error = ENOSPC;
				}
			}
		}
	}

	if (opened)
		closerr = VOP_CLOSE(vp, FWRITE, 1, 0, u.u_cred);
	else
		closerr = 0;

	if (!error)
		error = closerr;

	if (IS_V2(req)) {
		if (!error) {
			/*
			 * Get attributes again so we send the latest mod
			 * time to the client side for his cache.
			 */
			va.va_mask = AT_ALL;	/* now we want everything */
			error = VOP_GETATTR(vp, &va, 0, u.u_cred);
		}
		ns->ns_status = puterrno(error);
		if (!error) {
			vattr_to_nattr(&va, &ns->ns_attr);
		}
	} else if (IS_ESV(req)) {
		if (!error) {
			error = get_esv_attrs(vp, &cns->ns_attr,
					      svc_getrpccaller(req->rq_xprt));
		}
		cns->ns_status = puterrno(error);
	}
	if (!rdonly(exi, req) && !(vp->v_vfsp->vfs_flag & VFS_RDONLY) &&
	    (vp->v_type == VREG))
		VOP_RWUNLOCK(vp);
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_write: returning %d\n", error, 0);
}

/*
 * Create a file.
 * Creates a file with given attributes and returns those attributes
 * and an fhandle for the new file.
 */
STATIC void
rfs_create(args, res, exi, req)
	caddr_t *args;
	caddr_t *res;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfscreatargs *arg = (struct nfscreatargs *)args;
	struct nfsesvcreatargs *carg = (struct nfsesvcreatargs *)args;
	struct nfsdiropres *dr = (struct nfsdiropres *)res;
	struct nfsesvdiropres *cdr = (struct nfsesvdiropres *)res;
	/*
	 * note: arg->ca_da and carg->ca_da are equivalent, as are
	 * (dr->dr_status, cdr->dr_status) and (dr->dr_fhandle, cdr->dr_fhandle)
	 */
	register int error;
	struct vattr va;
	struct vnode *vp;
	register struct vnode *dvp;
	register char *name = arg->ca_da.da_name;
	struct acl *aclp;
	u_int nacl, dacl;
	lid_t tmplid;

	NFSLOG(0x10, "rfs_create: %s dfh %x ",
		name, arg->ca_da.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d",
		arg->ca_da.da_fhandle.fh_fsid.val[1], arg->ca_da.da_fhandle.fh_len);
	NFSLOG(0x10, ": %s prog\n", printnfsprog(req), 0);
	/*
	 *	Disallow NULL paths
	 */
	if (name == (char *) NULL || (*name == '\0')) {
		dr->dr_status = NFSERR_ACCES;
		return;
	}

	if (IS_V2(req))
		sattr_to_vattr(&arg->ca_sa, &va);
	else {
		nacl = acl_getmax();
		aclp = (struct acl *)kmem_alloc(nacl*sizeof(struct acl), KM_SLEEP);
		esvsattr_to_vattr(&carg->ca_sa, &va, &tmplid, aclp, &nacl);
	}
	/*
	 * This is a completely gross hack to make mknod
	 * work over the wire until we can wack the protocol
	 */
#define IFMT		0170000		/* type of file */
#define IFCHR		0020000		/* character special */
#define IFBLK		0060000		/* block special */
#define	IFSOCK		0140000		/* socket */
	if ((va.va_mode & IFMT) == IFCHR) {
		va.va_type = VCHR;
		if (va.va_size == (u_long)NFS_FIFO_DEV)
			va.va_type = VFIFO;	/* xtra kludge for named pipe */
		else
			va.va_rdev = (dev_t)va.va_size;
		va.va_size = 0;
	} else if ((va.va_mode & IFMT) == IFBLK) {
		va.va_type = VBLK;
		va.va_rdev = (dev_t)va.va_size;
		va.va_size = 0;
#ifndef	SYSV
	/*
	 *	System V doesn't believe in other file systems with other
	 *	file types.  Fix this.
	 *	XXX
	 */
	} else if ((va.va_mode & IFMT) == IFSOCK) {
		va.va_type = VSOCK;
#endif
	} else {
		va.va_type = VREG;
	}
	va.va_mode &= ~IFMT;

	/*
	 * XXX - Should get exclusive flag and use it.
	 */
	dvp = fhtovp(&arg->ca_da.da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		if (IS_ESV(req))
			kmem_free(aclp, acl_getmax() * sizeof(struct acl));
		return;
	}
	if (rdonly(exi, req) || (dvp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;
	} else {
		/*
		 * make sure user has
		 * MAC write access to directory.
		 */
		error = MAC_VACCESS(dvp, VWRITE, u.u_cred);
		/*
		 * level of file to be created (calling process level)
		 * must be dominated by the file system level ceiling
		 * of the parent directory and dominate the floor of
		 * the file system, unless process has P_FSYSRANGE privilege
		 * for now, we are not dealing with a race condition that
		 * may result from a MAC domination check (sleep)
		 */
		if (!error)
			if ((MAC_ACCESS(MACDOM, dvp->v_vfsp->vfs_macceiling, u.u_cred->cr_lid) ||
			     (MAC_ACCESS(MACDOM,u.u_cred->cr_lid, dvp->v_vfsp->vfs_macfloor))) &&
			    pm_denied(u.u_cred, P_FSYSRANGE))
				error= ERANGE;

		/*
		 * Very strange. If we are going to trunc the file
		 * (va_size == 0) we check the cache first to be sure
		 * this is not a delayed retransmission, otherwise we
		 * do the create and if it fails check the cache to see
		 * if it was a retransmission.
		 * XXX this really points out a protocol bug:
		 * The server should always do exclusive create.
		 *
		 * This has changed. Now we don't even get here if this is a
		 * duplicate request - this request should be processed.
		 */
		if (!error) {
			/* set va_mask type and mode before create */
			va.va_mask |= AT_TYPE|AT_MODE;
			error = VOP_CREATE(dvp, name,
			    &va, NONEXCL, VWRITE, &vp, u.u_cred);
			if (!error && IS_ESV(req)) {
				if (carg->ca_sa.sa_sens != (s_token)0)
					(void)nfsrv_setlevel(vp, tmplid, u.u_cred);
				if (carg->ca_sa.sa_acl != (s_token)0)
					NFSRV_SETACL(vp, nacl, dacl,
						     (vp->v_type == VDIR) ? 1:0,
						     aclp, u.u_cred);
			}
		}
	}
	if (IS_ESV(req))
		kmem_free(aclp, acl_getmax() * sizeof (struct acl));
	if (!error) {
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(vp, &va, 0, u.u_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
				error = makefh(&dr->dr_fhandle, vp, exi);
			}
		} else if (IS_ESV(req)) {
			error = get_esv_attrs(vp, &cdr->dr_attr,
					      svc_getrpccaller(req->rq_xprt));
			if (!error)
				error = makefh(&cdr->dr_fhandle, vp, exi);
		}
		VN_RELE(vp);
	}
	dr->dr_status = puterrno(error);
	VN_RELE(dvp);
	NFSLOG(0x20, "rfs_create: returning %d\n", error, 0);
}

/*
 * Remove a file.
 * Remove named file from parent directory.
 */
STATIC void
rfs_remove(da, status, exi, req)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *dvp;
	struct vnode *vp;
	lid_t vlid;

	NFSLOG(0x10, "rfs_remove %s dfh %x ",
		da->da_name, da->da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d\n",
		da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);
	/*
	 *	Disallow NULL paths
	 */
	if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
		*status = NFSERR_ACCES;
		return;
	}

	dvp = fhtovp(&da->da_fhandle, exi);
	if (dvp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	/* Get the target vnode for MAC checks */
	error = VOP_LOOKUP(dvp, da->da_name, &vp, (struct pathname *)NULL, 0,
			   (struct vnode *)NULL, u.u_cred);
	if (error) {
		goto rmout;
	}
	if (!(vp->v_macflag & VMAC_SUPPORT))
		NFSRV_GETLID(vp, u.u_cred);
	vlid = vp->v_lid;
	VN_RELE(vp);
	/*
	 * Must have MAC write access to the vnode's parent directory
	 * and be dominated by level on file (to prevent covert channel).
	 */
	if ((error = MAC_VACCESS(dvp, VWRITE, u.u_cred)) == 0 &&
	    (error = MAC_ACCESS(MACDOM, vlid, u.u_cred->cr_lid))) {
		if (!pm_denied(u.u_cred, P_COMPAT) ||
		    !pm_denied(u.u_cred, P_MACWRITE))
			error = 0;
	}
	if (error)
		goto rmout;
	if (rdonly(exi, req) || (dvp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;
	} else
		error = VOP_REMOVE(dvp, da->da_name, u.u_cred);
rmout:
	*status = puterrno(error);
	VN_RELE(dvp);
	NFSLOG(0x20, "rfs_remove: %s returning %d\n", da->da_name, error);
}

/*
 * rename a file
 * Give a file (from) a new name (to).
 */
STATIC void
rfs_rename(args, status, exi, req)
	struct nfsrnmargs *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *fromvp;
	register struct vnode *tovp;
	struct vnode *vp;

	NFSLOG(0x10, "rfs_rename %s ffh %x ",
	    args->rna_from.da_name, args->rna_from.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d -> ",
	    args->rna_from.da_fhandle.fh_fsid.val[1],
	    args->rna_from.da_fhandle.fh_len);
	NFSLOG(0x10, "%s tfh %x ",
	    args->rna_to.da_name, args->rna_to.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d\n",
	    args->rna_to.da_fhandle.fh_fsid.val[1],
	    args->rna_to.da_fhandle.fh_len);
	/*
	 *	Disallow NULL paths
	 */
	if ((args->rna_from.da_name == (char *) NULL) ||
	    (*args->rna_from.da_name == '\0') ||
	    (args->rna_to.da_name == (char *) NULL) ||
	    (*args->rna_to.da_name == '\0')) {
		*status = NFSERR_ACCES;
		return;
	}

	fromvp = fhtovp(&args->rna_from.da_fhandle, exi);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;
		NFSLOG(0x20, "rfs_rename: returning %d\n", *status, 0);
		return;
	}
	tovp = fhtovp(&args->rna_to.da_fhandle, exi);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);
		NFSLOG(0x20, "rfs_rename: returning %d\n", *status, 0);
		return;
	}
	if (rdonly(exi, req) || (fromvp->v_vfsp->vfs_flag & VFS_RDONLY) ||
	    (tovp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;
		goto toerr;
	}
	if (error = VOP_LOOKUP(fromvp, args->rna_from.da_name, &vp,
			       (struct pathname *)NULL, 0, (struct vnode *)NULL,
			       u.u_cred)) {
		goto toerr;
	}
	if (!(vp->v_macflag & VMAC_SUPPORT))
		NFSRV_GETLID(vp, u.u_cred);
	/*
	 * Must have MAC write access to both the source's parent directory
	 * and the target's parent directory.  If the source file itself
	 * is a directory, MAC write access is required on it as well
	 * since inum for ".." will change.
	 */
	if ((error = MAC_VACCESS(fromvp, VWRITE, u.u_cred)) == 0 &&
	    (error = MAC_VACCESS(tovp, VWRITE, u.u_cred)) == 0) {
		if (vp->v_type == VDIR)
			error = MAC_VACCESS(vp, VWRITE, u.u_cred);
	}
	VN_RELE(vp);

	if (!error)
		error = VOP_RENAME(fromvp, args->rna_from.da_name,
				   tovp, args->rna_to.da_name, u.u_cred);
toerr:
	VN_RELE(tovp);
	VN_RELE(fromvp);
	*status = puterrno(error);
	NFSLOG(0x20, "rfs_rename: returning %d\n", error, 0);
}

/*
 * Link to a file.
 * Create a file (to) which is a hard link to the given file (from).
 */
STATIC void
rfs_link(args, status, exi, req)
	struct nfslinkargs *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *fromvp;
	register struct vnode *tovp;

	NFSLOG(0x10, "rfs_link ffh %x %x ",
		args->la_from.fh_fsid.val[0], args->la_from.fh_fsid.val[1]);
	NFSLOG(0x10, "%d -> %s ", args->la_from.fh_len, args->la_to.da_name);
	NFSLOG(0x10, "tfh %x %x ",
		args->la_to.da_fhandle.fh_fsid.val[0],
		args->la_to.da_fhandle.fh_fsid.val[1]);
	NFSLOG(0x10, "%d\n", args->la_to.da_fhandle.fh_len, 0);
	/*
	 *	Disallow NULL paths
	 */
	if ((args->la_to.da_name == (char *) NULL) ||
	    (*args->la_to.da_name == '\0')) {
		*status = NFSERR_ACCES;
		return;
	}

	fromvp = fhtovp(&args->la_from, exi);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;
		NFSLOG(0x20, "rfs_link: returning %d\n", *status, 0);
		return;
	}
	tovp = fhtovp(&args->la_to.da_fhandle, exi);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);
		NFSLOG(0x20, "rfs_link: returning %d\n", *status, 0);
		return;
	}
	/*
	 * Must have MAC write access to both the source file and target
	 * directory.  MAC write access is required on the source because
	 * the link count in its inode will change.
	 */
	if ((error = MAC_VACCESS(fromvp, VWRITE, u.u_cred)) == 0)
		error = MAC_VACCESS(tovp, VWRITE, u.u_cred);
	if (error)
		goto linkerr;

	if (rdonly(exi, req) || (fromvp->v_vfsp->vfs_flag & VFS_RDONLY) ||
	    (tovp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;
	} else
		error = VOP_LINK(tovp, fromvp, args->la_to.da_name, u.u_cred);
linkerr:
	*status = puterrno(error);
	VN_RELE(fromvp);
	VN_RELE(tovp);
	NFSLOG(0x20, "rfs_link: returning %d\n", error, 0);
}

/*
 * Symbolicly link to a file.
 * Create a file (to) with the given attributes which is a symbolic link
 * to the given path name (to).
 */
STATIC void
rfs_symlink(args, status, exi, req)
	caddr_t *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfsslargs *arg = (struct nfsslargs *)args;
	struct nfsesvslargs *carg = (struct nfsesvslargs *)args;
	/*
	 * note: arg->sla_from and carg->sla_from are equivalent, as are
	 * arg->sla_tnm and carg->sla_tnm.
	 */
	int error;
	struct vattr va;
	register struct vnode *vp;
	struct acl *aclp;
	u_int nacl, dacl;
	lid_t tmplid;

	NFSLOG(0x10, "rfs_symlink %s ffh %x ",
		arg->sla_from.da_name,
		arg->sla_from.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d -> ",
		arg->sla_from.da_fhandle.fh_fsid.val[1],
		arg->sla_from.da_fhandle.fh_len);
	NFSLOG(0x10, "%s: %s prog\n", arg->sla_tnm, printnfsprog(req));
	/*
	 *	Disallow NULL paths
	 */
	if ((arg->sla_from.da_name == (char *) NULL) ||
	    (*arg->sla_from.da_name == '\0')) {
		*status = NFSERR_ACCES;
		return;
	}

	if (IS_V2(req))
		sattr_to_vattr(&arg->sla_sa, &va);
	else {
		nacl = acl_getmax();
		aclp = (struct acl *)kmem_alloc(nacl*sizeof(struct acl), KM_SLEEP);
		esvsattr_to_vattr(&carg->sla_sa, &va, &tmplid, aclp, &nacl);
	}
	va.va_type = VLNK;
	va.va_mask |= (AT_TYPE|AT_MODE);
	vp = fhtovp(&arg->sla_from.da_fhandle, exi);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		if (IS_ESV(req))
			kmem_free(aclp, acl_getmax() * sizeof(struct acl));
		NFSLOG(0x20, "rfs_symlink: returning %d\n",geterrno(*status),0);
		return;
	}
	/*
	 * need MAC write access to parent directory
	 */
	error = MAC_VACCESS(vp, VWRITE, u.u_cred);
	if (!error && (rdonly(exi, req) || (vp->v_vfsp->vfs_flag & VFS_RDONLY)))
		error = EROFS;
	if (error)
		goto slerr;

	error = VOP_SYMLINK(vp, arg->sla_from.da_name,
	    &va, arg->sla_tnm, u.u_cred);

	if (IS_ESV(req)) {
		struct vnode *slvp;

		if (VOP_LOOKUP(vp, arg->sla_from.da_name, &slvp,
			       (struct pathname *)NULL, 0,
			       (struct vnode *)NULL, u.u_cred) == 0) {
		
			if (carg->sla_sa.sa_sens != (s_token)0)
				(void)nfsrv_setlevel(slvp, tmplid, u.u_cred);
			if (carg->sla_sa.sa_acl != (s_token)0)
				NFSRV_SETACL(vp, nacl, dacl, 0, aclp, u.u_cred);
			VN_RELE(slvp);
		}
	}
slerr:
	if (IS_ESV(req))
		kmem_free(aclp, acl_getmax() * sizeof(struct acl));
	*status = puterrno(error);
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_symlink: returning %d\n", error, 0);
}

/*
 * Make a directory.
 * Create a directory with the given name, parent directory, and attributes.
 * Returns a file handle and attributes for the new directory.
 */
STATIC void
rfs_mkdir(args, res, exi, req)
	caddr_t *args;
	caddr_t *res;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfscreatargs *arg = (struct nfscreatargs *)args;
	struct nfsesvcreatargs *carg = (struct nfsesvcreatargs *)args;
	struct nfsdiropres *dr = (struct nfsdiropres *)res;
	struct nfsesvdiropres *cdr = (struct nfsesvdiropres *)res;
	/*
	 * note: the following pairs are equivalent:
	 * arg->ca_da and carg->ca_da,  dr->dr_status and cdr->dr_status,
	 * dr->dr_fhandle and cdr->dr_fhandle.
	 */
	int error;
	struct vattr va;
	register struct vnode *dvp;
	struct vnode *vp;
	register char *name = arg->ca_da.da_name;
	struct acl *aclp;
	u_int nacl, dacl;
	lid_t tmplid;

	NFSLOG(0x10, "rfs_mkdir %s ffh %x",
		name, arg->ca_da.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d",
		arg->ca_da.da_fhandle.fh_fsid.val[1], arg->ca_da.da_fhandle.fh_len);
	NFSLOG(0x10, ": %s prog\n", printnfsprog(req), 0);
	/*
	 *	Disallow NULL paths
	 */
	if ((name == (char *) NULL) || (*name == '\0')) {
		dr->dr_status = NFSERR_ACCES;
		return;
	}

	if (IS_V2(req))
		sattr_to_vattr(&arg->ca_sa, &va);
	else {
		nacl = acl_getmax();
		aclp = (struct acl *)kmem_alloc(nacl*sizeof(struct acl), KM_SLEEP);
		esvsattr_to_vattr(&carg->ca_sa, &va, &tmplid, aclp, &nacl);
	}

	va.va_type = VDIR;
	/*
	 * Should get exclusive flag and pass it on here
	 */
	dvp = fhtovp(&arg->ca_da.da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		if (IS_ESV(req))
			kmem_free(aclp, acl_getmax() * sizeof(struct acl));
		return;
	}

	/*
	 * Perform same MAC checks as rfs_create():
	 * 1) MAC write access to directory.
	 */
	error = MAC_VACCESS(dvp, VWRITE, u.u_cred);
	/*
	 * 2) level of file to be created (calling process level)
	 * must be dominated by the file system level ceiling
	 * of the parent directory and dominate the floor of
	 * the file system, unless process has P_FSYSRANGE privilege
	 * for now, we are not dealing with a race condition that
	 * may result from a MAC domination check (sleep)
	 */
	if (!error)
		if ((MAC_ACCESS(MACDOM, dvp->v_vfsp->vfs_macceiling, u.u_cred->cr_lid) ||
		     MAC_ACCESS(MACDOM, u.u_cred->cr_lid, dvp->v_vfsp->vfs_macfloor)) &&
		    pm_denied(u.u_cred, P_FSYSRANGE))
			error= ERANGE;

	if (!error && (rdonly(exi, req) || (dvp->v_vfsp->vfs_flag&VFS_RDONLY)))
		error = EROFS;
	if (error)
		goto mkdirerr;

	/* set vattr mask bits before mkdir */
	va.va_mask |= AT_TYPE|AT_MODE;

	if (IS_ESV(req) && SA_TSTMLD(&carg->ca_sa)) {
		if (pm_denied(u.u_cred, P_MULTIDIR))
			error = EPERM;
		else {
			va.va_size = 0;
			va.va_mask &= ~AT_SIZE;
			error = VOP_MAKEMLD(dvp, name, &va, &vp, u.u_cred);
			if (error == ENOSYS)
				error = EINVAL;
		}
	} else
		error = VOP_MKDIR(dvp, name, &va, &vp, u.u_cred);

	if (!error && IS_ESV(req)) {
		if (carg->ca_sa.sa_sens != (s_token)0)
			(void)nfsrv_setlevel(vp, tmplid, u.u_cred);
		if (carg->ca_sa.sa_acl != (s_token)0)
			NFSRV_SETACL(vp, nacl, dacl, 1, aclp, u.u_cred);
	}
	if (!error) {
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(vp, &va, 0, u.u_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
			}
		} else if (IS_ESV(req)) {
			error = get_esv_attrs(vp, &cdr->dr_attr,
					      svc_getrpccaller(req->rq_xprt));
		}
		if (!error)
			error = makefh(&dr->dr_fhandle, vp, exi);
		VN_RELE(vp);
	}
mkdirerr:
	if (IS_ESV(req))
		kmem_free(aclp, acl_getmax() * sizeof(struct acl));
	dr->dr_status = puterrno(error);
	VN_RELE(dvp);
	NFSLOG(0x20, "rfs_mkdir: returning %d\n", error, 0);
}

/*
 * Remove a directory.
 * Remove the given directory name from the given parent directory.
 */
STATIC void
rfs_rmdir(da, status, exi, req)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	struct vnode *vp;
	register struct vnode *dvp;
	lid_t vlid;

	NFSLOG(0x10, "rfs_rmdir %s fh %x ",
		da->da_name, da->da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x10, "%x %d\n",
		da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);
	/*
	 *	Disallow NULL paths
	 */
	if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
		*status = NFSERR_ACCES;
		return;
	}

	dvp = fhtovp(&da->da_fhandle, exi);
	if (dvp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	/* Get the target vnode for MAC checks */
	error = VOP_LOOKUP(dvp, da->da_name, &vp, (struct pathname *)NULL, 0,
			   (struct vnode *)NULL, u.u_cred);
	if (error) {
		goto rmdout;
	}
	if (!(vp->v_macflag & VMAC_SUPPORT))
		NFSRV_GETLID(vp, u.u_cred);
	vlid = vp->v_lid;
	VN_RELE(vp);
	/*
	 * Must have MAC write access to the vnode's parent directory
	 * and be dominated by level on file (to prevent covert channel).
	 */
	if ((error = MAC_VACCESS(dvp, VWRITE, u.u_cred)) == 0 &&
	    (error = MAC_ACCESS(MACDOM, vlid, u.u_cred->cr_lid))) {
		if (!pm_denied(u.u_cred, P_COMPAT) ||
		    !pm_denied(u.u_cred, P_MACWRITE))
			error = 0;
	}
	if (error)
		goto rmdout;
	if (rdonly(exi, req) || (dvp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;
	} else {
		/*
		 *	VOP_RMDIR now takes a new third argument (the current
		 *	directory of the process).  That's because someone
		 *	wants to return EINVAL if one tries to remove ".".
		 *	Of course, NFS servers have no idea what their
		 *	clients' current directories are.  We fake it by
		 *	supplying a vnode known to exist and illegal to
		 *	remove.
		 */
		error = VOP_RMDIR(dvp, da->da_name, rootdir, u.u_cred);
		if (error == NFSERR_EXIST) {	
			/* kludge for incompatible errnos */
			error = NFSERR_NOTEMPTY;
		}
	}
rmdout:
	*status = puterrno(error);
	VN_RELE(dvp);
	NFSLOG(0x20, "rfs_rmdir returning %d\n", error, 0);
}

/*ARGSUSED*/
STATIC void
rfs_readdir(rda, res, exi, req)
	struct nfsrddirargs *rda;
	caddr_t *res;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfsrddirres *rd = (struct nfsrddirres *)res;
	struct nfsesvrddirres *crd = (struct nfsesvrddirres *)res;
	/*
	 * rd->rd_status and cdr->rd_status, rd->rd_bufsize and crd->rd_bufsize,
	 * and rd->rd_origreqsize and crd->rd_origreqsize are equivalent.
	 */
	int error, closerr;
	int iseof;
	struct iovec iov;
	struct uio uio;
	register struct vnode *vp;
	int opened;
	struct vnode *avp;	/* addressable */

	opened = 0;
	NFSLOG(0x10, "rfs_readdir fh %x %x ",
		rda->rda_fh.fh_fsid.val[0], rda->rda_fh.fh_fsid.val[1]);
	NFSLOG(0x10, "%d count %d", rda->rda_fh.fh_len, rda->rda_count);
	NFSLOG(0x10, ": %s prog\n", printnfsprog(req), 0);
	vp = fhtovp(&rda->rda_fh, exi);
	if (vp == NULL) {
		rd->rd_status = NFSERR_STALE;
		return;
	}
	VOP_RWLOCK(vp);
	if (vp->v_type != VDIR) {
		NFSLOG(1, "rfs_readdir: attempt to read non-directory\n", 0, 0);
		error = ENOTDIR;
		goto bad;
	}
	/*
	 * check read access of dir.  we have to do this here because
	 * the opendir doesn't go over the wire.
	 */
	error = MAC_VACCESS(vp, VREAD, u.u_cred);
	if (!error)
		error = VOP_ACCESS(vp, VREAD, 0, u.u_cred);
	if (error) {
		goto bad;
	}

	avp = vp;
	error = VOP_OPEN(&avp, FREAD, u.u_cred);
	vp = avp;
	if (error) {
		goto bad;
	}
	opened = 1;

	if (rda->rda_count == 0) {
		if (IS_V2(req)) {
			rd->rd_size = 0;
			rd->rd_eof = FALSE;
			rd->rd_entries = NULL;
			rd->rd_bufsize = 0;
		} else if (IS_ESV(req)) {
			crd->rd_size = 0;
			crd->rd_eof = FALSE;
			crd->rd_entries = NULL;
			crd->rd_bufsize = 0;
		}
		goto bad;
	}

	rda->rda_count = MIN(rda->rda_count, NFS_MAXDATA);

	/*
	 * Allocate data for entries.  This will be freed by rfs_rdfree.
	 */
	rd->rd_bufsize = rda->rda_count;
	rd->rd_origreqsize = rda->rda_count;
	if (IS_V2(req)) {
		rd->rd_entries = (struct dirent *)kmem_alloc((u_int)rda->rda_count, KM_SLEEP);
	} else if (IS_ESV(req)) {
		crd->rd_entries = (struct dirent *)kmem_alloc((u_int)rda->rda_count, KM_SLEEP);
	}

	/*
	 * Set up io vector to read directory data
	 */
	iov.iov_base = (IS_V2(req) ? (caddr_t)rd->rd_entries : (caddr_t)crd->rd_entries);
	iov.iov_len = rda->rda_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = rda->rda_offset;
	uio.uio_resid = rda->rda_count;

	/*
	 * read directory
	 */
	error = VOP_READDIR(vp, &uio, u.u_cred, &iseof);

	/*
	 * Clean up
	 */
	if (error) {
		if (IS_V2(req))
			rd->rd_size = 0;
		else if (IS_ESV(req))
			crd->rd_size = 0;
		goto bad;
	}

	/* if ESV protocol return attributes */
	if (IS_ESV(req))
		error = get_esv_attrs(vp, &crd->rd_attr,
				      svc_getrpccaller(req->rq_xprt));
	/*
	 * set size and eof
	 */
	if (rda->rda_count && uio.uio_resid == rda->rda_count) {
		if (IS_V2(req)) {
			rd->rd_size = 0;
			rd->rd_eof = TRUE;
		} else if (IS_ESV(req)) {
			crd->rd_size = 0;
			crd->rd_eof = TRUE;
		}
	} else {
		if (IS_V2(req)) {
			rd->rd_size = rda->rda_count - uio.uio_resid;
			if (iseof)
				rd->rd_eof = TRUE;
			else
				rd->rd_eof = FALSE;
		} else if (IS_ESV(req)) {
			crd->rd_size = rda->rda_count - uio.uio_resid;
			if (iseof)
				crd->rd_eof = TRUE;
			else
				crd->rd_eof = FALSE;
		}
	}

bad:
	if (opened)
		closerr = VOP_CLOSE(vp, FREAD, 1, 0, u.u_cred);
	else
		closerr = 0;
	if (!error)
		error = closerr;
	rd->rd_status = puterrno(error);
	VOP_RWUNLOCK(vp);
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_readdir: returning %d\n", error, 0);
}

STATIC void
rfs_rddirfree(rd)
	struct nfsrddirres *rd;
{
	kmem_free((caddr_t)rd->rd_entries, (u_int)rd->rd_bufsize);
}

STATIC void
rfs_esvrddirfree(rd)
	struct nfsesvrddirres *rd;
{
	kmem_free((caddr_t)rd->rd_entries, (u_int)rd->rd_bufsize);
}

/*ARGSUSED*/
STATIC void
rfs_statfs(fh, fs, exi, req)
	fhandle_t *fh;
	register struct nfsstatfs *fs;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	struct statvfs sb;
	register struct vnode *vp;

	NFSLOG(0x10, "rfs_statfs fh %x %x ",
	    fh->fh_fsid.val[0], fh->fh_fsid.val[1]);
	NFSLOG(0x10, "%d\n", fh->fh_len, 0);
	vp = fhtovp(fh, exi);
	if (vp == NULL) {
		fs->fs_status = NFSERR_STALE;
		return;
	}
	error = VFS_STATVFS(vp->v_vfsp, &sb);
	/*
	 * If the level of the calling process does not dominate the
	 * file system level ceiling, zero out blocks free and files
	 * free to prevent a covert channel.  If the process has
	 * P_FSYSRANGE or P_COMPAT, don't bother.
	 */
	if (MAC_ACCESS(MACDOM, u.u_cred->cr_lid, vp->v_vfsp->vfs_macceiling) &&
	    pm_denied(u.u_cred, P_FSYSRANGE) &&
	    pm_denied(u.u_cred, P_COMPAT)) {
		sb.f_bfree = 0;
		sb.f_ffree = 0;
	}
	fs->fs_status = puterrno(error);
	if (!error) {
		fs->fs_tsize = nfstsize();
		fs->fs_bsize = sb.f_frsize;
		fs->fs_blocks = sb.f_blocks;
		fs->fs_bfree = sb.f_bfree;
		fs->fs_bavail = sb.f_bavail;
	}
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_statfs returning %d\n", error, 0);
}

/*
 * rfs_access: Determine if the requested access will be allowed for this
 * client to this file (extended (ESV) protocol only).
 */
STATIC void
rfs_access(acca, accr, exi, req)
	register struct nfsaccessargs *acca;
	register struct nfsaccessres *accr;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	int mode = 0;
	register struct vnode *vp;

	NFSLOG(0x10, "rfs_access fh %x %x ",
		acca->acc_fhandle.fh_fsid.val[0], acca->acc_fhandle.fh_fsid.val[1]);
	NFSLOG(0x10, "%d\n", acca->acc_fhandle.fh_len, 0);
	
	vp = fhtovp(&acca->acc_fhandle, exi);
	if (vp == NULL) {
		accr->acc_status = NFSERR_STALE;
		return;
	}

	if (acca->acc_flag & (ACCESS_READ))
		mode |= VREAD;
	if (acca->acc_flag & (ACCESS_WRITE|ACCESS_APPEND))
		mode |= VWRITE;
	if (acca->acc_flag & (ACCESS_EXEC|ACCESS_SEARCH))
		mode |= VEXEC;

#ifdef DEBUG
	NFSLOG(0x40, "access type %d: ", acca->acc_flag, 0);
	if (mode&VREAD)
		NFSLOG(0x40, "read ", 0, 0);
	if (mode&VWRITE)
		NFSLOG(0x40, "write ", 0, 0);
	if (mode&VEXEC)
		NFSLOG(0x40, "exec ", 0, 0);
	NFSLOG(0x40, "\n", 0, 0);
#endif

	error = MAC_VACCESS(vp, mode, u.u_cred);
	if (!error)
		error = VOP_ACCESS(vp, mode, 0, u.u_cred);

	if (error)
		accr->acc_stat = FALSE;
	else
		accr->acc_stat = TRUE;
	if (!error)
		error = get_esv_attrs(vp, &accr->acc_attr,
				      svc_getrpccaller(req->rq_xprt));
	accr->acc_status = puterrno(error);
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_access returning %d, access %s\n", error,
		(accr->acc_stat) ? "granted" : "denied");
}

/*ARGSUSED*/
STATIC void
rfs_null(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	/* do nothing */
}

/*ARGSUSED*/
STATIC void
rfs_error(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	/* return (EOPNOTSUPP); */
}

STATIC void
nullfree()
{
}

/*ARGSUSED*/
STATIC void
rfs_success(argp, resp)
	caddr_t	*argp;
	caddr_t	*resp;
{
	*(int *)resp = 0;	/* put a success indicator in the status */
				/* field of an NFS response struct */
}

/*
 * rfs_diropres: recreate the response for a previously successful
 * create or mkdir operation. Create a diropres struct (or esvdiropres struct)
 * with the created file or directory fhandle and its attributes.
 */
/*ARGSUSED*/
STATIC void
rfs_diropres(argp, resp, exi, req)
	caddr_t	*argp;
	caddr_t	*resp;
	struct exportinfo *exi;
	struct svc_req *req;
{
	struct nfscreatargs *arg = (struct nfscreatargs *)argp;
	struct nfsdiropres *dr = (struct nfsdiropres *)resp;
	struct nfsesvdiropres *cdr = (struct nfsesvdiropres *)resp;
	fhandle_t *fhp = &arg->ca_da.da_fhandle;
	struct vnode *vp, *cvp;
	int error;
	struct vattr va;
	NFSLOG(0x10, "rfs_diropres fh %x %x ",
		fhp->fh_fsid.val[0], fhp->fh_fsid.val[1]);
	NFSLOG(0x10, "%d: %s prog\n", fhp->fh_len, printnfsprog(req));
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		if (IS_V2(req))
			dr->dr_status = NFSERR_STALE;
		else if (IS_ESV(req))
			cdr->dr_status = NFSERR_STALE;
		return;
	}
	error = VOP_LOOKUP(vp, arg->ca_da.da_name, &cvp,
			   (struct pathname *)NULL, 0, (struct vnode *)NULL,
			   u.u_cred);
	if (!error) {
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(cvp, &va, 0, u.u_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
				error = makefh(&dr->dr_fhandle, cvp, exi);
			}
		} else if (IS_ESV(req)) {
			error = get_esv_attrs(cvp, &cdr->dr_attr,
					     svc_getrpccaller(req->rq_xprt));
			if (!error)
				error = makefh(&cdr->dr_fhandle, cvp, exi);
		}
		VN_RELE(cvp);
	} else {
		/* interesting. We successfully created this before. */
		/* Let's try the original create again */
		if (req->rq_proc == RFS_CREATE)
			rfs_create(argp, resp, exi, req);
		else
			rfs_mkdir(argp, resp, exi, req);
		return;
	}
	dr->dr_status = puterrno(error);
	VN_RELE(vp);
	NFSLOG(0x20, "rfs_diropres: returning %d\n", error, 0);
}

/* debugging only */
STATIC char rfscallnames[][20] = {
	"RFS_NULL",	"RFS_GETATTR",	"RFS_SETATTR",	"RFS_ROOT",
	"RFS_LOOKUP",	"RFS_READLINK",	"RFS_READ",	"RFS_WRITECACHE",
	"RFS_WRITE",	"RFS_CREATE",	"REMOVE",	"RFS_RENAME",
	"RFS_LINK",	"RFS_SYMLINK",	"RFS_MKDIR",	"RFS_RMDIR",
	"RFS_READDIR",	"RFS_STATFS",	"RFS_ACCESS"
};

/*
 * rfs dispatch table
 * Indexed by version, proc
 */

STATIC struct rfsdisp {
	void	  (*dis_proc)();	/* proc to call */
	xdrproc_t dis_xdrargs;		/* xdr routine to get args */
	int	  dis_argsz;		/* sizeof args */
	xdrproc_t dis_xdrres;		/* xdr routine to put results */
	int	  dis_ressz;		/* size of results */
	void	  (*dis_resfree)();	/* frees space allocated by proc */
	void	  (*dis_reply)();	/* proc to call to just send a reply */
} rfsdisptab[][RFS_NPROC]  = {
	{
	/*
	 * VERSION 2
	 * Changed rddirres to have eof at end instead of beginning
	 * NOTE: some of the reply procs make use of the fact that the
	 *       getattr args consist of only a fhandle.
	 */
	/* RFS_NULL = 0 */
	{rfs_null, xdr_void, 0,
	    xdr_void, 0, nullfree, rfs_null},
	/* RFS_GETATTR = 1 */
	{rfs_getattr, xdr_fhandle, sizeof (fhandle_t),
	    xdr_attrstat, sizeof (struct nfsattrstat), nullfree, rfs_getattr},
	/* RFS_SETATTR = 2 */
	{rfs_setattr, xdr_saargs, sizeof (struct nfssaargs),
	    xdr_attrstat, sizeof (struct nfsattrstat), nullfree, rfs_getattr},
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree, rfs_error},
	/* RFS_LOOKUP = 4 */
	{rfs_lookup, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_diropres, sizeof (struct nfsdiropres), nullfree, rfs_lookup},
	/* RFS_READLINK = 5 */
	{rfs_readlink, xdr_fhandle, sizeof (fhandle_t),
	    xdr_rdlnres, sizeof (struct nfsrdlnres), rfs_rlfree, rfs_readlink},
	/* RFS_READ = 6 */
	{rfs_read, xdr_readargs, sizeof (struct nfsreadargs),
	    xdr_rdresult, sizeof (struct nfsrdresult), rfs_rdfree, rfs_read},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree, rfs_error},
	/* RFS_WRITE = 8 */
	{rfs_write, xdr_writeargs, sizeof (struct nfswriteargs),
	    xdr_attrstat, sizeof (struct nfsattrstat), nullfree, rfs_getattr},
	/* RFS_CREATE = 9 */
	{rfs_create, xdr_creatargs, sizeof (struct nfscreatargs),
	    xdr_diropres, sizeof (struct nfsdiropres), nullfree, rfs_diropres},
	/* RFS_REMOVE = 10 */
	{rfs_remove, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_RENAME = 11 */
	{rfs_rename, xdr_rnmargs, sizeof (struct nfsrnmargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_LINK = 12 */
	{rfs_link, xdr_linkargs, sizeof (struct nfslinkargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_SYMLINK = 13 */
	{rfs_symlink, xdr_slargs, sizeof (struct nfsslargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_MKDIR = 14 */
	{rfs_mkdir, xdr_creatargs, sizeof (struct nfscreatargs),
	    xdr_diropres, sizeof (struct nfsdiropres), nullfree, rfs_diropres},
	/* RFS_RMDIR = 15 */
	{rfs_rmdir, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_READDIR = 16 */
	{rfs_readdir, xdr_rddirargs, sizeof (struct nfsrddirargs),
	    xdr_putrddirres, sizeof (struct nfsrddirres), rfs_rddirfree, rfs_readdir},
	/* RFS_STATFS = 17 */
	{rfs_statfs, xdr_fhandle, sizeof (fhandle_t),
	    xdr_statfs, sizeof (struct nfsstatfs), nullfree, rfs_statfs},
	}
};

STATIC struct rfsdisp rfsdisptab_esv[][RFS_ESVNPROC]  = {
	{
	/*
	 * PROGRAM NFS_ESVPROG, VERSION 1
	 * Similar to v.2, but new arguments/results structures
	 */
	/* RFS_NULL = 0 */
	{rfs_null, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_GETATTR = 1 */
	{rfs_getattr, xdr_fhandle, sizeof (fhandle_t),
	    xdr_esvattrstat, sizeof (struct nfsesvattrstat), nullfree, rfs_getattr},
	/* RFS_SETATTR = 2 */
	{rfs_setattr, xdr_esvsaargs, sizeof (struct nfsesvsaargs),
	    xdr_esvattrstat, sizeof (struct nfsesvattrstat), nullfree, rfs_getattr},
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree, rfs_error},
	/* RFS_LOOKUP = 4 */
	{rfs_lookup, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_esvdiropres, sizeof (struct nfsesvdiropres), nullfree, rfs_lookup},
	/* RFS_READLINK = 5 */
	{rfs_readlink, xdr_fhandle, sizeof (fhandle_t),
	    xdr_esvrdlnres, sizeof (struct nfsesvrdlnres), rfs_rlfree, rfs_readlink},
	/* RFS_READ = 6 */
	{rfs_read, xdr_readargs, sizeof (struct nfsreadargs),
	    xdr_esvrdresult, sizeof (struct nfsesvrdresult), rfs_esvrdfree, rfs_read},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree, rfs_error},
	/* RFS_WRITE = 8 */
	{rfs_write, xdr_writeargs, sizeof (struct nfswriteargs),
	    xdr_esvattrstat, sizeof (struct nfsesvattrstat), nullfree, rfs_getattr},
	/* RFS_CREATE = 9 */
	{rfs_create, xdr_esvcreatargs, sizeof (struct nfsesvcreatargs),
	    xdr_esvdiropres, sizeof (struct nfsesvdiropres), nullfree, rfs_diropres},
	/* RFS_REMOVE = 10 */
	{rfs_remove, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_RENAME = 11 */
	{rfs_rename, xdr_rnmargs, sizeof (struct nfsrnmargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_LINK = 12 */
	{rfs_link, xdr_linkargs, sizeof (struct nfslinkargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_SYMLINK = 13 */
	{rfs_symlink, xdr_esvslargs, sizeof (struct nfsesvslargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_MKDIR = 14 */
	{rfs_mkdir, xdr_esvcreatargs, sizeof (struct nfsesvcreatargs),
	    xdr_esvdiropres, sizeof (struct nfsesvdiropres), nullfree, rfs_diropres},
	/* RFS_RMDIR = 15 */
	{rfs_rmdir, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_READDIR = 16 */
	{rfs_readdir, xdr_rddirargs, sizeof (struct nfsrddirargs),
	    xdr_esvputrddirres, sizeof (struct nfsesvrddirres), rfs_esvrddirfree, rfs_readdir},
	/* RFS_STATFS = 17 */
	{rfs_statfs, xdr_fhandle, sizeof (fhandle_t),
	    xdr_statfs, sizeof (struct nfsstatfs), nullfree, rfs_statfs},
	/* RFS_ACCESS = 18 */
	{rfs_access, xdr_accessargs, sizeof (struct nfsaccessargs),
	    xdr_accessres, sizeof (struct nfsaccessres), nullfree, rfs_access}
	}
};

STATIC caddr_t
rfsget()
{
	int i;
	struct rfsdisp *dis;
	caddr_t ret;

	if (rfssize == 0) {
		for (i = 0; i < 1 + VERSIONMAX - VERSIONMIN; i++) {
			for (dis = &rfsdisptab[i][0];
			    dis < &rfsdisptab[i][RFS_NPROC];
			    dis++) {
				rfssize = MAX(rfssize, dis->dis_argsz);
				rfssize = MAX(rfssize, dis->dis_ressz);
			}
		}
	}

	if (rfsfreesp) {
		ret = (caddr_t)rfsfreesp;
		rfsfreesp = rfsfreesp->rs_next;
	} else {
		ret = kmem_alloc((u_int)rfssize, KM_SLEEP);
	}
	return (ret);
}

STATIC void
rfsput(rs)
	struct rfsspace *rs;
{

	rs->rs_next = rfsfreesp;
	rfsfreesp = rs;
}

/*
 *	If nfs_portmon is set, then clients are required to use privileged
 *	ports (ports < IPPORT_RESERVED) in order to get NFS services.
 *
 *	N.B.:  this attempt to carry forward the already ill-conceived
 *	notion of privileged ports for TCP/UDP is really quite ineffectual.
 *	Not only is it transport-dependent, it's laughably easy to spoof.
 *	If you're really interested in security, you must start with secure
 *	RPC instead.
 */
int nfs_portmon = 0;

STATIC void
rfs_dispatch(req, xprt)
	struct svc_req *req;
	register SVCXPRT *xprt;
{
	int prog;
	int vers;
	int which;
	caddr_t	args = NULL;
	caddr_t	res = NULL;
	register struct rfsdisp *disp = (struct rfsdisp *) NULL;
	struct cred *tmpcr;
	struct cred *newcr = NULL;
	int error;
	struct exportinfo *exi;

	svstat.ncalls++;
	error = 0;
	prog = req->rq_prog;
	vers = req->rq_vers;
	which = req->rq_proc;
	if (prog == NFS_PROGRAM) {
		if (vers < VERSIONMIN || vers > VERSIONMAX) {
			NFSLOG(1, "rfs_dispatch: bad vers %d low %d high ",
			    vers, VERSIONMIN);
			NFSLOG(1, "%d\n", VERSIONMAX, 0);
			svcerr_progvers(req->rq_xprt, (u_long)VERSIONMIN,
			    (u_long)VERSIONMAX);
			error++;
			cmn_err(CE_CONT, "nfs_server: bad version number\n");
			goto done;
		} else {
			vers -= VERSIONMIN;
			if (which < 0 || which >= RFS_NPROC) {
				NFSLOG(1, "rfs_dispatch: bad proc %d\n", which, 0);
				svcerr_noproc(req->rq_xprt);
				error++;
				cmn_err(CE_CONT, "nfs_server: bad proc number\n");
				goto done;
			}
			disp = &rfsdisptab[vers][which];
		}
	} else if (prog == NFS_ESVPROG) {
		if (vers != NFS_ESVVERS) {
			NFSLOG(1, "rfs_dispatch: bad vers %d low %d high ",
			    vers, NFS_ESVVERS);
			NFSLOG(1, "%d\n", NFS_ESVVERS, 0);
			svcerr_progvers(req->rq_xprt, (u_long)NFS_ESVVERS,
			    (u_long)NFS_ESVVERS);
			error++;
			cmn_err(CE_CONT, "nfs_server: bad version number\n");
			goto done;
		} else {
			vers -= ESVVERSIONMIN;
			if (which < 0 || which >= RFS_ESVNPROC) {
				NFSLOG(1, "rfs_dispatch: bad proc %d\n", which, 0);
				svcerr_noproc(req->rq_xprt);
				error++;
				cmn_err(CE_CONT, "nfs_server: bad proc number\n");
				goto done;
			}
			disp = &rfsdisptab_esv[vers][which];
		}
	} else {
		cmn_err(CE_PANIC, "nfs_server: bad program number\n");
	}

	/*
	 * Allocate args struct and deserialize into it.
	 */
	args = rfsget();
	bzero(args, (u_int)rfssize);
	if ( ! SVC_GETARGS(xprt, disp->dis_xdrargs, args)) {
		svcerr_decode(xprt);
		error++;
		cmn_err(CE_CONT, "nfs_server: bad getargs\n");
		goto done;
	}

	/*
	 * Find export information and check authentication,
	 * setting the credential if everything is ok.
	 */
	if (which != RFS_NULL) {
		/*
		 * XXX: this isn't really quite correct. Instead of doing
		 * this blind cast, we should extract out the fhandle for
		 * each NFS call. What's more, some procedures (like rename)
		 * have more than one fhandle passed in, and we should check
		 * that the two fhandles point to the same exported path.
		 */
		/* LINTED pointer alignment */
		fhandle_t *fh = (fhandle_t *) args;

		newcr = crget();
		tmpcr = u.u_cred;
		u.u_cred = newcr;
		/* LINTED pointer alignment */
		exi = findexport(&fh->fh_fsid, (struct fid *) &fh->fh_xlen);
		if (exi != NULL && !checkauth(exi, req, newcr)) {
			svcerr_weakauth(xprt);
			error++;
			/*
			 * No error message if we're not MAC and the request
			 * was - that is expected. Just reject the request.
			 */
			if (mac_installed || req->rq_cred.oa_flavor != AUTH_ESV)
				cmn_err(CE_CONT, "nfs_server: weak authentication\n");
			goto done;
		}
	}

	/*
	 * Allocate results struct.
	 */
	res = rfsget();
	bzero(res, (u_int)rfssize);

	svstat.reqs[which]++;

	NFSLOG(0x40, "rfs_dispatch: %s (prog %s)",
	       rfscallnames[which], ((prog == NFS_ESVPROG) ? "NFS_ESVPROG" : "NFS_PROGRAM"));
	if (which == RFS_WRITE)
		/* LINTED pointer alignment */
		NFSLOG(0x40, ": offset %x, count %x\n",
			((struct nfswriteargs *) args)->wa_offset,
			((struct nfswriteargs *) args)->wa_count);
	else
		NFSLOG(0x40, "\n", 0, 0);
	/*
	 */
	/*
	 * Duplicate request cache processing:
	 * 1) First see if we can throw this request away untouched (this is
	 *    already done in svc_getreq() in the KRPC code).
	 * 2) Second see if we only need to respond w/o retrying the operation
	 *    Note that only successful completions will allow this, so the
	 *    status will always be 0.
	 * 3) Record the start of the operation
	 * 4) Call service routine with arg struct and results struct
	 * 5) Record the result status of the operation
	 */
	if (svc_clts_kdup_reply(req)) {
		NFSLOG(0x40, "rfs_dispatch: duplicate request proc %d, sending reply\n", req->rq_proc, 0);
		(*disp->dis_reply)(args, res, exi, req);
	} else {
		svc_clts_kdupstart(req);
		(*disp->dis_proc)(args, res, exi, req);
		/* LINTED pointer alignment */
		svc_clts_kdupend(req, *(int *)res);
	}

done:
	if (disp) {
		/*
		 * Free arguments struct
		 */
		if (!SVC_FREEARGS(xprt, disp->dis_xdrargs, args) ) {
			cmn_err(CE_CONT, "nfs_server: bad freeargs\n");
			error++;
		}
	}
	if (args != NULL) {
		/* LINTED pointer alignment */
		rfsput((struct rfsspace *)args);
	}

	/*
	 * Serialize and send results struct
	 */
	if (!error) {
		if (!svc_sendreply(xprt, disp->dis_xdrres, res)) {
			cmn_err(CE_CONT, "nfs_server: bad sendreply\n");
			error++;
		}
	}

	/*
	 * Free results struct
	 */
	if (res != NULL) {
		if ( disp->dis_resfree != nullfree ) {
			(*disp->dis_resfree)(res);
		}
		/* LINTED pointer alignment */
		rfsput((struct rfsspace *)res);
	}
	/*
	 * restore original credentials
	 */
	if (newcr) {
		u.u_cred = tmpcr;
		crfree(newcr);
	}
	svstat.nbadcalls += error;
}

void
sattr_to_vattr(sa, vap)
	register struct nfssattr *sa;
	register struct vattr *vap;
{
	vap->va_mask = 0;
	vap->va_mode = sa->sa_mode;
	vap->va_uid = sa->sa_uid;
	vap->va_gid = sa->sa_gid;
	vap->va_size = sa->sa_size;
	if ((sa->sa_mtime.tv_sec != (u_long)-1) &&
	    (sa->sa_mtime.tv_usec == 1000000)) {
		vap->va_mtime.tv_sec  = hrestime.tv_sec;
		vap->va_mtime.tv_nsec = hrestime.tv_nsec;
		vap->va_atime.tv_sec  = vap->va_mtime.tv_sec;
		vap->va_atime.tv_nsec = vap->va_mtime.tv_nsec;
	} else {
		vap->va_atime.tv_sec  = sa->sa_atime.tv_sec;
		vap->va_atime.tv_nsec = sa->sa_atime.tv_usec*1000;
		vap->va_mtime.tv_sec  = sa->sa_mtime.tv_sec;
		vap->va_mtime.tv_nsec = sa->sa_mtime.tv_usec*1000;
	}
	if (vap->va_mode != (mode_t) -1)
		vap->va_mask |= AT_MODE;
	if (vap->va_uid != (uid_t) -1)
		vap->va_mask |= AT_UID;
	if (vap->va_gid != (uid_t) -1)
		vap->va_mask |= AT_GID;
	if (vap->va_size != (ulong) -1)
		vap->va_mask |= AT_SIZE;
	if (vap->va_atime.tv_sec != (unsigned long) -1)
		vap->va_mask |= AT_ATIME;
	if (vap->va_mtime.tv_sec != (unsigned long) -1)
		vap->va_mask |= AT_MTIME;
}

void
esvsattr_to_vattr(sa, vap, lidp, aclp, nacl)
	register struct nfsesvsattr *sa;
	register lid_t *lidp;
	register struct vattr *vap;
	register struct acl *aclp;	/* ptr to buffer to write ACL entries */
	u_int *nacl;			/* Number of ACL entries that will fit in aclp.
					 * Will be overwritten with the number of
					 * ACL entries actually copied.
					 */
{
	vap->va_mask = 0;
	vap->va_mode = sa->sa_mode;
	vap->va_uid = sa->sa_uid;
	vap->va_gid = sa->sa_gid;
	vap->va_size = sa->sa_size;
	if ((sa->sa_mtime.tv_sec != (u_long)-1) &&
	    (sa->sa_mtime.tv_usec == 1000000)) {
		vap->va_mtime.tv_sec  = hrestime.tv_sec;
		vap->va_mtime.tv_nsec = hrestime.tv_nsec;
		vap->va_atime.tv_sec  = vap->va_mtime.tv_sec;
		vap->va_atime.tv_nsec = vap->va_mtime.tv_nsec;
	} else {
		vap->va_atime.tv_sec  = sa->sa_atime.tv_sec;
		vap->va_atime.tv_nsec = sa->sa_atime.tv_usec*1000;
		vap->va_mtime.tv_sec  = sa->sa_mtime.tv_sec;
		vap->va_mtime.tv_nsec = sa->sa_mtime.tv_usec*1000;
	}
	if (vap->va_mode != (mode_t) -1)
		vap->va_mask |= AT_MODE;
	if (vap->va_uid != (uid_t) -1)
		vap->va_mask |= AT_UID;
	if (vap->va_gid != (uid_t) -1)
		vap->va_mask |= AT_GID;
	if (vap->va_size != (ulong) -1)
		vap->va_mask |= AT_SIZE;
	if (vap->va_atime.tv_sec != (unsigned long) -1)
		vap->va_mask |= AT_ATIME;
	if (vap->va_mtime.tv_sec != (unsigned long) -1)
		vap->va_mask |= AT_MTIME;

	if (map_local_token(sa->sa_sens, SENS_T,
			    (caddr_t)lidp, sizeof(lid_t)) != sizeof (lid_t))
		*lidp = (lid_t)0;
	*nacl = map_local_token(sa->sa_acl, ACL_T,
				(caddr_t)aclp, *nacl * sizeof(struct acl));
	*nacl /= sizeof(struct acl);
}

/*
 * Convert an fhandle into a vnode.
 * Uses the file id (fh_len + fh_data) in the fhandle to get the vnode.
 * WARNING: users of this routine must do a VN_RELE on the vnode when they
 * are done with it.
 */
STATIC struct vnode *
fhtovp(fh, exi)
	fhandle_t *fh;
	struct exportinfo *exi;
{
	register struct vfs *vfsp;
	struct vnode *vp;
	int error;

	if (exi == NULL) {
		return (NULL);	/* not exported */
	}
	vfsp = getvfs(&fh->fh_fsid);
	if (vfsp == NULL) {
		return (NULL);
	}
	/* LINTED pointer alignment */
	error = VFS_VGET(vfsp, &vp, (struct fid *)&(fh->fh_len));
	if (error || vp == NULL) {
		NFSLOG(1, "fhtovp(%x) couldn't vget\n", fh, 0);
		return (NULL);
	}
	if (!(vp->v_macflag & VMAC_SUPPORT))
		NFSRV_GETLID(vp, u.u_cred);

	NFSLOG(0x40, "fhtovp: fh %x, new vp lid %d\n", fh, vp->v_lid);
	return (vp);
}

/*
 *	Determine whether two addresses are equal.
 *
 *	This is not as easy as it seems, since netbufs are opaque addresses
 *	and we're really concerned whether the host parts of the addresses
 *	are equal.  The solution is to check the supplied mask, whose address
 *	bits are 1 if we should compare the corresponding bits in addr1 and
 *	addr2, and 0 otherwise.
 */
int
eqaddr(addr1, addr2, mask)
	struct netbuf *addr1;
	struct netbuf *addr2;
	struct netbuf *mask;
{
	register char *a1, *a2, *m, *mend;

	if (addr1 == NULL || addr2 == NULL || mask == NULL)
		return (0);

	if ((addr1->len != addr2->len) || (addr1->len != mask->len))
		return (0);

	for (a1 = addr1->buf,
	     a2 = addr2->buf,
	     m = mask->buf,
	     mend = mask->buf + mask->len; m < mend; a1++, a2++, m++)
		if (((*a1) & (*m)) != ((*a2) & (*m)))
			return (0);
	return (1);
}

STATIC int
hostinlist(na, addrs)
	struct netbuf *na;
	struct exaddrlist *addrs;
{
	int i;

	for (i = 0; i < addrs->naddrs; i++) {
		if (eqaddr(na, &addrs->addrvec[i], &addrs->addrmask[i])) {
			return (1);
		}
	}
	return (0);
}

/*
 * Check to see if the given name corresponds to a
 * root user of the exported filesystem.
 */
STATIC int
rootname(ex, netname)
	struct export *ex;
	char *netname;
{
	int i;
	int namelen;

	namelen = strlen(netname) + 1;
	for (i = 0; i < ex->ex_des.nnames; i++) {
		if (bcmp(netname, ex->ex_des.rootnames[i], namelen) == 0) {
			return (1);
		}
	}
	return (0);
}

STATIC int
checkauth(exi, req, cred)
	struct exportinfo *exi;
	struct svc_req *req;
	struct cred *cred;
{
	struct authunix_parms *aup;
	struct authdes_cred *adc;
	struct authesv_parms *acp;
	int flavor;
	short grouplen;

	/*
	 *      Check for privileged port number
	 *      N.B.:  this assumes that we know the format of a netbuf.
	 */
	if (nfs_portmon) {
		 /* LINTED pointer alignment */
		 struct sockaddr *ca = (struct sockaddr *) svc_getrpccaller(req->rq_xprt)->buf;

		 if ((ca->sa_family == AF_INET) &&
		     (ntohs(((struct sockaddr_in *) ca)->sin_port) >= IPPORT_RESERVED)) {
			cmn_err(CE_CONT, "NFS request from unprivileged port.\n");
			return (0);
		}
	}

	/*
	 * Set uid, gid, and gids to auth params
	 */
	flavor = req->rq_cred.oa_flavor;
	if (flavor != exi->exi_export.ex_auth) {
		/*
		 * Allow AUTH_UNIX to an AUTH_ESV filesystem.
		 * NOTE: AUTH_UNIX will not be used for any exports in SVR4.1.
		 * (AUTH_ESV and AUTH_DES will be the only two possible.)
		 */
		if (!(flavor == AUTH_UNIX && exi->exi_export.ex_auth == AUTH_ESV))
		flavor = AUTH_NULL;
	}
	switch (flavor) {
	case AUTH_NULL:
		cred->cr_wkgpriv = cred->cr_maxpriv = (pvec_t)0;
		if (mac_installed)
			applynfslp(svc_getrpccaller(req->rq_xprt), cred, 1);
		cred->cr_uid = exi->exi_export.ex_anon;
		cred->cr_gid = exi->exi_export.ex_anon;
		cred->cr_ruid = exi->exi_export.ex_anon;
		cred->cr_rgid = exi->exi_export.ex_anon;
		cred->cr_flags &= ~CR_MLDREAL;
		break;

	case AUTH_UNIX:	/* NOTE: export flavor is AUTH_ESV */
		/* LINTED pointer alignment */
		aup = (struct authunix_parms *)req->rq_clntcred;
		cred->cr_wkgpriv = cred->cr_maxpriv = (pvec_t)0;
		if (mac_installed)
			applynfslp(svc_getrpccaller(req->rq_xprt), cred, 1);
		if (aup->aup_uid == 0 &&
		    !hostinlist(svc_getrpccaller(req->rq_xprt),
				&exi->exi_export.ex_esv.esvrootaddrs)) {
			cred->cr_uid = exi->exi_export.ex_anon;
			cred->cr_gid = exi->exi_export.ex_anon;
			cred->cr_ruid = exi->exi_export.ex_anon;
			cred->cr_rgid = exi->exi_export.ex_anon;
			cred->cr_ngroups = 0;
		} else {
			cred->cr_uid = aup->aup_uid;
			cred->cr_gid = aup->aup_gid;
			cred->cr_ruid = aup->aup_uid;
			cred->cr_rgid = aup->aup_gid;
			bcopy((caddr_t)aup->aup_gids,
			      (caddr_t)cred->cr_groups,
			      aup->aup_len*sizeof(cred->cr_groups[0]));
			cred->cr_ngroups = aup->aup_len;
			if (aup->aup_uid == 0) {
				pm_setbits(P_ALLPRIVS, cred->cr_wkgpriv);
				cred->cr_maxpriv = cred->cr_wkgpriv;
			}
		}
		cred->cr_flags &= ~CR_MLDREAL;
		break;

	case AUTH_DES:
		/* LINTED pointer alignment */
		adc = (struct authdes_cred *)req->rq_clntcred;
		cred->cr_wkgpriv = cred->cr_maxpriv = (pvec_t)0;
		if (mac_installed)
			applynfslp(svc_getrpccaller(req->rq_xprt), cred, 1);
		if (adc->adc_fullname.window > exi->exi_export.ex_des.window) {
			return (0);
		}
		if (!authdes_getucred(adc, &cred->cr_uid, &cred->cr_gid,
		    &grouplen, cred->cr_groups)) {
			if (rootname(&exi->exi_export,
			    adc->adc_fullname.name)) {
				cred->cr_uid = cred->cr_ruid = 0;
			} else {
				cred->cr_uid = cred->cr_ruid = exi->exi_export.ex_anon;
			}
			cred->cr_gid = cred->cr_rgid = exi->exi_export.ex_anon;
			grouplen = 0;
		}
		if ((cred->cr_uid == 0) && !rootname(&exi->exi_export,
		    adc->adc_fullname.name)) {
			cred->cr_uid = cred->cr_gid = exi->exi_export.ex_anon;
			cred->cr_ruid = cred->cr_rgid = exi->exi_export.ex_anon;
			grouplen = 0;
		}
		cred->cr_ngroups = grouplen;
		cred->cr_flags &= ~CR_MLDREAL;
		break;

	case AUTH_ESV:
		/* LINTED pointer alignment */
		acp = (struct authesv_parms *)req->rq_clntcred;
		if (!mac_installed)
			return (0);
		cred->cr_uid = acp->auc_uid;
		cred->cr_gid = acp->auc_gid;
		cred->cr_ruid = acp->auc_uid;
		cred->cr_rgid = acp->auc_gid;
		bcopy((caddr_t)acp->auc_gids, (caddr_t)cred->cr_groups,
		    acp->auc_len * sizeof (cred->cr_groups[0]));
		cred->cr_ngroups = acp->auc_len;
		if (map_local_token(acp->auc_privs, PRIVS_T,
				    (caddr_t)&cred->cr_wkgpriv,
				    sizeof(pvec_t)) == sizeof(pvec_t))
			cred->cr_maxpriv = cred->cr_wkgpriv;
		else
			return (0);
		if (map_local_token(acp->auc_sens, SENS_T,
				    (caddr_t)&cred->cr_lid,
				    sizeof(lid_t)) != sizeof(lid_t))
			return (0);
		applynfslp(svc_getrpccaller(req->rq_xprt), cred, 0);
		cred->cr_flags |= CR_MLDREAL;
		break;

	default:
		return (0);
	}
	return (cred->cr_uid != (uid_t) -1);
}
