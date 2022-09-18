/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/local.c	1.15"
#endif

#include <ctype.h>
#include <IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <PopupWindo.h>
#include <FButtons.h>
#include <StaticText.h>
#include <TextField.h>
#include <Caption.h>
#include <CoreP.h>
#include "inet.h"
#include <MenuGizmo.h>
#include <InputGizmo.h>
#include <ChoiceGizm.h>
#include "STextGizmo.h"
#include <LabelGizmo.h>
#include "error.h"

extern char *		ApplicationName;
extern PopupGizmo *	localPopup;

extern void		FreeList();
extern void		Save();
extern void		SetLocalFields();
extern void		ChangeCB();
extern void		HelpCB();
extern void		AddrChangeCB();
extern Boolean		VerifyAddrField();

static void             LocalPopdownCB();
static void		HandleButtonCB();
static void		FullAddrButtonSelectCB();
static void		AllowButtonSelectCB();
static Boolean		VerifyAllFields();
static void		InitLocal();

static HelpText AppHelp = {
    title_properties, HELP_FILE, help_properties,
};

static MenuItems  PropMenuItems[] = {
	{True,	label_apply,	mnemonic_apply, NULL, HandleButtonCB },
	{True,	label_reset,	mnemonic_reset, NULL, HandleButtonCB },
	{True,	label_cancel,	mnemonic_cancel, NULL, HandleButtonCB },
	{True,	label_help,	mnemonic_help, NULL, HelpCB, (char *)&AppHelp },
	{ 0 }
};

static MenuGizmo PropMenu = {
	NULL, "basic_but", "_X_", PropMenuItems
};

/* Define the Local Host Name field in the local property sheet */

static StaticTextGizmo localName = {
	NULL,			/* HELP		*/
	"localName",		/* NAME		*/
	NULL,			/* TEXT		*/
	NorthWestGravity,	/* GRAVITY	*/
	"fixed"
};

static GizmoRec hostnames[] = {
        {StaticTextGizmoClass,  &localName}
};

static LabelGizmo hostName = {
	NULL,			/* HELP		*/
	"hostName",		/* NAME		*/
	label_local,		/* CAPTION	*/
	hostnames,		/* GIZMOARRAY	*/
	XtNumber(hostnames),	/* 	*/
	OL_FIXEDROWS,		/* LAYOUT	*/
	1,			/* MEASURE	*/
	0,			/* Arg array	*/
	0,			/* numArg	*/
	True			/* ALIGNCAP	*/
};

/* Create the full addressing check box */

static Setting address = {
	"address", (XtPointer)"_", (XtPointer)"_"
};

static MenuItems addressItems[] = {
        {True,		/* sensitive	*/
	"",		/* LABEL	*/
	"\0",		/* MNEMONIC	*/
	"addressMode",	/* NEXTTIER	*/
	FullAddrButtonSelectCB,	/* SELCB	*/
	0,		/* CLIENT_DATA	*/
	False},		/* SET	*/
        { 0 }
};

static MenuGizmo addressMenu = {
        NULL,		/*HELP		*/
        "full",		/* name		*/
        "_X_",		/* title	*/
        addressItems,	/* Items	*/
        0,		/* Function	*/
        NULL,		/* Client data	*/
        CHK,		/* Button type	*/
        OL_FIXEDCOLS,	/* Layout type	*/
        1,		/* Measure	*/
        OL_NO_ITEM	/* Default item	*/
};

static ChoiceGizmo addressMode = {
        NULL,			/* HELP		*/
        "address",		/* NAME		*/
        label_full,		/* CAPTION	*/
        &addressMenu,		/* MENU		*/
        &address		/* SETTINGS	*/
};

/* Define the IP address numeric fields of the property sheet */

static IPaddrSettings IPaddr = {
	{"octet1",	(XtPointer)"223"},
	{"octet2",	(XtPointer)"254"},
	{"octet3",	(XtPointer)"254"},
	{"octet4",	(XtPointer)""}
};

static InputGizmo net1_addr = {
	NULL,			/* HELP		*/
	"net1_addr",		/* NAME		*/
	"",			/* CAPTION	*/
	NULL,			/* TEXT		*/
	&IPaddr.octet1,		/* SETTING	*/
	3,			/* CHARSVIS	*/
	AddrChangeCB,		/* VERIFY	*/
};

static InputGizmo net2_addr = {
	NULL,			/* HELP		*/
	"net2_addr",		/* NAME		*/
	".",			/* CAPTION	*/
	NULL,			/* TEXT		*/
	&IPaddr.octet2,		/* SETTING	*/
	3,			/* CHARSVIS	*/
	AddrChangeCB,		/* VERIFY	*/
};

static InputGizmo net3_addr = {
	NULL,			/* HELP		*/
	"net3_addr",		/* NAME		*/
	".",			/* CAPTION	*/
	NULL,			/* TEXT		*/
	&IPaddr.octet3,		/* SETTING	*/
	3,			/* CHARSVIS	*/
	AddrChangeCB,		/* VERIFY	*/
};
static InputGizmo net4_addr = {
	NULL,			/* HELP		*/
	"net4_addr",		/* NAME		*/
	".",			/* CAPTION	*/
	NULL,			/* TEXT		*/
	&IPaddr.octet4,		/* SETTING	*/
	3,			/* CHARSVIS	*/
	AddrChangeCB,		/* VERIFY	*/
};

static GizmoRec IPAddrArray[] = {
	{InputGizmoClass,	&net1_addr},
	{InputGizmoClass,	&net2_addr},
	{InputGizmoClass,	&net3_addr},
	{InputGizmoClass,	&net4_addr}
};

static LabelGizmo hostAddr = {
	NULL,			/* HELP		*/
	"hostAddr",		/* NAME		*/
	label_netAddr,	/* CAPTION	*/
	IPAddrArray,		/* GIZMOARRAY	*/
	XtNumber(IPAddrArray),	/* 	*/
	OL_FIXEDROWS,		/* LAYOUT	*/
	1,			/* MEASURE	*/
	0,			/* Arg array	*/
	0,			/* numArg	*/
	False			/* ALIGNCAP	*/
};

/* Define the Comment input field in the basic property sheet */

static Setting comment = {
	"", "",
};

static InputGizmo hostComment = {
	NULL,			/* HELP		*/
	"comment",		/* NAME		*/
	label_comment,		/* CAPTION	*/
	NULL,			/* TEXT		*/
	&comment,		/* SETTING	*/
	36,			/* CHARSVIS	*/
};

static GizmoRec PropGiz[] = {
	{LabelGizmoClass,	&hostName},		/* 0 */
	{ChoiceGizmoClass,	&addressMode },		/* 1 */
	{LabelGizmoClass,	&hostAddr},		/* 2 */
	{InputGizmoClass,	&hostComment},		/* 3 */
};

static char title[160];
extern PopupGizmo LocalPopup = {
	NULL,			/* help			*/
	"basic",		/* name of the shell	*/
	title,
	&PropMenu,		/* Pointer to menu info	*/
	PropGiz,		/* the gizmo list	*/
	XtNumber (PropGiz),	/* number of gizmos	*/
};

static void
ShowFullAddress()
{
	GizmoRec * gp;
	static LabelGizmo *lgp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		lgp = (LabelGizmo *)QueryGizmo(PopupGizmoClass,
				&LocalPopup,
				GetGizmoGizmo,
				"hostAddr");
	}

	for (gp = lgp->gizmos; gp < &lgp->gizmos[lgp->num_gizmos-1]; gp++)
		XtManageChild( ((InputGizmo *)gp->gizmo)->captionWidget);
	/* reset the caption widget's label of the last part of the IPaddr */
	SetValue(((InputGizmo *)gp->gizmo)->captionWidget,
			XtNlabel, ((InputGizmo *)gp->gizmo)->caption);
	hf->address = True;
	
} /* ShowFullAddress */

static void
ShowSimpleAddress()
{
	GizmoRec * gp;
	static LabelGizmo *lgp;
	static Boolean first_time = True;

	if(first_time) {
	first_time = False;
	lgp = (LabelGizmo *)QueryGizmo(PopupGizmoClass,
			&LocalPopup,
			GetGizmoGizmo,
			"hostAddr");
	}
	for (gp = lgp->gizmos; gp < &lgp->gizmos[lgp->num_gizmos-1]; gp++)
		XtUnmanageChild( ((InputGizmo *)gp->gizmo)->captionWidget);
	/* erase the caption widget's label of the last part of the IPaddr */
	SetValue(((InputGizmo *)gp->gizmo)->captionWidget,
			XtNlabel, " ");
	hf->address = False;
}

static void
FullAddrButtonSelectCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	static Boolean	first_time = True;
	if (first_time) {
		first_time = False;
		SetValue ( QueryGizmo(PopupGizmoClass,
				&LocalPopup,
				GetGizmoWidget,
				"address"),
				XtNunselectProc,	ShowSimpleAddress
			);
	}
	ShowFullAddress();
} /* FullAddrButtonSelectCB */

static void
InitLocal()
{
	GizmoRec * gp;
	static LabelGizmo *lgp;

	sprintf(title, "%s: %s", ApplicationName, GGT(label_propertiesLTitle));
	SET_HELP(AppHelp);
	hf->localPopup = CreateGizmo(hf->toplevel,
					PopupGizmoClass,
					&LocalPopup,
					NULL,
					0);

	XtAddCallback (
                hf->localPopup,
                XtNpopdownCallback,
                LocalPopdownCB,
                (XtPointer)0
        );

	/* tailor the net_addr textfield widget */
	lgp = (LabelGizmo *)QueryGizmo(PopupGizmoClass,
			&LocalPopup,
			GetGizmoGizmo,
			"hostAddr");
	for (gp = lgp->gizmos; gp <= &lgp->gizmos[lgp->num_gizmos-1]; gp++)
		/* set the source to each textfield widget */
		SetValue(((InputGizmo *)gp->gizmo)->textFieldWidget,
			XtNmaximumSize, 3);

} /* InitLocal */

