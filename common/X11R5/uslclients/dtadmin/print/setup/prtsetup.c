/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/prtsetup.c	1.28"
#endif

#include <stdio.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Xol/ScrolledWi.h>
#include <Xol/Form.h>
#include <Xol/OlCursors.h>

#include <Desktop.h>
#include <libDtI/DtI.h>
#include <libDtI/FIconBox.h>

#include "properties.h"
#include "printer.h"
#include "remote.h"
#include "error.h"

enum {
    Actions_Button, Printer_Button, Help_Button,
};

enum {
    Add_Button, Delete_Button, Prop_Button,
};

enum {
    Dflt_Button, Cntl_Button, Install_Button, Rmt_Acc_Button, Exit_Button,
};

static Widget	BuildIconBox (Widget);
static void	MakeIcon (Widget);
extern void	DeletePrt (Widget, XtPointer, XtPointer);
static void	AddCB (Widget, XtPointer, XtPointer);
static void	PropCB (Widget, XtPointer, XtPointer);
static void	DfltPrtCB (Widget, XtPointer, XtPointer);
static void	ControlCB (Widget, XtPointer, XtPointer);
static void	SelectIcon (Widget, XtPointer, XtPointer);
extern void	InstallCB (Widget, XtPointer, XtPointer);
extern void	InstallPrt (Widget, XtPointer, XtPointer);
static void	ExitCB (Widget, XtPointer, XtPointer);
static void	CreateIconCursor (Widget, XtPointer, XtPointer);
static void	AcknowledgeDtReply (Widget, XtPointer, XEvent *, Boolean *);

extern void	SetButtonState (Cardinal, Boolean);
extern Boolean	IsAdmin (void);


char	*AppName;
char	*AppTitle;

/* Empty printer properties for adding new printers */
static PropertyData	NewParallel;
static PropertyData	NewSerial;
static PropertyData	NewRemote;

XtAppContext	AppContext;
static Widget	TopLevel;
Widget		IconBox;
static Widget	PrinterMenu;
static Widget	ActionMenu;
static Widget	PropMenu;

/* Menu items and fields */

String	MenuFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};

int	NumMenuFields = XtNumber (MenuFields);

static MenuItem MenuBarItems [] = {
    { (XtArgVal) TXT_actions, (XtArgVal) MNEM_actions, (XtArgVal) True,
	  (XtArgVal) 0, (XtArgVal) True, },		/* Actions */
    { (XtArgVal) TXT_printer, (XtArgVal) MNEM_printer, (XtArgVal) True,
	  (XtArgVal) 0, },				/* Printer */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) True,
	  (XtArgVal) 0, },				/* Help */
};

static MenuItem PrinterItems [] = {
    { (XtArgVal) TXT_add, (XtArgVal) MNEM_add, (XtArgVal) True,
	  (XtArgVal) 0, },				/* Add */
    { (XtArgVal) TXT_delete, (XtArgVal) MNEM_delete, (XtArgVal) False,
	  (XtArgVal) DeletePrt, },			/* Delete */
    { (XtArgVal) TXT_properties, (XtArgVal) MNEM_properties, (XtArgVal) False,
	  (XtArgVal) 0, },				/* Properties */
};

static MenuItem ActionItems [] = {
    { (XtArgVal) TXT_dfltPrt, (XtArgVal) MNEM_dfltPrt, (XtArgVal) False,
	  (XtArgVal) DfltPrtCB, },			/* Default Printer */
    { (XtArgVal) TXT_control, (XtArgVal) MNEM_control, (XtArgVal) False,
	  (XtArgVal) ControlCB, },			/* Control */
    { (XtArgVal) TXT_installW, (XtArgVal) MNEM_installW, (XtArgVal) False,
	  (XtArgVal) InstallCB, },			/* Install */
    { (XtArgVal) TXT_remoteAccess, (XtArgVal) MNEM_remoteAccess,
	  (XtArgVal) True, (XtArgVal) RemoteSystemsCB, }, /* Remote Access */
    { (XtArgVal) TXT_exit, (XtArgVal) MNEM_exit, (XtArgVal) True,
	  (XtArgVal) ExitCB, },				/* Exit */
};

