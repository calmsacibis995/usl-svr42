/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/properties.c	1.20"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <X11/Shell.h>

#include <Xol/Category.h>
#include <Xol/ChangeBar.h>
#include <Xol/Caption.h>
#include <Xol/MenuShell.h>
#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>

#include <lp.h>
#include <printers.h>

#include "properties.h"
#include "printer.h"
#include "lpsys.h"
#include "error.h"

#define NUM_SHEETS	3
#define NONE_POSTED	~0

enum {
    Inches_Button, Centimeters_Button, Units_Button,
};

static void	ApplyCB (Widget, XtPointer, XtPointer);
static void	ResetCB (Widget, XtPointer, XtPointer);
static void	PopdownCB (Widget, XtPointer, XtPointer);
static void	NewPageCB (Widget, XtPointer, XtPointer);
static void	DestroyText (Widget, XtPointer, XtPointer);

static Widget	CreateCategory (Widget, PropertyCntl *);
static void	ChangeSheet (Widget, PropertyData *, Cardinal, Boolean);

static CategoryPage	Sheets [NUM_SHEETS];
static int	SheetCnt = 0;

static HelpText PropHelp = {
    TXT_propHelp, HELP_FILE, TXT_propHelpSect,
};

static HelpText AddHelp = {
    TXT_addHelp, HELP_FILE, TXT_addHelpSect,
};

/* Lower Control Area buttons */
static MenuItem AddItems [] = {
    { (XtArgVal) TXT_addBtn, (XtArgVal) MNEM_addBtn, (XtArgVal) True,
	  (XtArgVal) ApplyCB, (XtArgVal) True, },	/* Add */
    { (XtArgVal) TXT_reset, (XtArgVal) MNEM_reset, (XtArgVal) True,
	  (XtArgVal) ResetCB, },			/* Reset */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &AddHelp, },			/* Help */
};
static MenuItem PropItems [] = {
    { (XtArgVal) TXT_apply, (XtArgVal) MNEM_apply, (XtArgVal) True,
	  (XtArgVal) ApplyCB, (XtArgVal) True, },	/* Apply */
    { (XtArgVal) TXT_reset, (XtArgVal) MNEM_reset, (XtArgVal) True,
	  (XtArgVal) ResetCB, },			/* Reset */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &PropHelp, },			/* Help */
};

ButtonItem UnitItems [] = {
    { (XtArgVal) 'i', (XtArgVal) TXT_in, },			/* in. */
    { (XtArgVal) 'c', (XtArgVal) TXT_cm, },			/* cm */
    { (XtArgVal) ' ', (XtArgVal) TXT_chars, },			/* chars */
};

String	ButtonFields [] = {
    XtNuserData, XtNlabel,
};

int	NumButtonFields = XtNumber (ButtonFields);

/* Properties
 *
 * Post Property sheet for creating printer and changing existing properties.
 * If the requested sheet is already posted, simply raise it to the top.
 * Otherwise, change to the new page.
 */
void
Properties (Widget widget, PropertyData *properties, Cardinal itemIndex)
{
    Widget		category;
    PropertyCntl	*controls = properties->controls;

    if (properties == controls->owner && itemIndex == controls->posted)
	XRaiseWindow (XtDisplay (controls->popupWidget),
		      XtWindow (controls->popupWidget));
    else
    {
	if (!controls->popupWidget)
	    category = CreateCategory (widget, controls);
	else
	    category = XtParent (controls->sheets [0]);
	ChangeSheet (category, properties, itemIndex, True);
    }
}	/* End of Properties () */

/* CreateCategory
 *
 * Create a category widget in a popup window for property sheets.
 */
static Widget
CreateCategory (Widget parent, PropertyCntl *controls)
{
    Widget		popup;
    Widget		category;
    Widget		lca;
    Widget		categoryMenuShell;
    MenuItem		*items;
    register		i;
    static Boolean	first = True;
    extern PropertyCntl	ChangeControls;

    /* Set Labels */
    if (first)
    {
	first = False;
	SetLabels (AddItems, XtNumber (AddItems));
	SetLabels (PropItems, XtNumber (PropItems));
	SetHelpLabels (&AddHelp);
	SetHelpLabels (&PropHelp);

	if (!IsAdmin ())
	{
	    AddItems [Apply_Button].sensitive = (XtArgVal) False;
	    PropItems [Apply_Button].sensitive = (XtArgVal) False;
	}
    }

    controls->popupWidget = popup = XtVaCreatePopupShell ("properties",
		transientShellWidgetClass, parent,
		XtNpushpin,		(XtArgVal) OL_OUT,
		XtNwinType,		(XtArgVal) OL_WT_CMD,
		0);

    category = XtVaCreateManagedWidget ("category", categoryWidgetClass, popup,
		XtNlayoutWidth,		(XtArgVal) OL_MAXIMIZE,
		XtNlayoutHeight,	(XtArgVal) OL_MAXIMIZE,
		XtNshowFooter,		(XtArgVal) True,
		XtNtraversalOn,		(XtArgVal) False,
		0);
    XtAddCallback (category, XtNnewPage, NewPageCB, (XtPointer) controls);

    XtVaGetValues (category,
		XtNlowerControlArea,	(XtArgVal) &lca,
		0);

    /* We want an "apply" and "reset" buttons in both the lower control
     * area and in a popup menu on the upper control area.
     */
    items = (controls == &ChangeControls) ? PropItems : AddItems;
    (void) XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNclientData,		(XtArgVal) controls,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) XtNumber (AddItems),
		0);

    categoryMenuShell = XtVaCreatePopupShell ("categoryMenuShell",
		popupMenuShellWidgetClass, category,
		0);

    (void) XtVaCreateManagedWidget ("categoryMenu",
		flatButtonsWidgetClass, categoryMenuShell,
		XtNclientData,		(XtArgVal) controls,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) XtNumber (AddItems),
		0);

    OlAddDefaultPopupMenuEH (category, categoryMenuShell);
 
    /* Create a control area for each of the property sheets */
    for (i=0; i<SheetCnt; i++)
    {
	controls->sheets [i] = XtVaCreateManagedWidget ("page",
		controlAreaWidgetClass,
		category,
		XtNavailableWhenUnmanaged,	(XtArgVal) False,
		XtNallowChangeBars,		(XtArgVal) True,
		XtNalignCaptions,		(XtArgVal) True,
		XtNlayoutType,			(XtArgVal) OL_FIXEDCOLS,
		XtNpageLabel,			(XtArgVal) Sheets [i].lbl,
		0);
    }

    XtAddCallback (popup, XtNpopdownCallback, PopdownCB,
		   (XtPointer) controls);
    return (category);
}	/* End of CreateCategory () */

/* ApplyCB
 *
 * Check property sheet for validity and make necessary updates.  client_data
 * is pointer to controls structure.
 */
static void
ApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropertyCntl	*controls = (PropertyCntl *) client_data;
    PropertyData	*properties = controls->owner;
    PropertyData	*newProperties;
    Widget		category = XtParent (controls->sheets [0]);
    char		*err;
    register		i;
    static Boolean	first = True;
    static char		*noneSelectedMsg;
    extern PropertyCntl	ChangeControls;

    if (first)
    {
	first = False;

	noneSelectedMsg = GetStr (TXT_noneSelected);
    }

    /* Check all property sheets for validity.  Errors here are not
     * horribly serious, so display the message in the footer.
     */
    if (!properties)
    {
	FooterMsg (category, noneSelectedMsg);
	return;
    }

    FooterMsg (category, NULL);
    for (i=0; i<SheetCnt; i++)
    {
	if (err = (*Sheets[i].checkProc)(controls->sheets[i], properties))
	{
	    ChangeSheet (category, properties, i, True);
	    FooterMsg (category, err);
	    return;
	}
    }

    /* Update the lp database. */
    if (err = UpdatePrinter (properties))
    {
	Error (category, err, OL_ERROR);
	return;
    }

    /* If creating a new printer, create a properties record. */
    if (!properties->prtName)
    {
	newProperties = (PropertyData *) XtCalloc (1, sizeof(*newProperties));
	*newProperties = *properties;
	properties->config = (PRINTER *) 0;
    }
    else
	newProperties = properties;

    /* Make the updates to the properties and add new printers to
     * the icon box.
     */
    for (i=0; i<SheetCnt; i++)
	(*Sheets [i].applyProc)(controls->sheets [i], newProperties);
    newProperties->controls = &ChangeControls;

    if (properties != newProperties)
	AddPrinter (newProperties);

    BringDownPopup (controls->popupWidget);
}	/* End of ApplyCB () */

