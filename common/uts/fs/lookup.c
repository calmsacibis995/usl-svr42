/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)uts-comm:fs/lookup.c	1.20.3.6"
#ident	"$Header: $"

#include <acc/audit/audit.h>	
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/syscall.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>


/*
 * Lookup the user file name,
 * Handle allocation and freeing of pathname buffer, return error.
 */
int
lookupname(fnamep, seg, followlink, dirvpp, compvpp)
	char *fnamep;			/* user pathname */
	enum uio_seg seg;		/* addr space that name is in */
	enum symfollow followlink;	/* follow sym links */
	vnode_t **dirvpp;		/* ret for ptr to parent dir vnode */
	vnode_t **compvpp;		/* ret for ptr to component vnode */
{
	struct pathname lookpn;
	register int error;

	if (error = pn_get(fnamep, seg, &lookpn))
		return error;
	error = lookuppn(&lookpn, followlink, dirvpp, compvpp);
	pn_free(&lookpn);
	return error;
}

/*
 * Function mld_deflect() performs the deflection to the appropriate
 * effective directory.  If the effective directory does not
 * exist, it is created.  The P_MACWRITE privilege allows the
 * creation of an effective directory at a level which is different
 * from the MLD.  The level of the created effective directory must be
 * within file system range, and must dominate the level of the MLD.
 * Note that the latter requires no additional check since search access
 * to the MLD itself has already been done.  Before allowing the deflection,
 * the effective directory must be a directory and must be at the level
 * of the calling process.
 */
int
mld_deflect(vp, eff_dirname, tvpp, pnp, lookup_flags, rdir, crp)
	vnode_t		*vp;		/* current directory vp */
	char		*eff_dirname;	/* ptr to storage for MLD eff dir */
	vnode_t		**tvpp;		/* ptr to addressable temp ptr */
	pathname_t	*pnp;		/* pathname to lookup */
	int		lookup_flags;	/* flags to VOP_LOOKUP */
	vnode_t		*rdir;		/* current root vp */
	cred_t		*crp;		/* credential pointer */
{
	struct vattr	vattr;		/* vattr to get mode of MLD */
	struct vattr	e_vattr;	/* vattr for MLD eff dir */
	struct cred	*lcred;		/* ptr to cred with MAC override */
	int		error	= 0;
	vfs_t		*vfsp	= vp->v_vfsp;
	vnode_t		*tvp	= *tvpp;

	if ((error = VOP_LOOKUP(vp, eff_dirname, &tvp, pnp, lookup_flags, 
		rdir, crp)) == ENOENT 
	&&  (error = VOP_GETATTR(vp, &vattr, 0, crp)) == 0) { 
		if ( MAC_ACCESS(MACEQUAL, vfsp->vfs_macfloor,   crp->cr_lid) 
		&&   MAC_ACCESS(MACEQUAL, vfsp->vfs_macceiling, crp->cr_lid) 
		&&  (MAC_ACCESS(MACDOM,   vfsp->vfs_macceiling, crp->cr_lid) 
		||   MAC_ACCESS(MACDOM,   crp->cr_lid, vfsp->vfs_macfloor  ))
		&&   pm_denied(crp, P_FSYSRANGE) ) {
			error = EACCES; 
		} else { 
			e_vattr.va_mask = AT_TYPE | AT_MODE; 
			e_vattr.va_type = VDIR; 
			e_vattr.va_mode = vattr.va_mode & MODEMASK; 
			lcred = crdup(crp); 
			lcred->cr_wkgpriv |= pm_privbit(P_MACWRITE); 
			error = VOP_MKDIR(vp, eff_dirname, &e_vattr,
				&tvp, lcred); 
			crfree(lcred); 
		} 
	} 
	if (error == 0) { 
		if (!(tvp->v_macflag & VMAC_SUPPORT) 
		&&  tvp->v_vfsp) 
			tvp->v_lid = tvp->v_vfsp->vfs_macfloor; 
		if (tvp->v_type != VDIR) 
			error = ENOTDIR; 
		else if (tvp->v_lid != crp->cr_lid) 
			error = EINVAL; 
		if (error) 
			VN_RELE(tvp); 
	} 
	*tvpp = tvp;
	return error;
}

