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

#ident	"@(#)uts-x86:fs/vncalls.c	1.40.4.17"
#ident	"$Header: $"

/*
 * System call routines for operations on files.  These manipulate
 * the global and per-process file table entries which refer to
 * vnodes, the system generic file abstraction.
 *
 * Many operations take a path name.  After preparing arguments, a
 * typical operation may proceed with:
 *
 *	error = lookupname(name, seg, followlink, &dvp, &vp);
 *
 * where "name" is the path name operated on, "seg" is UIO_USERSPACE
 * or UIO_SYSSPACE to indicate the address space in which the path
 * name resides, "followlink" specifies whether to follow symbolic
 * links, "dvp" is a pointer to a vnode for the directory containing
 * "name", and "vp" is a pointer to a vnode for "name".  (Both "dvp"
 * and "vp" are filled in by lookupname()).  "error" is zero for a
 * successful lookup, or a non-zero errno (from errno.h) if an
 * error occurred.  This paradigm, in which routines return error
 * numbers to their callers and other information is returned via
 * reference parameters, now appears in many places in the kernel.
 *
 * lookupname() fetches the path name string into an internal buffer
 * using pn_get() (pathname.c) and extracts each component of the path
 * by iterative application of the file system-specific VOP_LOOKUP
 * operation until the final vnode and/or its parent are found.
 * (There is provision for multiple-component lookup as well.)  If
 * either of the addresses for dvp or vp are NULL, lookupname() assumes
 * that the caller is not interested in that vnode.  Once a vnode has
 * been found, a vnode operation (e.g. VOP_OPEN, VOP_READ) may be
 * applied to it.
 *
 * With few exceptions (made only for reasons of backward compatibility)
 * operations on vnodes are atomic, so that in general vnodes are not
 * locked at this level, and vnode locking occurs at lower levels (either
 * locally, or, perhaps, on a remote machine.  (The exceptions make use
 * of the VOP_RWLOCK and VOP_RWUNLOCK operations, and include VOP_READ,
 * VOP_WRITE, and VOP_READDIR).  In addition permission checking is
 * generally done by the specific filesystem, via its VOP_ACCESS
 * operation.  The upper (vnode) layer performs checks involving file
 * types (e.g. VREG, VDIR), since the type is static over the life of
 * the vnode.
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
#include <proc/unistd.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/locking.h>
#include <svc/resource.h>
#include <svc/sco.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Open a file.
 */
struct opena {
	char	*fname;
	int	fmode;
	int	cmode;
};

#if defined(__STDC__)
STATIC int	copen(char *, int, int, rval_t *);
#else
STATIC int	copen();
#endif

int
open(uap, rvp)
	register struct opena *uap;
	rval_t *rvp;
{
	return copen(uap->fname, (int)(uap->fmode-FOPEN), uap->cmode, rvp);
}

/*
 * Create a file.
 */
struct creata {
	char	*fname;
	int	cmode;
};

int
creat(uap, rvp)
	register struct creata *uap;
	rval_t *rvp;
{
	return copen(uap->fname, FWRITE|FCREAT|FTRUNC, uap->cmode, rvp);
}

/*
 * Common code for open() and creat().  Check permissions, allocate
 * an open file structure, and call the device open routine (if any).
 */
STATIC int
copen(fname, filemode, createmode, rvp)
	char *fname;
	int filemode;
	int createmode;
	rval_t *rvp;
{
	vnode_t *vp;
	file_t *fp;
	register int error;
	int fd, dupfd;
	enum vtype type;

	if ((filemode & (FREAD|FWRITE)) == 0)
		return EINVAL;

	if ((filemode & (FNONBLOCK|FNDELAY)) == (FNONBLOCK|FNDELAY))
		filemode &= ~FNDELAY;

	if (error = falloc((vnode_t *)NULL, filemode & FMASK, &fp, &fd))
		return error;
	
	/*
	 * Last arg is a don't-care term if !(filemode & FCREAT).
	 */
	error = vn_open(fname, UIO_USERSPACE, filemode,
	  (int)((createmode & MODEMASK) & ~u.u_cmask), &vp, CRCREAT);
	if (error) {
		setf(fd, NULLFP);
		unfalloc(fp);
	} else if (vp->v_flag & VDUP) {
		/*
		 * Special handling for /dev/fd.  Give up the file pointer
		 * and dup the indicated file descriptor (in v_rdev).  This
		 * is ugly, but I've seen worse.
		 */
		setf(fd, NULLFP);
		unfalloc(fp);
		dupfd = getminor(vp->v_rdev);
		type = vp->v_type;
		vp->v_flag &= ~VDUP;
		VN_RELE(vp);
		if (type != VCHR)
			return EINVAL;
		if (error = getf(dupfd, &fp))
			return error;
		setf(fd, fp);
		fp->f_count++;
		rvp->r_val1 = fd;
	} else {
		/* SCO Enhanced Application Compatibility Support */

		extern	int dev_autocad_major; 
		int	major;

		major = getmajor(vp->v_rdev);

		if(major == dev_autocad_major && vp->v_type == VCHR) {
			cmn_err(CE_WARN, "Trying to open a disabled driver");
			VN_RELE(vp);
			setf(fd, NULLFP);
			unfalloc(fp);
			return( EINVAL);
		}

		/* End  Enhanced Application Compatibility Support */

		MAC_ASSERT (vp, MAC_DOMINATES);	/* open assumed correct */
		fp->f_vnode = vp;
		rvp->r_val1 = fd;
	}

	return error;
}

/*
 * Close a file.
 */
struct closea {
	int	fdes;
};

/* ARGSUSED */
int
close(uap, rvp)
	register struct closea *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	error = closef(fp);
	setf(uap->fdes, NULLFP);
	return error;
}

/*
 * Read and write.
 */
struct rwa {
	int fdes;
	char *cbuf;
	unsigned count;
};

/*
 * Readv and writev.
 */
struct rwva {
	int fdes;
	struct iovec *iovp;
	int iovcnt;
};

#if defined(__STDC__)

STATIC int	rwv(struct rwva *, rval_t *, int);
STATIC int	rdwr(file_t *, uio_t *, rval_t *, int);
#else

STATIC int	rwv();
STATIC int	rdwr();

#endif


int
read(uap, rvp)
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


int
write(uap, rvp)
	register struct rwa *uap;
	rval_t *rvp;
{
	struct uio auio;
	register struct uio *uio = &auio;
	register vnode_t *vp;
	struct iovec aiov;
	int ioflag;
	int count;
	file_t *fp;
	int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	if (((uio->uio_fmode = fp->f_flag) & FWRITE) == 0)
		return EBADF;
	sysinfo.syswrite++;
	aiov.iov_base = (caddr_t)uap->cbuf;
	if ((count = uio->uio_resid = aiov.iov_len = uap->count) < 0)
		return EINVAL;
	uio->uio_iov = &aiov;
	uio->uio_iovcnt = 1;
	vp = fp->f_vnode;
	uio->uio_offset = fp->f_offset;
	uio->uio_segflg = UIO_USERSPACE;
	uio->uio_limit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
	ioflag = 0;
	if (fp->f_flag & FAPPEND)
		ioflag |= IO_APPEND;
	if (fp->f_flag & FSYNC)
		ioflag |= IO_SYNC;

	VOP_RWLOCK(vp);
	if (setjmp(&u.u_qsav))
		error = EINTR;
	else
		error = VOP_WRITE(vp, uio, ioflag, fp->f_cred);
	VOP_RWUNLOCK(vp);

	if (error == EINTR && uio->uio_resid != count)
		error = 0;

	rvp->r_val1 = count - uio->uio_resid;
	u.u_ioch += (unsigned)rvp->r_val1;
	if (vp->v_type == VFIFO)	/* Backward compatibility */
		fp->f_offset = rvp->r_val1;
	else if (((fp->f_flag & FAPPEND) == 0) || vp->v_type != VREG
	  || uap->count != 0)		/* POSIX */
		fp->f_offset = uio->uio_offset;
	sysinfo.writech += (unsigned)rvp->r_val1;
	if (vp->v_vfsp != NULL)
		vp->v_vfsp->vfs_bcount += rvp->r_val1 >> SCTRSHFT;
	return error;
}

int
readv(uap, rvp)
	struct rwva *uap;
	rval_t *rvp;
{
	return rwv(uap, rvp, FREAD);
}

int
writev(uap, rvp)
	struct rwva *uap;
	rval_t *rvp;
{
	return rwv(uap, rvp, FWRITE);
}

/*
 * The interface (as defined in the read(2) and write(2) man pages)
 * allows for a maximum iovcnt of 16.
 */
#define	IOVSIZ_MAX	16

STATIC int
rwv(uap, rvp, mode)
	struct rwva *uap;
	rval_t *rvp;
	register int mode;
{
	file_t *fp;
	int error;
	struct uio auio;
	struct iovec aiov[IOVSIZ_MAX];

	if (error = getf(uap->fdes, &fp))
		return error;
	if ((fp->f_flag & mode) == 0)
		return EBADF;
	if (mode == FREAD)
		sysinfo.sysread++;
	else
		sysinfo.syswrite++;
	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0]))
		return EINVAL;
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	if (copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	  (unsigned)(uap->iovcnt * sizeof(struct iovec))))
		return EFAULT;
	return rdwr(fp, &auio, rvp, mode);
}

/*
 * Common code for read and write calls: check permissions, set base,
 * count, and offset, and switch out to VOP_READ or VOP_WRITE code.
 */