static MenuItem AddItems [] = {
    { (XtArgVal) TXT_parallel, (XtArgVal) MNEM_parallel, (XtArgVal)True,
	  (XtArgVal)AddCB, (XtArgVal) True,
	  (XtArgVal) &NewParallel, },			/* Parallel */
    { (XtArgVal) TXT_serial, (XtArgVal) MNEM_serial, (XtArgVal)True,
	  (XtArgVal)AddCB, (XtArgVal) False,
	  (XtArgVal) &NewSerial, },			/* Serial */
    { (XtArgVal) TXT_remote, (XtArgVal) MNEM_remote, (XtArgVal)True,
	  (XtArgVal)AddCB, (XtArgVal) False,
	  (XtArgVal) &NewRemote, },			/* Remote */
};

static MenuItem PropItems [] = {
    { (XtArgVal) TXT_basic, (XtArgVal) MNEM_basic, (XtArgVal) True,
	  (XtArgVal) PropCB, },				/* Basic */
    { (XtArgVal) TXT_configuration, (XtArgVal) MNEM_configuration,
	  (XtArgVal) True, (XtArgVal) PropCB, },	/* Configuration */
    { (XtArgVal) TXT_communication, (XtArgVal) MNEM_communication,
	  (XtArgVal) True, (XtArgVal) PropCB, },	/* Communications */
};

static HelpText AppHelp = {
    TXT_appHelp, HELP_FILE, TXT_appHelpSect,
};

static HelpText TOCHelp = {
    TXT_tocHelp, HELP_FILE, 0,
};

static MenuItem HelpItems [] = {
    { (XtArgVal) TXT_application, (XtArgVal) MNEM_application, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) True,
	  (XtArgVal) &AppHelp, },			/* Application */
    { (XtArgVal) TXT_TOC, (XtArgVal) MNEM_TOC, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &TOCHelp, },			/* Table o' Contents */
    { (XtArgVal) TXT_helpDesk, (XtArgVal) MNEM_helpDesk, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) 0, },				/* Help Desk */
};

/* Icon Box fields and types */
static String IconFields [] = {
    XtNlabel, XtNobjectData, XtNx, XtNy, XtNwidth, XtNheight, XtNset,
    XtNuserData,
};

static XrmOptionDescRec	Options [] =
{
	{ "-o", ".administrator", XrmoptionNoArg, (XtPointer) "True" },
};

