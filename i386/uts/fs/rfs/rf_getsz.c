/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:fs/rfs/rf_getsz.c	1.2"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <io/stream.h>
#include <fs/rfs/rf_adv.h>
#include <fs/rfs/nserve.h>
#include <fs/stat.h>
#include <fs/statfs.h>
#include <fs/vnode.h>
#include <fs/rfs/rf_messg.h>
#include <fs/buf.h>
#include <proc/cred.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <util/list.h>
#include <fs/rfs/rf_cirmgr.h>
#include <fs/rfs/rf_debug.h>
#include <mem/seg.h>
#include <fs/rfs/rf_admin.h>
#include <fs/rfs/rf_comm.h>
#include <fs/rfs/rf_serve.h>
#include <fs/rfs/rf_vfs.h>
#include <fs/rfs/rfcl_subr.h>
#include <fs/rfs/du.h>
