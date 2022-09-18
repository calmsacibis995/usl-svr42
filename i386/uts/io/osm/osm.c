/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/osm/osm.c	1.3"
#ident	"$Header: $"

/*
 *	OSM - operating system messages, allows system printf's to
 *	be read via special file.
 *	minor 0: starts from beginning of buffer and waits for more.
 *	minor 1: starts from beginning of buffer but doesn't wait.
 *	minor 2: starts at current buffer position and waits.
 */

#include <util/param.h>
#include <util/types.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <mem/immu.h>
#include <proc/user.h>
#include <fs/buf.h>
#include <fs/file.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <io/conf.h>

extern	char	putbuf[];	/* system putchar circular buffer */
extern	int	putbufsz;		/* size of above */
extern	int	putbufwpos;	/* next position for system putchar */

int osmdevflag = D_OLD;

osmopen(dev)
{
	struct file *fp;
	int error;

	dev = minor(dev);
	if (error = getf(u.u_rval1, &fp))
		return(error);
	if (dev == 2)
		fp->f_offset = putbufwpos;
	else if (dev > 2)
		u.u_error = ENODEV;
	else if (putbufwpos > putbufsz)
		fp->f_offset = putbufwpos - putbufsz;
}

osmread(dev)
{
	register int	index;
	register int	offset;
	register int	count;

	dev = minor(dev);
	if (dev == 1 && u.u_offset >= putbufwpos)
		return;

	spl6();
	while (u.u_offset == putbufwpos)
		sleep(putbuf, PWAIT);
	spl0();

	ASSERT(u.u_offset < putbufwpos);

	while (u.u_count  &&  u.u_offset < putbufwpos  && u.u_error == 0) {
		offset = u.u_offset % putbufsz;
		index = putbufwpos % putbufsz;
		if (offset < index) {
			count = min(index - offset, u.u_count);
		} else {
			count = min(putbufsz - offset, u.u_count);
		}
		iomove(&putbuf[offset], count, B_READ);
	}
}

osmwrite()
{
	register int	cc;

	while ((cc = cpass()) >= 0)
		putbuf[putbufwpos++ % putbufsz] = cc;
	wakeup(putbuf);
}
