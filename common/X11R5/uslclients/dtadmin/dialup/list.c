/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/list.c	1.36"
#endif

/******************************file*header********************************

    Description:
        This routine uses the flattened list widget (aka. FlatList)
        to display the contents of the /etc/uucp/Systems file.
*/

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/PopupWindo.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/ControlAre.h>
#include <Xol/Footer.h>
#include <Xol/FList.h>
#include <Gizmos.h>
#include <STextGizmo.h>
#include <LabelGizmo.h>
#include <MenuGizmo.h>
#include <ChoiceGizm.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "uucp.h"
#include "error.h"

extern char *		OlTextFieldGetString();
extern char *		ApplicationName;
extern caddr_t		PortSpeed();
extern void		UnselectSelect();
extern void		FreeNew();
extern void		ApplyNewEntry();
extern void		SetFields();
extern void		ResetFields();
extern void		Warning();
extern void		DisallowPopdown();
extern void		HandleButtonCB();
extern Boolean		ChangeCB();
extern Boolean		VerifyAllFields();
extern Boolean		VerifyNameField();
extern Boolean		VerifyPhoneField();
extern void		GetFlatItems();
extern void		ModifyCB ();
extern Widget		CreateCaption();
extern int		CreateAttrsFile();

static void		SelectedType();
static void		ModemCB();
static void		DirectCB();
static void		DatakitCB();
static void		SelExtraCB();
static void		UnselExtraCB();
static void		PropPopdownCB();

Widget		w_phone;
DeviceItems	support_type[] = {
	{ "Modem", "ACU" },
	{ "Direct Line", "Direct" },
	{ "Data Kit", "DK" },
};

typedef enum {
	TypeACU, TypeDirect, TypeDK
} TypeMenuIndex;

Arg arg[50];
static char **target;

static String itemResources[] = {
    XtNformatData,
};

static void
CleanUpCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	char *fname;
	char msg[BUFSIZ];
	sprintf(msg, GGT(string_installDone),
		(fname=strrchr(*target, '/'))? fname + 1: *target);
	PUTMSG(msg);
	if (*target) XtFree (*target);
} /* CleanUpCB */

static void
DropProcCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	HostData		*dp;
	FILE			*attrp;
	OlFlatDropCallData	*drop = (OlFlatDropCallData *)call_data;
	static char 		node_directory[PATH_MAX];
	char 			buf[PATH_MAX];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;
	DmItemPtr		itemp;
	DmObjectPtr		op;

	dp = sf->flatItems[sf->currentItem].pField;

	/* check to see if the local machine has been selected */
	if (strcmp(dp->f_name, sf->nodeName) == 0) {
		PUTMSG(GGT(string_sameNode));
		return;
	}

	if (first_time) {
		first_time = False;
		sprintf (node_directory, "%s/.node", sf->userHome);
	}
	/* check the node directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(node_directory, stat_buf) )
		if (mkdir(node_directory, DMODE) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
		if (chown(node_directory, getuid(), getgid()) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
	else
		if ( stat_buf.st_mode != DMODE ) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}

#ifdef debug
	fprintf(stderr,"the DMODE is: %o\n", DMODE);
	fprintf(stderr,"the mode for %s is: %o\n", node_directory,
						stat_buf.st_mode);
#endif

	itemp = (DmItemPtr ) drop->item_data.items;
	num = drop->item_data.num_items;
	if (num = 1) {
		sprintf (buf, "%s/%s", node_directory, dp->f_name);
		target = (char **)malloc(sizeof(char *) * (1 + 1));
		*(target + 1) = NULL;
		*target = strdup(buf);
		attrp = fopen( *target, "w");
		if (attrp == (FILE *) 0) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}

		if (chmod( *target, MODE) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}

		/* put the node's properties here */
		fprintf( attrp, "DUSER=%s\nDPATH=\n",
			sf->userName);
		(void) fflush(attrp);
		fclose( attrp );
	} else {
		PUTMSG(GGT(string_noMultiple));
		return;
	}

	DtNewDnDTransaction(
		w,				/* owner widget */
		target,				/* file list */
		DT_B_STATIC_LIST,		/* options */
		drop->root_info->root_x,	/* root x */
		drop->root_info->root_y,	/* root y */
		drop->ve->xevent->xbutton.time,	/* timestamp */
		drop->dst_info->window,		/* dst window */
		DT_LINK_OP,			/* dnd hint */
		NULL,				/* del proc */
		CleanUpCB,			/* state proc */
		(XtPointer) *target		/* client data */
		);

} /* DropProcCB */

