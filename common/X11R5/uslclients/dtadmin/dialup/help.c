/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/help.c	1.7"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <FButtons.h>
#include <MenuShell.h>
#include <PopupWindo.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

extern char *		ApplicationName;
extern char *		Program;
extern void		DisplayHelp();
extern void		HelpCB();

/* HelpCB
 *
 * Display help.  clientData in the item is a pointer to the HelpText data.
 */
void
HelpCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    DisplayHelp (widget, (HelpText *) client_data);
} /* HelpCB () */

/* DisplayHelp
 *
 * Send a message to dtm to display a help window.  If help is NULL, then
 * ask dtm to display the help desk.
 */
void
DisplayHelp (Widget widget, HelpText *help)
{
    DtRequest			*req;
    static DtDisplayHelpRequest	displayHelpReq;
    Display			*display = XtDisplay(widget);
    Window			win = XtWindow(widget);

    if (help)
    {
	req = (DtRequest *) &displayHelpReq;
	displayHelpReq.rqtype = DT_DISPLAY_HELP;
	displayHelpReq.serial = 0;
	displayHelpReq.version = 1;
	displayHelpReq.client = win;
	displayHelpReq.nodename = NULL;
	displayHelpReq.source_type =
	    help->section ? DT_SECTION_HELP : DT_TOC_HELP;
	displayHelpReq.app_name = Program;
	displayHelpReq.app_title = ApplicationName;
	displayHelpReq.title = help->title;
	displayHelpReq.help_dir = NULL;
	displayHelpReq.file_name = help->file;
	displayHelpReq.sect_tag = help->section;
    }
    else
    {
	req = (DtRequest *) &displayHelpReq;
	displayHelpReq.rqtype = DT_DISPLAY_HELP;
	displayHelpReq.source_type = DT_OPEN_HELPDESK;
	displayHelpReq.serial = 0;
	displayHelpReq.version = 1;
	displayHelpReq.client = win;
	displayHelpReq.nodename = NULL;
    }

    (void)DtEnqueueRequest(XtScreen (widget), _HELP_QUEUE (display),
			   _HELP_QUEUE (display), win, req);
}	/* End of DisplayHelp () */
