/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:readprop.c	1.27"
#endif

#define READPROP_C

#include <ctype.h>
#include <IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <CoreP.h>
#include "mail.h"
#include "RMailGizmo.h"
#include <Gizmo/SpaceGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/ChoiceGizm.h>

extern HelpInfo		ReaderPropertiesHelp;
extern BaseWindowGizmo	MainWindow;
extern ReadRec *	readRec;
extern MailRec *	mailRec;
extern char *		ApplicationName;
extern char *		BriefKeywords;
extern char *		BriefKeywordsToIgnore;

ListHead	IgnoreList;

char *Headings[] = {
	"Cc",
	"Date",
	"From",
	"Subject",
	"To",
};

typedef enum {PropSet, PropReset, PropCancel, PropHelp} PropMenuItemIndex;

HeaderSettings ShowHeader;

typedef struct _ReadSetting {
	Setting header;
} ReadSetting;

ReadSetting readSettings = {
	{"Header",	NULL,	NULL,	(XtPointer)0	}
};

static MenuItems headerItems[] = {
	{True, BUT_BRIEF,	MNEM_BRIEF},
	{True, BUT_FULL,	MNEM_FULL},
	{NULL}
};

static MenuGizmo headerMenu = {
	NULL, "header", NULL, headerItems, 0, NULL, EXC,
	OL_FIXEDROWS, 1, OL_NO_ITEM
};

ChoiceGizmo Header = {
	NULL, BRIEFHEADER, TXT_HEADER_COLON, &headerMenu, &readSettings.header
};

static SpaceGizmo space = {
	(Dimension)5, (Dimension)10
};

Setting HeaderListSetting = {
	"Header",
	(XtPointer)NULL,
	(XtPointer)NULL,
	(XtPointer)"x"
};

/* Define the header scrolling list in the read property */

ListGizmo BriefList = {
	NULL, HEADER_LIST, "", (Setting *)&HeaderListSetting,
	"%s", False, 5, NULL,
	&IgnoreList
};

static GizmoRec listArray[] = {
	{ListGizmoClass,	&BriefList},
};

static LabelGizmo listContainer = {
	NULL, "", TXT_BRIEF_HEADER, listArray, XtNumber (listArray),
	OL_FIXEDROWS, 1, 0, 0, True
};

static GizmoRec PropReadGiz[] = {
	{ChoiceGizmoClass,	&Header},
	{SpaceGizmoClass,	&space},
	{LabelGizmoClass,	&listContainer},
};

/* Define the apply/reset/reset to factory menu */
static MenuItems readApplyItems[] = {
	{True, BUT_APPLY,	MNEM_APPLY},
	{True, BUT_RESET,	MNEM_RESET},
	{True, BUT_CANCEL,	MNEM_CANCEL},
	{True, BUT_HELP,	MNEM_HELP,	 NULL, HelpCB, (XtPointer)HelpReaderProperties},
	{NULL}
};

static void PropertyCB();

static MenuGizmo readApply = {
	NULL,			/* Help		*/
	NULL,			/* Name		*/
	"readApply",		/* Title	*/
	readApplyItems,		/* Items	*/
	PropertyCB,		/* Function	*/
	NULL,			/* Client data	*/
	CMD,			/* Button type	*/
	OL_FIXEDROWS,		/* Layout type	*/
	1,			/* Measure	*/
	0			/* Default item	*/
};

/* Define the read property popup */

static PopupGizmo *	propReadPopup = (PopupGizmo *)0;

PopupGizmo	PropReadPopup = {
	&ReaderPropertiesHelp,
	"propRead",
	TXT_PROP_READER,
	&readApply,
	PropReadGiz,
	XtNumber (PropReadGiz),
};

/* Create the read property sheet popup */

void
ReadPropPopupCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	MailRec *mp = FindMailRec (wid);

	/* Get the list of headings to be ignored */

	if (propReadPopup == (PopupGizmo *)0) {
		propReadPopup = &PropReadPopup;
		CreateGizmo (
			GetBaseWindowShell (&MainWindow),
			PopupGizmoClass,
			propReadPopup,
			NULL, 0
		);
		/* Set the correct button on the Header menu */
		OlVaFlatSetValues (
			(Widget)QueryGizmo (
				PopupGizmoClass, propReadPopup,
				GetGizmoWidget, BRIEFHEADER
			),
			ShowHeader == Full ? 1 : 0,
			XtNset,	True,
			(String)0
		);
	}
	MapGizmo (PopupGizmoClass, propReadPopup);
}

/* Apply the given command to all open mailx's */

void
ApplyToEachMailx (cmd)
PFV	cmd;
{
	MailRec *	mp;
	for (mp = mailRec; mp != NULL; mp = mp-> next) {
		(*cmd) (mp);
	}
}

