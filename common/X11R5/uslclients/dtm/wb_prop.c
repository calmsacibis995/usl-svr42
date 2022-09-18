/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:wb_prop.c	1.105"

/******************************file*header********************************

    Description:
     This file contains the source code for the wastebasket's Properties
	window.
*/
                              /* #includes go here     */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/IntegerFie.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
#include <Xol/Modal.h>
#include <Xol/StaticText.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static void	DisplayMsg(char * message, Boolean important);
static void	CleanUpCB(Widget, XtPointer, XtPointer); 
static void	UnitCB(Widget, XtPointer, XtPointer); 
static void	DelCurFiles(Widget parent, int ninterval,
				unsigned long ntime_unit);
static void	DoDelCurFiles(Widget, XtPointer, XtPointer);
static void	SavePropValues();
static void	VerifyPopdnCB(Widget, XtPointer, XtPointer);
static void	PopdownCB(Widget, XtPointer, XtPointer);
static void	ApplyCB(Widget, XtPointer, XtPointer);
static void	ResetCB(Widget, XtPointer, XtPointer);
static void	FactoryCB(Widget, XtPointer, XtPointer);
static void	CancelCB(Widget, XtPointer, XtPointer);
static void	HelpCB(Widget, XtPointer, XtPointer); 


/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

typedef struct {
	unsigned int   cleanUpMethod;
	unsigned int   interval;
	unsigned int   unit_idx;
	unsigned long  tm_interval;
	unsigned long  time_unit;
	char           *time_str;
} CurValRec, *CurValPtr;

static unsigned long timer_unit[]  = { MINUNIT, HOURUNIT, DAYUNIT };
static char *timer_strs[6];

static Widget	prop_shell = NULL;
static Widget	unit_excl;
static Widget	cleanup_method;
static Widget	tf;
static Widget	tf_caption;
static Widget	footer;
static Boolean	popdown;
static Boolean first = True;
static CurValPtr tvp;

/* for registering context-sensitive help */
static OlDtHelpInfo help_info;

/* Define gizmo for delete files notice */

static MenuItems menubarItems[] = {
    MENU_ITEM( TXT_YES,		TXT_M_YES,	NULL ),
    MENU_ITEM( TXT_NO,		TXT_M_NO,	NULL ),
    MENU_ITEM( TXT_P_HELP,	TXT_M_HELP,	NULL ),
    { NULL }			/* NULL terminated */
};

MENU_BAR("delfileNoticeMenubar", menubar, DoDelCurFiles, 1); /* default: No */

static HelpInfo DelFileWinHelp =
{ TXT_WB_TITLE, NULL, "DesktopMgr/wb.hlp", "300", NULL };

static ModalGizmo delfileGizmo = {
	&DelFileWinHelp,   /* help info          */
	"delfileNotice",   /* shell name         */
	TXT_WB_TITLE,      /* title              */
	&menubar,          /* menu               */
	TXT_DELETE_NOW,    /* message            */
	NULL, 0,           /* gizmos, num_gizmos */
};

/***************************public*procedures****************************

    Public Procedures
*/


/****************************procedure*header*****************************
 * Creates and displays the Wastebasket properties window.  Invoked when
 * Set Properties button in Actions menu is selected.
 */
