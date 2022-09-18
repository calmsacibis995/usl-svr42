/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/edit.c	1.26"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <FButtons.h>
#include <FList.h>
#include <PopupWindo.h>
#include <Notice.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

extern char *		ApplicationName;

extern void		FreeHostData();
extern Boolean		IsSystemFile();
extern void		BringDownPopup();
extern void		SetFields();
extern void		ResetFields();
extern void		AddLineToBuffer();
extern void		DeleteLineFromBuffer();
extern Widget		AddMenu();
extern void		UnselectSelect();
extern void		PropPopupCB();
#ifdef debug
extern void		PrintFlatItems();
#endif
extern void		CreateBlankEntry();
extern void		CBCreate();
extern void		CBProperty();
extern void		CBConfirm();
extern void		CBDelete();

void		AddEntryCB();
void		VerifyDeleteCB();
void		CancelCB();
void		DeleteEntryCB();

Arg arg[50];

static Items sys_editItems[] = {
	{AddEntryCB, NULL, (XA)TRUE}, /* New */
	{VerifyDeleteCB, NULL, (XA)TRUE},/* Delete */
	{PropPopupCB, NULL, (XA)TRUE},   /* Properties */
};

static Menus sys_editMenu = {
	"edit",
	sys_editItems,
	XtNumber (sys_editItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};

static Items dev_editItems[] = {
	{CBCreate, NULL, (XA)TRUE},
	{CBConfirm, NULL, (XA)TRUE},
	{CBProperty, NULL, (XA)TRUE},
};

static Menus dev_editMenu = {
	"edit",
	dev_editItems,
	XtNumber (dev_editItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};

static Items cancelItems[] = {
	{DeleteEntryCB, NULL, (XA)TRUE},
	{CancelCB, NULL, (XA)TRUE},
};

static Menus cancelMenu = {
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
	Boolean sys = IsSystemFile(wid);
	
	if (sys) {
		SET_LABEL(sys_editItems,0,new);
		SET_LABEL(sys_editItems,1,delete);
		SET_LABEL(sys_editItems,2,properties);
		return AddMenu (wid, &sys_editMenu, False);
	} else
		SET_LABEL(dev_editItems,0,new);
		SET_LABEL(dev_editItems,1,delete);
		SET_LABEL(dev_editItems,2,properties);
		return AddMenu (wid, &dev_editMenu, False);
} /* AddEditMenu */

void
AddEntryCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	/* Create a new entry */
	CreateBlankEntry ();
	ResetFields(new->pField);
	/* Reset the cusor to the first text field */
	AcceptFocus(sf->w_name);
	/* clear up the footer msg on the base window */
	CLEARMSG();
	ClearLeftFooter(sf->sfooter);
	SetValue(sf->propPopup, XtNfocusWidget, (Widget)sf->w_name);
	XtPopup(sf->propPopup, XtGrabNone);
} /* AddEntryCB */

void
ApplyNewEntry()
{
	register i;
	HostData *dp;
	LinePtr	lp;
	char 	text[BUFSIZ*4];
	char 	buf[BUFSIZ];

	sf->flatItems[sf->numFlatItems] = *new;
	new = (FlatList *) NULL;
	dp = sf->flatItems[sf->numFlatItems].pField;
	/* widgets to sf->flatItems->pField */
	SetFields(dp);
	sf->currentItem += 1;
	if (sf->numFlatItems) {
		/* Move all the entries back by one upto the current item */
		for (i=sf->numFlatItems; i>sf->currentItem; i--) {
			sf->flatItems[i].pField = sf->flatItems[i-1].pField;
		}
		sf->flatItems[sf->currentItem].pField = dp;
	}
	sf->numFlatItems += 1;
	XtVaSetValues (
		sf->scrollingList,
		XtNitems,		sf->flatItems,
		XtNnumItems,		sf->numFlatItems,
		XtNviewHeight,          VIEWHEIGHT,
		XtNitemsTouched,	True,
		(String)0
	);

	XtVaSetValues (
		sf->scrollingList,
		XtNviewItemIndex,	sf->currentItem,
		(String)0
	);

        if (sf->numFlatItems > 0) {
                /* Select the new item */
                OlVaFlatSetValues (
                        sf->scrollingList,
                        sf->currentItem,
                        XtNset, True,
                        0
                );
        }

	lp = (LinePtr) XtMalloc (sizeof(LineRec));
	if (strcmp(dp->f_type, "ACU") == 0) {
		XtFree(dp->f_expect);
		dp->f_expect = (XtPointer) strdup("\"\" \\r\\d \"\" \\r\\d");
	}
	sprintf (text,
		"%s %s %s %s %s %s %s %s %s %s\n",
		dp->f_name,
		dp->f_time,
		dp->f_type,
		dp->f_class,
		dp->f_phone,
		dp->f_expect,
		dp->f_expect1,
		dp->f_login,
		dp->f_expect2,
		dp->f_passwd);
	lp->text = strdup(text);
	lp->next = NULL;
	if (sf->numFlatItems > 1) {
		AddLineToBuffer(sf->flatItems[sf->currentItem - 1].pField->lp, lp);
	} else {
		AddLineToBuffer(NULL, lp);
	}
	dp->lp = lp;
	sf->changesMade = True;
} /* ApplyNewEntry */

static void
CancelCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (sf->cancelNotice);
} /* CancelCB */