STATIC int
rdwr(fp, uio, rvp, mode)
	file_t *fp;
	register struct uio *uio;
	rval_t *rvp;
	register int mode;
{
	register vnode_t	*vp;
	enum vtype		type;
	iovec_t			*iovp;
	int			ioflag;
	int			count;
	int			i;
	register int		error;

	uio->uio_resid = 0;
	iovp = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iovp->iov_len < 0)
			return EINVAL;
		uio->uio_resid += iovp->iov_len;
		if (uio->uio_resid < 0)
			return EINVAL;
		iovp++;
	}
	vp = fp->f_vnode;
	type = vp->v_type;
	count = uio->uio_resid;
	uio->uio_offset = fp->f_offset;
	uio->uio_segflg = UIO_USERSPACE;
	uio->uio_fmode = fp->f_flag;
	uio->uio_limit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
	ioflag = 0;
	if (fp->f_flag & FAPPEND)
		ioflag |= IO_APPEND;
	if (fp->f_flag & FSYNC)
		ioflag |= IO_SYNC;

	VOP_RWLOCK(vp);
	if (setjmp(&u.u_qsav))
		error = EINTR;
	else if (mode == FREAD)
		error = VOP_READ(vp, uio, ioflag, fp->f_cred);
	else {
		MAC_ASSERT (vp, MAC_SAME); /* since file is open
			for writing, it's legal to assume that
			labels are same as process */
		error = VOP_WRITE(vp, uio, ioflag, fp->f_cred);
	}
	VOP_RWUNLOCK(vp);

	if (error == EINTR && uio->uio_resid != count)
		error = 0;

	rvp->r_val1 = count - uio->uio_resid;
	u.u_ioch += (unsigned)rvp->r_val1;
	if (type == VFIFO)	/* Backward compatibility */
		fp->f_offset = rvp->r_val1;
	else
		fp->f_offset = uio->uio_offset;
	if (mode == FREAD) {
		sysinfo.readch += (unsigned)rvp->r_val1;
		if (vp->v_vfsp != NULL)
			vp->v_vfsp->vfs_bcount += 
			  rvp->r_val1 >> SCTRSHFT;

	} else {
		sysinfo.writech += (unsigned)rvp->r_val1;
		if (vp->v_vfsp != NULL)
			vp->v_vfsp->vfs_bcount += 
			  rvp->r_val1 >> SCTRSHFT;
	}
	return error;
}

/*
 * Change current working directory (".").
 */
struct chdira {
	char *fname;
};

#if defined(__STDC__)
STATIC int	chdirec(vnode_t *, vnode_t **);
#else
STATIC int	chdirec();
#endif

/* ARGSUSED */
int
chdir(uap, rvp)
	struct chdira *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int  error;

	if (error = lookupname(uap->fname, UIO_USERSPACE, 
	    FOLLOW, NULLVPP, &vp))
		return error;

	error = chdirec(vp, &u.u_cdir);

	if (!error) {
		if (audit_on)
			adt_pathupd(uap->fname);
	}

	return(error);
}

/*
 * File-descriptor based version of 'chdir'.
 */
struct fchdira {
	int  fd; 
};

/* ARGSUSED */
int
fchdir(uap, rvp)
	struct fchdira *uap;
	rval_t *rvp;
{
	file_t *fp;
	vnode_t *vp;
	register int error;
	
	if (error = getf(uap->fd, &fp))
		return error;
	vp = fp->f_vnode;
	VN_HOLD(vp);
	return chdirec(vp, &u.u_cdir);
}

/*
 * Change notion of root ("/") directory.
 */
/* ARGSUSED */
int
chroot(uap, rvp)
	struct chdira *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int 	error;

	if (pm_denied(u.u_cred, P_FILESYS))
		return EPERM;
	if (error = lookupname(uap->fname, UIO_USERSPACE, 
	    FOLLOW, NULLVPP, &vp))
		return error;

	error = chdirec(vp, &u.u_rdir);

	if (!error) {
		if (audit_on)
			adt_pathupd(uap->fname);
	}

	return(error);

}

/*
 * Chdirec() takes as an argument a vnode pointer and a vpp as an
 * out parameter.  If the vnode passed in corresponds to a 
 * directory for which the user has execute permission, then
 * vpp, if it is non-NULL, is updated to point to the vnode
 * passed in.  
 */
STATIC int
chdirec(vp, vpp)
	vnode_t *vp;
	vnode_t **vpp;
{
	register int error;

	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}
	PREEMPT();
	/*
         * Must have MAC search access to the directory.
	 */
	if ((error = MAC_VACCESS(vp, VEXEC, u.u_cred)) != 0)
		goto bad;

	MAC_ASSERT (vp, MAC_DOMINATES);	/* search access implies process
					   must dominate */

	if (error = VOP_ACCESS(vp, VEXEC, 0, u.u_cred))
		goto bad;
	if (*vpp)
		VN_RELE(*vpp);
	*vpp = vp;
	return 0;

bad:
	VN_RELE(vp);
	return error;
}

/*
 * Create a special file, a regular file, or a FIFO.
 */

/* SVR3 mknod arg */
struct mknoda {
	char	*fname;		/* pathname passed by user */
	mode_t	fmode;		/* mode of pathname */
	dev_t	dev;		/* device number - b/c specials only */
};

#if defined(__STDC__)
STATIC int	cmknod(int, char *, mode_t, dev_t, rval_t *);
#else
STATIC int	cmknod();
#endif

/* SVR3 mknod */
int
mknod(uap, rvp)
	register struct mknoda *uap;
	rval_t *rvp;
{
	return cmknod(_R3_MKNOD_VER, uap->fname, uap->fmode, uap->dev, rvp);
}

struct xmknoda {
	int	version;	/* version of this syscall */
	char	*fname;		/* pathname passed by user */
	mode_t	fmode;		/* mode of pathname */
	dev_t	dev;		/* device number - b/c specials only */
};

/*
 * Expanded mknod.
 */
xmknod(uap, rvp)
	register struct xmknoda *uap;
	rval_t *rvp;
{
	return cmknod(uap->version, uap->fname, uap->fmode, uap->dev, rvp);
}

/* ARGSUSED */
STATIC int
cmknod(version, fname, fmode, dev, rvp)
	int version;
	char *fname;
	mode_t fmode;
	dev_t dev;
	rval_t *rvp;
{
	vnode_t *vp;
	struct vattr vattr;
	int error;
	extern u_int maxminor;
	major_t major = getemajor(dev);
	minor_t highminor = maxminor;	/* `maxminor' is a tunable */

	/*
	 * Zero type is equivalent to a regular file.
	 */
	if ((fmode & S_IFMT) == 0)
		fmode |= S_IFREG;

	/*
	 * Must be a P_FILESYS privileged process unless making a FIFO node.
	 */
	if (((fmode & S_IFMT) != S_IFIFO) 
	/* XENIX Support */
	  && ((fmode & S_IFMT) != S_IFNAM)
	/* End XENIX Support */
	  && pm_denied(u.u_cred, P_FILESYS))
		return EPERM;
	/*
	 * Set up desired attributes and vn_create the file.
	 */
	vattr.va_type = IFTOVT(fmode);
	vattr.va_mode = (fmode & MODEMASK) & ~u.u_cmask;
	vattr.va_mask = AT_TYPE|AT_MODE;
	if (vattr.va_type == VCHR || vattr.va_type == VBLK
	  || vattr.va_type == VXNAM) {
		if (version == _MKNOD_VER && vattr.va_type != VXNAM) {
			if (dev == (dev_t)NODEV
			  || getemajor(dev) == (dev_t)NODEV)
				return EINVAL;
			else
				vattr.va_rdev = dev;
		} else {
			/* dev is in old format */
			if ((emajor(dev)) == (dev_t)NODEV
			  || dev == (dev_t)NODEV)
				return EINVAL;
			else
				vattr.va_rdev = expdev(dev);
		}
		/*
		 * Check minor number range
		 */
		if (vattr.va_type == VCHR) {
			if (major < cdevcnt && *cdevsw[major].d_flag & D_OLD)
			   	highminor = OMAXMIN; /* old-style driver */
		} else if (vattr.va_type == VBLK) {
			if (major < bdevcnt && *bdevsw[major].d_flag & D_OLD)
			   	highminor = OMAXMIN; /* old-style driver */
		} /* VXNAM devices are limited to `maxminor' */

		/*
		 * If the driver is not installed, or is new-style, `highminor'
		 * will be equal to the tunable, `maxminor', otherwise it
		 * will be equal to `OMAXMIN'.
		 */
		if (geteminor(vattr.va_rdev) > highminor)
			return EINVAL;

		vattr.va_mask |= AT_RDEV;
	}
	if ((error = vn_create(fname, UIO_USERSPACE,
	  &vattr, EXCL, 0, &vp, CRMKNOD)) == 0)
		VN_RELE(vp);
	return error;
}

/*
 * Make a directory.
 */
struct mkdira {
	char *dname;
	int dmode;
};

/* ARGSUSED */
int
mkdir(uap, rvp)
	register struct mkdira *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	struct vattr vattr;
	int error;

	vattr.va_type = VDIR;
	vattr.va_mode = (uap->dmode & MODEMASK) & ~u.u_cmask;
	vattr.va_mask = AT_TYPE|AT_MODE;
	if ((error = vn_create(uap->dname, UIO_USERSPACE, &vattr,
	  EXCL, 0, &vp, CRMKDIR)) == 0)
		VN_RELE(vp);
	return error;
}

/*
 * Make a hard link.
 */
struct linka {
	char	*from;
	char	*to;
};

/* ARGSUSED */
int
link(uap, rvp)
	register struct linka *uap;
	rval_t *rvp;
{
	return vn_link(uap->from, uap->to, UIO_USERSPACE);
}

/*
 * Rename or move an existing file.
 *
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function
 */
struct renamea {
	char	*from;
	char	*to;
};

/* ARGSUSED */
int
rename(uap, rvp)
	struct renamea *uap;
	rval_t *rvp;
{
	return vn_rename(uap->from, uap->to, UIO_USERSPACE);
}

/*
 * Create a symbolic link.  Similar to link or rename except target
 * name is passed as string argument, not converted to vnode reference.
 */
struct symlinka {
	char	*target;
	char	*linkname;
};

