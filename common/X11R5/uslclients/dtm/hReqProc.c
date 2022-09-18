/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:hReqProc.c	1.53"

/******************************file*header********************************

    Description:
     This file contains the source code to handle the following requests:
	- DT_DISPLAY_HELP
	- DT_OL_DISPLAY_HELP
	- DT_ADD_TO_HELPDESK
	- DT_DEL_FROM_HELPDESK.
*/
                              /* #includes go here     */

#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include "X11/StringDefs.h"
#include "X11/Xatom.h"
#include <Xol/OpenLook.h>
#include <Dt/DesktopI.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static int DisplayHelp(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *reply);

static int AddToHelpDesk(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *reply);

static int DelFromHelpDesk(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *reply);

static int DisplayOLHelp(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *reply);

/*
 * This array must match with information in DtHMMsg.h.
 */
static int (*(help_procs[]))() = {
	DisplayHelp,
	AddToHelpDesk,
	DelFromHelpDesk,
	DisplayOLHelp,
};

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
	Sets up functions for handling client requests for on-line help.
 */

void
DmInitHMReqProcs(w)
Widget w;
{
	static DmPropEventData help_prop_ev_data = {
				(Atom)0,
				Dt__help_msgtypes,
				help_procs,
				XtNumber(help_procs)
	};

	/*
	 * Since _HELP_QUEUE is not constant,
	 * it cannot be initialized at compile time.
	 */
	help_prop_ev_data.prop_name = _HELP_QUEUE(XtDisplay(w));

	DmRegisterReqProcs(w, &help_prop_ev_data);
}


/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
	Handles a DT_DISPLAY_HELP client request to either display help
	or to open the Help Desk.
 */
static int
DisplayHelp(scrn, xevent, request, reply)
Screen    *scrn;
XEvent    *xevent;
DtRequest *request;
DtReply   *reply;
{
	DmHelpAppPtr hap;
	Boolean success = True;

#define REQ request->display_help
	/* Simply call DmHelpDeskCB() if source_type is DT_OPEN_HELPDESK */ 
	if (REQ.source_type == DT_OPEN_HELPDESK) {
		DmHelpDeskCB(NULL, NULL, NULL);
		goto bye;
	}

	/* app_name must be specified for all other source_types */
	if (REQ.app_name == NULL)
		goto bye;

	if (hap = DmNewHelpAppID(scrn, request->header.client, REQ.app_name,
		REQ.app_title, request->header.nodename, REQ.help_dir,
		REQ.icon_file)) {

		if (REQ.source_type == DT_SECTION_HELP ||
			REQ.source_type == DT_TOC_HELP) {

			char *hfile;
			struct stat  hstat;

			/* check if help file exists */
			if (*(REQ.file_name) == '/')
				hfile = REQ.file_name;
			else if (REQ.help_dir)
				hfile = Dm__MakePath(REQ.help_dir, REQ.file_name);
			else if (REQ.file_name == NULL || strcmp(REQ.file_name,"") == 0)
				hfile = NULL;
			else
				hfile = XtResolvePathname(DisplayOfScreen(scrn), "help",
					REQ.file_name, NULL, NULL, NULL, 0, NULL);

			if (hfile == NULL || stat(hfile, &hstat) != 0)
				success = False;

		} else if (REQ.source_type == DT_STRING_HELP && ! REQ.string) {
				success = False;
		}

		if (!success) {
			DmDisplayHelpString(&(hap->hlp_win), hap->app_id, NULL,
				Dm__gettxt(TXT_NO_HELP_AVAIL),
				UNSPECIFIED_POS, UNSPECIFIED_POS);
			goto bye;
		}

		switch(REQ.source_type) {
		case DT_STRING_HELP:
			DmDisplayHelpString(&(hap->hlp_win), hap->app_id,
				REQ.title, REQ.string,
				UNSPECIFIED_POS, UNSPECIFIED_POS);
			break;

		case DT_SECTION_HELP:
			DmDisplayHelpSection(&(hap->hlp_win), hap->app_id,
				REQ.title, REQ.file_name, REQ.sect_tag,
				UNSPECIFIED_POS, UNSPECIFIED_POS);
			break;

		case DT_TOC_HELP:
			DmDisplayHelpTOC(NULL, &(hap->hlp_win), REQ.title,
				REQ.file_name, hap->app_id);
			break;

		default:
			Dm__VaPrintMsg(TXT_UNKNOWN_HELP_TYPE, REQ.source_type);
			break;
		}
	} else {
		Dm__VaPrintMsg(TXT_CANT_ALLOC_HAP);
	}

bye:
	/* free data in request structure */
	XtFree(REQ.app_name);
	XtFree(REQ.app_title);
	XtFree(REQ.string);
	XtFree(REQ.file_name);
	XtFree(REQ.sect_tag);
	XtFree(REQ.icon_file);
	return(0);

#undef REQ

} /* end of DisplayHelp */

/****************************procedure*header*****************************
	Handles a DT_ADD_TO_HELPDESK client request.  Adds an application
	to the Help Desk.
 */
