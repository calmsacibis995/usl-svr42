/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/getsizes.c	1.2.2.2"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/rfs/nserve.h>
#include <io/conf.h>
#include <io/iobuf.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/tty.h>
#include <mem/as.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <mem/tuneable.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/priocntl.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/ipc/ipc.h>
#include <proc/ipc/msg.h>
#include <proc/ipc/sem.h>
#include <proc/ipc/shm.h>
#include <svc/callo.h>
#include <svc/hrtcntl.h>
#include <svc/hrtsys.h>
#include <svc/resource.h>
#include <util/map.h>
#include <util/param.h>
#include <util/types.h>
#include <util/var.h>
