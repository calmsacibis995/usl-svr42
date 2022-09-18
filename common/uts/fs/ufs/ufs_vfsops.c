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

#ident	"@(#)uts-comm:fs/ufs/ufs_vfsops.c	1.9.4.7"
#ident	"$Header: $"

#include <fs/sfs/sfs_inode.h>
#include <fs/vfs.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/mod/moddefs.h>

/*
 * ufs vfs operations are defined in sfs_vfsops.c.
 */


STATIC int ufs_load(void);

int ufs_fsflags = 0;  /* to initialize vswp->vsw_flag */

MOD_FS_WRAPPER(ufs, ufs_load, NULL, "Loadable UFS FS Type");

STATIC int
ufs_load(void)
{
      /* 
       * Initialize quotas.  These should only be utilized
       * if UFS is installed.
       */
#ifdef QUOTA
      sfs_qtinit();
#endif
      return 0;
}

/*
 * Initialize UFS file system type at system initialization time.
 */

int
ufsinit(vswp, fstype)
	struct vfssw	*vswp;
	int		fstype;
{
	vswp->vsw_vfsops = &ufs_vfsops;
	
	/* 
	 * Initialize quotas.  These should only be utilized
	 * if UFS is installed.
	 */
#ifdef QUOTA
	sfs_qtinit();
#endif
	return 0;
}
