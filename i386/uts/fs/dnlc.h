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

#ifndef _FS_DNLC_H	/* wrapper symbol for kernel use */
#define _FS_DNLC_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/dnlc.h	1.6"
#ident	"$Header: $"
/*
 * This structure describes the elements in the cache of recent
 * names looked up.
 */

#ifdef _KERNEL_HEADERS

#ifndef _FS_VNODE_H
#include <fs/vnode.h>	/* REQUIRED */
#endif

#ifndef _PROC_CRED_H
#include <proc/cred.h>	/* REQUIRED */
#endif

#ifndef _FS_VFS_H
#include <fs/vfs.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/vnode.h>	/* REQUIRED */
#include <sys/cred.h>	/* REQUIRED */
#include <sys/vfs.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	NC_NAMLEN	15	/* maximum name segment length we bother with */

struct ncache {
	struct ncache *hash_next; 	/* hash chain, MUST BE FIRST */
	struct ncache *hash_prev;
	struct ncache *lru_next; 	/* LRU chain */
	struct ncache *lru_prev;
	struct vnode *vp;		/* vnode the name refers to */
	struct vnode *dp;		/* vnode of parent of name */
	char namlen;			/* length of name */
	char name[NC_NAMLEN];		/* segment name */
	struct cred *cred;		/* credentials */
	lid_t lid;			/* MAC level */
};

/*
 * Stats on usefulness of name cache.
 */
struct ncstats {
	int	hits;		/* hits that we can really use */
	int	misses;		/* cache misses */
	int	enters;		/* number of enters done */
	int	dbl_enters;	/* number of enters tried when already cached */
	int	long_enter;	/* long names tried to enter */
	int	long_look;	/* long names tried to look up */
	int	lru_empty;	/* LRU list empty */
	int	purges;		/* number of purges of cache */
};

#define	ANYCRED	((cred_t *) -1)
#define	NOCRED	((cred_t *) 0)

struct nc_hash {
        struct ncache *hash_next;
        struct ncache *hash_prev;
};

/*
 * External routines.
 */

#if defined(__STDC__)

void	dnlc_init(void);
void	dnlc_enter(vnode_t *, char *, vnode_t *, cred_t *);
vnode_t	*dnlc_lookup(vnode_t *, char *, cred_t *);
void	dnlc_purge(void);
int	dnlc_purge1(void);
void	dnlc_purge_vp(vnode_t *);
int	dnlc_purge_vfsp(vfs_t *, int);
void	dnlc_remove(vnode_t *, char *);

#else

void	dnlc_init();
void	dnlc_enter();
vnode_t	*dnlc_lookup();
void	dnlc_purge();
int	dnlc_purge1();
void	dnlc_purge_vp();
int	dnlc_purge_vfsp();
void	dnlc_remove();

#endif

#endif	/* _FS_DNLC_H */
