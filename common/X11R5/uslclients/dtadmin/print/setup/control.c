/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/control.c	1.11"
#endif

/* Enable/Disable printers */

#include <stdio.h>
#include <string.h>
#include <search.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/PopupWindo.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/FList.h>

#include <lp.h>

#include "properties.h"
#include "error.h"
#include "lpsys.h"

static void	StatusApplyCB (Widget, XtPointer, XtPointer);
static void	StatusResetCB (Widget, XtPointer, XtPointer);
extern void	CancelCB (Widget, XtPointer, XtPointer);
static void	WhenCB (Widget, XtPointer, XtPointer);
static void	StatusPopdownCB (Widget, XtPointer, XtPointer);
extern void	ConfirmCB (Widget, XtPointer, XtPointer);

static void	CreateStatusWindow (Widget);
static void	CreateWhenPopup (void);
static void	UpdateStatus (LpState, LpState, LpWhen);
static void	StatusFooterMsg (char *);

enum {
    Accept_Button, Reject_Button,
};

enum {
    Enable_Button, Disable_Button,
};

static HelpText CtrlHelp = {
    TXT_ctrlHelp, HELP_FILE, TXT_ctrlHelpSect,
};

/* Lower Control Area buttons */
static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_apply, (XtArgVal) MNEM_apply, (XtArgVal) True,
	  (XtArgVal) StatusApplyCB, (XtArgVal) True, },	/* Apply */
    { (XtArgVal) TXT_reset, (XtArgVal) MNEM_reset, (XtArgVal) True,
	  (XtArgVal) StatusResetCB, },			/* Reset */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &CtrlHelp, },			/* Help */
};

static MenuItem WhenItems [] = {
    { (XtArgVal) TXT_requeue, (XtArgVal) MNEM_requeue, (XtArgVal) True,
	  (XtArgVal) WhenCB, (XtArgVal) True, },	/* Requeue */
    { (XtArgVal) TXT_complete, (XtArgVal) MNEM_complete, (XtArgVal) True,
	  (XtArgVal) WhenCB, },				/* Complete */
    { (XtArgVal) TXT_delete, (XtArgVal) MNEM_delete, (XtArgVal) True,
	  (XtArgVal) WhenCB, },				/* Delete */
};

static ButtonItem AcceptItems [] = {
    { (XtArgVal) Lp_On, (XtArgVal) TXT_accept, },	/* Accept */
    { (XtArgVal) Lp_Off, (XtArgVal) TXT_reject, },	/* Reject */
};

static ButtonItem EnableItems [] = {
    { (XtArgVal) Lp_On, (XtArgVal) TXT_enable, },	/* Enable */
    { (XtArgVal) Lp_Off, (XtArgVal) TXT_disable, },	/* Disable */
};

PrinterStatus PrtStatus;

/* PrtControl
 *
 * Post property sheet for printer indicating if the printer is enable or
 * disabled and if it is accepting or rejecting jobs.  If the sheet is
 * already posted, raise it to the top.  Even if the window is already
 * posted for the selected printer, go ahead and "reset" the data to make
 * sure it reflects the current state of the world. 
 */
void
PrtControl (Widget widget, PropertyData *properties)
{
    if (properties == PrtStatus.owner)
	XRaiseWindow (XtDisplay (PrtStatus.popupWindow),
		      XtWindow (PrtStatus.popupWindow));
    else
    {
	if (!PrtStatus.popupWindow)
	    CreateStatusWindow (widget);
    }

    PrtStatus.owner = properties;
    StatusResetCB (widget, 0, 0);

    if (!PrtStatus.poppedUp)
    {
	XtPopup (PrtStatus.popupWindow, XtGrabNone);
	PrtStatus.poppedUp = True;
    }
}	/* End of PrtControl () */

/* CreateStatusWindow
 *
 * Create a popup window for printer status and control.
 */
