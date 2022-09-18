/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/file.c	1.24"
#endif

#include <IntrinsicP.h>
#include <Xatom.h>
#include <CoreP.h>
#include <CompositeP.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <FButtons.h>
#include <PopupWindo.h>
#include <Caption.h>
#include <TextField.h>
#include <Notice.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

extern char *		ApplicationName;
extern char *		system_path;
extern char *		device_path;

extern void		QuickDialCB();
extern Widget		AddMenu();
extern Boolean		IsSystemFile();
extern void		BringDownPopup();
extern void		DeleteSystemFile();
extern void		CreateSystemFile();
extern void		PutFlatItems();
extern void		DevicePopupCB();
extern void		GetFlatItems();
extern void		DeleteFlatItems();
extern void		GetContainerItems();
extern void		PutContainerItems();
extern void		DeleteContainerItems();
extern void		UnselectSelect();
extern void		InstallCB();

void Save();
static void Quit();
static void QuitDevice();
static void Exit();
static void SaveOldAndQuit();
static void DiscardOldAndQuit();
static void CancelQuit();

Arg arg[50];

static friend = 1;
static port = 2;

static Items fileItems[] = {
	{Save, NULL, (XA)TRUE},
	{DevicePopupCB, NULL, (XA)TRUE},
	{InstallCB, NULL, (XA)TRUE, NULL, NULL, (XA)&friend},
	{Exit, NULL, (XA)TRUE},
};

static Menus fileButtonMenu = {
	"file",
	fileItems,
	XtNumber (fileItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};

static Items dfileItems[] = {
	{QuickDialCB, NULL, (XA)TRUE},
	{InstallCB, NULL, (XA)TRUE, NULL, NULL, (XA)&port},
	{Exit, NULL, (XA)TRUE},
};

static Menus dfileButtonMenu = {
	"file",
	dfileItems,
	XtNumber (dfileItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};

static Items quitNoticeItems [] = {
	{SaveOldAndQuit, NULL, (XA)TRUE},
	{DiscardOldAndQuit, NULL, (XA)TRUE},
	{CancelQuit, NULL, (XA)TRUE},
};

static Menus quitMenu = {
	"quit",
	quitNoticeItems,
	XtNumber(quitNoticeItems),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

void
CheckForChange()
{
	HostData *flp;
	
	if(sf->numFlatItems == 0)
		return;
	/* If the current item changed then we never saved it */
	/* in our data structure, so save it now. */
	
	flp = sf->flatItems[sf->currentItem].pField;
	if (flp->changed) {
		sf->changesMade = True;
	}
}

Widget
AddFileMenu(wid)
Widget wid;
{
	Boolean sys = IsSystemFile(wid);
	if(sys) {
		fileItems[0].sensitive = sf->update;
		SET_LABEL (fileItems,0,save);
		SET_LABEL (fileItems,1,devices);
		SET_LABEL (fileItems,2,install);
		SET_LABEL (fileItems,3,exit);
		return AddMenu (wid, &fileButtonMenu, False);
	} else {
		SET_LABEL (dfileItems,0,dial);
		SET_LABEL (dfileItems,1,install);
		SET_LABEL (dfileItems,2,exit);
		return AddMenu (wid, &dfileButtonMenu, False);
	}
} /* AddFileMenu */

void
SaveOldAndQuit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Save (wid, 0, 0);
	Quit ();
} /* SaveOldAndQuit */

void
DiscardOldAndQuit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	HostData *flp;

	sf->changesMade = False;
	df->changesMade = False;

	/* Update the current item "changed" flag */
	
	if (sf->currentItem == -1)
		sf->currentItem = 0;
	flp = sf->flatItems[sf->currentItem].pField;
	flp->changed = False;
	Quit ();
} /* DiscardOldAndQuit */

void
CancelQuit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (sf->quitNotice);
} /* CancelQuit */

