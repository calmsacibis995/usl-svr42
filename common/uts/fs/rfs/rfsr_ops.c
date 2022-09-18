/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/rfs/rfsr_ops.c	1.12.3.5"
#ident	"$Header: $"
/*
 * Operations for kernel daemon process that handles requests
 * for file activity from RFSs.
 */

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/list.h>
#include <util/sysinfo.h>
#include <svc/time.h>
#include <fs/rfs/rf_acct.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <svc/systm.h>
#include <fs/mode.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <mem/immu.h>
#include <io/stream.h>
#include <mem/seg.h>
#include <fs/rfs/rf_admin.h>
#include <fs/rfs/rf_messg.h>
#include <fs/rfs/rf_comm.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <fs/dirent.h>
#include <fs/rfs/nserve.h>
#include <fs/rfs/rf_cirmgr.h>
#include <fs/rfs/idtab.h>
#include <util/var.h>
#include <fs/file.h>
#include <fs/fstyp.h>
#include <fs/fcntl.h>
#include <proc/proc.h>
#include <fs/stat.h>
#include <fs/statfs.h>
#include <util/inline.h>
#include <util/debug.h>
#include <fs/rfs/rf_debug.h>
#include <util/cmn_err.h>
#include <io/conf.h>
#include <fs/buf.h>
#include <fs/rfs/rf_adv.h>
#include <io/uio.h>
#include <fs/rfs/rf_vfs.h>
#include <fs/pathname.h>
#include <fs/rfs/hetero.h>
#include <fs/rfs/rf_serve.h>
#include <fs/ustat.h>
#include <fs/statvfs.h>
#include <fs/rfs/rfcl_subr.h>
#include <fs/rfs/rf_auth.h>
#include <fs/fbuf.h>
#include <mem/seg_map.h>
#include <fs/rfs/rf_canon.h>
#include <proc/mman.h>
#include <mem/page.h>
#include <fs/rfs/rf_cache.h>
#include <fs/rfs/du.h>
#include <mem/kmem.h>
#include <fs/fs_subr.h>
#include <io/stropts.h>
#include <acc/mac/mac.h>
#include <acc/dac/acl.h>

STATIC int rfsr_vn_remove();
STATIC int rfsr_mkdir_common();

/* imports */
extern char	*strcpy();
extern int	strlen();

/*
 * Table indexed by opcode points to these functions, which handle
 * remote file requests.
 */

/*
 * VOP_ACCESS
 */
/* ARGSUSED */
STATIC int
rfsr_access(stp, ctrlp)
	register rfsr_state_t *stp;
	register rfsr_ctrl_t *ctrlp;
{
	register rf_request_t *req = RF_REQ(stp->sr_in_bp);
	vnode_t *vp = stp->sr_rdp->rd_vp;
	register int fmode = req->rq_mode_op.fmode;
	int			error	= 0;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);

	/* validate vop */
	if ((error = MAC_VACCESS(vp, fmode, stp->sr_cred)) != 0)
		return error;

	if (fmode & VWRITE && stp->sr_srmp->srm_flags & SRM_RDONLY) {
		return EROFS;
	} else {
		return VOP_ACCESS(vp, fmode, 0, stp->sr_cred);
	}
}

/*
 * VOP_PATHCONF
 */
/* ARGSUSED */
STATIC int
rfsr_pathconf(stp, ctrlp)
	register rfsr_state_t *stp;
	register rfsr_ctrl_t *ctrlp;
{
	register rf_request_t *req = RF_REQ(stp->sr_in_bp);
	vnode_t *vp = stp->sr_rdp->rd_vp;
	register int cmd = req->rq_pathconf.cmd;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	return VOP_PATHCONF(vp, cmd, (u_long *)&stp->sr_ret_val, stp->sr_cred);
}

/*
 * VOP_CLOSE
 */
/* ARGSUSED */
STATIC int
rfsr_close(stp, ctrlp)
	register rfsr_state_t 	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register rcvd_t 	*rdp = stp->sr_rdp;
	register vnode_t 	*vp = rdp->rd_vp;
	int			error = 0;
	register int		fmode = req->rq_close.fmode & (FREAD | FWRITE);
	vattr_t			vattr;

	rfsr_fsinfo.fsivop_close++;
	if (!rdu_modecheck(stp->sr_rdup, fmode)) {
		return rfsr_j_accuse("rfsr_close:  close unmatched with open",
		  stp);
	}

	/* Use file table values of client */

	error = VOP_CLOSE(vp, fmode, req->rq_close.count,
	  req->rq_close.foffset, stp->sr_cred);

	if (req->rq_close.count == 1) {

		/*
		 * Last reference from a file table entry on client
		 * corresponds to last reference for some giving of this
		 * gift; give up corresponding reference in rduser structure.
		 */

		if ((fmode & (FREAD | FWRITE)) == (FREAD | FWRITE)) {
			if (!stp->sr_rdup->ru_frwcnt) {
				return
				  rfsr_discon("rfsr_close: double close", stp);
			}
			--stp->sr_rdup->ru_frwcnt;
		} else if (fmode & FREAD) {
			if (!stp->sr_rdup->ru_frcnt) {
				return
				  rfsr_discon("rfsr_close: double close", stp);
			}
			--stp->sr_rdup->ru_frcnt;
		} else {
			if (!stp->sr_rdup->ru_fwcnt) {
				return
				  rfsr_discon("rfsr_close: double close", stp);
			}
			--stp->sr_rdup->ru_fwcnt;
		}
	}

	/*
	 * 3.2 clients need vcode in return value continue cacheing a file upon
	 * each close.
	 */

	if (!error) {
		vattr.va_mask = AT_VCODE;
		if (!(error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred))) {
			stp->sr_ret_val = vattr.va_vcode;
		}
	}

	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);

	/*
	 * In vnode protocol, clients give up last reference with last close.
	 * Get cache consisitency info now, because later our reference will
	 * be gone.
	 */

	if (stp->sr_vcver > RFS1DOT0 &&
	  req->rq_close.lastclose &&
	  !(error = rfsr_cacheck(stp, u.u_srchan->sd_mntid))) {
		STATIC int rfsr_inactive();

		error = rfsr_inactive(stp, ctrlp);
		*ctrlp = SR_NORMAL;
	} else {
		SR_FREEMSG(stp);
	}
	return error;
}

STATIC int
rfsr_inactive(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register unsigned	vcount;
	register long		mntid = RF_COM(stp->sr_in_bp)->co_mntid;
	register sysid_t	sysid = stp->sr_gdpp->sysid;

	rfsr_fsinfo.fsivop_other++;

	if (stp->sr_vcver == RFS1DOT0) {
		vcount = 1;
	} else {
		vcount = RF_REQ(stp->sr_in_bp)->rq_rele.vcount;
		*ctrlp = SR_NO_RESP;
	}

	SR_FREEMSG(stp);
	if (vcount < 1 || stp->sr_srmp->srm_refcnt < vcount + 1 ||
	  !(stp->sr_rdp->rd_stat & RDUSED)) {
		return rfsr_j_accuse("rfsr_inactive redundant VN_RELE", stp);
	}

	/*
	 * There is no need to allocate response for new clients.  Either
	 * we got in here via a VOP_CLOSE, which already allocated a
	 * response, or we got called via VOP_INACTIVE, in which case no
	 * response is expected by the client.
	 */

	if (stp->sr_vcver < RFS2DOT0) {
		ASSERT(!stp->sr_out_bp);
		stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
		(void)rfsr_cacheck(stp, mntid);
	}
 	stp->sr_srmp->srm_refcnt -= vcount;
	rcvd_delete(&stp->sr_rdp, sysid, mntid, vcount, stp->sr_cred);
	return 0;
}

/*
 * VOP_CREATE
 */
STATIC int
rfsr_create(stp, ctrlp)
	register rfsr_state_t		*stp;
	register rfsr_ctrl_t		*ctrlp;
{
	vnode_t				*dvp = stp->sr_rdp->rd_vp;
	register rf_request_t		*req;
	register int			version = stp->sr_vcver;
	register struct rqmkdent	*rqdp;
	register int			error = 0;
	register size_t			hdrsz = RF_MIN_REQ(version);
	register size_t			datasz;
	int				fmode;
	vattr_t				vattr;
	char				*compname;
	vnode_t				*compvp = NULLVP;

	if (version < RFS2DOT0) {
		return dusr_creat(stp, ctrlp);
	}
	rfsr_fsinfo.fsivop_create++;

	datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	if (datasz <= sizeof(struct vattr)) {
		return rfsr_j_accuse("rfsr_create:  bad data", stp);
	}
	if (RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) {
		return rfsr_j_accuse("rfsr_create bad data", stp);
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY ||
	  stp->sr_srmp->srm_flags & SRM_RDONLY) {
		return EROFS;
	}

	req = RF_REQ(stp->sr_in_bp);
	rqdp = (struct rqmkdent *)rf_msgdata(stp->sr_in_bp, hdrsz);
	if (stp->sr_gdpp->hetero != NO_CONV &&
	  !rf_fcanon(MKDENT_FMT, (caddr_t)rqdp, (caddr_t)rqdp + datasz,
	  (caddr_t)rqdp)) {
		return rfsr_j_accuse("rfsr_create bad data", stp);
	}
	rftov_attr(&vattr, &rqdp->attr);
	vattr.va_atime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	vattr.va_ctime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	vattr.va_mtime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	if (dvp->v_type != VDIR || vattr.va_type == VLNK ||
	  vattr.va_type == VNON || vattr.va_type == VBAD) {
		return rfsr_j_accuse("rfsr_create bad file type", stp);
	}
	if (vattr.va_type != VREG
	&&  vattr.va_type != VFIFO
	&&  vattr.va_type != VXNAM
	&&  pm_denied(stp->sr_cred, P_FILESYS)) {
		return EPERM;
	}
	fmode = req->rq_create.fmode;
	/*
	 * NULL terminate compname for safety.  (Of course the
	 * client should already have done this, but verifying that would
	 * be as expensive as this assignment.)
	 */
	rf_msgdata(stp->sr_in_bp, hdrsz)[datasz - 1] = '\0';
	/*
	 * Disallow paths containing '/'.
	 */
	compname = rqdp->nm;
	while (*compname) {
		if (*compname++ == '/') {
			return rfsr_j_accuse("rfsr_creat:  bad data", stp);
		}
	}
	if (req->rq_create.ex == NONEXCL) {
		pathname_t		pn;

		compname = rqdp->nm;
		if ((error = pn_get(compname, UIO_SYSSPACE, &pn)) != 0) {
			return error;
		}
		if ((error = pn_stripcomponent(&pn, compname)) != 0 ||
		  pn.pn_pathlen) {
			pn_free(&pn);
			return error;
		}
		error = VOP_LOOKUP(dvp, compname, &compvp, &pn, 0,
		  rootdir, stp->sr_cred);
		pn_free(&pn);
		if (error == ENOENT) {
			error = 0;
		} else if (error) {
			return error;
		}
		if (compvp != NULLVP) {
			if (compvp->v_vfsmountedhere) {
				if ((error = traverse(&compvp)) != 0) {
					VN_RELE(compvp);
					compvp = NULLVP;
					goto out;
				}
			}

			/*
			 * File already exists.	 If a mandatory lock has been
			 * applied, return EAGAIN.
			 */

			/* validate vop */
			if ((error = MAC_VACCESS(compvp, VWRITE, stp->sr_cred))
				!= 0) {

                                VN_RELE(compvp);
				compvp = NULLVP;
                                goto out;
			}

			if (compvp->v_filocks != NULL) {
				struct vattr v2;

				v2.va_mask = AT_MODE;
				if (error = VOP_GETATTR(compvp, &v2, 0, 
				  stp->sr_cred)) {
					VN_RELE(compvp);
					compvp = NULLVP;
					goto out;
				}
				if (MANDLOCK(compvp, v2.va_mode)) {
					error = EAGAIN;
					VN_RELE(compvp);
					compvp = NULLVP;
					goto out;
				}
			}

			/*
			 * If the file is the root of a VFS, we've crossed a
			 * mount point and the "containing" directory that we
			 * acquired above (dvp) is irrelevant because it's in
			 * a different file system.  We apply VOP_CREATE to the
			 * target itself instead of to the containing directory
			 * and supply a null path name to indicate 
			 * (conventionally) the node itself as the "component" 
			 * of interest.
			 */
			if (compvp->v_flag & VROOT) {
				error = VOP_CREATE(compvp, "", &vattr,
				  req->rq_create.ex, fmode, &compvp,
				  stp->sr_cred);
				VN_RELE(compvp);
				goto out;
			} else {
				VN_RELE(compvp);
				compvp = NULLVP;
			}
		}
	}
	/* file does not exist */
	/*validate vop */
	if ((error = MAC_VACCESS(dvp, VWRITE, stp->sr_cred)) != 0)
		goto out;
	/* race below */
	if ((MAC_ACCESS(MACDOM,	dvp->v_vfsp->vfs_macceiling,
				stp->sr_cred->cr_lid))
	||  (MAC_ACCESS(MACDOM,	stp->sr_cred->cr_lid,
				dvp->v_vfsp->vfs_macfloor))
	&&  pm_denied(stp->sr_cred, P_FSYSRANGE)) {
		error= EACCES;
		goto out;
	 }
	error = VOP_CREATE(dvp, rqdp->nm, &vattr, req->rq_create.ex, fmode,
	  &compvp, stp->sr_cred);

out:
	SR_FREEMSG(stp);
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);

	if (error && compvp) {
		VN_RELE(compvp);
	} else if (!error) {
		ASSERT(compvp);
		if (!(error = rfsr_gift_setup(stp, compvp, u.u_srchan))) {
			rcvd_t		*rd = vtord(compvp);
			rd_user_t	*rdup;

			/*
			 * These ASSERTs are okay because they rely only on
			 * local state.
			 */

			ASSERT(rd);
			rdup = rdu_find(rd, SDTOSYSID(u.u_srchan),
			  u.u_srchan->sd_mntid);
			ASSERT(rdup);
			if (fmode & VREAD) {
				rdup->ru_flag |= RU_R_CREAT;
			}
			if (fmode & VWRITE) {
				rdup->ru_flag |= RU_W_CREAT;
			}
			RDU_RELE(&rd, &rdup);
		}
	}
	return error;
}

