/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/fio.c	1.5.3.5"
#ident	"$Header: $"

#include <fs/file.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/resource.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/sysinfo.h>
#include <util/types.h>

/*
 * Common code to get to the appropriate file table entry slot.
 *
 * Before the call, fd contains the requested file descriptor;
 * after the call, fd contains the relative index to the current
 * ufp (which points to the latest file chunk).
 */
#define	GETF(fd, ufp) {					\
	(ufp) = &u.u_flist;				\
	while ((fd) >= NFPCHUNK) {			\
		(fd) -= NFPCHUNK;			\
		(ufp) = (ufp)->uf_next;			\
	}						\
}

/*
 * Common code to free a file table entry.
 */
#define	UNFALLOC(fp) {					\
	if ((fp)->f_prev)				\
		(fp)->f_prev->f_next = (fp)->f_next;	\
	else						\
		file = (fp)->f_next;			\
	if ((fp)->f_next)				\
		(fp)->f_next->f_prev = (fp)->f_prev;	\
	crfree((fp)->f_cred);				\
	kmem_free((caddr_t)(fp), sizeof(file_t));	\
	filecnt--;					\
}

STATIC unsigned int filecnt;	/* number of entries in file list */
KSTATIC struct file *file;	/* file list */

/*
 * Convert a user supplied file descriptor into a pointer to a file
 * structure.  Only task is to check range of the descriptor (soft
 * resource limit was enforced at open time and shouldn't be checked
 * here).
 */
int
getf(fd, fpp)
	register int fd;
	struct file **fpp;
{
	register struct ufchunk *ufp;
	register struct file *fp;

	if (fd < 0)
		return EBADF;

	/*
	 * Most commonly fd will be small -- optimize this case.  Note
	 * that code elsewhere must guarantee (u.u_nofiles >= NFPCHUNK).
	 * At system startup time, u_nofiles of the first process is
	 * initialized to NFPCHUNK.  At process creation time, a child
	 * process inherits u_nofiles from its parent.
	 */
	if (fd < NFPCHUNK)  {
		if ((fp = u.u_flist.uf_ofile[fd]) == NULLFP)
			return EBADF;
		*fpp = fp;
		return 0;
	}

	if (fd >= u.u_nofiles)
		return EBADF;
	
	GETF(fd, ufp);	/* fd and ufp are modified */

	if ((fp = ufp->uf_ofile[fd]) == NULLFP)
		return EBADF;

	*fpp = fp;
	return 0;
}

/*
 * Close all open files of the current process (as part of cleaning
 * up a process, e.g., at process exit time).  If flag is set, zero
 * the file descriptor entries as well.
 */
void
closeall(flag)
	register int flag;
{
	register int i;
	file_t *fp;

	for (i = 0; i < u.u_nofiles; i++) {
		if (getf(i, &fp) == 0) {
			(void)closef(fp);
			if (flag)
				setf(i, NULLFP);
		}
	}
}

/*
 * Internal form of close.  Decrement reference count on file
 * structure.  Decrement reference count on the vnode following
 * removal of the referencing file structure.
 */
int
closef(fp)
	register struct file *fp;
{
	register struct vnode *vp;
	register int error;

	/*
	 * Sanity check.
	 */
	if (fp == NULL || fp->f_count <= 0)
		return 0;

 	stop_aio(u.u_procp, fp);

	vp = fp->f_vnode;
	error =
	  VOP_CLOSE(vp, fp->f_flag, fp->f_count, fp->f_offset, fp->f_cred);
	if ((unsigned)fp->f_count > 1) {
		fp->f_count--;
		return error;
	}

/* XENIX Support */
	if (vp->v_type == VXNAM)
		closesem(fp, vp);
	vp->v_flag &= ~VXLOCKED;
/* End XENIX Support */

	VN_RELE(vp);
	UNFALLOC(fp);
	return error;
}

/*
 * Allocate a user file descriptor greater than or equal to "start" (supplied).
 */
