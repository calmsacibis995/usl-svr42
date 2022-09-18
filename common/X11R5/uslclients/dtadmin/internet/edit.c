/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/edit.c	1.21"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <FButtons.h>
#include <FList.h>
#include <PopupWindo.h>
#include <Notice.h>
#include "inet.h"
#include "error.h"

extern char *		ApplicationName;

extern Widget		AddMenu();
extern void		ExpandHostsCB();
extern void		SetLocalFields();
extern void		SetFields();
extern void		UnselectSelect();
extern void		PropPopupCB();
extern void		CreateBlankEntry();

#ifdef debug
extern void		PrintFlatItems();
#endif

void			ApplyNewEntry();
void			AddEntryCB();
void			VerifyDeleteCB();
void			CancelCB();
void			DeleteEntryCB();

Arg arg[50];

static Items sys_editItems[] = {
	{AddEntryCB, NULL, (XA)TRUE},	/* New    */
	{VerifyDeleteCB, NULL, (XA)TRUE},	/* Delete */
	{ExpandHostsCB, NULL, (XA)TRUE},	/* Expand */
	{PropPopupCB, NULL, (XA)TRUE},	/* Prop   */
};

static Menu sys_editMenu = {
	"edit",
	sys_editItems,
	XtNumber (sys_editItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};

static Items cancelItems[] = {
	{DeleteEntryCB, NULL, (XA)TRUE},
	{CancelCB, NULL, (XA)TRUE},
};


static Menu cancelMenu = {
	"cancel",
	cancelItems,
	XtNumber (cancelItems),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

Widget
AddEditMenu(wid)
Widget wid;
{
	SET_LABEL(sys_editItems,0,new);
	SET_LABEL(sys_editItems,1,delete);
	SET_LABEL(sys_editItems,2,expand);
	SET_LABEL(sys_editItems,3,properties);
	return AddMenu (wid, &sys_editMenu, False);
} /* AddEditMenu */

void
AddEntryCB()
{

	/* Create a new entry */
	CreateBlankEntry ();
	PropPopupCB((Widget)0, 0, 0);
} /* AddEntryCB */

static void
CancelCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (hf->cancelNotice);
} /* CancelCB */

void
DeleteEntryCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	HostData *dp;
	register i;

	dp = hf->flatItems[hf->currentItem].pField;

	/* check for the possibility of newly added items */
	if (dp->index != -1) {
		XtFree(hf->Lines[dp->index]);
		hf->Lines[dp->index] = (char *) -1;
	}
	FreeHostData(dp);
	hf->numFlatItems -= 1;
	if (hf->numFlatItems) {
		for (i=hf->currentItem; i<hf->numFlatItems; i++) {
			hf->flatItems[i].pField = hf->flatItems[i+1].pField;
		}
		/* the very last entry */
		if (hf->currentItem == hf->numFlatItems)
			hf->currentItem -= 1;
	} else { /* hf->numFlatItems == 0 */
		hf->currentItem = -1;
		XtPopdown(hf->propPopup);
	}
	XtVaSetValues (
		hf->scrollingList,
		XtNitems,		hf->flatItems,
		XtNnumItems,		hf->numFlatItems,
		XtNviewHeight,          VIEWHEIGHT,
		XtNitemsTouched,	True,
		(String)0
	);
	if (hf->numFlatItems) {
#ifdef debug
		PrintFlatItems();
#endif
		UnselectSelect ();
	}
	hf->changesMade = True;
	if(hf->cancelNotice)
		BringDownPopup (hf->cancelNotice);
} /* DeleteEntryCB */

static void
VerifyDeleteCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static Widget	textArea,
			controlArea;
	char		warning[BUFSIZ];
	char		buf[BUFSIZ];
	
	if (hf->numFlatItems == 0) { /* nothing to delete */
		return;
	}
	if (hf->cancelNotice == (Widget) NULL) {

		sprintf(buf, "%s: %s", ApplicationName, GGT(title_deleteSys));
		XtSetArg (arg[0], XtNtitle, buf);

		hf->cancelNotice = XtCreatePopupShell(
			"CancelNotice",
			noticeShellWidgetClass,
			hf->toplevel,
			arg,
			1
		);

		XtVaGetValues(
			hf->cancelNotice,
			XtNtextArea, &textArea,
			XtNcontrolArea, &controlArea,
			(String)0
		);

		SET_LABEL(cancelItems,0,delete);
		SET_LABEL(cancelItems,1,cancel);
		AddMenu (controlArea, &cancelMenu, False);
	}
	sprintf (warning, GGT(string_deleteConfirm),
		((HostData*)(hf->flatItems[hf->currentItem].pField))->f_name);
	XtVaSetValues(
		textArea,
		XtNstring, warning,
		XtNborderWidth, 0,
		(String)0
	);
	XtPopup(hf->cancelNotice, XtGrabExclusive);
} /* VerifyDeleteCB */

void
ApplyNewEntry(local_flag)
Boolean local_flag;
{
	register	i;
	HostData	*dp;

	hf->flatItems[hf->numFlatItems] = *new;
	new = (FlatList *) NULL;
	dp = hf->flatItems[hf->numFlatItems].pField;
	/* widgets to hf->flatItems->pField */
	if (local_flag)
		SetLocalFields(dp);
	else {
		SetFields(dp);
	}
	hf->currentItem += 1;
	if (hf->numFlatItems) {
		/* Move all the entries back by one upto the current item */
		for (i=hf->numFlatItems; i>hf->currentItem; i--) {
			hf->flatItems[i].pField = hf->flatItems[i-1].pField;
		}
		hf->flatItems[hf->currentItem].pField = dp;
	}
	hf->numFlatItems += 1;
	XtVaSetValues (
		hf->scrollingList,
		XtNitems,		(hf->numFlatItems)?hf->flatItems:NULL,
		XtNnumItems,		hf->numFlatItems,
		XtNviewHeight,          VIEWHEIGHT,
		XtNitemsTouched,	True,
		(String)0
	);

	XtVaSetValues (
		hf->scrollingList,
		XtNviewItemIndex,	hf->currentItem,
		(String)0
	);

	if (hf->numFlatItems > 0) {
		/* Select the new item */
		OlVaFlatSetValues (
			hf->scrollingList,
			hf->currentItem,
			XtNset,	True,
			0
		);
	}
	hf->changesMade = True;
} /* ApplyNewEntry */