/*
 * VOP_FRLOCK
 */
/* ARGSUSED */
STATIC int
rfsr_frlock(stp, ctrlp)
	register rfsr_state_t 	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t 	*rvp = stp->sr_rdp->rd_vp;
	register int 		cmd = req->rq_fcntl.cmd;
	register int 		fflag = req->rq_fcntl.fflag;
	off_t 			offset = (off_t)req->rq_fcntl.offset;
	int			error = 0;
	register int		canon = stp->sr_gdpp->hetero != NO_CONV;
	/*
	 * Deadlock avoidance:
	 * Hold incoming data for ops that might sleep, letting us free
	 * incoming streams message.
	 * We assume that a struct flock fits in one request/response.
	 */
	flock_t			flock;

	rfsr_fsinfo.fsivop_other++;
	stp->sr_ret_val = cmd;

	/* validate vop */
	switch (cmd) {
		case F_SETLK:
		case F_SETLKW:
		case F_RSETLK:
		case F_RSETLKW:
			if ((error=MAC_VACCESS(rvp, VWRITE, stp->sr_cred)) != 0)
				return error;
	}

	if (!rdu_modecheck(stp->sr_rdup, fflag)) {
		error = EPROTO;
	}

	/*
	 * Lock data is prewritten.
	 * Likely to sleep on a lock.
	 * Copy data into stack to free streams message.
	 */
	
	if (!error  &&
	  !(error = rfsr_copyflock((caddr_t)&flock, stp))) {
		SR_FREEMSG(stp);
		error = VOP_FRLOCK(rvp, cmd, (int)&flock, fflag, offset,
		  stp->sr_cred);
	}
	if (!error) {
		if (cmd == F_GETLK || cmd == F_O_GETLK) {
			caddr_t		rpdata;

			ASSERT(!stp->sr_out_bp);
			stp->sr_out_bp = rfsr_rpalloc(canon ?
			  sizeof(flock_t) + FLOCK_XP : sizeof(flock_t),
			  stp->sr_vcver);
			rpdata = rf_msgdata(stp->sr_out_bp, 
			  RF_MIN_RESP(stp->sr_vcver));
			if (canon) {
				RF_RESP(stp->sr_out_bp)->rp_count =
				  rf_tcanon(FLOCK_FMT, (caddr_t)&flock, rpdata);
			} else {
				*(flock_t *)rpdata = flock;
			}
		} else {
			ASSERT(!stp->sr_out_bp);
			stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
		}
	}
	return error;
}

/*
 * VOP_SPACE
 */
/* ARGSUSED */
STATIC int
rfsr_space(stp, ctrlp)
	register rfsr_state_t 	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t 	*vp = stp->sr_rdp->rd_vp;
	register int 		cmd = req->rq_fcntl.cmd;
	register long 		flag = req->rq_fcntl.fflag;
	off_t 			offset = (off_t)req->rq_fcntl.offset;
	int			error = 0;
	/*
	 * Deadlock avoidance:
	 * Hold incoming data for ops that might sleep, letting us free
	 * incoming streams message.
	 * We assume that a flock_t fits in one request/response
	 */
	flock_t flock;

	rfsr_fsinfo.fsivop_other++;
	stp->sr_ret_val = cmd;

	/* validate vop */
	if ((error = MAC_VACCESS(vp, VWRITE, stp->sr_cred)) != 0)
		return error;
	/*
	 * Lock data is prewritten.
	 * Likely to sleep on a lock.
	 * Copy data into stack to free streams message.
	 */
	error = rfsr_copyflock((caddr_t)&flock, stp);
	if (!error) {
		SR_FREEMSG(stp);
		if (vp->v_type != VREG) {
			error = EINVAL;
		} else if (!rdu_modecheck(stp->sr_rdup, FWRITE)) {
			error = EPROTO;
		} else if (!(error = VOP_SPACE(vp, cmd, (int)&flock, flag,
		  offset, stp->sr_cred))) {
			ASSERT(!stp->sr_out_bp);
			stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
		}
	}
	return error;
}

/*
 * VOP_SETFL
 */
/* ARGSUSED */
STATIC int
rfsr_setfl(stp, ctrlp)
	register rfsr_state_t 	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t 	*vp = stp->sr_rdp->rd_vp;
	register int 		oflags = req->rq_fcntl.fcntl;
	register int 		nflags = req->rq_fcntl.fflag;
	int			error = 0;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	if (!rdu_modecheck(stp->sr_rdup, nflags)) {
		error = EPROTO;
	} else if (!(error = VOP_SETFL(vp, oflags, nflags, stp->sr_cred))) {
		ASSERT(!stp->sr_out_bp);
		stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	}
	return error;
}

/*
 * VOP_GETATTR
 */
/* ARGSUSED */
STATIC int
rfsr_getattr(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register int		error = 0;
	register rf_response_t	*rp;
	int			canon = stp->sr_gdpp->hetero != NO_CONV;
	int			flags;
	vattr_t			vattr;

	rfsr_fsinfo.fsivop_other++;

	/* validate vop */
	if ((error = MAC_VACCESS(vp, VREAD, stp->sr_cred)) != 0)
		return error;

	vattr.va_mask = req->rq_getattr.mask;
	flags = req->rq_getattr.flags & (ATTR_EXEC | ATTR_COMM);
	SR_FREEMSG(stp);
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc(stp->sr_gdpp->hetero != NO_CONV ?
	  sizeof(rf_attr_t) + ATTR_XP : sizeof(rf_attr_t), stp->sr_vcver);
	rp = RF_RESP(stp->sr_out_bp);
	if ((error = VOP_GETATTR(vp, &vattr, flags, stp->sr_cred)) != 0 ||
	  (error = rfsr_vattr_map(stp, &vattr)) != 0) {
		/* reset these to reflect the failure */
		rp->rp_count = 0;
		rp->rp_nodata = 1;
	} else {
		register rf_attr_t	*rap;
		register caddr_t	data;
		rf_attr_t		rf_attr;

		data = rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
		rap = canon ? &rf_attr : (rf_attr_t *)data;
		vtorf_attr(rap, &vattr);
		if (canon) {
			rp->rp_count = rf_tcanon(ATTR_FMT, (caddr_t)rap, data);
		}
	}
	return error;
}

/* ARGSUSED */
STATIC int
rfsr_setattr(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register rf_attr_t	*rap;
	register int		error;
	size_t			datasz;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	int			hetero = stp->sr_gdpp->hetero;

	rfsr_fsinfo.fsivop_other++;

	/* validate vop */
	if ((error = MAC_VACCESS(vp, VWRITE, stp->sr_cred)) != 0)
		return error;

	datasz = hetero == NO_CONV ? sizeof(rf_attr_t) :
	  sizeof(rf_attr_t) + ATTR_XP;
	if (RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) {
		return rfsr_j_accuse("rfsr_setattr bad data", stp);
	}
	rap = (rf_attr_t *)rf_msgdata(stp->sr_in_bp, hdrsz);
	if (hetero != NO_CONV &&
	  !rf_fcanon(ATTR_FMT, (caddr_t)rap, (caddr_t)rap + datasz,
	  (caddr_t)rap)) {
		return rfsr_j_accuse("rfsr_setattr bad data", stp);
	}
	rap->rfa_uid = gluid(stp->sr_gdpp, rap->rfa_uid);
	rap->rfa_gid = glgid(stp->sr_gdpp, rap->rfa_gid);

	if (vp->v_vfsp->vfs_flag & VFS_RDONLY || stp->sr_srmp->srm_flags &
	  SRM_RDONLY) {
		error = EROFS;
	} else {
		vattr_t	vattr;

		rftov_attr(&vattr, rap);
		vattr.va_atime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		vattr.va_ctime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		vattr.va_mtime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		error = VOP_SETATTR(vp, &vattr,
		  RF_REQ(stp->sr_in_bp)->rq_setattr.flags, stp->sr_cred);
		if (!error && vattr.va_mask & AT_MODE &&
		  MANDLOCK(vp, vattr.va_mode)) {

			/*
			 * Other clients will have received disable messages
			 * if this one enabled locks.  Now we send this one
			 * an implicit disable message.
			 */

			stp->sr_rdup->ru_flag &= ~RU_CACHE_ON;
			stp->sr_rdup->ru_flag |= RU_CACHE_DISABLE;
		}
		SR_FREEMSG(stp);
	}
	return error;
}

/*
 * VOP_FSYNC
 */
/* ARGSUSED */
STATIC int
rfsr_fsync(stp, ctrlp)
	register rfsr_state_t *stp;
	register rfsr_ctrl_t *ctrlp;
{
	vnode_t *vp = stp->sr_rdp->rd_vp;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	return VOP_FSYNC(vp, stp->sr_cred);
}

/*
 * VOP_READDIR
 */
