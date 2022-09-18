/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XRegstFlt.c	1.1"
/*
 * $XConsortium: XRegstFlt.c,v 1.11 91/06/05 09:17:42 rws Exp $
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
#include "Xlcint.h"

static void
_XFreeIMFilters(display)
    Display *display;
{
    register XFilterEventList fl;

    while (fl = display->im_filters) {
        display->im_filters = fl->next;
        Xfree((char *)fl);
    }
}

/*
 * Register a filter with the filter machinery by event mask.
 */
void
_XRegisterFilterByMask(display, window, event_mask, filter, client_data)
    Display *display;
    Window window;
    unsigned long event_mask;
    Bool (*filter)(
#if NeedNestedPrototypes
		   Display*, Window, XEvent*, XPointer
#endif
		   );
    XPointer client_data;
{
    XFilterEventRec		*rec;

    rec = (XFilterEventList)Xmalloc(sizeof(XFilterEventRec));
    if (!rec)
	return;
    rec->window = window;
    rec->event_mask = event_mask;
    rec->start_type = 0;
    rec->end_type = 0;
    rec->filter = filter;
    rec->client_data = client_data;
    LockDisplay(display);
    rec->next = display->im_filters;
    display->im_filters = rec;
    display->free_funcs->im_filters = _XFreeIMFilters;
    UnlockDisplay(display);
}

/*
 * Register a filter with the filter machinery by type code.
 */
void
_XRegisterFilterByType(display, window, start_type, end_type,
		       filter, client_data)
    Display *display;
    Window window;
    int start_type;
    int end_type;
    Bool (*filter)(
#if NeedNestedPrototypes
		   Display*, Window, XEvent*, XPointer
#endif
		   );
    XPointer client_data;
{
    XFilterEventRec		*rec;

    rec = (XFilterEventList)Xmalloc(sizeof(XFilterEventRec));
    if (!rec)
	return;
    rec->window = window;
    rec->event_mask = 0;
    rec->start_type = start_type;
    rec->end_type = end_type;
    rec->filter = filter;
    rec->client_data = client_data;
    LockDisplay(display);
    rec->next = display->im_filters;
    display->im_filters = rec;
    display->free_funcs->im_filters = _XFreeIMFilters;
    UnlockDisplay(display);
}

void
_XUnregisterFilter(display, window, filter, client_data)
    Display *display;
    Window window;
    Bool (*filter)(
#if NeedNestedPrototypes
		   Display*, Window, XEvent*, XPointer
#endif
		   );
    XPointer client_data;
{
    register XFilterEventList	*prev, fl;

    for (prev = &display->im_filters; fl = *prev; ) {
	if (fl->window == window &&
	    fl->filter == filter && fl->client_data == client_data) {
	    *prev = fl->next;
	    Xfree((char *)fl);
	} else
	    prev = &fl->next;
    }
}
