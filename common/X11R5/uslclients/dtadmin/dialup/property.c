/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/property.c	1.6"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <StaticText.h>
#include "uucp.h"

void
PropPopupCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{

	if (!new && sf->numFlatItems == 0) { /* nothing to operate */
		return;
	}
	/* select the current entry */
	if (!new && sf->numFlatItems)
		ResetFields(sf->flatItems[sf->currentItem].pField);
	else
		ResetFields(new->pField);
	
	ClearLeftFooter(sf->sfooter);
	SetValue(sf->propPopup, XtNfocusWidget, (Widget)sf->w_name);
	XtPopup(sf->propPopup, XtGrabNone);
	XRaiseWindow(DISPLAY, XtWindow(sf->propPopup));
} /* PropPopupCB */
