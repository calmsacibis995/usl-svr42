/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/install.c	1.17"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Xol/PopupWindo.h>

#include <Desktop.h>

#include "properties.h"
#include "printer.h"
#include "error.h"

static char		*BadInstallMsg;

extern void	InstallPrt (Widget, XtPointer, XtPointer);
extern void	ConfirmCB (Widget, XtPointer, XtPointer);
static void	CleanUpCB (Widget, XtPointer, XtPointer);

static char	*Home;
static char	*DfltToolPath;

/* Popup Controls for installing printer use icons */
static Widget		PopupWindow;
static TxtChoice	ToolboxCtrl;

static HelpText InstallHelp = {
    TXT_installHelp, HELP_FILE, TXT_installHelpSect,
};

static MenuItem InstallItems [] = {
    { (XtArgVal) TXT_install, (XtArgVal) MNEM_install, (XtArgVal) True,
	  (XtArgVal) InstallPrt, (XtArgVal) True, },	/* Install */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &InstallHelp, },			/* Help */
};


/* InstallCB
 *
 * Install printer use icons in the control room
 */
void
InstallCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget	lca;
    Widget	uca;
    register	i;
    static char	*toolboxLbl;
    static char	*titleLbl;

    /* If first time, create the popup and its controls */
    if (!PopupWindow)
    {
	titleLbl = GetStr (TXT_installTitle);
	toolboxLbl = GetStr (TXT_toolbox);
	SetLabels (InstallItems, XtNumber (InstallItems));
	SetHelpLabels (&InstallHelp);

	if (!Home)
	{
	    Home = getenv ("HOME");
	    if (!Home)
		Home = "";
	}

	/* Remove all trailing slashes */
	for (i=strlen(Home); --i>=0 && Home[i]=='/'; )
	    ;	/* Do Nothing! */
	Home [i+1] = 0;

	DfltToolPath = XtMalloc (strlen (Home) + 1);
	sprintf (DfltToolPath, "%s", Home);

	PopupWindow = XtVaCreatePopupShell ("install",
		popupWindowShellWidgetClass, widget,
		XtNtitle,		(XtArgVal) titleLbl,
		0);

	XtVaGetValues (PopupWindow,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		0);

	MakeText (uca, toolboxLbl, &ToolboxCtrl, 40);

	(void) XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNclientData,		(XtArgVal) False,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) InstallItems,
		XtNnumItems,		(XtArgVal) XtNumber (InstallItems),
		0);

	XtAddCallback (PopupWindow, XtNverify, VerifyCB, (XtPointer) 0);
    }


    XtVaSetValues (ToolboxCtrl.txt,
		XtNstring,		(XtArgVal) DfltToolPath,
		0);
    XtPopup (PopupWindow, XtGrabNone);
}	/* End of InstallCB () */

/* InstallPrt
 *
 * Install printer use icons in a toolbox.  client_data is False if the
 * location to install printers was specified by text field and True if
 * by drag and drop.  If True, call_data is a OlFlatDropCallData structure.
 * (If False, call data is a OlFlatCallData structure, but it is ignored.)
 */
