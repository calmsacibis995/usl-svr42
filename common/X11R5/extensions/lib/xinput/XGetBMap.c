/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGetBMap.c	1.1"
/* $XConsortium: XGetBMap.c,v 1.4 89/12/06 20:38:18 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGetDeviceButtonMapping - Get the button mapping of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"
#ifdef MIN			/* some systems define this in <sys/param.h> */
#undef MIN
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int
XGetDeviceButtonMapping (dpy, device, map, nmap)
    register 	Display	*dpy;
    XDevice		*device;
    unsigned 	char 	map[];
    unsigned 	int 	nmap; 
    {
    int	status = 0;
    unsigned char mapping[256];				/* known fixed size */
    long nbytes;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    register xGetDeviceButtonMappingReq *req;
    xGetDeviceButtonMappingReply rep;

    LockDisplay(dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);
    GetReq(GetDeviceButtonMapping, req);

    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetDeviceButtonMapping;
    req->deviceid = device->device_id;

    status = _XReply (dpy, (xReply *)&rep, 0, xFalse);
    if (status == 1)
	{
	nbytes = (long)rep.length << 2;
	_XRead (dpy, (char *)mapping, nbytes);

	/* don't return more data than the user asked for. */
	if (rep.nElts) 
	    bcopy ((char *) mapping, (char *) map, MIN ((int)rep.nElts, nmap) );
	status = rep.nElts;
	}
    else
	status = 0;
    UnlockDisplay(dpy);
    SyncHandle();
    return (status);
    }
