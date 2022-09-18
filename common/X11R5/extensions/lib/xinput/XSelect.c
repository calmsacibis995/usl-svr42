/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XSelect.c	1.1"
/* $XConsortium: XSelect.c,v 1.4 89/12/06 20:38:49 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XSelectExtensionEvent - Select input from an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "Xproto.h"
#include "XInput.h"
#include "extutil.h"

int
XSelectExtensionEvent (dpy, w, event_list, count)
    register 	Display *dpy;
    Window 	w;
    XEventClass	*event_list;
    int		count;
    {
    register 		xSelectExtensionEventReq *req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy,XInput_Initial_Release) == -1)
	return (NoSuchExtension);
    GetReq(SelectExtensionEvent,req);		

    req->reqType = info->codes->major_opcode;
    req->ReqType = X_SelectExtensionEvent;
    req->window = w;
    req->count = count;
    req->length += count;

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    count <<= 2;
    Data (dpy, (char *) event_list, count);

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }
