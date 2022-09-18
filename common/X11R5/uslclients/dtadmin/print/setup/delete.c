/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/delete.c	1.9"
#endif

#include <stdio.h>
#include <string.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <libDtI/DtI.h>

#include <Xol/Modal.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>

#include <lp.h>
#include <msgs.h>

#include "properties.h"
#include "printer.h"
#include "error.h"

static void DeleteAndPopdown (Widget, XtPointer, XtPointer);
static void DestroyPrinters (Widget, XtPointer, XtPointer);
static void DestroyProperties (PropertyData *);
static void CleanUp (Widget, XtPointer, XtPointer);
static void KillJobs (Widget, XtPointer, XtPointer);
static void SkipPrinter (Widget, XtPointer, XtPointer);
static void SkipAndPopdown (Widget, XtPointer, XtPointer);

static void PrinterBusy (Widget, IconItem *);

static Widget		DeleteNotice;
static Widget		DeleteText;
static Widget		DeleteBtns;
static IconItem		**DeleteList;
static Cardinal		DeleteCnt;

/* Lower Control Area buttons */
static MenuItem DeleteItems [] = {
    { (XtArgVal) TXT_delete, (XtArgVal) MNEM_delete, (XtArgVal) True,
	  (XtArgVal) DeleteAndPopdown, (XtArgVal) True, }, /* Delete */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CleanUp, },			/* Cancel */
};

static MenuItem BusyItems [] = {
    { (XtArgVal) TXT_delete, (XtArgVal) MNEM_delete, (XtArgVal) True,
	  (XtArgVal) KillJobs, (XtArgVal) True, },	/* Delete */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) SkipAndPopdown, },			/* Cancel */
};

/* DeletePrt
 *
 * Delete a printer from the lp system and remove its icon from the icon
 * box.  User data is the flat item is the icon box widget.
 */
void
DeletePrt (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
    Widget		iconBox;
    IconItem		*item;
    Cardinal		cnt;
    char		*prtStr;
    char		*msg;
    register		i;
    static char		*confirmMsg;
    static char		*deleteTitle;
    static Boolean	first = True;

    iconBox = (Widget) ((MenuItem *) flatData->
			items) [flatData->item_index].userData;

    /* Set Labels and Create a notice asking if the user really wants
     * to do this.
     */
    if (first)
    {
	first = False;
	SetLabels (DeleteItems, XtNumber (DeleteItems));
	confirmMsg = GetStr (TXT_reallyDelete);
	deleteTitle = GetStr (TXT_deleteTitle);

	DeleteNotice = XtVaCreatePopupShell ("deleteNotice",
		modalShellWidgetClass, widget,
		XtNnoticeType,		(XtArgVal) OL_WARNING,
		XtNtitle,		(XtArgVal) deleteTitle,
		0);

	/* Add the error message widget */
	DeleteText = XtVaCreateManagedWidget ("delTxt", staticTextWidgetClass,
		DeleteNotice,
		XtNalignment,		(XtArgVal) OL_CENTER,
    		XtNfont,		(XtArgVal) _OlGetDefaultFont (widget,
							OlDefaultNoticeFont),
		0);

	/* Add delete/cancel buttons to the bottom */
	DeleteBtns = XtVaCreateManagedWidget ("lcaButton",
		flatButtonsWidgetClass, DeleteNotice,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) DeleteItems,
		XtNnumItems,		(XtArgVal) XtNumber (DeleteItems),
		0);
    }

    /* Get the select printers */
    XtVaGetValues (iconBox,
		XtNitems,		(XtArgVal) &item,
		XtNnumItems,		(XtArgVal) &cnt,
	        0);

    prtStr = XtMalloc (cnt * 20);
    prtStr [0] = 0;

    DeleteList = (IconItem **) XtMalloc (cnt * sizeof (*DeleteList));
    DeleteCnt = 0;
    for (i=cnt; --i>=0; item++)
    {
	if (item->selected)
	{
	    if (DeleteCnt != 0)
		strcat (prtStr, ", ");
	    strcat (prtStr, (char *) item->lbl);
	    DeleteList [DeleteCnt++] = item;
	}
    }

    msg = XtMalloc (strlen (confirmMsg) + strlen (prtStr) + 1);
    sprintf (msg, confirmMsg, prtStr);
    XtVaSetValues (DeleteText,
		XtNstring,		(XtArgVal) msg,
		0);

    XtPopup (DeleteNotice, XtGrabExclusive);

    XtFree (prtStr);
    XtFree (msg);
}	/* End of DeletePrt () */

/* CleanUp
 *
 * Free resources associated with the delete notice.  Popdown the notice,
 * which is the parent of widget.
 */
static void
CleanUp (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown (XtParent (widget));
    XtFree ((char *) DeleteList);
}	/* End of CleanUp () */

/* DeleteAndPopdown
 *
 * Popdown the notice, which is the parent of widget and delete the seleted
 * printers.
 */
static void
DeleteAndPopdown (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown (XtParent (widget));
    DestroyPrinters (widget, client_data, call_data);
}	/* End of DeleteAndPopdown () */

/* DestroyPrinters
 *
 * Check the printers to be deleted for active jobs.  If there are none,
 * remove the printer; if there are jobs, ask the user if the jobs should
 * be canceled.  If the jobs are not removed, this printer will not be
 * deleted, but all others in the list will be.  The list of printers to
 * remove is in the global DeleteList.
 */