main (argc, argv)
    int		argc;
    char	**argv;
{

    Widget	form;
    Widget	menuBar;
    Window	owner;
    Cardinal	numActionItems;
    Boolean	networking;
    register	i;

    AppName = APP_NAME;
    AppTitle = GetStr (TXT_appName);

    OlToolkitInitialize (&argc, argv, NULL);

    TopLevel = XtAppInitialize(
			&AppContext,		/* app_context_return	*/
			APPNAME,		/* application_class	*/
			Options,		/* options		*/
			XtNumber (Options),	/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String *) NULL,	/* fallback_resources	*/
			(ArgList) NULL,		/* args			*/
			(Cardinal) 0		/* num_args		*/
    );

    XtVaSetValues (TopLevel,
		XtNtitle,		(XtArgVal) GetStr (TXT_appTitle),
		XtNmappedWhenManaged,	(XtArgVal) False,
		XtNwidth,		(XtArgVal) 1,
		XtNheight,		(XtArgVal) 1,
		0);

    XtRealizeWidget (TopLevel);

    /* Check if we are already running. */
    owner = DtSetAppId (XtDisplay (TopLevel), XtWindow (TopLevel),
			"prtsetup");
    if (owner != None)
    {
	/* We are already running.  Bring that window to the top and die. */
	XRaiseWindow (XtDisplay (TopLevel), owner);
	XFlush (XtDisplay (TopLevel));
	exit (0);
    }

    DtInitialize (TopLevel);
    XtAddEventHandler (TopLevel, (EventMask) NoEventMask, True,
		       AcknowledgeDtReply, (XtPointer) NULL);

    InitSupportedPrinters (TopLevel);
    InitPropertySheets (TopLevel);
    networking = IsNetworking ();

    form = XtVaCreateManagedWidget ("form", formWidgetClass, TopLevel,
		0);

    SetLabels (MenuBarItems, XtNumber (MenuBarItems));
    SetLabels (ActionItems, XtNumber (ActionItems));
    SetLabels (PrinterItems, XtNumber (PrinterItems));
    SetLabels (AddItems, XtNumber (AddItems));
    SetLabels (PropItems, XtNumber (PropItems));
    SetLabels (HelpItems, XtNumber (HelpItems));
    SetHelpLabels (&AppHelp);
    SetHelpLabels (&TOCHelp);

    NewParallel.kind = ParallelPort;
    InitProperties (TopLevel, &NewParallel, 0);
    NewSerial.kind = SerialPort;
    InitProperties (TopLevel, &NewSerial, 0);

    if (networking)
    {
	NewRemote.kind = RemotePort;
	InitProperties (TopLevel, &NewRemote, 0);
	numActionItems = XtNumber (ActionItems);
    }
    else
    {
	for (i=Rmt_Acc_Button+1; i<XtNumber(ActionItems); i++)
	    ActionItems [i-1] = ActionItems [i];
	numActionItems = XtNumber (ActionItems) - 1;
    }

    if (!IsAdmin ())
    {
	PrinterItems [Add_Button].sensitive = (XtArgVal) False;
	PrinterItems [Delete_Button].sensitive = (XtArgVal) False;
    }

    PrinterItems [Add_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("addMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    (void) XtVaCreateManagedWidget ("addMenu", flatButtonsWidgetClass,
		(Widget) PrinterItems [Add_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) AddItems,
		XtNnumItems,		(XtArgVal) XtNumber (AddItems) -
						(networking ? 0 : 1),
		0);

    PrinterItems [Prop_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("propMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    PropMenu = XtVaCreateManagedWidget ("propMenu", flatButtonsWidgetClass,
		(Widget) PrinterItems [Prop_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) PropItems,
		XtNnumItems,		(XtArgVal) XtNumber (PropItems),
		0);

    MenuBarItems [Printer_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("printerMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    PrinterMenu = XtVaCreateManagedWidget ("printerMenu",
		flatButtonsWidgetClass,
	        (Widget) MenuBarItems [Printer_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) PrinterItems,
		XtNnumItems,		(XtArgVal) XtNumber (PrinterItems),
		0);

    MenuBarItems [Actions_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("actionMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    ActionMenu = XtVaCreateManagedWidget ("actionMenu",
		flatButtonsWidgetClass,
	        (Widget) MenuBarItems [Actions_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) ActionItems,
		XtNnumItems,		(XtArgVal) numActionItems,
		0);

    MenuBarItems [Help_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("helpMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    (void) XtVaCreateManagedWidget ("helpMenu",
		flatButtonsWidgetClass,
	        (Widget) MenuBarItems [Help_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) HelpItems,
		XtNnumItems,		(XtArgVal) XtNumber (HelpItems),
		0);

    menuBar = XtVaCreateManagedWidget ("menuBar",
		flatButtonsWidgetClass, form,
		XtNvPad,		(XtArgVal) 6,
		XtNhPad,		(XtArgVal) 6,
		XtNmenubarBehavior,	(XtArgVal) True,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) MenuBarItems,
		XtNnumItems,		(XtArgVal) XtNumber (MenuBarItems),
		0);

    IconBox = BuildIconBox (form);
    OlVaFlatSetValues (PrinterMenu, Delete_Button,
		XtNuserData,		(XtArgVal) IconBox,
		0);

    XtVaSetValues (PropMenu,
		XtNclientData,		(XtArgVal) IconBox,
		0);

    MakeIcon (TopLevel);

    XtSetMappedWhenManaged (TopLevel, True);
    /* Because of a "bug" in the intrinsics, we must manually map the shell--
     * XtSetMappedWhenManaged won't do it for us.
     */
    XtMapWidget (TopLevel);
    XtAppMainLoop (AppContext);
} /* End of main () */

/* Make Icon box and fill with currently known printers */
static Widget
BuildIconBox (Widget parent)
{
    IconItem		*items;
    int			itemCnt;
    Widget		iconbox;
    Widget		scrolledWin;
    extern void		DrawIcon ();

    GetActivePrinters(parent, &items, &itemCnt, ICONBOX_WIDTH, ICONBOX_HEIGHT);

    scrolledWin = XtVaCreateManagedWidget ("scrolledWin",
		scrolledWindowWidgetClass, parent,
		XtNyRefName,		(XtArgVal) "menuBar",
		XtNyAddHeight,		(XtArgVal) True,
		XtNxAttachRight,	(XtArgVal) True,
		XtNyAttachBottom,	(XtArgVal) True,
		XtNxResizable,		(XtArgVal) True,
		XtNyResizable,		(XtArgVal) True,
		XtNviewWidth,		(XtArgVal) ICONBOX_WIDTH,
		XtNviewHeight,		(XtArgVal) ICONBOX_HEIGHT,
		0);

    iconbox = XtVaCreateManagedWidget ("iconbox", flatIconBoxWidgetClass,
		scrolledWin,
		XtNdrawProc,		(XtArgVal) DrawIcon,
		XtNpostSelectProc,	(XtArgVal) SelectIcon,
		XtNpostAdjustProc,	(XtArgVal) SelectIcon,
		XtNmovableIcons,	(XtArgVal) False,
		XtNitemFields,		(XtArgVal) IconFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (IconFields),
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) itemCnt,
		XtNclientData,		(XtArgVal) True,
		XtNdropProc,		(XtArgVal) InstallPrt,
		XtNdragCursorProc,	(XtArgVal) CreateIconCursor,
		0);

    return (iconbox);
}	/* End of BuildIconBox () */

/* SelectIcon
 *
 * Set the select state of menu items depending on the number of icons
 * selected.
 */
static void
SelectIcon (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFIconBoxButtonCD	*iconData = (OlFIconBoxButtonCD *) call_data;
    IconItem		*item;
    IconItem		*selectedItem;
    PropertyData	*properties;
    register		i;
    int			numSelectedIcons;
    Boolean		isSerial;
    extern PropertyCntl	ChangeControls;

    numSelectedIcons = 0;
    item = (IconItem *) iconData->item_data.items;
    for (i=0; i<iconData->item_data.num_items; i++, item++)
    {
	if (item->selected)
	{
	    numSelectedIcons++;
	    selectedItem = item;
	    properties = (PropertyData *) selectedItem->properties;
	    isSerial = ((PropertyData *) item->properties)->kind == SerialPort;
	}
    }

    SetButtonState (numSelectedIcons, isSerial);

    /* If the property sheet for this printer is posted, update the
     * properties to reflect the new selection.
     */
    if (ChangeControls.poppedUp)
    {
	if (numSelectedIcons == 1)
	{
	    Cardinal		postPage;

	    if (ChangeControls.posted == Comm_Page &&
		properties->kind != SerialPort)
		postPage = Basic_Page;
	    else
		postPage = ChangeControls.posted;
	    Properties (widget, properties, postPage);
	}
	else if (numSelectedIcons == 0)
	    ChangeControls.owner = (PropertyData *) 0;
    }

    if (PrtStatus.poppedUp)
    {
	if (numSelectedIcons == 1)
	    PrtControl (widget, properties);
	else if (numSelectedIcons == 0)
	    PrtStatus.owner = (PropertyData *) 0;
    }
}	/* End of SelectIcon () */

/* SetButtonState
 *
 * Set the state of the delete and properties button in the printer menu.
 * The delete button is turned on when at least one icon is selected.  The
 * property, default printer, and control buttons are only allowed when
 * exactly one icon is selected.  If the isSerial flag is set, then the
 * communication properties sheet is allowed.
 */
void
SetButtonState (Cardinal numSelected, Boolean isSerial)
{
    if (numSelected > 0)
    {
	if (IsAdmin ())
	{
	    if (!PrinterItems [Delete_Button].sensitive)
		OlVaFlatSetValues (PrinterMenu, Delete_Button,
				   XtNsensitive,	(XtArgVal) True,
				   0);
	}

	if (!ActionItems [Install_Button].sensitive)
	    OlVaFlatSetValues (ActionMenu, Install_Button,
			       XtNsensitive,	(XtArgVal) True,
			       0);
    }
    else
    {
	if (PrinterItems [Delete_Button].sensitive)
	    OlVaFlatSetValues (PrinterMenu, Delete_Button,
			       XtNsensitive,	(XtArgVal) False,
			       0);

	if (ActionItems [Install_Button].sensitive)
	    OlVaFlatSetValues (ActionMenu, Install_Button,
			       XtNsensitive,	(XtArgVal) False,
			       0);
    }

    if (numSelected == 1)
    {
	XtVaSetValues (PropMenu,
		   XtNnumItems,	(XtArgVal) (isSerial ?
			    XtNumber (PropItems) : XtNumber (PropItems) - 1),
		   0);
	if (!PrinterItems [Prop_Button].sensitive)
	    OlVaFlatSetValues (PrinterMenu, Prop_Button,
			       XtNsensitive,	(XtArgVal) True,
			       0);
	if (!ActionItems [Dflt_Button].sensitive)
	    OlVaFlatSetValues (ActionMenu, Dflt_Button,
			       XtNsensitive,	(XtArgVal) True,
			       0);
	if (!ActionItems [Cntl_Button].sensitive)
	    OlVaFlatSetValues (ActionMenu, Cntl_Button,
			       XtNsensitive,	(XtArgVal) True,
			       0);
    }
    else
    {
	if (PrinterItems [Prop_Button].sensitive)
	    OlVaFlatSetValues (PrinterMenu, Prop_Button,
			       XtNsensitive,	(XtArgVal) False,
			       0);
	if (ActionItems [Dflt_Button].sensitive)
	    OlVaFlatSetValues (ActionMenu, Dflt_Button,
			       XtNsensitive,	(XtArgVal) False,
			       0);
	if (ActionItems [Cntl_Button].sensitive)
	    OlVaFlatSetValues (ActionMenu, Cntl_Button,
			       XtNsensitive,	(XtArgVal) False,
			       0);
    }
}	/* End of SetButtonState () */

/* AddCB
 *
 * Add a new printer.  User data in the flat item is the properties
 * data for a new printer;
 */
static void
AddCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
    PropertyData	*properties;

    properties = (PropertyData *) ((MenuItem *) flatData->
				   items) [flatData->item_index].userData;
    Properties (widget, properties, 0);
}	/* End of AddCB () */

/* PropCB
 *
 * Update the properties of a printer.  It is assumed that only one icon is
 * selected in the icon box.  Client data is the icon box widget.
 */
static void
PropCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget		iconBox = (Widget) client_data;
    OlFlatCallData	*callData = (OlFlatCallData *) call_data;
    IconItem		*item;
    Cardinal		cnt;
    register		i;

    XtVaGetValues (iconBox,
		XtNitems,		(XtArgVal) &item,
		XtNnumItems,		(XtArgVal) &cnt,
	        0);

    for (i=cnt; --i>=0; item++)
    {
	if (item->selected)
	    break;
    }

    Properties (widget, (PropertyData *) item->properties,
		callData->item_index);
}	/* End of PropCB () */

