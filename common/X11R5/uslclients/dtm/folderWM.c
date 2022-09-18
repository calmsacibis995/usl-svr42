/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:folderWM.c	1.6" */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "Dtm.h"
#include "extern.h"


void
FolderWinWMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlWMProtocolVerify *wm_data = (OlWMProtocolVerify *)call_data;

    if (wm_data->msgtype == OL_WM_DELETE_WINDOW)
    {
	DmFolderWinPtr fwp = (DmFolderWinPtr)client_data;

	if (fwp == DESKTOP_TOP_FOLDER(Desktop))
	    DmPromptExit(False);

	else
	    DmCloseFolderWindow(fwp);

    } else
    {
	/* do the default */
	OlWMProtocolAction(w, wm_data, OL_DEFAULTACTION);
    }
}