/* ARGSUSED */
int
symlink(uap, rvp)
	register struct symlinka *uap;
	rval_t *rvp;
{
	vnode_t *dvp;
	struct vattr vattr;
	struct pathname tpn;
	struct pathname lpn;
	int error;

	if (error = pn_get(uap->linkname, UIO_USERSPACE, &lpn))
		return error;
	if (error = lookuppn(&lpn, NO_FOLLOW, &dvp, NULLVPP)) {
		pn_free(&lpn);
		return error;
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	if (error = pn_get(uap->target, UIO_USERSPACE, &tpn))
		goto out;
	/*
	 * Must have MAC write access on the parent directory of
	 * "linkname" (so that we can create  the symbolic link file).
	 */
	if ((error = MAC_VACCESS(dvp, VWRITE, u.u_cred)) != 0)
		goto out1;

	MAC_ASSERT (dvp, MAC_SAME);	/* MAC write implies same labels */

	/*
	 * Level of symbolic link is that of the parent directory.
	 * Make sure that this level is within the fs range.
	 * The MAC equality checks are added for performance,
	 * i.e., if the level is that of the floor or ceiling,
	 * there is no need to do domination checks.
	 */
	if (MAC_ACCESS(MACEQUAL, dvp->v_vfsp->vfs_macfloor, dvp->v_lid)
	&&  MAC_ACCESS(MACEQUAL, dvp->v_vfsp->vfs_macceiling, dvp->v_lid)
	&&  (MAC_ACCESS(MACDOM, dvp->v_vfsp->vfs_macceiling, dvp->v_lid)
	  || MAC_ACCESS(MACDOM, dvp->v_lid, dvp->v_vfsp->vfs_macfloor))
	&&  pm_denied(u.u_cred, P_FSYSRANGE)) {
		error = EACCES;
		goto out1;
	}

	vattr.va_type = VLNK;
	vattr.va_mode = 0777;
	vattr.va_mask = AT_TYPE|AT_MODE;
	error = VOP_SYMLINK(dvp, lpn.pn_path, &vattr, tpn.pn_path, u.u_cred);
out1:
	pn_free(&tpn);
out:
	pn_free(&lpn);
	VN_RELE(dvp);
	if (audit_on)
		adt_symlink(uap->target, uap->linkname);/* audit recording function */
	return error;
}

/*
 * Unlink (i.e. delete) a file.
 */
struct unlinka {
	char	*fname;
};

/* ARGSUSED */
int
unlink(uap, rvp)
	struct unlinka *uap;
	rval_t *rvp;
{
	return vn_remove(uap->fname, UIO_USERSPACE, RMFILE);
}

/*
 * Remove a directory.
 */
struct rmdira {
	char *dname;
};

/* ARGSUSED */
int
rmdir(uap, rvp)
	struct rmdira *uap;
	rval_t *rvp;
{
	return vn_remove(uap->dname, UIO_USERSPACE, RMDIRECTORY);
}

/*
 * Get directory entries in a file system-independent format.
 */
struct getdentsa {
	int fd;
	char *buf;
	int count;
};

int
getdents(uap, rvp)
	struct getdentsa *uap;
	rval_t *rvp;
{
	register vnode_t *vp;
	file_t *fp;
	struct uio auio;
	struct iovec aiov;
	register int error;
	int sink;

	if (error = getf(uap->fd, &fp))
		return error;
	vp = fp->f_vnode;
	if (vp->v_type != VDIR)
		return ENOTDIR;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	VOP_RWLOCK(vp);
	error = VOP_READDIR(vp, &auio, fp->f_cred, &sink);
	VOP_RWUNLOCK(vp);
	if (error)
		return error;
	rvp->r_val1 = uap->count - auio.uio_resid;
	fp->f_offset = auio.uio_offset;
	return 0;
}

/*
 * Seek on file.
 */
struct lseeka {
	int	fdes;
	off_t	off;
	int	sbase;
};

int
lseek(uap, rvp)
	register struct lseeka *uap;
	rval_t *rvp;
{
	file_t *fp;
	register vnode_t *vp;
	struct vattr vattr;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	vp = fp->f_vnode;
	if (uap->sbase == 1)
		uap->off += fp->f_offset;
	else if (uap->sbase == 2) {
		vattr.va_mask = AT_SIZE;
		if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred))
			return error;
		uap->off += vattr.va_size;
	} else if (uap->sbase != 0) {
		/* XENIX Support */
		/* For XENIX compatibility, don't send SIGSYS */
		if(!VIRTUAL_XOUT)	
		/* End XENIX Support */
			psignal(u.u_procp, SIGSYS);
		return EINVAL;
	}
	if ((error = VOP_SEEK(vp, fp->f_offset, &uap->off)) == 0)
		rvp->r_off = fp->f_offset = uap->off;
	return error;
}

/*
 * Determine accessibility of file.
 */
struct accessa {
	char	*fname;
	int	fmode;
};

/* ARGSUSED */
int
access(uap, rvp)
	register struct accessa *uap;
	rval_t *rvp;
{
	struct vattr vattr;
	vnode_t *vp;
	cred_t *tmpcr;
	cred_t *savecr;
	register int error, mode, eok, exok;
	extern	void	pm_recalc();		/* for privilege mechanism */

	if (uap->fmode & ~(EFF_ONLY_OK|EX_OK|R_OK|W_OK|X_OK))
		return EINVAL;

	mode = ((uap->fmode & (R_OK|W_OK|X_OK)) << 6);
	eok = (uap->fmode & EFF_ONLY_OK);
	exok = (uap->fmode & EX_OK);

	if (!eok) {
		savecr = u.u_cred;
		u.u_cred = crdup(savecr);
		u.u_cred->cr_uid = savecr->cr_ruid;
		u.u_cred->cr_gid = savecr->cr_rgid;
		u.u_cred->cr_ruid = savecr->cr_uid;
		u.u_cred->cr_rgid = savecr->cr_gid;
		/*
		 * uid-based privilege mechanism only: recalculate
		 * privileges based on "new" effective user-ID.
		 * Has no effect for a file-based privilege mechanism.
		 */
		pm_recalc(u.u_cred);
	}

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp)) {
		if (!eok) {
			tmpcr = u.u_cred;
			u.u_cred = savecr;
			crfree(tmpcr);
		}
		return error;
	}

	if (mode || exok) {
		/*
		 * Must have the requested MAC access to the vnode,
		 * before checking discretionary access, or getting
		 * the file attributes. Note that we need MAC read
		 * access in addition to the requested discretionary
		 * access if we are doing new "exec" check, since
		 * we will be calling vop_getattr.
		 * If type is VFIFO then always check for WR
		 */

		if (vp->v_type == VFIFO)
			error = MAC_VACCESS(vp, VWRITE, u.u_cred);
		else
			error = MAC_VACCESS(vp, (exok ? mode | R_OK : mode), u.u_cred);
	}
	if (!error && mode) {
		/* MAC READ,EXEC, or WRITE implies at least dominates.*/
		MAC_ASSERT (vp, MAC_DOMINATES);
		error = VOP_ACCESS(vp, mode, 0, u.u_cred);
	}

	if (!error && exok) {
		/* MAC READ,EXEC, or WRITE implies at least dominates.*/
		MAC_ASSERT (vp, MAC_DOMINATES);
		vattr.va_mask = AT_MODE;
		error = VOP_GETATTR(vp, &vattr, 0, u.u_cred);
		if ((!error) && ((vp->v_type != VREG) ||
			((vattr.va_mode & (VEXEC|(VEXEC>>3)|(VEXEC>>6))) 
			== 0))) 
			error = EACCES;
	}
	
	if (!eok) {
		tmpcr = u.u_cred;
		u.u_cred = savecr;
		crfree(tmpcr);
	}

	VN_RELE(vp);
	return error;
}

/*
 * Get file attribute information through a file name or a file descriptor.
 */
struct stata {
	char	*fname;
	struct stat *sb;
};

#if defined(__STDC__)
STATIC int	cstat(vnode_t *, struct stat *, struct cred *);
#else
STATIC int	cstat();
#endif

/* ARGSUSED */
int
stat(uap, rvp)
	register struct stata *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp))
		return error;
	error = cstat(vp, uap->sb, u.u_cred);
	VN_RELE(vp);
	return error;
}

struct xstatarg {
	int version;
	char *fname;
	struct xstat *sb;
};

#if defined(__STDC__)
STATIC int	xcstat(vnode_t *, struct xstat *, struct cred *);
#else
STATIC int	xcstat();
#endif

/* ARGSUSED */
int
xstat(uap, rvp)
	register struct xstatarg *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp))
		return error;

	/*
	 * Check version.
	 */
	switch (uap->version) {

	case _STAT_VER:
		/* SVR4 stat */
		error = xcstat(vp, uap->sb, u.u_cred);
		break;

	case _R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(vp, (struct stat *)uap->sb, u.u_cred);
		break;

	default:
		error = EINVAL;
	}

	VN_RELE(vp);
	return error;
}

struct lstata {
	char	*fname;
	struct stat *sb;
};

/* ARGSUSED */
int
lstat(uap, rvp)
	register struct lstata *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  NO_FOLLOW, NULLVPP, &vp))
		return error;
	error = cstat(vp, uap->sb, u.u_cred);
	VN_RELE(vp);
	return error;
}

/* ARGSUSED */
int
lxstat(uap, rvp)
	register struct xstatarg *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  NO_FOLLOW, NULLVPP, &vp))
		return error;

	/*
	 * Check version.
	 */
	switch (uap->version) {

	case _STAT_VER:
		/* SVR4 stat */
		error = xcstat(vp, uap->sb, u.u_cred);
		break;

	case _R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(vp, (struct stat *) uap->sb, u.u_cred);
		break;

	default:
		error = EINVAL;
	}

	VN_RELE(vp);
	return error;
}

struct fstata {
	int	fdes;
	struct stat *sb;
};