static void
DestroyPrinters (Widget widget, XtPointer client_data, XtPointer call_data)
{
    int			status;
    PropertyData	*properties;
    static char		*deleteMsg;
    static Boolean	first = True;
    extern DmGlyphPtr	DfltPrtGlyph;

    if (first)
    {
	first = False;
	deleteMsg = GetStr (TXT_badDelete);
    }

    /* Because deleting printers might be interrupted (because a printer has
     * active jobs), process the deletion list from back to front, removing
     * items off the end after successfulling deleting them.
     */
    while (DeleteCnt > 0)
    {
	status = LpDelete (DeleteList [DeleteCnt - 1]->lbl);
	if (status != MOK)
	{
	    if (status == MBUSY)
	    {
		/* Printer has active jobs.  Post a notice and await further
		 * orders.
		 */
		PrinterBusy (widget, DeleteList [DeleteCnt - 1]);
		break;
	    }
	    else
	    {
		/* Something bad happened.  Most likely, the scheduler is
		 * not running.  Can not delete this printer now.
		 */
		Error (widget, deleteMsg, OL_ERROR);
		DeleteCnt--;
		continue;
	    }
	}

	/* The printer was deleted.  Remove it from the list, and if
	 * its property sheet was posted, remove it.
	 */
	properties = (PropertyData *) DeleteList [--DeleteCnt]->properties;
	if (DeleteList [DeleteCnt]->glyph == (XtArgVal) DfltPrtGlyph)
	    SaveDefault (NULL);
	if (properties == properties->controls->owner)
	    properties->controls->owner = (PropertyData *) 0;

	DestroyProperties ((PropertyData *) DeleteList[DeleteCnt]->properties);
	XtFree ((char *) DeleteList [DeleteCnt]->lbl);
	DeleteList [DeleteCnt]->properties = 0;
    }

    if (DeleteCnt == 0)
	XtFree ((char *) DeleteList);

    /* Remove the deleted printers from the icon box */
    DelPrinters ();
}	/* End of DestroyPrinters () */

/* PrinterBusy
 *
 * Post a notice asking if the user wants to cancel all jobs for a printer
 * or skip the deletion of the printer.  Reuse the notice created for the
 * delete confirmation.
 */
static void
PrinterBusy (Widget widget, IconItem *printerItem)
{
    char		*msg;
    static Widget	buttons;
    static char		*msgString;
    static Boolean	first = True;

    /* Set Labels */
    if (first)
    {
	first = False;
	SetLabels (BusyItems, XtNumber (BusyItems));
	msgString = GetStr (TXT_activeJobs);
    }

    XtVaSetValues (DeleteBtns,
		XtNclientData,		(XtArgVal) printerItem,
		XtNitems,		(XtArgVal) BusyItems,
		XtNnumItems,		(XtArgVal) XtNumber (BusyItems),
		0);

    msg = (char *) XtMalloc (strlen (msgString) *
			     strlen ((char *) printerItem->lbl));
    sprintf (msg, msgString, (char *) printerItem->lbl);

    XtVaSetValues (DeleteText,
		XtNstring,		(XtArgVal) msg,
		0);

    XtPopup (DeleteNotice, XtGrabExclusive);

    XtFree (msg);
}	/* End of PrinterBusy () */

/* SkipAndPopdown
 *
 * Popdown the notice, which is the parent of widget and cancel the deletion
 * of the printer.
 */
static void
SkipAndPopdown (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown (XtParent (widget));
    SkipPrinter (widget, client_data, call_data);
}	/* End of SkipAndPopdown () */

/* SkipPrinter
 *
 * Cancel the deletion of a printer and continue processing the deletion
 * list. */
static void
SkipPrinter (Widget widget, XtPointer client_data, XtPointer call_data)
{
    DeleteCnt--;
    DestroyPrinters (widget, 0, 0);
}	/* End of SkipPrinter () */

/* KillJobs
 *
 * Kill all active print jobs for a printer and retry deleting the printers
 * in the deletion list.  Popdown the notice, which is the parent of widget.
 * client_data is the icon box item for the printer.
 */
static void
KillJobs (Widget widget, XtPointer client_data, XtPointer call_data)
{
    IconItem		*item = (IconItem *) client_data;
    static char		*killMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	killMsg = GetStr (TXT_badCancel);
    }

    XtPopdown (XtParent (widget));

    /* Cancel all the jobs for the printer */
    if (!LpCancelAll ((char *) item->lbl))
    {
	/* Could not cancel all jobs, probably because the scheduler is
	 * not running.  Post an error and skip the deletion of the printer.
	 */
	Error (widget, killMsg, OL_ERROR);
	SkipPrinter (widget, 0, 0);
    }
    else
	DestroyPrinters (widget, 0, 0);
}	/* End of KillJobs () */

/* DestroyProperties
 *
 * Free all dynamically allocated space in properties structure then free the
 * structure itself.
 */
static void
DestroyProperties (PropertyData *properties)
{
    XtFree (properties->prtName);
    XtFree (properties->stty);
    XtFree (properties->activeJob);
    FreeConfig (properties->config, False);
    switch (properties->kind) {
    case ParallelPort:
	XtFree (properties->device.parallel.miscDev);
	break;
    case SerialPort:
	XtFree (properties->device.serial.miscDev);
	break;
    case RemotePort:
	XtFree (properties->device.remote.system);
	XtFree (properties->device.remote.rmtName);
	break;
    }
    XtFree ((char *) properties);
}	/* End of DestroyProperties () */