#define RF_MINDIRENT	12	/* dir entry with one char name */
/* ARGSUSED */
STATIC int
rfsr_readdir(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register sndd_t		*sdp = u.u_srchan;		/* gag */
	size_t			resid = req->rq_xfer.count;
	size_t			oresid = resid;
	register int		error = 0;
	int			dircanon = stp->sr_gdpp->hetero != NO_CONV;
	size_t			datasz = stp->sr_gdpp->datasz;
	size_t			hdrsz = RF_MIN_RESP(stp->sr_vcver);
	int			eof = 0;
	long			base = req->rq_xfer.base;
	struct uio		uio;
	struct iovec		iovec;
	caddr_t			workspace;
	size_t			worksize;

	rfsr_fsinfo.fsivop_readdir++;

	if (vp->v_type != VDIR || !rdu_modecheck(stp->sr_rdup, FREAD)) {
		return EPROTO;
	}

	uio.uio_iov = &iovec;
	uio.uio_iovcnt = 1;
	uio.uio_offset = req->rq_xfer.offset;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = 0;	/* TO DO:  ill-specified use */
	SR_FREEMSG(stp);

	if (dircanon) {
		worksize = MAX(RF_MAXDIRENT, MIN(datasz, oresid));
		/*
		 * Don't bother with rf_maxkmem, this is a temporary
		 * allocation.
		 */
		workspace = kmem_alloc(worksize, KM_SLEEP);
	}
	ASSERT(!stp->sr_out_bp);
	VOP_RWLOCK(vp);
	while (!error && resid) {
		register size_t		readsize;
		register rf_response_t	*rp;
		caddr_t			data;
		caddr_t			rpdata;
		off_t			ooff = uio.uio_offset;

		/*
		 * If resid is smaller than the next directory entry,
		 * we can get an unwarranted error.
		 */
		uio.uio_resid = iovec.iov_len = readsize
		  = MAX(RF_MAXDIRENT, MIN(datasz, resid));
		stp->sr_out_bp = rfsr_rpalloc(readsize +
		  (dircanon ? readsize / RF_MINDIRENT * DIRENT_XP : 0),
		  stp->sr_vcver);
		rp = RF_RESP(stp->sr_out_bp);
		rpdata = rf_msgdata(stp->sr_out_bp, hdrsz);
		iovec.iov_base = dircanon ? workspace : rpdata;
		data = iovec.iov_base;
		error = VOP_READDIR(vp, &uio, stp->sr_cred, &eof);
		if (!error && resid < RF_MAXDIRENT &&
		  !rfsr_lastdirents(data, &uio, ooff, resid, &eof) &&
		  resid == oresid) {
			/*
			 * Can't fit an entry into response, first time through
			 * loop.
			 */
			error = EINVAL;
		}
		if (!error) {
			/*
			 * The size of the data transfer is the size request
			 * from the readdir less the residual count from the
			 * op.  The total residual count for the IO is reduced
			 * by the amount of data moved.
			 *
			 * The original protocol has the server doing client
			 * pointer arithmetic, not a wonderful idea.  We
			 * continue for compatability, but clients should
			 * ignore it.  (4.0 clients do.)
			 */
			rp->rp_count = readsize - uio.uio_resid;
			rp->rp_nodata = !rp->rp_count;
			rp->rp_offset = uio.uio_offset;
			rp->rp_copyout.buf = base;
			rp->rp_xfer.eof = eof;
			base += rp->rp_count;
			resid -= rp->rp_count;
			if (!rp->rp_nodata && dircanon) {
				rp->rp_count = rf_dentcanon(rp->rp_count,
				  workspace, rpdata);
			}
			if (resid == 0 || uio.uio_resid == readsize || eof) {
				/*
				 * Send last data in bracketing response.
				 * We can't assume an equivalence between
				 * the last two terms because rfsr_lastdirents
				 * may have diddled uio_resid.
				 */
				break;
			}
			/*
			 * Assume there is more data to come and send the
			 * message.
			 */
			RF_COM(stp->sr_out_bp)->co_opcode = RFCOPYOUT;
			rp->rp_copyout.copysync = 0; /* relic of static queue */
			error = rf_sndmsg(sdp, stp->sr_out_bp,
			  hdrsz + (size_t)rp->rp_count,
			  (rcvd_t *)NULL, FALSE);
			stp->sr_out_bp = NULL;
		}
	}
	VOP_RWUNLOCK(vp);
	if (dircanon) {
		kmem_free(workspace, worksize);
	}
	stp->sr_ret_val = oresid - resid;
	return error;
}

/*
 * VOP_IOCTL
 */
/* ARGSUSED */
STATIC int
rfsr_ioctl(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register int		error;
	file_t			*fp;
	int			fflag = req->rq_ioctl.fflag;

	if (!rdu_modecheck(stp->sr_rdup, fflag)) {
		return EPROTO;
	}
	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	error = VOP_IOCTL(vp, req->rq_ioctl.cmd, req->rq_ioctl.arg, fflag,
	  stp->sr_cred, ((int *)&stp->sr_ret_val));
	if (getf(0, &fp) == 0) {

		/*
		 * Ugly hack for ioctls that do opens.  Only known
		 * example is /proc.
		 */

		register vnode_t	*nvp = fp->f_vnode;

		if (!stp->sr_out_bp) {
			stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
		}
		if (error) {
			closef(fp);
		} else {
			VN_HOLD(nvp);
			if ((error = rfsr_gift_setup(stp, nvp, u.u_srchan))) {
				closef(fp);
			} else {
				ASSERT(stp->sr_gift);
				rdu_open(stp->sr_gift, stp->sr_gdpp->sysid,
				  u.u_srchan->sd_mntid,
		  		  (int)(fp->f_flag & (FREAD | FWRITE)));
				unfalloc(fp);
				VN_RELE(nvp);
			}
		}
		setf(0, NULLFP);
	}
	return error;
}

/*
 * VOP_LINK
 */
STATIC int
rfsr_link(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register vnode_t	*dvp = stp->sr_rdp->rd_vp;
	register vnode_t	*fvp;
	int			error = 0;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	int			pathsz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;

	if (stp->sr_vcver < RFS2DOT0) {
		return dusr_link1(stp, ctrlp);
	}
	rfsr_fsinfo.fsivop_other++;

	/*
	 * All pathnames are fully resolved, and the named target file
	 * is a simple component in the directory represented by the
	 * rd on which the request came.
	 *
	 * The checks are insurance against ill-behaved clients.
	 *
	 * Find source operand vnode and target dir vnode, and make sure
	 * they're in the same writable VFS.
	 */

	if ((fvp = rf_gifttovp(&RF_REQ(stp->sr_in_bp)->rq_link.from,
	  stp->sr_vcver)) == NULL) {
		error = ENOENT;
	} else if (dvp->v_type != VDIR || fvp->v_vfsp != dvp->v_vfsp) {
		error = EXDEV;
	} else if (dvp->v_vfsp->vfs_flag & VFS_RDONLY  ||
	  stp->sr_srmp->srm_flags & SRM_RDONLY) {
		error =  EROFS;
	} else if ((error = MAC_VACCESS(dvp, VWRITE, stp->sr_cred)) != 0){
		return error; /* target directory */
	} else if ((error = MAC_VACCESS(fvp, VWRITE, stp->sr_cred)) != 0){
		return error; /* source file */
	} else {
		VN_HOLD(fvp);
		if (RF_PULLUP(stp->sr_in_bp, hdrsz, (size_t)pathsz)) {
			error = rfsr_j_accuse("rfsr_link bad data", stp);
		} else {
			rf_msgdata(stp->sr_in_bp, hdrsz)[pathsz - 1] = '\0';
			error = VOP_LINK(dvp, fvp, rf_msgdata(stp->sr_in_bp,
			  hdrsz), stp->sr_cred);
		}
		VN_RELE(fvp);
	}
	SR_FREEMSG(stp)
	return error;
}


STATIC int
rfsr_lookup(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	vnode_t			*vp;
	vnode_t			**cvpp;
	vnode_t			**dvpp;
	int			error  = 0;
	register int		hetero = stp->sr_gdpp->hetero != NO_CONV;
	rflkc_info_t		rflkc_info;
	int			flags  = RF_REQ(stp->sr_in_bp)->rq_lookup.flags;
	symfollow_t		followlink	= flags & FOLLOW_SYMLINK
						? FOLLOW
						: NO_FOLLOW;

	rfsr_fsinfo.fsivop_lookup++;
	if (flags & LOOKUP_DIR) {
		dvpp = &vp;
		cvpp = NULLVPP;
	} else {
		dvpp = NULLVPP;
		cvpp = &vp;
	}
		
	error = rfsr_lookupname(followlink, stp, dvpp, cvpp, ctrlp);
	SR_FREEMSG(stp);
	if (*ctrlp != SR_NORMAL || error) {
		return error;
	}

	ASSERT(vp);

	ASSERT(!stp->sr_out_bp);
	if (hetero) {
		stp->sr_out_bp = rfsr_rpalloc(sizeof(rflkc_info_t) +
		  RFLKC_XP, stp->sr_vcver);
	} else {
		stp->sr_out_bp = rfsr_rpalloc(sizeof(rflkc_info_t),
		  stp->sr_vcver);
	}
	VN_HOLD(vp);
	if (!(error = rfsr_gift_setup(stp, vp, u.u_srchan))) {
		/*
		 * Provide some commonly used information about vp.
		 */
		register rf_response_t *rp = RF_RESP(stp->sr_out_bp);
		vattr_t			vattr;

		ASSERT(vtord(vp));
		ASSERT(vtord(vp)->rd_vp == vp);

		vattr.va_mask = AT_ALL;
		if (!VOP_GETATTR(vp, &vattr, 0, stp->sr_cred) &&
		  !rfsr_vattr_map(stp, &vattr)) {
			register caddr_t	rp_data;
			register rflkc_info_t	*rflp;

			rp_data = rf_msgdata(stp->sr_out_bp,
			  RF_MIN_RESP(stp->sr_vcver));
			rflp = hetero ? &rflkc_info : (rflkc_info_t *)rp_data;
			vtorf_attr(&rflp->rflkc_attr, &vattr);
			rflp->rflkc_read_err =
				VOP_ACCESS(vp, VREAD, 0, stp->sr_cred);
			rflp->rflkc_write_err =
				VOP_ACCESS(vp, VWRITE, 0, stp->sr_cred);
			rflp->rflkc_exec_err =
				VOP_ACCESS(vp, VEXEC, 0, stp->sr_cred);
			if (hetero) {
				rp->rp_count = rf_tcanon(RFLKC_FMT,
				  (caddr_t)rflp, rp_data);
			}
			rp->rp_nodata = 0;
		} else {
			rp->rp_count = 0;
			rp->rp_nodata = 1;
		}
	}
	VN_RELE(vp);
	return error;
}


/*
 * Offset and len are page-normalized here because client and server page
 * sizes may vary (in particular the client may have smaller pages).
 */
/* ARGSUSED */
STATIC int
rfsr_addmap(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	uint			offset = req->rq_map.offset & PAGEMASK;
	uint			len = (req->rq_map.len + PAGEOFFSET) & PAGEMASK;
	uint			maxprot = RFPROT_TO_PROT(req->rq_map.maxprot);
	int			error = 0;

	if (!maxprot) {
		maxprot = PROT_READ;
	}
	if (vp->v_type != VREG && vp->v_type != VBLK) {
		return ENODEV;
	}
	if (vp->v_flag & VNOMAP) {
		return ENOSYS;
	}
	if ((int)offset < 0 || (int)(offset + len) < 0) {
		return EINVAL;
	}

	/*
	 * Since prot is manipulable above the file system interface, only
	 * maxprot is sent.  Verify that the client references the file,
	 * that if the client is attempting a writable mapping, that it
	 * has opened the file for writing, or else that it has opened the
	 * file in some way (to cover PROT_READ and PROT_EXEC mappings).
	 *
	 * TO DO:  We should really call the file system VOP_MAP to validate
	 * this and other parameters, and let IT set up the mapping, but first
	 * we'll have to make sure that everyone can deal with a NULL as.
	 */
	error = rfm_check(vp, stp->sr_cred, stp->sr_rdup, offset, len, maxprot);
	if (error) {
		return error;
	}

	/*
	 * Since prot is manipulable above the file system interface, only
	 * maxprot is sent, and is used in the VOP_ADDMAP(DELMAP) below.
	 */

	if (!(error = VOP_ADDMAP(vp, offset, (struct as *)NULL, (addr_t)NULL,
	  len, maxprot, maxprot, maxprot & PROT_WRITE ? MAP_SHARED : 
	  MAP_PRIVATE, stp->sr_cred))) {

		rfm_lock(stp->sr_rdup);
		error = rfm_addmap(stp->sr_rdup, offset, len, maxprot);
		rfm_unlock(stp->sr_rdup);
		if (error) {
			(void)VOP_DELMAP(vp, offset, (struct as *)NULL,
			  (addr_t)NULL, len, maxprot, maxprot,
			  maxprot & PROT_WRITE ? MAP_SHARED : MAP_PRIVATE, 
			  stp->sr_cred);
		}
	}
	return error;
}