static int
AddToHelpDesk(scrn, xevent, request, reply)
Screen    *scrn;
XEvent    *xevent;
DtRequest *request;
DtReply   *reply;
{
#define REQ request->add_to_helpdesk
#define REP reply

	struct stat hstat;
	char *tfile;

	/* REQ.app_name and REQ.help_file must both be specified. */
	if (REQ.app_name == NULL || REQ.help_file == NULL) {
		REP->header.status = DT_FAILED;
		goto bye;
	}

	/*
	 * Check whether REQ.help_file exists. It can be a full or
	 * relative path name. If it is the latter and a help directory
	 * is not specified, then it is expected to be in the standard
	 * search path.
	 */

	if (*(REQ.help_file) == '/')
		tfile = REQ.help_file;
	else if (REQ.help_dir)
		tfile = DmMakePath(REQ.help_dir, REQ.help_file);
	else
		/* should be in standard search path */
		tfile = XtResolvePathname(DisplayOfScreen(scrn), "help",
				REQ.help_file, NULL, NULL, NULL, 0, NULL);
		
	if ((stat(tfile, &hstat)) != 0) {
		REP->header.status = DT_FAILED;
		goto bye;
	}

	if (DmAddAppToHD(REQ.app_name, REQ.icon_label, REQ.icon_file,
		REQ.help_file, REQ.help_dir, tfile) == 0)

		REP->header.status = DT_OK;
	else
		REP->header.status = DT_FAILED;
	 

bye:
	/* free data in request structure */
	XtFree(REQ.app_name);
	XtFree(REQ.icon_label);
	XtFree(REQ.icon_file);
	XtFree(REQ.help_file);
	XtFree(REQ.help_dir);

#undef REQ
#undef REP
	return(1);

} /* end of AddToHelpDesk */

/****************************procedure*header*****************************
	Handles a DT_DEL_FROM_HELPDESK client request.  Removes an application
	from the Help Desk.
 */
static int
DelFromHelpDesk(scrn, xevent, request, reply)
Screen	*scrn;
XEvent	*xevent;
DtRequest	*request;
DtReply	*reply;
{
#define REQ	request->del_from_helpdesk
#define REP	reply

	if (REQ.app_name && DmDelAppFromHD(REQ.app_name, REQ.help_file) == 0)
		REP->header.status = DT_OK;
	else
		REP->header.status = DT_FAILED;

	/* free data in request structure */
	XtFree(REQ.app_name);
	XtFree(REQ.help_file);
	return(1);

#undef REQ
#undef REP

} /* end of DelFromHelpDesk */

/****************************procedure*header*****************************
 * Called when a DT_OL_DISPLAY_HELP request is received from the
 * OPEN LOOK toolkit and OPEN LOOK window manager.  Actually, this
 * is a re-routed request for context-sensitive help. Displays default
 * help if help file is not found and source_type is OL_DISK_SOURCE or
 * OL_DESKTOP_SOURCE. app_name must be specified for all source_types.
 */

static int
DisplayOLHelp(scrn, xevent, request, reply)
Screen    *scrn;
XEvent    *xevent;
DtRequest *request;
DtReply   *reply;
{
#define REQ	request->ol_display_help

	struct stat  hstat;
	DmHelpAppPtr hap;
	char         *hfile;
	Boolean      success = True;

	if (! REQ.app_name)
		goto bye;

	if (hap = DmNewHelpAppID(scrn, request->header.client, REQ.app_name,
	    REQ.app_title, request->header.nodename, REQ.help_dir, NULL)) {

		if (REQ.source_type == OL_DISK_SOURCE ||
			REQ.source_type == OL_DESKTOP_SOURCE) {

			if (*(REQ.file_name) == '/')
    				hfile = REQ.file_name;
			else if (REQ.help_dir)
				hfile = Dm__MakePath(REQ.help_dir, REQ.file_name);
			else if (REQ.file_name == NULL || strcmp(REQ.file_name,"") == 0)
				hfile = NULL;
			else
				hfile = XtResolvePathname(DisplayOfScreen(scrn), "help",
						REQ.file_name, NULL, NULL, NULL, 0, NULL);

			if (hfile == NULL || stat(hfile, &hstat) != 0)
				success = False;

		} else if (REQ.source_type == OL_STRING_SOURCE && ! REQ.string) {
			success = False;
		}

		if (!success) {
			DmDisplayHelpString(&(hap->hlp_win), hap->app_id, NULL,
				Dm__gettxt(TXT_NO_HELP_AVAIL),
				UNSPECIFIED_POS, UNSPECIFIED_POS);
			goto bye;
		}

		switch(REQ.source_type) {
		case OL_STRING_SOURCE:
			DmDisplayHelpString(&(hap->hlp_win), hap->app_id, REQ.title,
				REQ.string, REQ.x, REQ.y);
			XtFree(REQ.string);
			break;

		case OL_DISK_SOURCE:
			DmDisplayHelpSection(&(hap->hlp_win), hap->app_id,
				REQ.title, REQ.file_name, NULL, REQ.x, REQ.y);
			XtFree(REQ.file_name);
			break;

		case OL_DESKTOP_SOURCE:
			DmDisplayHelpSection(&(hap->hlp_win), hap->app_id,
				REQ.title, REQ.file_name, REQ.sect_tag, REQ.x, REQ.y);
			XtFree(REQ.file_name);
			XtFree(REQ.sect_tag);
			break;

		default:
			Dm__VaPrintMsg(TXT_UNKNOWN_HELP_TYPE, REQ.source_type);
			break;
		}
	} else
		Dm__VaPrintMsg(TXT_CANT_ALLOC_HAP);

bye:
	/* free data in request structure */
	XtFree(REQ.app_name);
	XtFree(REQ.app_title);
	XtFree(REQ.title);
	return(0);

#undef REQ
} /* end of DisplayOLHelp  */
