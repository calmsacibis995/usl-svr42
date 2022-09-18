/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_sync.c	1.4.8.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/libnsl/nsl/t_sync.c,v 1.1 91/02/28 20:52:25 ccs Exp $"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/errno.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/tihdr.h"
#include "sys/timod.h"
#include "sys/tiuser.h"
#include "sys/signal.h"
#include "_import.h"

extern struct _ti_user *_ti_user;
extern long openfiles;
extern struct _ti_user *_t_checkfd();
extern int t_errno;
extern int errno;
extern void (*sigset())();
extern _null_tiptr();

t_sync(fd)
int fd;
{
	struct _ti_user *tiptr;
	void (*sigsave)();
	struct T_info_ack info;
	int retval, arg, retlen;

	/*
	 * Initialize "openfiles" - global variable
  	 * containing number of open files allowed
	 * per process.
	 */
	openfiles = OPENFILES;
	
	if (fd < 0) {
		t_errno = TBADF;
		return(-1);
	}
	/* 
	 * Check for fork/exec case.
	 */

	if (!_ti_user)
		if ((_ti_user = (struct _ti_user *)calloc(1, (unsigned)(openfiles*sizeof(struct _ti_user)))) == NULL) {
			t_errno = TSYSERR;
			return(-1);
		}

	sigsave = sigset(SIGPOLL, SIG_HOLD);
	info.PRIM_type = T_INFO_REQ;
	if ((retval = ioctl(fd, I_FIND, "timod")) < 0) {
		sigset(SIGPOLL, sigsave);
		t_errno = TBADF;
		return(-1);
	}
	if (!retval) {
		sigset(SIGPOLL, sigsave);
		t_errno = TBADF;
		return(-1);
	}
	if (!_t_do_ioctl(fd, (caddr_t)&info, sizeof(struct T_info_req), TI_GETINFO, &retlen) < 0) {
		sigset(SIGPOLL, sigsave);
		return(-1);
	}
	sigset(SIGPOLL, sigsave);
	if (retlen != sizeof(struct T_info_ack)) {
		errno = EIO;
		t_errno = TSYSERR;
		return(-1);
	}
	tiptr = &_ti_user[fd];
	if (!(tiptr->ti_flags & USED)) {
		if (_t_alloc_bufs(fd, tiptr, info) < 0) {
			_null_tiptr(tiptr);
			t_errno = TSYSERR;
			return(-1);
		}
	}
	switch (info.CURRENT_state) {

	case TS_UNBND:
		return(tiptr->ti_state = T_UNBND);
	case TS_IDLE:
		if((retval = ioctl(fd,I_NREAD,&arg)) < 0) {
			/*
			 * We must give some state and T_FAKE
			 * seems appropriate. Dont bother to free
			 * _ti_user as it will come in handy for
			 * subsequent operations.
			 */
			tiptr->ti_state = T_FAKE;
			t_errno = TSYSERR;
			return(-1);
		}
		if(retval == 0 )
			return(tiptr->ti_state = T_IDLE);
		else {
			/*
			 * To handle DISCONNECT indications that
			 * might be at the stream head waiting to be read.
			 */
			return(tiptr->ti_state = T_DATAXFER);
		}
	case TS_WRES_CIND:
		return(tiptr->ti_state = T_INCON);
	case TS_WCON_CREQ:
		return(tiptr->ti_state = T_OUTCON);
	case TS_DATA_XFER:
		return(tiptr->ti_state = T_DATAXFER);
	case TS_WIND_ORDREL:
		return(tiptr->ti_state = T_OUTREL);
	case TS_WREQ_ORDREL:
		if((retval = ioctl(fd,I_NREAD,&arg)) < 0) {
			/*
			 * See comments in TS_IDLE case.
			 */
			tiptr->ti_state = T_FAKE;
			t_errno = TSYSERR;
			return(-1);
		}
		if(retval == 0 )
			return(tiptr->ti_state = T_INREL);
		else {
			/*
			 * To handle T_ORDREL_IND indication that
			 * might be at the stream head waiting to be read.
			 */
			return(tiptr->ti_state = T_DATAXFER);
		}
	default:
		tiptr->ti_state = T_FAKE;
		t_errno = TSTATECHNG;
		return(-1);
	}
}