/* ResetCB
 *
 * Reset values in the current property sheet to their original values.
 * client_data refers to the controls data for the sheet.
 */
static void
ResetCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropertyCntl	*controls = (PropertyCntl *) client_data;
    Widget		category = XtParent (controls->sheets [0]);

    if (!controls->owner)
	return;

    FooterMsg (category, NULL);
    (*Sheets [controls->posted].resetProc)
	(controls->sheets [controls->posted], controls->owner);
}	/* End of ResetCB () */

/* CancelCB
 *
 * Make the property sheet pop down.  Don't debate the topic.
 */
void
CancelCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget	shell;

    shell = XtParent(widget);
    while (!XtIsShell (shell))
	shell = XtParent(shell);

    XtPopdown (shell);
}	/* End of CancelCB () */

/* VerifyCB
 *
 * Verify callback.  Because the verify callback is not called
 * in motif mode, never set the flag to true; the individual button
 * callbacks will bring down the popup in needed.
 */
void
VerifyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Boolean	*pOk = (Boolean *) call_data;

    *pOk = False;
}	/* End of VerifyCB () */

/* BringDownPopup
 *
 * Check the state of the pushpin, and if it is not in, popdown the
 * property sheet.
 */
void
BringDownPopup (Widget popup)
{
    OlDefine	pinState;

    XtVaGetValues (popup,
		XtNpushpin,		(XtArgVal) &pinState,
		0);

    if (pinState != OL_IN)
	XtPopdown (popup);

}	/* End of BringDownPopup () */

/* PopdownCB
 *
 * Popdown callback.  Mark the property sheet as unposted and reset all
 * properties to their applied values.  client_data is the controls data.
 */
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropertyCntl	*controls = (PropertyCntl *) client_data;

    FooterMsg (XtParent (controls->sheets [0]), NULL);
    controls->posted = NONE_POSTED;
    controls->owner = (PropertyData *) 0;
    controls->poppedUp = False;
}	/* End of PopdownCB () */

/* ChangeSheet
 *
 * Change the current property sheet, creating the new page if it doesn't
 * already exist.  If the popup window is not already up, post it.
 */
static void
ChangeSheet (Widget category, PropertyData *properties, Cardinal page,
	     Boolean tellCategory)
{
    PropertyCntl	*controls = properties->controls;
    Boolean		apply_all;
    register		i;
    char		title [80];
    static char		*addLbl;
    static char		*propertyLbl;
    static char		*parallelLbl;
    static char		*serialLbl;
    static char		*remoteLbl;
    static Boolean	first = True;

    /* Set Labels */
    if (first)
    {
	first = False;
	addLbl = GetStr (TXT_addPrt);
	propertyLbl = GetStr (TXT_prtProp);
	parallelLbl = GetStr (TXT_parallel);
	serialLbl = GetStr (TXT_serial);
	remoteLbl = GetStr (TXT_remote);
    }

    if (properties == controls->owner && controls->posted == page)
	return;		/* Correct page already set */

    if (!(controls->pageCreated & (1<<page)))
    {
	/* Make the property sheet */
	(*Sheets [page].createProc) (controls->sheets [page], properties);
    }
    else
    {
	if (properties != controls->owner)
	    for (i=0; i<SheetCnt; i++)
		(*Sheets [i].resetProc)(controls->sheets [i], properties);
    }

    if (properties != controls->owner)
    {
	if (!properties->prtName)
	{
	    char	*portName;

	    switch (properties->kind) {
	    case ParallelPort:
		portName = parallelLbl;
		break;
	    case SerialPort:
		portName = serialLbl;
		break;
	    case RemotePort:
		portName = remoteLbl;
		break;
	    }
	    sprintf (title, addLbl, portName);
	}
	else
	    sprintf (title, propertyLbl, properties->prtName);

	XtVaSetValues (controls->popupWidget,
		       XtNtitle,		(XtArgVal) title,
		       0);
    }

    if (properties->kind == SerialPort)
    {
	if (!XtIsManaged (controls->sheets [Comm_Page]))
	    XtManageChild (controls->sheets [Comm_Page]);
    }

    if (tellCategory)
	apply_all = OlCategorySetPage(category, controls->sheets [page]);

    if (properties->kind != SerialPort)
    {
	if (XtIsManaged (controls->sheets [Comm_Page]))
	    XtUnmanageChild (controls->sheets [Comm_Page]);
    }

    if (!controls->poppedUp)
    {
	XtPopup (controls->popupWidget, XtGrabNone);
	controls->poppedUp = True;
    }
    controls->owner = properties;
    controls->posted = page;
}	/* End of ChangeSheet () */

