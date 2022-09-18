/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5FBLK_H	/* wrapper symbol for kernel use */
#define _FS_S5FS_S5FBLK_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/s5fs/s5fblk.h	1.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _FS_S5FS_S5PARAM_H
#include <fs/s5fs/s5param.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/fs/s5param.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct	fblk
{
	int	df_nfree;
	daddr_t	df_free[NICFREE];
};

#endif	/* _FS_S5FS_S5FBLK_H */