/*
 * Common audit code used in lookuppn().
 */

/*
 * Check if appending the current pathname component will exceed
 * the size of the audit pathanme buffer.
 */
#define	ADT_CHKSPACE(x)							\
	if (adt_path_alloc) {						\
		if ((adtpath.pn_pathlen + x) >= adt_pathsize) {		\
			char *obuf;					\
			obuf=adtpath.pn_buf;           			\
			adt_pathsize += MAXPATHLEN;			\
			adtpath.pn_buf=(char *)kmem_alloc(adt_pathsize,KM_SLEEP);\
			bcopy(obuf,adtpath.pn_buf,adtpath.pn_pathlen);	\
			tp=adtpath.pn_buf + adtpath.pn_pathlen;		\
			kmem_free(obuf,adtpath.pn_pathlen);		\
		}							\
	}

/*

/*
 * Add '/' to component path.
 */
#define	ADT_ADDSLASH(tp, adtpath)					\
	if (adt_path_alloc) {						\
		*(tp)='/';						\
		ADT_CHKSPACE(1);					\
		(adtpath).pn_pathlen++;					\
		tp++;							\
	}

/*
 * Free allocated audit path.
 */
#define	ADT_FREEPATH(adtpathp)						\
	if (adt_path_alloc) {						\
        	pn_free(&adtpath);					\
	}

/*
 * Record file name.
 */
#define	ADT_RECFN(tp, adtpath, vp)				 	\
	if (adt_path_alloc) {						\
		*tp='\0';						\
		(tp) = (adtpath).pn_buf;				\
		if (adt_auditme(A_OBJCHK,0,(vp)->v_lid))		\
			adt_filenm(tp,(adtpath).pn_pathlen,vp);		\
		pn_free(&(adtpath));					\
	}


/*
 * Add component to audit path.
 */
#define	ADT_ADDCOMP(tp, adtpath, comp)					\
	if (adt_path_alloc) {						\
		*tp='/';						\
		ADT_CHKSPACE(strlen(comp) + 2);				\
		tp++;							\
		bcopy(comp,tp,strlen(comp));				\
		(tp)+=strlen(comp);					\
		*(tp)++='/';						\
		(adtpath).pn_pathlen= adtpath.pn_pathlen + 2 +strlen(comp);\
	}

/*
 * Starting at current directory, translate pathname pnp to end.
 * Leave pathname of final component in pnp, return the vnode
 * for the final component in *compvpp, and return the vnode
 * for the parent of the final component in dirvpp.
 *
 * This is the central routine in pathname translation and handles
 * multiple components in pathnames, separating them at /'s.  It also
 * implements mounted file systems and processes symbolic links.
 */

