/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/property.c	1.29"
#endif

#include <ctype.h>
#include <IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <PopupWindo.h>
#include <FButtons.h>
#include <FList.h>
#include <StaticText.h>
#include <TextField.h>
#include <Caption.h>
#include <CoreP.h>
#include "inet.h"
#include <ListGizmo.h>
#include <MenuGizmo.h>
#include <InputGizmo.h>
#include <ChoiceGizm.h>
#include <STextGizmo.h>
#include <SpaceGizmo.h>
#include <LabelGizmo.h>
#include "error.h"

extern char *		ApplicationName;

extern void		FreeList();
extern void		SetFields();
extern void		ResetFields();
extern void		UnselectSelect();
extern void		ChangeCB();
extern void		AddrChangeCB();
extern void		HelpCB();
extern Boolean		VerifyNameField();
extern Boolean		VerifyAddrField();
extern XtArgVal		GetValue();

static void             PropPopdownCB();
static void		InsertCB();
static void		ApplyCB();
static void		DeleteCB();
static void		GetPrivilegeInfo();
static char *		SetRhosts();
static void		HandleButtonCB();
static void		FullAddrButtonSelectCB();
static void		AllOrNoneAllowCB();
static void		SpecifyAllowCB();
static Boolean		VerifyAllFields();
static void		InitProp();

static HelpText AppHelp = {
    title_properties, HELP_FILE, help_properties,
};

static MenuItems  PropMenuItems[] = {
	{True,	label_apply,	mnemonic_apply, NULL, HandleButtonCB },
	{True,	label_reset,	mnemonic_reset, NULL, HandleButtonCB },
	{True,	label_cancel,	mnemonic_cancel, NULL, HandleButtonCB },
	{True,	label_help,	mnemonic_help, NULL, HelpCB, (char*)&AppHelp },
	{ 0 }
};

static MenuGizmo PropMenu = {
	NULL, "basic_but", "_X_", PropMenuItems
};

static SpaceGizmo space = {
	5, 10
};

/* Define the Host Name input field in the basic property sheet */

static Setting name = {
	"", "",
};