/* NewPageCB
 *
 * Update the currently posted page.  client_data is a pointer to controls
 * data, and call_data contains the new page data.
 */
void
NewPageCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropertyCntl	*controls = (PropertyCntl *) client_data;
    OlCategoryNewPage	*newPage = (OlCategoryNewPage *) call_data;
    register		i;

    for (i=0; i<SheetCnt; i++)
	if (newPage->new_page == controls->sheets [i])
	    break;

    ChangeSheet (widget, controls->owner, i, False);
}	/* End of NewPageCB () */

/* ButtonSelectCB
 *
 * Update the currently select button within an exclusives.  client_data is
 * structure for individual button.
 */
void
ButtonSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    BtnChoice		*button = (BtnChoice *) client_data;
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;

    button->setIndx = pFlatData->item_index;

}	/* End of ButtonSelectCB () */

/* NoneSetSelectCB
 *
 * Update the currently select button within an exclusives.  Make sure
 * the buttons are not dim.  client_data is structure for individual button.
 */
void
NoneSetSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    BtnChoice		*button = (BtnChoice *) client_data;
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;

    button->setIndx = pFlatData->item_index;

    XtVaSetValues (widget,
		XtNdim,		(XtArgVal) False,
		0);
}	/* End of NoneSetSelectCB () */

/* NoneSetUnselectCB
 *
 * Update the currently select button within an exclusives to indicate no
 * buttons are currently selected.  Make sure the buttons are dim.
 * client_data is structure for individual button.
 */
void
NoneSetUnselectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    BtnChoice		*button = (BtnChoice *) client_data;
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;

    button->setIndx = OL_NO_ITEM;

    XtVaSetValues (widget,
		XtNdim,		(XtArgVal) True,
		0);
}	/* End of NoneSetUnselectCB () */

/* FooterMsg
 *
 * Display a message in the footer of the category widget.
 */
void
FooterMsg (Widget widget, char *msg)
{
    XtVaSetValues (widget,
		XtNleftFoot,		(XtArgVal) msg,
		0);
}	/* End of FooterMsg () */

/* InitProperties
 *
 * Initialize properties structure.  If config is 0, then the properties
 * structure is initialized with default values for adding new printers.
 * The structure is assumed to have the kind of port connection already set.
 * If config is not 0, then the properties and connection type are extracted
 * from the configuration data.
 */
void
InitProperties (Widget widget, PropertyData *properties, PRINTER *config)
{
    register	i;

    /* Initialize sections pertaining to each property sheet. */
    for (i=0; i<SheetCnt; i++)
	(*Sheets [i].initProc)(properties->controls->sheets [i],
			       properties, config);

} /* End of InitProperties () */

/* UpdatePrinter
 *
 * Populate a printer configuration structure and call the lp utilities to
 * update the lp database.  Return a pointer to the error text if there
 * was an error.
 */