void
InstallPrt (Widget widget, XtPointer client_data, XtPointer call_data)
{
    IconItem		*item;
    Cardinal		cnt;
    DtRequest		request;
    char		*name;
    char		**pName;
    char		**fileList;
    int			fileCnt;
    int			bufLen;
    char		dfltsFileName [128];
    char		toolBoxName [128];
    char		msg [300];
    char		*installedBuf;
    char		*errorBuf;
    char		*dfltsEnd;
    char		*toolBoxEnd;
    OlFlatDropCallData	*dropData;
    struct stat		statbuf;
    register		i;
    extern Widget	IconBox;
    static int		owner;
    static int		group;
    static char		*prtDir;
    static char		*prtDirMsg;
    static char		*installedMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	prtDirMsg = GetStr (TXT_noPrtDir);
	BadInstallMsg = GetStr (TXT_badInstall);
	installedMsg = GetStr (TXT_installed);

	owner = getuid ();
	group = getgid ();

	if (!Home)
	{
	    Home = getenv ("HOME");
	    if (!Home)
		Home = "";
	}
	
	prtDir = XtMalloc (strlen (Home) + 9 + 1);
	sprintf (prtDir, "%s/.printer", Home);
    }

    /* Check if the printer directory exists.  If not, try to create it.
     * As we might be running with privilege, we need to change the owner
     * back to the real owner.
     */
    if (stat (prtDir, &statbuf) != 0)
    {
	if (errno != ENOENT ||
	     mkdir (prtDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
	{
	    Error (widget, prtDirMsg, OL_ERROR);
	    return;
	}
	(void) chown (prtDir, owner, group);
    }
    else
    {
	if (!S_ISDIR (statbuf.st_mode))
	{
	    Error (widget, prtDirMsg, OL_ERROR);
	    return;
	}
    }

    sprintf (dfltsFileName, "%s/", prtDir);
    dfltsEnd = dfltsFileName + strlen (dfltsFileName);

    if (client_data)
    {
	dropData = (OlFlatDropCallData *) call_data;
	item = (IconItem *) dropData->item_data.items;
	cnt = dropData->item_data.num_items;
	i = dropData->item_data.item_index;
    }
    else
    {
	XtVaGetValues (IconBox,
		       XtNitems,		(XtArgVal) &item,
		       XtNnumItems,		(XtArgVal) &cnt,
		       0);

	name = GetText (&ToolboxCtrl);
	if (!name || !*name)
	    strcpy (toolBoxName, DfltToolPath);
	else
	    if (*name != '/')
		sprintf (toolBoxName, "%s/%s", DfltToolPath, name);
	    else
		strcpy (toolBoxName, name);

	toolBoxEnd = toolBoxName + strlen (toolBoxName);
	if (toolBoxEnd [-1] != '/')
	{
	    strcpy (toolBoxEnd, "/");
	    toolBoxEnd++;
	}
    }

    /* Get the printers to install.  If using drag and drop, if the drag item
     * is selected, then get all selected items from the icon box.  If the
     * drag item is not selected, only the one printer is installed.  If
     * not using drag and drop, use all selected printers.
     */
    if (client_data && !item [i].selected)
    {
	fileList = (char **) XtMalloc (3 * sizeof (char *));
	fileList [0] = (char *) item [i].lbl;
	fileCnt = 1;
	bufLen = strlen ((char *) item->lbl) + 3;
    }
    else
    {
	fileList = (char **) XtMalloc ((cnt+2) * sizeof (char *));

	fileCnt = 0;
	bufLen = 1;
	for (i=0; i++<cnt; item++)
	{
	    if (item->selected)
	    {
		fileList [fileCnt++] = (char *) item->lbl;
		bufLen += strlen ((char *) item->lbl) + 2;  /* plus ", " */
	    }
	}
    }

    fileList [fileCnt] = (char *) 0;

    errorBuf = XtMalloc (bufLen);
    errorBuf [0] = 0;
    if (!client_data)
    {
	installedBuf = XtMalloc (bufLen);
	installedBuf [0] = 0;
    }

    /* For each printer, make a defaults file.  If we are not using drag and
     * drop, go ahead and make a symbolic link in the installation location
     * to this file.
     */
    i = 0;
    for (pName=fileList; *pName; pName++)
    {
	/* Check if a defaults file exists for the printer.  If not,
	 * make one.  Set it's owner to be the real owner.
	 */
	strcpy (dfltsEnd, *pName);
	if (access (dfltsFileName, R_OK))
	{
	    int	fd;

	    fd = creat (dfltsFileName, 0644);
	    if (fd < 0)
	    {
		strcat (errorBuf, ", ");
		strcat (errorBuf, *pName);
		fileCnt--;
		continue;
	    }
	    close (fd);
	    (void) chown (dfltsFileName, owner, group);
	}

	if (!client_data)
	{
	    strcpy (toolBoxEnd, *pName);
	    if (symlink (dfltsFileName, toolBoxName))
	    {
		strcat (errorBuf, ", ");
		strcat (errorBuf, *pName);
		fileCnt--;
		continue;
	    }

	    strcat (installedBuf, ", ");
	    strcat (installedBuf, *pName);
	}
	else
	    fileList [i++] = strdup (dfltsFileName);
    }

    if (fileCnt > 0)
    {
	if (client_data)
	{
	    /* Drag and drop message to dtm */
	    fileList [i++] = (char *) 0;
	    fileList [i] = strdup (errorBuf);
	    DtNewDnDTransaction(widget, fileList, DT_B_STATIC_LIST,
				dropData->root_info->root_x,
				dropData->root_info->root_y,
				dropData->ve->xevent->xbutton.time,
				dropData->dst_info->window,
				DT_LINK_OP, NULL, CleanUpCB,
				(XtPointer) fileList);
	}
	else
	{
	    /* Protocol message to dtm just to sync the window */
	    memset (&request, 0, sizeof (request));
	    request.sync_folder.rqtype= DT_SYNC_FOLDER;
	    request.sync_folder.path = toolBoxName;
	    (void) DtEnqueueRequest (XtScreen (widget),
				     _DT_QUEUE (XtDisplay (widget)),
				     _DT_QUEUE (XtDisplay (widget)),
				     XtWindow (widget), &request);

	    msg [0] = 0;
	    if (installedBuf [0])
		sprintf (msg, installedMsg, installedBuf + 2);

	    if (errorBuf [0])
	    {
		sprintf (msg + strlen (msg), "%s%s", BadInstallMsg,
			 errorBuf + 2);
		Error (widget, msg, OL_ERROR);
	    }
	    else
		ErrorConfirm (widget, msg, OL_INFORMATION,
			      ConfirmCB, (XtPointer) PopupWindow);

	    XtFree ((char *) fileList);
	}
    }
    else
    {
	if (errorBuf [0])
	{
	    sprintf (msg, "%s%s", BadInstallMsg, errorBuf + 2);
	    Error (widget, msg, OL_ERROR);
	}

	XtFree ((char *) fileList);
    }

    XtFree (errorBuf);
    if (!client_data)
	XtFree (installedBuf);
}	/* End of InstallPrt () */

/* CleanUpCB
 *
 * Clean up after a drag and drop operation.  Check to see if the links
 * got created correctly, and remove the file list.  client_data is the
 * file list, null terminated.  Additional errors are in the string
 * immediately following the null (yes, it's a hack).
 */
static void
CleanUpCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    char	**pName = (char **) client_data;
    char	*msg;
    char	*errorBuf;
    int		bufLen;
    int		allocLen;

    bufLen = 1;
    errorBuf = XtMalloc (allocLen = 256);
    errorBuf [0] = 0;

    for ( ; *pName; pName++)
    {
	if (access (*pName, F_OK))
	{
	    char	*name;
	    strcat (errorBuf, ", ");
	    name = strrchr (*pName, '/') + 1;
	    bufLen += strlen (name) + 2;
	    if (bufLen >= allocLen)
	    {
		allocLen = bufLen + 50;
		errorBuf = XtRealloc (errorBuf, allocLen);
	    }
	    strcat (errorBuf, name);
	}
	XtFree (*pName);
    }

    if (errorBuf [0])
    {
	msg = XtMalloc (bufLen + strlen (BadInstallMsg) + 1);
	sprintf (msg, "%s%s", BadInstallMsg, errorBuf + 2);
	Error (widget, msg, OL_ERROR);
	XtFree (msg);
    }

    XtFree (errorBuf);
    XtFree (client_data);
}	/* End of CleanUpCB () */