/* ARGSUSED */
int
fstat(uap, rvp)
	register struct fstata *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	return cstat(fp->f_vnode, uap->sb, u.u_cred);
}

struct fxstatarg {
	int	version;
	int	fdes;
	struct xstat *sb;
};

/* ARGSUSED */
int
fxstat(uap, rvp)
	register struct fxstatarg *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

	/*
	 * Check version number.
	 */
	switch (uap->version) {
	case _STAT_VER:
		break;
	default:
		return EINVAL;
	}

	if (error = getf(uap->fdes, &fp))
		return error;

	switch (uap->version) {
	case _STAT_VER:
		/* SVR4 stat */
		error = xcstat(fp->f_vnode, uap->sb, u.u_cred);
		break;

	case _R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(fp->f_vnode, (struct stat *) uap->sb, u.u_cred);
		break;

	default:
		error = EINVAL;
	}

	return error;
}

/*
 * Common code for stat(), lstat(), and fstat().
 */
STATIC int
cstat(vp, ubp, cr)
	register vnode_t *vp;
	struct stat *ubp;
	struct cred *cr;
{
	struct stat sb;
	struct vattr vattr;
	register int error;

        /*
         * Must have MAC read access to the vnode.
	 */
	if ((error = MAC_VACCESS(vp, VREAD, cr)) != 0)
		return error;
	MAC_ASSERT (vp, MAC_DOMINATES);	/* MAC read access implies dominates */

	vattr.va_mask = AT_STAT;
	if (error = VOP_GETATTR(vp, &vattr, 0, cr))
		return error;
	sb.st_mode = (o_mode_t) (VTTOIF(vattr.va_type) | vattr.va_mode);
	/*
	 * Check for large values.
	 */
	if (vattr.va_uid > USHRT_MAX || vattr.va_gid > USHRT_MAX
	  || vattr.va_nodeid > USHRT_MAX || vattr.va_nlink > SHRT_MAX )
		return EOVERFLOW;
	sb.st_uid = (o_uid_t) vattr.va_uid;
	sb.st_gid = (o_gid_t) vattr.va_gid;
	/*
	 * Need to convert expanded dev to old dev format.
	 */
	if (vattr.va_fsid & 0x8000)
		sb.st_dev = (o_dev_t) vattr.va_fsid;
	else
		sb.st_dev = (o_dev_t) cmpdev(vattr.va_fsid);
	sb.st_ino = (o_ino_t) vattr.va_nodeid;
	sb.st_nlink = (o_nlink_t) vattr.va_nlink;
	sb.st_size = vattr.va_size;
	sb.st_atime = vattr.va_atime.tv_sec;
	sb.st_mtime = vattr.va_mtime.tv_sec;
	sb.st_ctime = vattr.va_ctime.tv_sec;
	sb.st_rdev = (o_dev_t)cmpdev(vattr.va_rdev);

	PREEMPT();

	if (copyout((caddr_t)&sb, (caddr_t)ubp, sizeof(sb)))
		error = EFAULT;
	return error;
}

/*
 * Common code for xstat(), lxstat(), and fxstat().
 */
STATIC int
xcstat(vp, ubp, cr)
	register vnode_t *vp;
	struct xstat *ubp;
	struct cred *cr;
{
	struct xstat sb;
	struct vattr vattr;
	register int error;
	register struct vfssw *vswp;
	struct vfs *vfsp;

        /*
         * Must have MAC read access to the vnode.
	 */
	if ((error = MAC_VACCESS(vp, VREAD, cr)) != 0)
		return error;

	MAC_ASSERT (vp, MAC_DOMINATES);	/* MAC read access implies dominates */

	vattr.va_mask = AT_STAT|AT_NBLOCKS|AT_BLKSIZE;
	if (error = VOP_GETATTR(vp, &vattr, 0, cr))
		return error;

	struct_zero((caddr_t)&sb, sizeof(sb));

	sb.st_mode = VTTOIF(vattr.va_type) | vattr.va_mode;
	sb.st_uid = vattr.va_uid;
	sb.st_gid = vattr.va_gid;
	sb.st_dev = vattr.va_fsid;
	sb.st_ino = vattr.va_nodeid;
	sb.st_nlink = vattr.va_nlink;
	sb.st_size = vattr.va_size;
	sb.st_atime = vattr.va_atime;
	sb.st_mtime = vattr.va_mtime;
	sb.st_ctime = vattr.va_ctime;
	sb.st_rdev = vattr.va_rdev;
	sb.st_blksize = vattr.va_blksize;
	sb.st_blocks = vattr.va_nblocks;
	if (vp->v_vfsp) {
		vswp = &vfssw[vp->v_vfsp->vfs_fstype];
		if (vswp->vsw_name && *vswp->vsw_name)
			strcpy(sb.st_fstype, vswp->vsw_name);

	}
	if (error = VOP_GETACLCNT(vp, cr, &sb.st_aclcnt)) {
		if (error == ENOSYS) {
			sb.st_aclcnt = NACLBASE;
			error = 0;
		} else
			return error;
	}
	sb.st_level = vp->v_lid;
	if (mac_installed && (vp->v_macflag & VMAC_ISMLD))
		sb.st_flags |= S_ISMLD;

	/* For block/char special files check if dev is mounted. 
	 * This supports a semantic available in ustat(2) where
	 * user level code can determine if the device is mounted.
	 * This implementation assumes both the block/char major
	 * number map to the same driver entry point. 
	 */

	if (vattr.va_type == VBLK || vattr.va_type == VCHR)
		for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next)
			if (vattr.va_rdev == vfsp->vfs_dev) {
				sb.st_flags |= _S_ISMOUNTED;
				break;
			}

	if (copyout((caddr_t)&sb, (caddr_t)ubp, sizeof(sb)))
		error = EFAULT;
	return error;
}

#if defined(__STDC__)
STATIC int	cpathconf(vnode_t *, int, rval_t *, struct cred *);
#else
STATIC int	cpathconf();
#endif

/* fpathconf/pathconf interfaces 
 *
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function
 */

struct fpathconfa {
	int	fdes;
	int	name;
};

/* ARGSUSED */
int
fpathconf(uap, rvp)
	register struct fpathconfa *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	return cpathconf(fp->f_vnode, uap->name, rvp, u.u_cred);
}

struct pathconfa {
	char	*fname;
	int	name;
};

/* ARGSUSED */
int
pathconf(uap, rvp)
	register struct pathconfa *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp))
		return error;
	error = cpathconf(vp, uap->name, rvp, u.u_cred);
	VN_RELE(vp);
	return error;
}
/*
 * Common code for pathconf(), fpathconf() system calls
 */
STATIC int
cpathconf(vp, cmd, rvp, cr)
	register vnode_t *vp;
	int cmd;
	rval_t *rvp;
	struct cred *cr;
{
	register int error;
	u_long val;

	if ((error = VOP_PATHCONF(vp, cmd, &val, cr)) == 0)
		rvp->r_val1 = val;

	return error;
}

/*
 * Read the contents of a symbolic link.
 */
struct readlinka {
	char	*name;
	char	*buf;
	int	count;
};

int
readlink(uap, rvp)
	register struct readlinka *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	struct iovec aiov;
	struct uio auio;
	int error;

	if ((error = lookupname(uap->name, UIO_USERSPACE,
	  NO_FOLLOW, NULLVPP, &vp)) != 0)
		return error;

	if (vp->v_type != VLNK) {
		error = EINVAL;
		goto out;
	}

        /*
         * Must have MAC read access to the symbolic link.
	 */
	if ((error = MAC_VACCESS(vp, VREAD, u.u_cred)) != 0)
		goto out;

	MAC_ASSERT (vp, MAC_DOMINATES);	/* MAC read access implies dominates */

	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	error = VOP_READLINK(vp, &auio, u.u_cred);
out:
	VN_RELE(vp);
	rvp->r_val1 = uap->count - auio.uio_resid;
	return error;
}

#if defined(__STDC__)
STATIC int	namesetattr(char *, enum symfollow, vattr_t *, int);
static int	fdsetattr(int, vattr_t *);
#else
STATIC int	namesetattr();
static int	fdsetattr();
#endif

/*
 * Change mode of file given path name.
 */
struct chmoda {
	char	*fname;
	int	fmode;
};

/* ARGSUSED */
int
chmod(uap, rvp)
	register struct chmoda *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	vattr.va_mode = uap->fmode & MODEMASK;
	vattr.va_mask = AT_MODE;
	return namesetattr(uap->fname, FOLLOW, &vattr, 0);
}

/*
 * Change mode of file given file descriptor.
 */
struct fchmoda {
	int	fd;
	int	fmode;
};

/* ARGSUSED */
int
fchmod(uap, rvp)
	register struct fchmoda *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	vattr.va_mode = uap->fmode & MODEMASK;
	vattr.va_mask = AT_MODE;
	return fdsetattr(uap->fd, &vattr);
}

/*
 * Change ownership of file given file name.
 */
struct chowna {
	char	*fname;
	int	uid;
	int	gid;
};

/* ARGSUSED */
int
chown(uap, rvp)
	register struct chowna *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	if (uap->uid < (uid_t)-1 || uap->uid > MAXUID
	  || uap->gid < (gid_t)-1 || uap->gid > MAXUID)
		return EINVAL;
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != (uid_t)-1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != (gid_t)-1)
		vattr.va_mask |= AT_GID;
	return namesetattr(uap->fname, FOLLOW, &vattr, 0);
}

/* ARGSUSED */
int
lchown(uap, rvp)
	register struct chowna *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	if (uap->uid < (uid_t)-1 || uap->uid > MAXUID
	  || uap->gid < (gid_t)-1 || uap->gid > MAXUID)
		return EINVAL;
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != (uid_t)-1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != (gid_t)-1)
		vattr.va_mask |= AT_GID;
	return namesetattr(uap->fname, NO_FOLLOW, &vattr, 0);
}