/* DfltPrtCB
 *
 * Set the default printer queue.  It is assumed that only one icon is
 * selected in the icon box.
 */
static void
DfltPrtCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*callData = (OlFlatCallData *) call_data;
    IconItem		*item;
    IconItem		*dflt;
    Cardinal		cnt;
    register		i;
    extern DmGlyphPtr	PrtGlyph;
    extern DmGlyphPtr	DfltPrtGlyph;


    XtVaGetValues (IconBox,
		XtNitems,		(XtArgVal) &item,
		XtNnumItems,		(XtArgVal) &cnt,
	        0);

    /* Reset the current default and set the new default. */
    for (i=0; i<cnt; i++, item++)
    {
	if (item->glyph == (XtArgVal) DfltPrtGlyph)
	{
	    item->glyph = (XtArgVal) PrtGlyph;
	    OlFlatRefreshItem (IconBox, i, True);
	}

	if (item->selected)
	{
	    dflt = item;
	    item->glyph = (XtArgVal) DfltPrtGlyph;
	    OlFlatRefreshItem (IconBox, i, True);
	}
    }

    /* Store the default value in .Xdefaults and as a desktop property. */
    SaveDefault ((char *) dflt->lbl);
}	/* End of DfltPrtCB () */

/* SaveDefault
 *
 * Save the name of the default printer in a .Xdefaults resource and in dtm
 * property.
 */