int
lookuppn(pnp, followlink, dirvpp, compvpp)
	register struct pathname *pnp;	/* pathname to lookup */
	enum symfollow followlink;	/* (don't) follow sym links */
	vnode_t **dirvpp;		/* ptr for parent vnode */
	vnode_t **compvpp;		/* ptr for entry vnode */
{
	register vnode_t *vp;		/* current directory vp */
	register vnode_t *cvp = NULLVP;	/* current component vp */
	vnode_t *tvp;			/* addressable temp ptr */
	char component[MAXNAMELEN];	/* buffer for component (incl null) */
	register int error;
	register int nlink = 0;
	int lookup_flags = 0;
	char eff_dirname[MLD_SZ];	/* storage for MLD eff dir */
	char *tcomp = NULL;		/* ptr to component when MLD */
	char *tp;			/* testing pathname  */
	register tplen;			/* testing pathname length */
	struct pathname adtpath;	/* audit pathname struct   */
	int adt_path_alloc;		/* audit pathname allocated */
	int adt_pathsize=0;		/* audit allocated pathname space */
	vnode_t *rdir = u.u_rdir ? u.u_rdir : rootdir;	/* use this root dir */

	sysinfo.namei++;
	if(dirvpp)
		lookup_flags |= LOOKUP_DIR;
	if(followlink == FOLLOW)
		lookup_flags |= FOLLOW_SYMLINK;

	/*
	 * Allocate pathname structure for Auditing (or not).
	 */
	if (audit_on) {
		adtpath.pn_pathlen = 0;
		pn_alloc(&adtpath);
		tp = adtpath.pn_buf;
		adt_pathsize = MAXPATHLEN; /* space allocated for file name */
		adt_path_alloc = 1;	/* also indicates audit_on */
	} else
		adt_path_alloc = 0;

	/*
	 * Start at current directory.
	 */
	vp = u.u_cdir;
	VN_HOLD(vp);

begin:
	/*
	 * Disallow the empty path name.
	 */
	if (pnp->pn_pathlen == 0) {
		error = ENOENT;
		goto bad;
	}

	/*
	 * Each time we begin a new name interpretation (e.g.
	 * when first called and after each symbolic link is
	 * substituted), we allow the search to start at the
	 * root directory if the name starts with a '/', otherwise
	 * continuing from the current directory.
	 */
	if (pn_peekchar(pnp) == '/') {
		VN_RELE(vp);
		PN_SKIPSLASH(pnp);
		ADT_ADDSLASH(tp, adtpath);
		vp = rdir;
		VN_HOLD(vp);
	}

	/*
	 * Eliminate any trailing slashes in the pathname.
	 */
	pn_fixslash(pnp);

next:
	PREEMPT();

	/*
	 * Make sure we have a directory.
	 */
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}

	/*
	 * Process the next component of the pathname.
	 */
	if (error = pn_stripcomponent(pnp, component))
		goto bad;

	/*
	 * Check for degenerate name (e.g. / or "")
	 * which is a way of talking about a directory,
	 * e.g. "/." or ".".
	 */
	if (component[0] == 0) {
		/*
		 * If the caller was interested in the parent then
		 * return an error since we don't have the real parent.
		 */
		if (dirvpp != NULL) {
			VN_RELE(vp);
			ADT_FREEPATH(&adtpath);
			return EINVAL;
		}
		(void) pn_set(pnp, ".");
		ADT_RECFN(tp, adtpath, vp);
		if (compvpp != NULL) 
			*compvpp = vp;
		else
			VN_RELE(vp);
		return 0;
	}

	if (adt_path_alloc) {
		tplen=strlen(component);
		ADT_CHKSPACE(tplen);
		bcopy(component,tp,tplen);
		adtpath.pn_pathlen+=tplen;
		tp+=tplen;
	}

	/*
	 * Handle "..": two special cases.
	 * 1. If we're at the root directory (e.g. after chroot)
	 *    then ignore ".." so we can't get out of this subtree.
	 * 2. If this vnode is the root of a mounted file system,
	 *    then replace it with the vnode that was mounted on
	      so that we take the ".." in the other file system.
	 */
	if ((component[0] == '.') && (component[1] == '.') 
		&& (component[2] == '\0')) {
checkforroot:
		if (VN_CMP(vp, u.u_rdir) || VN_CMP(vp, rootdir)) {
			cvp = vp;
			VN_HOLD(cvp);
			goto skip;
		}
		if (vp->v_flag & VROOT) {
			cvp = vp;
			vp = vp->v_vfsp->vfs_vnodecovered;
			VN_HOLD(vp);
			VN_RELE(cvp);
			goto checkforroot;
		}
	}

	/*
	 * Determine MAC search access to the current directory.
	 * If access is granted, lookup the next component.
	 */
	if ((error = MAC_VACCESS(vp, VEXEC, u.u_cred)) == 0
	&&  (error = VOP_LOOKUP(vp, component, &tvp, pnp, lookup_flags,
			rdir, u.u_cred)) == 0
	&&  mac_installed) {
		/*
		 * Assign the vnode's level at this time.
		 * If the file system type does not support levels,
		 * get the level from the file system's root vnode.
		 * The root vnode's level is assigned at mount time to
		 * the mount point's level.
		 * Note that the vnode's level is only assigned if it
		 * has not been previously set.
		 */
		if (!(tvp->v_macflag & VMAC_SUPPORT)) {
			/*
			 * tvp's level is assigned the level floor
			 * of the file system level range.
			 * If the file system level range is not
			 * assigned, tvp's level remains unassigned
			 * (remains 0 which is an invalid LID that can only be
			 * overridden with MAC privileges).
			 * Note that the check for non-NULL v_vfsp
			 * is here because this field is not guaranteed to
			 * be filled in by all file system types.
			 * In particular, the mkdir call for rf_lookup
			 * has this field NULLed.
			 */
			if (tvp->v_vfsp)  
				tvp->v_lid = tvp->v_vfsp->vfs_macfloor; 
		}
		/*
		 * MLD deflection if necessary:
		 *	1. target is MLD
		 *	2. virtual mode
		 *	3. avoid . case in MLD itself
		 */
		if ((tvp->v_macflag & VMAC_ISMLD)
		&&  !(u.u_cred->cr_flags & CR_MLDREAL)
		&&  vp != tvp) {
			if (strcmp(component, "..") == 0)
				tcomp = component;
			else {
				fs_itoh(u.u_cred->cr_lid, eff_dirname);
				tcomp = eff_dirname;
			}
			ADT_ADDCOMP(tp, adtpath, tcomp);
			if ((error = MAC_VACCESS(tvp, VEXEC, u.u_cred)) != 0) {
				VN_RELE(tvp);
			} else {
				VN_RELE(vp);
				vp = tvp;
				if (tcomp == component) { /* ".." case */
					/*
					 * Repeat special casing.
					 */
checkforrt:
					if (VN_CMP(vp, u.u_rdir)
					||  VN_CMP(vp, rootdir)) {
						cvp = vp;
						VN_HOLD(cvp);
						goto skip;
					}
					if (vp->v_flag & VROOT) {
						cvp = vp;
						vp = vp->v_vfsp->vfs_vnodecovered;
						VN_HOLD(vp);
						VN_RELE(cvp);
						goto checkforrt;
					}
					error = VOP_LOOKUP(vp, tcomp, &tvp, pnp,
						lookup_flags, rdir, u.u_cred);
					/* set vnode's level if necessary */
					if (error == 0
					&&  !(tvp->v_macflag & VMAC_SUPPORT)
					&&  tvp->v_vfsp)
						tvp->v_lid = tvp->v_vfsp->vfs_macfloor; 
				} else
					error = mld_deflect(vp, eff_dirname,
						&tvp, pnp, lookup_flags, rdir,
						u.u_cred);
			}
		} /* endif check for MLD */
	}

	if (error == 0)
		cvp = tvp;
	else {
		cvp = NULL;
		/*
		 * On error, return hard error if
		 * (a) we're not at the end of the pathname yet, or
		 * (b) the caller didn't want the parent directory, or
		 * (c) we failed for some reason other than a missing entry.
		 */
		if (pn_pathleft(pnp) || dirvpp == NULL || error != ENOENT)
			goto bad;
		pn_setlast(pnp);
		*dirvpp = vp;
		if (compvpp != NULL)
			*compvpp = NULL;
		ADT_RECFN(tp, adtpath, vp);
		return 0;
	}

	/*
	 * Traverse mount points.
	 */
	if (cvp->v_vfsmountedhere != NULL) {
		tvp = cvp;
		if ((error = traverse(&tvp)) != 0)
			goto bad;
		/*
		 * This code assumes that you cannot mount on an
		 * already mounted file system (restriction in
		 * mount() call).  If this changes in the future,
		 * deflection code must be added in traverse
		 * itself.
		 *
		 * Notes:
		 *	1. MAC check is not necessary because root
		 *	   vnode level is equal to mount point level.
		 *	2. no ".." case to deal with on traversal.
		 */
		if (mac_installed
		&& (tvp->v_macflag & VMAC_ISMLD)
		&&  !(u.u_cred->cr_flags & CR_MLDREAL)) {
			fs_itoh(u.u_cred->cr_lid, eff_dirname);
			ADT_ADDCOMP(tp, adtpath, eff_dirname);
			VN_RELE(vp);
			vp = tvp;
			if ((error = mld_deflect(vp, eff_dirname, &tvp, pnp,
				lookup_flags, rdir, u.u_cred)) != 0) {
				cvp = NULL;
				goto bad;
			}
		}
		cvp = tvp;
	}

	/*
	 * If we hit a symbolic link and there is more path to be
	 * translated or this operation does not wish to apply
	 * to a link, then place the contents of the link at the
	 * front of the remaining pathname.
	 */
	if (cvp->v_type == VLNK) { 

		/* Set level of link to that of it's parent */
		cvp->v_lid = vp->v_lid;


		if (followlink == FOLLOW || pn_pathleft(pnp)) {
			struct pathname linkpath;

			if (++nlink > MAXSYMLINKS) {
				error = ELOOP;
				goto bad;
			}
			pn_alloc(&linkpath);
			if (error = pn_getsymlink(cvp, &linkpath, u.u_cred)) {
				pn_free(&linkpath);
				goto bad;
			}
			if (pn_pathleft(&linkpath) == 0)
				(void) pn_set(&linkpath, ".");
			error = pn_insert(pnp, &linkpath);	/* linkpath before pn */
   			if (pn_peekchar(pnp) == '/') {
				if (adt_path_alloc)
                        		tp = adtpath.pn_buf;
			}
			pn_free(&linkpath);
			if (error)
				goto bad;
			VN_RELE(cvp);
			cvp = NULL;
			goto begin;
		}
	}

