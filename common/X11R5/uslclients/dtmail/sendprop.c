/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:sendprop.c	1.21"
#endif

#define SENDPROP_C

#include <IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <CoreP.h>
#include "mail.h"
#include "TextGizmo.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/SpaceGizmo.h>

RecordType		RecordOutgoing;

extern HelpInfo		SenderPropertiesHelp;
extern char *		Record;
extern BaseWindowGizmo	MainWindow;
extern MenuGizmo	ReadApply;
extern char *		Signature;

#define BLANK		"                                   "
#define RECORD		"Record"

typedef enum {
	PropSet, PropReset, PropCancel, PropHelp
} PropMenuItemIndex;

typedef struct _SendSettings {
	Setting record;
	Setting signature;
} SendSettings;

static SendSettings inputSettings = {
	{"record",	NULL, NULL, (XtPointer)0	},
	{"signature",	NULL, NULL, (XtPointer)BLANK	}
};

static MenuItems recordItems[] = {
	{True, BUT_ON,	MNEM_ON,   "record"},
	{True, BUT_OFF,	MNEM_OFF,  "norecord"},
	{NULL}
};

static MenuGizmo recordMenu = {
	NULL, "record", NULL, recordItems, 0, NULL, EXC,
	OL_FIXEDROWS, 1, OL_NO_ITEM
};

static ChoiceGizmo record = {
	NULL,
	RECORD,
	TXT_RECORD_OUTGOING,
	&recordMenu,
	&inputSettings.record,
};

static TextGizmo signBox = {
	"", 4, 35
};

static GizmoRec signArray[] = {
	{TextGizmoClass,	&signBox}
};

static LabelGizmo signHere = {
	NULL, "sign", TXT_SIGNATURE,
	signArray, XtNumber (signArray),
	OL_FIXEDROWS, 1,
	0, 0,
	False
};

static GizmoRec PropSendGiz[] = {
	{ChoiceGizmoClass,	&record},
	{LabelGizmoClass,	&signHere},
};

/* Define the Apply menu */

static MenuItems compApplyItems[] = {
	{True,	BUT_APPLY,	MNEM_APPLY},
	{True,	BUT_RESET,	MNEM_RESET},
	{True,	BUT_CANCEL,	MNEM_CANCEL},
	{True,	BUT_HELP,	MNEM_HELP,NULL,HelpCB,(XtPointer)HelpSenderProperties},
	{NULL}
};

static void PropertyCB();

static MenuGizmo compApply = {
	NULL, "", NULL, compApplyItems, PropertyCB,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

/* Define the send property popup */

static PopupGizmo *	propSendPopup = (PopupGizmo *)0;

static PopupGizmo	PropSendPopup = {
	&SenderPropertiesHelp,
	"mailProp",
	TXT_MAIL_PROPERTIES,
	&compApply,
	PropSendGiz,
	XtNumber (PropSendGiz),
};

/* Create the send window property sheet popup */

void
SendPropPopupCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	SendRec *sp = FindSendRec (wid);
	Widget	shell;
	Widget	control;

	/* Set the appropriate state of the Record menu */
	if (RecordOutgoing == DoIt) {
		inputSettings.record.previous_value = (XtPointer)0;
	}
	else {
		inputSettings.record.previous_value = (XtPointer)1;
	}
	/* Set the signature if there is one from mailrc */
	if (
		Signature != NULL &&
		Signature[0] != '\0'
	) {
		signBox.source = Signature;
	}
	if (propSendPopup == (PopupGizmo *)0) {
		propSendPopup = &PropSendPopup;
		shell = CreateGizmo (
			GetBaseWindowShell (&MainWindow),
			PopupGizmoClass,
			propSendPopup,
			NULL, 0
		);
		XtVaGetValues (
			shell, XtNupperControlArea, &control, (String)0
		);
		XtVaSetValues (
			control, XtNalignCaptions, False, (String)0
		);
	}

	MapGizmo (PopupGizmoClass, propSendPopup);
}

Setting *		RecordSetting;

static void
OutputRecord (mp)
MailRec *	mp;
{
	char	cmd[BUF_SIZE];

	if ((int)RecordSetting->current_value == 0) {
		RecordOutgoing = DoIt;
		sprintf (cmd, "%s record=%s", SET_CMD, Record);
	}
	else {
		RecordOutgoing = DontDoIt;
		sprintf (cmd, "%s norecord", SET_CMD);
	}
	(void)ProcessCommand (mp, cmd, NULL);
}

static void
PropertyCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	Widget			shell;

	shell = GetPopupGizmoShell(propSendPopup);

	switch (p->item_index) {
		case PropSet:
			ManipulateGizmo (
				(GizmoClass)PopupGizmoClass,
				propSendPopup,
				GetGizmoValue
			);

			ManipulateGizmo (
				(GizmoClass)PopupGizmoClass,
				propSendPopup,
				SetGizmoValue
			);
			RecordSetting = (Setting *)QueryGizmo (
				PopupGizmoClass, propSendPopup,
				GetGizmoSetting, RECORD
			);
			ApplyToEachMailx (OutputRecord);
			/* Get the signature */
			if (Signature != NULL) {
				FREE (Signature);
			}
			Signature = GetTextFieldValue (&signBox);
			BringDownPopup(shell);
			break;
		case PropReset:
			SetTextFieldValue (&signBox, Signature);
			ManipulateGizmo (
				(GizmoClass)PopupGizmoClass,
				propSendPopup,
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
