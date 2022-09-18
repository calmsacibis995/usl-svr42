/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGetProp.c	1.1"
/* $XConsortium: XGetProp.c,v 1.5 90/05/18 11:23:31 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGetDeviceDontPropagateList - Get the dont_propagate_list for a
 * window.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

XEventClass
*XGetDeviceDontPropagateList (dpy, window, count)
    register Display 	*dpy;
    Window 		window;
    int 		*count;
    {       
    XEventClass		*list;
    int			rlen;
    xGetDeviceDontPropagateListReq 	*req;
    xGetDeviceDontPropagateListReply 	rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return ((XEventClass *) NoSuchExtension);

    GetReq(GetDeviceDontPropagateList,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetDeviceDontPropagateList;
    req->window = window;

    if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) 
	{
	UnlockDisplay(dpy);
	SyncHandle();
	return (XEventClass *) NULL;
	}
    *count = rep.count;

    rlen = rep.length << 2;
    list = (XEventClass *) Xmalloc (rlen);
    if (list)
	_XRead (dpy, list, rlen);
    else
	_XEatData (dpy, (unsigned long) rlen);

    UnlockDisplay(dpy);
    SyncHandle();
    return (list);
    }

