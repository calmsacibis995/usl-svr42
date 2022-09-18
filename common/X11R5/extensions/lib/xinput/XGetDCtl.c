/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGetDCtl.c	1.1"
/* $Consortium: XGetDCtl.c,v 1.2 90/11/13 13:16:52 gms Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGetDeviceControl - get the Device control state of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "Xlib.h"
#include "XInput.h"
#include "extutil.h"

XDeviceControl
*XGetDeviceControl (dpy, dev, control)
    register	Display 	*dpy;
    XDevice			*dev;
    int				control;
    {
    int	size = 0;
    int	nbytes, i;
    XDeviceControl *Device = NULL;
    XDeviceControl *Sav = NULL;
    xDeviceState *d = NULL;
    xDeviceState *sav = NULL;
    xGetDeviceControlReq *req;
    xGetDeviceControlReply rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Add_XChangeDeviceControl) == -1)
	return ((XDeviceControl *) NoSuchExtension);

    GetReq(GetDeviceControl,req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetDeviceControl;
    req->deviceid = dev->device_id;
    req->control = control;

    if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) 
	{
	UnlockDisplay(dpy);
	SyncHandle();
	return (XDeviceControl *) NULL;
	}
    if (rep.length > 0) 
	{
	nbytes = (long)rep.length << 2;
	d = (xDeviceState *) Xmalloc((unsigned) nbytes);
        if (!d)
	    {
	    _XEatData (dpy, (unsigned long) nbytes);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (XDeviceControl *) NULL;
	    }
	sav = d;
	_XRead (dpy, (char *) d, nbytes);

	switch (d->control)
	    {
	    case DEVICE_RESOLUTION:
		{
		xDeviceResolutionState *r;

		r = (xDeviceResolutionState *) d;
		size += sizeof (XDeviceResolutionState) + 
			(3 * sizeof(int) * r->num_valuators);
		break;
		}
	    default:
		size += d->length;
		break;
	    }

	Device = (XDeviceControl *) Xmalloc((unsigned) size);
        if (!Device)
	    {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (XDeviceControl *) NULL;
	    }
	Sav = Device;

	d = sav;
	switch (control)
	    {
	    case DEVICE_RESOLUTION:
		{
		int *iptr, *iptr2;
		xDeviceResolutionState *r;
		XDeviceResolutionState *R;
		r = (xDeviceResolutionState *) d;
		R = (XDeviceResolutionState *) Device;

		R->control = DEVICE_RESOLUTION;
		R->length = sizeof (XDeviceResolutionState);
		R->num_valuators = r->num_valuators;
		iptr = (int *) (R+1);
		iptr2 = (int *) (r+1);
		R->resolutions = iptr;
		R->min_resolutions = iptr + R->num_valuators;
		R->max_resolutions = iptr + (2 * R->num_valuators);
		for (i=0; i < (3 * R->num_valuators); i++)
		    *iptr++ = *iptr2++;
		break;
		}
	    default:
		break;
	    }
	XFree (sav);
	}

    UnlockDisplay(dpy);
    SyncHandle();
    return (Sav);
    }

XFreeDeviceControl (control)
    XDeviceControl *control;
    {
    XFree (control);
    }
