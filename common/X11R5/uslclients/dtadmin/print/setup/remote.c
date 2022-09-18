/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/remote.c	1.15"
#endif

/* Administer remote systems list for lp */

#include <stdio.h>
#include <string.h>
#include <search.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/PopupWindo.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

#include <DtI.h>

#include <lp.h>
#include <systems.h>

#include "properties.h"
#include "error.h"
#include "lpsys.h"

#define WHITESPACE	" \t\n"
#define HOST_ALLOC_SIZE	50

typedef struct {
    XtPointer	name;
    XtPointer	OS;
    XtPointer	glyph;
    XtPointer	enabled;
} FormatData;

typedef struct {
    XtArgVal	formatData;
    XtArgVal	printerOS;
} ListItem;

typedef struct {
    Widget	listWidget;
    ListItem	*listItems;
    FormatData	*list;
    unsigned	cnt;
    unsigned	allocated;
} HostList;

static void RmtApplyCB (Widget, XtPointer, XtPointer);
static void RmtResetCB (Widget, XtPointer, XtPointer);
static void RmtSelectCB (Widget, XtPointer, XtPointer);
static void RmtPopdownCB (Widget, XtPointer, XtPointer);

static Widget	MakeRemoteSheet (Widget);
static ListItem	*GetItems (HostList *);
static void	FreeHosts (HostList *);
static Boolean	ReadHosts (HostList *);
static int	HostCmp ();

static Widget	RemoteSheet;
static Pixmap	PrtGlyph;
static HostList	Hosts;

static char	*EnabledStr;
static char	*S5Str;
static char	*BSDStr;

static String	ListFields [] = {
    XtNformatData, XtNuserData,
};

static HelpText RmtHelp = {
    TXT_rmtHelp, HELP_FILE, TXT_rmtHelpSect,
};

/* Lower Control Area buttons */
static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_save, (XtArgVal) MNEM_save, (XtArgVal) True,
	  (XtArgVal) RmtApplyCB, (XtArgVal) True, },	/* Save */
    { (XtArgVal) TXT_reset, (XtArgVal) MNEM_reset, (XtArgVal) True,
	  (XtArgVal) RmtResetCB, },			/* Reset */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &RmtHelp, },			/* Help */
};

static FormatData	ColHdrs [1];
static ListItem		ColItem [] = {
    (XtArgVal) ColHdrs,
};

/* RemoteSystemsCB
 *
 * Popup the remote access property sheet.  If it doesn't exist, make it; if
 * it does, bring it to the top.
 */
void
RemoteSystemsCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget	uca;
    Widget	scrolledWindow;
    Widget	list;

    if (RemoteSheet)
    {
	/* Property sheet already exists */
	XRaiseWindow (XtDisplay (RemoteSheet),
		      XtWindow (RemoteSheet));
    }
    else
    {
	/* Make one */
	RemoteSheet = MakeRemoteSheet (widget);
	XtPopup (RemoteSheet, XtGrabNone);
    }
}	/* End of RemoteSystemsCB () */

/* MakeRemoteSheet
 *
 * Make remote property sheet
 */
