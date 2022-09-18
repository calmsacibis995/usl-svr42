/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/pipe.c	1.4.3.3"
#ident	"$Header: $"

#include <acc/mac/cca.h>
#include <fs/fifofs/fifonode.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/param.h>
#include <util/types.h>

/*
 * pipe(2) system call.
 * Create a pipe by connecting two streams together. Associate
 * each end of the pipe with a vnode, a file descriptor and 
 * one of the streams.
 */
/*ARGSUSED*/
int
pipe(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	struct vnode *vp1, *vp2;
	extern ushort fifogetid();
	struct file *fp1, *fp2;
	register int error = 0;
	int fd1, fd2;

	/*
	 * Make pipe ends.
	 */
	if (error = fifo_mkpipe(&vp1, &vp2, u.u_cred))
		return (error);

	/*
	 * Allocate and initialize two file table entries and two
	 * file pointers. Each file pointer is open for read and
	 * write.
	 */
	if (error = falloc(vp1, FWRITE|FREAD, &fp1, &fd1)) {
		fifo_rmpipe(vp1, vp2, u.u_cred);
		return (error);
	}

	if (error = falloc(vp2, FWRITE|FREAD, &fp2, &fd2)) {
		unfalloc(fp1);
		setf(fd1, NULLFP);
		fifo_rmpipe(vp1, vp2, u.u_cred);
		return (error);
	}

	/*
	 * Return the file descriptors to the user. They now
	 * point to two different vnodes which have different
	 * stream heads.
	 */
	rvp->r_val1 = fd1;
	rvp->r_val2 = fd2;
	return (0);
}
