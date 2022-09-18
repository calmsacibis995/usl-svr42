/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/nfs/nfs_xdr.c	1.6.2.3"
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

#include <util/param.h>
#include <util/spl.h>
#include <util/types.h>
#include <svc/systm.h>
#include <proc/user.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <fs/dirent.h>
#include <fs/vfs.h>
#include <io/stream.h>
#include <util/debug.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#undef NFSSERVER
#include <fs/nfs/nfs.h>
#include <net/tcpip/in.h>
#include <mem/hat.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_kmem.h>

STATIC char *xdropnames[] = { "encode", "decode", "free" };

STATIC	void	rrokfree();

/*
 * These are the XDR routines used to serialize and deserialize
 * the various structures passed as parameters accross the network
 * between NFS clients and servers.
 */

/*
 * File access handle
 * The fhandle struct is treated a opaque data on the wire
 */
bool_t
xdr_fhandle(xdrs, fh)
	XDR *xdrs;
	fhandle_t *fh;
{

	if (xdr_opaque(xdrs, (caddr_t)fh, NFS_FHSIZE)) {
		NFSLOG(0x80, "xdr_fhandle: %s %x\n", xdropnames[(int)xdrs->x_op], fh);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_fhandle %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}


/*
 * Arguments to remote write and writecache
 */
bool_t
xdr_writeargs(xdrs, wa)
	XDR *xdrs;
	struct nfswriteargs *wa;
{
	if (xdr_fhandle(xdrs, &wa->wa_fhandle) &&
	    xdr_long(xdrs, (long *)&wa->wa_begoff) &&
	    xdr_long(xdrs, (long *)&wa->wa_offset) &&
	    xdr_long(xdrs, (long *)&wa->wa_totcount) &&
	    xdr_bytes(xdrs, &wa->wa_data, (u_int *)&wa->wa_count,
		    NFS_MAXDATA)) {
		NFSLOG(0x80, "xdr_writeargs: %s off %d ",
			xdropnames[(int)xdrs->x_op], wa->wa_offset);
		NFSLOG(0x80, "count %d\n", wa->wa_totcount, 0);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_writeargs: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}


/*
 * File attributes
 */
bool_t
xdr_fattr(xdrs, na)
	XDR *xdrs;
	register struct nfsfattr *na;
{
	register long *ptr;

	NFSLOG(0x80, "xdr_fattr: %s\n", xdropnames[(int)xdrs->x_op], 0);
	if (xdrs->x_op == XDR_ENCODE) {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			IXDR_PUT_ENUM(ptr, na->na_type);
			IXDR_PUT_LONG(ptr, na->na_mode);
			IXDR_PUT_LONG(ptr, na->na_nlink);
			IXDR_PUT_LONG(ptr, na->na_uid);
			IXDR_PUT_LONG(ptr, na->na_gid);
			IXDR_PUT_LONG(ptr, na->na_size);
			IXDR_PUT_LONG(ptr, na->na_blocksize);
			IXDR_PUT_LONG(ptr, na->na_rdev);
			IXDR_PUT_LONG(ptr, na->na_blocks);
			IXDR_PUT_LONG(ptr, na->na_fsid);
			IXDR_PUT_LONG(ptr, na->na_nodeid);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_usec);
			return (TRUE);
		}
	} else {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			na->na_type = IXDR_GET_ENUM(ptr, enum nfsftype);
			na->na_mode = IXDR_GET_LONG(ptr);
			na->na_nlink = IXDR_GET_LONG(ptr);
			na->na_uid = IXDR_GET_LONG(ptr);
			na->na_gid = IXDR_GET_LONG(ptr);
			na->na_size = IXDR_GET_LONG(ptr);
			na->na_blocksize = IXDR_GET_LONG(ptr);
			na->na_rdev = IXDR_GET_LONG(ptr);
			na->na_blocks = IXDR_GET_LONG(ptr);
			na->na_fsid = IXDR_GET_LONG(ptr);
			na->na_nodeid = IXDR_GET_LONG(ptr);
			na->na_atime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_atime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_usec = IXDR_GET_LONG(ptr);
			return (TRUE);
		}
	}
	if (xdr_enum(xdrs, (enum_t *)&na->na_type) &&
	    xdr_u_long(xdrs, &na->na_mode) &&
	    xdr_u_long(xdrs, &na->na_nlink) &&
	    xdr_u_long(xdrs, &na->na_uid) &&
	    xdr_u_long(xdrs, &na->na_gid) &&
	    xdr_u_long(xdrs, &na->na_size) &&
	    xdr_u_long(xdrs, &na->na_blocksize) &&
	    xdr_u_long(xdrs, &na->na_rdev) &&
	    xdr_u_long(xdrs, &na->na_blocks) &&
	    xdr_u_long(xdrs, &na->na_fsid) &&
	    xdr_u_long(xdrs, &na->na_nodeid) &&
	    xdr_timeval(xdrs, &na->na_atime) &&
	    xdr_timeval(xdrs, &na->na_mtime) &&
	    xdr_timeval(xdrs, &na->na_ctime) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_fattr: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * Extended (ESV) protocol file attributes
 */
bool_t
xdr_esvfattr(xdrs, na)
	XDR *xdrs;
	register struct nfsesvfattr *na;
{
	register long *ptr;

	NFSLOG(0x80, "xdr_esvfattr: %s\n", xdropnames[(int)xdrs->x_op], 0);
	if (xdrs->x_op == XDR_ENCODE) {
		ptr = XDR_INLINE(xdrs, 23 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			IXDR_PUT_ENUM(ptr, na->na_type);
			IXDR_PUT_LONG(ptr, na->na_mode);
			IXDR_PUT_LONG(ptr, na->na_nlink);
			IXDR_PUT_LONG(ptr, na->na_uid);
			IXDR_PUT_LONG(ptr, na->na_gid);
			IXDR_PUT_LONG(ptr, na->na_size);
			IXDR_PUT_LONG(ptr, na->na_blocksize);
			IXDR_PUT_LONG(ptr, na->na_rdev);
			IXDR_PUT_LONG(ptr, na->na_blocks);
			IXDR_PUT_LONG(ptr, na->na_fsid);
			IXDR_PUT_LONG(ptr, na->na_nodeid);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_privs);
			IXDR_PUT_LONG(ptr, na->na_sens);
			IXDR_PUT_LONG(ptr, na->na_info);
			IXDR_PUT_LONG(ptr, na->na_integ);
			IXDR_PUT_LONG(ptr, na->na_ncs);
			IXDR_PUT_LONG(ptr, na->na_acl);
			return (TRUE);
		}
	} else {
		ptr = XDR_INLINE(xdrs, 23 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			na->na_type = IXDR_GET_ENUM(ptr, enum nfsftype);
			na->na_mode = IXDR_GET_LONG(ptr);
			na->na_nlink = IXDR_GET_LONG(ptr);
			na->na_uid = IXDR_GET_LONG(ptr);
			na->na_gid = IXDR_GET_LONG(ptr);
			na->na_size = IXDR_GET_LONG(ptr);
			na->na_blocksize = IXDR_GET_LONG(ptr);
			na->na_rdev = IXDR_GET_LONG(ptr);
			na->na_blocks = IXDR_GET_LONG(ptr);
			na->na_fsid = IXDR_GET_LONG(ptr);
			na->na_nodeid = IXDR_GET_LONG(ptr);
			na->na_atime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_atime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_privs = IXDR_GET_LONG(ptr);
			na->na_sens = IXDR_GET_LONG(ptr);
			na->na_info = IXDR_GET_LONG(ptr);
			na->na_integ = IXDR_GET_LONG(ptr);
			na->na_ncs = IXDR_GET_LONG(ptr);
			na->na_acl = IXDR_GET_LONG(ptr);
			return (TRUE);
		}
	}
	if (xdr_enum(xdrs, (enum_t *)&na->na_type) &&
	    xdr_u_long(xdrs, &na->na_mode) &&
	    xdr_u_long(xdrs, &na->na_nlink) &&
	    xdr_u_long(xdrs, &na->na_uid) &&
	    xdr_u_long(xdrs, &na->na_gid) &&
	    xdr_u_long(xdrs, &na->na_size) &&
	    xdr_u_long(xdrs, &na->na_blocksize) &&
	    xdr_u_long(xdrs, &na->na_rdev) &&
	    xdr_u_long(xdrs, &na->na_blocks) &&
	    xdr_u_long(xdrs, &na->na_fsid) &&
	    xdr_u_long(xdrs, &na->na_nodeid) &&
	    xdr_timeval(xdrs, &na->na_atime) &&
	    xdr_timeval(xdrs, &na->na_mtime) &&
	    xdr_timeval(xdrs, &na->na_ctime) &&
	    xdr_u_long(xdrs, &na->na_privs) &&
	    xdr_u_long(xdrs, &na->na_sens) &&
	    xdr_u_long(xdrs, &na->na_info) &&
	    xdr_u_long(xdrs, &na->na_integ) &&
	    xdr_u_long(xdrs, &na->na_ncs) &&
	    xdr_u_long(xdrs, &na->na_acl) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvfattr: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * Arguments to remote read
 */
bool_t
xdr_readargs(xdrs, ra)
	XDR *xdrs;
	struct nfsreadargs *ra;
{

	if (xdr_fhandle(xdrs, &ra->ra_fhandle) &&
	    xdr_long(xdrs, (long *)&ra->ra_offset) &&
	    xdr_long(xdrs, (long *)&ra->ra_count) &&
	    xdr_long(xdrs, (long *)&ra->ra_totcount) ) {
		NFSLOG(0x80, "xdr_readargs: %s off %d ",
		    xdropnames[(int)xdrs->x_op], ra->ra_offset);
		NFSLOG(0x80, "count %d\n", ra->ra_totcount, 0);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_readargs: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 *	Info necessary to free the mapping which is also dynamically allocated.
 */
struct rrokinfo {
	void	(*func)();
	mblk_t	*arg;
	int	done;
	struct vnode *vp;
	char	*map;
	char	*data;
	u_int	count;
};

STATIC void
rrokfree(rip)
	struct rrokinfo *rip;
{
	int s;
	NFSLOG(0x80, "rrokfree: rip %x\n", rip, 0);
	s = splimp();
	while (rip->done == 0) {
		(void) sleep((caddr_t)rip, PZERO-1);
	}
	(void) splx(s);
	/*
	 * Unlock and release the mapping.
	 */
	(void) as_fault(&kas, rip->data, rip->count, F_SOFTUNLOCK, S_READ);
	(void) segmap_release(segkmap, rip->map, 0);
	NFSLOG(0x80, "rrokfree: release vnode %x\n", rip->vp, 0);
	VN_RELE(rip->vp);
	NFSLOG(0x80, "rrokfree: free block %x\n", rip->arg, 0);
	(void) freeb(rip->arg);
	NFSLOG(0x80, "rrokfree: Done\n", 0, 0);
}

/*
 * Wake up user process to free mapping and vp (rrokfree)
 */
/*
STATIC void
rrokwakeup(rip)
	struct rrokinfo *rip;
{

	rip->done = 1;
	wakeprocs((caddr_t)rip, PRMPT);
}
*/

/*
 * Status OK portion of remote read reply
 */
bool_t
xdr_rrok(xdrs, rrok)
	XDR *xdrs;
	struct nfsrrok *rrok;
{

	if (xdr_fattr(xdrs, &rrok->rrok_attr)) {
		if (xdrs->x_op == XDR_ENCODE && rrok->rrok_map) {
			/* server side */
			struct rrokinfo *rip;
			mblk_t *mp;

			while (!(mp = allocb(sizeof(*rip), BPRI_LO)))
				if (strwaitbuf(sizeof(*rip), BPRI_LO, 1)) {
					NFSLOG(0x81, "xdr_rrok: allocb failed\n", 0, 0);
					return (FALSE);
				}
			/* LINTED pointer alignment */
			rip = (struct rrokinfo *) mp->b_rptr;
			rip->func = rrokfree;
			rip->arg = mp;
			rip->done = 0;
			rip->vp = rrok->rrok_vp;
			rip->map = rrok->rrok_map;
			rip->data = rrok->rrok_data;
			rip->count = rrok->rrok_count;
			xdrs->x_public = (caddr_t)rip;
			rip->done = 1;
			/*
			 * Try it the old, slow way.
			 */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
			    (u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {
				NFSLOG(0x80, "xdr_rrok: %s %d ",
					xdropnames[(int)xdrs->x_op],
					rrok->rrok_count);
				NFSLOG(0x80, "addr %x\n", rrok->rrok_data, 0);
				return (TRUE);
			}
		} else {			/* client side */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
			    (u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {
				NFSLOG(0x80, "xdr_rrok: %s %d ",
					xdropnames[(int)xdrs->x_op],
					rrok->rrok_count);
				NFSLOG(0x80, "addr %x\n", rrok->rrok_data, 0);
				return (TRUE);
			}
		}
	}
	NFSLOG(0x81, "xdr_rrok: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * Status OK portion of remote read reply for extended (ESV) protocol
 */
bool_t
xdr_esvrrok(xdrs, rrok)
	XDR *xdrs;
	struct nfsesvrrok *rrok;
{

	if (xdr_esvfattr(xdrs, &rrok->rrok_attr)) {
		if (xdrs->x_op == XDR_ENCODE && rrok->rrok_map) {
			/* server side */
			struct rrokinfo *rip;
			mblk_t *mp;

			while (!(mp = allocb(sizeof(*rip), BPRI_LO)))
				if (strwaitbuf(sizeof(*rip), BPRI_LO, 1)) {
					NFSLOG(0x81, "xdr_rrok: allocb failed\n", 0, 0);
					return (FALSE);
				}
			/* LINTED pointer alignment */
			rip = (struct rrokinfo *) mp->b_rptr;
			rip->func = rrokfree;
			rip->arg = mp;
			rip->done = 0;
			rip->vp = rrok->rrok_vp;
			rip->map = rrok->rrok_map;
			rip->data = rrok->rrok_data;
			rip->count = rrok->rrok_count;
			xdrs->x_public = (caddr_t)rip;
			rip->done = 1;
			/*
			 * Try it the old, slow way.
			 */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
			    (u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {
				NFSLOG(0x80, "xdr_rrok: %s %d ",
					xdropnames[(int)xdrs->x_op],
					rrok->rrok_count);
				NFSLOG(0x80, "addr %x\n", rrok->rrok_data, 0);
				return (TRUE);
			}
		} else {			/* client side */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
			    (u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {
				NFSLOG(0x80, "xdr_rrok: %s %d ",
					xdropnames[(int)xdrs->x_op],
					rrok->rrok_count);
				NFSLOG(0x80, "addr %x\n", rrok->rrok_data, 0);
				return (TRUE);
			}
		}
	}
	NFSLOG(0x81, "xdr_esvrrok: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

STATIC struct xdr_discrim rdres_discrim[2] = {
	{ (int)NFS_OK, xdr_rrok },
	{ __dontcare__, NULL_xdrproc_t }
};

STATIC struct xdr_discrim esvrdres_discrim[2] = {
	{ (int)NFS_OK, xdr_esvrrok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Reply from remote read
 */
bool_t
xdr_rdresult(xdrs, rr)
	XDR *xdrs;
	struct nfsrdresult *rr;
{

	NFSLOG(0x80, "xdr_rdresult: %s\n", xdropnames[(int)xdrs->x_op], 0);
	if (xdr_union(xdrs, (enum_t *)&(rr->rr_status),
	      (caddr_t)&(rr->rr_ok), rdres_discrim, xdr_void) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_rdresult: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * Reply from remote read - extended (ESV) protocol
 */
bool_t
xdr_esvrdresult(xdrs, rr)
	XDR *xdrs;
	struct nfsesvrdresult *rr;
{

	NFSLOG(0x80, "xdr_esvrdresult: %s\n", xdropnames[(int)xdrs->x_op], 0);
	if (xdr_union(xdrs, (enum_t *)&(rr->rr_status),
	      (caddr_t)&(rr->rr_ok), esvrdres_discrim, xdr_void) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvrdresult: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * File attributes which can be set
 */
bool_t
xdr_sattr(xdrs, sa)
	XDR *xdrs;
	struct nfssattr *sa;
{

	if (xdr_u_long(xdrs, &sa->sa_mode) &&
	    xdr_u_long(xdrs, &sa->sa_uid) &&
	    xdr_u_long(xdrs, &sa->sa_gid) &&
	    xdr_u_long(xdrs, &sa->sa_size) &&
	    xdr_timeval(xdrs, &sa->sa_atime) &&
	    xdr_timeval(xdrs, &sa->sa_mtime) ) {
		NFSLOG(0x80, "xdr_sattr: %s mode %o ",
		    xdropnames[(int)xdrs->x_op], sa->sa_mode);
		   
		NFSLOG(0x80, "uid %d gid %d ", sa->sa_uid, sa->sa_gid);
		NFSLOG(0x80, "size %d\n", sa->sa_size, 0);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_sattr: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * File attributes which can be set - extended (ESV) protocol
 */
bool_t
xdr_esvsattr(xdrs, sa)
	XDR *xdrs;
	struct nfsesvsattr *sa;
{

	if (xdr_u_long(xdrs, &sa->sa_mode) &&
	    xdr_u_long(xdrs, &sa->sa_uid) &&
	    xdr_u_long(xdrs, &sa->sa_gid) &&
	    xdr_u_long(xdrs, &sa->sa_size) &&
	    xdr_timeval(xdrs, &sa->sa_atime) &&
	    xdr_timeval(xdrs, &sa->sa_mtime) &&
	    xdr_u_long(xdrs, &sa->sa_privs) &&
	    xdr_u_long(xdrs, &sa->sa_sens) &&
	    xdr_u_long(xdrs, &sa->sa_info) &&
	    xdr_u_long(xdrs, &sa->sa_integ) &&
	    xdr_u_long(xdrs, &sa->sa_ncs) &&
	    xdr_u_long(xdrs, &sa->sa_acl) ) {
		NFSLOG(0x80, "xdr_esvsattr: %s mode %o ",
		    xdropnames[(int)xdrs->x_op], sa->sa_mode);
		   
		NFSLOG(0x80, "uid %d gid %d ", sa->sa_uid, sa->sa_gid);
		NFSLOG(0x80, "size %d\n", sa->sa_size, 0);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvsattr: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

STATIC struct xdr_discrim attrstat_discrim[2] = {
	{ (int)NFS_OK, xdr_fattr },
	{ __dontcare__, NULL_xdrproc_t }
};

STATIC struct xdr_discrim esvattrstat_discrim[2] = {
	{ (int)NFS_OK, xdr_esvfattr },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Reply status with file attributes
 */
bool_t
xdr_attrstat(xdrs, ns)
	XDR *xdrs;
	struct nfsattrstat *ns;
{

	if (xdr_union(xdrs, (enum_t *)&(ns->ns_status),
	      (caddr_t)&(ns->ns_attr), attrstat_discrim, xdr_void) ) {
		NFSLOG(0x80, "xdr_attrstat: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], ns->ns_status);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_attrstat: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * Reply status with file attributes for extended (ESV) protocol
 */
bool_t
xdr_esvattrstat(xdrs, ns)
	XDR *xdrs;
	struct nfsesvattrstat *ns;
{

	if (xdr_union(xdrs, (enum_t *)&(ns->ns_status),
	      (caddr_t)&(ns->ns_attr), esvattrstat_discrim, xdr_void) ) {
		NFSLOG(0x80, "xdr_esvattrstat: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], ns->ns_status);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvattrstat: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * NFS_OK part of read sym link reply union
 */
bool_t
xdr_srok(xdrs, srok)
	XDR *xdrs;
	struct nfssrok *srok;
{

	if (xdr_bytes(xdrs, &srok->srok_data, (u_int *)&srok->srok_count,
	    NFS_MAXPATHLEN) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_srok: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * NFS_OK part of read sym link reply union for extended (ESV) protocol
 */
bool_t
xdr_esvsrok(xdrs, srok)
	XDR *xdrs;
	struct nfsesvsrok *srok;
{

	if (xdr_bytes(xdrs, &srok->srok_data, (u_int *)&srok->srok_count,
	    NFS_MAXPATHLEN) &&
	    xdr_esvfattr(xdrs, &srok->srok_attr) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvsrok: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

STATIC struct xdr_discrim rdlnres_discrim[2] = {
	{ (int)NFS_OK, xdr_srok },
	{ __dontcare__, NULL_xdrproc_t }
};

STATIC struct xdr_discrim esvrdlnres_discrim[2] = {
	{ (int)NFS_OK, xdr_esvsrok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Result of reading symbolic link
 */
bool_t
xdr_rdlnres(xdrs, rl)
	XDR *xdrs;
	struct nfsrdlnres *rl;
{

	NFSLOG(0x80, "xdr_rdlnres: %s\n", xdropnames[(int)xdrs->x_op], 0);
	if (xdr_union(xdrs, (enum_t *)&(rl->rl_status),
	      (caddr_t)&(rl->rl_srok), rdlnres_discrim, xdr_void) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_rdlnres: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * Result of reading symbolic link for extended (ESV) protocol
 */
bool_t
xdr_esvrdlnres(xdrs, rl)
	XDR *xdrs;
	struct nfsesvrdlnres *rl;
{

	NFSLOG(0x80, "xdr_esvrdlnres: %s\n", xdropnames[(int)xdrs->x_op], 0);
	if (xdr_union(xdrs, (enum_t *)&(rl->rl_status),
	      (caddr_t)&(rl->rl_srok), esvrdlnres_discrim, xdr_void) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvrdlnres: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * Arguments to readdir
 */
bool_t
xdr_rddirargs(xdrs, rda)
	XDR *xdrs;
	struct nfsrddirargs *rda;
{

	if (xdr_fhandle(xdrs, &rda->rda_fh) &&
	    xdr_u_long(xdrs, &rda->rda_offset) &&
	    xdr_u_long(xdrs, &rda->rda_count) ) {
		NFSLOG(0x80, "xdr_rddirargs: %s off %d, ",
			xdropnames[(int)xdrs->x_op], rda->rda_offset);
		NFSLOG(0x80, "count %d\n", rda->rda_count, 0);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_rddirargs: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);
	return (FALSE);
}

/*
 * Directory read reply:
 * union (enum status) {
 *	NFS_OK: entlist;
 *		boolean eof;
 *	default:
 * }
 *
 * Directory entries
 *	struct  direct {
 *		off_t   d_off;	       	       * offset of next entry *
 *		u_long  d_fileno;	       * inode number of entry *
 *		u_short d_reclen;	       * length of this record *
 *		u_short d_namlen;	       * length of string in d_name *
 *		char    d_name[MAXNAMLEN + 1];  * name no longer than this *
 *	};
 * are on the wire as:
 * union entlist (boolean valid) {
 * 	TRUE: struct otw_dirent;
 *	      u_long nxtoffset;
 * 	      union entlist;
 *	FALSE:
 * }
 * where otw_dirent is:
 * 	struct dirent {
 *		u_long	de_fid;
 *		string	de_name<NFS_MAXNAMELEN>;
 *	}
 */

#define	nextdp(dp)	((struct dirent *)((int)(dp) + (dp)->d_reclen))
#ifdef	SYSV
/* #define MINDIRSIZ(dp)	(sizeof(struct dirent) + strlen((dp)->d_name)) */
/* sizeof(struct dirent) is rounded up, so subtract 1 */
#define MINDIRSIZ(dp)	(sizeof(struct dirent) - 1 + strlen((dp)->d_name))
#else
#define	MINDIRSIZ(dp)	(sizeof(struct dirent) - MAXNAMLEN + (dp)->d_namlen)
#endif

#ifdef	SYSV
#define	d_fileno	d_ino
#endif

/*
 * ENCODE ONLY: common part of putrddirres (both v.2 and ESV protocols)
 */
STATIC bool_t
xdr_commonrddirres(xdrs, rd, origreqsize)
	XDR *xdrs;
	struct nfsrdok *rd;
	u_long origreqsize;
{
	struct dirent *dp;
	char *name;
	int size;
	u_int namlen;
	int xdrpos;
	int lastxdrpos;
	bool_t true = TRUE;
	bool_t false = FALSE;

	xdrpos = XDR_GETPOS(xdrs);
	lastxdrpos = xdrpos;
	for (size = rd->rdok_size, dp = rd->rdok_entries;
	     size > 0;
	     size -= dp->d_reclen, dp = nextdp(dp) ) {
		if (dp->d_reclen == 0 || MINDIRSIZ(dp) > dp->d_reclen) {
			NFSLOG(0x81, "xdr_commonrddirres: bad directory\n", 0, 0);
			return (FALSE);
		}
#ifdef	SYSV
		NFSLOG(0x80, "xdr_commonrddirres: entry %d %s",
			dp->d_fileno, dp->d_name);
		NFSLOG(0x80, "(%d) %d ", strlen(dp->d_name), dp->d_off);
		NFSLOG(0x80, "%d %d ", dp->d_reclen, XDR_GETPOS(xdrs));
		NFSLOG(0x80, "%d\n", size, 0);
#else
		printf("xdr_commonrddirres: entry %d %s(%d) %d %d %d %d\n",
		    dp->d_fileno, dp->d_name, dp->d_namlen, dp->d_off,
		    dp->d_reclen, XDR_GETPOS(xdrs), size);
#endif	/* SYSV */
		if (dp->d_fileno == 0) {
			continue;
		}
		name = dp->d_name;
#ifdef	SYSV
		namlen = strlen(name);
#else
		namlen = dp->d_namlen;
#endif
		if (!xdr_bool(xdrs, &true) ||
		    !xdr_u_long(xdrs, &dp->d_fileno) ||
		    !xdr_bytes(xdrs, &name, &namlen, NFS_MAXNAMLEN) ||
		    !xdr_u_long(xdrs, (u_long *) &dp->d_off) ) {
			return (FALSE);
		}
		if (XDR_GETPOS(xdrs) - xdrpos >= origreqsize -
		    2 * RNDUP(sizeof (bool_t))) {
			XDR_SETPOS(xdrs, lastxdrpos);
			rd->rdok_eof = FALSE;
			break;
		} else
			lastxdrpos = XDR_GETPOS(xdrs);
	}
	if (!xdr_bool(xdrs, &false)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &rd->rdok_eof)) {
		return (FALSE);
	}
	if (XDR_GETPOS(xdrs) - xdrpos >= origreqsize) {
		printf ("xdr_commonrddirres: encoding overrun\n");
		return (FALSE);
	} else
		return (TRUE);
}

/*
 * ENCODE ONLY
 */
bool_t
xdr_putrddirres(xdrs, rd)
	XDR *xdrs;
	struct nfsrddirres *rd;
{
	NFSLOG(0x80, "xdr_putrddirres: %s size %d ",
		xdropnames[(int)xdrs->x_op], rd->rd_size);
	NFSLOG(0x80, "offset %d\n", rd->rd_offset, 0);
	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}
	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}
	return(xdr_commonrddirres(xdrs, &(rd->rd_u.rd_rdok_u), rd->rd_origreqsize));
}

/*
 * ENCODE ONLY - extended (ESV) readdir results
 */
bool_t
xdr_esvputrddirres(xdrs, rd)
	XDR *xdrs;
	struct nfsesvrddirres *rd;
{
	NFSLOG(0x80, "xdr_esvputrddirres: %s size %d ",
		xdropnames[(int)xdrs->x_op], rd->rd_size);
	NFSLOG(0x80, "offset %d\n", rd->rd_offset, 0);
	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}
	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}
	if (!xdr_esvfattr(xdrs, &rd->rd_attr)) {
		return(FALSE);
	}
	return(xdr_commonrddirres(xdrs, (struct nfsrdok *)&((rd)->rd_offset), rd->rd_origreqsize));
}

#define roundtoint(x)	(((x) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#ifdef	SYSV

#define reclen(dp)	roundtoint((strlen((dp)->d_name) + 1 + sizeof(long) +\
				sizeof(unsigned short)))
#undef	DIRSIZ
#define	DIRSIZ(dp,namlen)	\
	(((sizeof (struct dirent) - 1 + (namlen + 1)) + 3) & ~3)

#else

#define reclen(dp)	roundtoint(((dp)->d_namlen + 1 + sizeof(u_long) +\
				2 * sizeof(u_short)))
#undef	DIRSIZ
#define	DIRSIZ(dp)	\
	(((sizeof (struct dirent) - (MAXNAMLEN + 1) + ((dp)->d_namlen+1)) + 3) & ~3)

#endif	/* SYSV */

/*
 * DECODE ONLY
 */
bool_t
xdr_getrddirres(xdrs, rd)
	XDR *xdrs;
	struct nfsrddirres *rd;
{
	struct dirent *dp;
	int size;
	bool_t valid;
	bool_t first = TRUE;
        off_t offset = (off_t)-1;

	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_getrddirres: %s size %d\n",
	    xdropnames[(int)xdrs->x_op], rd->rd_size);
	size = rd->rd_size;
	dp = rd->rd_entries;
	for (;;) {
		if (!xdr_bool(xdrs, &valid)) {
			return (FALSE);
		}
		if (valid) {
#ifdef	SYSV
			u_int namlen;
			u_long tmp_fileno;

			if (!xdr_u_long(xdrs, &tmp_fileno) ||
			    !xdr_u_int(xdrs, &namlen)) {
				return (FALSE);
			} else {
				dp->d_fileno = tmp_fileno;
			}

			if (DIRSIZ(dp, namlen) > size) {
				/*
				 *	Entry won't fit.  If this isn't the
				 *	first one, just quit and return what
				 *	we've already XDR-ed.  Else, it's an
				 *	error.
				 */
				if (first == FALSE) {
					rd->rd_eof = FALSE;
					rd->rd_size = (int)dp -
							(int)(rd->rd_entries);
					rd->rd_offset = offset;
					return (TRUE);
				} else {
					return (FALSE);
				}
			}
			if (!xdr_opaque(xdrs, dp->d_name, namlen)||
			    !xdr_u_long(xdrs, (u_long *) &dp->d_off) ) {
				NFSLOG(0x81, "xdr_getrddirres: entry error\n",0,0);
				return (FALSE);
			}
			dp->d_reclen = DIRSIZ(dp, namlen);
			dp->d_name[namlen] = '\0';
			offset = dp->d_off;
			first = FALSE;
			NFSLOG(0x80, "xdr_getrddirres: entry %d %s(%d) %d %d\n",
			    dp->d_fileno, dp->d_name);
			NFSLOG(0x80, "(%d) %d ", namlen, dp->d_reclen);
			NFSLOG(0x80, "%d\n", dp->d_off, 0);

#else	/* SYSV */
			if (!xdr_u_long(xdrs, &dp->d_fileno) ||
			    !xdr_u_short(xdrs, &dp->d_namlen) ||
			    (DIRSIZ(dp) > size) ||
			    !xdr_opaque(xdrs, dp->d_name, (u_int)dp->d_namlen)||
			    !xdr_u_long(xdrs, (u_long *) &dp->d_off) ) {
				NFSLOG(0x81, "xdr_getrddirres: entry error\n",0,0);
				return (FALSE);
			}
			dp->d_reclen = DIRSIZ(dp);
			dp->d_name[dp->d_namlen] = '\0';
			offset = dp->d_off;
			NFSLOG(0x80, "xdr_getrddirres: entry %d %s",
				dp->d_fileno, dp->d_name);
			NFSLOG(0x80, "(%d) %d ", dp->d_namlen, dp->d_reclen);
			NFSLOG(0x80, "%d\n", dp->d_off, 0);
#endif	/* SYSV */
		} else {
			break;
		}
		size -= reclen(dp);
		if (size <= 0) {
			return (FALSE);
		}
		dp = nextdp(dp);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	rd->rd_size = (int)dp - (int)(rd->rd_entries);
	rd->rd_offset = offset;
	NFSLOG(0x80, "xdr_getrddirres: returning size %d offset %d ",
		rd->rd_size, rd->rd_offset);
	NFSLOG(0x80, "eof %d\n", rd->rd_eof, 0);
	return (TRUE);
}

/*
 * DECODE ONLY - for extended (ESV) protocol
 */
bool_t
xdr_esvgetrddirres(xdrs, rd)
	XDR *xdrs;
	struct nfsesvrddirres *rd;
{
	struct dirent *dp;
	int size;
	bool_t valid;
	bool_t first = TRUE;
        off_t offset = (off_t)-1;

	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}
	if (!xdr_esvfattr(xdrs, &rd->rd_attr)) {
		return(FALSE);
	}

	NFSLOG(0x80, "xdr_esvgetrddirres: %s size %d\n",
	    xdropnames[(int)xdrs->x_op], rd->rd_size);
	size = rd->rd_size;
	dp = rd->rd_entries;
	for (;;) {
		if (!xdr_bool(xdrs, &valid)) {
			return (FALSE);
		}
		if (valid) {
#ifdef	SYSV
			u_int namlen;
			u_long tmp_fileno;

			if (!xdr_u_long(xdrs, &tmp_fileno) ||
			    !xdr_u_int(xdrs, &namlen)) {
				return (FALSE);
			} else {
				dp->d_fileno = tmp_fileno;
			}

			if (DIRSIZ(dp, namlen) > size) {
				/*
				 *	Entry won't fit.  If this isn't the
				 *	first one, just quit and return what
				 *	we've already XDR-ed.  Else, it's an
				 *	error.
				 */
				if (first == FALSE) {
					rd->rd_eof = FALSE;
					rd->rd_size = (int)dp -
							(int)(rd->rd_entries);
					rd->rd_offset = offset;
					return (TRUE);
				} else {
					return (FALSE);
				}
			}
			if (!xdr_opaque(xdrs, dp->d_name, namlen)||
			    !xdr_u_long(xdrs, (u_long *) &dp->d_off) ) {
				NFSLOG(0x81, "xdr_esvgetrddirres: entry error\n",0,0);
				return (FALSE);
			}
			dp->d_reclen = DIRSIZ(dp, namlen);
			dp->d_name[namlen] = '\0';
			offset = dp->d_off;
			first = FALSE;
			NFSLOG(0x80, "xdr_esvgetrddirres: entry %d %s\n",
			    dp->d_fileno, dp->d_name);
			NFSLOG(0x80, "(%d) %d ", namlen, dp->d_reclen);
			NFSLOG(0x80, "%d\n", dp->d_off, 0);

#else	/* SYSV */
			if (!xdr_u_long(xdrs, &dp->d_fileno) ||
			    !xdr_u_short(xdrs, &dp->d_namlen) ||
			    (DIRSIZ(dp) > size) ||
			    !xdr_opaque(xdrs, dp->d_name, (u_int)dp->d_namlen)||
			    !xdr_u_long(xdrs, (u_long *) &dp->d_off) ) {
				NFSLOG(0x81, "xdr_esvgetrddirres: entry error\n",0,0);
				return (FALSE);
			}
			dp->d_reclen = DIRSIZ(dp);
			dp->d_name[dp->d_namlen] = '\0';
			offset = dp->d_off;
			NFSLOG(0x80, "xdr_esvgetrddirres: entry %d %s",
				dp->d_fileno, dp->d_name);
			NFSLOG(0x80, "(%d) %d ", dp->d_namlen, dp->d_reclen);
			NFSLOG(0x80, "%d\n", dp->d_off, 0);
#endif	/* SYSV */
		} else {
			break;
		}
		size -= reclen(dp);
		if (size <= 0) {
			return (FALSE);
		}
		dp = nextdp(dp);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	rd->rd_size = (int)dp - (int)(rd->rd_entries);
	rd->rd_offset = offset;
	NFSLOG(0x80, "xdr_esvgetrddirres: returning size %d offset %d ",
		rd->rd_size, rd->rd_offset);
	NFSLOG(0x80, "eof %d\n", rd->rd_eof, 0);
	return (TRUE);
}

/*
 * Arguments for directory operations
 */
bool_t
xdr_diropargs(xdrs, da)
	XDR *xdrs;
	struct nfsdiropargs *da;
{

	if (xdr_fhandle(xdrs, &da->da_fhandle) &&
	    xdr_string(xdrs, &da->da_name, NFS_MAXNAMLEN) ) {
		NFSLOG(0x80, "xdr_diropargs: %s '%s'\n",
		    xdropnames[(int)xdrs->x_op], da->da_name);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_diropargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * NFS_OK part of directory operation result
 */
bool_t
xdr_drok(xdrs, drok)
	XDR *xdrs;
	struct nfsdrok *drok;
{

	if (xdr_fhandle(xdrs, &drok->drok_fhandle) &&
	    xdr_fattr(xdrs, &drok->drok_attr) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_drok: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * NFS_OK part of directory operation result for extended (ESV) protocol
 */
bool_t
xdr_esvdrok(xdrs, drok)
	XDR *xdrs;
	struct nfsesvdrok *drok;
{

	if (xdr_fhandle(xdrs, &drok->drok_fhandle) &&
	    xdr_esvfattr(xdrs, &drok->drok_attr) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_drok: FAILED\n", 0, 0);
	return (FALSE);
}

STATIC struct xdr_discrim diropres_discrim[2] = {
	{ (int)NFS_OK, xdr_drok },
	{ __dontcare__, NULL_xdrproc_t }
};

STATIC struct xdr_discrim esvdiropres_discrim[2] = {
	{ (int)NFS_OK, xdr_esvdrok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Results from directory operation 
 */
bool_t
xdr_diropres(xdrs, dr)
	XDR *xdrs;
	struct nfsdiropres *dr;
{

	if (xdr_union(xdrs, (enum_t *)&(dr->dr_status),
	      (caddr_t)&(dr->dr_drok), diropres_discrim, xdr_void) ) {
		NFSLOG(0x80, "xdr_diropres: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], dr->dr_status);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_diropres: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * Results from directory operation for extended (ESV) protocol
 */
bool_t
xdr_esvdiropres(xdrs, dr)
	XDR *xdrs;
	struct nfsesvdiropres *dr;
{

	if (xdr_union(xdrs, (enum_t *)&(dr->dr_status),
	      (caddr_t)&(dr->dr_drok), esvdiropres_discrim, xdr_void) ) {
		NFSLOG(0x80, "xdr_diropres: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], dr->dr_status);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvdiropres: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * Time structure
 */
bool_t
xdr_timeval(xdrs, tv)
	XDR *xdrs;
	struct timeval *tv;
{

	if (xdr_long(xdrs, &tv->tv_sec) &&
	    xdr_long(xdrs, &tv->tv_usec) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_timeval: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to setattr
 */
bool_t
xdr_saargs(xdrs, argp)
	XDR *xdrs;
	struct nfssaargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->saa_fh) &&
	    xdr_sattr(xdrs, &argp->saa_sa) ) {
		NFSLOG(0x80, "xdr_saargs: %s\n", xdropnames[(int)xdrs->x_op], 0);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_saargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to setattr for extended (ESV) protocol
 */
bool_t
xdr_esvsaargs(xdrs, argp)
	XDR *xdrs;
	struct nfsesvsaargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->saa_fh) &&
	    xdr_esvsattr(xdrs, &argp->saa_sa) ) {
		NFSLOG(0x80, "xdr_esvsaargs: %s\n", xdropnames[(int)xdrs->x_op], 0);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvsaargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to create and mkdir
 */
bool_t
xdr_creatargs(xdrs, argp)
	XDR *xdrs;
	struct nfscreatargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->ca_da) &&
	    xdr_sattr(xdrs, &argp->ca_sa) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_creatargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to create and mkdir for extended (ESV) protocol
 */
bool_t
xdr_esvcreatargs(xdrs, argp)
	XDR *xdrs;
	struct nfsesvcreatargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->ca_da) &&
	    xdr_esvsattr(xdrs, &argp->ca_sa) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvcreatargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to link
 */
bool_t
xdr_linkargs(xdrs, argp)
	XDR *xdrs;
	struct nfslinkargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->la_from) &&
	    xdr_diropargs(xdrs, &argp->la_to) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_linkargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to rename
 */
bool_t
xdr_rnmargs(xdrs, argp)
	XDR *xdrs;
	struct nfsrnmargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->rna_from) &&
	    xdr_diropargs(xdrs, &argp->rna_to) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_rnmargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to symlink
 */
bool_t
xdr_slargs(xdrs, argp)
	XDR *xdrs;
	struct nfsslargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->sla_from) &&
	    xdr_string(xdrs, &argp->sla_tnm, (u_int)MAXPATHLEN) &&
	    xdr_sattr(xdrs, &argp->sla_sa) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_slargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to symlink for extended (ESV) protocol
 */
bool_t
xdr_esvslargs(xdrs, argp)
	XDR *xdrs;
	struct nfsesvslargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->sla_from) &&
	    xdr_string(xdrs, &argp->sla_tnm, (u_int)MAXPATHLEN) &&
	    xdr_esvsattr(xdrs, &argp->sla_sa) ) {
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_esvslargs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * NFS_OK part of statfs operation
 */
bool_t
xdr_fsok(xdrs, fsok)
	XDR *xdrs;
	struct nfsstatfsok *fsok;
{

	if (xdr_long(xdrs, (long *)&fsok->fsok_tsize) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bsize) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_blocks) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bfree) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bavail) ) {
		NFSLOG(0x80, "xdr_fsok: %s tsz %d ",
			xdropnames[(int)xdrs->x_op], fsok->fsok_tsize);
		NFSLOG(0x80, "bsz %d blks %d ",
			fsok->fsok_bsize, fsok->fsok_blocks);
		NFSLOG(0x80, "bfree %d bavail %d\n",
			fsok->fsok_bfree, fsok->fsok_bavail);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_fsok: FAILED\n", 0, 0);
	return (FALSE);
}

STATIC struct xdr_discrim statfs_discrim[2] = {
	{ (int)NFS_OK, xdr_fsok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Results of statfs operation
 */
bool_t
xdr_statfs(xdrs, fs)
	XDR *xdrs;
	struct nfsstatfs *fs;
{

	if (xdr_union(xdrs, (enum_t *)&(fs->fs_status),
	      (caddr_t)&(fs->fs_fsok), statfs_discrim, xdr_void) ) {
		NFSLOG(0x80, "xdr_statfs: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], fs->fs_status);
		return (TRUE);
	}
	NFSLOG(0x81, "xdr_statfs: FAILED\n", 0, 0);
	return (FALSE);
}

/*
 * arguments to access operation (extended protocol only)
 */
bool_t
xdr_accessargs(xdrs, argp)
	XDR *xdrs;
	struct nfsaccessargs *argp;
{
	if (xdr_fhandle(xdrs, &argp->acc_fhandle) &&
	    xdr_u_long(xdrs, &argp->acc_flag) ) {
		return(TRUE);
	}
	NFSLOG(0x81, "xdr_accessargs: FAILED\n", 0, 0);
	return(FALSE);
}

/*
 * NFS_OK part of access operation
 */
bool_t
xdr_accessok(xdrs, accok)
	XDR *xdrs;
	struct nfsaccessok *accok;
{
	if (xdr_bool(xdrs, &accok->accok_status) &&
	    xdr_esvfattr(xdrs, &accok->accok_attr) ) {
		NFSLOG(0x80, "xdr_accessok: %s stat %d\n",
			xdropnames[(int)xdrs->x_op], accok->accok_status);
		return(TRUE);
	}
	NFSLOG(0x80, "xdr_accessok: FAILED\n", 0, 0);
	return(FALSE);
}

STATIC struct xdr_discrim accessres_discrim[2] = {
	{ (int)NFS_OK, xdr_accessok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Results of access operation
 */
bool_t
xdr_accessres(xdrs, accrs)
	XDR *xdrs;
	struct nfsaccessres *accrs;
{
	if (xdr_union(xdrs, (enum_t *)&(accrs->acc_status),
	      (caddr_t)&(accrs->acc_accok), accessres_discrim, xdr_void) ) {
		NFSLOG(0x80, "xdr_accessres: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], accrs->acc_status);
		return(TRUE);
	}
	NFSLOG(0x81, "xdr_accessres: FAILED\n", 0, 0);
	return(FALSE);
}
