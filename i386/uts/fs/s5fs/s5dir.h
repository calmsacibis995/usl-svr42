/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5DIR_H	/* wrapper symbol for kernel use */
#define _FS_S5FS_S5DIR_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/s5fs/s5dir.h	1.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifndef	DIRSIZ
#define	DIRSIZ	14
#endif
struct	direct
{
	o_ino_t	d_ino;		/* s5 inode type */
	char	d_name[DIRSIZ];
};

#define	SDSIZ	(sizeof(struct direct))

#endif	/* _FS_S5FS_S5DIR_H */
