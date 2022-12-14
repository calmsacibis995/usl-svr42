/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifndef _FS_PATHNAME_H	/* wrapper symbol for kernel use */
#define _FS_PATHNAME_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/pathname.h	1.4"
/*
 * Pathname structure.
 * System calls that operate on path names gather the path name
 * from the system call into this structure and reduce it by
 * peeling off translated components.  If a symbolic link is
 * encountered the new path name to be translated is also
 * assembled in this structure.
 *
 * By convention pn_buf is not changed once it's been set to point
 * to the underlying storage; routines which manipulate the pathname
 * do so by changing pn_path and pn_pathlen.  pn_pathlen is redundant
 * since the path name is null-terminated, but is provided to make
 * some computations faster.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef struct pathname {
	char	*pn_buf;		/* underlying storage */
	char	*pn_path;		/* remaining pathname */
	u_int	pn_pathlen;		/* remaining length */
} pathname_t;

#define PN_STRIP	0	/* Strip next component from pn */
#define PN_PEEK		1	/* Only peek at next component of pn */
#define pn_peekcomponent(pnp, comp) pn_getcomponent(pnp, comp, PN_PEEK)
#define pn_stripcomponent(pnp, comp) pn_getcomponent(pnp, comp, PN_STRIP)

#define	pn_peekchar(pnp)	((pnp)->pn_pathlen > 0 ? *((pnp)->pn_path) : 0)
#define pn_pathleft(pnp)	((pnp)->pn_pathlen)

extern void	pn_alloc();		/* allocate buffer for pathname */
extern int	pn_get();		/* allocate buffer, copy path into it */
extern int	pn_set();		/* set pathname to string */
extern int	pn_insert();		/* combine two pathnames (symlink) */
extern int	pn_getsymlink();	/* get symlink into pathname */
extern int	pn_getcomponent();	/* get next component of pathname */
extern void	pn_setlast();		/* set pathname to last component */
extern void	pn_skipslash();		/* skip over slashes */
extern void	pn_fixslash();		/* eliminate trailing slashes */
extern void	pn_free();		/* free pathname buffer */

extern int	lookupname();		/* convert name to vnode */
extern int	lookuppn();		/* convert pathname buffer to vnode */
extern int	traverse();		/* traverse a mount point */

/*
 * Macro to avoid function call to pn_skipslash().
 */
#define	PN_SKIPSLASH(pnp) 						\
	while ((pnp)->pn_pathlen > 0 && *(pnp)->pn_path == '/') {	\
		(pnp)->pn_path++;					\
		(pnp)->pn_pathlen--;					\
	}

#endif	/* _FS_PATHNAME_H */