static Widget
MakeRemoteSheet (Widget parent)
{
    Widget		popup;
    Widget		uca;
    Widget		lca;
    Widget		lcaMenu;
    Widget		ucaMenuShell;
    Widget		ucaMenu;
    Widget		scrolledWindow;
    static char		format [25];
    static char		*titleLbl;
    static Boolean	first = True;

    /* Set Labels */
    if (first)
    {
	DmGlyphPtr	glyph;
	Dimension	glyphWidth;

	first = False;
	titleLbl = GetStr (TXT_rmtlpsys);

	SetLabels (CommandItems, XtNumber (CommandItems));
	SetHelpLabels (&RmtHelp);

	if (!IsAdmin ())
	    CommandItems [Apply_Button].sensitive = (XtArgVal) False;

	glyphWidth = OlMMToPixel (OL_HORIZONTAL, 2);
	glyph = DmGetPixmap (XtScreen (parent), "smprt.icon");
	if (glyph)
	{
	    PrtGlyph = glyph->pix;
	    glyphWidth += glyph->width;
	}

	EnabledStr = GetStr (TXT_enabled);
	sprintf (format, "%%20s%%8s%%%dp%%15s", glyphWidth);
	ColHdrs [0].name = (XtPointer) GetStr (TXT_sysName);
	ColHdrs [0].OS = (XtPointer) GetStr (TXT_osType);
	ColHdrs [0].enabled = (XtPointer) GetStr (TXT_rmtAccess);
    }

    /* Get the systems named in /etc/hosts */
    if (!ReadHosts (&Hosts))
    {
	RmtError ();
	return ((Widget) 0);
    }

    /* Create property sheet */
    popup = XtVaCreatePopupShell ("properties",
		popupWindowShellWidgetClass, parent,
		XtNtitle,		(XtArgVal) titleLbl,
		0);

    XtVaGetValues (popup,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		0);

    /* Create a list of systems in /etc/hosts.  Add a flat list above this
     * with a single item in it to act as column headers.
     */
    Hosts.listWidget = XtVaCreateManagedWidget ("colHdr",
		flatListWidgetClass, uca,
		XtNviewHeight,		(XtArgVal) 1,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) 0,
		XtNitems,		(XtArgVal) ColItem,
		XtNnumItems,		(XtArgVal) 1,
		XtNitemFields,		(XtArgVal) ListFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ListFields),
		XtNformat,		(XtArgVal) format,
		XtNtraversalOn,		(XtArgVal) False,
		0);

    scrolledWindow = XtVaCreateManagedWidget ("scrolledWindow",
		scrolledWindowWidgetClass, uca,
		0);

    Hosts.listItems = GetItems (&Hosts);

    Hosts.listWidget = XtVaCreateManagedWidget ("remoteList",
		flatListWidgetClass, scrolledWindow,
		XtNviewHeight,		(XtArgVal) 9,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) RmtSelectCB,
		XtNitems,		(XtArgVal) Hosts.listItems,
		XtNnumItems,		(XtArgVal) Hosts.cnt,
		XtNitemFields,		(XtArgVal) ListFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ListFields),
		XtNformat,		(XtArgVal) format,
		0);

    /* We want an "apply" and "reset" buttons in both the lower control
     * area and in a popup menu on the upper control area.
     */
    lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNclientData,		(XtArgVal) Hosts.listWidget,
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
		XtNclientData,		(XtArgVal) Hosts.listWidget,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    OlAddDefaultPopupMenuEH (uca, ucaMenuShell);

    /* Add callbacks to verify and destroy all widget when the property sheet
     * goes away
     */
    XtAddCallback (popup, XtNverify, VerifyCB, (XtPointer) 0);
    XtAddCallback (popup, XtNpopdownCallback, RmtPopdownCB,
		   (XtPointer) Hosts.listItems);

    return (popup);
}	/* End of MakePropertySheet () */

/* RmtSelectCB
 *
 * Select system
 */
static void
RmtSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;
    FormatData		*formatData;

    formatData = (FormatData *) ((ListItem *) pFlatData->items +
				 pFlatData->item_index)->formatData;
    if (!formatData->OS)
	formatData->OS = (XtPointer) S5Str;
    else if (formatData->OS == S5Str)
	formatData->OS = (XtPointer) BSDStr;
    else
	formatData->OS = (XtPointer) 0;
    if (formatData->OS)
    {
	formatData->glyph = (XtPointer) PrtGlyph;
	formatData->enabled = (XtPointer) EnabledStr;
    }
    else
	formatData->glyph = formatData->enabled = (XtPointer) 0;

    OlVaFlatSetValues (widget, pFlatData->item_index,
		XtNformatData,		(XtArgVal) formatData,
		XtNset,			(XtArgVal) False,
		0);
}	/* End of RemoteSelectCB () */

/* RmtPopdownCB
 *
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 */
static void
RmtPopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtDestroyWidget (widget);
    FreeHosts (&Hosts);
    XtFree (client_data);
    RemoteSheet = (Widget) 0;
}	/* End of RmtPopdownCB () */

/* RmtApplyCB
 *
 * Remote apply callback.  Inform lp of the changes to the remotes systems
 * and update the internal list.
 */
