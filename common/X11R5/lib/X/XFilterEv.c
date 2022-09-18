/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFilterEv.c	1.2"
/*
 * $XConsortium: XFilterEv.c,v 1.8 91/06/05 09:15:44 rws Exp $
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

#define NEED_EVENTS
#include "Xlibint.h"
#include "Xlcint.h"

#if defined(__STDC__)
#define Const const
#else
#define Const /**/
#endif
extern long Const _Xevent_to_mask[];

/*
 * Look up if there is a specified filter for the event.
 */
Bool
XFilterEvent(ev, window)
    XEvent *ev;
    Window window;
{
    XFilterEventList	p;
    Window		win;
    long		mask;
    Bool		ret;

    if (window)
	win = window;
    else
	win = ev->xany.window;
    if (ev->type >= LASTEvent)
	mask = 0;
    else
	mask = _Xevent_to_mask[ev->type];

    LockDisplay(ev->xany.display);
    for (p = ev->xany.display->im_filters; p != NULL; p = p->next) {
	if (win == p->window) {
	    if ((mask & p->event_mask) ||
		(ev->type >= p->start_type && ev->type <= p->end_type)) {
		ret = (*(p->filter))(ev->xany.display, p->window, ev,
				      p->client_data);
		UnlockDisplay(ev->xany.display);
		return(ret);
	    }
	}
    }
    UnlockDisplay(ev->xany.display);
    return(False);
}