/*
 * Offset and len are page-normalized here because client and server page
 * sizes may vary (in particular the client may have smaller pages).
 */
/* ARGSUSED */
STATIC int
rfsr_delmap(stp, ctrlp)
	register rfsr_state_t	*stp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	uint			offset = req->rq_map.offset & PAGEMASK;
	uint			len = (req->rq_map.len + PAGEOFFSET) & PAGEMASK;
	uint			maxprot = RFPROT_TO_PROT(req->rq_map.maxprot);
		
	/*
	 * Since prot is manipulable above the file system interface, 
	 * only maxprot is sent and used.
	 */
	if (!maxprot) {
		maxprot = PROT_READ;
	}
	rfm_lock(stp->sr_rdup);
	rfm_delmap(stp->sr_rdup, offset, len, maxprot, stp->sr_rdp->rd_vp,
	  stp->sr_cred);
	rfm_unlock(stp->sr_rdup);
	return 0;
}

/*
 * VOP_MKDIR
 */
STATIC int
rfsr_mkdir(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	return rfsr_mkdir_common(stp, ctrlp, SR_MKDIR, "rfsr_mkdir bad data");
}

STATIC int
rfsr_mkdir_common(stp, ctrlp, op, jmsg)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
	rfsr_mkdir_t		op;
	char			*jmsg;
{
	vnode_t			*vp;
	vnode_t			*dvp = stp->sr_rdp->rd_vp;
	register struct rqmkdent *rqdp;
	register int		error = 0;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	size_t			datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	vattr_t			vattr;
	char			*compname;

	rfsr_fsinfo.fsivop_create++;

	if (stp->sr_vcver < RFS2DOT0) {
		return dusr_mkdir(stp, ctrlp);
	}
	/* validate vop */
	if((error = MAC_VACCESS(dvp, VWRITE, stp->sr_cred)) != 0)
		return error;

	if ((MAC_ACCESS(MACDOM, dvp->v_vfsp->vfs_macceiling,
				stp->sr_cred->cr_lid)
	||  (MAC_ACCESS(MACDOM,	stp->sr_cred->cr_lid,
				dvp->v_vfsp->vfs_macfloor)))
	&&  pm_denied(stp->sr_cred, P_FSYSRANGE)) {
		return EACCES;
	}
	if (dvp->v_type != VDIR || dvp->v_vfsp->vfs_flag & VFS_RDONLY ||
	  stp->sr_srmp->srm_flags & SRM_RDONLY) {
		return EROFS;
	}
	if (datasz <= sizeof(struct vattr)) {
		return rfsr_j_accuse(jmsg, stp);
	}
	if (RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) {
		return rfsr_j_accuse(jmsg, stp);
	}
	rqdp = (struct rqmkdent *)rf_msgdata(stp->sr_in_bp, hdrsz);
	if (stp->sr_gdpp->hetero != NO_CONV && !rf_fcanon(MKDENT_FMT,
	  (caddr_t)rqdp, (caddr_t)rqdp + datasz, (caddr_t)rqdp)) {
		return rfsr_j_accuse(jmsg, stp);
	}
	rftov_attr(&vattr, &rqdp->attr);
	if (vattr.va_type != VDIR) {
		return EBADF;
	}
	vattr.va_atime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	vattr.va_ctime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	vattr.va_mtime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	/*
	 * NULL terminate compname so pn_get will be safe.  (Of course the
	 * client should already have done this, but verifying that would
	 * be as expensive as this assignment.)
	 */
	rf_msgdata(stp->sr_in_bp, hdrsz)[datasz - 1] = '\0';
	/*
	 * Disallow paths containing '/'.
	 */
	compname = rqdp->nm;
	while (*compname) {
		if (*compname++ == '/') {
			return rfsr_j_accuse(jmsg, stp);
		}
	}
	error = op == SR_MKDIR
		? VOP_MKDIR  (dvp, rqdp->nm, &vattr, &vp, stp->sr_cred)
		: VOP_MAKEMLD(dvp, rqdp->nm, &vattr, &vp, stp->sr_cred);
	SR_FREEMSG(stp);
	if (!error) {
		ASSERT(!stp->sr_out_bp);
		stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
		error = rfsr_gift_setup(stp, vp, u.u_srchan);
	}
	return error;
}

/*
 * VOP_OPEN
 */
STATIC int
rfsr_open(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	int			fmode;
	vnode_t			*vp;
	vnode_t			*ovp;
	int			error = 0;
	rcvd_t			*rdp;
	int			amode = 0;

	if (stp->sr_vcver < RFS2DOT0) {
		return dusr_open(stp, ctrlp);
	}
	rfsr_fsinfo.fsivop_open++;

	rdp = stp->sr_rdp;
	ovp = vp = rdp->rd_vp;
	fmode = RF_REQ(stp->sr_in_bp)->rq_open.fmode;
	SR_FREEMSG(stp);

	/* validate vop */
	VN_MACCHGDENY(vp); /* set vnode not-a-lock */
	if ((error = MAC_VACCESS(vp, fmode, stp->sr_cred)) != 0)
		goto out;

	if (vp->v_type == VLNK || vp->v_type == VBAD || vp->v_type == VNON) {
		error = EBADF;
		goto out;
	}
	if (!(fmode & (FREAD | FWRITE))) {
		error = EINVAL;
		goto out;
	}
	if (fmode & FREAD) {
		amode = VREAD;
	} 
	if (fmode & (FWRITE|FTRUNC)) {
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY ||
		  stp->sr_srmp->srm_flags & SRM_RDONLY) {
			error = EROFS;
			goto out;
		}
		amode |= VWRITE;
	}
	if (fmode & FCREAT) {
		fmode &= ~(FTRUNC|FEXCL);
		if (fmode & FWRITE && !(stp->sr_rdup->ru_flag & RU_W_CREAT) ||
		  fmode & FREAD && !(stp->sr_rdup->ru_flag & RU_R_CREAT)) {
			error = rfsr_j_accuse("rfsr_open:  create not done\n",
			  stp);
			goto out;
		}
	} else if ((error = VOP_ACCESS(vp, amode, 0, stp->sr_cred)) != 0 &&
	  ((amode != VREAD) || vp->v_type != VREG || (error = VOP_ACCESS(vp,
	  VEXEC, 0, stp->sr_cred)) != 0)) {
		goto out;
	}

	/*
	 * In case VOP_OPEN swaps vp with another, make sure
	 * it doesn't disappear, because all of a single client's
	 * contribute just 1 to the vnode reference count.
	 */

	VN_HOLD(ovp);
	if ((error = VOP_OPEN(&vp, fmode, stp->sr_cred)) == 0) {
		if (vp != ovp) {

			/*
			 * VOP_OPEN gave us a new vnode.  Client will
			 * release old one in its time.
			 */

			ASSERT(!stp->sr_out_bp);
			stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);

			/*
			 * Hold vp for the case that rfsr_gift_setup fails and
			 * releases it, we still need vp to VOP_CLOSE.
			 */

			VN_HOLD(vp);
			if ((error = rfsr_gift_setup(stp, vp, u.u_srchan)) 
			  != 0) {
				(void)VOP_CLOSE(vp, fmode, 1, (off_t)0, 
				  stp->sr_cred);
			} else {
				rdu_open(stp->sr_gift, stp->sr_gdpp->sysid,
				  u.u_srchan->sd_mntid, fmode);
			}
			VN_RELE(vp);
		} else {
			rdu_open(rdp, stp->sr_gdpp->sysid,
			  u.u_srchan->sd_mntid, fmode);
			VN_RELE(ovp);
		}
	} else {
		VN_RELE(ovp);
	}
out:
	VN_MACCHGOK(vp); /* unset vnode not-a-lock */
	return error;
}

/*
 * We do explicit data movement, staying out of RF_SERVER() hooks in
 * copy(in|out) (f|s)u(byte|word), etc.  The intent is that only ioctls
 * still go through those, and that only because drivers know about data
 * direction and format, and we don't.
 *
 * If more than one data movement message is required, we send all but the
 * last as RFCOPYOUT messages.  The last is left in stp->sr_out_bp, for later
 * processing in the server; that message will have a RFREAD opcode.  The
 * number of data bytes in that message is in overloaded rp->rp_count.
 */
STATIC int
rfsr_read(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	/* for compatability, we do ptr update for client
	 */
	caddr_t			base = (caddr_t)req->rq_xfer.base;
	register enum vtype	vtype = vp->v_type;
	unsigned		ioflag = 0;
	struct uio		uio;
	struct iovec		iovec;
	register int		error = 0;
	int			isastream;
	rval_t			rval;		

	rfsr_fsinfo.fsivop_read++;
	uio.uio_iov = &iovec;
	error = rfsr_rdwrinit(stp, &uio, vp, &ioflag, ctrlp);
		SR_FREEMSG(stp);
	if (error || *ctrlp != SR_NORMAL) {
		return error;
	}

	/*
	 * We have to check if this is a stream because, if so,
	 * we want to handle the I/O in one vnode op, because of all
	 * the weird options that deal with message boundaries.
	 *
	 * The I_CANPUT ioctl is assumed to fail iff this is not
	 * a stream, and will do no data movement if this is a
	 * stream.  This seems safer than checking v_stream, because
	 * we don't want to make assumptions about the presence or
	 * absence of multihop.
	 */

	if (VOP_IOCTL(vp, I_CANPUT, 0, FREAD | FWRITE, stp->sr_cred, &rval)) {
		isastream = FALSE;
	} else {
		isastream = TRUE;
	}
	if (isastream ||
	  vtype == VCHR && stp->sr_ret_val > stp->sr_gdpp->datasz) {

		/* streams reads and big reads from raw devices */

		error = rfsr_rawread(stp, vp, &uio, ioflag, base, isastream);
	} else {
		error = rfsr_cookedread(stp, vp, &uio, ioflag, base);
	}
	if (!stp->sr_out_bp) {
		stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	}
	return error;
}

/*
 * VOP_READLINK
 */
/* ARGSUSED */
STATIC int
rfsr_readlink(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register unsigned	resid = req->rq_xfer.count;
	register rf_response_t	*resp;
	register caddr_t	rpdata;
	struct uio		uio;
	struct iovec		iovec;
	int			error = 0;

	rfsr_fsinfo.fsivop_other++;

	/* validate vop */
	if ((error = MAC_VACCESS(vp, VREAD, stp->sr_cred)) != 0)
		return error;

	if (resid > MAXPATHLEN) {
		return EINVAL;
	}
	if (vp->v_type != VLNK) {
		return EBADF;
	}
	uio.uio_iov = &iovec;
	uio.uio_iovcnt = 1;
	uio.uio_offset = req->rq_xfer.offset;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = 0;
	SR_FREEMSG(stp);
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc(MAXPATHLEN, stp->sr_vcver);
	resp = RF_RESP(stp->sr_out_bp);
	rpdata = rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
	uio.uio_resid = iovec.iov_len = resid;
	iovec.iov_base = rpdata;
	error = VOP_READLINK(vp, &uio, stp->sr_cred);
	stp->sr_ret_val = resid - uio.uio_resid;

	/*
	 * rfsr_rpalloc assigned the following values; correct
	 * them now that the actual
	 * values are known.
	 */

	if ((resp->rp_count = stp->sr_ret_val) == 0) {
		resp->rp_nodata = 1;
	}
	return error;
}

/*
 * VOP_SYMLINK
 */