void
SelectCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatCallData *call_data;
{
	register HostData	*dp;
	int itemIndex = call_data->item_index;
	static Boolean first_time = True;
	static char	*phone_label;
	static char	*destination_label;

	if (first_time == True) {
		first_time = False;
		phone_label = GGT(label_phone);
		destination_label = GGT(label_destination);
	}
	ClearFooter(sf->footer); /* clear mesaage area */
	ClearLeftFooter(sf->sfooter);
	if (new) FreeNew();
	sf->currentItem = itemIndex;
	dp = sf->flatItems[itemIndex].pField;
	UnselectSelect ();
	if (strcmp(dp->f_type, support_type[TypeDirect].value) != 0) {
		SetValue( w_phone, XtNmappedWhenManaged, True, NULL);
		if (strcmp(dp->f_type, support_type[TypeACU].value) == 0)
			SetValue( w_phone, XtNlabel, phone_label, NULL);
		if (strcmp(dp->f_type, support_type[TypeDK].value) == 0)
			SetValue( w_phone, XtNlabel, destination_label, NULL);
	} else
		SetValue( w_phone, XtNmappedWhenManaged, False, NULL);
} /* SelectCB */

/* the title fields for the main flatlist */

static StaticTextGizmo text = {
	NULL, "text",
	string_header,
	NorthWestGravity,
	OlDefaultFixedFont
};

static GizmoRec header[] = {
	{StaticTextGizmoClass,	&text},
};

LabelGizmo HeaderLine = {
	NULL, "header", "", header, XtNumber (header),
	OL_FIXEDROWS, 1, 0, 0, True
};

/* create the Device type exclusive Gizmo */
static Setting stype;

static MenuItems  TypeItems[] = {
	{True,	label_modem,	mnemonic_modem, "modem", ModemCB },
	{True,	label_direct,	mnemonic_direct, "direct", DirectCB },
	{True,	label_datakit,	mnemonic_datakit, "datakit", DatakitCB },
	{ 0 }
};

static MenuGizmo TypeMenu = {
	NULL, "type", "_X_", TypeItems, NULL, NULL, EXC,
        OL_FIXEDROWS,	/* Layout type	*/
        1,		/* Measure	*/
        OL_NO_ITEM	/* Default item	*/
};

static ChoiceGizmo TypeChoice = {
	NULL,
	"type",
	label_modemType,
	&TypeMenu,
	&stype,
};

/* Create the extra item check box */

static Setting extra = {
	"extra", (XtPointer)"_"
};

static MenuItems extraItems[] = {
        {True,		/* sensitive	*/
	"",		/* LABEL	*/
	"\0",		/* MNEMONIC	*/
	"extraMode",	/* NEXTTIER	*/
	SelExtraCB,	/* SELCB	*/
	0,		/* CLIENT_DATA	*/
	False},		/* SET	*/
        { 0 }
};

static MenuGizmo extraMenu = {
        NULL,		/*HELP		*/
        "extra",	/* name		*/
        "_X_",		/* title	*/
        extraItems,	/* Items	*/
        0,		/* Function	*/
        NULL,		/* Client data	*/
        CHK,		/* Button type	*/
        OL_FIXEDCOLS,	/* Layout type	*/
        1,		/* Measure	*/
        OL_NO_ITEM	/* Default item	*/
};