skip:
	/*
	 * If no more components, return last directory (if wanted) and
	 * last component (if wanted).
	 */
	if (pn_pathleft(pnp) == 0) {
		if (mac_installed
		&& (vp->v_macflag & VMAC_ISMLD)
		&&  !(u.u_cred->cr_flags & CR_MLDREAL)
		&&  tcomp)
			pn_set(pnp, tcomp);
		else
			pn_setlast(pnp);
		if (dirvpp != NULL) {
			/*
			 * Check that we have the real parent and not
			 * an alias of the last component.
			 */
			if (VN_CMP(vp, cvp)) {
				VN_RELE(vp);
				VN_RELE(cvp);
				ADT_FREEPATH(&adtpath);
				return EINVAL;
			}
			*dirvpp = vp;
		} else
			VN_RELE(vp);

		ADT_RECFN(tp, adtpath, cvp);

		if (compvpp != NULL)
			*compvpp = cvp;
		else
			VN_RELE(cvp);
		return 0;
	}

	/*
	 * Skip over slashes from end of last component.
	 */ 
	PN_SKIPSLASH(pnp);

	ADT_ADDSLASH(tp, adtpath);

	/*
	 * Searched through another level of directory:
	 * release previous directory handle and save new (result
	 * of lookup) as current directory.
	 */
	VN_RELE(vp);
	vp = cvp;
	cvp = NULL;
	goto next;