static void
CreateStatusWindow (Widget parent)
{
    Widget		uca;
    Widget		lca;
    Widget		lcaMenu;
    Widget		ucaMenuShell;
    Widget		ucaMenu;
    Widget		footer;
    static char		*titleLbl;
    static char		*acceptLbl;
    static char		*enableLbl;
    static Boolean	first = True;

    /* Set Labels */
    if (first)
    {
	first = False;
	titleLbl = GetStr (TXT_statusTitle);
	acceptLbl = GetStr (TXT_acceptCap);
	enableLbl = GetStr (TXT_enableCap);

	SetLabels (CommandItems, XtNumber (CommandItems));
	SetButtonLbls (AcceptItems, XtNumber(AcceptItems));
	SetButtonLbls (EnableItems, XtNumber(EnableItems));
	SetHelpLabels (&CtrlHelp);

	if (!IsAdmin ())
	    CommandItems [Apply_Button].sensitive = (XtArgVal) False;
    }

    /* Create property sheet */
    PrtStatus.popupWindow = XtVaCreatePopupShell ("status",
		popupWindowShellWidgetClass, parent,
		XtNtitle,		(XtArgVal) titleLbl,
		0);

    XtVaGetValues (PrtStatus.popupWindow,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		XtNfooterPanel,		(XtArgVal) &footer,
		0);

    /* We want an "apply" and "reset" buttons in both the lower control
     * area and in a popup menu on the upper control area.
     */
    lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    ucaMenuShell = XtVaCreatePopupShell ("ucaMenuShell",
		popupMenuShellWidgetClass, uca,
		0);

    ucaMenu = XtVaCreateManagedWidget ("ucaMenu",
		flatButtonsWidgetClass, ucaMenuShell,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    OlAddDefaultPopupMenuEH (uca, ucaMenuShell);

    PrtStatus.footer = XtVaCreateManagedWidget ("footer",
		staticTextWidgetClass, footer,
		0);

    XtAddCallback (PrtStatus.popupWindow, XtNverify, VerifyCB, (XtPointer) 0);
    XtAddCallback (PrtStatus.popupWindow, XtNpopdownCallback, StatusPopdownCB,
		   (XtPointer) 0);

    PrtStatus.state = XtVaCreateManagedWidget ("statusTxt",
		staticTextWidgetClass, uca,
		0);

    MakeButtons (uca, acceptLbl, AcceptItems, XtNumber (AcceptItems),
		 &PrtStatus.acceptCtrl);

    MakeButtons (uca, enableLbl, EnableItems, XtNumber (EnableItems),
		 &PrtStatus.enableCtrl);

}	/* End of CreateStatusWindow () */

/* StatusApplyCB
 *
 * Update property sheet to reflect the changed values.  Because these
 * sorts of properties are volatile, they are not actually saved in the
 * properties structure.
 */
static void
StatusApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropertyData	*properties = PrtStatus.owner;
    LpState		accept;
    LpState		enable;
    static Boolean	first = True;
    static char		*noneSelectedMsg;

    if (first)
    {
	first = False;

	noneSelectedMsg = GetStr (TXT_noneSelected);
    }

    if (!properties)
    {
	StatusFooterMsg (noneSelectedMsg);
	return;
    }

    StatusFooterMsg (NULL);

    /* Update lp. */
    if ((properties->accepting ? Accept_Button : Reject_Button) !=
	PrtStatus.acceptCtrl.setIndx)
	accept = (LpState) AcceptItems [PrtStatus.acceptCtrl.setIndx].userData;
    else
	accept = Lp_No_Change;

    if ((properties->enabled ? Enable_Button : Disable_Button) !=
	PrtStatus.enableCtrl.setIndx)
	enable = (LpState) EnableItems[PrtStatus.enableCtrl.setIndx].userData;
    else
	enable = Lp_No_Change;

    if (enable == Lp_Off && properties->activeJob)
    {
	if (!PrtStatus.whenPopupWindow)
	    CreateWhenPopup ();

	PrtStatus.pendingAccept = accept;
	XtPopup (PrtStatus.whenPopupWindow, XtGrabExclusive);
    }
    else
	UpdateStatus (accept, enable, Lp_Requeue);
}	/* End of StatusApplyCB () */

/* StatusResetCB
 *
 * Get the current state of the printer.
 *
 */
static void
StatusResetCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropertyData	*properties = PrtStatus.owner;
    char		statusLine [80];
    static Boolean	first = True;
    static char		*printingMsg;
    static char		*idleMsg;
    static char		*faultedMsg;
    static char		*disabledMsg;
    static char		*noStatusMsg;

    if (first)
    {
	first = False;

	idleMsg = GetStr (TXT_idlePrintf);
	printingMsg = GetStr (TXT_printingPrintf);
	faultedMsg = GetStr (TXT_faultedPrintf);
	disabledMsg = GetStr (TXT_disabledPrintf);
	noStatusMsg = GetStr (TXT_noStatus);
    }

    StatusFooterMsg (NULL);

    if (!properties)
	return;

    if (properties->activeJob)
    {
	XtFree (properties->activeJob);
	properties->activeJob = 0;
    }

    if (!LpPrinterStatus (properties->prtName, &properties->activeJob,
			  &properties->accepting, &properties->enabled,
			  &properties->faulted))
	sprintf (statusLine, noStatusMsg, properties->prtName);
    else
    {
	if (properties->activeJob)
	    sprintf (statusLine, printingMsg, properties->prtName,
		     properties->activeJob);
	else if (properties->faulted)
	    sprintf (statusLine, faultedMsg, properties->prtName);
	else if (!properties->enabled)
	    sprintf (statusLine, disabledMsg, properties->prtName);
	else
	    sprintf (statusLine, idleMsg, properties->prtName);
    }

    XtVaSetValues (PrtStatus.state,
		XtNstring,		(XtArgVal) statusLine,
		0);

    PrtStatus.acceptCtrl.setIndx = properties->accepting ?
	Accept_Button : Reject_Button;
    OlVaFlatSetValues (PrtStatus.acceptCtrl.btn, PrtStatus.acceptCtrl.setIndx,
		XtNset,			(XtArgVal) True,
		0);

    PrtStatus.enableCtrl.setIndx = properties->enabled ?
	Enable_Button : Disable_Button;
    OlVaFlatSetValues (PrtStatus.enableCtrl.btn, PrtStatus.enableCtrl.setIndx,
		XtNset,			(XtArgVal) True,
		0);

}	/* End of StatusResetCB () */

