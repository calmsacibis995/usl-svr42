/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_USTAT_H	/* wrapper symbol for kernel use */
#define _FS_USTAT_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/ustat.h	1.4"
#ident	"$Header: $"

/* WARNING: The ustat system call will become obsolete in the
** next major release following SVR4. Application code should
** migrate to the replacement system call statvfs(2).
*/

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#else

#include <sys/types.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

struct  ustat {
	daddr_t	f_tfree;	/* total free */
	o_ino_t	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};

#endif /* _FS_USTAT_H */
