/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/kdvm/kdvm.c	1.4"
#ident	"$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <mem/immu.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <proc/user.h>
#include <util/inline.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <io/ws/vt.h>
#include <io/ansi/at_ansi.h>
#include <io/uio.h>
#include <io/kd/kd.h>
#include <io/xque/xque.h>
#include <io/stream.h>
#include <io/termios.h>
#include <io/strtty.h>
#include <io/stropts.h>
#include <io/ws/ws.h>
#include <io/ws/chan.h>
#include <io/gvid/vid.h>
#include <io/gvid/vdc.h>
#include <proc/cred.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/seg_objs.h>
#include <proc/mman.h>
#include <io/ddi.h>
#include <io/event/event.h>

int kdvm_devflag = 0;



/* ARGSUSED */
int
kdvm_open(devp, flag, otyp, cr)
	dev_t *devp;
	int flag;
	int otyp;
	struct cred *cr;
{
	int	dev;
		
	if(ws_getvtdev(&dev) < 0)
		return(-1);

	return ws_open (dev, flag, otyp, cr);
}

/* ARGSUSED */
int
kdvm_close(dev, flag, otyp, cr)
	dev_t dev;
	int flag;
	int otyp;
	struct cred *cr;
{
	return 0;
}

/* ARGSUSED */
int
kdvm_ioctl(dev, cmd, arg, mode, cr, rvalp)
	dev_t dev;
	int cmd;
	int arg;
	int mode;
	struct cred *cr;
	int *rvalp;
{
	if(ws_getvtdev(&dev) < 0)
		return(-1);

	return ws_ioctl (dev, cmd, arg, mode, cr, rvalp);
}