/* StatusPopdownCB
 *
 * Popdown callback.  Mark the property sheet as unposted.
 */
static void
StatusPopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    StatusFooterMsg (NULL);
    PrtStatus.owner = (PropertyData *) 0;
    PrtStatus.poppedUp = False;
}	/* End of StatusPopdownCB () */

/* UpdateStatus
 *
 * Update the status of a printer as requested.  Post a notice indicating
 * the outcome.
 */
static void
UpdateStatus (LpState accept, LpState enable, LpWhen when)
{
    char	msg [256];
    int		class;
    unsigned	success;
    static Boolean	first = True;
    static char		*cantAcceptMsg;
    static char		*cantRejectMsg;
    static char		*cantEnableMsg;
    static char		*cantDisableMsg;
    static char		*acceptMsg;
    static char		*rejectMsg;
    static char		*enableMsg;
    static char		*disableMsg;

    /* Set Labels */
    if (first)
    {
	first = False;
	cantAcceptMsg = GetStr (TXT_cantAccept);
	cantRejectMsg = GetStr (TXT_cantReject);
	cantEnableMsg = GetStr (TXT_cantEnable);
	cantDisableMsg = GetStr (TXT_cantDisable);
	acceptMsg = GetStr (TXT_acceptTxt);
	rejectMsg = GetStr (TXT_rejectTxt);
	enableMsg = GetStr (TXT_enableTxt);
	disableMsg = GetStr (TXT_disableTxt);
    }

    success = LpAcceptEnable (PrtStatus.owner->prtName, accept, enable, when);

    msg [0] = 0;
    if (accept != Lp_No_Change)
	if (!(success & Lp_Accept_Flag))
	{
	    strcat (msg, accept == Lp_On ? cantAcceptMsg : cantRejectMsg);
	    class = OL_ERROR;
	}
	else
	{
	    strcat (msg, accept == Lp_On ? acceptMsg : rejectMsg);
	    class = OL_INFORMATION;
	}

    if (enable != Lp_No_Change)
	if (!(success & Lp_Enable_Flag))
	{
	    strcat (msg, enable == Lp_On ? cantEnableMsg : cantDisableMsg);
	    class = OL_ERROR;
	}
	else
	{
	    strcat (msg, enable == Lp_On ? enableMsg : disableMsg);
	    class = OL_INFORMATION;
	}

    if (msg [0])
	ErrorConfirm (PrtStatus.popupWindow, msg, class,
		      ConfirmCB, (XtPointer) PrtStatus.popupWindow);
    else
	BringDownPopup (PrtStatus.popupWindow);

    StatusResetCB (PrtStatus.popupWindow, 0, 0);
}	/* End of UpdateStatus () */

/* ConfirmCB
 *
 * After the user has seen the status message, bring down the owning popup.
 * client_data is the popup widget.
 */
void
ConfirmCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    BringDownPopup ((Widget) client_data);
}	/* End of ConfirmCB () */

/* CreateWhenPopup
 *
 * Create a window to ask the user when they want to disable the printer.
 */
static void
CreateWhenPopup (void)
{
    Widget	uca;
    Widget	lca;

    SetLabels (WhenItems, XtNumber (WhenItems));

    PrtStatus.whenPopupWindow = XtVaCreatePopupShell ("whenPopup",
		popupWindowShellWidgetClass, PrtStatus.popupWindow,
		XtNtitle,		(XtArgVal) GetStr (TXT_whenTitle),
		XtNpushpin,		(XtArgVal) OL_NONE,
		0);

    XtVaGetValues (PrtStatus.whenPopupWindow,
		XtNupperControlArea,	(XtArgVal) &uca,
		XtNlowerControlArea,	(XtArgVal) &lca,
		0);

    /* Add the command buttons to the bottom */
    (void) XtVaCreateManagedWidget ("lcaButton",
		flatButtonsWidgetClass, lca,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) WhenItems,
		XtNnumItems,		(XtArgVal) XtNumber (WhenItems),
		0);

    (void) XtVaCreateManagedWidget ("whenText",
		staticTextWidgetClass, uca,
		XtNstring,	(XtArgVal) GetStr (TXT_whenText),
		0);

}	/* End of CreateWhenPopup () */

/* WhenCB
 *
 * Callback for buttons indicating when to disable printer.
 */
static void
WhenCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;

    UpdateStatus (PrtStatus.pendingAccept, Lp_Off, pFlatData->item_index);
}	/* End of WhenCB () */

/* StatusFooterMsg
 *
 * Write message in footer of status window.
 */
static void
StatusFooterMsg (char *msg)
{
    XtVaSetValues (PrtStatus.footer,
		XtNstring,		(XtArgVal) msg,
		0);
}	/* End of StatusFooterMsg () */
