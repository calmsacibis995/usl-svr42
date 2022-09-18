/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/file.c	1.19"
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
#include "inet.h"
#include "error.h"

extern char *		ApplicationName;
extern Widget		AddMenu();
extern void		PutFlatItems();
extern void		DeleteFlatItems();
extern void		Save();
extern void		SaveOld();
extern void		DiscardOld();
extern void		AppendOrReplaceList();
extern void		InstallCB();

static int		Quit();
static void		Exit();
static void		SaveOldAndQuit();
static void		DiscardOldAndQuit();
static void		CancelQuit();

Arg arg[50];

static Items fileItems[] = {
	{Save, NULL, (XA)TRUE},
	{InstallCB, NULL, (XA)TRUE},
	{Exit, NULL, (XA)TRUE},
};

static Menu fileButtonMenu = {
	"file",
	fileItems,
	XtNumber (fileItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};

static Items quitNoticeItems [] = {
	{SaveOldAndQuit, NULL, (XA)TRUE},
	{DiscardOldAndQuit, NULL, (XA)TRUE},
	{CancelQuit, NULL, (XA)TRUE},
};

static Menu quitMenu = {
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
	
	if(hf->numFlatItems == 0)
		return;
	/* If the current item changed then we never saved it */
	/* in our data structure, so save it now. */
	
	flp = hf->flatItems[hf->currentItem].pField;
	if (flp->changed) {
		hf->changesMade = True;
	}
}

Widget
AddFileMenu(wid)
Widget wid;
{
	fileItems[0].sensitive = hf->update;
	SET_LABEL (fileItems,0,saveList);
	SET_LABEL (fileItems,1,install);
	SET_LABEL (fileItems,2,exit);
	return AddMenu (wid, &fileButtonMenu, False);
} /* AddFileMenu */

void
SaveOld(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (hf->appendPopup);
	Save (wid, 0, 0);
	AppendOrReplaceList(APPEND);
} /* SaveOld */

static void
SaveOldAndQuit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Save (wid, 0, 0);
	Quit ();
} /* SaveOldAndQuit */

void
DiscardOld(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (hf->appendPopup);
	hf->changesMade = False;
	AppendOrReplaceList(APPEND);
} /* DiscardOld */

static void
DiscardOldAndQuit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	hf->changesMade = False;
	Quit ();
} /* DiscardOldAndQuit */

static void
CancelQuit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (hf->quitNotice);
} /* CancelQuit */

static void
HandleQuitWarning()
{
	if (hf->quitNotice == (Widget) NULL) {
		Widget	textArea,
			controlArea;
		char	buf[BUFSIZ];

		sprintf(buf, "%s: %s", ApplicationName, GGT(label_quitWarn));
		XtSetArg (arg[0], XtNtitle, buf);
		hf->quitNotice = XtCreatePopupShell(
			"QuitNotice",
			noticeShellWidgetClass,
			hf->toplevel,
			arg, 1
		);

		XtVaGetValues(
			hf->quitNotice,
			XtNtextArea, &textArea,
			XtNcontrolArea, &controlArea,
			(String) 0
		);

		XtVaSetValues (
			textArea,
			XtNstring, GGT(string_itemNotSave ),
			XtNborderWidth, 0,
			(String)0
		);

		SET_LABEL (quitNoticeItems,0,save);
		SET_LABEL (quitNoticeItems,1,discard);
		SET_LABEL (quitNoticeItems,2,cancel);
		AddMenu (controlArea, &quitMenu, False);
	}

	XtPopup(hf->quitNotice, XtGrabExclusive);
} /* HandleQuitWarning */

static void
WriteOutput(wid, filename)
Widget wid;
char *filename;
{
#ifdef debug
	printf ("Write file %s\n", filename);
#endif
	CheckForChange();
	if(hf->changesMade)
		PutFlatItems (filename);
	hf->changesMade = False;
} /* WriteOutput */

 void
Save(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	if (hf->filename[0] == '\0')
		WriteOutput (wid, system_path);
	else
		WriteOutput (wid, hf->filename);
} /* Save */

static void
Exit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Quit ();
} /* Exit */

static
Quit()
{
	CheckForChange();
	if (hf->changesMade) {
		HandleQuitWarning ();
	} else {
		/* system file */
		XtDestroyWidget (hf->toplevel);
		XtFree((char *)hf->popupMenuItems);
		if (hf->filename != NULL) {
			XtFree (hf->filename);
		}
		DeleteFlatItems ();
		XtFree ((char *)hf->flatItems);
		XtFree ((char *)hf);
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
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
	case OL_WM_DELETE_WINDOW:
#ifdef debug
		fprintf (stdout, "Delete yourself\n");
#endif
		Quit ();
		break;

	case OL_WM_SAVE_YOURSELF:
		/*
		 *	Do nothing for now; just respond.
		 */
#ifdef debug
		fprintf (stdout, "Save yourself\n");
#endif
		Quit ();
		break;

	default:
#ifdef debug
		fprintf (stdout, "Default action\n");
#endif
		OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
		break;
	}
} /* WindowManagerEventHandler */
