/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NAMEFS_NAMENODE_H	/* wrapper symbol for kernel use */
#define _FS_NAMEFS_NAMENODE_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/namefs/namenode.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>		/* REQUIRED */
#endif

#ifndef _ACC_DAC_ACL_H
#include <acc/dac/acl.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/acl.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * This structure is used to pass a file descriptor from user
 * level to the kernel. It is first used by fattach() and then
 * by NAMEFS.
 */
struct namefd {
	int fd;
};

/*
 * Each NAMEFS object is identified by a struct namenode/vnode pair.
 */
struct namenode {
	struct vnode    nm_vnode;	/* represents mounted file desc.*/
	ushort		nm_flag;	/* flags defined below */
	struct vattr    nm_vattr;	/* attributes of mounted file desc.*/
	struct vnode	*nm_filevp;	/* file desc. prior to mounting */
	struct file	*nm_filep;	/* file pointer of nm_filevp */
	struct vnode	*nm_mountpt;	/* mount point prior to mounting */
	struct namenode *nm_nextp;	/* next link in the linked list */
	struct namenode *nm_backp;	/* back link in linked list */
	long		nm_aclcnt;	/* ACL count */
	long		nm_daclcnt;	/* default ACL count */
	struct acl	*nm_aclp;	/* ACL entries */
};

/*
 * Valid flags for namenodes.
 */
#define NMLOCK        01	/* the namenode is locked */
#define NMWANT        02	/* a process wants the namenode */


/*
 * Constants.
 */
#define	NMBSIZE		1024	/* NAMEFS block size */
#define	NMFSIZE		1024	/* NAMEFS fundamental block size */

/*
 * Macros to convert a vnode to a namenode, and vice versa.
 */
#define VTONM(vp) ((struct namenode *)((vp)->v_data))
#define NMTOV(nm) (&(nm)->nm_vnode)

extern struct namenode *namefind();

#endif	/* _FS_NAMEFS_NAMENODE_H */
