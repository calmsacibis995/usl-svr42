/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGetVers.c	1.1"
/* $XConsortium: XGetVers.c,v 1.7 91/02/09 17:50:57 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGetExtensionVersion - Get the version of the input extension.
 *
 */

#include "XIproto.h"
#include "Xlibint.h"
#include "XI.h"
#include "XInput.h"
#include "extutil.h"

XExtensionVersion
*XGetExtensionVersion (dpy, name)
    register Display 	*dpy;
    char		*name;
    {       
    xGetExtensionVersionReq 	*req;
    xGetExtensionVersionReply 	rep;
    XExtensionVersion		*ext;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, Dont_Check) == -1)
	return ((XExtensionVersion *) NoSuchExtension);

    GetReq(GetExtensionVersion,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetExtensionVersion;
    req->nbytes = name ? strlen(name) : 0;
    req->length += (unsigned)(req->nbytes+3)>>2;
    _XSend(dpy, name, (long)req->nbytes);

    if (! _XReply (dpy, (xReply *) &rep, 0, xTrue)) 
	{
	UnlockDisplay(dpy);
	SyncHandle();
	return (XExtensionVersion *) NULL;
	}
    ext = (XExtensionVersion *) Xmalloc (sizeof (XExtensionVersion));
    if (ext)
	{
	ext->present = rep.present;
	if (ext->present)
	    {
	    ext->major_version = rep.major_version;
	    ext->minor_version = rep.minor_version;
	    }
	}
    UnlockDisplay(dpy);
    SyncHandle();
    return (ext);
    }