/*
 * Change ownership of file given file descriptor.
 */
struct fchowna {
	int	fd;
	int	uid;
	int	gid;
};

/* ARGSUSED */
int
fchown(uap, rvp)
	register struct fchowna *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	if (uap->uid < (uid_t)-1 || uap->uid > MAXUID
	  || uap->gid < (gid_t)-1 || uap->gid > MAXUID)
		return EINVAL;
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != (uid_t)-1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != (gid_t)-1)
		vattr.va_mask |= AT_GID;
	return fdsetattr(uap->fd, &vattr);
}

/* XENIX Support */
/* 
 * Change file size.
 */
struct chsizea {
	int fdes;
	int size;
};

/* ARGSUSED */
chsize(uap, rvp)
	register struct chsizea *uap;
	rval_t *rvp;
{
	register vnode_t *vp;
	int error;
	file_t *fp;
	struct flock bf;

	if (uap->size < 0L || uap->size > u.u_rlimit[RLIMIT_FSIZE].rlim_cur)
		return EFBIG;
	if (error = getf(uap->fdes, &fp))
		return error;
	if ((fp->f_flag & FWRITE) == 0)
		return EBADF;
	vp = fp->f_vnode;
	if (vp->v_type != VREG)
		return EINVAL;         /* could have better error */
	bf.l_whence = 0;
	bf.l_start = uap->size;
	bf.l_type = F_WRLCK;
	bf.l_len = 0;
	if(error =  VOP_SPACE(vp, F_FREESP, &bf, fp->f_flag, fp->f_offset, u.u_cred))
		if (BADVISE_PRE_SV && (error == EAGAIN))
			error = EACCES;
	return error;
}

/* 
 * Read check.
 */
struct rdchka {
	int fdes;
};

/* ARGSUSED */
rdchk(uap, rvp)
	register struct rdchka *uap;
	rval_t *rvp;
{
	register vnode_t *vp;
	file_t *fp;
	vattr_t vattr;
	register int error;

	if ((error = getf(uap->fdes, &fp)) != 0)
		return error;
	if ((fp->f_flag & FREAD) == 0)
		return EBADF;
	vp = fp->f_vnode;
	if (vp->v_type == VCHR)
		error = spec_rdchk(vp, u.u_cred, &rvp->r_val1);
	else if (vp->v_type == VFIFO) {
		vattr.va_mask = AT_SIZE;
		if ((error = VOP_GETATTR(vp, &vattr, 0, u.u_cred)) != 0)
			return error;
		if (vattr.va_size > 0 || fifo_rdchk(vp) <= 0
		  || fp->f_flag & (FNDELAY|FNONBLOCK))
			rvp->r_val1 = 1;
		else
			rvp->r_val1 = 0;
	} else
		rvp->r_val1 = 1;

	return error;
}

/*
 * XENIX locking() system call.  Locking() is a system call subtype called
 * through the cxenix sysent entry.
 *
 * The following is a summary of how locking() calls map onto fcntl():
 *
 *	locking() 	new fcntl()	acts like fcntl()	with flock 
 *	 'mode'		  'cmd'		     'cmd'		 'l_type'
 *	---------	-----------     -----------------	-------------
 *
 *	LK_UNLCK	F_LK_UNLCK	F_SETLK			F_UNLCK
 *	LK_LOCK		F_LK_LOCK	F_SETLKW		F_WRLCK
 *	LK_NBLCK	F_LK_NBLCK	F_SETLK			F_WRLCK
 *	LK_RLCK		F_LK_RLCK	F_SETLKW		F_RDLCK
 *	LK_NBRLCK	F_LK_NBRLCK	F_SETLW			F_RDLCK
 *
 */
struct lockinga {
	int  fdes;
	int  mode;
	long arg;
};

/* ARGSUSED */
int
locking(uap, rvp)
	struct lockinga *uap;
	rval_t *rvp;
{
	file_t *fp;
	struct flock bf;
	struct o_flock obf;
	register int error, cmd, scolk;

	if (error = getf(uap->fdes, &fp))
		return error;

	/*
	 * Map the locking() mode onto the fcntl() cmd.
	 */
	switch (uap->mode) {
	case LK_UNLCK:
		cmd = F_SETLK;
		bf.l_type = F_UNLCK;
		break;
	case LK_LOCK:
		cmd = F_SETLKW;
		bf.l_type = F_WRLCK;
		break;
	case LK_NBLCK:
		cmd = F_SETLK;
		bf.l_type = F_WRLCK;
		break;
	case LK_RLCK:
		cmd = F_SETLKW;
		bf.l_type = F_RDLCK;
		break;
	case LK_NBRLCK:
		cmd = F_SETLK;
		bf.l_type = F_RDLCK;
		break;
	/* XENIX Support */
	case F_O_GETLK:
	case F_SETLK:
	case F_SETLKW:
		/*
		 * Kludge to some SCO fcntl/lockf
		 * x.outs (they map onto locking, instead of
		 * onto fcntl...).
		 */
/* Enhanced Application Compatibility Support */
		if (VIRTUAL_XOUT) {
/* End Enhanced Application Compatibility Support */
			cmd = uap->mode;
			scolk++;
			break;
		}
		else
			return EINVAL;
	/* End XENIX Support */
	default:
		return EINVAL;
	}
	if(scolk==0){
		bf.l_whence = 1;
		if (uap->arg < 0) {
			bf.l_start = uap->arg;
			bf.l_len = -(uap->arg);
		} else {
			bf.l_start = 0L;
			bf.l_len = uap->arg;
		}
	}
	else{				/* SCO fcntl/lockf */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof(struct o_flock)))
			return EFAULT;
		else
			bf.l_type = XMAP_TO_LTYPE(bf.l_type);	
	}


#ifdef _SECURE_CCA
	{
	    vnode_t *tvp;

	    tvp = fp->f_vnode;
	    /*
	     * This shouldn't be necessary, but coding the
	     * call this way allows the CCA tool to
	     * interpret the filesystem-specific code
	     * with only vnodes appropriate
	     * to each file system.
	     */
	    error = VOP_FRLOCK(tvp, cmd, &bf, fp->f_flag,
	      fp->f_offset, u.u_cred);
	}
#else
	error = VOP_FRLOCK(fp->f_vnode, cmd, &bf, fp->f_flag,
	  fp->f_offset, u.u_cred);
#endif

	if (error != 0) {
		if (BADVISE_PRE_SV && (error == EAGAIN))
			error = EACCES;
	}

	else {
	 	if (error == 0 && (uap->mode == F_SETLK || uap->mode == F_SETLKW)) 
			fp->f_vnode->v_flag |= VXLOCKED;
		if (uap->mode != F_SETLKW) {
			bf.l_type = XMAP_FROM_LTYPE(bf.l_type);
		}

		
		if(cmd == F_O_GETLK) {
			obf.l_type = bf.l_type;
			obf.l_whence = bf.l_whence;
			obf.l_start = bf.l_start;
			obf.l_len = bf.l_len;
			if(bf.l_sysid > SHRT_MAX || bf.l_pid > SHRT_MAX) 
					return EOVERFLOW;
			obf.l_sysid = (short) bf.l_sysid;
			obf.l_pid = (o_pid_t) bf.l_pid;
			if (copyout((caddr_t)&obf, (caddr_t)uap->arg, sizeof obf))
				return EFAULT;
		}
	}
	return error;
}
/* End XENIX Support */

/*
 * Set access/modify times on named file.
 */
struct utimea {
	char	*fname;
	time_t	*tptr;
};

/* ARGSUSED */
int
utime(uap, rvp)
	register struct utimea *uap;
	rval_t *rvp;
{
	time_t tv[2];
	struct vattr vattr;
	int flags = 0;

	if (uap->tptr != NULL) {
		if (copyin((caddr_t)uap->tptr,(caddr_t)tv, sizeof(tv)))
			return EFAULT;
		flags |= ATTR_UTIME;
	} else {
		tv[0] = hrestime.tv_sec;
		tv[1] = hrestime.tv_sec;
	}
	vattr.va_atime.tv_sec = tv[0];
	vattr.va_atime.tv_nsec = 0;
	vattr.va_mtime.tv_sec = tv[1];
	vattr.va_mtime.tv_nsec = 0;
	vattr.va_mask = AT_ATIME|AT_MTIME;
	return namesetattr(uap->fname, FOLLOW, &vattr, flags);
}

/*
 * Common routine for modifying attributes of named files.
 */
STATIC int
namesetattr(fnamep, followlink, vap, flags)
	char *fnamep;
	enum symfollow followlink;
	struct vattr *vap;
	int flags;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(fnamep, UIO_USERSPACE, followlink,
	  NULLVPP, &vp))
		return error;	
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
		error = EROFS;
	else {
              	/*
                 * Must have MAC write access to the file before calling
                 * VOP_SETATTR.
                 */
                error = MAC_VACCESS(vp, VWRITE, u.u_cred);
		if (!error)
		{
			MAC_ASSERT(vp, MAC_SAME); /* MAC write implies
						   same labels */
                        error = VOP_SETATTR(vp, vap, flags, u.u_cred);
		}
	}
	VN_RELE(vp);
	return error;
}

/*
 * Common routine for modifying attributes of files referenced
 * by descriptor.
 */
static int
fdsetattr(fd, vap)
	int fd;
	struct vattr *vap;
{
	file_t *fp;
	register vnode_t *vp;
	register int error;

	if ((error = getf(fd, &fp)) == 0) {
		vp = fp->f_vnode;
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
			return EROFS;
		/*
                 * Must have MAC write access to the file before calling
                 * VOP_SETATTR.
                 */
                error = MAC_VACCESS(vp, VWRITE, u.u_cred);
		if (!error)
		{
			MAC_ASSERT(vp, MAC_SAME); /* MAC write implies
						   same labels */
                        error = VOP_SETATTR(vp, vap, 0, u.u_cred);
		}
	}
	return error;
}

