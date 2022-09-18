/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_STATVFS_H	/* wrapper symbol for kernel use */
#define _FS_STATVFS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/statvfs.h	1.4"
#ident	"$Header: $"
/*
 * Structure returned by statvfs(2).
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	FSTYPSZ	16

typedef struct statvfs {
	u_long	f_bsize;	/* file system block size */
	u_long	f_frsize;	/* fundamental fs block (fragment) size */
	u_long	f_blocks;	/* total # of blocks of f_frsize on fs */
	u_long	f_bfree;	/* total # of free blocks of f_frsize */
	u_long	f_bavail;	/* # of free blocks avail to unprivileged process */
	u_long	f_files;	/* total # of file nodes (inodes) */
	u_long	f_ffree;	/* total # of free file nodes */
	u_long	f_favail;	/* # of free nodes avail to unprivileged process */
	u_long	f_fsid;		/* file system id (dev for now) */
	char	f_basetype[FSTYPSZ]; /* target fs type name, null-terminated */
	u_long	f_flag;		/* bit-mask of flags */
	u_long	f_namemax;	/* maximum file name length */
	char	f_fstr[32];	/* filesystem-specific string */
	u_long	f_filler[16];	/* reserved for future expansion */
} statvfs_t;

/*
 * Flag definitions.
 */

#define	ST_RDONLY	0x01	/* read-only file system */
#define	ST_NOSUID	0x02	/* does not support setuid/setgid semantics */
#define ST_NOTRUNC	0x04	/* does not truncate long file names */

#if defined(__STDC__) && !defined(_KERNEL)
int statvfs(const char *, struct statvfs *);
int fstatvfs(int, struct statvfs *);
#endif

#endif	/* _FS_STATVFS_H */
