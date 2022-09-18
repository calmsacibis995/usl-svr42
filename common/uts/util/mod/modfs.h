/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MODFS_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODFS_H	/* subject to change without notice */

#ident	"@(#)uts-comm:util/mod/modfs.h	1.5"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _FS_VFS_H
#include <fs/vfs.h>
#endif

#elif defined(_KERNEL)

#include <sys/vfs.h>

#endif /* _KERNEL_HEADERS */

extern struct modctl *mod_shadowvfssw[];
extern const char *mod_fsname(unsigned int);

struct mod_fs_data {
	char * mfd_name;
	struct vfsops *mfd_vfsops;
	unsigned long *mfd_fsflags;
};

#endif	/* _UTIL_MOD_MODFS_H */