/*
 * Flush output pending for file.
 */
struct fsynca {
	int fd;
};

/* ARGSUSED */
int
fsync(uap, rvp)
	struct fsynca *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

#ifdef _SECURE_CCA
        if ((error = getf(uap->fd, &fp)) == 0) {
		if ((fp->f_flag & FWRITE) == 0)
			error = EBADF;
		else {
			vnode_t *tvp;

			tvp = fp->f_vnode;
			/*
			 * This shouldn't be necessary, but coding the
			 * call this way allows the CCA tool to
			 * interpret the filesystem-specific code
			 * with only vnodes appropriate
			 * to each file system.
			 */
			error = VOP_FSYNC(tvp, u.u_cred);
		}
	}
#else
	if ((error = getf(uap->fd, &fp)) == 0) {
		if ((fp->f_flag & FWRITE) == 0)
			error = EBADF;
		else
			error = VOP_FSYNC(fp->f_vnode, u.u_cred);
	}
#endif
	return error;
}

/*
 * File control.
 */

struct fcntla {
	int fdes;
	int cmd;
	int arg;
};

int
fcntl(uap, rvp)
	register struct fcntla *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int i, error;
	vnode_t *vp;
	off_t offset;
	int flag, fd;
	struct flock bf;
	struct o_flock obf;
	char flags;
	/* XENIX Support */
	register unsigned virt_xout = 0;
	/* End XENIX Support */

	if (error = getf(uap->fdes, &fp))
		return error;
	vp = fp->f_vnode;
	flag = fp->f_flag;
	offset = fp->f_offset;

	switch (uap->cmd) {

	case F_DUPFD:
		if ((i = uap->arg) < 0 
		  || i >= u.u_rlimit[RLIMIT_NOFILE].rlim_cur)
			error = EINVAL;
		else if ((error = ufalloc(i, &fd)) == 0) {
			setf(fd, fp);
			flags = getpof(fd);
			flags = flags & ~FCLOSEXEC;
			setpof(fd, flags);
			fp->f_count++;
			rvp->r_val1 = fd;
			break;
		}
		break;

	case F_GETFD:
		rvp->r_val1 = getpof(uap->fdes);
		break;

	case F_SETFD:
		(void) setpof(uap->fdes, (char)uap->arg);
		break;

	case F_GETFL:
		rvp->r_val1 = fp->f_flag+FOPEN;
		break;

	case F_SETFL:
		if ((uap->arg & (FNONBLOCK|FNDELAY)) == (FNONBLOCK|FNDELAY))
			uap->arg &= ~FNDELAY;
		if ((error = VOP_SETFL(vp, flag, uap->arg, u.u_cred)) == 0) {
			uap->arg &= FMASK;
			fp->f_flag &= (FREAD|FWRITE);
			fp->f_flag |= (uap->arg-FOPEN) & ~(FREAD|FWRITE);
		}
		break;

	case F_SETLK:
	case F_SETLKW:
		/*
		 * Must have MAC write access to the vnode in question.
		 * Shoud this be after the copyin()?
		 */
		if ((error = MAC_VACCESS(vp, VWRITE, u.u_cred)) != 0)
			break;

		MAC_ASSERT(vp, MAC_SAME); /* MAC write implies same labels */
		/* FALLTHROUGH */
	case F_GETLK:
	case F_O_GETLK:
		/*
		 * Copy in input fields only.
		 */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof obf)) {
			error = EFAULT;
			break;
		}
		/* XENIX Support */
		else {
			virt_xout = VIRTUAL_XOUT;
			/* Map lock type for XENIX binaries */
			if (virt_xout) 
				bf.l_type = XMAP_TO_LTYPE(bf.l_type);	
			/* Indicate to VOP_FRLOCK() that it was called by
			 * fcntl(), instead of from chklock(), etc.
			 * This info is needed to support XENIX behavior
			 * in VOP_FRLOCK().
			 */
			u.u_renv |= UB_FCNTL; 
		}
		/* End XENIX Support */
		if ((uap->cmd == F_SETLK || uap->cmd == F_SETLKW) &&
			bf.l_type != F_UNLCK) {
			setpof(uap->fdes, getpof(uap->fdes)|UF_FDLOCK);
		}
		if (error =
		  VOP_FRLOCK(vp, uap->cmd, &bf, flag, offset, u.u_cred)) {
			/*
			 * Translation for backward compatibility.
			 */
			/* XENIX Support */
			if ((error == EAGAIN) && !(virt_xout))
			/* End XENIX Support */
				error = EACCES;
			break;
		}
		/* XENIX Support */
		else {
			if (virt_xout) {
				/* Turn on lock enforcement bit */
				if (uap->cmd == F_SETLK || uap->cmd == F_SETLKW)
					vp->v_flag |= VXLOCKED;
				/* Map lock type for XENIX binaries */
				if (uap->cmd != F_SETLKW)
					bf.l_type = XMAP_FROM_LTYPE(bf.l_type);
			}
		}
		/* End XENIX Support */

		/*
		 * If command is GETLK and no lock is found, only
		 * the type field is changed.
		 */
		if ((uap->cmd == F_O_GETLK || uap->cmd == F_GETLK)
		  && bf.l_type == F_UNLCK) {
			if (copyout((caddr_t)&bf.l_type,
			  (caddr_t)&((struct flock *)uap->arg)->l_type,
			  sizeof(bf.l_type)))
				error = EFAULT;
			break;
		}

		if (uap->cmd == F_O_GETLK) {
			/*
			 * Return an SVR3 flock structure to the user.
			 */
			obf.l_type = bf.l_type;
			obf.l_whence = bf.l_whence;
			obf.l_start = bf.l_start;
			obf.l_len = bf.l_len;
			if (bf.l_sysid > SHRT_MAX || bf.l_pid > SHRT_MAX) {
				/*
				 * One or both values for the above fields
				 * is too large to store in an SVR3 flock
				 * structure.
				 */
				error = EOVERFLOW;
				break;
			}
			obf.l_sysid = (short) bf.l_sysid;
			obf.l_pid = (o_pid_t) bf.l_pid;
			if (copyout((caddr_t)&obf, (caddr_t)uap->arg,
			  sizeof obf))
				error = EFAULT;
		} else if (uap->cmd == F_GETLK) {
			/*
			 * Copy out SVR4 flock.
			 */
			int i;

			for (i = 0; i < 4; i++)
				bf.l_pad[i] = 0;
		    	if (copyout((caddr_t)&bf, (caddr_t)uap->arg, sizeof bf))
			  	error = EFAULT;
		}
		/* XENIX Support */
		if (virt_xout)
			u.u_renv &= ~UB_FCNTL;
		/* End XENIX Support */
		break;

	case F_RSETLK:
	case F_RSETLKW:
		/*
		 * Must have MAC write access to the vnode in question.
		 * Should this be after the copyin()?
		 */
		if ((error = MAC_VACCESS(vp, VWRITE, u.u_cred)) != 0)
			break;

		MAC_ASSERT(vp, MAC_SAME); /* MAC write implies same labels */
		/* FALLTHROUGH */
	case F_RGETLK:
		/*
		 * EFT only interface, applications cannot use
		 * this interface when _STYPES is defined.
		 * This interface supports an expanded
		 * flock struct--see fcntl.h.
		 */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof bf)) {
			error = EFAULT;
			break;
		}
		/* XENIX Support */
		else {
			virt_xout = VIRTUAL_XOUT;
			/* Map lock type for XENIX binaries */
			if (virt_xout) 
				bf.l_type = XMAP_TO_LTYPE(bf.l_type);	
			/* Indicate to VOP_FRLOCK() that it was called by
			 * fcntl(), instead of from chklock(), etc.
			 * This info is needed to support XENIX behavior
			 * in VOP_FRLOCK().
			 */
			u.u_renv |= UB_FCNTL; 
		}
		/* End XENIX Support */
		if (error =
		  VOP_FRLOCK(vp, uap->cmd, &bf, flag, offset, u.u_cred)) {
			/*
			 * Translation for backward compatibility.
			 */
		/* XENIX Support */
			if ((error == EAGAIN) && !(virt_xout))
		/* End XENIX Support */
				error = EACCES;
			break;
		}
		/* XENIX Support */
		else {
			if (virt_xout) {
				/* Turn on lock enforcement bit */
				if (uap->cmd == F_RSETLK || uap->cmd == F_RSETLKW)
					vp->v_flag |= VXLOCKED;
				/* Map lock type for XENIX binaries */
				if (uap->cmd != F_RSETLKW)
					bf.l_type = XMAP_FROM_LTYPE(bf.l_type);
			}
		}
		/* End XENIX Support */
		if (uap->cmd == F_RGETLK
		  && copyout((caddr_t)&bf, (caddr_t)uap->arg, sizeof bf))
			  error = EFAULT;
		/* XENIX Support */
		if (virt_xout)
			u.u_renv &= ~UB_FCNTL;
		/* End XENIX Support */
		break;

	case F_CHKFL:
		/*
		 * This is for internal use only, to allow the vnode layer
		 * to validate a flags setting before applying it.  User
		 * programs can't issue it.
		 */
		error = EINVAL;
		break;

	case F_ALLOCSP:
	case F_FREESP:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else if (vp->v_type != VREG)
			error = EINVAL;
		/*
		 * For compatibility we overlay an SVR3 flock on an SVR4
		 * flock.  This works because the input field offsets 
		 * in "struct flock" were preserved.
		 */
		else if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof obf))
			error = EFAULT;
		else {
			/*
			 * Must have MAC write access to the vnode in
			 * question.
			 */
			if ((error = MAC_VACCESS(vp, VWRITE, u.u_cred)) != 0) {
				break;
			} else {
				MAC_ASSERT(vp, MAC_SAME); /* MAC write implies
						   same labels */
				error = VOP_SPACE(vp, uap->cmd, &bf,
					flag, offset, u.u_cred);
			}
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * Duplicate a file descriptor.
 */
struct dupa {
	int	fdes;
};

int
dup(uap, rvp)
	register struct dupa *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;
	int fd;

	if (error = getf(uap->fdes, &fp))
		return error;
	if (error = ufalloc(0, &fd))
		return error;
	setf(fd, fp);
	fp->f_count++;
	rvp->r_val1 = fd;
	return 0;
}