void
SaveDefault (char *name)
{
    DtRequest	request;

    InitializeResourceBuffer ();
    AppendToResourceBuffer ("", "*defaultPrinter", name);
    SendResourceBuffer (XtDisplay (TopLevel), XtWindow (TopLevel), 0, APPNAME);

    memset (&request, 0, sizeof (request));
    request.set_property.rqtype= DT_SET_DESKTOP_PROPERTY;
    request.set_property.name = _DEFAULT_PRINTER;
    request.set_property.value = name;
    request.set_property.attrs = 0;
    (void) DtEnqueueRequest (XtScreen (TopLevel),
			     _DT_QUEUE (XtDisplay(TopLevel)),
			     _DT_QUEUE (XtDisplay(TopLevel)),
			     XtWindow (TopLevel), &request);
}	/* End of SaveDefault () */

/* ControlCB
 *
 * Status and control of printer queue.  It is assumed that only one icon is
 * selected in the icon box.
 */
static void
ControlCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*callData = (OlFlatCallData *) call_data;
    IconItem		*item;
    Cardinal		cnt;
    register		i;

    XtVaGetValues (IconBox,
		XtNitems,		(XtArgVal) &item,
		XtNnumItems,		(XtArgVal) &cnt,
	        0);

    for (i=cnt; --i>=0; item++)
    {
	if (item->selected)
	    break;
    }

    PrtControl (widget, (PropertyData *) item->properties);
}	/* End of ControlCB () */

