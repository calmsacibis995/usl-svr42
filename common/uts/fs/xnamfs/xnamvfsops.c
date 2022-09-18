/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/xnamfs/xnamvfsops.c	1.2.2.2"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <fs/buf.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <fs/vfs.h>
#include <mem/swap.h>
#include <fs/vnode.h>
#include <fs/xnamfs/xnamnode.h>
#include <fs/fs_subr.h>

struct vfsops xnam_vfsops = {
	fs_nosys,	/* mount */
	fs_nosys,	/* unmount */
	fs_nosys,	/* root */
	fs_nosys,	/* statvfs */
	fs_nosys,	/* sync */
	fs_nosys,	/* vget */
	fs_nosys,	/* mountroot */
	fs_nosys,	/* not used */
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};