/* ARGSUSED */
STATIC int
rfsr_symlink(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register vnode_t	*rvp = stp->sr_rdp->rd_vp;
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	struct rqsymlink	*argp;
	int			error = 0;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	size_t			datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	char			target[MAXPATHLEN + 1];

	rfsr_fsinfo.fsivop_other++;

	/* validate vop */
	if ((error = MAC_VACCESS(rvp, VWRITE, stp->sr_cred)) != 0)
		return error;
	/*
	 * Level of symbolic link is that of the parent directory.
	 * Make sure that this level is within the fs range.
	 * The MAC equality checks are added for performance,
	 * i.e., if the level is that of the floor or ceiling,
	 * there is no need to do domination checks.
	 */
	if ( MAC_ACCESS(MACEQUAL, rvp->v_vfsp->vfs_macfloor,   rvp->v_lid)
	&&   MAC_ACCESS(MACEQUAL, rvp->v_vfsp->vfs_macceiling, rvp->v_lid)
	&&  (MAC_ACCESS(MACDOM,   rvp->v_vfsp->vfs_macceiling, rvp->v_lid)
	  || MAC_ACCESS(MACDOM,   rvp->v_lid,   rvp->v_vfsp->vfs_macfloor))
	&&  pm_denied(stp->sr_cred, P_FSYSRANGE)) {
		return EACCES;
	}

	if (rvp->v_type != VDIR) {
		return EBADF;
	}
	if (RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) {
		return rfsr_j_accuse("rfsr_symlink bad data", stp);
	}
	argp = (struct rqsymlink *)rf_msgdata(stp->sr_in_bp, hdrsz);

	if (stp->sr_gdpp->hetero != NO_CONV &&
	  !rf_fcanon(SYMLNK_FMT, (caddr_t)argp, (caddr_t)argp + datasz,
	  (caddr_t)argp)) {
		return rfsr_j_accuse("rfsr_symlink bad data", stp);
	}
	if (req->rq_slink.tflag) {
		rf_msgdata(stp->sr_in_bp, hdrsz)[datasz - 1] = '\0';
		strcpy(target, argp->target);
	} else if (req->rq_slink.targetln > MAXPATHLEN ||
	  req->rq_slink.targetln <= 0) {
		/*
		 * We have to move the data in one response.
		 */
		return EPROTO;
	} else if (rcopyin(NULL, target, (uint)req->rq_slink.targetln, 1)) {
		return  EFAULT;
	} else {
		target[MAXPATHLEN] = '\0';
	}
	if (rvp->v_vfsp->vfs_flag & VFS_RDONLY ||
	  stp->sr_srmp->srm_flags & SRM_RDONLY) {
		return EROFS;
	}
	if (!error) {
		vattr_t	vattr;

		vattr.va_type = VLNK;
		vattr.va_mode = 0777;
		vattr.va_mask = AT_TYPE|AT_MODE;
		error = VOP_SYMLINK(rvp, argp->rqmkdent.nm, &vattr,
		  target, stp->sr_cred);
	}
	SR_FREEMSG(stp);
	return error;
}

/*
 * VOP_REMOVE
 */
/* ARGSUSED */
STATIC int
rfsr_remove(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	return rfsr_vn_remove(stp, NULLVP);
}

/*
 * Common code for remove and rmdir.
 */
STATIC int
rfsr_vn_remove(stp, cdvp)
	register rfsr_state_t	*stp;
	vnode_t			*cdvp;
{
	int			error;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	vnode_t			*dvp = stp->sr_rdp->rd_vp;
	vfs_t			*vfsp;
	enum vtype		vtype;
	lid_t 			vlid;
	vnode_t			*compvp;
	char			*compname;
	size_t			compsz;
	pathname_t		pn;

	rfsr_fsinfo.fsivop_other++;
	if (dvp->v_type != VDIR || dvp->v_vfsp->vfs_flag & VFS_RDONLY ||
	  stp->sr_srmp->srm_flags & SRM_RDONLY) {
		return EROFS;
	}
	compsz = (size_t)RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	if (RF_PULLUP(stp->sr_in_bp, hdrsz, compsz)) {
		return rfsr_j_accuse("rfsr_remove:  bad data", stp);
	}

	compname = rf_msgdata(stp->sr_in_bp, hdrsz);
	/*
	 * NULL terminate compname so pn_get will be safe.  (Of course the
	 * client should already have done this, but verifying that would
	 * be as expensive as this assignment.)
	 */
	compname[compsz - 1] = '\0';
	if ((error = pn_get(compname, UIO_SYSSPACE, &pn)) != 0) {
		return error;
	}
	if ((error = pn_stripcomponent(&pn, compname)) != 0 ||
	  pn.pn_pathlen) {
		pn_free(&pn);
		return error;
	}

	error = VOP_LOOKUP(dvp, compname, &compvp, &pn, 0, 
	  rootdir, stp->sr_cred);
	pn_free(&pn);
	if (error) {
		return error;
	}

	/*
	 * If the named file is the root of a mounted filesystem, fail.
	 */
	if (compvp->v_vfsmountedhere) {
		error = EBUSY;
		goto out;
	}
	vfsp = compvp->v_vfsp;

	/*
	 * Make sure filesystem is writeable.
	 */
	if (vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}

	/*
	 * Release vnode before removing.
	 */
	vtype = compvp->v_type;
	vlid  = compvp->v_lid;
	VN_RELE(compvp);
	compvp = NULL;

	if ((error = MAC_VACCESS(dvp, VWRITE, stp->sr_cred)) == 0
	&&  (error = MAC_ACCESS(MACDOM, vlid, stp->sr_cred->cr_lid))) {
		if (!pm_denied(stp->sr_cred, P_COMPAT)
		||  !pm_denied(stp->sr_cred, P_MACWRITE))
			error = 0;
	}
	if(error)
		goto out;

	if (cdvp) {
		/*
		 * Caller is using rmdir(2), which can only be applied to
		 * directories.
		 */
		if (vtype != VDIR) {
			error = ENOTDIR;
		} else {
			error = VOP_RMDIR(dvp, compname, cdvp, stp->sr_cred);
		}
	} else {
		error = VOP_REMOVE(dvp, compname, stp->sr_cred);
	}
out:
	if (compvp) {
		VN_RELE(compvp);
	}
	SR_FREEMSG(stp);
	return error;
}


/*
 * VOP_RENAME
 */
/* ARGSUSED */
STATIC int
rfsr_rename(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req;
	vnode_t			*fdvp;
	vnode_t			*tdvp;
	char			*fnm;
	char			*tnm;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	int			error = 0;
	int			pathsz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	vnode_t			*fvp;
	pathname_t		pn;


	rfsr_fsinfo.fsivop_other++;

	if (RF_PULLUP(stp->sr_in_bp, hdrsz, (size_t)pathsz)) {
		return rfsr_j_accuse("rfsr_create bad data", stp);
	}

	req = RF_REQ(stp->sr_in_bp);

	if ((fdvp = rf_gifttovp(&req->rq_rename.from, stp->sr_vcver)) == NULL ||
	  (tdvp = rf_gifttovp(&req->rq_rename.to, stp->sr_vcver)) == NULL) {
		return ENOENT;
	}

	if ((error = MAC_VACCESS(fdvp, VWRITE, stp->sr_cred)) != 0) 
		return error;
	if ((error = MAC_VACCESS(tdvp, VWRITE, stp->sr_cred)) != 0)
		return error;

	if (fdvp->v_type != VDIR || tdvp->v_type != VDIR) {
		error = EXDEV;
	}
	VN_HOLD(fdvp);
	VN_HOLD(tdvp);

	/*
	 * NULL terminate the names to be safe.  (Of course the
	 * client should already have done this, but verifying that would
	 * be as expensive as this assignment.)
	 */
	fnm = rf_msgdata(stp->sr_in_bp, hdrsz);
	fnm[pathsz - 1] = '\0';
	tnm = fnm + strlen(fnm) + 1;
	if ((error = pn_get(fnm, UIO_SYSSPACE, &pn)) != 0) {
		VN_RELE(fdvp);
		VN_RELE(tdvp);
		return error;
	}
	if ((error = pn_stripcomponent(&pn, fnm)) != 0 ||
	  pn.pn_pathlen) {
		pn_free(&pn);
		VN_RELE(fdvp);
		VN_RELE(tdvp);
		return error;
	}

	error = VOP_LOOKUP(fdvp, fnm, &fvp, &pn, 0, 
	  rootdir, stp->sr_cred);
	pn_free(&pn);
	if (error) {
		VN_RELE(fdvp);
		VN_RELE(tdvp);
		return error;
	}

	/*
	 * Traverse mount points.
	 */
	if (fvp->v_vfsmountedhere != NULL) {
		if ((error = traverse(&fvp)) != 0) {
			VN_RELE(fvp);
			VN_RELE(fdvp);
			VN_RELE(tdvp);
			return error;
		}
	}


	if (fvp->v_vfsp != tdvp->v_vfsp) {
		error = EXDEV;
	} 
	VN_RELE(fvp);
	if (!error) {
		if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY ||
		  stp->sr_srmp->srm_flags & SRM_RDONLY) {
			error = EROFS;
		} else {
			error = VOP_RENAME(fdvp, fnm, tdvp, tnm, stp->sr_cred);
		}
	}
	VN_RELE(fdvp);
	VN_RELE(tdvp);

	SR_FREEMSG(stp);

	/* The client will release its references to fdvp and tdvp. */

	return error;
}

STATIC int
rfsr_rsignal(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_common_t	*cop = RF_COM(stp->sr_in_bp);
	register proc_t		*procp;

	*ctrlp = SR_NO_RESP;

	ASSERT(!rfsr_active_lock++);

	for (procp = rfsr_active_procp; procp; procp = procp->p_rlink) {
		if (procp != u.u_procp &&
		  procp->p_epid == (short)cop->co_pid &&
		  procp->p_sysid == u.u_procp->p_sysid) {
			break;
		}
	}

	if (!procp) {
		/* didn't find surrogate of signalled client */
		register mblk_t *sbp;
		register sndd_t *srchan = u.u_srchan;

		ASSERT(!--rfsr_active_lock);

		srchan->sd_srvproc = NULL;
		/* look for request message we are supposed to interrupt
		 */
		if ((sbp = rfsr_chkrdq(stp->sr_rdp, cop->co_pid, cop->co_sysid))
		  != NULL) {
			/* Found unserviced predecessor request;
			 * replace the current message with the predecessor,
			 * turn on the signal bit so that the signal will
			 * be processed in the context of the operation.
			 */
			register rf_message_t *msig;

			SR_FREEMSG(stp);
			stp->sr_in_bp = sbp;
			msig = RF_MSG(sbp);
			msig->m_stat |= RF_SIGNAL;
			sndd_set(srchan, msig->m_queue, &msig->m_gift);
			*ctrlp = SR_OUT_OF_BAND;
		} else {
			/*
			 * The predecessor request message must have
			 * completed already (no server, no pending request)
			 */
			SR_FREEMSG(stp);
		}
	} else {

		/* found surrogate of signalled client */

		ASSERT(!--rfsr_active_lock);

		psignal(procp, SIGTERM);
		SR_FREEMSG(stp);
	}
	return 0;
}

/*
 * VOP_RMDIR
 */
STATIC int
rfsr_rmdir(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	rf_request_t		*req = RF_REQ(stp->sr_in_bp);
	vnode_t 		*cdvp;

	if (stp->sr_vcver < RFS2DOT0) {
		return dusr_rmdir(stp, ctrlp);
	}

	if ((cdvp = rf_gifttovp(&req->rq_rmdir.dir, stp->sr_vcver)) != NULLVP
	  && cdvp->v_type != VDIR) {
		return rfsr_j_accuse("rfsr_rmdir bad directory", stp);
	} else if (!cdvp) {
		cdvp = rootdir;
	}
	return rfsr_vn_remove(stp, cdvp);
}

