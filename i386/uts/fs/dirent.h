/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_DIRENT_H	/* wrapper symbol for kernel use */
#define _FS_DIRENT_H	/* subject to change without notice */
#define _SYS_DIRENT_H

#ident	"@(#)uts-x86:fs/dirent.h	1.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * File-system independent directory entry.
 */
struct dirent {
	ino_t		d_ino;		/* "inode number" of entry */
	off_t		d_off;		/* offset of disk directory entry */
	unsigned short	d_reclen;	/* length of this record */
	char		d_name[1];	/* name of file */
};

typedef	struct	dirent	dirent_t;

#if !defined(_POSIX_SOURCE) 
#if defined(__STDC__) && !defined(_KERNEL)
int getdents(int, struct dirent *, unsigned);
#else
int getdents( );
#endif
#endif /* !defined(_POSIX_SOURCE) */ 

#endif	/* _FS_DIRENT_H */