void
DeleteEntryCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	HostData *dp;
	register i;

	dp = sf->flatItems[sf->currentItem].pField;

	FreeHostData (dp);
	sf->numFlatItems -= 1;
	if (sf->numFlatItems) {
		for (i=sf->currentItem; i<sf->numFlatItems; i++) {
			sf->flatItems[i].pField = sf->flatItems[i+1].pField;
		}
		/* the very last entry */
		if (sf->currentItem == sf->numFlatItems)
			sf->currentItem -= 1;
	} else { /* sf->numFlatItems == 0 */
		sf->currentItem = -1;
		XtPopdown(sf->propPopup);
	}
	XtVaSetValues (
		sf->scrollingList,
		XtNitems,		sf->flatItems,
		XtNnumItems,		sf->numFlatItems,
		XtNviewHeight,          VIEWHEIGHT,
		XtNitemsTouched,	True,
		(String)0
	);
	if (sf->numFlatItems) {
#ifdef debug
		PrintFlatItems();
#endif
		UnselectSelect ();
	}
	sf->changesMade = True;
	if(sf->cancelNotice)
		BringDownPopup (sf->cancelNotice);
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
	
	if (sf->numFlatItems == 0) { /* nothing to delete */
		return;
	}
	if (sf->cancelNotice == (Widget) NULL) {

		sprintf(buf, "%s: %s", ApplicationName, GGT(title_deleteSys));
		XtSetArg (arg[0], XtNtitle, buf);
		sf->cancelNotice = XtCreatePopupShell(
			"CancelNotice",
			noticeShellWidgetClass,
			sf->toplevel,
			arg,
			1
		);

		XtVaGetValues(
			sf->cancelNotice,
			XtNtextArea, &textArea,
			XtNcontrolArea, &controlArea,
			(String)0
		);

		SET_LABEL(cancelItems,0,delete);
		SET_LABEL(cancelItems,1,cancel);
		AddMenu (controlArea, &cancelMenu, False);
	}
	sprintf (warning, GGT(string_deleteConfirm),
		((HostData*)(sf->flatItems[sf->currentItem].pField))->f_name);
	XtVaSetValues(
		textArea,
		XtNstring, warning,
		XtNborderWidth, 0,
		(String)0
	);
	XtPopup(sf->cancelNotice, XtGrabExclusive);
} /* VerifyDeleteCB */
