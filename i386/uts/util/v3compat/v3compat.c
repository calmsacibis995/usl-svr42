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

#ident	"@(#)uts-x86:util/v3compat/v3compat.c	1.1"
#ident	"$Header: $"

/*
 * This module contains backward compatibility code for SVR3.2 binaries
 * that read a directory. When this is included the entry point
 * for the read() system call becomes v3read() defined below.
 * Any changes made to fs/vncalls.c read() should also be made here.
 *
 */

#include <acc/audit/audit.h>
#include <acc/dac/acl.h>
#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/fcntl.h>
#include <fs/fifofs/fifonode.h>
#include <fs/file.h>
#include <fs/filio.h>
#include <fs/mode.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/mkdev.h>
#include <io/poll.h>
#include <io/termios.h>
#include <io/ttold.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/locking.h>
#include <svc/resource.h>
#include <svc/sco.h>
#include <svc/syscall.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>

int	v3read();

STATIC	struct	sysent	s_v3read = { 3, ASYNC|IOSYS, v3read };


void
v3compatinit()
{
	sysent[SYS_read] = s_v3read;
}

/*
 * Read and write.
 */
struct rwa {
	int fdes;
	char *cbuf;
	unsigned count;
};

int
v3read(uap, rvp)
	register struct rwa *uap;
	rval_t *rvp;
{
	struct uio auio;
	register struct uio	*uio = &auio;
	register vnode_t	*vp;
	struct iovec aiov;
	int count;
	file_t *fp;
	int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	if (((uio->uio_fmode = fp->f_flag) & FREAD) == 0)
		return EBADF;

	sysinfo.sysread++;
	aiov.iov_base = (caddr_t)uap->cbuf;

	if ((count = uio->uio_resid = aiov.iov_len = uap->count) < 0)
		return EINVAL;
	uio->uio_iov = &aiov;
	uio->uio_iovcnt = 1;
	vp = fp->f_vnode;
	uio->uio_offset = fp->f_offset;
	uio->uio_segflg = UIO_USERSPACE;
	/* don't need to keep track of uio_limit as not increasing file
	 * size
	 */
	VOP_RWLOCK(vp);
	if (setjmp(&u.u_qsav))
		error = EINTR;
	else if ((vp->v_type == VDIR) && ((u.u_renv & U_RENVMASK) != U_ISELF))
		error = dircompat (vp, uio, fp->f_cred);
	else
		error = VOP_READ(vp, uio, 0, fp->f_cred);
	VOP_RWUNLOCK(vp);

	if (error == EINTR && uio->uio_resid != count)
		error = 0;
	rvp->r_val1 = count - uio->uio_resid;
	u.u_ioch += (unsigned)rvp->r_val1;
	if (vp->v_type == VFIFO)	/* Backward compatibility */
		fp->f_offset = rvp->r_val1;
	else
		fp->f_offset = uio->uio_offset;
	sysinfo.readch += (unsigned)rvp->r_val1;
	if (vp->v_vfsp != NULL) {
		vp->v_vfsp->vfs_bcount += rvp->r_val1 >> SCTRSHFT;
	}
	return error;
}

#include	<fs/dirent.h>
#define	direct	s5_direct
#include	<fs/s5fs/s5dir.h>
#undef	direct
#define	DIRBLKS	1024

static
dircnt (buf, len)
char	*buf;
int	len;
{
register int	cnt;

	cnt = 0;
	while (len > 0)
	{
		register struct	dirent	*dp;

		dp = (struct dirent *) buf;
		if (dp->d_ino != 0)
			cnt++;
		if (dp->d_reclen == 0)
			return (0);
		buf += dp->d_reclen;
		len -= dp->d_reclen;
	}
	return (cnt);
}

static
dir2sysv (dir_buf, dir_len, s5)
char	*dir_buf;
int	dir_len;
struct	s5_direct	*s5;
{
int	s5cnt;

	s5cnt = 0;
	while (dir_len > 0)
	{
		struct	dirent	*dp;

		dp = (struct dirent *) dir_buf;
		if (dp->d_ino != 0)
		{
			s5[s5cnt].d_ino = dp->d_ino;
			{
				register int	i;
				register char	*p;
				char	*q;

				p = s5[s5cnt].d_name;
				q = dp->d_name;
				for (i = 0; i < 14; i++)
				{
					if ((p[i] = q[i]) == '\0')
						break;
				}
				for ( ; i < 14; i++)
					p[i] = 0;
			}
			s5cnt++;
		}
		if (dp->d_reclen == 0)
			return (0);
		dir_buf += dp->d_reclen;
		dir_len -= dp->d_reclen;
	}
	return (s5cnt);
}