/*
 * VFS_MOUNT
 *
 * If all is okay, create a mount of a resource for the requesting client.
 *
 * NOTE:  Because concurrency could result in inconsistency, this routine
 * has been ordered so that there is no possibility of sleeping from the
 * return of rfsr_rpalloc to the point in rfsr_gift_setup that the resource's
 * rcvd's reference count has been bumped.
 */
/* ARGSUSED */
STATIC int
rfsr_mount(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register struct gdp	*gdpp = stp->sr_gdpp;
	register rf_request_t	*req;
	register caddr_t	mdata;
	int			mflags;
	int			error = 0;
	rf_resource_t		*rp;
	register sndd_t		*srchan = u.u_srchan;
	register sysid_t	my_sysid;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	size_t			datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	char			resname[RFS_NMSZ];

	rfsr_fsinfo.fsivop_other++;

	if (RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) {
		error = rfsr_j_accuse("rfsr_mount bad data", stp);
		goto out;
	}
	req = RF_REQ(stp->sr_in_bp);
	if (!gdpp->mntcnt) {
		hibyte(gdpp->sysid) =
		  lobyte(loword(RF_COM(stp->sr_in_bp)->co_sysid));
		u.u_procp->p_sysid = gdpp->sysid;
		rfsr_adj_timeskew(gdpp, req->rq_srmount.synctime, 0);
	}
	my_sysid = u.u_procp->p_sysid;
	stp->sr_srmp = NULL;
	mdata = rf_msgdata(stp->sr_in_bp, hdrsz);
	mflags = (int)req->rq_srmount.mntflag;
	if (!rf_fcanon("c0", mdata, mdata + datasz, resname)) {
		error = rfsr_j_accuse("rfsr_mount bad data", stp);
		goto out;
	}
	if ((error = srm_alloc(&stp->sr_srmp)) != 0) {
		goto out;
	}
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	if ((rp = name_to_rsc(resname)) == NULL) {
		error =  ENODEV;
		goto out;
	}
 	if (id_to_srm(rp, my_sysid) != NULL) {
		error = EBUSY;
		goto out;
	}
	if ((rp->r_flags & R_RDONLY) && !(mflags & VFS_RDONLY)) {
		error = EROFS;
		goto out;
	}
	/* see if client is authorized  */
	if (rp->r_clistp &&
	  !rf_checkalist(rp->r_clistp, stp->sr_gdpp->token.t_uname)) {
		error = EACCES;
		goto out;
	}
	if (rp->r_flags & R_FUMOUNT) {
		error = ENONET;
		goto out;
	}
	if (stp->sr_vcver < RFS2DOT0 && rp->r_rootvp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	if (rp->r_flags & R_UNADV) {
		error = ENONET;
		goto out;
	}

	rf_setgdplp(gdpp); /* for Enhanced Security */

	stp->sr_srmp->srm_mntid = srchan->sd_mntid;
	srchan->sd_mntid = rp->r_mntid;
	stp->sr_srmp->srm_sysid = my_sysid;
	stp->sr_srmp->srm_slpcnt = 0;
	if (mflags & VFS_RDONLY) {
		stp->sr_srmp->srm_flags |= SRM_RDONLY;
	}
	if (mflags & MS_CACHE && rfc_time != -1) {
		stp->sr_srmp->srm_flags |= SRM_CACHE;
		stp->sr_ret_val |= MCACHE;
	}
	stp->sr_srmp->srm_nextp = rp->r_mountp;
	stp->sr_srmp->srm_prevp = NULL;
	if (rp->r_mountp) {
		rp->r_mountp->srm_prevp = stp->sr_srmp;
	}
	rp->r_mountp = stp->sr_srmp;

	VN_HOLD(rp->r_rootvp);
	if (!(error = rfsr_gift_setup(stp, rp->r_rootvp, srchan))) {
		gdpp->mntcnt++;
	} else {
		/*
		 * As awkward as the following is, it is necessary because
		 * rfsr_gift_setup may have slept, allowing our rp and/or srmp
		 * to be removed by another process.
		 */
		sr_mount_t *trash_srmp;

		if ((rp = name_to_rsc(resname)) != NULL &&
		  (trash_srmp = id_to_srm(rp, my_sysid)) != NULL) {
 			(void)srm_remove(&rp, &trash_srmp);
			stp->sr_srmp = NULL;
		}
	}
out:
	if (error) {
		if (stp->sr_srmp) {
			srm_free(&stp->sr_srmp);
			stp->sr_srmp = NULL;
		}
		stp->sr_gift = NULL;
	}
	SR_FREEMSG(stp);
	return error;
}

/*
 * VFS_UMOUNT
 */
/* ARGSUSED */
STATIC int
rfsr_umount(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register long		mntid = u.u_srchan->sd_mntid;
	register unsigned	vcount;
	register sysid_t	sysid = stp->sr_gdpp->sysid;
	sr_mount_t		*srp = stp->sr_srmp;
	int			error = 0;

	rfsr_fsinfo.fsivop_other++;
	if (stp->sr_vcver == RFS1DOT0) {
		vcount = 1;
	} else {
		vcount = RF_REQ(stp->sr_in_bp)->rq_rele.vcount;
	}
	SR_FREEMSG(stp);

	DUPRINT2(DB_MNT_ADV, "rfsr_umount: resource %x\n", stp->sr_rsrcp);

	if (srp->srm_refcnt != vcount) {
		/*  still busy for client machine or recovery is going on */
		return EBUSY;
	}

	srp->srm_refcnt -= vcount;

	/*
	 * Free sr_mount structure.  If R_UNADV flag is set
	 * in the resource structure and this was the last
	 * rfsr_mount structure, the resource structure will
	 * also be deallocated.
	 */

	if (srp->srm_flags & SRM_FUMOUNT ||
	  !(error = srm_remove(&stp->sr_rsrcp, &srp))) {
		stp->sr_srmp = NULL;
		rcvd_delete(&stp->sr_rdp, sysid, mntid, vcount, stp->sr_cred);
		stp->sr_gdpp->mntcnt--;
	}
	return error;
}

/*
 * VFS_STATVFS
 */
/* ARGSUSED */
STATIC int
rfsr_statvfs(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	statvfs_t		*svfsb;
	vfs_t			*vfsp = stp->sr_rdp->rd_vp->v_vfsp;
	int			error = 0;
	register rf_response_t	*rp;
	int			canon = stp->sr_gdpp->hetero != NO_CONV;
	caddr_t			rpdata;
	statvfs_t		statvfs;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc(stp->sr_gdpp->hetero != NO_CONV ?
	  sizeof(statvfs_t) + STATVFS_XP : sizeof(statvfs_t), stp->sr_vcver);
	rp = RF_RESP(stp->sr_out_bp);
	rpdata = rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
	if (canon) {
		svfsb = &statvfs;
	} else {
		svfsb = (statvfs_t *)rpdata;
	}
	if ((error = VFS_STATVFS(vfsp, svfsb)) != 0) {
		/* reset these to reflect the failure */
		rp->rp_count = 0;
		rp->rp_nodata = 1;
	} else if (canon) {
		rp->rp_count = rf_tcanon(STATVFS_FMT, (caddr_t)svfsb, rpdata);
	}

	if (MAC_ACCESS(MACDOM, stp->sr_cred->cr_lid, vfsp->vfs_macceiling)
	&&  pm_denied(stp->sr_cred, P_FSYSRANGE)
	&&  pm_denied(stp->sr_cred, P_COMPAT)) {
		svfsb->f_bfree = 0;
		svfsb->f_ffree = 0;
	}

	return error;
}

/* ARGSUSED */
STATIC int
rfsr_ustat(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	long			cbuf = req->rq_ustat.buf;
	register struct ustat	*usp;
	vnode_t			*vp;
	register struct vfs	*vfsp;
	register int		error = 0;
	register rf_response_t	*rp;
	caddr_t			rpdata;
	int			canon = stp->sr_gdpp->hetero != NO_CONV;
	struct statvfs		statvfs;
	struct ustat		ustat;
	register int		i;
	register char		*cp;
	register char		*cp2;

	vfsp = rfsr_dev_dtov((int)req->rq_ustat.dev);
	SR_FREEMSG(stp);
	if (!vfsp) {
		return EINVAL;
	}
	if (error = VFS_ROOT(vfsp, &vp)) {
		return error;
	}
	error = VFS_STATVFS(vfsp, &statvfs);
	VN_RELE(vp);
	if (error) {
		return error;
	}
	if (statvfs.f_ffree > USHRT_MAX) {
		return EOVERFLOW;
	}
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc(stp->sr_gdpp->hetero != NO_CONV ?
	  sizeof(struct ustat) + USTAT_XP : sizeof(struct ustat),
	  stp->sr_vcver);
	rp = RF_RESP(stp->sr_out_bp);
	rpdata = rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
	if (canon) {
		usp = &ustat;
	} else {
		usp = (struct ustat *)rpdata;
	}

	if (MAC_ACCESS(MACDOM, stp->sr_cred->cr_lid, vfsp->vfs_macceiling)
	&&  pm_denied(stp->sr_cred, P_FSYSRANGE)
	&&  pm_denied(stp->sr_cred, P_COMPAT)) {
		usp->f_tfree  = 0;
		usp->f_tinode = 0;
	} else {
		usp->f_tfree  = (daddr_t)
			(statvfs.f_bfree * (statvfs.f_frsize/512));
		usp->f_tinode = (o_ino_t)statvfs.f_ffree;
	}

	/*
	 * Fill f_name, f_pack from variable length strings in f_fstr.
	 */
	cp = statvfs.f_fstr;
	cp2 = usp->f_fname;
	for (i = 0; i < sizeof(usp->f_fname); i++, cp2++) {
		if (*cp != '\0') {
			*cp2 = *cp++;
		} else {
			*cp2 = '\0';
		}
	}
	while (cp++ != '\0' && i < sizeof(statvfs.f_fstr) - 
	  sizeof(usp->f_fpack)) {
		i++;
	}
	cp2 = usp->f_fpack;
	for (i = 0; i < sizeof(usp->f_fpack); i++, cp2++) {
		if (*cp != '\0') {
			*cp2 = *cp++;
		} else {
			*cp2 = '\0';
		}
	}

	if (canon) {
		rp->rp_count = rf_tcanon(USTAT_FMT, (caddr_t)usp, rpdata);
	}
	rp->rp_copyout.buf = cbuf;
	return error;
}

/*
 * VOP_WRITE and VOP_PUTPAGE, as well as system call protocol.
 *
 * TO DO:  remove overloading
 *
 * Client-side write(2) system calls and kernel-generated client writes
 * are handled here.  Different functions were used in SVR3, primarily
 * to keep accounting straight, but we share code here, and just check
 * the opcode for accounting.  By combining the write and writei ops, we
 * also allow client kernels to write to character devices, like network
 * connections, for example.
 *
 * We do explicit data movement, staying out of rf_server() hooks in
 * copy(in|out), (f|s)u(byte|word), etc.  The intent is that only ioctls
 * still go through those, and that only because drivers know about data
 * direction and format, and we don't.
 *
 * If intermediate data movement messages are required, we issue DUCOPYIN
 * messages.
 */
STATIC int
rfsr_write(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	/* base is updated for old clients. */

	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	caddr_t			base = (caddr_t)req->rq_xfer.base;
	register vtype_t	vtype = vp->v_type;
	unsigned		ioflag = 0;
	off_t			apoffset;	/* remember for appends */
	vattr_t			vattr;
	uio_t			uio;
	iovec_t			iovec;
	register int		error = 0;
	long			prewrite = req->rq_xfer.prewrite;
	int			isastream;
	rval_t			rval;		

	if (stp->sr_opcode == RFPUTPAGE) {
		req->rq_xfer.fmode = 0;		/* prevent FAPPEND */
		rfsr_fsinfo.fsivop_putpage++;
		if ((error = rfm_check(vp, stp->sr_cred, stp->sr_rdup,
		  (uint)req->rq_xfer.offset, (uint)req->rq_xfer.count,
		  PROT_WRITE)) != 0) {
			return error;
		}
	} else {
		rfsr_fsinfo.fsivop_write++;
	}
	uio.uio_iov = &iovec;
	if ((error = rfsr_rdwrinit(stp, &uio, vp, &ioflag, ctrlp)) != 0 ||
	  *ctrlp != SR_NORMAL) {
		SR_FREEMSG(stp);
		return error;
	}
	/*
	 * If there's no prewritten data free the incoming message
	 * block.
	 */
	if (!prewrite) {
		SR_FREEMSG(stp);	/* NULLs stp->sr_in_bp */
	}
	if (uio.uio_fmode & FAPPEND) {
		vattr.va_mask = AT_SIZE;
		if (error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred)) {

			/* multihop, net down, e.g. */

			SR_FREEMSG(stp);
			return error;
		}
		apoffset = uio.uio_offset = vattr.va_size;
	} else {
		uio.uio_offset = req->rq_xfer.offset;
	}

	/*
	 * We have to check if this is a stream because, if so,
	 * we want to handle the I/O in one vnode op, because of all
	 * the weird options that deal with message boundaries.
	 *
	 * The I_CANPUT ioctl is assumed to fail iff this is not
	 * a stream, and will do no data movement if this is a
	 * stream.  This seems safer than checking v_stream, because
	 * we don't want to make assumptions about the presence or
	 * absence of multihop.
	 */

	if (VOP_IOCTL(vp, I_CANPUT, 0, FREAD | FWRITE, stp->sr_cred, &rval)) {
		isastream = FALSE;
	} else {
		isastream = TRUE;
	}
	if (isastream ||
	  vtype == VCHR && stp->sr_ret_val > prewrite) {

		/* streams writes and big writes from raw devices */

		error = rfsr_rawwrite(stp, vp, &uio, ioflag, base, isastream);
	} else {
		error = rfsr_cookedwrite(stp, vp, &uio, ioflag, base);
	}
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	if (stp->sr_vcver < RFS2DOT0 && !error) {
		rf_response_t	*rp;

		/* Only old clients look at rdwr.isize */

		rp = RF_RESP(stp->sr_out_bp);
		if (uio.uio_fmode & FAPPEND) {

			/*
			 * Protocol history:  client adjusts f_offset,
			 * and the upper level of the inode kernel wants
			 * to add the number of characters moved.
			 */

			rp->rp_rdwr.isize = apoffset;
		} else if (!(error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred))) {
			rp->rp_rdwr.isize = vattr.va_size;
		}
	}
	return error;
}