void
DmWBPropCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	Widget parent;
	Widget uca;
	Widget lca;
	Widget fpanel;
	Widget cleanup_caption;
	Widget buttons;
	char   buf[256];
	XtArgVal  *btn_items;
	XtArgVal  *method_items;
	XtArgVal  *unit_items;
	XtArgVal  *p;

	static char *btn_fields[] = {
       XtNlabel, XtNmnemonic, XtNselectProc, XtNdefault, XtNclientData
	};

	static char *exc_fields[] = {
       XtNlabel, XtNmnemonic, XtNset, XtNdefault, XtNclientData
	};

	if (prop_shell != NULL) {
		XtMapWidget(prop_shell);
		XRaiseWindow(XtDisplay(prop_shell), XtWindow(prop_shell));
		return;
	}

	if (first) {
		/* get strings for timer units and save them */
		timer_strs[0] = Dm__gettxt(TXT_MINUTE_STR);
		timer_strs[1] = Dm__gettxt(TXT_HOUR_STR);
		timer_strs[2] = Dm__gettxt(TXT_DAY_STR);
		timer_strs[3] = Dm__gettxt(TXT_MINUTES_STR);
		timer_strs[4] = Dm__gettxt(TXT_HOURS_STR);
		timer_strs[5] = Dm__gettxt(TXT_DAYS_STR);
		first = False;
	}

	tvp = (CurValPtr)MALLOC(sizeof(CurValRec));
	tvp->cleanUpMethod = wbdp->cleanUpMethod;
	tvp->interval      = wbdp->interval;
	tvp->unit_idx      = wbdp->unit_idx;
	tvp->tm_interval   = wbdp->tm_interval;
	tvp->time_unit     = wbdp->time_unit;

	if (wbdp->interval == 1)
		tvp->time_str = wbdp->time_str = timer_strs[wbdp->unit_idx];
	else
		tvp->time_str = wbdp->time_str = timer_strs[wbdp->unit_idx + 3];

	XtSetArg(Dm__arg[0], XtNtitle, Dm__gettxt(TXT_WB_PROP_WIN_TITLE));
	prop_shell = XtCreatePopupShell("prop_shell",
                        popupWindowShellWidgetClass, w, Dm__arg, 1);

	XtAddCallback(prop_shell, XtNverify, VerifyPopdnCB, NULL);
	XtAddCallback(prop_shell, XtNpopdownCallback, PopdownCB, NULL);

	XtSetArg(Dm__arg[0], XtNupperControlArea, (XtArgVal)&uca);
	XtSetArg(Dm__arg[1], XtNlowerControlArea, (XtArgVal)&lca);
	XtSetArg(Dm__arg[2], XtNfooterPanel,      (XtArgVal)&fpanel);
	XtGetValues(prop_shell, Dm__arg, 3);

	XtSetArg(Dm__arg[0], XtNposition,  OL_LEFT);
	XtSetArg(Dm__arg[1], XtNalignment, OL_CENTER);
	XtSetArg(Dm__arg[2], XtNlabel,     Dm__gettxt(TXT_WB_METHOD));
	cleanup_caption = XtCreateManagedWidget("cleanup_caption",
                                captionWidgetClass, uca, Dm__arg, 3);

	method_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 5 * 4);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_WB_BY_TIMER));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_WB_BY_TIMER);
	*p++ = (XtArgVal)WB_BY_TIMER(wbdp);
	*p++ = (XtArgVal)True;
	*p++ = NULL;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_WB_ON_EXIT));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_WB_ON_EXIT);
	*p++ = (XtArgVal)WB_ON_EXIT(wbdp);
	*p++ = (XtArgVal)False;
	*p++ = NULL;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_WB_IMMEDIATELY));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_WB_IMMEDIATELY);
	*p++ = (XtArgVal)WB_IMMEDIATELY(wbdp);
	*p++ = (XtArgVal)False;
	*p++ = NULL;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_WB_NEVER));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_WB_NEVER);
	*p++ = (XtArgVal)WB_NEVER(wbdp);
	*p++ = (XtArgVal)False;
	*p++ = NULL;

	XtSetArg(Dm__arg[0], XtNitems,        (XtArgVal)method_items);
	XtSetArg(Dm__arg[1], XtNnumItems,     (XtArgVal)4);
	XtSetArg(Dm__arg[2], XtNitemFields,   (XtArgVal)exc_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields,(XtArgVal)XtNumber(exc_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,   (XtArgVal)OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,      (XtArgVal)1);
	XtSetArg(Dm__arg[6], XtNbuttonType,   (XtArgVal)OL_RECT_BTN);
	XtSetArg(Dm__arg[7], XtNexclusives,   (XtArgVal)True);
	XtSetArg(Dm__arg[8], XtNnoneSet,      (XtArgVal)False);
	XtSetArg(Dm__arg[9], XtNselectProc,   (XtArgVal)CleanUpCB);

	cleanup_method = XtCreateManagedWidget("cleanup_method",
					flatButtonsWidgetClass, cleanup_caption,
					Dm__arg, 10);

	XtSetArg(Dm__arg[0], XtNposition,  OL_LEFT);
	XtSetArg(Dm__arg[1], XtNalignment, OL_CENTER);
	XtSetArg(Dm__arg[2], XtNlabel,     Dm__gettxt(TXT_WB_REM_AFTER));
	XtSetArg(Dm__arg[3], XtNsensitive, WB_BY_TIMER(wbdp));

	tf_caption = XtCreateManagedWidget("caption",
                                captionWidgetClass, uca, Dm__arg, 4);

	XtSetArg(Dm__arg[0], XtNshadowThickness, 0);
	parent = XtCreateManagedWidget("foo", controlAreaWidgetClass,
				   tf_caption, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNcharsVisible,     5);
	XtSetArg(Dm__arg[1], XtNvalue,            wbdp->interval);
	XtSetArg(Dm__arg[2], XtNvalueMin,         1);
	XtSetArg(Dm__arg[3], XtNvalueMax,		  44640);
	XtSetArg(Dm__arg[4], XtNvalueGranularity, 1);
	tf = XtCreateManagedWidget("tf", integerFieldWidgetClass,
					parent, Dm__arg, 5);

	unit_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 5 * 3);
	*p++ = (XtArgVal)(Dm__gettxt(TXT_WB_U_MINUTE));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_WB_U_MINUTE);
	*p++ = (XtArgVal)(wbdp->unit_idx == 0 ? True : False);
	*p++ = (XtArgVal)True;
	*p++ = NULL;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_WB_U_HOUR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_WB_U_HOUR);
	*p++ = (XtArgVal)(wbdp->unit_idx == 1 ? True : False);
	*p++ = (XtArgVal)False;
	*p++ = NULL;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_WB_U_DAY));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_WB_U_DAY);
	*p++ = (XtArgVal)(wbdp->unit_idx == 2 ? True : False);
	*p++ = (XtArgVal)False;
	*p++ = NULL;

	XtSetArg(Dm__arg[0], XtNitems,         unit_items);
	XtSetArg(Dm__arg[1], XtNnumItems,      3);
	XtSetArg(Dm__arg[2], XtNitemFields,    exc_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(exc_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNbuttonType,    OL_RECT_BTN);
	XtSetArg(Dm__arg[7], XtNexclusives,    True);
	XtSetArg(Dm__arg[8], XtNnoneSet,       False);
	XtSetArg(Dm__arg[9], XtNselectProc,    UnitCB);

	unit_excl = XtCreateManagedWidget("unit_excl", flatButtonsWidgetClass,
				parent, Dm__arg, 10);

	btn_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 5 * 5);
	*p++ = (XtArgVal)(Dm__gettxt(TXT_APPLY_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_APPLY_STR);
	*p++ = (XtArgVal)ApplyCB;
	*p++ = (XtArgVal)True;
	*p++ = (XtArgVal)prop_shell;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_RESET_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_RESET_STR);
	*p++ = (XtArgVal)ResetCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)prop_shell;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_FACTORY_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_FACTORY_STR);
	*p++ = (XtArgVal)FactoryCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)prop_shell;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_CANCEL_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_CANCEL_STR);
	*p++ = (XtArgVal)CancelCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)prop_shell;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HELP_ELLIPSIS));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HELP_STR);
	*p++ = (XtArgVal)HelpCB;
	*p++ = (XtArgVal)False;
	*p++ = NULL;

	XtSetArg(Dm__arg[0], XtNitems,         btn_items);
	XtSetArg(Dm__arg[1], XtNnumItems,      5);
	XtSetArg(Dm__arg[2], XtNitemFields,    btn_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(btn_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNclientData,    prop_shell);
	
	buttons = XtCreateManagedWidget("buttons", flatButtonsWidgetClass,
				lca, Dm__arg, 7);

	/* display current clean up settings */
	if (WB_BY_TIMER(wbdp))
		sprintf(buf, "%s %d %s.", Dm__gettxt(TXT_WB_BY_TIMER_MSG),
			wbdp->interval, wbdp->time_str);
	else if (WB_IMMEDIATELY(wbdp))
			sprintf(buf, "%s", Dm__gettxt(TXT_WB_IMMEDIATE_MSG));
	else if (WB_ON_EXIT(wbdp))
			sprintf(buf, "%s", Dm__gettxt(TXT_WB_ON_EXIT_MSG));
	else
			sprintf(buf, "%s", Dm__gettxt(TXT_WB_NEVER_MSG));

	XtSetArg(Dm__arg[0], XtNgravity, (XtArgVal)WestGravity);
	XtSetArg(Dm__arg[1], XtNstring,  (XtArgVal)buf);

	footer = XtCreateManagedWidget("footer", staticTextWidgetClass,
			fpanel, Dm__arg, 2);

	XtPopup(prop_shell, XtGrabNone);

	/* register help for window */
	help_info.app_title = Dm__gettxt(TXT_WASTEBASKET);
	help_info.filename  = "DesktopMgr/wb.hlp";
	help_info.section   = "250";
	help_info.path      = NULL;
	help_info.title     = NULL;

	OlRegisterHelp(OL_WIDGET_HELP, prop_shell, NULL, OL_DESKTOP_SOURCE,
		(XtPointer)&help_info);

} /* end of DmWBPropCB */