char *
UpdatePrinter (PropertyData *properties)
{
    PRINTER		*config;
    Bool		rc;
    register		i;
    static char		*updateMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	updateMsg = GetStr (TXT_badAdmin);
    }

    config = (PRINTER *) XtMalloc (sizeof (*config));
    if (properties->config)
	CopyConfig (config, properties->config);
    else
    {
	/* Create a default config.  Because this can only happen for
	 * new printers, the basic page must exist, and UpdateBasic will
	 * take care of most of the initialization.  All we have to
	 * provide here are the defaults for communication and communication.
	 */
	(void) memset (config, 0, sizeof(*config));

	config->banner = BAN_ALWAYS;
	if (properties->kind == RemotePort)
	    config->fault_alert.shcmd = strdup ("none");
	else
	{
	    char	buf [32];

	    sprintf (buf, "mail %s", getname ());
	    config->fault_alert.shcmd = strdup (buf);
	}
    }

    for (i=0; i<SheetCnt; i++)
	(*Sheets [i].updateProc)(properties->controls->sheets [i],
				 properties, config);

    rc = LpAdmin (config, properties->newAllowRmt);

    if (!rc)
    {
	FreeConfig (config, False);
	return (updateMsg);
    }

    /* If creating a new printer, enable the printer. */
    if (!properties->prtName)
	(void) LpAcceptEnable (config->name, Lp_On, Lp_On, Lp_Requeue);
    else
	FreeConfig (properties->config, False);

    properties->config = config;

    return (NULL);
} /* End of UpdatePrinter () */

/* MakeButton
 *
 * Make a button control with a caption.
 */
void
MakeButtons (Widget parent, char *lbl, ButtonItem *items, Cardinal numItems,
	    BtnChoice *data)
{
    Widget	caption;

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    data->btn = XtVaCreateManagedWidget ("button",
		flatButtonsWidgetClass, caption,
		XtNbuttonType,		(XtArgVal) OL_RECT_BTN,
		XtNselectProc,		(XtArgVal) ButtonSelectCB,
		XtNclientData,		(XtArgVal) data,
		XtNexclusives,		(XtArgVal) True,
		XtNitemFields,		(XtArgVal) ButtonFields,
		XtNnumItemFields,	(XtArgVal) NumButtonFields,
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) numItems,
		0);
}	/* End of MakeButton () */

/* MakeNoneSetButtons
 *
 * Similar to MakeButtons, except that it is permitted to have no buttons
 * pressed.  When this is the case, the buttons should be dim.
 */
void
MakeNoneSetButtons (Widget parent, char *lbl, ButtonItem *items,
		    Cardinal numItems, BtnChoice *data)
{
    Widget	caption;

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    data->btn = XtVaCreateManagedWidget ("noneSetButton",
		flatButtonsWidgetClass, caption,
		XtNbuttonType,		(XtArgVal) OL_RECT_BTN,
		XtNnoneSet,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) NoneSetSelectCB,
		XtNunselectProc,	(XtArgVal) NoneSetUnselectCB,
		XtNclientData,		(XtArgVal) data,
		XtNexclusives,		(XtArgVal) True,
		XtNitemFields,		(XtArgVal) ButtonFields,
		XtNnumItemFields,	(XtArgVal) NumButtonFields,
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) numItems,
		0);
}	/* End of MakeNoneSetButtons () */

/* ResetNoneSetButton
 *
 * Reset a none-set buttons widget to value.  If value is OL_NO_ITEM, the
 * buttons should be dim.  Also, if we are resetting to a definite value,
 * then disallow the NoneSet option.
 */
void
ResetNoneSetButton (BtnChoice *ctrl, Cardinal value)
{
    int		cnt;
    Boolean	pressed;
    register	i;

    if (value == OL_NO_ITEM)
    {
	/* If a button is currently pressed, unset it. */
	XtVaGetValues (ctrl->btn,
		XtNnumItems,		(XtArgVal) &cnt,
		0);

	for (i=0; i<cnt; i++)
	{
	    OlVaFlatGetValues (ctrl->btn, i,
			XtNset,		(XtArgVal) &pressed,
			0);
	    if (pressed)
	    {
		OlVaFlatSetValues (ctrl->btn, i,
			XtNset,		(XtArgVal) False,
			0);
		break;
	    }
	}

	XtVaSetValues (ctrl->btn,
		XtNdim,			(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) NoneSetSelectCB,
		XtNunselectProc,	(XtArgVal) NoneSetUnselectCB,
		0);
    }
    else
    {
	OlVaFlatSetValues (ctrl->btn, value,
		XtNset,			(XtArgVal) True,
		0);

	XtVaSetValues (ctrl->btn,
		XtNdim,			(XtArgVal) False,
		XtNnoneSet,		(XtArgVal) False,
		XtNselectProc,		(XtArgVal) ButtonSelectCB,
		XtNunselectProc,	(XtArgVal) 0,
		0);
    }

    ctrl->setIndx = value;
}	/* End of ResetNoneSetButton () */