/*
 * I/O control.
 */
struct ioctla {
	int fdes;
	int cmd;
	int arg;
};

int
ioctl(uap, rvp)
	register struct ioctla *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;
	register vnode_t *vp;
	struct vattr vattr;
	off_t offset;
	int flag;

	if (error = getf(uap->fdes, &fp))
		return error;
	vp = fp->f_vnode;

	if (vp->v_type == VREG || vp->v_type == VDIR) {
		/*
		 * Handle these two ioctls for regular files and
		 * directories.  All others will usually be failed
		 * with ENOTTY by the VFS-dependent code.  System V
		 * always failed all ioctls on regular files, but SunOS
		 * supported these.
		 */
		switch (uap->cmd) {
		case FIONREAD:
			if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred))
				return error;
			offset = vattr.va_size - fp->f_offset;
			if (copyout((caddr_t)&offset, (caddr_t)uap->arg, 
			    sizeof(offset)))
				return EFAULT;
			return 0;

		case FIONBIO:
			if (copyin((caddr_t)uap->arg, (caddr_t)&flag, 
			  sizeof(int)))
				return EFAULT;
			if (flag)
				fp->f_flag |= FNDELAY;
			else
				fp->f_flag &= ~FNDELAY;
			return 0;

		default:
			break;
		}
	}
#ifdef _SECURE_CCA
	{
	    vnode_t *tvp;

	    tvp = fp->f_vnode;
	    /*
	     * This shouldn't be necessary, but coding the call
	     * this way allows the CCA tool to interpret the
	     * filesystem-specific code with only vnodes appropriate
	     * to each file system.
	     */
	    error = VOP_IOCTL(tvp, uap->cmd, uap->arg,
		fp->f_flag, u.u_cred, &rvp->r_val1);
	}
#else
	error = VOP_IOCTL(fp->f_vnode, uap->cmd, uap->arg,
	    fp->f_flag, u.u_cred, &rvp->r_val1);
#endif
	if (error == 0) {
		switch (uap->cmd) {
		case FIONBIO:
			if (copyin((caddr_t)uap->arg, (caddr_t)&flag,
			  sizeof(int)))
				return EFAULT;		/* XXX */
			if (flag)
				fp->f_flag |= FNDELAY;
			else
				fp->f_flag &= ~FNDELAY;
			break;

		default:
			break;
	    }
	}
	return error;
}

/*
 * Old stty and gtty.  (Still.)
 */
struct sgttya {
	int	fdes;
	int	arg;
};

int
stty(uap, rvp)
	register struct sgttya *uap;
	rval_t *rvp;
{
	struct ioctla na;

	na.fdes = uap->fdes;
	na.cmd = TIOCSETP;
	na.arg = uap->arg;
	return ioctl(&na, rvp);
}

int
gtty(uap, rvp)
	register struct sgttya *uap;
	rval_t *rvp;
{
	struct ioctla na;

	na.fdes = uap->fdes;
	na.cmd = TIOCGETP;
	na.arg = uap->arg;
	return ioctl(&na, rvp);
}


/*
 * Poll file descriptors for interesting events.
 */
STATIC int pollwait;

struct polla {
	struct pollfd *fdp;
	unsigned long nfds;
	long	timo;
};

#if defined(__STDC__)
static void	polladd(struct pollhead *, int, void(*)(), long, struct polldat *);
static void	pollrun(proc_t *);
static void	polltime(proc_t *);
#else
static void	polladd();
static void	pollrun();
static void	polltime();
#endif

int
poll(uap, rvp)
	register struct polla *uap;
	rval_t *rvp;
{
	register int i, s;
	register fdcnt = 0;
	struct pollfd *pollp = NULL;
	struct pollfd parray[NFPCHUNK];
	clock_t t;
	int lastd;
	int rem;
	int id;
	int psize;
	int dsize;
	file_t *fp;
	struct pollhead *php;
	struct pollhead *savehp = NULL;
	struct polldat *darray;
	struct polldat *curdat;
	int error = 0;
	proc_t *p = u.u_procp;

	if (uap->nfds < 0 || uap->nfds > u.u_rlimit[RLIMIT_NOFILE].rlim_cur)
		return EINVAL;
	t = lbolt;

	/*
	 * Allocate space for the pollfd array and space for the
	 * polldat structures used by polladd().  Then copy in
	 * the pollfd array from user space.
	 */
	if (uap->nfds != 0) {
		psize = uap->nfds * sizeof(struct pollfd);
		if (uap->nfds <= NFPCHUNK)
			pollp = parray;
		else if ((pollp = (struct pollfd *) kmem_alloc(psize, KM_NOSLEEP)) == NULL)
				return EAGAIN;
		dsize = uap->nfds * sizeof(struct polldat);
		if ((darray = (struct polldat *) kmem_zalloc(dsize, KM_NOSLEEP)) == NULL) {
			if (pollp != parray)
				kmem_free((caddr_t)pollp, psize);
			return EAGAIN;
		}
		if (copyin((caddr_t)uap->fdp, (caddr_t)pollp, psize)) {
			error = EFAULT;
			goto pollout;
		}

		/*
		 * Chain the polldat array together.
		 */
		lastd = uap->nfds - 1;
		if (lastd > 0) {
			darray[lastd].pd_chain = darray;
			for (i = 0; i < lastd; i++) {
				darray[i].pd_chain = &darray[i+1];
			}
		} else {
			darray[0].pd_chain = darray;
		}
		curdat = darray;
	}

	/*
	 * Retry scan of fds until an event is found or until the
	 * timeout is reached.
	 */
retry:		

	/*
	 * Polling the fds is a relatively long process.  Set up the
	 * SINPOLL flag so that we can see if something happened
	 * to an fd after we checked it but before we go to sleep.
	 */
	p->p_pollflag = SINPOLL;
	if (savehp) {			/* clean up from last iteration */
		polldel(savehp, --curdat);
		savehp = NULL;
	}
	curdat = darray;
	php = NULL;
	for (i = 0; i < uap->nfds; i++) {
		s = splhi();
		if (pollp[i].fd < 0) 
			pollp[i].revents = 0;
		else if (pollp[i].fd >= u.u_nofiles || getf(pollp[i].fd, &fp))
			pollp[i].revents = POLLNVAL;
		else {
			php = NULL;
#ifdef _SECURE_CCA
			{
			    vnode_t *tvp;

			    tvp = fp->f_vnode;
			    /*
			     * This shouldn't be necessary, but coding the
			     * call this way allows the CCA tool to
			     * interpret the filesystem-specific code
			     * with only vnodes appropriate
			     * to each file system.
			     */
			    error = VOP_POLL(tvp, pollp[i].events, fdcnt,
				&pollp[i].revents, &php);
			}
#else
			error = VOP_POLL(fp->f_vnode, pollp[i].events, fdcnt,
			    &pollp[i].revents, &php);
#endif
			if (error) {
				splx(s);
				goto pollout;
			}
		}
		if (pollp[i].revents)
			fdcnt++;
		else if (fdcnt == 0 && php) {
			polladd(php, pollp[i].events, pollrun,
			  (long)p, curdat++);
			savehp = php;
		}
		splx(s);
	}
	if (fdcnt) 
		goto pollout;

	/*
	 * If you get here, the poll of fds was unsuccessful.
	 * First make sure your timeout hasn't been reached.
	 * If not then sleep and wait until some fd becomes
	 * readable, writable, or gets an exception.
	 */
	rem = uap->timo < 0 ? 1 : uap->timo - ((lbolt - t)*1000)/HZ;
	if (rem <= 0)
		goto pollout;

	s = splhi();

	/*
	 * If anything has happened on an fd since it was checked, it will
	 * have turned off SINPOLL.  Check this and rescan if so.
	 */
	if (!(p->p_pollflag & SINPOLL)) {
		splx(s);
		goto retry;
	}
	p->p_pollflag &= ~SINPOLL;

	if (uap->timo > 0) {
		/*
		 * Turn rem into milliseconds and round up.
		 */
		rem = ((rem/1000) * HZ) + ((((rem%1000) * HZ) + 999) / 1000);
		p->p_pollflag |= SPOLLTIME;
		id = timeout((void(*)())polltime, (caddr_t)p, rem);
	}

	/*
	 * The sleep will usually be awakened either by this poll's timeout 
	 * (which will have cleared SPOLLTIME), or by the pollwakeup function 
	 * called from either the VFS, the driver, or the stream head.
	 */
	if (sleep((caddr_t)&pollwait, (PZERO+1)|PCATCH)) {
		if (uap->timo > 0)
			untimeout(id);
		splx(s);
		error = EINTR;
		goto pollout;
	}
	splx(s);

	/*
	 * If SPOLLTIME is still set, you were awakened because an event
	 * occurred (data arrived, can write now, or exceptional condition).
	 * If so go back up and poll fds again. Otherwise, you've timed
	 * out so you will fall through and return.
	 */
	if (uap->timo > 0) {
		if (p->p_pollflag & SPOLLTIME) {
			untimeout(id);
			goto retry;
		}
	} else
		goto retry;

pollout:

	/*
	 * Poll cleanup code.
	 */
	p->p_pollflag = 0;
	if (savehp)
		polldel(savehp, --curdat);
	if (error == 0) {
		/*
		 * Copy out the events and return the fdcnt to the user.
		 */
		rvp->r_val1 = fdcnt;
		if (uap->nfds != 0)
			if (copyout((caddr_t)pollp, (caddr_t)uap->fdp, psize))
				error = EFAULT;
	}
	if (uap->nfds != 0) {
		kmem_free((caddr_t)darray, dsize);
		if (pollp != parray)
			kmem_free((caddr_t)pollp, psize);
	}
	return error;
}