static void
RmtApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget		listWidget = (Widget) client_data;
    ListItem		*pItem;
    Boolean		popdownOK;
    register FormatData	*pHost;
    register		i;
    static char		*sysMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	sysMsg = GetStr (TXT_badSystem);
    }

    popdownOK = True;

    XtVaGetValues (listWidget,
		XtNitems,	(XtArgVal) &pItem,
		0);

    for (pHost=Hosts.list, i=0; i<Hosts.cnt; i++, pHost++, pItem++)
    {
	if (pHost->OS != (XtPointer) pItem->printerOS)
	{
	    int	os;

	    if (pHost->OS)
		os = (pHost->OS == S5Str) ? S5_OS : BSD_OS;
	    else
		os = No_OS;

	    if (LpSystem ((char *) pHost->name, os))
		pItem->printerOS = (XtArgVal) pHost->OS;
	    else
		popdownOK = False;
	}
    }

    (void) LpSystem ((char *) 0, No_OS);
    if (!popdownOK)
    {
	Error (widget, sysMsg, OL_ERROR);
	RmtResetCB (widget, client_data, call_data);
    }
    else
	BringDownPopup (RemoteSheet);
}	/* End of RmtApplyCB () */

/* Reset callback
 *
 * Reset the list to the original values.  client_data refers to the list
 * widget.
 */
static void
RmtResetCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget		listWidget = (Widget) client_data;
    ListItem		*pItem;
    register FormatData	*pHost;
    register		i;

    XtVaGetValues (listWidget,
		XtNitems,	(XtArgVal) &pItem,
		0);

    for (pHost=Hosts.list, i=0; i<Hosts.cnt; i++, pHost++, pItem++)
    {
	if (pHost->OS != (XtPointer) pItem->printerOS)
	{
	    pHost->OS = (XtPointer) pItem->printerOS;
	    if (pHost->OS)
	    {
		pHost->glyph = (XtPointer) PrtGlyph;
		pHost->enabled = (XtPointer) EnabledStr;
	    }
	    else
	    {
		pHost->glyph = (XtPointer) 0;
		pHost->enabled = (XtPointer) 0;
	    }
	    OlVaFlatSetValues (listWidget, i,
			       XtNformatData,	(XtArgVal) pHost,
			       0);
	    OlFlatRefreshItem (listWidget, i, True);
	}
    }
}	/* End of RmtResetCB () */

/* AddRemote
 *
 * Check if a remote system is able to talk lp.  If not, try to add it to
 * the lp system database.  If the remote access property sheet is posted,
 * "Apply" the one item.  Return True if the operation worked, False if the
 * system does not exist in /etc/hosts or could do the system command.
 */
Boolean
AddRemote (char *name, int osType)
{
    FormatData	key;
    FormatData	*format;
    ListItem	*item;
    Cardinal	itemIndex;
    char	*OS;
    Boolean	rc;

    if (!RemoteSheet)
    {
	/* Popup is not posted--we have to read /etc/hosts. */
	if (!ReadHosts (&Hosts))
	{
	    RmtError ();
	    return (False);
	}
    }

    OS = (osType == S5_OS) ? S5Str : BSDStr;

    /* Find the name in the host list */
    key.name = (XtPointer) name;
    format = bsearch (&key, Hosts.list, Hosts.cnt, sizeof (key), HostCmp);
    if (!format)
	rc = False;
    else
    {
	/* Add the system to lp and update the property sheet.  If the remote
	 * property sheet is not posted, then only the format data in Hosts
	 * is valid.  If the sheet is posted, then we have to compare the
	 * operating system value to the applied value, which is in the list
	 * item.
	 */
	rc = True;
	itemIndex = format - Hosts.list;
	if (RemoteSheet)
	{
	    if ((char *) Hosts.listItems [itemIndex].printerOS != OS)
	    {
		if (!LpSystem (name, osType))
		    rc = False;
		LpSystem ((char *) 0, No_OS);
		format->glyph = (XtPointer) PrtGlyph;
		format->enabled = (XtPointer) EnabledStr;
		format->OS = (XtPointer) OS;
		Hosts.listItems [itemIndex].printerOS = (XtArgVal) OS;
		OlFlatRefreshItem (Hosts.listWidget, itemIndex, True);
	    }
	}
	else
	{
	    if (Hosts.list [itemIndex].OS != OS)
	    {
		if (!LpSystem (name, osType))
		    rc = False;
		LpSystem ((char *) 0, No_OS);
	    }
	}
    }

    if (!RemoteSheet)
	FreeHosts (&Hosts);

    return (rc);
}	/* End of AddRemote () */

