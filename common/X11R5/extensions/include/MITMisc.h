/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:include/MITMisc.h	1.1"
/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology


********************************************************/

/* RANDOM CRUFT! THIS HAS NO OFFICIAL X CONSORTIUM BLESSING */

/* $XConsortium: MITMisc.h,v 1.4 91/07/12 10:06:43 rws Exp $ */

#ifndef _XMITMISC_H_
#define _XMITMISC_H_

#include <X11/Xfuncproto.h>

#define X_MITSetBugMode			0
#define X_MITGetBugMode			1

#define MITMiscNumberEvents		0

#define MITMiscNumberErrors		0

#ifndef _MITMISC_SERVER_

_XFUNCPROTOBEGIN

Bool XMITMiscQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_basep */,
    int*		/* error_basep */
#endif
);

Status XMITMiscSetBugMode(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Bool		/* onOff */
#endif
);

Bool XMITMiscGetBugMode(
#if NeedFunctionPrototypes
    Display*		/* dpy */
#endif
);

_XFUNCPROTOEND

#endif

#endif