/****************************procedure*header*****************************
 * Applies new properties and update appropriate resources in .Xdefaults.
 */
static void
ApplyCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	char *	str;
	char	buf[256];

	popdown = True;		/* Assume popup can be popped down */

	XtSetArg(Dm__arg[0], XtNstring, (XtArgVal)&str);
	XtGetValues(tf, Dm__arg, 1);

	if (strchr(str, '-')) {
		DisplayMsg(Dm__gettxt(TXT_NOT_POS_INT), True);
		popdown = False;
		return;
	}

	(void)sscanf(str, "%d", &(tvp->interval));
	sprintf(buf, "%d", tvp->interval);

	if (tvp->interval <= 0) {
		DisplayMsg(Dm__gettxt(TXT_INTERVAL_LE0), True);
		popdown = False;
		return;
	}

	switch(tvp->cleanUpMethod) {
	case WBByTimer:
		if (strlen(str) == 0) {
			DisplayMsg(Dm__gettxt(TXT_NO_INTERVAL), True);
			popdown = False;
			return;
		}

		if ((tvp->interval > 31 && tvp->unit_idx == 2) ||
		    (tvp->interval > 744 && tvp->unit_idx == 1) ||
		    (tvp->interval > 44640 && tvp->unit_idx == 0)) {

			DisplayMsg(Dm__gettxt(TXT_GT31_DAYS), True);
			popdown = False;
			return;
		}

		if (tvp->interval == 1)
			tvp->time_str = timer_strs[tvp->unit_idx];
		else
			tvp->time_str = timer_strs[tvp->unit_idx + 3];

		wbdp->cleanUpMethod = WBByTimer;
		wbdp->interval  = tvp->interval;
		wbdp->time_str  = tvp->time_str;
		wbdp->time_unit = tvp->time_unit;
		wbdp->unit_idx  = tvp->unit_idx;

		/*
		 * Prompt user to delete files which are now "eligible" for
		 * deletion only if timer is not suspended and the wastebasket
		 * is not empty.
		 */
		if (!wbdp->suspend && !WB_IS_EMPTY(Desktop) &&
		    ((unsigned long)
		     (tvp->interval * tvp->time_unit) != tvp->tm_interval))
		{
		    DelCurFiles(w, tvp->interval, tvp->time_unit);
		}

		DmWBRestartTimer();
		sprintf(buf, "%s %d %s.", Dm__gettxt(TXT_WB_BY_TIMER_MSG),
			tvp->interval, tvp->time_str);
		DisplayMsg(buf, False);

		break;

	case WBOnExit:
		wbdp->cleanUpMethod  = WBOnExit;
		DisplayMsg(Dm__gettxt(TXT_WB_ON_EXIT_MSG), False);
		break;

	case WBImmediately:
		wbdp->cleanUpMethod  = WBImmediately;
		DisplayMsg(Dm__gettxt(TXT_WB_IMMEDIATE_MSG), False);

		if (!WB_IS_EMPTY(Desktop))
			DelCurFiles(w, 0, tvp->time_unit);

		break;

	case WBNever:
		wbdp->cleanUpMethod  = WBNever;
		DisplayMsg(Dm__gettxt(TXT_WB_NEVER_MSG), False);
		break;
	}

	if (wbdp->cleanUpMethod != 0) {
		/* By Timer not selected, turn off timer. */
		if (wbdp->timer_id != NULL) {
			XtRemoveTimeOut(wbdp->timer_id);
			wbdp->timer_id = NULL;
		}
		wbdp->interval = tvp->interval;
		wbdp->unit_idx = tvp->unit_idx;
		wbdp->restart  = False;
		DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop), NULL);

		/* make Resume/Suspend Timer in Actions menu insensitive */
		DmWBToggleTimerBtn(Dm__gettxt(TXT_SUSPEND_LBL),
			*Dm__gettxt(TXT_M_SUSPEND_LBL), False);
	}
