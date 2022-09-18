/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FBUF_H	/* wrapper symbol for kernel use */
#define _FS_FBUF_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/fbuf.h	1.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * A struct fbuf is used to get a mapping to part of a file using the
 * segkmap facilities.  After you get a mapping, you can fbrelse() it
 * (giving a seg code to pass back to segmap_release), you can fbwrite()
 * it (causes a synchronous write back using the file mapping information),
 * or you can fbiwrite it (causing indirect synchronous write back to
 * the block number given without using the file mapping information).
 */

struct fbuf {
	addr_t	fb_addr;
	u_int	fb_count;
};

extern int fbread(/* vp, off, len, rw, fbpp */);
extern void fbzero(/* vp, off, len, fbpp */);
extern int fbwrite(/* fbp */);
extern int fbiwrite(/* fbp, vp, bn, bsize */);
extern void fbrelse(/* fbp, rw */);

#endif	/* _FS_FBUF_H */