static
dircompat(vp, uio, cr)
	struct vnode *vp;
	struct uio *uio;
	struct cred *cr;
{
	struct	uio	dir_uio;
	struct	iovec	dir_iov[1];
	int	ufs_cnt;
	int	eof;

	static	caddr_t	dir_pool;
	/* buffer of dirent structures */
	caddr_t	dir_buf;
	/* length (in bytes) of buffer of dirent structures */
	int	dir_len;
	/* index of first dirent structure in dir_buf */
	int	dir_index;

	static	caddr_t	s5_pool;
	caddr_t	s5_buf;

	int error = 0;

	if (uio->uio_resid <= 0)
		return;

	/* allocate buffer for block of dirent entries */
	dir_buf = kmem_fast_alloc (&dir_pool, DIRBLKS, 1, KM_SLEEP);
	/*
	 * allocate buffer for block of s5 entries;
	 * each dirent takes at least 8 bytes (actually, I think min is 12),
	 * so expansion to s5 format can't use more than 2x space.
	 */
	s5_buf = kmem_fast_alloc (&s5_pool, 2 * DIRBLKS, 1, KM_SLEEP);

	/* fs-dependent offset is updated in loop by VOP_READDIR */
	dir_uio.uio_offset = 0;
	/* index of first entry is updated in loop */
	dir_index = 0;
	while (1)
	{
		int	cur_index;

		dir_iov[0].iov_base = dir_buf;
		dir_iov[0].iov_len = DIRBLKS;
		dir_uio.uio_iov = dir_iov;
		dir_uio.uio_iovcnt = 1;
		dir_uio.uio_resid = DIRBLKS;
		dir_uio.uio_segflg = UIO_SYSSPACE;
		dir_uio.uio_fmode = uio->uio_fmode;
		dir_uio.uio_limit = uio->uio_limit;
		if (error = VOP_READDIR(vp, &dir_uio, cr, &eof))
			goto	done;
		dir_len = DIRBLKS - dir_uio.uio_resid;
		if (dir_len <= 0)
			goto	done;
		cur_index = dircnt (dir_buf, dir_len);
		if (cur_index <= 0)
			continue;
		cur_index += dir_index;
		if ((cur_index << 4) >= uio->uio_offset)
			break;
		dir_index = cur_index;
	}

	while (1)
	{
		/* number of entries in the current buffer */
		int	cur_cnt;
		/* offset into buffer */
		int	cur_off;
		/* bytes to copy */
		int	cur_len;

		/* copy (portion of) this buffer to user */
		cur_cnt = dir2sysv(dir_buf, dir_len, s5_buf);
		if (cur_cnt > 0)
		{
			/* offset into buffer of 1st byte to copy out */
			cur_off = uio->uio_offset - (dir_index << 4);
			/* number of bytes to copy from this buffer */
			cur_len = (cur_cnt << 4) - cur_off;
			if (cur_len > uio->uio_resid)
				cur_len = uio->uio_resid;
			uiomove (&(s5_buf[cur_off]), cur_len, UIO_READ, uio);
			if (uio->uio_resid <= 0)
				break;

			dir_index += cur_cnt;
		}
		/* read next block */
		dir_iov[0].iov_base = dir_buf;
		dir_iov[0].iov_len = DIRBLKS;
		dir_uio.uio_iov = dir_iov;
		dir_uio.uio_iovcnt = 1;
		dir_uio.uio_resid = DIRBLKS;
		dir_uio.uio_segflg = UIO_SYSSPACE;
		dir_uio.uio_fmode = uio->uio_fmode;
		dir_uio.uio_limit = uio->uio_limit;
		if (error = VOP_READDIR(vp, &dir_uio, cr, &eof))
			goto	done;
		dir_len = DIRBLKS - dir_uio.uio_resid;
		if (dir_len <= 0)
			goto	done;
	}
done:
	kmem_fast_free(&dir_pool, dir_buf);
	kmem_fast_free(&s5_pool, s5_buf);
	return (error);
}