#ifdef NOT_USE
	wbdp->cleanUpMethod = tvp->cleanUpMethod;
#endif
	/* save resources values in .Xdefaults file */
	SavePropValues();

} /* end of ApplyCB */

/****************************procedure*header*****************************
 * Resets property settings to the most recently Apply'ed settings.
 */
static void
ResetCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	char	buf[15];

	sprintf(buf, "%d", wbdp->interval);
	XtSetArg(Dm__arg[0], XtNstring, (XtArgVal)buf);
	XtSetValues(tf, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNset, (XtArgVal)True);
	OlFlatSetValues(unit_excl, wbdp->unit_idx, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNsensitive, WB_BY_TIMER(wbdp));
	XtSetValues(unit_excl, Dm__arg, 1);
	XtSetValues(tf, Dm__arg, 1);
	XtSetSensitive(tf_caption, WB_BY_TIMER(wbdp));

	XtSetArg(Dm__arg[0], XtNset, (XtArgVal)True);
	OlFlatSetValues(cleanup_method, wbdp->cleanUpMethod, Dm__arg, 1);

	tvp->time_str      = wbdp->time_str;
	tvp->interval      = wbdp->interval;
	tvp->time_unit     = wbdp->time_unit;
	tvp->unit_idx      = wbdp->unit_idx;
	tvp->cleanUpMethod = wbdp->cleanUpMethod;
	popdown        = False;

	DisplayMsg((char *)NULL, False);
} /* end of ResetCB */