/* MakeText
 *
 * Make a text field with a caption.  The initial value of the string is
 * taken from the applied value in "data".
 */
void
MakeText (Widget parent, char *lbl, TxtChoice *data, int charsVisible)
{
    Widget	caption;

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    data->txt = XtVaCreateManagedWidget ("text",
		textFieldWidgetClass, caption,
		XtNcharsVisible,	(XtArgVal) charsVisible,
		0);

    data->setText = (char *) 0;

    XtAddCallback (data->txt, XtNdestroyCallback, DestroyText, data);
}	/* End of MakeText () */

/* DestroyText
 *
 * Clean up after text control.
 */
static void
DestroyText (Widget widget, XtPointer client_data, XtPointer call_data)
{
    TxtChoice	*data = (TxtChoice *) client_data;

    if (data->setText)
	XtFree (data->setText);
}	/* End of DestroyText () */

/* ApplyText
 *
 * Apply changes to a text control.  This function assumes that there has
 * been a previous call to GetText to get the string value from the widget.
 */
void
ApplyText (TxtChoice *control, char **pValue)
{
    if (*pValue)
	XtFree (*pValue);

    *pValue = control->setText;
    control->setText = (char *) 0;
}	/* End of ApplyText () */

/* GetText
 *
 * Get the value of a text control.  The returned string should NOT be
 * freed by the caller, and should be regarded as temporary.
 */
char *
GetText (TxtChoice *txtCtrl)
{
    if (txtCtrl->setText)
	XtFree (txtCtrl->setText);

    XtVaGetValues (txtCtrl->txt,
		XtNstring,	(XtArgVal) &txtCtrl->setText,
		0);

    return (txtCtrl->setText);
}	/* End of GetText () */

/* MakeScaled
 *
 * A scaled number control is a text field that contains a positive real
 * number and an exclusives that scales the number in inches, centimeters,
 * and characters.
 */
void
MakeScaled (Widget parent, char *lbl, ScaledChoice *scaled)
{
    Widget		caption;
    Widget		control;
    static Boolean	first = True;

    if (first)
    {
	first = False;

	SetButtonLbls (UnitItems, XtNumber (UnitItems));
    }

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    control = XtVaCreateManagedWidget ("ctrlArea",
		controlAreaWidgetClass, caption,
		XtNshadowThickness,	(XtArgVal) 0,
		0);

    scaled->value.txt = XtVaCreateManagedWidget ("text",
		textFieldWidgetClass, control,
		XtNcharsVisible,	(XtArgVal) 6,
		0);
    scaled->value.setText = (char *) 0;

    XtAddCallback (scaled->value.txt, XtNdestroyCallback, DestroyText,
		   &scaled->value);

    scaled->units.btn = XtVaCreateManagedWidget ("button",
		flatButtonsWidgetClass, control,
		XtNbuttonType,		(XtArgVal) OL_RECT_BTN,
		XtNselectProc,		(XtArgVal) ButtonSelectCB,
		XtNclientData,		(XtArgVal) &scaled->units,
		XtNexclusives,		(XtArgVal) True,
		XtNitemFields,		(XtArgVal) ButtonFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ButtonFields),
		XtNitems,		(XtArgVal) UnitItems,
		XtNnumItems,		(XtArgVal) XtNumber (UnitItems),
		0);
}	/* End of MakeScaled () */

/* ApplyScaled
 *
 * Save the current values set in the scaled number.
 */
