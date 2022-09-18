/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:toolboxWM.c	1.3"
#endif

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "Dtm.h"
#include "extern.h"


void
ToolboxWinWMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlWMProtocolVerify *wm_data = (OlWMProtocolVerify *)call_data;

    if (wm_data->msgtype == OL_WM_DELETE_WINDOW)
    {
	DmToolboxWinPtr twp = (DmToolboxWinPtr)client_data;

	if (twp == DESKTOP_TOOLBOX(Desktop))
	    DmPromptExit();

	else
	    DmCloseToolboxWindow(twp);

    } else
    {
	/* do the default */
	OlWMProtocolAction(w, wm_data, OL_DEFAULTACTION);
    }
}