/****************************procedure*header*****************************
 * Resets property settings to factory settings.
 */
static void
FactoryCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	char	buf[10];

	tvp->cleanUpMethod = 0;
	tvp->interval      = DEFAULT_WB_TIMER_INTERVAL;
	tvp->time_unit     = FTIMEUNIT;
	tvp->time_str      = Dm__gettxt(TXT_DAYS_STR);
	tvp->unit_idx      = 2;
	
	XtSetArg(Dm__arg[0], XtNsensitive, (XtArgVal)True);
	XtSetValues(unit_excl, Dm__arg, 1);
	XtSetValues(tf, Dm__arg, 1);
	XtSetSensitive(tf_caption, True);

	sprintf(buf, "%d", tvp->interval);
	XtSetArg(Dm__arg[1], XtNstring, (XtArgVal)buf);
	XtSetValues(tf, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNset, (XtArgVal)True);
	OlFlatSetValues(unit_excl, 2, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNset, (XtArgVal)True);
	OlFlatSetValues(cleanup_method, 0, Dm__arg, 1);

	popdown = False;
	DisplayMsg((char *)NULL, False);
} /* end of FactoryCB */

/****************************procedure*header*****************************
 * Callback for unit exclusives. 
 */
static void
UnitCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
        OlFlatCallData  *fcd = (OlFlatCallData *)call_data;

        tvp->time_unit = timer_unit[fcd->item_index];
        tvp->unit_idx  = fcd->item_index;
} /* end of UnitCB *

/****************************procedure*header*****************************
 * Displays messages in properties window.
 */