static InputGizmo hostName = {
	NULL,			/* HELP		*/
	"hostName",		/* NAME		*/
	label_system,		/* CAPTION	*/
	NULL,			/* TEXT		*/
	&name,			/* SETTING	*/
	14,			/* CHARSVIS	*/
	ChangeCB		/* VERIFY	*/
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

static StaticTextGizmo dot = {
	NULL,			/* HELP		*/
	"dot_text",		/* NAME		*/
	"."
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
	label_netAddr,		/* CAPTION	*/
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


static MenuItems  allowItems[] = {
	{True,	label_none,	mnemonic_none,0, AllOrNoneAllowCB},
	{True,	label_all,	mnemonic_all,0, AllOrNoneAllowCB},
	{True,	label_specify,	mnemonic_specify,0, SpecifyAllowCB},
	{ 0 }
};

static Setting allow;

static MenuGizmo  allowMenu =
	{ NULL,
	"allowed",
	"_X_",
	allowItems,
	NULL,
	NULL,
	EXC,
	OL_FIXEDROWS,
	1,
	0,
};

static ChoiceGizmo allowMode = {
        NULL,
        "allow",
        "",
        &allowMenu,
        &allow,
};

/* Define the login input field in the basic property sheet */
static Setting login;

static InputGizmo loginField = {
	NULL,			/* HELP		*/
	"login",		/* NAME		*/
	"",			/* CAPTION	*/
	NULL,			/* TEXT		*/
	&login,			/* SETTING	*/
	8,			/* CHARSVIS	*/
	ChangeCB,		/* VERIFY	*/
};

static GizmoRec allowArray[] = {
	{ChoiceGizmoClass,	&allowMode },
	{InputGizmoClass,	&loginField}
};

static LabelGizmo allowControl = {
	NULL,
	"allowControl",
        label_permit,
	allowArray,
	XtNumber (allowArray),
	OL_FIXEDROWS,
	1,
	0,
	0,
	False
};


static ListHead currentItem = {
	(ListItem *)0, 0, 0
};

static ListHead previousItem = {
	(ListItem *)0, 0, 0
};

static ListHead initialItem = {
	(ListItem *)0, 0, 0
};

static Setting RhostsList = {
	"Rhosts",
	(XtPointer)&initialItem,
	(XtPointer)&currentItem,
	(XtPointer)&previousItem
};

/* Define the rhosts scrolling list in the basic property sheet 
 * 	XtNformat	"%13s"
 *	XtNviewHeight	4
*/
static ListGizmo list = {
	NULL,
	"rhostsList",
	"rhosts list",
	(Setting *)&RhostsList,
	"%13s",
	True,
	4,
	"fixed",
	True
};

static MenuItems rhostsItems[] = {
    {False,	label_insert,	mnemonic_insert,	NULL,	InsertCB},
    {False,	label_delete,	mnemonic_delete,	NULL,	DeleteCB},
    {False,	label_applyEdit,mnemonic_applyEdit,	NULL,	ApplyCB},
    { NULL }
};

static MenuGizmo Rhosts =	{
	NULL,
	"host_but",
	"Rhosts",
	rhostsItems,
	NULL,
	NULL,
	CMD,
	OL_FIXEDCOLS,
	1,
	OL_NO_ITEM
};

static GizmoRec menuArray[] = {
	{SpaceGizmoClass,	&space},
	{MenuBarGizmoClass,	&Rhosts}
};

static LabelGizmo menuContainer = {
	NULL,
	"menuContainer",
	NULL,
	menuArray,
	XtNumber (menuArray),
	OL_FIXEDCOLS,
	1,
	0,
	0,
	False
};

static GizmoRec listArray[] = {
	{ListGizmoClass,	&list},
	{LabelGizmoClass,	&menuContainer}
};

static LabelGizmo listContainer = {
	NULL,
	"listContainer",
	label_allow,
	listArray,
	XtNumber (listArray),
	OL_FIXEDROWS,
	1,
	0,
	0,
	True
};

static GizmoRec PropGiz[] = {
	{InputGizmoClass,	&hostName},		/* 0 */
	{ChoiceGizmoClass,	&addressMode },		/* 1 */
	{LabelGizmoClass,	&hostAddr},		/* 2 */
	{InputGizmoClass,	&hostComment},		/* 3 */
	{LabelGizmoClass,	&allowControl},		/* 4 */
	{LabelGizmoClass,	&listContainer},	/* 5 */
};

static char title[160];
static PopupGizmo PropPopup = {
	NULL,			/* help			*/
	"basic",		/* name of the shell	*/
	title,			/* title		*/
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
				&PropPopup,
				GetGizmoGizmo,
				"hostAddr");
	}

	for (gp = lgp->gizmos; gp < &lgp->gizmos[lgp->num_gizmos-1]; gp++)
		XtManageChild( ((InputGizmo *)gp->gizmo)->captionWidget);
	/* reset the caption widget's label of the last part of the IPaddr */
	SetValue(((InputGizmo *)gp->gizmo)->captionWidget,
			XtNlabel, ((InputGizmo *)gp->gizmo)->caption);
	hf->address = 1;
	
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
			&PropPopup,
			GetGizmoGizmo,
			"hostAddr");
	}
	for (gp = lgp->gizmos; gp < &lgp->gizmos[lgp->num_gizmos-1]; gp++)
		XtUnmanageChild( ((InputGizmo *)gp->gizmo)->captionWidget);
	/* erase the caption widget's label of the last part of the IPaddr */
	SetValue(((InputGizmo *)gp->gizmo)->captionWidget,
			XtNlabel, " ");
	hf->address = 0;
} /* ShowSimpleAddress */

static void
FullAddrButtonSelectCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	static Boolean	first_time = True;
	Widget		w_check;

	if (first_time) {
		first_time = False;
		SetValue ( w_check = QueryGizmo(PopupGizmoClass,
				&PropPopup,
				GetGizmoWidget,
				"address"),
				XtNunselectProc,	ShowSimpleAddress,
				(XtPointer)0
			);
		OlVaFlatSetValues( w_check,
				0,
				XtNset, True,
				(XtPointer)0
			);
	}
	ShowFullAddress();
} /* FullAddrButtonSelectCB */