void
ApplyScaled (ScaledChoice *scaled, float *pValue, char *pUnits)
{
    if (strlen (scaled->value.setText) == 0)
    {
	*pValue = 0.0;
	*pUnits = 0;
    }
    else
    {
	sscanf (scaled->value.setText, "%f", pValue);
	*pUnits = (char) UnitItems [scaled->units.setIndx].userData;
    }
}	/* End if ApplyScaled () */

/* ResetScaled
 *
 * Reset the current value.
 */
void
ResetScaled (ScaledChoice *scaled, float value, char units)
{
    Cardinal	indx;
    char	buf [16];

    if (units)
    {
	sprintf (buf, "%.2f", value);
	XtVaSetValues (scaled->value.txt,
		XtNstring,		(XtArgVal) buf,
		0);
    }
    else
	XtVaSetValues (scaled->value.txt,
		XtNstring,		(XtArgVal) 0,
		0);

    switch (units) {
    case 'i':
	indx = Inches_Button;
	break;
    case 'c':
	indx = Centimeters_Button;
	break;
    case ' ':
    default:
	indx = Units_Button;
	break;
    }
    scaled->units.setIndx = indx;
    OlVaFlatSetValues (scaled->units.btn, indx,
		XtNset,			(XtArgVal) True,
		0);
}	/* End of ResetScaled () */

/* CheckScaled
 *
 * Check that the scaled number is a positive real number.  Return True if ok.
 */
Boolean
CheckScaled (ScaledChoice *scaled)
{
    if (scaled->value.setText)
	XtFree (scaled->value.setText);

    XtVaGetValues (scaled->value.txt,
		XtNstring,		(XtArgVal) &scaled->value.setText,
		0);

    if (strlen (scaled->value.setText) > (unsigned) 0)
    {
	char	*ptr;
	float	val;
	char	buf [32];

	/* it seems kind of silly to convert the string to a double and then
	 * back into a string, but it is a convenient way to remove any white
	 * space from the original string.
	 */
	val = (float) strtod (scaled->value.setText, &ptr);
	if (val <= 0.0 || *ptr)
	    return (False);
	XtFree (scaled->value.setText);
	sprintf (buf, "%.2f", val);
	scaled->value.setText = strdup (buf);
    }
    else
    {
	XtFree (scaled->value.setText);
	scaled->value.setText = (char *) 0;
    }

    return (True);
}	/* End of CheckScaled () */

/* CopyConfig
 *
 * Copy the printer configuration.  Not all aspects of the configuration are
 * duplicated.  Use FreeConfig to correctly get rid of a config with trashing
 * out the malloc arena.
 */
void
CopyConfig (PRINTER *dst, PRINTER *src)
{
    *dst = *src;
    if (dst->stty)
	dst->stty = strdup (dst->stty);
    if (dst->remote)
	dst->remote = strdup (dst->remote);
    if (dst->fault_alert.shcmd)
	dst->fault_alert.shcmd = strdup (dst->fault_alert.shcmd);
}	/* End of CopyConfig () */

/* FreeConfig
 *
 * Free malloc'ed data from a printer configuration.  The flag indicated if
 * the PRINTER structure came from a call to getprinter, in which case, don't
 * free the structure itself, but free ALL of the members.  Otherwise, free
 * the structure, but some of the members might still have pointers to them in
 * other structures.  Very nice.
 */
void
FreeConfig (PRINTER *config, Boolean freeall)
{
    if (freeall)
	freeprinter (config);
    else
    {
	XtFree (config->stty);
	XtFree (config->remote);
	XtFree (config->fault_alert.shcmd);
	XtFree ((char *) config);
    }
}	/* End of FreeConfig () */

/* InitPropertySheets
 *
 * Initialize each page in the category widget.
 */
void
InitPropertySheets (Widget widget)
{
    extern void	InitBasicSheet (Widget, CategoryPage *);
    extern void	InitConfigurationSheet (Widget, CategoryPage *);
    extern void	InitCommunicationSheet (Widget, CategoryPage *);

    SheetCnt = 0;

    InitBasicSheet (widget, &Sheets [SheetCnt++]);
    InitConfigurationSheet (widget, &Sheets [SheetCnt++]);
    InitCommunicationSheet (widget, &Sheets [SheetCnt++]);
}	/* End of InitPropertySheets () */
