/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XChProp.c	1.2"

#include "Xlibint.h"

#if NeedFunctionPrototypes
XChangeProperty (
    register Display *dpy,
    Window w,
    Atom property,
    Atom type,
    int format,  /* 8, 16, or 32 */
    int mode,  /* PropModeReplace, PropModePrepend, PropModeAppend */
    _Xconst unsigned char *data,
    int nelements)
#else
XChangeProperty (dpy, w, property, type, format, mode, data, nelements)
    register Display *dpy;
    Window w;
    Atom property, type;
    int format;  /* 8, 16, or 32 */
    int mode;  /* PropModeReplace, PropModePrepend, PropModeAppend */
    unsigned char *data;
    int nelements;
#endif
    {
    register xChangePropertyReq *req;
    register long len;

    LockDisplay(dpy);
    GetReq (ChangeProperty, req);
    req->window = w;
    req->property = property;
    req->type = type;
    req->mode = mode;
    if (nelements < 0) {
	req->nUnits = 0;
	req->format = 0; /* ask for garbage, get garbage */
    } else {
	req->nUnits = nelements;
	req->format = format;
    }

    switch (req->format) {
      case 8:
	len = req->length + (((long)nelements + 3)>>2);
	if (len <= 65535) {
	    req->length = len;
	    Data (dpy, (char *)data, nelements);
	} /* else force BadLength */
        break;
 
      case 16:
	len = req->length + (((long)nelements + 1)>>1);
	if (len <= 65535) {
	    req->length = len;
	    len = (long)nelements << 1;
	    Data16 (dpy, (short *) data, len);
	} /* else force BadLength */
	break;

      case 32:
	len = req->length + (long)nelements;
	if (len <= 65535) {
	    req->length = len;
	    len = (long)nelements << 2;
	    Data32 (dpy, (long *) data, len);
	} /* else force BadLength */
	break;

      default:
        /* BadValue will be generated */ ;
      }

    UnlockDisplay(dpy);
    SyncHandle();
    }





