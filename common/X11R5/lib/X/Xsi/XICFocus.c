/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XICFocus.c	1.1"
/*
 * $XConsortium: XICFocus.c,v 1.12 91/06/05 09:24:13 rws Exp $
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

/*
 * Require the input manager to focus the focus window attached to the ic
 * argument.
 */
void
_XipSetICFocus(supic)
    XIC supic;
{
    XipIC		ic = (XipIC)supic;
    XipIM		im = ipIMofIC(ic);
    ximICFocusReq	req;
    ximEventReply	reply;

    if (im->fd < 0) {
	return;
    }
    _XRegisterFilterByMask(im->core.display, ic->core.focus_window,
			   KeyPressMask,
			   ic->prototype_filter, (XPointer)ic);
    req.reqType = XIM_SetICFocus;
    req.length = sz_ximICFocusReq;
    req.xic = ic->icid;
    if ((_XipWriteToIM(im, (char *)&req, sz_ximICFocusReq) >= 0) &&
	(_XipFlushToIM(im) >= 0)) {
	for (;;) {
	    if ((_XipReadFromIM(im, (char *)&reply, sz_ximEventReply) < 0) ||
		(reply.state == 0xffff)) {
		return;
	    }
	    if (reply.detail == XIM_CALLBACK) {
		if (_XipCallCallbacks(ic) < 0) {
		    return;
		}
	    } else {
		return;
	    }
	}
    }
}

/*
 * Require the input manager to unfocus the focus window attached to the ic
 * argument.
 */
void
_XipUnsetICFocus(supic)
    XIC supic;
{
    XipIC		ic = (XipIC)supic;
    XipIM		im = ipIMofIC(ic);
    ximICFocusReq	req;
    ximEventReply	reply;

    if (im->fd < 0) {
	return;
    }
    _XUnregisterFilter(im->core.display, ic->core.focus_window,
		       ic->prototype_filter, (XPointer)ic);
    req.reqType = XIM_UnsetICFocus;
    req.length = sz_ximICFocusReq;
    req.xic = ic->icid;
    if ((_XipWriteToIM(im, (char *)&req, sz_ximICFocusReq) >= 0) &&
	(_XipFlushToIM(im) >= 0)) {
	for (;;) {
	    if ((_XipReadFromIM(im, (char *)&reply, sz_ximEventReply) < 0) ||
		(reply.state == 0xffff)) {
		return;
	    }
	    if (reply.detail == XIM_CALLBACK) {
		if (_XipCallCallbacks(ic) < 0) {
		    return;
		}
	    } else {
		return;
	    }
	}
    }
}