static ChoiceGizmo extraMode = {
        NULL,			/* HELP		*/
        "extraItem",		/* NAME		*/
        label_extra,		/* CAPTION	*/
        &extraMenu,		/* MENU		*/
        &extra			/* SETTINGS	*/
};

static	void
UnselExtraCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	SetValue(sf->w_more, XtNmappedWhenManaged, FALSE, NULL);
} /* UnselExtraCB */

static void
SelExtraCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	static Boolean	first_time = True;
	Widget		w_check;

	if (first_time) {
		first_time = False;
		SetValue ( w_check = (Widget)QueryGizmo(ChoiceGizmoClass,
				&extraMode,
				GetGizmoWidget,
				"extraItem"),
				XtNunselectProc,	UnselExtraCB,
				(XtPointer)0
			);
		OlVaFlatSetValues( w_check,
				0,
				XtNset, True,
				(XtPointer)0
			);
		XtManageChild(sf->w_more);
	}
	SetValue( sf->w_more, XtNmappedWhenManaged, TRUE, NULL);
} /* SelExtraCB */

extern void             HelpCB();
static HelpText AppHelp = {
    title_property, HELP_FILE, help_property,
};

static Items propertyItems [] = {
    { HandleButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)APPLY}, /* apply */
    { HandleButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)RESET}, /* reset */
    { HandleButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)CANCEL}, /* cancel */
    { HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&AppHelp},
};