static void
DisplayMsg(char *message, Boolean important)
{
       if (message) {
                if (important)
                        _OlBeepDisplay(footer, 1);
        } else {
                message = "";
        }
        XtSetArg(Dm__arg[0], XtNstring, (XtArgVal)message);
        XtSetValues(footer, Dm__arg, 1);
} /* end of DisplayMsg */

/****************************procedure*header*****************************
 * Popdown verify callback.
 */
static void
VerifyPopdnCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
    Boolean *	pop_it_down = (Boolean *)call_data;

    if (!popdown && *pop_it_down)
	*pop_it_down = False;

} /* end of VerifyPopdnCB */

/****************************procedure*header*****************************
 * Called when window is closed/dismissed.
 */
static void
PopdownCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	if (prop_shell) {
		XtDestroyWidget(prop_shell);
		prop_shell = NULL;
	}
} /* end of PopdownCB */

/****************************procedure*header*****************************
 * Called when a change in timer settings results in files due for deletion.
 * Asks the user if he/she wants to delete those files now.
 */
static void
DelCurFiles(parent, ninterval, ntime_unit)
Widget        parent;
int           ninterval;
unsigned long ntime_unit;
{
	DmItemPtr itp;
	char      **src_list;
	char      **src;
	int       cnt;
	time_t    current_time;
	int       interval_unit;
	Boolean   reset = False;

     src = src_list = (char **)MALLOC(sizeof(char *) * NUM_WB_OBJS(Desktop));

     /* find out which files meet the deletion criterion, if any */
	if (ninterval > 0) {
		time(&current_time);

		/* convert from milliseconds to seconds */
		interval_unit = (ntime_unit) / 1000;
     }

     for (itp = DESKTOP_WB_WIN(Desktop)->itp;
	  itp < DESKTOP_WB_WIN(Desktop)->itp + DESKTOP_WB_WIN(Desktop)->nitems;
	  itp++) {

		if (ITEM_MANAGED(itp) && (WB_IMMEDIATELY(wbdp) ||
			(((current_time -
				atol(DmGetObjProperty(ITEM_OBJ(itp), TIME_STAMP, NULL))) /
				interval_unit) >= ninterval))) {

			/* Skip busy items and reset time stamp to current time
			 * if this function was called due to a change in timer
			 * settings.
			 */
			if (ITEM_BUSY(itp)) {
				if (!WB_IMMEDIATELY(wbdp)) {
					char buf[16];
					sprintf(buf, "%ld", current_time);
					DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP, buf,NULL);
					reset = True;
				}
				continue;
			}
			*src++ = strdup(ITEM_OBJ_NAME(itp));
		}
	}

	if (reset)
		DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->cp, wb_dtinfo, 0);

     if ( (cnt = src - src_list) == 0 )
     {
		/* No files to delete after all so change our minds and set
		   popdown to True.
		*/
		FREE((void *)src_list);
		popdown = True;
		return;
     }

     /* FIX THIS: This is a workaround for a bug in the Modal widget.
	Set the prop sheet busy so it doesn't get put on the Modal's popup
	busy list.
     */
     if (prop_shell != NULL)
     {
	 XtSetArg(Dm__arg[0], XtNbusy, True);
	 XtSetValues(prop_shell, Dm__arg, 1);
     }

     /* Create the modal gizmo */
     if (delfileGizmo.shell == NULL)
		CreateGizmo(XtParent(DESKTOP_WB_WIN(Desktop)->box),
			ModalGizmoClass, &delfileGizmo, NULL, 0);

     /* this must be done here because cnt and src_list
      * are different for every invocation of this function.
      */
     delfileGizmo.menu->client_data = (XtPointer)cnt;
     XtSetArg(Dm__arg[0], XtNuserData, src_list);
     XtSetValues(delfileGizmo.shell, Dm__arg, 1);

     MapGizmo(ModalGizmoClass, &delfileGizmo);

}				/* end of DelCurFiles */