static void
HandleQuitWarning()
{
	if (sf->quitNotice == (Widget) NULL) {
		Widget	textArea,
			controlArea;
		char	buf[BUFSIZ];

		sprintf(buf, "%s: %s", ApplicationName, GGT(label_quitWarn));
		XtSetArg (arg[0], XtNtitle, buf);
		sf->quitNotice = XtCreatePopupShell(
			"QuitNotice",
			noticeShellWidgetClass,
			sf->toplevel,
			arg, 1
		);

		XtVaGetValues(
			sf->quitNotice,
			XtNtextArea, &textArea,
			XtNcontrolArea, &controlArea,
			(String) 0
		);

		XtVaSetValues (
			textArea,
			XtNstring, GGT(string_itemNotSave),
			XtNborderWidth, 0,
			(String)0
		);

		SET_LABEL (quitNoticeItems,0,save);
		SET_LABEL (quitNoticeItems,1,discard);
		SET_LABEL (quitNoticeItems,2,cancel);
		AddMenu (controlArea, &quitMenu, False);
	}

	XtPopup(sf->quitNotice, XtGrabExclusive);
} /* HandleQuitWarning */

void
WriteOutput(wid, filename)
Widget wid;
char *filename;
{
	Boolean sys = IsSystemFile(wid);
#ifdef debug
	printf ("Write file %s\n", filename);
#endif
	CheckForChange();
	if (sys) {
		PutFlatItems (filename);
		sf->changesMade = False;
	} else {
		PutContainerItems (filename);
		df->changesMade = False;
	}
} /* WriteOutput */

void
Save(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Boolean sys = IsSystemFile(wid);
	if (sys)
		if (sf->filename[0] == '\0')
			WriteOutput (wid, system_path);
		else
			WriteOutput (wid, sf->filename);
	else
		if (df->filename[0] == '\0')
			WriteOutput (wid, device_path);
		else
			WriteOutput (wid, df->filename);
} /* Save */

static void
Exit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Boolean sys = IsSystemFile(wid);
	if (sys)
		Quit ();
	else
		QuitDevice();
} /* Exit */

void
QuitDevice()
{
	if (df->changesMade) {
		HandleQuitWarning ();
	} else {
		if(df->propPopup &&
		   XtIsRealized(df->propPopup) &&
		   GetWMState(XtDisplay(df->propPopup),
			      XtWindow(df->propPopup)) != WithdrawnState)
			XtPopdown(df->propPopup);

		if(df->QDPopup &&
		   XtIsRealized(df->QDPopup) &&
		   GetWMState(XtDisplay(df->QDPopup),
			      XtWindow(df->QDPopup)) != WithdrawnState)
			XtPopdown(df->QDPopup);

		if(GetWMState(XtDisplay(df->toplevel),
		   XtWindow(df->toplevel)) != IconicState)
			XtUnmapWidget (df->toplevel);
		else
			XWithdrawWindow(XtDisplay(df->toplevel),
					XtWindow(df->toplevel),
					XScreenNumberOfScreen(XtScreen(df->toplevel)));
		/*DeleteContainerItems ();*/
	}
	XSync (DISPLAY, False);
} /* QuitDevice */

static void
Quit()
{
	CheckForChange();
	if (sf->changesMade||df->changesMade) {
		HandleQuitWarning ();
	} else {
		/* system file */
		XtDestroyWidget (sf->toplevel);
		XtFree((char *)sf->popupMenuItems);
		if (sf->filename != NULL) {
			XtFree (sf->filename);
		}
		DeleteFlatItems ();
		XtFree ((char *)sf->flatItems);
		XtFree ((char *)sf);
		if (df->toplevel) {
			/* device file */
			XtDestroyWidget (df->toplevel);
			DeleteContainerItems ();
			XtFree((char *)df->popupMenuItems->label);
			XtFree((char *)df->popupMenuItems);
			if (df->filename != NULL) {
				XtFree (df->filename);
			}
		}
		XtFree ((char *)df);
		XSync (DISPLAY, False);
		exit (0);
	}
} /* Quit */

void
WindowManagerEventHandler(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Boolean sys = IsSystemFile(wid);
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
	case OL_WM_DELETE_WINDOW:
#ifdef debug
		fprintf (stdout, "Delete yourself\n");
#endif
		if (sys)
			Quit ();
		else
			QuitDevice ();
		break;

	case OL_WM_SAVE_YOURSELF:
		/*
		 *	Do nothing for now; just respond.
		 */
#ifdef debug
		fprintf (stdout, "Save yourself\n");
#endif
		if (sys)
			Quit ();
		else
			QuitDevice ();
		break;

	default:
#ifdef debug
		fprintf (stdout, "Default action\n");
#endif
		OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
		break;
	}
} /* WindowManagerEventHandler */