static Menus propertyMenu = {
	"property",
	propertyItems,
	XtNumber (propertyItems),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

Widget
InitLists(form)
Widget form;
{
	register	i;
	int		index;
	char 		buf[128];
	Widget		entryForm;
	Widget		header;
	Widget		sw;
	Widget		controlBottom;
	Widget		footerPanel;
	HostData	*dp;
	void		HandleButtonCB();

	SET_HELP(AppHelp);
	GetFlatItems (sf->filename);

	if (sf->numFlatItems == 0)
		dp = (HostData *) NULL;
	else
		dp = sf->flatItems[0].pField;
	header = CreateGizmo(form, LabelGizmoClass, &HeaderLine, arg, 0);
	SetValue(header, XtNweight,		0);
	/* Create the scrolling list of indices */

	sw = XtVaCreateManagedWidget (
		"Scrolled Window",
		scrolledWindowWidgetClass,
		form,
		XtNgranularity,		1,
		(String)0
	);
	sf->scrollingList = XtVaCreateManagedWidget (
		"Scrolling List",
		flatListWidgetClass,
		sw,
		XtNitems,		sf->flatItems,
		XtNnumItems,		sf->numFlatItems,
		XtNitemFields,		itemResources,
		XtNfont,		_OlGetDefaultFont(form, OlDefaultFixedFont),
		XtNnumItemFields,	XtNumber(itemResources),
		XtNviewHeight,		VIEWHEIGHT,
		XtNformat,		FORMAT,
		XtNselectProc,		SelectCB,
		XtNweight,		1,
		XtNdropProc,		DropProcCB,
		(String)0
	);

	/* Create the popup shell to contain the entry */

	sprintf(buf, "%s: %s", ApplicationName, GGT(title_properties));
	sf->propPopup = 
		XtVaCreatePopupShell(
			"systemProperties",		/* instance name */
			popupWindowShellWidgetClass,	/* widget class */
			form,				/* parent widget */
			XtNtitle,	buf,
			0
		);
	XtAddCallback (
                sf->propPopup,
                XtNverify,
                DisallowPopdown,
                (XtPointer)0
        );
	XtAddCallback (
                sf->propPopup,
                XtNpopdownCallback,
                PropPopdownCB,
                (XtPointer)0
        );

	/*
	 * retrieve the widget IDs of the control areas and the footer panel
	 */

	XtVaGetValues(
		sf->propPopup,
		XtNupperControlArea,	&entryForm,
		XtNlowerControlArea,	&controlBottom,
		XtNfooterPanel,		&footerPanel,
		0
	);
	sf->sfooter = XtVaCreateManagedWidget(
		"system_footer",
		footerWidgetClass,
		footerPanel,
		XtNweight,		0,
		XtNleftFoot,		"",
		XtNleftWeight,		100,
		XtNrightFoot,		"",
		XtNrightWeight,		0,
		(String)0
	);

	XtVaSetValues (
		entryForm,
        	XtNlayoutType, OL_FIXEDCOLS,
        	XtNmeasure, 1,
        	XtNborderWidth, 0,
		XtNalignCaptions,	True,
		(String)0
	);
	sf->w_name = (TextFieldWidget) XtVaCreateManagedWidget(
		"name",
		textFieldWidgetClass,
		CreateCaption(entryForm, GGT(label_system)),
		XtNcharsVisible, MAXNAMESIZE,
                XtNmaximumSize, MAXNAMESIZE,
		XtNstring, (dp) ? dp->f_name : NULL,
		(String)0
	);

	/* ChoiceGizmo returns the caption widget, not the flatbutton widget */
	CreateGizmo(entryForm, ChoiceGizmoClass, &TypeChoice, arg, 0);

	sf->w_type = GetChoiceButtons(&TypeChoice);
	PortSpeed(NEW, entryForm);
	if (dp != (HostData *) NULL) {
		PortSpeed(SET, dp->f_class);
	}
	sf->w_phone = (TextFieldWidget) XtVaCreateManagedWidget(
		"phone",
		textFieldWidgetClass,
		w_phone = CreateCaption(entryForm, GGT(label_phone)),
		XtNcharsVisible, MAXPHONESIZE,
                XtNmaximumSize, MAXPHONESIZE,
		XtNstring, (dp) ? dp->f_phone : NULL,
		(String)0
	);
	SetValue(w_phone,
		XtNmappedWhenManaged,
		(XtArgVal)(!strcmp(dp->f_type, support_type[TypeACU].value))
		);
	sf->w_passwd = (TextFieldWidget) XtVaCreateManagedWidget(
		"passwd",
		textFieldWidgetClass,
		CreateCaption(entryForm, GGT(label_password)),
		XtNcharsVisible, MAXNAMESIZE,
                XtNmaximumSize, MAXNAMESIZE,
		XtNstring, (dp) ? dp->f_passwd : NULL,
		(String)0
	);

	CreateGizmo(entryForm, ChoiceGizmoClass, &extraMode, arg, 0);

	sf->w_more = XtVaCreateWidget(
		"control",
		controlAreaWidgetClass,
		entryForm,
		XtNlayoutType,	(XtArgVal) OL_FIXEDCOLS,
		XtNcenter,	(XtArgVal) FALSE,
		XtNalignCaptions,	(XtArgVal) TRUE,
		XtNshadowThickness,	(XtArgVal) 0,
		(String)0
	);
	sf->w_expect1 = (TextFieldWidget) XtVaCreateManagedWidget(
		"expect1",
		textFieldWidgetClass,
		CreateCaption(sf->w_more, GGT(label_loginSeq1)),
		XtNcharsVisible, MAXNAMESIZE,
                XtNmaximumSize, MAXNAMESIZE,
		XtNstring, (dp) ? dp->f_expect1 : NULL,
		(String)0
	);
	sf->w_expect2 = (TextFieldWidget) XtVaCreateManagedWidget(
		"expect2",
		textFieldWidgetClass,
		CreateCaption(sf->w_more, GGT(label_loginSeq2)),
		XtNcharsVisible, MAXNAMESIZE,
                XtNmaximumSize, MAXNAMESIZE,
		XtNstring, (dp) ? dp->f_expect2 : NULL,
		(String)0
	);
	XtAddCallback (
		(Widget) sf->w_name,
		XtNmodifyVerification,
		(XtCallbackProc)ModifyCB,
		(caddr_t) sf->sfooter
	);
	XtAddCallback (
		(Widget) sf->w_phone,
		XtNmodifyVerification,
		(XtCallbackProc)ModifyCB,
		(caddr_t) sf->sfooter
	);
	XtAddCallback (
		(Widget) sf->w_name,
		XtNverification,
		(XtCallbackProc)ChangeCB,
		(caddr_t) F_NAME
	);
	XtAddCallback (
		(Widget) sf->w_phone,
		XtNverification,
		(XtCallbackProc)ChangeCB,
		(caddr_t) F_PHONE
	);
	SET_LABEL (propertyItems,0,apply);
	SET_LABEL (propertyItems,1,reset);
	SET_LABEL (propertyItems,2,cancel);
	SET_LABEL (propertyItems,3,help);
	AddMenu (controlBottom, &propertyMenu, False);
	return sw;
} /* InitLists */

/*
 * HandleButtonCB() - manages events that effect the state of the 
 * entire popup window.
 */
void HandleButtonCB(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	register HostData	*dp;
	register i;
	int	action = (int) client_data;
	Widget  popup = sf->propPopup;
	Setting *setting;
	Boolean	flag;

	switch (action) {

	case APPLY:
		ManipulateGizmo((GizmoClass)&ChoiceGizmoClass,
				&TypeChoice,
				GetGizmoValue);
		if (new)
			dp = new->pField;
		else
			dp = sf->flatItems[sf->currentItem].pField;

		ClearFooter(sf->footer); /* clear mesaage area */
		ClearLeftFooter(sf->sfooter);

		/* LATER: need to check the device avaiablability */
		
		if (VerifyAllFields() == INVALID) { /* something wrong */
			return;
		}
		/* needs to check for duplication */
		if(new) ApplyNewEntry();
		else {
			SetFields(dp);
			dp->changed = True;
			sf->changesMade = True;
			XtVaSetValues (
				sf->scrollingList,
				XtNviewHeight,          VIEWHEIGHT,
				XtNitemsTouched,	True,
				(String)0
			);
			/* Re-select the current item since we have */
                        /* given a new list                         */

			XtVaSetValues (
				sf->scrollingList,
				XtNviewItemIndex,	sf->currentItem,
				(String)0
			);
                        OlVaFlatSetValues (
                                sf->scrollingList,
                                sf->currentItem,
                                XtNset, True,
                                0
                        );
		}

		ManipulateGizmo((GizmoClass)&ChoiceGizmoClass,
				&TypeChoice,
				ApplyGizmoValue);
		BringDownPopup(popup);
		break;

	case RESET:
		if (new) {
			dp = new->pField;
		} else {
			dp = sf->flatItems[sf->currentItem].pField;
		}
		ResetFields(dp);
		ClearLeftFooter(sf->sfooter);
		break;

	case CANCEL:
		/* check if this entry is a newly added but net yet applied */
		if (new) {
			FreeNew();
		}
		XtPopdown(sf->propPopup);
		break;

	default:
		Warning("HandleButtonCB: Unknown action %d\n", action);
		break;
	}

	return;
} /* HandleButtonCB */

Boolean
ChangeCB (textField, which, tf_verify)
Widget	textField;
int	which;
OlTextFieldVerify       *tf_verify;
{
	char *		string;

	string = tf_verify->string;

	switch (which) { 
	case F_NAME:
#ifdef debug
		printf ("ChangeCB for name field\n");
#endif
		if (VerifyNameField(string) == INVALID)  {
			tf_verify->ok = False;
			return(INVALID);
		}
		break;
	case F_PHONE:
#ifdef debug
		printf ("ChangeCB for phone field\n");
#endif
		if (VerifyPhoneField(string) == INVALID) {
			tf_verify->ok = False;
			return(INVALID);
		}
		break;
	}
	return(VALID);
} /* ChangeCB */

/* Perform the form-wise checking */
Boolean
VerifyAllFields()
{
	char *	phone;

	if(VerifyNameField((XtPointer) OlTextFieldGetString
		((Widget) sf->w_name, NULL)) == INVALID) {
		return(INVALID);
	}
	if(VerifyPhoneField((XtPointer) OlTextFieldGetString
		((Widget) sf->w_phone, NULL)) == INVALID) {
		return(INVALID);
	}
	return(VALID);
}

/* String Ok: return VALID, otherwise INVALID */
Boolean
VerifyPhoneField(string)
String	string;
{
	int 	deviceType;
	static Boolean first_time = True;
	static char *badPhone, *blankPhone;

	if (first_time) {
		first_time = False;
		badPhone = GGT(string_badPhone);
		blankPhone = GGT(string_blankPhone);
	}
	if(strlen(string) == 0) {
		LeftFooterMsg(sf->sfooter, "%s", blankPhone);
		return (INVALID);
	}
	ManipulateGizmo((GizmoClass)&ChoiceGizmoClass,
				&TypeChoice,
				GetGizmoValue);
	deviceType = (int) stype.current_value;
	/* is TypeACU ? */
	if ( deviceType == TypeACU )
		if (strlen(string) != strspn(string, "0123456789=-*#")) {
			LeftFooterMsg(sf->sfooter, "%s", badPhone);
			return (INVALID);
		}
	else if ( deviceType == TypeDK )
		return(VerifyNameField(string));
	return (VALID);
} /* VerifyPhoneField */

/* String Ok: return VALID, otherwise INVALID */
Boolean
VerifyNameField(string)
String	string;
{
	static Boolean first_time = True;
	static char *badName, *blankName;

	if (first_time) {
		first_time = False;
		badName = GGT(string_badName);
		blankName = GGT(string_blankName);
	}
	if(strlen(string) == 0) {
		LeftFooterMsg(sf->sfooter, "%s", blankName);
		return (INVALID);
	}
	if(isdigit(string[0]) || (strchr(string, ' ') != NULL)) {
		LeftFooterMsg(sf->sfooter, "%s", badName);
		return (INVALID);
	}
	return (VALID);
} /* VerifyNameField */

/* ModifyCB
 *
 * Textfield modify callback.  As characters are types, check for white
 * space an non-printable characters.  Throw these out.  Also disallow
 * the wild-card characters * and ?.
 * call_data is a pointer to OlTextModifyCallData.
 * client_data is the footer widget to display the message.
 */
void
ModifyCB (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
    OlTextModifyCallDataPointer		tf_verify;
    char	*cp;
    register		i;
    static Boolean	first_time = True;
    static char		*format;

    if (first_time)
    {
	first_time = False;

	format = GGT (string_invalidChar);
    }

    ClearLeftFooter(sf->sfooter);
    tf_verify = (OlTextModifyCallDataPointer) call_data;
    for (cp=tf_verify->text, i=tf_verify->text_length; --i>=0; cp++)
    {
	if (!isgraph (*cp) || *cp == '?')
	{
	    char	msg [256];
	    sprintf (msg, format, *cp);
	    LeftFooterMsg ((Widget) client_data, "%s", msg);

	    tf_verify->ok = False;
	    return;
	}
    }
} /* ModifyCB () */
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

static void
ModemCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static Boolean first_time = True;
	static char	*phone_label;

	if (first_time == True) {
		first_time = False;
		phone_label = GGT(label_phone);
	}
	SetValue( sf->w_phone,
		XtNstring,		"",
		NULL);
	SetValue( w_phone,
		XtNmappedWhenManaged,	True,
		NULL);
	SetValue( w_phone, XtNlabel, phone_label, NULL);
} /* ModemCB */

static void
DirectCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SetValue( sf->w_phone,
		XtNstring,		"direct",
		NULL);
	SetValue( w_phone,
		XtNmappedWhenManaged,	False,
		NULL);
} /* DirectCB */

static void
DatakitCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	HostData	*dp;
	static Boolean first_time = True;
	static char	*destination_label;

	if (first_time == True) {
		first_time = False;
		destination_label = GGT(label_destination);
	}

	dp = sf->flatItems[sf->currentItem].pField;

	SetValue( sf->w_phone,
		XtNstring, (new) ? OlTextFieldGetString((Widget)sf->w_name, NULL)
					:dp->f_name,
		NULL);
	SetValue( w_phone,
		XtNmappedWhenManaged,	True,
		NULL);
	SetValue( w_phone, XtNlabel, destination_label, NULL);
} /* DatakitCB */

void
SetFields(hp)
HostData	*hp;
{
	int	index;
	XtFree (hp->f_name);
	XtFree (hp->f_type);
	XtFree (hp->f_class);
	XtFree (hp->f_expect1);
	XtFree (hp->f_passwd);
	XtFree (hp->f_expect2);
	hp->f_name = OlTextFieldGetString ((Widget)sf->w_name, NULL);
	hp->f_class = strdup((String) PortSpeed(GET, 0));
	hp->f_expect1 = OlTextFieldGetString ((Widget)sf->w_expect1, NULL);
	hp->f_passwd = OlTextFieldGetString ((Widget)sf->w_passwd, NULL);
	hp->f_expect2 = OlTextFieldGetString ((Widget)sf->w_expect2, NULL);
	XtFree (hp->f_phone);
	hp->f_phone = OlTextFieldGetString ((Widget)sf->w_phone, NULL);
	index = (int) stype.current_value;
	hp->f_type = strdup((char *)support_type[index].value);
} /* SetFields */

void
ResetFields(hp)
HostData	*hp;
{
	char buf[BUFSIZ];
	register i;
	int	 index;

	SetValue (
		sf->w_name,
		XtNstring, hp->f_name
	);
	PortSpeed(SET, hp->f_class);
	SetValue (
		sf->w_phone,
		XtNstring, hp->f_phone
	);
	SetValue (
		sf->w_expect1,
		XtNstring, hp->f_expect1
	);
	SetValue (
		sf->w_expect2,
		XtNstring, hp->f_expect2
	);
	SetValue (
		sf->w_passwd,
		XtNstring, hp->f_passwd
	);
	for (i=0; i< XtNumber(support_type); i++)
		if (!strcmp(support_type[i].value, hp->f_type))
			break;
	if (i == XtNumber(support_type)) {
		index = 0;
		sprintf (buf, "entry has unknown type; set it to Modem type");
		XtVaSetValues (sf->footer, XtNstring, buf, (String)0);
	} else  index = i;
	stype.previous_value = (XtPointer)index;
	SelectedType(hp);
	if (XtIsRealized(sf->propPopup)) {
		ManipulateGizmo (
			(GizmoClass)&ChoiceGizmoClass,
			&TypeChoice,
			ResetGizmoValue
		);
	}
} /* ResetFields */

