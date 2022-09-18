/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XIMProto.c	1.1"
/*
 * $XConsortium: XIMProto.c,v 1.9 91/05/07 19:27:52 rws Exp $
 */

/*
 * Copyright 1990, 1991 by OMRON Corporation
 * Copyright 1991 by the Massachusetts Institute of Technology
 *
 *
 * OMRON AND MIT DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * EVENT SHALL OMRON OR MIT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *
 *	Author:	Seiji Kuwari	OMRON Corporation
 *				kuwa@omron.co.jp
 *				kuwa%omron.co.jp@uunet.uu.net
 */				

#include "Xlibint.h"
#include "Xi18nint.h"
#include "XIMlibint.h"
#include <stdio.h>

#ifndef	lint
static lock_err_fun;
#endif	/* lint */

static void (*_XipIOErrorFunction)();

/*ARGSUSED*/
static void
_XipDefaultIOError(im)
    XipIM im;
{
    (void)fprintf(stderr, "XIM: fatal IO error on X Input Manager\n");
}

static void
_XipIOError(im)
    XipIM im;
{
    LockMutex(&lock_err_fun);
    if (_XipIOErrorFunction)
	(*_XipIOErrorFunction)(im);
    else
	_XipDefaultIOError(im);
    UnlockMutex(&lock_err_fun);
}

void (*_XipSetIOErrorHandler(handler))()
    void (*handler)();
{
    void (*oldhandler)();
    
    LockMutex(&lock_err_fun);
    oldhandler = _XipIOErrorFunction;

    if (!oldhandler) {
	oldhandler = _XipDefaultIOError;
    }
    if (handler != NULL) {
	_XipIOErrorFunction = handler;
    } else {
	_XipIOErrorFunction = _XipDefaultIOError;
    }
    UnlockMutex(&lock_err_fun);
    return oldhandler;
}

static int
_Writen(im, num)
    register XipIM im;
    register int num;
{
    register int	i, ret;

    for (i = 0; i < num;) {
	ret = _Write(im->fd, &im->send_buf[i], num - i);
	if (ret <= 0) {
	    im->fd = -1;
	    _XipIOError(im);
	    return(-1);
	}
	i += ret;
    }
    return(0);
}

int
_XipFlushToIM(im)
    register XipIM im;
{
    if (im->sp == 0) return(0);
    if (_Writen(im, im->sp) < 0) return(-1);
    im->sp = 0;
    return(0);
}

int
_XipWriteToIM(im, p, num)
    register XipIM im;
    register char *p;
    register int num;
{
    if (num <= 0) return(0);
    for(;;) {
	if ((im->sp + num) > SEND_BUF_SZ) {
	    bcopy(p, &im->send_buf[im->sp], SEND_BUF_SZ - im->sp);
	    if (_Writen(im, SEND_BUF_SZ) < 0) return(-1);
	    num -= (SEND_BUF_SZ - im->sp);
	    p += (SEND_BUF_SZ - im->sp);
	    im->sp = 0;
	} else {
	    bcopy(p, &im->send_buf[im->sp], num);
	    im->sp +=num;
	    return(0);
	}
    }
}

int
_XipReadFromIM(im, p, num)
    register XipIM im;
    register char *p;
    register int num;
{
    register char *x = p;

    if (num <= 0) return(0);
    for(;;) {
	if (num > im->rc) {
	    if (im->rc > 0) {
		bcopy(&im->recv_buf[im->rp], x, im->rc);
		x += im->rc;
		num -= im->rc;
		im->rc = 0;
	    }
	    im->rc = _Read(im->fd, im->recv_buf, RECV_BUF_SZ);
	    if (im->rc <= 0) {
		im->rc = 0;
		im->rp = 0;
		im->fd = -1;
		_XipIOError(im);
		return(-1);
	    }
	    im->rp = 0;
	} else {
	    bcopy(&im->recv_buf[im->rp], x, num);
	    im->rc -= num;
	    im->rp += num;
	    return(0);
	}
    }
}

