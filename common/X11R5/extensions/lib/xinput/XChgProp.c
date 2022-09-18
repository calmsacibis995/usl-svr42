/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XChgProp.c	1.1"
/* $XConsortium: XChgProp.c,v 1.5 91/07/23 12:26:35 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XChangeDeviceDontPropagateList - Get the dont_propagate_list for a
 * window.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XChangeDeviceDontPropagateList (dpy, window, count, events, mode)
    register Display 	*dpy;
    Window 		window;
    int 		count;
    XEventClass		*events;
    int 		mode;
    {       
    xChangeDeviceDontPropagateListReq 	*req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(ChangeDeviceDontPropagateList,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_ChangeDeviceDontPropagateList;
    req->window = window;
    req->count = count;
    req->mode = mode;
    req->length += count;

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    count <<= 2;
    Data (dpy, (char *) events, count);

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }

