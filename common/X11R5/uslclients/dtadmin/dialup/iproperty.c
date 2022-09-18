/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/iproperty.c	1.14"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <libDtI/DtI.h>
#include <Gizmos.h>
#include "error.h"
#include "uucp.h"

DeviceData	deviceDefaults = {
	"com1",		/* portNumber */
	"hayes",	/* modemFamily */
	"Any",		/* portSpeed */
	"",		/* DTP */
};

extern char *	ApplicationName;

extern XtArgVal	GetValue();	/* get a single resource value */
extern void	SetValue();	/* set a single resource value */
extern void	Save();
extern void	AlignIcons();
extern void	Warning();
extern caddr_t	ModemFamily();
extern caddr_t	PortNumber();

extern DeviceItems     ports[];
extern DeviceItems     families[];
extern DeviceItems     speeds[];

void HandleButtonAction();

void
CBPopupPropertyWindow(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	XtPopup(df->propPopup, XtGrabNone);	/* popup the window */
	XRaiseWindow(DISPLAY, XtWindow(df->propPopup));
} /* CBPopupPropertyWindow */

void
CBUpdateContainer(wid, client_data, call_data)
Widget	wid;
XtPointer	client_data;
XtPointer	call_data;
{
        extern XFontStruct	*font;
	extern DmFclassPtr	acu_fcp, dir_fcp;
	extern DmObjectPtr	new_object();
	DmObjectPtr		op;
	DmItemPtr		ip;
	DeviceData		*tmp;
	Dimension		width;
	Cardinal		nitems;
	register		i;
	char			name[20];
	
	XtVaGetValues(df->iconbox,
		XtNnumItems, &nitems,
		0
	);
	width = GetValue(df->iconbox, XtNwidth);
#ifdef NITEMS
		fprintf(stderr, "In Creation(B4), nitems = %d\n", nitems);
		fprintf(stderr, "In Creation(B4), width = %d\n", width);
#endif
	tmp = (DeviceData *) XtMalloc(sizeof(DeviceData));
	if (tmp == NULL) {
		fprintf(stderr,
		"CBUpdateConatainer: couldn't XtMalloc an DeviceData\n");
		exit(1);
	}

	tmp->DTP = strdup("");
	tmp->holdModemFamily = tmp->modemFamily = strdup(holdData.modemFamily);
	tmp->holdPortSpeed = tmp->portSpeed = strdup(holdData.portSpeed);
	tmp->holdPortNumber = tmp->portNumber = strdup(holdData.portNumber);
	strcpy(name, tmp->portNumber);

	if (df->request_type == B_MOD) {
		for (i=0, ip = df->itp; i < nitems; i++, ip++) {
		    if(ITEM_MANAGED(ip) == False)
			    continue;
		    if( (op = (DmObjectPtr)OBJECT_PTR(ip)) == df->select_op) {
			/* found it */
			FreeDeviceData(op->objectdata);
			op->objectdata = tmp;
			free((char *)ip->label);
			ip->label = (XA)strdup(name);
			free(op->name);
			op->name = strdup(name);
			break;
		    }
		}
	} else {
		if ((op=new_object(name, tmp)) == (DmObjectPtr) OL_NO_ITEM) {
				return;
		}
	}
	if (!strcmp(tmp->modemFamily, "direct")
	    || !strcmp(tmp->modemFamily, "datakit"))
		op->fcp = dir_fcp;
	else
		op->fcp = acu_fcp;

	op->x = op->y = UNSPECIFIED_POS;
	if (df->request_type == B_MOD) {
		OlFlatRefreshItem(
			df->iconbox,
			i,
			True
		);
	} else {
		Dm__AddObjToIcontainer(df->iconbox, &(df->itp),
				&(nitems),
				df->cp, op,
				op->x, op->y, DM_B_NO_INIT | DM_B_CALC_SIZE,
				NULL,
				font,
				width,
				(Dimension)INC_X,
				(Dimension)INC_Y
				);
#ifdef NITEMS
		fprintf(stderr, "In Creation, nitems = %d\n", nitems);
		fprintf(stderr, "In Creation, width = %d\n", width);
#endif
		AlignIcons();
	}
} /* CBUpdateContainer */

void
CBUnbusyButton(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	SetValue((Widget) client_data, XtNbusy, FALSE);	/* unbusy button */
	return;
} /* CBUnbusyButton */

extern void		DisallowPopdown();
extern void             HelpCB();

static HelpText AppHelp = {
    title_property, HELP_FILE, help_dproperty,
};

static Items propertyItems [] = {
    { (XA)HandleButtonAction, NULL, (XA)TRUE, NULL, NULL, (XA)APPLY}, /* apply */
    { (XA)HandleButtonAction, NULL, (XA)TRUE, NULL, NULL, (XA)RESET}, /* reset */
    { (XA)HandleButtonAction, NULL, (XA)TRUE, NULL, NULL, (XA)CANCEL}, /* cancel */
    { (XA)HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&AppHelp},
};

