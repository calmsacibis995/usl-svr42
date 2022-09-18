/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/nfs/nfs_common.c	1.5.2.2"
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

#include <svc/errno.h>
#include <util/param.h>
#include <util/types.h>
#include <acc/dac/acl.h>
#include <proc/user.h>
#include <fs/stat.h>
#include <svc/time.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <net/rpc/types.h>
#include <net/rpc/token.h>
#include <fs/nfs/nfs.h>
#include <fs/mode.h>
#include <util/cmn_err.h>

/*
 * General utilities
 */

/*
 * Returns the prefered transfer size in bytes based on
 * what network interfaces are available.
 */
int
nfstsize()
{
#ifdef	SYSV
	/*
	 *	Unfortunately, the networking architecture in System V doesn't
	 *	allow us to ask this question of the network interfaces.
	 */
	return (NFS_MAXDATA);
#else
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_name[0] == 'e' && ifp->if_name[1] == 'c') {
			printf("nfstsize: %d\n", ECTSIZE);
			return (ECTSIZE);
		}
	}
	printf("nfstsize: %d\n", IETSIZE);
	return (IETSIZE);
#endif
}

/*
 * Utilities used by both client and server.
 */

void
vattr_to_nattr(vap, na)
	register struct vattr *vap;
	register struct nfsfattr *na;
{
	na->na_type = (enum nfsftype)vap->va_type;

	if (vap->va_mode == (unsigned short) -1)
                na->na_mode = (unsigned long) -1;
        else
                na->na_mode = VTTOIF(vap->va_type) | vap->va_mode;

        if (vap->va_uid == (unsigned short) -1)
                na->na_uid = (unsigned long) -1;
        else
                na->na_uid = vap->va_uid;
 
        if (vap->va_gid == (unsigned short) -1)
                na->na_gid = (unsigned long) -1;
        else
                na->na_gid = vap->va_gid;

	na->na_fsid = vap->va_fsid;
	na->na_nodeid = vap->va_nodeid;
	na->na_nlink = vap->va_nlink;
	na->na_size = vap->va_size;
	na->na_atime.tv_sec  = vap->va_atime.tv_sec;
	na->na_atime.tv_usec = vap->va_atime.tv_nsec/1000;
	na->na_mtime.tv_sec  = vap->va_mtime.tv_sec;
	na->na_mtime.tv_usec = vap->va_mtime.tv_nsec/1000;
	na->na_ctime.tv_sec  = vap->va_ctime.tv_sec;
	na->na_ctime.tv_usec = vap->va_ctime.tv_nsec/1000;
	na->na_rdev = vap->va_rdev;
	na->na_blocks = vap->va_nblocks;
	na->na_blocksize = vap->va_blksize;

	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * VFIFO type to the special over-the-wire type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-Sun server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (vap->va_type == VFIFO)
		NA_SETFIFO(na);
}

void
vattr_to_esvnattr(vap, na, addr, lidp, aclp, nacl)
	register struct vattr *vap;
	register struct nfsesvfattr *na;
	register struct netbuf *addr;
	register lid_t *lidp;
	register struct acl *aclp;	/* ptr to ACL entries buffer */
	register u_int nacl;		/* number of ACL entries present */
{
	na->na_type = (enum nfsftype)vap->va_type;

	if (vap->va_mode == (unsigned short) -1)
                na->na_mode = (unsigned long) -1;
        else
                na->na_mode = VTTOIF(vap->va_type) | vap->va_mode;

        if (vap->va_uid == (unsigned short) -1)
                na->na_uid = (unsigned long) -1;
        else
                na->na_uid = vap->va_uid;
 
        if (vap->va_gid == (unsigned short) -1)
                na->na_gid = (unsigned long) -1;
        else
                na->na_gid = vap->va_gid;

	na->na_fsid = vap->va_fsid;
	na->na_nodeid = vap->va_nodeid;
	na->na_nlink = vap->va_nlink;
	na->na_size = vap->va_size;
	na->na_atime.tv_sec  = vap->va_atime.tv_sec;
	na->na_atime.tv_usec = vap->va_atime.tv_nsec/1000;
	na->na_mtime.tv_sec  = vap->va_mtime.tv_sec;
	na->na_mtime.tv_usec = vap->va_mtime.tv_nsec/1000;
	na->na_ctime.tv_sec  = vap->va_ctime.tv_sec;
	na->na_ctime.tv_usec = vap->va_ctime.tv_nsec/1000;
	na->na_rdev = vap->va_rdev;
	na->na_blocks = vap->va_nblocks;
	na->na_blocksize = vap->va_blksize;

	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * VFIFO type to the special over-the-wire type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-Sun server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (vap->va_type == VFIFO)
		NA_SETFIFO(na);

	na->na_privs = (s_token)0;
	na->na_sens = get_remote_token(addr, SENS_T, (caddr_t)lidp, sizeof(lid_t));
	na->na_info = (s_token)0;
	na->na_integ = (s_token)0;
	na->na_ncs = (s_token)0;
	na->na_acl = get_remote_token(addr, ACL_T, (caddr_t)aclp, nacl * sizeof(struct acl));
}

#ifdef DEBUG
/*
 * NFS kernel debugging tool. Nfslog is a bitmask specifying the types
 * of traces wanted:
 *
 *	0x001	errors
 *	0x002	client procedure entries
 *	0x004	client procedure exits
 *	0x008	client misc
 *	0x010	server procedure entries
 *	0x020	server procedure exits
 *	0x040	server misc
 *	0x080	XDR misc (non-error)
 *
 *	Can/will be expanded...
 */

int nfslog = 0;

int
nfs_log(level, str, a1, a2)
	ulong		level;
	register char	*str;
	register int	a1, a2;
{
	if (level & nfslog) {
		cmn_err(CE_CONT, str, a1, a2);
		dodmddelay();
	}
	return(0);
}
#endif /* DEBUG */