/* ExitCB
 *
 * The says about all you need to know.
 */
static void
ExitCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    exit (0);
}	/* End of ExitCB () */

/* HelpCB
 *
 * Display help.  userData in the item is a pointer to the HelpText data.
 */
void
HelpCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
    MenuItem		*selected;

    selected = (MenuItem *) flatData->items + flatData->item_index;
    DisplayHelp (widget, (HelpText *) selected->userData);
}	/* End of HelpCB () */

/* MakeIcon
 *
 * Create the process icon
 */
static void
MakeIcon (Widget toplevel)
{
    DmGlyphPtr	glyph;
    Pixmap	icon;
    Pixmap	mask;

    glyph = DmGetPixmap (XtScreen (toplevel), "prtset48.icon");
    if (glyph)
    {
	icon = glyph->pix;
	mask = glyph->mask;
    }
    else
	icon = mask = (Pixmap) 0;

    XtVaSetValues (toplevel,
		XtNiconPixmap,		(XtArgVal) icon,
		XtNiconMask,		(XtArgVal) mask,
		XtNiconName,		(XtArgVal) GetStr (TXT_setup),
		0);
} /* End of MakeIcon () */

/* SetLabels
 *
 * Set menu item labels and mnemonics.
 */
void
SetLabels (MenuItem *items, int cnt)
{
    char	*mnem;

    for ( ; --cnt>=0; items++)
    {
	items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
	mnem = GetStr ((char *) items->mnem);
	items->mnem = (XtArgVal) mnem [0];
    }
}	/* End of SetLabels */