void
LocalPopupCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{

	HostData *	hp;
	if (hf->localPopup == (Widget) 0) {
		InitLocal();
        }
        MapGizmo(PopupGizmoClass, &LocalPopup);

	SetStaticTextGizmoText(&localName, hf->nodeName);
	XtFree(IPaddr.octet1.previous_value);
	XtFree(IPaddr.octet2.previous_value);
	XtFree(IPaddr.octet3.previous_value);
	XtFree(IPaddr.octet4.previous_value);
	hp = new->pField;
	IPaddr.octet1.previous_value = strdup(hp->f_net1Addr);
	IPaddr.octet2.previous_value = strdup(hp->f_net2Addr);
	IPaddr.octet3.previous_value = strdup(hp->f_net3Addr);
	IPaddr.octet4.previous_value = strdup(hp->f_net4Addr);
	
	ManipulateGizmo((GizmoClass)&PopupGizmoClass,
			&LocalPopup,
			ResetGizmoValue);

	if(hf->address == False)
		ShowSimpleAddress();
	else
		FullAddrButtonSelectCB(wid, 0, 0);
} /* LocalPopupCB */

/*
 * HandleButtonCB() - manages events that effect the state of the 
 * entire popup window.
 */
void
HandleButtonCB(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	
	register HostData	*dp;
	int	action = (int) ((OlFlatCallData *)call_data)->item_index;
	Widget	popup = GetPopupGizmoShell(&LocalPopup);

	switch (action) {

	case PropApply:
		ManipulateGizmo((GizmoClass)&PopupGizmoClass,
				&LocalPopup,
				GetGizmoValue);
		if (VerifyAllFields() == True) { /* something wrong */
			return;
		}
		ApplyNewEntry(True);
		
		ManipulateGizmo((GizmoClass)&PopupGizmoClass,
				&LocalPopup,
				ApplyGizmoValue);
		Save (wid, 0, 0);
		InitNetwork(wid);
		BringDownPopup(popup);
		break;

	case PropReset:
		dp = new->pField;
		ResetFields(dp);
		XtVaSetValues (
			hf->scrollingList,
			XtNviewHeight,          VIEWHEIGHT,
			XtNitemsTouched,	True,
			(String)0
		);
		break;

	case PropCancel:
		
		SetWMPushpinState(XtDisplay(popup), XtWindow(popup), WMPushpinIsOut);
		XtPopdown(hf->localPopup);
		break; /* intentional */

	default:
		fprintf(stderr,"HandleButtonCB: Unknown action %d\n", action);
		break;
	}

	return;
} /* HandleButtonCB */


/* Perform the form-wise checking */
static Boolean
VerifyAllFields()
{
#define SETTING(WHICH)	WHICH.current_value
	Boolean	flag = False;

	if(VerifyAddrField((XtPointer) SETTING(IPaddr.octet1),
			F_NET1_ADDR) == True) flag = True;
	if(VerifyAddrField((XtPointer) SETTING(IPaddr.octet2),
			F_NET2_ADDR) == True) flag = True;
	if(VerifyAddrField((XtPointer) SETTING(IPaddr.octet3),
			F_NET3_ADDR) == True) flag = True;
	if(VerifyAddrField((XtPointer) SETTING(IPaddr.octet4),
			F_NET4_ADDR) == True) flag = True;
	return(flag);
}

static void
LocalPopdownCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	FreeNew();
	if (!local)
		rexit(9, GGT(string_noLocal), "");
} /* LocalPopdownCB */

void
SetLocalFields(hp)
HostData	*hp;
{
	HostData	*lp;
	XtFree (hp->f_name);
	XtFree (hp->f_net1Addr);
	XtFree (hp->f_net2Addr);
	XtFree (hp->f_net3Addr);
	XtFree (hp->f_net4Addr);
	XtFree (hp->f_IPaddr);
	XtFree (hp->f_comment);
	hp->f_name = strdup(hf->nodeName);
	hp->f_net1Addr = strdup(IPaddr.octet1.current_value);
	hp->f_net2Addr = strdup(IPaddr.octet2.current_value);
	hp->f_net3Addr = strdup(IPaddr.octet3.current_value);
	hp->f_net4Addr = strdup(IPaddr.octet4.current_value);
	hp->f_IPaddr = (char *) XtMalloc(sizeof(char) * 15);
	sprintf(hp->f_IPaddr,"%s.%s.%s.%s",
				hp->f_net1Addr,
				hp->f_net2Addr,
				hp->f_net3Addr,
				hp->f_net4Addr
		);
	hp->f_comment = strdup(comment.current_value);
	if (!local) {
		local = (FlatList *) XtMalloc (sizeof(FlatList));
		lp = local->pField = (HostData *)XtMalloc(sizeof(HostData));
	} else {
		XtFree (lp->f_net1Addr);
		XtFree (lp->f_net2Addr);
		XtFree (lp->f_net3Addr);
	}
	lp->f_net1Addr = (XtPointer)strdup(hp->f_net1Addr);
	lp->f_net2Addr = (XtPointer)strdup(hp->f_net2Addr);
	lp->f_net3Addr = (XtPointer)strdup(hp->f_net3Addr);
} /* SetLocalFields */

