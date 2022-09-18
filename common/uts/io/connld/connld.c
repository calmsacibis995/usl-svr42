/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:io/connld/connld.c	1.9.3.3"
#ident	"$Header: $"
/*
 * This module establishes a unique connection on
 * a STREAMS-based pipe.
 */

#include <acc/priv/privilege.h>
#include <fs/fifofs/fifonode.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Define local and external routines.
 */
STATIC	int connopen(), connclose(), connput();
STATIC	struct vnode *connvnode();
extern int fifo_close();
extern int strclose(), strioctl();

/*
 * Define STREAMS header information.
 */
STATIC struct module_info conn_info = {
	1003, 
	"conn", 
	0, 
	INFPSZ, 
	STRHIGH, 
	STRLOW 
};
STATIC struct qinit connrinit = { 
	connput, 
	NULL, 
	connopen, 
	connclose, 
	NULL, 
	&conn_info, 
	NULL 
};
STATIC struct qinit connwinit = { 
	connput, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	&conn_info, 
	NULL
};
struct streamtab conninfo = { 
	&connrinit, 
	&connwinit 
};

int conndevflag = D_OLD;

/*
 * For each invokation of connopen(), create a new pipe. One end of the pipe 
 * is sent to the process on the other end of this STREAM. The vnode for 
 * the other end is returned to the open() system call as the vnode for 
 * the opened object.
 *
 * On the first invokation of connopen(), a flag is set and the routine 
 * retunrs 0, since the first open corresponds to the pushing of the module.
 */
/*ARGSUSED*/
STATIC	int
connopen(rqp, dev, flag, sflag)
queue_t *rqp;
dev_t dev;
int flag;
int sflag;
{
	register int error = 0;
	struct vnode *vp1;
	struct vnode *vp2;
	struct vnode *streamvp = NULL; 
	struct file *filep;
	struct fifonode *streamfnp = NULL;
	struct fifonode *matefnp = NULL;
	int fd, rvalp;

	if ((streamvp = connvnode(rqp)) == NULL) {
		u.u_error = EINVAL;
		return(OPENFAIL);
	}
	/*
	 * CONNLD is only allowed to be pushed onto a "pipe" that has both 
	 * of its ends open.
	 */
	if (streamvp->v_type != VFIFO) {
		u.u_error = EINVAL;
		return(OPENFAIL);
	}
	if (!(VTOF(streamvp)->fn_flag & ISPIPE) || 
		!(VTOF(streamvp)->fn_mate)) {
			u.u_error = EPIPE;
			return(OPENFAIL);
	}

	/*
	 * If this is the first time CONNLD was opened while on this stream,
	 * it is being pushed. Therefore, set a flag and return 0.
	 */
	if ((int)rqp->q_ptr == 0) { 
		rqp->q_ptr = (caddr_t)1;
		/*
		 * if process has MAC_WRITE priv then set FIFOMACPRIV,
		 * which will bypass the MAC check in fifo_open.
		 */
		if (pm_denied(u.u_cred, P_MACWRITE)) 
			VTOF(streamvp)->fn_flag &= ~FIFOMACPRIV;
		else
			VTOF(streamvp)->fn_flag |= FIFOMACPRIV;
		return (0);
	}

	/*
	 * Make pipe ends.
	 */
	if (error = fifo_mkpipe(&vp1, &vp2, u.u_cred))
		return (error);

	/*
	 * Allocate a file descriptor and file pointer for one of the pipe 
	 * ends. The file descriptor will be used to send that pipe end to 
	 * the process on the other end of this stream.
	 */
	if (error = falloc(vp1, FWRITE|FREAD, &filep, &fd)) {
		fifo_rmpipe(vp1, vp2, u.u_cred);
		return (error);
	}

	/*
	 * Send one end of the new pipe to the process on the other 
	 * end of this pipe and block until the other process
	 * received it.
	 * If the other process exits without receiving it, fail this open
	 * request.
	 * Note that fifo_rmpipe() cannot be called on failure after
	 * strioctl() has been called to send the fd.
	 */
	streamfnp = VTOF(streamvp);
	matefnp = VTOF(streamfnp->fn_mate);
	matefnp->fn_flag |= FIFOSEND;
	error = strioctl(streamvp, I_SENDFD, fd, flag, K_TO_K, filep->f_cred,
	    &rvalp);
	if (error != 0)
		goto out;

	while (matefnp->fn_flag & FIFOSEND) {
		if (sleep((caddr_t) &matefnp->fn_unique, PPIPE|PCATCH)) {
			error = OPENFAIL;
			goto out;
		}
		if (streamfnp->fn_mate == NULL) {
			error = OPENFAIL;
			goto out;
		}
	}
	/*
	 * all is okay...return new pipe end to user
	 */
	streamfnp->fn_unique = vp2;
	streamfnp->fn_flag |= FIFOPASS;
	closef(filep);
	setf(fd, NULLFP);
	return (0);
out:
	streamfnp->fn_unique = NULL;
	streamfnp->fn_flag &= ~FIFOPASS;
	matefnp->fn_flag &= ~FIFOSEND;
	fifo_close(vp2, 0, 0, 0, u.u_cred);
	closef(filep);
	setf(fd, NULLFP);
	return (error);
}

/*ARGSUSED*/
STATIC	int
connclose(q)
queue_t *q;
{
	return (0);
}

/*
 * Use same put procedure for write and read queues.
 */
STATIC	int
connput(q, bp)
queue_t *q;
mblk_t *bp;
{
	putnext(q, bp);
	return (0);
}

/*
 * Get the vnode for the stream connld is push onto. Follow the 
 * read queue until the stream head is reached. The vnode is taken 
 * from the stdata structure, which is obtaine from the q_ptr field 
 * of the queue.
 */
STATIC struct vnode *
connvnode(qp)
queue_t *qp;
{
	queue_t *tempqp;
	struct vnode *streamvp = NULL;
	
	for(tempqp = qp; tempqp->q_next; tempqp = tempqp->q_next)
		;
	if (tempqp->q_qinfo != &strdata)
		return (NULL);
	streamvp = ((struct stdata *)(tempqp->q_ptr))->sd_vnode;
	return (streamvp);
}