/****************************procedure*header*****************************
 * Deletes files which are due for deletion due to a change in timer settings.
 */
static void
DoDelCurFiles(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
     DmWBCPDataPtr   cpdp;
     char            **src_list;
     DmFileOpInfoPtr opr_info;
     int             cnt;
     OlFlatCallData  *fcd = (OlFlatCallData *)call_data;

     if (fcd->item_index == 2) { /* Help was selected */
	DmHelpAppPtr   help_app = DmGetHelpApp(WB_HELP_ID(Desktop));

	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
		NULL, "DesktopMgr/wb.hlp", "300", UNSPECIFIED_POS, UNSPECIFIED_POS);
	XtAddGrab(help_app->hlp_win.shell, False, False);
	return;
     }

     cnt = (int)delfileGizmo.menu->client_data;
     XtSetArg(Dm__arg[0], XtNuserData, &src_list);
     XtGetValues(delfileGizmo.shell, Dm__arg, 1);
     XtPopdown(delfileGizmo.shell);

     /* If the prop sheet is still up (pinned), unbusy it */
     if (prop_shell != NULL)
     {
	 XtSetArg(Dm__arg[0], XtNbusy, False);
	 XtSetValues(prop_shell, Dm__arg, 1);
     }

     if (fcd->item_index == 1) { /* Cancel was selected */
		int i;

		for (i = 0; i < cnt; i++)
			FREE(src_list[i]);
		FREE((void *)src_list);
		return;
     }

     cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
     cpdp->op_type = DM_TIMERCHG;

     opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
     opr_info->type        = DM_DELETE;
     opr_info->options	  = 0;
     opr_info->target_path = NULL;
     opr_info->src_path	  = strdup(DM_WB_PATH(Desktop));
     opr_info->src_list	  = src_list;
     opr_info->src_cnt	  = cnt;
     opr_info->src_win	  = DESKTOP_WB_WIN(Desktop);
     opr_info->dst_win	  = NULL;
     opr_info->x	       = opr_info->y = UNSPECIFIED_POS;
     DmDoFileOp(opr_info, DmWBClientProc, (XtPointer)cpdp);

} /* end of DoDelCurFiles */

/****************************procedure*header*****************************
 * Displays help on the Wastebasket properties window.
 */
static void
HelpCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmHelpAppPtr   help_app = DmGetHelpApp(WB_HELP_ID(Desktop));

	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
		NULL, "DesktopMgr/wb.hlp", "250", UNSPECIFIED_POS, UNSPECIFIED_POS);
	popdown = False;

}	/* end of HelpCB */

/****************************procedure*header*****************************
 * Pops down the Wastebasket properties window. 
 */
static void
CancelCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	XtPopdown(prop_shell);
	popdown = True;

}	/* end of CancelCB */

/****************************procedure*header*****************************
 * Called when the Clean Up Method exclusives is selected.
 */
static void
CleanUpCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData  *fcd = (OlFlatCallData *)call_data;
	Boolean		sensitive = (fcd->item_index == 0);

	tvp->cleanUpMethod = fcd->item_index;

	XtSetSensitive(unit_excl, sensitive);
	XtSetSensitive(tf_caption, sensitive);

}	/* end of CleanUpCB */

/****************************procedure*header*****************************
 * Save current wastebasket property settings in .Xdefaults file.
 */
static void
SavePropValues()
{
	extern void MergeResources();
	char buf[512];

	sprintf(buf,
	    "dtm.wbSuspend:%d\ndtm.wbCleanUpMethod:%d\ndtm.wbTimerInterval:%d\n"
	    "dtm.wbTimerUnit:%d\n", wbdp->suspend, wbdp->cleanUpMethod,
		wbdp->interval, wbdp->unit_idx);

    MergeResources(buf);
}					/* end of SavePropValues */
