/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_open.c	1.5.6.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/libnsl/nsl/t_open.c,v 1.1 91/02/28 20:52:04 ccs Exp $"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/errno.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/tihdr.h"
#include "sys/timod.h"
#include "sys/tiuser.h"
#include "sys/signal.h"
#include "sys/fcntl.h"
#include "_import.h"


extern struct _ti_user *_ti_user;
extern int t_errno;
extern int errno;
extern long openfiles;
extern char *calloc();
extern void (*sigset())();
extern int ioctl();
extern int open();
extern int close();
extern long ulimit();


t_open(path, flags, info)
char *path;
int flags;
register struct t_info *info;
{
	int retval, fd;
	struct T_info_ack inforeq;
	register struct _ti_user *tiptr;
	int retlen;
	void (*sigsave)();

	if (!(flags & O_RDWR)) {
		errno = 0;
		t_errno = TBADFLAG;
		return (-1);
	}

	if ((fd = open(path, flags)) < 0) {
		t_errno = TSYSERR;
		return(-1);
	}

	/*
	 * is module already pushed
	 */
	if ((retval = ioctl(fd, I_FIND, "timod")) < 0) {
		t_errno = TSYSERR;
		close(fd);
		return(-1);
	}

	if (!retval)
		if (ioctl(fd, I_PUSH, "timod") < 0) {
			t_errno = TSYSERR;
			close(fd);
			return(-1);
		}
	

	
	sigsave = sigset(SIGPOLL, SIG_HOLD);
	inforeq.PRIM_type = T_INFO_REQ;

	if (!_t_do_ioctl(fd, (caddr_t)&inforeq, sizeof(struct T_info_req), TI_GETINFO, &retlen)) {
		sigset(SIGPOLL, sigsave);
		close(fd);
		return(-1);
	}
	sigset(SIGPOLL, sigsave);
		
	if (retlen != sizeof(struct T_info_ack)) {
		t_errno = TSYSERR;
		errno = EIO;
		close(fd);
		return(-1);
	}

	if (info != NULL) {
		info->addr = inforeq.ADDR_size;
		info->options = inforeq.OPT_size;
		info->tsdu = inforeq.TSDU_size;
		info->etsdu = inforeq.ETSDU_size;
		info->connect = inforeq.CDATA_size;
		info->discon = inforeq.DDATA_size;
		info->servtype = inforeq.SERV_type;
	}

	/*
	 * if routine never been called before
	 * then allocate the ti_user structures
	 * for all file desc.
	 */
	 if (!_ti_user) { 
		openfiles = OPENFILES;
		if ((_ti_user = (struct _ti_user *)calloc(1, (unsigned)(openfiles*sizeof(struct _ti_user)))) == NULL) {
			t_errno = TSYSERR;
			close(fd);
			return(-1);
		}
	}


	/*
	 * if first time done then initialize data structure
	 * and allocate buffers
	 */
	tiptr = &_ti_user[fd];
	if (tiptr->ti_flags & USED) {

		/*
		 * This is the case when fd was closed (not!! t_close)
		 * before a fork(). We need to clean up tiptr and allocate
		 * bufs with the current inforeq (it is conceivable that
		 * the provider atributes are different from those
		 * that are in tiptr from an earlier open).
		 *
		 * After a fork() & exec() this would however not be true.
		 */
		if (tiptr->ti_rcvbuf != NULL)
			(void)free(tiptr->ti_rcvbuf);
		if (tiptr->ti_lookdbuf != NULL)
			(void)free(tiptr->ti_lookdbuf);
		(void)free(tiptr->ti_ctlbuf);
		(void)free(tiptr->ti_lookcbuf);
		_null_tiptr(tiptr);
	}
	if ((_t_alloc_bufs(fd, tiptr, inforeq) < 0) ||
	    (ioctl(fd, I_FLUSH, FLUSHRW) < 0)) {
		t_close(fd);
		t_errno = TSYSERR;
		return(-1);
	}

	tiptr->ti_state = TLI_NEXTSTATE(T_OPEN, tiptr->ti_state);
	return(fd);
}