/*
 * VOP_GETPAGE
 *
 * Client-generated page faults are handled here.  We don't just process
 * them as reads because page faults cannot be allowed to lock vnodes.
 * Otherwise, a deadlock can happen.
 *
 * If more than one data movement message is required, we send all but the
 * last as RFCOPYOUT messages.  The last is left in stp->out_bp, for later
 * processing in the server; that message will have an RFGETPAGE opcode.  The
 * number of data bytes in that message is in overloaded rp->rp_count.
 */
/* ARGSUSED */
STATIC int
rfsr_getpage(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	vnode_t		*vp = stp->sr_rdp->rd_vp;
	off_t		resid = req->rq_xfer.count;
	off_t		oresid = resid;
	off_t		offset = req->rq_xfer.offset;
	size_t		datasz = stp->sr_gdpp->datasz;
	register int	error = 0;
	unsigned	disabled;	/* flag cache is disabled */

	rfsr_fsinfo.fsivop_getpage++;
	SR_FREEMSG(stp);

	/*
	 *  return value of IO requests is residual char count
	 */

	stp->sr_oldoffset = offset;		/* for caching */
	if (resid <= 0) {
		return EINVAL;
	}

	if ((error = rfm_check(vp, stp->sr_cred, stp->sr_rdup, (uint)offset,
	  (uint)resid, PROT_READ)) != 0) {
		return error;
	}
	disabled = stp->sr_srmp->srm_flags & SRM_CACHE &&
	  stp->sr_rdup->ru_flag & RU_CACHE_DISABLE;

	/*
	 * Iteratively lock down ranges of addresses, esballoc streams
	 * message block to cover the data, and ship it.
	 *
	 * We don't even try to prevent faulting beyond EOF, because the vnode
	 * is not held locked, so the file can change under us.
	 */

	do {
		register off_t	mboff;	/* MAXBOFFSET aligned file offset */
		register off_t	mbon;	/* for first iteration, unaligned
					 * offset in excess of mboff */
		register size_t	nbytes;	/* rfesb_fbread request */
		rf_response_t	*rp;
		rf_common_t	*cop;

		mboff = offset & MAXBMASK;
		mbon = offset & MAXBOFFSET;
		nbytes = MIN(PAGESIZE, MIN(datasz, MIN(MAXBSIZE-mbon, resid)));
		offset += nbytes;
		resid -= nbytes;

		/*
		 * Allocate a streams message without data buffer to
		 * avoid a copy, sending the transport provider enough
		 * information to enqueue the cleanup work for the rf_daemon.
		 * We go to this trouble because we don't want the provider
		 * to sleep in fbrelse.
		 */

		ASSERT(!stp->sr_out_bp);
		if ((error = rfesb_fbread(vp, mboff + mbon, nbytes, S_READ,
		  RF_MIN_RESP(stp->sr_vcver), BPRI_MED, FALSE,
		  &stp->sr_out_bp)) != 0) {
			break;
		}

		rp = RF_RESP(stp->sr_out_bp);
		rp->rp_count = nbytes;
		rp->rp_nodata = FALSE;

		cop = RF_COM(stp->sr_out_bp);
		cop->co_type = RF_RESP_MSG;
		if (resid) {
			cop->co_opcode = RFCOPYOUT;
			error = rf_sndmsg(u.u_srchan, stp->sr_out_bp,
			  RF_MIN_RESP(stp->sr_vcver) +
			  (size_t)nbytes, (rcvd_t *)NULL, FALSE);
			stp->sr_out_bp = NULL;

		}

		if (disabled) {
			rfc_info.rfci_dis_data++;
		}
		rfsr_fsinfo.fsireadch += nbytes;

	} while (!error && resid);

	stp->sr_ret_val = oresid - resid;
	return error;
}	/* rfsr_getpage */

/*
 * VOP_GETACL
 */
/* ARGSUSED */
STATIC int
rfsr_getacl(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	vnode_t 		*vp	= stp->sr_rdp->rd_vp;
	register rf_request_t	*req	= RF_REQ(stp->sr_in_bp);
	int			nentries= req->rq_getacl.nentries;
	int			dentries;
	struct	acl		*aclbufp;
	int			nacls;
	int			error	= 0;
	size_t 			dataun;
	size_t			datasz;
	caddr_t			rpdata;
	rf_response_t		*rp;
	int			canon	= stp->sr_gdpp->hetero != NO_CONV;

	rfsr_fsinfo.fsivop_other++;

	SR_FREEMSG(stp); /* free incoming request now that nentries known */

	/* validate vop */
	if ((error = MAC_VACCESS(vp, VREAD, stp->sr_cred)) != 0)
		return error;

	nentries = MIN(nentries, acl_getmax());

	dataun = canon  ? sizeof(struct acl) + ACL_XP : sizeof(struct acl);
	datasz = dataun * nentries;

	stp->sr_out_bp	= rfsr_rpalloc(datasz, stp->sr_vcver);
	rp 		= RF_RESP(stp->sr_out_bp);
	rpdata		= rf_msgdata(stp->sr_out_bp,RF_MIN_RESP(stp->sr_vcver));

	aclbufp = (struct acl *)(
		canon
		? kmem_alloc(sizeof(struct acl) * nentries, KM_SLEEP)
		: rpdata /* deposit acl's directly into message */
		);

	if ((error = VOP_GETACL(vp, nentries, &dentries,
				aclbufp, stp->sr_cred, &nacls)) == 0){
		rp->rp_getacl.dentries	= (long)dentries;
		stp->sr_ret_val		= (long)nacls;
		rf_acl_idmap(SR2CL, stp->sr_gdpp, aclbufp, nacls);
		if (canon){
			rp->rp_count = rf_acltcanon(aclbufp, rpdata, nacls);
			kmem_free((caddr_t)aclbufp,
				sizeof(struct acl) * nentries);
		} else {
			rp->rp_count = dataun * nacls;
		}
	} else {
		rp->rp_count  = 0;
		rp->rp_nodata = TRUE;
	}
	return error;
}
/*
 * VOP_GETACLCNT
 */
/* ARGSUSED */
STATIC int
rfsr_getaclcnt(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	vnode_t *vp	= stp->sr_rdp->rd_vp;
	int	error	= 0;

	rfsr_fsinfo.fsivop_other++;

	SR_FREEMSG(stp);

	/* validate vop */
	if ((error = MAC_VACCESS(vp, VREAD, stp->sr_cred)) != 0)
		return error;

	return VOP_GETACLCNT(vp, stp->sr_cred, (int *)&stp->sr_ret_val);
}

/*
 * VOP_SETACL
 */
/* ARGSUSED */
STATIC int
rfsr_setacl(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	rf_request_t	*req	= RF_REQ(stp->sr_in_bp);
	size_t		hdrsz	= RF_MIN_REQ(stp->sr_vcver);
	caddr_t		rqdata	= rf_msgdata(stp->sr_in_bp, hdrsz);
	vnode_t		*vp	= stp->sr_rdp->rd_vp;
	int		nentries= (int)req->rq_setacl.nentries;
	size_t		dataun;
	size_t		datasz;
	int		canon	= stp->sr_gdpp->hetero != NO_CONV;
	int		error	= 0;

	rfsr_fsinfo.fsivop_other++;

	/* validate vop */
	if ((error = MAC_VACCESS(vp, VWRITE, stp->sr_cred)) != 0)
		return error;

	if ((vp->v_vfsp->vfs_flag    & VFS_RDONLY) || 
	    (stp->sr_srmp->srm_flags & SRM_RDONLY)) 
		return EROFS;

	dataun = canon  ? sizeof(struct acl) + ACL_XP : sizeof(struct acl);
	datasz = dataun * nentries;

	if (RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) {
		return rfsr_j_accuse("rfsr_setacl bad data", stp);
	}
	if (canon){ /*convert in place, can only shrink */
		if (!rf_aclfcanon( rqdata, (struct acl *)rqdata, nentries)){
			return rfsr_j_accuse("rfsr_setacl bad data", stp);
		}
	}
	rf_acl_idmap(CL2SR, stp->sr_gdpp, (struct acl *)rqdata, nentries);

	return VOP_SETACL(vp, nentries, (int)req->rq_setacl.dentries,
			(struct acl *)rqdata, stp->sr_cred);
	/* calling function frees input message */
}

/*
 * VOP_SETLEVEL
 */
/* ARGSUSED */
STATIC int
rfsr_setlevel(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	vnode_t 	*vp	= stp->sr_rdp->rd_vp;
	rf_request_t	*req	= RF_REQ(stp->sr_in_bp);
	lid_t		wrklvl	= (lid_t)req->rq_setlevel.lid;
	int		error	= 0;

	rfsr_fsinfo.fsivop_other++;

	if ((vp->v_vfsp->vfs_flag    & VFS_RDONLY) || 
	    (stp->sr_srmp->srm_flags & SRM_RDONLY)) 
		return EROFS;

	/* validate vop */
	if ((error = MAC_VACCESS(vp, VWRITE, stp->sr_cred)) != 0)
		return error;
	if (error = pm_denied(stp->sr_cred, P_SETFLEVEL)) {
		if (MAC_ACCESS(MACDOM, wrklvl, vp->v_lid) == 0
		&& (error = pm_denied(stp->sr_cred, P_MACUPGRADE))){
			return error;
		} else
			return error; /* from P_SETFLEVEL */
	}

	return VOP_SETLEVEL(vp, wrklvl, stp->sr_cred);
}