/*
 * Get the list of users that are allowed to access the local machine without
 * the password checking.  This list comes from querying the /etc/host.equiv file.
*/

static void
GetRhostsList (buf)
char *		buf;
{
	char **		tmp;
	char *		cp;
	int		i;
	ListHead *	hp;
	static Boolean	first_time = True;

	if(first_time) {
		hp = (ListHead *) (list.settings->current_value);
		first_time = False;

	} else {
		hp = (ListHead *) (list.settings->previous_value);
	}
	hp->numFields = 1;

	/* Determine the number of items by the # of CRs */

	hp->size = 0;
	for (cp=buf; *cp!='\0'; cp++) {
		if (*cp == '\n') {
			hp->size += 1;
		}
	}

	if (hp->size == 0) {
		hp->list = (ListItem *)0;
		return;
	}

	/* Alloc the space for numRhosts items */

	hp->list = (ListItem *)XtMalloc(
		sizeof(ListItem) * hp->size
	);

	/* Parse buf for lines and put each line into a list item */

	cp = buf;
	for (i=0; i<hp->size; i++) {
		hp->list[i].set = False;
		hp->list[i].fields = (XtArgVal)XtMalloc (
			sizeof (XtArgVal *) * 1
		);
		tmp = (char **)hp->list[i].fields;
		tmp[0] = strdup (strtok (cp, "\n"));
		cp = NULL;
	}
} /* GetRhostsList */

static void
SetSensitiveList(flag)
Boolean	flag;
{
	static MenuGizmo *gp;
	static Boolean	first_time = True;
	static Widget	buttons;
	static Widget	w_login;
	register	i;
	if (first_time) {
		first_time = False;
		gp = (MenuGizmo *)QueryGizmo(PopupGizmoClass,
				&PropPopup,
				GetGizmoGizmo,
				"host_but");
		buttons = gp->child;
		w_login = (Widget)QueryGizmo(PopupGizmoClass,
				&PropPopup,
				GetGizmoWidget,
				"login");
	}
	for (i=0; i< XtNumber(rhostsItems) -1; i++)
		OlVaFlatSetValues(buttons, i, XtNsensitive, flag, 0);
	if (flag)
		XtManageChild(w_login);
	else
		XtUnmanageChild(w_login);

} /* SetSensitiveList */

void
SpecifyPrivilegeLogin(hp)
HostData *hp;
{

	if (hp->f_allow == SPECIFY) {
		SetSensitiveList(True);
	} else {
		SetSensitiveList(False);
	}

} /* SpecifyPrivilegeLogin */

static char *
SetRhosts ()
{
	HostData *	hp = hf->flatItems[hf->currentItem].pField;
	char		line[BUFSIZ];
	char		buf[BUFSIZ];
	char **		tmp;
	int		i;

	memset(line, 0, BUFSIZ);
	memset(buf, 0, BUFSIZ);
	/* allow all user items */
	/* and reset them from the list */
	for (i=0; i<currentItem.size; i++) {
		tmp = (char **) currentItem.list[i].fields;
		sprintf (line, "%s\n", tmp[0]);
		strcat(buf, line);
	}
	return (buf);
}

static void
AllOrNoneAllowCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	Gizmo *		List;
	static ListGizmo *lgp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		lgp = (ListGizmo *)QueryGizmo(PopupGizmoClass,
				&PropPopup,
				GetGizmoGizmo,
				"rhostsList");
	}
	XtVaSetValues (
		GetList (lgp),
		XtNnumItems,		0,
		XtNitems,		NULL,
		XtNviewHeight,		list.height,
		XtNitemsTouched,	True,
		(String)0
	);
	SetSensitiveList(False);
} /* AllOrNoneAllowCB */

static void
SpecifyAllowCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	SetSensitiveList(True);
} /* SpecifyAllowCB */

