/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dtamlib/dtamICCCM.c	1.3"
#endif
/*
 *	set up ICCCM facility for handling DELETE_WINDOW and SAVE_YOURSELF
 *	messages (remove temporary files, set up reinvocation command.)
 */
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>

extern	char *	save_command[];
extern	int	save_count;

char	*tmpfiles = NULL;

void
_DtamNoteTmpFile(char *fname)
{
	if (tmpfiles == NULL)
		tmpfiles = strdup(fname);
	else {
		tmpfiles = (char *)realloc(tmpfiles,
					strlen(tmpfiles)+strlen(fname)+2);
		strcat(strcat(tmpfiles," "),fname); 
	}
}

static	void
_DtamDeleteTmpFiles(void)
{
	char	*ptr;

	if (tmpfiles) {
		for (ptr=strtok(tmpfiles," "); ptr; ptr=strtok(NULL," "))
			unlink(ptr);
	}
}

static	void
WindowManagerEventHandler(wid, client_data, call_data)
	Widget wid;
	XtPointer client_data;
	XtPointer call_data;
{
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
	case OL_WM_DELETE_WINDOW:
		_DtamDeleteTmpFiles();
		XtUnmapWidget(wid);
		XCloseDisplay(XtDisplay(wid));
		exit(0);
		break;

	case OL_WM_SAVE_YOURSELF:
		/*
		 *	simply respond with the invocation command for now
		 *	(in more complex cases, program state may need to be
		 *	captured and command options specified to restart.)
		 */

		XSetCommand(XtDisplay(wid), XtWindow(wid), 
				save_command, save_count);
		XFlush(XtDisplay(wid));
		break;

	default:
		OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
		break;
	}
}

_DtamWMProtocols(toplevel)
	Widget	toplevel;
{
	XtAddEventHandler(toplevel, NoEventMask, TRUE,
				WindowManagerEventHandler, (XtPointer) 0);
	XtVaSetValues(toplevel,
			XtNwmProtocolInterested,
				(OL_WM_DELETE_WINDOW | OL_WM_SAVE_YOURSELF),
			NULL);
	OlAddCallback(toplevel, XtNwmProtocol,
			WindowManagerEventHandler, (XtPointer) 0);
}