/*
 * This function is placed in the callout table to time out a process
 * waiting on poll.  If the poll completes, this function is removed
 * from the table.  Its argument is a flag to the caller indicating a
 * timeout occurred.
 */
static void
polltime(p)
	register proc_t *p;
{
	if (p->p_wchan == (caddr_t)&pollwait) {
		setrun(p);
		p->p_pollflag &= ~SPOLLTIME;
	}
}

/*
 * This function is called to inform a process that
 * an event being polled for has occurred.
 */
static void
pollrun(p)
	register proc_t *p;
{
	register int s;

	s = splhi();
	if (p->p_wchan == (caddr_t)&pollwait) {
		if (p->p_stat == SSLEEP)
			setrun(p);
		else
			unsleep(p);
	}
	p->p_pollflag &= ~SINPOLL;
	splx(s);
}

STATIC int pollcoll = 0;	/* collision counter (temporary) */

/*
 * This function allocates a polldat structure, fills in the given
 * data, and places it on the given pollhead list.  This routine MUST
 * be called at splhi() to avoid races.
 */
static void
polladd(php, events, fn, arg, pdp)
	register struct pollhead *php;
	short events;
	void (*fn)();
	long arg;
	register struct polldat *pdp;
{
	pdp->pd_events = events;
	pdp->pd_fn = fn;
	pdp->pd_arg = arg;
	if (php->ph_list) {
		pdp->pd_next = php->ph_list;
		php->ph_list->pd_prev = pdp;
		if (php->ph_events & events)
			pollcoll++;
	} else {
		pdp->pd_next = NULL;
	}
	pdp->pd_prev = (struct polldat *)php;
	pdp->pd_headp = php;
	php->ph_list = pdp;
	php->ph_events |= events;
}

/*
 * This function frees all the polldat structures related by
 * the sibling chain pointer.  These were all the polldats
 * allocated as the result of one poll system call.  Because
 * of race conditions, pdp may not be on php's list.
 */
void
polldel(php, pdp)
	register struct pollhead *php;
	register struct polldat *pdp;
{
	register struct polldat *p;
	register struct polldat *startp;
	register int s;

	s = splhi();
	for (p = php->ph_list; p; p = p->pd_next) {
		if (p == pdp) {
			startp = p;
			do {
				if (p->pd_headp != NULL) {
					if (p->pd_next)
						p->pd_next->pd_prev =
						  p->pd_prev;
					p->pd_prev->pd_next = p->pd_next;

					/*
					 * Recalculate the events on the list.
					 * The list is short - trust me.
					 * Note reuse of pdp here.
					 */
					p->pd_headp->ph_events = 0;
					for (pdp = p->pd_headp->ph_list;
					  pdp; pdp = pdp->pd_next)
						p->pd_headp->ph_events |=
						  pdp->pd_events;
					p->pd_next = NULL;
					p->pd_prev = NULL;
					p->pd_headp = NULL;
				}
				p = p->pd_chain;
			} while (p != startp);
			splx(s);
			return;
		}
	}
	splx(s);
}


/******************************************************************
** filepriv(): set or get privileges of the named file.
**
**	       Takes four parameters:
**
**		path -> pointer to file name.
**
**		cmd ->  command type.
**			Depends on privilege mechanism in use.
**
**		privp ->  pointer to an array of PRIDs which are
**			  a list of longs.
**		count ->  number of PRIDs contained in privp.
**
** This syscall returns the value in error.
**
*/
struct filepriva {
	char	*fname;
	int	cmd;
	priv_t	*privp;
	int	count;
};

/* ARGSUSED */
int
filepriv(uap, rvp)
	register struct filepriva *uap;
	rval_t *rvp;
{
	struct vnode *vp;
	struct	vattr	vattr;
	register int error = 0;
	register int x_error = 0;

	if (error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, NULLVPP, &vp))
		return error;

	if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred)) {
		VN_RELE(vp);
		return error;
	}

	/*
	 * If the file we're looking at isn't a regular file or there
	 * are no exec bits on in the mode, set error to EINVAL
	 * but don't return just yet since other conditions must be
	 * checked by pm_file().
	 */
	if (vp->v_type != VREG || (((vattr.va_mode & 0111) == 0))) {
		x_error = EINVAL;
	}
	error = pm_file(uap->cmd, vp, &vattr, rvp, u.u_cred, uap->privp, uap->count);
	VN_RELE(vp);
	/*
	 * This entire check of the error and x_error values is to close
	 * a potential covert channel.  Simply stated, if EACCES is returned
	 * from pm_file() then that must be the error reported even if the
	 * file is not a regular file or has any execute bits turned on in
	 * the mode.
	 */
	if (error) {
		if (error == EACCES)
			return error;
		else if (!x_error)
			return error;
	}
	return x_error;

}	/* end of filepriv system call */


/*
 * This system call gets or sets the MAC level of a file.
 * With the MAC module installed, processing is handled within
 * the MAC module by clvlfile().  Without the MAC module
 * installed, a process with P_SETFLEVEL privilege can set the
 * level of a file on a file system which supports labels.
 */

struct lvlfilea {
	char	*path;
	int	cmd;
	lid_t	*lidp;
};

#if defined(__STDC__)
STATIC int	clvlfile(struct vnode *, int, lid_t *);
#else
STATIC int	clvlfile();
#endif

/* ARGSUSED */
int
lvlfile(uap, rvp)
	register struct lvlfilea *uap;
	rval_t *rvp;
{
	struct vnode *vp;
	int error;

	if ((error = lookupname(uap->path, UIO_USERSPACE, FOLLOW, NULLVPP, &vp))
		== 0) {
			/* don't copyin lidp 'cause clvlfile() will do it */
			error = clvlfile(vp, uap->cmd, uap->lidp);
			VN_RELE(vp);
	}
	
	return error;
}




/*
 * This is the system call entry point to get or set the level of a file
 * by passing a file descriptor. The pathname is resolved to a vnode and a
 * common routine is called to handle the actual processing.
 */

struct flvlfilea {
	int	fildes;
	int	cmd;
	lid_t	*lidp;
};

/* ARGSUSED */
int
flvlfile(uap,rvp)
	register struct flvlfilea *uap;
	rval_t *rvp;
{
	struct file *fp;
	int error;
	struct vnode *vp;


	if (error = getf(uap->fildes, &fp))
		return error;
	vp = fp->f_vnode;

	/*
	 * Can do a set on a block or character special device only.
	 * If anything else, return EINVAL.
	 */

	if (uap->cmd == MAC_SET && vp->v_type != VCHR && vp->v_type != VBLK)
		return ENODEV;

	/* don't copyin lidp 'cause clvlfile() will do it*/

	return clvlfile(vp, uap->cmd, uap->lidp);
}

/*
 * This is the common routine which handles the processing for both the
 * lvlfile() and flvlfile system calls. It returns 0 on success, EACCES
 * if the process does not have the appropriate MAC access, and EPERM
 * if the process does not have the appropriate privilege to set the level.
 */

STATIC int
clvlfile(vp, cmd, usrlevel)
	struct vnode *vp;
	int cmd;
	lid_t *usrlevel;
{
	lid_t worklevel;
	int error;
	int setattempt = 0;

	switch (cmd) {
	case MAC_SET:
		/* check for read-only fs */
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			error = EROFS;
			break;
		}
		/* if copyin fails, get out */
		if (copyin((caddr_t)usrlevel, (caddr_t)&worklevel,
		     sizeof(lid_t)) == -1)  {
			error = EFAULT;
			break;
		}
		/* if lid is invalid, get out */
		if (mac_valid(worklevel)) {
			error = EINVAL;
			break;
		}
		/*
		 * VN_MACCHGON will return EBUSY if the VMAC_OPEN flag
		 * is set, this will prevent the current process from
		 * sleeping indefinitely.
		 */
		VN_MACCHGON(vp);
		setattempt++;
		/*
		 * Device special files are handled separately by
		 * SPECFS when VOP_SETLEVEL is called.
		 */
		if (vp->v_type != VCHR && vp->v_type != VBLK) {
			/*
			 * Must have MAC write access.
			 */
			if ((error = MAC_VACCESS(vp, VWRITE, u.u_cred)) != 0)
				break;
                        MAC_ASSERT(vp, MAC_SAME); /* MAC write implies same
                                                   * labels */
			/*
			 * Must either have P_SETFLEVEL privilege, or
			 * (if new level dominates old) P_MACUPGRADE
			 * privilege.
			 */
			if (error = pm_denied(u.u_cred, P_SETFLEVEL)) {
			    /* see if new level dominates old */
			    if (MAC_ACCESS(MACDOM, worklevel, vp->v_lid) == 0) {
				/* yep. see if we have upgrade privilege */
				if (error = pm_denied(u.u_cred, P_MACUPGRADE))
					break;
				/* nope. take error from 1st pm_denied call.*/
			    } else break;
			}
		}
		/*
		 * The file system dependent code is responsible
		 * for updating the vnode and inode level.
		 */
		error = VOP_SETLEVEL(vp, worklevel, u.u_cred);
		break;

	case MAC_GET:
		/*
		 * Must have MAC read access to the file.
		 */
		if ((error = MAC_VACCESS(vp, VREAD, u.u_cred)) != 0)
			break;
                MAC_ASSERT (vp, MAC_DOMINATES); /* MAC read access implies
                                                 * dominates */
		if (copyout((caddr_t)&vp->v_lid, (caddr_t)usrlevel,
		     sizeof(lid_t)) == -1)
			error = EFAULT;
		break;
		
	default:
		error = EINVAL;
		break;
	} /*switch (cmd)*/

	if (setattempt)
		VN_MACCHGDONE(vp);
	return error;
}