static void
ApplyCB (wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	int		n;	/* Find the current selected item */
	Widget		w_login;
	Gizmo *		gp;
	Gizmo *		List;
	char **		tmp;
	char *		text;
	char *		cp;
	Boolean		nonblank;

	n = 0;
	w_login = (Widget)QueryGizmo(PopupGizmoClass,
			&PropPopup,
			GetGizmoWidget,
			"login");
	text = (char *) GetValue (w_login, XtNstring);

	ClearFooter(hf->footer);	/* Clear footer on any operation */

	/* Rhosts Apply if text field is empty or blank. */

	if (text == NULL || text[0] == '\0') {
	  return;
	}
	nonblank = False;
	for (cp = text; *cp!='\0'; cp++) {
	  if (*cp != ' ' && *cp != '\t' && *cp != '\n') {
		  nonblank = True;
		  break;
	  }
	}
	if (nonblank == False) {
	  return;
	}

	if (currentItem.size == 0) {
	  /* If the list has no items in it, add one for the */
	  /* user. */
	  InsertCB (wid, clientData, callData);
	}

	for (; currentItem.list[n].set == False; n++) ; /*Current item*/

	tmp = (char **)currentItem.list[n].fields;
	if (tmp[0] != NULL) {
	  XtFree (tmp[0]);
	}
	tmp[0] = text;

	gp = GetGizmo (PopupGizmoClass, &PropPopup, 5);
	List = GetGizmo (LabelGizmoClass, gp, 0);
	XtVaSetValues (
		GetList ((ListGizmo *)List),
		XtNnumItems,		currentItem.size,
		XtNitems,		currentItem.list,
		XtNviewHeight,		list.height,
		XtNitemsTouched,	True,
		(String)0
	);
} /* ApplyCB */

static void
DeleteCB (wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	int		n;

	if (currentItem.size == 0) {
	  return;
	}

	/* Find the current selected item */
	for (n=0; currentItem.list[n].set == False; n++){
	}

	/* Delete the item from the list */

	DeleteListItem (&list, n);
} /* DeleteCB */

static void
InsertCB (wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	int		n;

	/* Find the current selected item */

	n = 0;
	if (currentItem.size != 0) {
	  for (; currentItem.list[n].set == False; n++){
	  }
	}

	/* Insert a blank item just before this the current item */

	InsertListItem (&list, n, NULL, 0);
} /* InsertCB */

/* Create the Privilege Login property block */

static void
GetPrivilegeInfo (hp)
HostData *hp;
{
	char *buf;

	buf = strdup(hp->equiv);
	/* Call the function that displays the output */
	GetRhostsList(buf);
	XtFree (buf);
} /* GetPrivilegeInfo */

static void
InitProp()
{
	GizmoRec * gp;
	static LabelGizmo *lgp;

	sprintf(title, "%s: %s", ApplicationName, GGT(label_propertiesTitle));
	SET_HELP(AppHelp);
	hf->propPopup = CreateGizmo(hf->toplevel,
					PopupGizmoClass,
					&PropPopup,
					NULL,
					0);

	XtAddCallback (
                hf->propPopup,
                XtNpopdownCallback,
                PropPopdownCB,
                (XtPointer)0
        );

	/* tailor the net_addr textfield widget */
	lgp = (LabelGizmo *)QueryGizmo(PopupGizmoClass,
			&PropPopup,
			GetGizmoGizmo,
			"hostAddr");
	for (gp = lgp->gizmos; gp <= &lgp->gizmos[lgp->num_gizmos-1]; gp++)
		/* set the source to each textfield widget */
		SetValue(((InputGizmo *)gp->gizmo)->textFieldWidget,
			XtNmaximumSize, 3);
} /* InitProp */