bad:
	/*
	 * Error.  Release vnodes and return.
	 */
	if (cvp)
		VN_RELE(cvp);
	ADT_RECFN(tp, adtpath, vp);
	VN_RELE(vp);
	return error;
}

/*
 * Traverse a mount point.  Routine accepts a vnode pointer as a reference
 * parameter and performs the indirection, releasing the original vnode.
 */
int
traverse(cvpp)
	vnode_t **cvpp;
{
	register struct vfs *vfsp;
	register int error = 0;
	register vnode_t *cvp;
	vnode_t *tvp;

	cvp = *cvpp;

	/*
	 * If this vnode is mounted on, then we transparently indirect
	 * to the vnode which is the root of the mounted file system.
	 * Before we do this we must check that an unmount is not in
	 * progress on this vnode.
	 */
mloop:
	while ((vfsp = cvp->v_vfsmountedhere) != NULL) {
		if (vfsp->vfs_flag & VFS_MLOCK) {
			vfsp->vfs_flag |= VFS_MWAIT;
			if (sleep((caddr_t)vfsp, PVFS|PCATCH))
				return EINTR;
			goto mloop;
		}
		if (error = VFS_ROOT(vfsp, &tvp))
			break;
		VN_RELE(cvp);
		cvp = tvp;
	}

	*cvpp = cvp;
	return error;
}