static void
SelectedType(dp)
HostData	*dp;
{
	char buf[BUFSIZ];
	register i;
	int	 index;
	static Boolean first_time = True;
	static char	*phone_label;
	static char	*destination_label;

	if (first_time == True) {
		first_time = False;
		phone_label = GGT(label_phone);
		destination_label = GGT(label_destination);
	}

	for (i=0; i< XtNumber(support_type); i++)
		if (!strcmp(support_type[i].value, dp->f_type))
			break;
	if (i == XtNumber(support_type)) {
		index = 0;
		sprintf (buf, "entry has unknown type; set it to Modem type");
		XtVaSetValues (sf->footer, XtNstring, buf, (String)0);
	} else  index = i;

	XtSetArg (arg[0], XtNset, TRUE);
	OlFlatSetValues(
		sf->w_type,
		index,   /* sub-item to modify */
		arg,
		1
	);
	SetValue(w_phone,
		XtNmappedWhenManaged,
		(XtArgVal)(strcmp(dp->f_type, support_type[TypeDirect].value) != 0)
		);
	if (strcmp(dp->f_type, support_type[TypeACU].value) == 0)
		SetValue( w_phone, XtNlabel, phone_label, NULL);
	if (strcmp(dp->f_type, support_type[TypeDK].value) == 0)
		SetValue( w_phone, XtNlabel, destination_label, NULL);
} /* SelectedType */
