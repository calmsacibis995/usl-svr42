/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_LOCKMGR_LOCKMGR_H	/* wrapper symbol for kernel use */
#define _NET_LOCKMGR_LOCKMGR_H	/* subject to change without notice */

#ident	"@(#)uts-x86:net/lockmgr/lockmgr.h	1.5"
#ident	"$Header: $"

/*
 * Header file for Kernel<->Network Lock-Manager implementation
 */

#ifdef _KERNEL_HEADERS

#ifndef _FS_VFS_H
#include <fs/vfs.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/vfs.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef DEBUG

#define		LOCKMGRLOG(A, B, C) ((void)((lockmgrlog) && lockmgr_log((A), (B), (C))))
#else
#define		LOCKMGRLOG(A, B, C)

#endif

/*
 * NOTE: size of a lockhandle-id should track
 * the size of an nfs fhandle
 */
#define KLM_LHSIZE	32

/*
 * the lockhandle uniquely describes any file in a domain
 */
typedef struct {
	struct vnode *lh_vp;			/* vnode of file */
	char *lh_servername;			/* file server machine name */
	struct {
		struct __lh_ufsid {
			fsid_t		__lh_fsid;
			struct fid	__lh_fid;
		} __lh_ufs;
#define KLM_LHPAD	(KLM_LHSIZE - sizeof (struct __lh_ufsid))
		char	__lh_pad[KLM_LHPAD];
	} lh_id;
} lockhandle_t;
#define lh_fsid	lh_id.__lh_ufs.__lh_fsid
#define lh_fid	lh_id.__lh_ufs.__lh_fid

/*
 * define 'well-known' information
 */
#define KLM_PROTO	IPPROTO_UDP

/*
 * define public routines
 */
int  klm_lockctl();

#endif /* _NET_LOCKMGR_LOCKMGR_H */
