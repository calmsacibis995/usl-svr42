/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_MOUNT_H	/* wrapper symbol for kernel use */
#define _FS_MOUNT_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/mount.h	1.4"
#ident	"$Header: $"

/*
 * Flags bits passed to mount(2).
 */
#define	MS_RDONLY	0x01	/* read-only */
#define	MS_FSS		0x02	/* old (4-argument) mount (compatibility) */
#define	MS_DATA		0x04	/* 6-argument mount */
#define	MS_NOSUID	0x10	/* setuid programs disallowed */
#define MS_REMOUNT	0x20	/* remount */
#define MS_NOTRUNC	0x40	/* return ENAMETOOLONG for long filenames */

/*
 * Exit codes for file system specific mount commands.
 * The codes from 32-55 are mainly common errors.
 * The rest are specific to the file systems.
 */
#define	RET_OK		0	/* success */
#define	RET_USAGE	32	/* usage */

#define	RET_EBUSY	33	/* mount() - is already mounted */
#define	RET_EPERM	34	/* mount() - permission denied */
#define	RET_ENXIO	35	/* mount() - no such device */
#define	RET_ENOTDIR	36	/* mount() - not a directory */
#define	RET_ENOENT	37	/* mount() - no such file or directory */
#define	RET_EINVAL	38	/* mount() - is not a <fs> file system */
#define	RET_ENOTBLK	39	/* mount() - not a block device */
#define	RET_EROFS	40	/* mount() - write-protected */
#define	RET_ENOSPC	41	/* mount() - is corrupted. needs checking */
#define	RET_ENOLOAD	42	/* mount() - file system module cannot be loaded */
#define	RET_ENODEV	43	/* mount() - no such device  or write-protected */
#define	RET_MISC	44	/* mount() - misc errors */

#define	RET_TMP_OPEN	45	/* cannot open temp file */
#define	RET_TMP_WRITE	46	/* cannot write to temp file */

#define	RET_MNT_OPEN	47	/* cannot open mnttab */
#define	RET_MNT_LOCK	48	/* cannot lock mnttab */
#define	RET_MNT_TOOLONG	49	/* line in mnttab exceeds <max> characters */
#define	RET_MNT_TOOFEW	50	/* line in mnttab has too few entries */
#define	RET_MNT_TOOMANY	51	/* line in mnttab has too many entries */

#define	RET_VFS_OPEN	52	/* cannot open vfstab */
#define	RET_VFS_STAT	53	/* cannot stat vfstab */
#define	RET_VFS_NOENT	54	/* entry not in vfstab */

#define	RET_MALLOC	55	/* malloc() error - no more memory */

/* sfs and ufs specific */
#define	RET_REALPATH	61	/* realpath() error */
#define	RET_ABS_PATH	62	/* directory path must begin with "/" */

/* ufs specific */
#define	RET_O_OPTION	65	/* cannot specify both -o f and -o n */

/* nfs specific */
#define	RET_RETRY	70	
#define	RET_GIVE_UP	71	/* gave up retrying */
#define	RET_HP		72	/* specify host:path */
#define	RET_INVOP	73	/* invalid option */
#define	RET_SERV	74	/* server not responding */
#define	RET_ADDR	75	/* could not get nfs service addr */
#define	RET_SEC		76	/* could not negotiate secure protocol */
#define	RET_ACCESS	77	/* access denied */
#define	RET_NOENT	78	/* no such directory */


#if defined(__STDC__) && !defined(_KERNEL)
int mount(const char *, const char *, int, ...);
int umount(const char *);
#endif

#endif	/* _FS_MOUNT_H */