void
PropPopupCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{

	HostData *	hp = hf->flatItems[hf->currentItem].pField;
	Widget 		w_name;
	if (!new && hf->numFlatItems == 0) { /* nothing to operate */
		return;
	}
	GetPrivilegeInfo(hp);
	if (hf->propPopup == (Widget) 0) {
		InitProp();
        }
        MapGizmo(PopupGizmoClass, &PropPopup);
	/* select the current entry */
	if (!new && hf->numFlatItems)
		ResetFields(hp);
	else
		ResetFields(new->pField);
	
	if (hf->address == 0)
		ShowSimpleAddress();
	else
		FullAddrButtonSelectCB(wid, 0, 0);
	if (hp->f_allow != SPECIFY)
		AllOrNoneAllowCB(wid, 0, 0);
	w_name = (Widget)QueryGizmo(PopupGizmoClass,
			&PropPopup,
			GetGizmoWidget,
			"hostName");

	/* Reset the cusor to the first text field */
	SetValue (hf->propPopup, XtNfocusWidget, w_name);

	/* clear up the footer msg on the base window */
	CLRMSG();
} /* PropPopupCB */

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
	Widget	popup = GetPopupGizmoShell(&PropPopup);

	switch (action) {

	case PropApply:
		ClearFooter(hf->footer); /* clear mesaage area */
		ManipulateGizmo((GizmoClass)&PopupGizmoClass,
				&PropPopup,
				GetGizmoValue);
		if (VerifyAllFields() == True) { /* something wrong */
			return;
		}
		/* needs to check for duplication */
		if(new) ApplyNewEntry(False);
		else {
			dp = hf->flatItems[hf->currentItem].pField;
			SetFields(dp);
			hf->changesMade = True;
			XtVaSetValues (
				hf->scrollingList,
				XtNviewHeight,          VIEWHEIGHT,
				XtNitemsTouched,	True,
				(String)0
			);
			/* Re-select the current item since we have */
			/* given a new list			    */
			XtVaSetValues (
				hf->scrollingList,
				XtNviewItemIndex,	hf->currentItem,
				(String)0
			);

			OlVaFlatSetValues (
				hf->scrollingList,
				hf->currentItem,
				XtNset,	True,
				0
			);
		}
		
		ManipulateGizmo((GizmoClass)&PopupGizmoClass,
				&PropPopup,
				ApplyGizmoValue);
		BringDownPopup(popup);
		break;

	case PropReset:
		if (new) {
			dp = new->pField;
		} else {
			dp = hf->flatItems[hf->currentItem].pField;
		}
		ResetFields(dp);
		break;

	case PropCancel:
		
		SetWMPushpinState(XtDisplay(popup), XtWindow(popup), WMPushpinIsOut);
		XtPopdown(hf->propPopup);
		break;

	default:
		fprintf(stderr,"HandleButtonCB: Unknown action %d\n", action);
		break;
	}

	return;
} /* HandleButtonCB */

extern void
ChangeCB (textField, which, tf_verify)
Widget	textField;
int	which;
OlTextFieldVerify       *tf_verify;
{
	char *		f_name;

	f_name = tf_verify->string;

	VerifyNameField(f_name);

} /* ChangeCB */

extern void
AddrChangeCB(textField, which, tf_verify)
Widget textField;
int which;
OlTextFieldVerify       *tf_verify;
{
	char *f_addr;

	f_addr = tf_verify->string;

	VerifyAddrField(f_addr);
	return;

} /* AddrChangeCB */

/* Perform the form-wise checking */
static Boolean
VerifyAllFields()
{
#define SETTING(WHICH)	WHICH.current_value
	Boolean	flag = False;

	if(VerifyNameField((XtPointer) SETTING(name)) == True) flag = True;
	if(VerifyAddrField((XtPointer) SETTING(IPaddr.octet1),
			F_NET1_ADDR) == True) flag = True;
	if(VerifyAddrField((XtPointer) SETTING(IPaddr.octet2),
			F_NET2_ADDR) == True) flag = True;
	if(VerifyAddrField((XtPointer) SETTING(IPaddr.octet3),
			F_NET3_ADDR) == True) flag = True;
	if(VerifyAddrField((XtPointer) SETTING(IPaddr.octet4),
			F_NET4_ADDR) == True) flag = True;
	return(flag);
} /* VerifyAllFields */

