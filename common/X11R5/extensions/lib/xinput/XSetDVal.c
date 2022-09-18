/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XSetDVal.c	1.1"
/* $XConsortium: XSetDVal.c,v 1.1 91/02/22 15:26:57 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XSetDeviceValuators - Set the value of valuators on an extension input
 * device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XSetDeviceValuators (dpy, dev, valuators, first_valuator, num_valuators)
    register Display 	*dpy;
    XDevice 		*dev;
    int			*valuators;
    int			first_valuator;
    int			num_valuators;
    {       
    xSetDeviceValuatorsReq 		*req;
    xSetDeviceValuatorsReply 	rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Add_XSetDeviceValuators) == -1)
	return (NoSuchExtension);

    GetReq(SetDeviceValuators,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_SetDeviceValuators;
    req->deviceid = dev->device_id;
    req->first_valuator = first_valuator;
    req->num_valuators = num_valuators;
    req->length += num_valuators;

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    num_valuators <<= 2;
    Data (dpy, (char *) valuators, num_valuators);

    (void) _XReply (dpy, (xReply *) &rep, 0, xTrue);
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.status);
    }