static void
ApplyToAllReadRecs (cmd)
PFV		cmd;
{
	ReadRec *	rp;

	/* Loop thru all the read shells and execute the command */

	for (rp=readRec; rp!=(ReadRec *)0; rp=rp->next) {
		(*cmd) (rp);
	}
}

/* Get the list of headers that are to be ignored.  This list comes
 * from querying the .mailrc file.  If zzzignore and zzzunignore are
 * set use the values in these lists.  Otherwise, use default values
 * and values that come from the ignore command.
 */

static void
InitBriefListFromDefaults (mp)
MailRec *	mp;
{
	char *		buf;
	char **		tmp;
	char *		cp;
	char *		string;
	char *		savept = NULL;
	int		i;
	ListHead *	hp;

	hp = (ListHead *)&IgnoreList;
	hp->numFields = 1;

	/* Read in the current ignore list set by .mailrc */
	buf = ProcessCommand (mp, IGNORE, NULL);

	hp->size = XtNumber (Headings);

	/* Alloc the space for the items */

	hp->items = (ListItem *)MALLOC(
		sizeof(ListItem)*hp->size
	);

	/* Add the default items to the list. */

	for (i=0; i<hp->size; i++) {
		hp->items[i].set = True;	/* Set current value */
		hp->items[i].clientData = True;	/* Set previous value */
		hp->items[i].fields = (XtArgVal)MALLOC (
			sizeof (XtArgVal *) * 1
		);
		tmp = (char **)hp->items[i].fields;
		tmp[0] = STRDUP (Headings[i]);
	}

	if (strcmp (buf, NTS_IGNORE_TEXT) == 0) {
		return;
	}

	cp = buf;
	while ((string = MyStrtok (cp, "\n", &savept)) != NULL) {
	    cp = NULL;
	    (void)InsertIntoList (mp, string, hp, True);
	}
}

static void
InitBriefListFromDT (mp)
MailRec *	mp;
{
	char *		buf;
	char **		tmp;
	char *		cp;
	char *		string;
	char *		savept = NULL;
	int		i;
	ListHead *	hp;

	hp = (ListHead *)&IgnoreList;
	hp->numFields = 1;
	hp->size = 0;

	hp->items = (ListItem *)MALLOC(
		sizeof(ListItem)*1
	);

	/*
	 * Insert words obtained from previous zzzunignore setting
	 */
	buf = BriefKeywords;
	/* Words in the list are separated by spaces.
	   Put these words into the list.
	 */
	while ((cp = MyStrtok (buf, " ", &savept)) != NULL) {
		buf = NULL;
		(void)InsertIntoList (mp, cp, hp, True);
	}
	for (i=0; i<hp->size; i++) {
		hp->items[i].set = True;	/* Set current value */
		hp->items[i].clientData = True;	/* Set previous value */
	}

	/* Now, insert those items to be ignored. */
	/* Obtained from zzzignore setting */
	buf = BriefKeywordsToIgnore;
	/* Put these words into the list.  */
	savept = NULL;
	while ((cp = MyStrtok (buf, " ", &savept)) != NULL) {
		buf = NULL;
		(void)InsertIntoList (mp, cp, hp, True);
	}
}

void
InitBriefList (mp)
MailRec *	mp;
{
	if (BriefKeywordsToIgnore != NULL || BriefKeywords != NULL) {
		InitBriefListFromDT (mp);
	}
	else {
		InitBriefListFromDefaults (mp);
	}
}

static void
ReRead (rp)
ReadRec *	rp;
{
	GetCurrentItem (rp->mp, rp);
}

static void
PropertyCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *		mp = FindMailRec (wid);
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	Widget			shell;
	Setting *		setting;

	shell = GetPopupGizmoShell(propReadPopup);

	switch (p->item_index) {
	    case PropSet:
		ManipulateGizmo (
			(GizmoClass)&PopupGizmoClass,
			propReadPopup,
			GetGizmoValue
		);
		  
		ManipulateGizmo (
			(GizmoClass)&PopupGizmoClass,
			propReadPopup,
			ApplyGizmoValue
		);
		setting = (Setting *)QueryGizmo (
			PopupGizmoClass, propReadPopup,
			GetGizmoSetting, BRIEFHEADER
		);
		ShowHeader = setting->current_value;
		BringDownPopup(shell);
		ApplyToAllReadRecs (ReRead);
		break;
	case PropReset:
		ManipulateGizmo (
			(GizmoClass)&PopupGizmoClass,
			propReadPopup,
			ResetGizmoValue
		);
		break;
	case PropCancel:
		SetWMPushpinState (
			XtDisplay(shell),
			XtWindow(shell),
			WMPushpinIsOut
		);
		XtPopdown(shell);
		break;
	case PropHelp:
		break;
	default:
		break;
    }
}