/* SetButtonLbls
 *
 * Set button item labels.
 */
void
SetButtonLbls (ButtonItem *items, int cnt)
{
    for ( ; --cnt>=0; items++)
	items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
}	/* End of SetButtonLbls */

/* SetHelpLabels
 *
 * Set strings for help text.
 */
void
SetHelpLabels (HelpText *help)
{
    help->title = GetStr (help->title);
    if (help->section)
	help->section = GetStr (help->section);
}	/* End of SetHelpLabels */

/* AcknowledgeDtReply
 *
 * Get reply to dtm request.
 */
static void
AcknowledgeDtReply (Widget widget, XtPointer client_data, XEvent *xevent,
		    Boolean *pContDisp)
{
    DtReply	reply;
    int		ret;

    if (xevent->type != SelectionNotify ||
	(xevent->xselection.selection != _DT_QUEUE(XtDisplay(widget))))
	return;

    memset (&reply, 0, sizeof (reply));
    ret = DtAcceptReply (XtScreen(widget), xevent->xselection.selection,
			 XtWindow (widget), &reply);

}				/* End of AcknowledgeDtReply () */

/* DisplayHelp
 *
 * Send a message to dtm to display a help window.  If help is NULL, then
 * ask dtm to display the help desk.
 */
void
DisplayHelp (Widget widget, HelpText *help)
{
    DtRequest			*req;
    static DtDisplayHelpRequest	displayHelpReq;
    Display			*display = XtDisplay (widget);
    Window			win = XtWindow (XtParent (XtParent (widget)));

    req = (DtRequest *) &displayHelpReq;
    displayHelpReq.rqtype = DT_DISPLAY_HELP;
    displayHelpReq.serial = 0;
    displayHelpReq.version = 1;
    displayHelpReq.client = win;
    displayHelpReq.nodename = NULL;

    if (help)
    {
	displayHelpReq.source_type =
	    help->section ? DT_SECTION_HELP : DT_TOC_HELP;
	displayHelpReq.app_name = AppName;
	displayHelpReq.app_title = AppTitle;
	displayHelpReq.title = help->title;
	displayHelpReq.help_dir = NULL;
	displayHelpReq.file_name = help->file;
	displayHelpReq.sect_tag = help->section;
    }
    else
	displayHelpReq.source_type = DT_OPEN_HELPDESK;

    (void)DtEnqueueRequest(XtScreen (widget), _HELP_QUEUE (display),
			   _HELP_QUEUE (display), win, req);
}	/* End of DisplayHelp () */

/* CreateIconCursor
 *
 * Create a cursor marginally useful for dragging icons around.  call_data
 * is a OlFlatDragCursorCallData.
 */
static void
CreateIconCursor (Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatDragCursorCallData	*cursorData =
					(OlFlatDragCursorCallData *) call_data;
    Display			*dpy = XtDisplay(w);
    DmGlyphPtr			glyph;
    XColor			white;
    XColor			black;
    static unsigned int		xHot, yHot;
    static Cursor		cursor;
    static Boolean		first = True;

    if (first)
    {
	XColor junk;

	first = False;
	XAllocNamedColor (dpy, DefaultColormapOfScreen (XtScreen (w)),
			  "white", &white, &junk);
	XAllocNamedColor (dpy, DefaultColormapOfScreen (XtScreen (w)),
			  "black", &black, &junk);

	glyph = DmGetCursor (XtScreen (w), AppResources.icon);
	if (glyph)
	{
	    xHot = glyph->width / 2;
	    yHot = glyph->height / 2;
	    cursor = XCreatePixmapCursor (dpy, glyph->pix, glyph->mask,
					  &black, &white, xHot, yHot);
	}
	else
	{
	    cursor = None;
	    xHot = yHot = 0;
	}
    }
    
    cursorData->yes_cursor = cursor;
    cursorData->x_hot = xHot;
    cursorData->y_hot = yHot;
}	/* End of CreateIconCursor () */