/* Name Ok: return False, otherwise True */
extern Boolean
VerifyNameField(f_name)
String	f_name;
{
	CLRMSG();
	if(strlen(f_name) == 0) {
		PUTMSG(GGT(string_blankName));
		return (True);
	}
	if(isdigit(f_name[0]) || (strchr(f_name, ' ') != NULL)) {
		PUTMSG(GGT(string_badName));
		return (True);
	}
	return (False);
} /* VerifyNameField */

/* Addr Ok: return False, otherwise True */
extern Boolean
VerifyAddrField(f_addr, which)
String	f_addr;
int	which;
{
	register	i, addr;
	CLRMSG();
	if(strlen(f_addr) == 0) {
		PUTMSG(GGT(string_blankAddr));
		return (True);
	}
	for (i = 0; f_addr[i]; i++) {
		if (!isdigit(f_addr[i]) && !isspace(f_addr[i])) {
			PUTMSG(GGT(string_badAddr));
			return (True);
		}
	}
	if ((addr = atoi(f_addr)) > 254 || addr < 0) {
		PUTMSG(GGT(string_badAddr));
		return (True);
	}

	switch (which) { 
	case F_NET2_ADDR:
	case F_NET3_ADDR:
	case F_NET4_ADDR:
		if(addr > 254 || addr < 1) {
			PUTMSG(GGT(string_badAddr));
			return (True);
		}
		break;
	case F_NET1_ADDR:
		if(addr > 223 || addr < 1) {
			PUTMSG(GGT(string_badAddr));
			return (True);
		}
		break;
	}
	return (False);
} /* VerifyAddrField */

void
SetFields(hp)
HostData	*hp;
{
	XtFree (hp->f_name);
	XtFree (hp->f_net1Addr);
	XtFree (hp->f_net2Addr);
	XtFree (hp->f_net3Addr);
	XtFree (hp->f_net4Addr);
	XtFree (hp->f_IPaddr);
	XtFree (hp->f_comment);
	hp->f_name = strdup(name.current_value);
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
	hp->f_comment = strdup((char *)comment.current_value);
	if ((hp->f_allow = (int) allow.current_value) == SPECIFY) {
		if (hp->equiv) XtFree(hp->equiv);
		hp->equiv = strdup(SetRhosts());
	} else {
		if (hp->equiv) {
			XtFree(hp->equiv);
			hp->equiv = NULL;
		}
	}
} /* SetFields */

/* Release all the property sheets fields and replaced with the
 * hosts data that is passed to it.  There are two possibilities
 * here; a new hosts (by using ``Add'' button) or an existing host
 * (by ``UnselectSelect''.
 */
void
ResetFields(hp)
HostData	*hp;
{
	if (hf->propPopup == NULL) {
		return;
	}
	XtFree(name.previous_value);
	XtFree(address.previous_value);
	XtFree(IPaddr.octet1.previous_value);
	XtFree(IPaddr.octet2.previous_value);
	XtFree(IPaddr.octet3.previous_value);
	XtFree(IPaddr.octet4.previous_value);
	XtFree(comment.previous_value);
	FreeList(&previousItem);

	name.previous_value = strdup(hp->f_name);
	address.previous_value = strdup((XtPointer)hf->address ? "x": "-");
	IPaddr.octet1.previous_value = strdup(hp->f_net1Addr);
	IPaddr.octet2.previous_value = strdup(hp->f_net2Addr);
	IPaddr.octet3.previous_value = strdup(hp->f_net3Addr);
	IPaddr.octet4.previous_value = strdup(hp->f_net4Addr);
	comment.previous_value = strdup(hp->f_comment);
	allow.previous_value = (XtPointer)hp->f_allow;
	GetPrivilegeInfo(hp);
	
	if (XtIsRealized(hf->propPopup)) {
		SpecifyPrivilegeLogin(hp);
		ManipulateGizmo((GizmoClass)&PopupGizmoClass,
				&PropPopup,
				ResetGizmoValue);
	}
} /* ResetFields */

static void
PropPopdownCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	/* check if this entry is a newly added but net yet applied */
	if (new) {
		FreeNew();
		UnselectSelect();
	}
}