/* ReadHosts
 *
 * Read /etc/hosts file.  Return True if there was no problem.
 */
static Boolean
ReadHosts (HostList *hosts)
{
    FILE		*hostFile;
    SYSTEM		*system;
    FormatData		*remoteSys;
    FormatData		searchkey;
    char		*token;
    char		buf [256];
    static Boolean	first = True;

    if (first)
    {
	first = False;

	S5Str = GetStr (TXT_sysv);
	BSDStr = GetStr (TXT_bsd);
    }

    if (!(hostFile = fopen ("/etc/hosts", "r")))
	return False;

    while (fgets (buf, 256, hostFile))
    {
	/* remove comments */
	token = strchr (buf, '#');
	if (token)
	    *token = 0;

	/* the host name is the second token */
	token = strtok (buf, WHITESPACE);
	if (token)
	{
	    token = strtok (NULL, WHITESPACE);
	    if (token)
	    {
		if (hosts->cnt >= hosts->allocated)
		{
		    hosts->allocated += HOST_ALLOC_SIZE;
		    hosts->list = (FormatData *)
			XtRealloc ((char *) hosts->list,
				   hosts->allocated * sizeof (FormatData));
		}

		hosts->list [hosts->cnt].name = (XtPointer) strdup (token);
		hosts->list [hosts->cnt].OS = (XtPointer) 0;
		hosts->list [hosts->cnt].glyph = (XtPointer) 0;
		hosts->list [hosts->cnt++].enabled = (XtPointer) 0;
	    }
	}
    }

    /* Sort the host list */
    qsort ((char *) hosts->list, hosts->cnt, sizeof (FormatData), HostCmp);

    /* Get those systems that we already talk to */
    while (system = getsystem ("all"))
    {
	searchkey.name = system->name;
	remoteSys = bsearch ((void *) &searchkey, (void *) hosts->list,
			  hosts->cnt, sizeof (searchkey), HostCmp);
	if (remoteSys)
	{
	    remoteSys->glyph = (XtPointer) PrtGlyph;
	    remoteSys->enabled = (XtPointer) EnabledStr;
	    remoteSys->OS = (XtPointer) ((system->protocol == S5_PROTO) ?
					S5Str : BSDStr);
	}
    }

    fclose (hostFile);
    return True;
}	/* End of ReadHosts () */

/* FreeHosts
 *
 * Free all old host data.  The list itself is kept around to reduce mallocing
 * on later calls.
 */
static void
FreeHosts (HostList *hosts)
{
    register	i;

    for (i=hosts->cnt; --i>=0; )
	XtFree (hosts->list [i].name);

    hosts->cnt = 0;
}	/* End of FreeHosts () */

/* GetItems
 *
 * Format the host list into items for the flat list.
 */
static ListItem *
GetItems (HostList *hosts)
{
    ListItem		*items;
    register ListItem	*pItem;
    register FormatData	*pData;
    register		i;

    items = pItem = (ListItem *) XtMalloc (hosts->cnt * sizeof (ListItem));
    pData = hosts->list;
    for (i=hosts->cnt; --i>=0; pItem++, pData++)
    {
	pItem->formatData = (XtArgVal) pData;
	pItem->printerOS = (XtArgVal) pData->OS;
    }

    return (items);
}	/* End of GetItems () */

/* HostCmp
 *
 * Comparison function for Host List sorter.
 */
static int
HostCmp (FormatData *h1, FormatData *h2)
{
    return (strcoll (h1->name, h2->name));
}	/* End of HostCmp () */

/* IsNetworking
 *
 * Determine if networking software has been installed on the system.  This
 * function assumes that if lpNet exists, then so must networking.  Not
 * bulletproof, but should be ok in practice.
 */
Boolean
IsNetworking (void)
{
    static		first = True;
    static Boolean	networking;

    if (first)
    {
	first = False;
	if (access ("/usr/lib/lp/lpNet", 0) == 0)
	    networking = True;
	else
	    networking = False;
    }

    return (networking);
}	/* End of IsNetworking () */

RmtError ()
{
}