static Menus propertyMenu = {
	"property",
	propertyItems,
	XtNumber (propertyItems),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

/*
 * Device Property Window routines: creating a Property Window and managing 
 * its fields.
 */
void
CreatePropertyWindow(w_parent)
Widget	w_parent;
{
	char 	buf[128];
	Widget	w_controlTop, w_controlBottom, w_footerPanel;

	static XtCallbackRec	reset[] = {
		{ (XtCallbackProc) HandleButtonAction, (caddr_t) RESET },
		{ NULL, NULL },
	};

	SET_HELP(AppHelp);
	sprintf(buf, "%s: %s", ApplicationName, GGT(title_dproperties));
	/*
	 * create the popup window widget
	 */
	df->propPopup = 
		XtVaCreatePopupShell(
			"deviceProperties",		/* instance name */
			popupWindowShellWidgetClass,	/* widget class */
			w_parent,			/* parent widget */
			XtNtitle,	buf,
			XtNpopdownCallback,	reset,
			0
		);
	XtAddCallback (
                df->propPopup,
                XtNverify,
                DisallowPopdown,
                (XtPointer)0
        );

	/*
	 * retrieve the widget IDs of the control areas and the footer panel
	 */
	XtVaGetValues(
		df->propPopup,
		XtNupperControlArea,	&w_controlTop,
		XtNlowerControlArea,	&w_controlBottom,
		XtNfooterPanel,		&w_footerPanel,
		0
	);

	/*
	 * Add property items to the popup.  Each object has its own 
	 * unique entry point where you send action messages such as 
	 * NEW, SET, and GET.  The Apply, Reset, ResetToFactory, and 
	 * Set Defaults actions are handled at this level, above 
	 * those objects.
	 */
	PortNumber(NEW, w_controlTop);
	ModemFamily(NEW, w_controlTop);
	SET_LABEL (propertyItems,0,apply);
	SET_LABEL (propertyItems,1,reset);
	SET_LABEL (propertyItems,2,cancel);
	SET_LABEL (propertyItems,3,help);
	AddMenu (w_controlBottom, &propertyMenu, False);
} /* CreatePropertyWindow */

/*
 * HandleButtonAction() - manages events that effect the state of the 
 * entire popup window.
 */
void HandleButtonAction(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	extern	void CBUpdateContainer();
	int	action = (int) client_data;

	switch (action) {

	case APPLY:
		ClearFooter(df->footer);
		if( holdData.portNumber != NULL )
		    XtFree(holdData.portNumber);
		holdData.portNumber = strdup((String) PortNumber(GET, 0));
		if (strcmp(holdData.portNumber, "other") == 0) {
		/* not com1, not com2; it is other */
			XtFree(holdData.portNumber);
			holdData.portNumber = OlTextFieldGetString(df->w_extra, NULL);
			if (holdData.portNumber != NULL) {
				/* strip off leading spaces if any */
				while (*holdData.portNumber == ' ')
					holdData.portNumber++;
				if ((*holdData.portNumber) == NULL) {
					/*PUTMSG(GGT(string_noDevice));*/
					return;
				}
			} else {
				/*PUTMSG(GGT(string_noDevice));*/
				return;
			}
		}
		if( holdData.modemFamily != NULL )
		    XtFree(holdData.modemFamily);
		holdData.modemFamily= strdup((String) ModemFamily(GET, 0));
		if (holdData.portSpeed == 0)
			holdData.portSpeed = strdup(deviceDefaults.portSpeed);
		CBUpdateContainer(wid, (caddr_t)0, (caddr_t)0);
		Save(wid, 0, 0);
		XtPopdown(df->propPopup);
		break;

	case RESET:
		/* if the request is B_ADD, then, it should fall through */
		if(df->request_type == B_MOD) {
		    PortNumber(SET,
			((DeviceData*)(df->select_op->objectdata))->holdPortNumber);
		    ModemFamily(SET,
			((DeviceData*)(df->select_op->objectdata))->holdModemFamily);
		    return;
		}

	case CANCEL:
		XtPopdown(df->propPopup);
		break;

	case RESETFACTORY:
		PortNumber(SET,	deviceDefaults.portNumber);
		ModemFamily(SET, deviceDefaults.modemFamily);
		break;

	default:
		Warning("HandleButtonAction: Unknown action %d\n", action);
		break;
	}

	return;
} /* HandleButtonAction */

void
CBselect(wid, client_data, call_data)
Widget  wid;
caddr_t client_data;
caddr_t call_data;
{
        OlFlatCallData *fcd = (OlFlatCallData *) call_data;
        String			label;

#ifdef debug
        /*
         * Get the resource value from the sub-object
         */
        OlVaFlatGetValues(
                wid,                            /* widget */
                fcd->item_index,  /* item_index */
		XtNlabel,       &label,
		0
        );

        fprintf(stderr,"set %s\n", label);
#endif
	if (wid == df->w_modem)
	    ModemFamily(SELECT,&families[fcd->item_index]);
	else if (wid == df->w_port)
	    PortNumber(SELECT,&ports[fcd->item_index]);
	else if (wid == sf->w_class)
	    PortSpeed(SELECT,&speeds[fcd->item_index]);
        return;
} /* CBselect */

/*
 * CBunselect() - use direct access to sub-object resources.  (The sub-object
 * data is actually maintained here in the application.)
 */
void
CBunselect(wid, client_data, call_data)
Widget  wid;
caddr_t client_data;
caddr_t call_data;
{
        OlFlatCallData  *fcd = (OlFlatCallData *) call_data;

#ifdef debug
        fprintf(stderr,
                "unset = %s\n",
		(wid == df->w_modem)
                ?families[fcd->item_index].label
		:ports[fcd->item_index].label
        );
#endif
        return;
} /*CBunselect */