int
ufalloc(start, fdp)
	int start;
	int *fdp;
{
	register struct ufchunk *ufp;
	register int j;
	register int count;
	register int i = start;

	/*
	 * First look for an unused entry.
	 */
	if (i < u.u_nofiles) {
		count = i / NFPCHUNK;
		for (ufp = &u.u_flist; count > 0; count--)
			ufp = ufp->uf_next;
		count = i / NFPCHUNK;
		for (; i < u.u_nofiles; i++) {
			if (i >= u.u_rlimit[RLIMIT_NOFILE].rlim_cur)
				return EMFILE;
			j = i / NFPCHUNK;
			if (j > count) {
				count = j;
				ufp = ufp->uf_next;
			}
			j = i % NFPCHUNK;
			if (ufp->uf_ofile[j] == NULL) {
				u.u_rval1 = i;	/* useful to old drivers */
				ufp->uf_pofile[j] = 0;
				*fdp = i;
				return 0;
			}
		}
		i = start;
	} else {
		if (i >= u.u_rlimit[RLIMIT_NOFILE].rlim_cur)
			return EMFILE;
	}

	/*
	 * We need to allocate more memory.
	 */
	if (u.u_nofiles >= u.u_rlimit[RLIMIT_NOFILE].rlim_cur)
		return EMFILE;
	for (ufp = &u.u_flist; ufp->uf_next; ufp = ufp->uf_next)
		;
	if (i == 0) {		/* just allocate one more chunk */
		ufp->uf_next = (struct ufchunk *)
			kmem_zalloc(sizeof(struct ufchunk), KM_SLEEP);
		u.u_rval1 = u.u_nofiles;	/* useful to old drivers */
		*fdp = u.u_nofiles;
		u.u_nofiles += NFPCHUNK;
	} else {

		/*
		 * We could enter this path under two different circumstances.
		 * First, "start" could have been less than u.u_nofiles, but
		 * greater than zero, and there were no free fds.  Here, we
		 * just want to allocate one more chunk and return the first
		 * slot in the chunk.  The second case is when "start" is
		 * greater than or equal to u.u_nofiles.  In this case, we
		 * need to allocate enough chunks to return fd number "start".
		 *
		 * The following calculation computes the number of chunks to
		 * be allocated (it can be less than zero, but this is okay).
		 * The do-while loop allocates at least one chunk.
		 *
		 * Alternatively, we could just jump back to the beginning
		 * of this routine, but that would be less efficient.
		 */
		j = ((i / NFPCHUNK) + 1) - (u.u_nofiles / NFPCHUNK);
		count = u.u_nofiles;
		do {
			ufp->uf_next = (struct ufchunk *)
				kmem_zalloc(sizeof(struct ufchunk), KM_SLEEP);
			u.u_nofiles += NFPCHUNK;
			ufp = ufp->uf_next;
		} while (--j > 0);
		if (i < count) {
			u.u_rval1 = count;	/* useful to old drivers */
			*fdp = count;
		} else {
			u.u_rval1 = i;		/* useful to old drivers */
			*fdp = i;
		}
	}
	return 0;
}

/*
 * Allocate a user file descriptor and a file structure.
 * Initialize the descriptor to point at the file structure.
 *
 * file table overflow -- if there are no available file structures.
 */
int
falloc(vp, flag, fpp, fdp)
	struct vnode *vp;
	int flag;
	struct file **fpp;
	int *fdp;
{
	register struct file *fp;
	int fd;
	register int error;

	if (error = ufalloc(0, &fd))
		return error;
	fp = (file_t *)kmem_zalloc(sizeof(file_t), KM_SLEEP);
	if (file)
		file->f_prev = fp;
	fp->f_next = file;
	fp->f_prev = NULLFP;
	file = fp;
	filecnt++;
	setf(fd, fp);
	fp->f_count++;
	fp->f_flag = (ushort)flag;
	fp->f_vnode = vp;
	fp->f_offset = 0;
	crhold(u.u_cred);
	fp->f_cred = u.u_cred;
	*fpp = fp;
	*fdp = fd;
	return 0;
}

/*
 * Initialize the file table.  Called at system startup time.
 */
void
finit()
{
	file = NULLFP;
	filecnt = 0;
}

/*
 * Decrement the reference count of a file table entry.
 * Free file table entry when there are no references to the entry.
 * Called by (error recovery) code to cleanup when file table entry has
 * already been allocated.
 */
void
unfalloc(fp)
	register struct file *fp;
{
	if (--fp->f_count <= 0)
		UNFALLOC(fp);
}

/*
 * Given a file descriptor, set the user's
 * file pointer to the given parameter.
 */
void
setf(fd, fp)
	register int fd;
	struct file *fp;
{
	register struct ufchunk *ufp;

	ASSERT(0 <= fd && fd < u.u_nofiles);
	GETF(fd, ufp);	/* fd and ufp are modified */
	ufp->uf_ofile[fd] = fp;
}

/*
 * Given a file descriptor, return the user's file flags.
 */
char
getpof(fd)
	register int fd;
{
	register struct ufchunk *ufp;

	if (fd >= u.u_nofiles)
		return 0;
	GETF(fd, ufp);	/* fd and ufp are modified */
	return ufp->uf_pofile[fd];
}

/*
 * Given a file descriptor and file flags,
 * set the user's file flags.
 */
void
#ifdef __STDC__
setpof(register int fd, char flags)
#else
setpof(fd, flags)
	register int fd;
	char flags;
#endif
{
	register struct ufchunk *ufp;

	ASSERT(0 <= fd && fd < u.u_nofiles);
	GETF(fd, ufp);	/* fd and ufp are modified */
	ufp->uf_pofile[fd] = flags;
}

/*
 * Allocate a file descriptor and assign it to the vnode "*vpp",
 * performing the usual open protocol upon it and returning the
 * file descriptor allocated.  It is the responsibility of the
 * caller to dispose of "*vpp" if any error occurs.
 */
int
fassign(vpp, mode, fdp)
	struct vnode **vpp;
	int mode;
	int *fdp;
{
	struct file *fp;
	register int error;
	int fd;

	if (error = falloc((struct vnode *)NULL, mode & FMASK, &fp, &fd))
		return error;
	if (error = VOP_OPEN(vpp, mode, u.u_cred)) {
		setf(fd, NULLFP);
		unfalloc(fp);
		return error;
	}
	fp->f_vnode = *vpp;
	*fdp = fd;
	return 0;
}