/*
 * VOP_GETDVSTAT
 */
/* ARGSUSED */
STATIC int
rfsr_getdvstat(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	vnode_t		*vp	= stp->sr_rdp->rd_vp;
	struct devstat	devstat;
	int		error;
	caddr_t		rpdata;
	size_t		datasz;
	int		canon	= stp->sr_gdpp->hetero != NO_CONV;
	rf_response_t	*rp;

	rfsr_fsinfo.fsivop_other++;

	SR_FREEMSG(stp);

	if (vp->v_type != VBLK && vp->v_type != VCHR){
		return ENODEV;
	}

	/* validate vop */
	if (pm_denied(stp->sr_cred, P_DEV))
		return EPERM;

	if ((error = VOP_GETDVSTAT(vp, &devstat, stp->sr_cred)) == 0) {
		datasz = canon
			? sizeof(struct devstat) + DVSTAT_XP
			: sizeof(struct devstat);
	 	stp->sr_out_bp	= rfsr_rpalloc(datasz, stp->sr_vcver);
		rpdata	= rf_msgdata(stp->sr_out_bp,RF_MIN_RESP(stp->sr_vcver));
		rp	= RF_RESP(stp->sr_out_bp);

		if (canon){
			rp->rp_count =
			  rf_tcanon(DVSTAT_FMT, (caddr_t)&devstat, rpdata);
		} else {
			*(struct devstat *)rpdata = devstat;
		}
	}
	return error;
}

/*
 * VOP_SETDVSTAT
 */
/* ARGSUSED */
STATIC int
rfsr_setdvstat(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register vnode_t	*vp    = stp->sr_rdp->rd_vp;
	size_t			hdrsz  = RF_MIN_REQ(stp->sr_vcver);
	size_t			datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	caddr_t			rqdata = rf_msgdata(stp->sr_in_bp, hdrsz);
	struct devstat		devstat;
	int			canon  = stp->sr_gdpp->hetero != NO_CONV;

	rfsr_fsinfo.fsivop_other++;


	/* validate vop */
	if (pm_denied(stp->sr_cred, P_DEV))
		return EPERM;
	if (vp->v_type != VBLK && vp->v_type != VCHR){
		return ENODEV;
	}
	/* EROFS check not relevant for this vop */


	if (RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) {
		return rfsr_j_accuse("rfsr_setdvstat bad data", stp);
	}
	if (canon) {
		if (!rf_fcanon( DVSTAT_FMT,rqdata, rqdata+datasz,
			(caddr_t)&devstat)){

			return rfsr_j_accuse("rfsr_setdvstat bad data", stp);
		}
	}else{
		devstat = *(struct devstat *)rqdata;
	}
	SR_FREEMSG(stp);

	return VOP_SETDVSTAT(vp, &devstat, stp->sr_cred);
}

/*
 * VOP_MAKEMLD
 */
/* ARGSUSED */
STATIC int
rfsr_makemld(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{

	/* validate vop */
	if (pm_denied(stp->sr_cred, P_MULTIDIR))
		return EPERM;

	return rfsr_mkdir_common(stp,ctrlp,SR_MAKEMLD,"rfsr_makemld bad data");
}

/*
 * complains about opcode in arg, returns error
 */
/* ARGSUSED */
int
rfsr_undef_op(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	return rfsr_j_accuse("rfs server undefined op", stp);
}

/* indexed by opcode */
int (*rfsr_ops[])() = {
	rfsr_setfl,	/* 0 RFSETFL */
	rfsr_delmap,	/* 1 RFDELMAP */
	rfsr_addmap,	/* 2 RFADDMAP */
	rfsr_read,	/* 3 RFREAD */
	rfsr_write,	/* 4 RFWRITE */
	rfsr_open,	/* 5 DUOPEN */
	rfsr_close,	/* 6 DUCLOSE */
	rfsr_lookup,	/* 7 RFLOOKUP */
	rfsr_create,	/* 8 RFSCREATE */
	dusr_link,	/* 9 DULINK */
	dusr_unlink,	/* 10 DUUNLINK */
	dusr_exec,	/* 11 DUEXEC */
	dusr_chdirec,	/* 12 DUCHDIR */
	rfsr_write,	/* 13 RFPUTPAGE */
	dusr_mknod,	/* 14 DUMKNOD */
	dusr_chmod,	/* 15 DUCHMOD */
	dusr_chown,	/* 16 DUCHOWN */
	rfsr_getpage,	/* 17 RFGETPAGE */
	dusr_stat,	/* 18 DUSTAT */
	dusr_seek,	/* 19 DUSEEK */
	rfsr_getattr,	/* 20 RFGETATTR */
	rfsr_undef_op,	/* 21 UNUSED */
	rfsr_undef_op,	/* 22 UNUSED */
	rfsr_setattr,	/* 23 RFSETATTR */
	rfsr_access,	/* 24 RFACCESS */
	rfsr_pathconf,	/* 25 RFPATHCONF */
	rfsr_undef_op,	/* 26 UNUSED */
	rfsr_undef_op,	/* 27 UNUSED */
	dusr_fstat,	/* 28 DUFSTAT */
	rfsr_undef_op,	/* 29 UNUSED */
	dusr_utime,	/* 30 DUUTIME */
	rfsr_undef_op,	/* 31 UNUSED */
	rfsr_undef_op,	/* 32 UNUSED */
	dusr_saccess,	/* 33 DUSACCESS - access system call */
	rfsr_undef_op,	/* 34 UNUSED */
	dusr_statfs,	/* 35 DUSTATFS */
	rfsr_undef_op,	/* 36 UNUSED */
	rfsr_undef_op,	/* 37 UNUSED */
	dusr_fstatfs,	/* 38 DUFSTATFS */
	rfsr_undef_op,	/* 39 UNUSED */
	rfsr_rename,	/* 40 RFRENAME */
	rfsr_undef_op,	/* 41 UNUSED */
	rfsr_undef_op,	/* 42 UNUSED */
	rfsr_undef_op,	/* 43 UNUSED */
	rfsr_undef_op,	/* 44 UNUSED */
	rfsr_undef_op,	/* 45 UNUSED */
	rfsr_undef_op,	/* 46 UNUSED */
	rfsr_undef_op,	/* 47 UNUSED */
	rfsr_undef_op,	/* 48 UNUSED */
	rfsr_undef_op,	/* 49 UNUSED */
	rfsr_undef_op,	/* 50 UNUSED */
	rfsr_undef_op,	/* 51 UNUSED */
	rfsr_undef_op,	/* 52 UNUSED */
	rfsr_undef_op,	/* 53 UNUSED */
	rfsr_ioctl,	/* 54 DUIOCTL */
	rfsr_undef_op,	/* 55 UNUSED */
	rfsr_undef_op,	/* 56 UNUSED */
	rfsr_ustat,	/* 57 RFUSTAT */
	rfsr_fsync,	/* 58 RFFSYNC */
	dusr_exec,	/* 59 DUEXECE */
	rfsr_undef_op,	/* 60 UNUSED */
	dusr_chdirec,	/* 61 DUCHROOT */
	dusr_fcntl,	/* 62 DUFCNTL */
	rfsr_space,	/* 63 RFSPACE */
	rfsr_frlock,	/* 64 RFFRLOCK */
	rfsr_undef_op,	/* 65 UNUSED */
	rfsr_undef_op,	/* 66 UNUSED */
	rfsr_undef_op,	/* 67 UNUSED */
	rfsr_undef_op,	/* 68 UNUSED */
	rfsr_undef_op,	/* 69 UNUSED */
	rfsr_undef_op,	/* 70 UNUSED */
	rfsr_undef_op,	/* 71 UNUSED */
	dusr_rmount,	/* 72 DURMOUNT */
	rfsr_undef_op,	/* 73 UNUSED */
	rfsr_undef_op,	/* 74 UNUSED */
	rfsr_undef_op,	/* 75 UNUSED */
	rfsr_undef_op,	/* 76 UNUSED */
	rfsr_undef_op,	/* 77 UNUSED */
	rfsr_undef_op,	/* 78 UNUSED */
	rfsr_rmdir,	/* 79 RFRMDIR */
	rfsr_mkdir,	/* 80 RFMKDIR */
	rfsr_readdir,	/* 81 RFREADDIR */
	rfsr_undef_op,	/* 82 UNUSED */
	rfsr_undef_op,	/* 83 UNUSED */
	rfsr_undef_op,	/* 84 UNUSED */
	rfsr_undef_op,	/* 85 UNUSED */
	rfsr_undef_op,	/* 86 UNUSED */
	rfsr_undef_op,	/* 87 UNUSED */
	rfsr_undef_op,	/* 88 UNUSED */
	rfsr_symlink,	/* 89 RFSYMLINK */
	rfsr_readlink,	/* 90 RFREADLINK */
	rfsr_undef_op,	/* 91 UNUSED */
	rfsr_undef_op,	/* 92 UNUSED */
	rfsr_undef_op,	/* 93 UNUSED */
	rfsr_undef_op,	/* 94 UNUSED */
	rfsr_undef_op,	/* 95 UNUSED */
	rfsr_undef_op,	/* 96 UNUSED */
	rfsr_mount,	/* 97 DUSRMOUNT */
	rfsr_umount,	/* 98 DUSRUMOUNT */
	rfsr_undef_op,	/* 99 UNUSED */
	rfsr_undef_op,	/* 100 UNUSED */
	rfsr_undef_op,	/* 101 UNUSED */
	rfsr_undef_op,	/* 102 UNUSED */
	rfsr_undef_op,	/* 103 UNUSED */
	rfsr_statvfs,	/* 104 RFSTATVFS */
	rfsr_undef_op,	/* 105 UNUSED */
	rfsr_undef_op,	/* 106 RFCOPYIN */
	rfsr_undef_op,	/* 107 RFCOPYOUT */
	rfsr_undef_op,	/* 108 UNUSED */
	rfsr_link,	/* 109 RFLINK */
	rfsr_undef_op,	/* 110 UNUSED */
	dusr_coredump,	/* 111 DUCOREDUMP */
	rfsr_write,	/* 112 DUWRITEI */
	rfsr_read,	/* 113 DUREADI */
	rfsr_undef_op,	/* 114 UNUSED */
	rfsr_undef_op,	/* 115 UNUSED */
	rfsr_undef_op,	/* 116 UNUSED */
	rfsr_undef_op,	/* 117 UNUSED */
	rfsr_undef_op,	/* 118 UNUSED */
	rfsr_rsignal,	/* 119 RFRSIGNAL */
	rfsr_undef_op,	/* 120 UNUSED */
	rfsr_undef_op,	/* 121 UNUSED */
	rfsr_undef_op,	/* 122 RFSYNCTIME */
	rfsr_undef_op,	/* 123 UNUSED */
	rfsr_undef_op,	/* 124 RFDOTDOT */
	rfsr_undef_op,	/* 125 UNUSED */
	rfsr_undef_op,	/* 126 RFFUMOUNT */
	rfsr_undef_op,	/* 127 DUSENDUMSG */
	rfsr_undef_op,	/* 128 RFGETUMSG */
	rfsr_remove,	/* 129 RFREMOVE */
	rfsr_undef_op,	/* 130 UNUSED */
	rfsr_inactive,	/* 131 RFINACTIVE */
	dusr_iupdate,	/* 132 DUIUPDATE */

	rfsr_undef_op,	/* 133 UNUSED */
	rfsr_undef_op,	/* 134 UNUSED */
	rfsr_getacl,	/* 135 RFGETACL */
	rfsr_getaclcnt,	/* 136 RFGETACLCNT */
	rfsr_setacl,	/* 137 RFSETACL */
	rfsr_setlevel,	/* 138 RFSETLEVEL */
	rfsr_getdvstat,	/* 139 RFGETDVSTAT */
	rfsr_setdvstat,	/* 140 RFSETDVSTAT */
	rfsr_makemld	/* 141 RFMAKEMLD */
};
