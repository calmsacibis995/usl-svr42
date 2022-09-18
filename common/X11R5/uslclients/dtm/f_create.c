/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_create.c	1.96.1.70"

/******************************file*header********************************

    Description:
	This file contains the source code for creating folder-related
	UI objects.
*/
						/* #includes go here	*/
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xol/OpenLookP.h>		/* for _OlGetShellOfWidget */
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/BulletinBo.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StaticText.h>
#include <Gizmos.h>
#include <MenuGizmo.h>
#include <BaseWGizmo.h>
#include <FileGizmo.h>
#include <ListGizmo.h>
#include <ModalGizmo.h>
#include <PopupGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "fn_find.h"
#include "SWinGizmo.h"
#include "StatGizmo.h"
#include "dm_exit.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

static void	CloseFolderCB(Widget, XtPointer, XtPointer);
static void	ExitCB(Widget, XtPointer, XtPointer);
static void	DetermineLayoutAttributes(DmFolderWinPtr window,
					      DtAttrs * loptions,
					      DtAttrs * coptions);
static void	MakeFolderMenu(MenuGizmo * menu);
static void	FolderWinWMCB(Widget, XtPointer, XtPointer);
static void	PlugRing(char ** visited, char *** head, char ** elem);
					/* public procedures		*/
void		DmApplyShowFullPath(Boolean show);
void		DmCloseFolderWindow(DmFolderWinPtr folder);
void		DmCloseFolderWindows(void);
void		DmCloseWindow(DmWinPtr window);
DmFolderWinPtr	DmFindFolderWindow(Widget w);
DmFolderWinPtr	DmOpenFolderWindow(char * path, DtAttrs attrs,
				   char * geom_str, Boolean iconic);
void		Dm__UpdateVisitedFolders(char * old_path, char * new_path);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define	VIEWTYPE		"viewType"
#define	FOLDERSIZE		"size"
#define	FOLDERICON		"icon"
#define EXTRA_ICON_SLOTS	5

/* Rudimentary macros to help with "ring". */
#define TAIL_(v)	( (v) + MAX_VISITED_FOLDERS - 1 )
#define NEXT_(p, v)	( ((p) < TAIL_(v)) ? (p) + 1 : (v) )
#define PREV_(p, v)	( ((p) == (v)) ? TAIL_(v) : (p) - 1 )

extern char *strndup();

/* Define a mapping for view names and view types */
static const DmMapping view_mapping[] = {
    "DM_ICONIC",	DM_ICONIC,
    "DM_LONG",		DM_LONG,
    "DM_NAME",		DM_NAME,
    NULL,		-1
};

static const DmMapping sort_mapping[] = {
    "BYNAME",		DM_BY_NAME,
    "BYSIZE",		DM_BY_SIZE,
    "BYTYPE",		DM_BY_TYPE,
    "BYTIME",		DM_BY_TIME,
    NULL,		-1
};

/*	MENU DEFINITIONS

	CAUTION: the order of the buttons for some menus below is also
	reflected in enum types in the source file associated with the
	callback or Dtm.h.
*/

#define XA	XtArgVal
#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE
#define B_U	(XtPointer)DM_B_UNDO

static MenuItems ExitMenuItems[] = {
 { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   NULL, DmFileOpenCB,	B_M },
 { True, TXT_FILE_NEW,	  TXT_M_FILE_NEW,    NULL, DmFileNewCB,		B_A },
 { True, TXT_FILE_COPY,   TXT_M_FILE_COPY,   NULL, DmFileCopyCB,	B_M },
 { True, TXT_FILE_RENAME, TXT_M_FILE_RENAME, NULL, DmFileRenameCB,	B_O },
 { True, TXT_FILE_MOVE,   TXT_M_FILE_MOVE,   NULL, DmFileMoveCB,	B_M },
 { True, TXT_FILE_LINK,   TXT_M_FILE_LINK,   NULL, DmFileLinkCB,	B_M },
 { True, TXT_FILE_DELETE, TXT_M_FILE_DELETE, NULL, DmFileDeleteCB,	B_M },
 { True, TXT_FILE_PRINT,  TXT_M_FILE_PRINT,  NULL, DmFilePrintCB,	B_M },
 { True, TXT_FILE_PROP,   TXT_M_FILE_PROP,   NULL, DmFilePropCB,	B_M },
 { True, TXT_FILE_FIND,	  TXT_M_FILE_FIND,   NULL, DmFindCB,		B_A },
 { True, TXT_FILE_DESKTOP_EXIT,	TXT_M_FILE_EXIT, NULL, ExitCB,		B_A },
 { NULL }
};

static MenuItems FileMenuItems[] = {
 { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   NULL, DmFileOpenCB,	B_M },
 { True, TXT_FILE_NEW,	  TXT_M_FILE_NEW,    NULL, DmFileNewCB,		B_A },
 { True, TXT_FILE_COPY,   TXT_M_FILE_COPY,   NULL, DmFileCopyCB,	B_M },
 { True, TXT_FILE_RENAME, TXT_M_FILE_RENAME, NULL, DmFileRenameCB,	B_O },
 { True, TXT_FILE_MOVE,   TXT_M_FILE_MOVE,   NULL, DmFileMoveCB,	B_M },
 { True, TXT_FILE_LINK,   TXT_M_FILE_LINK,   NULL, DmFileLinkCB,	B_M },
 { True, TXT_FILE_DELETE, TXT_M_FILE_DELETE, NULL, DmFileDeleteCB,	B_M },
 { True, TXT_FILE_PRINT,  TXT_M_FILE_PRINT,  NULL, DmFilePrintCB,	B_M },
 { True, TXT_FILE_PROP,   TXT_M_FILE_PROP,   NULL, DmFilePropCB,	B_M },
 { True, TXT_FILE_FIND,	  TXT_M_FILE_FIND,   NULL, DmFindCB,		B_A },
 { True, TXT_FILE_EXIT,   TXT_M_FILE_EXIT,   NULL, CloseFolderCB,	B_A },
 { NULL }
};

static MenuItems EditMenuItems[] = {
 { True, TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   NULL, DmEditSelectAllCB,  B_A},
 { True, TXT_EDIT_UNSELECT, TXT_M_EDIT_UNSELECT, NULL, DmEditUnselectAllCB,B_M},
 { False,TXT_EDIT_UNDO,     TXT_M_EDIT_UNDO,     NULL, DmEditUndoCB,       B_U},
 { NULL }
};

static MenuItems ViewFormatMenuItems[] = {
 { True, TXT_VIEW_ICON,  TXT_M_VIEW_ICON,  NULL, DmViewFormatCB },
 { True, TXT_VIEW_SHORT, TXT_M_VIEW_SHORT, NULL, DmViewFormatCB },
 { True, TXT_VIEW_LONG,  TXT_M_VIEW_LONG,  NULL, DmViewFormatCB },
 { NULL }
};

static MenuItems ViewSortMenuItems[] = {
 { True, TXT_SORT_TYPE, TXT_M_SORT_TYPE, NULL, DmViewSortCB },
 { True, TXT_SORT_NAME, TXT_M_SORT_NAME, NULL, DmViewSortCB },
 { True, TXT_SORT_SIZE, TXT_M_SORT_SIZE, NULL, DmViewSortCB },
 { True, TXT_SORT_TIME, TXT_M_SORT_TIME, NULL, DmViewSortCB },
 { NULL }
};

/* Folder Menu items are built dynamically from "visited" folder paths.
   Initialize the "constant" buttons here so that the text is i18n'ized only
   once (also, the MENU macro can then be used below).
*/
static MenuItems FolderMenuItems[] = {
 { True, TXT_PARENT_FOLDER, TXT_M_PARENT_FOLDER, NULL, DmFolderOpenParentCB },
 { True, TXT_FOLDER_MAP,    TXT_M_FOLDER_MAP,    NULL, DmFolderOpenTreeCB },
 { True, TXT_OTHER_FOLDER,  TXT_M_OTHER_FOLDER,  NULL, DmFolderOpenOtherCB },
 { NULL }
};

static MenuItems HelpMenuItems[] = {
 { True, TXT_HELP_FOLDER,   TXT_M_HELP_FOLDER,   NULL, DmHelpSpecificCB, B_A },
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      NULL, DmHelpTOCCB,      B_A },
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, NULL, DmHelpDeskCB,     B_A },
 { NULL }
};

/* Special help menu for "Main" window */
static MenuItems DesktopHelpMenuItems[] = {
 { True, TXT_HELP_DESKTOP,  TXT_M_HELP_DESKTOP,  NULL, DmHelpSpecificCB, B_A },
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      NULL, DmHelpTOCCB,      B_A },
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, NULL, DmHelpDeskCB,     B_A },
 { NULL }
};

MENU("editmenu",	EditMenu);
MENU("exitmenu",	ExitMenu);
MENU("filemenu",	FileMenu);
MENU("foldermenu",	FolderMenu);
MENU("helpmenu",	HelpMenu);
MENU("dthelpmenu",	DesktopHelpMenu);
MENU("viewformatmenu",	ViewFormatMenu);
MENU("viewsortmenu",	ViewSortMenu);

/* Menu items for the View folder menu */
static MenuItems ViewMenuItems[] = {
  { True, TXT_VIEW_ALIGN,  TXT_M_VIEW_ALIGN, NULL, DmViewAlignCB, B_A },
  { True, TXT_VIEW_SORT,   TXT_M_VIEW_SORT,  (void *)&ViewSortMenu, NULL, B_A },
  { True, TXT_VIEW_FORMAT, TXT_M_VIEW_FORMAT,(void *)&ViewFormatMenu, NULL, B_A },
  { True, TXT_VIEW_FILTER, TXT_M_VIEW_FILTER,NULL, DmViewCustomizedCB, B_A },
  { NULL }
};

MENU("viewmenu", ViewMenu);

/* Define the Folder menu bar.  The associated pop up menus are
   assigned in DmOpenFolderWindow, because they are shared menus and only
   need to be created once.
*/
static MenuItems FolderMenuBarItems[] = {
 { True, TXT_FILE,   TXT_M_FILE,   NULL, DmMenuSelectCB },
 { True, TXT_EDIT,   TXT_M_EDIT,   NULL, DmMenuSelectCB },
 { True, TXT_VIEW,   TXT_M_VIEW,   NULL, DmMenuSelectCB },
 { True, TXT_FOLDER, TXT_M_FOLDER, NULL, DmMenuSelectCB },
 { True, TXT_HELP,   TXT_M_HELP,   NULL, DmMenuSelectCB },
 { NULL }
};

MENUBAR("menubar", FolderMenuBar);

static SWinGizmo swin_gizmo = { "swin",	/* name */ };
static StatusGizmo status_gizmo = { "status", 50 /* left percent */ };
static GizmoRec folder_gizmos[] = {
  { SWinGizmoClass,	&swin_gizmo},
  { StatusGizmoClass,	&status_gizmo},
};

static BaseWindowGizmo FolderWindow = {
	NULL,			/* help */
	"folder",		/* shell widget name */
	NULL,			/* title (runtime) */
	&FolderMenuBar,		/* menu bar (shared) */
	folder_gizmos,		/* gizmo array */
	XtNumber(folder_gizmos),/* # of gizmos in array */
	NULL,			/* icon_name (runtime) */
	NULL,			/* name of pixmap file (runtime) */
	" ",			/* error message */
	" ",			/* status message */
	75			/* percent of footer for error message */
};

static OlDtHelpInfo FolderWinHelp =
  { NULL, NULL, "DesktopMgr/folder.hlp", NULL, NULL };

static OlDtHelpInfo DestinyWinHelp =
  { NULL, NULL, "DesktopMgr/desktop.hlp", NULL, NULL };


#undef XA
#undef B_A
#undef B_O
#undef B_M
#undef B_U

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    CloseFolderCB-
*/
static void
CloseFolderCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmCloseFolderWindow((DmFolderWinPtr)DmGetWinPtr(w));

}					/* end of CloseFolderCB */

/****************************procedure*header*****************************
    ExitCB-
*/
static void
ExitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmPromptExit(False);
}					/* end of ExitCB */

/****************************procedure*header*****************************
    DetermineLayoutAttributes-

	If there is a ".dtinfo" file and there is "saved" view type, set
	it in the window.  Otherwise, take the "default" options:
		compute position and size when re-laying out.

	If the saved type is ICONIC, then no "layout" options are needed.
	Otherwise, again, position and size must be calculated when the
	view is re-layed out.
*/
static void
DetermineLayoutAttributes(DmFolderWindow window, DtAttrs * geom_options,
			  DtAttrs * layout_options)
{
    *geom_options = DM_B_CALC_SIZE | DM_B_CALC_POS;	/* the common case */
    *layout_options = 0;
	
    /* if .dtinfo file exists */
    if ( !(window->cp->attrs & DM_B_NO_INFO) )
    {
	char * view_name = DtGetProperty(&(window->cp->plist), VIEWTYPE, NULL);

	if (view_name)
	{
	    window->view_type =
		DmValueToType(view_name, (DmMapping *)view_mapping);

	    switch(window->view_type)
	    {
	    case DM_ICONIC:
		*layout_options	= DM_B_CALC_SIZE;
		*geom_options	= 0;		/* ie. retain positions */
		break;

	    case DM_LONG:
		*layout_options	= DM_B_SPECIAL_NAME;
		break;

	    default:
		break;
	    }
	}
    }
}				/* end of DetermineLayoutAttributes() */

/****************************procedure*header*****************************
    MakeFolderMenu-
*/
static void
MakeFolderMenu(MenuGizmo * menu)
{
    MenuItems *	item;
    char **	path;
    int		num_visited;
    int		offset;
    int		fewest;
    char *	shortest;

    /* Count the number of visited folder entries.  While we're at it, keep
       track of the item with the fewest "components" in its path.  This
       will be used below to strip off any common dir components.
    */
    fewest = SHRT_MAX;
    for (path = VISITED_FOLDERS(Desktop), num_visited = 0;
	 (*path != NULL) && (num_visited < MAX_VISITED_FOLDERS);
	 path++, num_visited++)
    {
	if (!SHOW_FULL_PATH(Desktop) && (fewest > 2))
	{
	    int		num_components;
	    char *	sub_path = *path;

	    for (num_components = 0; num_components < fewest; num_components++)
	    {
		if ( (sub_path = strchr(sub_path, '/')) == NULL )
		    break;

		sub_path++;
	    }
		     
	    if (num_components < fewest)
	    {
		fewest = num_components;
		shortest = *path;
	    }
	}
    }

    /* Initially, menu->items will point to FolderMenuItems.  Since these
       labels have been i18n'ized (and this is static storage), make
       menu->items NULL and pick up FolderMenuItems again below.  Note here
       that XtNumber(FolderMenuItems) is correct here since it includes
       the NULL terminating item
    */
    if (menu->items == FolderMenuItems)
	menu->items = NULL;

    menu->items = (MenuItems *)
	REALLOC((void *)menu->items,
		(num_visited + XtNumber(FolderMenuItems)) * sizeof(MenuItems));

    /* Look for common directory "components".  In this way, the button
       label can display just the unique part of the path.
    */
    offset = 0;
    if (!SHOW_FULL_PATH(Desktop) && (num_visited > 0) && (fewest > 2))
    {
	char * dup_shortest;

	/* If this is the 1st and only, make it relative to 'home' */
	if (num_visited == 1)
	    shortest = dup_shortest = strdup( DESKTOP_HOME(Desktop) );
	else
	{
	    shortest = dup_shortest = strdup(shortest);
	    shortest = dirname(shortest);
	}
	shortest = dirname(shortest);

	while (!ROOT_DIR(shortest))
	{
	    int	i;
	    int	len = strlen(shortest);

	    for (i = 0, path = VISITED_FOLDERS(Desktop);
		 i < num_visited; i++, path++)
	    {
		if ((strncmp(*path, shortest, len) != 0) ||
		    ((*path)[len] != '/'))
		    break;
	    }
	    if (i == num_visited)
	    {
		offset = len + 1;
		break;
	    }
	    shortest = dirname(shortest);
	}

	FREE(dup_shortest);
    }
    
    /* Add entries for "visited" folder paths.  Read these out of the array
       from "head" backwards so the most-recently visited appears first.
    */
    if (num_visited > 0)
    {
	path = VISITED_HEAD(Desktop);
	for (item = menu->items; item < menu->items + num_visited; item++)
	{
	    /* "auto" decrement first since 1st item is just before "head" */
	    if (path > VISITED_FOLDERS(Desktop))
		path--;
	    else
		path = VISITED_FOLDERS(Desktop) + MAX_VISITED_FOLDERS - 1;
	    if (*path == NULL)
		break;

	    item->sensitive	= (XtArgVal)True;
	    item->label		= (*path) + offset;
	    item->mnemonic	= NULL;
	    item->real_mnemonic	= NULL;		/* clear any prev mnemonic */
	    item->mod.nextTier	= NULL;
	    item->function	= DmFolderOpenDirCB;
	    item->client_data	= *path;
	    item->set		= (XtArgVal)False;
	}
    }

    /* Now copy in the "constant" buttons */
    (void)memcpy((void *)(menu->items + num_visited),
		 (const void *)FolderMenuItems, sizeof(FolderMenuItems));

    XtSetArg(Dm__arg[0], XtNitemsTouched,	True);
    XtSetArg(Dm__arg[1], XtNitems, 		menu->items);
    XtSetArg(Dm__arg[2], XtNnumItems,
	     num_visited + XtNumber(FolderMenuItems) - 1);
    XtSetValues(menu->child, Dm__arg, 3);

}				/* end of MakeFolderMenu */

/****************************procedure*header*****************************
    FolderWinWMCB-
*/
static void
FolderWinWMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlWMProtocolVerify *wm_data = (OlWMProtocolVerify *)call_data;
    DmFolderWindow folder = (DmFolderWindow)client_data;

    if (wm_data->msgtype != OL_WM_DELETE_WINDOW)	/* do the default */
    {
	OlWMProtocolAction(w, wm_data, OL_DEFAULTACTION);
	return;
    }

    folder = (DmFolderWindow)client_data;

    /* Don't allow deleting the window while file op in progress */
    if (folder->attrs & DM_B_FILE_OP_BUSY)
	return;

    if (folder == DESKTOP_TOP_FOLDER(Desktop))
	DmPromptExit(False);

    else {
	Widget shell;

	/*
	 * If one of the shared menu is currently associated with this
	 * base window, pop it down too.
	 */
	if (folder == DmGetLastMenuShell(&shell)) {
		OlActivateWidget(shell, OL_CANCEL, NULL);
	}

	DmBringDownIconMenu(folder);

	DmCloseFolderWindow(folder);
    }
}					/* end of FolderWinWMCB */

/****************************procedure*header*****************************
    PlugRing- remove 'elem' from ring and plug any holes
*/
static void
PlugRing(char ** visited, char *** head, char ** elem)
{
    char ** p;

    /* If deleted element is head, no action needed */
    if (*head == elem)
	return;

    /* If deleted element immediately precedes head, just dec head */
    if (elem == PREV_(*head, visited))
    {
	*head = elem;
	return;
    }

    do {
	p = PREV_(elem, visited);
	*elem = *p;
	elem = p;
    } while ((*p != NULL) && (p != *head));

}					/* end of PlugRing */

static char *
DmWindowSize(DmWinPtr window)
{
	static char buff[24];
	Dimension w, h;

	XtSetArg(Dm__arg[0], XtNwidth, &w);
	XtSetArg(Dm__arg[1], XtNheight, &h);
	XtGetValues(window->shell, Dm__arg, 2);
	sprintf(buff, "%dx%d", w, h);
	return(buff);
}

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmApplyShowFullPath- process "Apply" from Desktop property sheet for
	Show Full Path setting.
	This routine is here so it can access the FolderMenu gizmo.

*/
void
DmApplyShowFullPath(Boolean show)
{
    DmFolderWindow folder;

    if (SHOW_FULL_PATH(Desktop) == show)	/* If no change, do nothing. */
	return;

    SHOW_FULL_PATH(Desktop) = show;

    /* Skip the first folder assuming it's the Destiny window */
    for (folder = DESKTOP_FOLDERS(Desktop)->next;
	 folder != NULL; folder = folder->next)
	DmUpdateFolderTitle(folder);

    MakeFolderMenu(&FolderMenu);
}					/* end of DmApplyShowFullPath */

/****************************procedure*header*****************************
    DmCloseFolderWindow-
*/
void
DmCloseFolderWindow(DmFolderWinPtr folder)
{
    static const struct foobar {
	GizmoClass	class;
	Cardinal	gizmo_offset;
    } *gp, gizmos[] = {
	{ FileGizmoClass,	XtOffsetOf(DmFolderWinRec, copy_prompt) },
	{ FileGizmoClass,	XtOffsetOf(DmFolderWinRec, move_prompt) },
	{ FileGizmoClass,	XtOffsetOf(DmFolderWinRec, link_prompt) },
	{ PopupGizmoClass,	XtOffsetOf(DmFolderWinRec, rename_prompt) },
	{ FileGizmoClass,	XtOffsetOf(DmFolderWinRec, folder_prompt) },
	{ PopupGizmoClass,	XtOffsetOf(DmFolderWinRec, customWindow) },
	{ PopupGizmoClass,	XtOffsetOf(DmFolderWinRec, finderWindow) },
	{ PopupGizmoClass,	XtOffsetOf(DmFolderWinRec, createWindow) },
	{ ModalGizmoClass,	XtOffsetOf(DmFolderWinRec, overwriteNotice) },
    };

    /* Can't close folder if it is busy with file op */
    if (folder->attrs & DM_B_FILE_OP_BUSY)
	return;

    /* If this folder is in the list to be updated, remove it */
    Dm__RmFromStaleList(folder);

    /* send notifications if any */
    if (folder->notify)
    {
	DmCloseNotifyPtr np;
	DtReply reply;

	reply.header.rptype = DT_OPEN_FOLDER;
	reply.header.status = DT_OK;
	for (np = folder->notify;
	     np < folder->notify + folder->num_notify; np++)
	{
	    reply.header.serial = np->serial;
	    reply.header.version= np->version;
	    DtSendReply(XtScreen(folder->box), np->replyq,
			np->client, &reply);
	}

	FREE((void *)folder->notify);
    }


    /* If this is the head of the list, point to next folder.  Otherwise,
       find folder in list of folders and remove it.
    */
    if (folder == DESKTOP_FOLDERS(Desktop))
    {
	DESKTOP_FOLDERS(Desktop) = folder->next;

    } else
    {
	DmFolderWinPtr tmp = DESKTOP_FOLDERS(Desktop);

	while (tmp->next != folder)
	    tmp = tmp->next;

	tmp->next = folder->next;
    }

    if(folder->finderWindow != NULL)
    {
        ListGizmo *	fnlist;
	ListHead *	head;
        entryptr	loclist;
        int		i;

	fnlist = (ListGizmo *)QueryGizmo(PopupGizmoClass,
					 (PopupGizmo *)folder->finderWindow,
					 GetGizmoGizmo, "fnlist");

	head = (ListHead *)(fnlist->settings->current_value);

	for(i = 0, loclist = (entryptr)head->list[0].fields;
	    i< head->size; i++, loclist = loclist->next)
	{
	    FREE(loclist->name);
	    if (loclist->dir)
	    	FREE(loclist->dir);
	}
    }

	/* free list items in New window */
	if (folder->createWindow != NULL) {
		ListTokenPtr list_items;

		Widget flist = (Widget)QueryGizmo(PopupGizmoClass,
					folder->createWindow, GetGizmoWidget, "template");
		XtVaGetValues(flist, XtNuserData, &list_items, NULL);
		if (list_items) {
			ListNodePtr save;
			ListNodePtr lnp = (ListNodePtr)(list_items->formatData);

			while(lnp) {
				save = lnp->next;
				FREE((void *)(lnp->name));
				FREE((void *)lnp);
				lnp = save;
			}
			FREE((void *)list_items);
		}
	}

    /* Free prompt gizmos, popup gizmos, etc. */
    for (gp = gizmos; gp < gizmos + XtNumber(gizmos); gp++)
    {
	Gizmo * gizmo = (Gizmo *)(((char *)folder) + gp->gizmo_offset);

	if (*gizmo != NULL)
	    FreeGizmo(gp->class, *gizmo);
    }

    FREE(DM_WIN_BASE_GIZMO(folder)->title);	/* Free base gizmo title */
    FREE(DM_WIN_BASE_GIZMO(folder)->icon_name);	/* Free base icon_name */
    FREE((void *)DM_WIN_BASE_GIZMO(folder));	/* Free base gizmo */

    DmCloseWindow((DmWinPtr)folder);
}					/* end of CloseFolderWindow */

/****************************procedure*header*****************************
    DmCloseFolderWindows-
*/
void
DmCloseFolderWindows()
{
    DmFolderWindow folder;

    for (folder = DESKTOP_FOLDERS(Desktop);
	 folder != NULL; folder = folder->next)
	DmCloseFolderWindow(folder);

    /* Indicate to sync timer that we're exiting */
    DESKTOP_FOLDERS(Desktop) = NULL;
}

/****************************procedure*header*****************************
    DmCloseWindow-
*/
void
DmCloseWindow(DmWinPtr window)
{
    int w, h;

    if (window->view_type == DM_ICONIC) {
	DmSaveXandY(window->itp, window->nitems);	/* save X & Y */
	DtSetProperty(&(window->cp->plist), VIEWTYPE, NULL, 0);
    }
    else
	/* save current view information (possibly sort info. later ) */
	DtSetProperty(&(window->cp->plist), VIEWTYPE,
		  DmTypeToValue(window->view_type, (DmMapping *)view_mapping),
		  NULL);

    /* save window dimension */
    DtSetProperty(&(window->cp->plist), FOLDERSIZE, DmWindowSize(window), NULL);

    /* Free the ContainerRec, free icon labels and destroy widget tree */
    DmCloseContainer(window->cp, 0);
    DmDestroyIconContainer(window->shell, window->box,
			   window->itp, window->nitems);

    FREE((void *)window);	/* free space in window structure */

}					/* end of DmCloseWindow */

/****************************procedure*header*****************************
    DmFindFolderWindow-
*/
DmFolderWinPtr
DmFindFolderWindow(Widget w)
{
    Widget		shell = _OlGetShellOfWidget(w);
    DmFolderWinPtr	window;

    if (shell == NULL)
	window = NULL;

    else
	for (window = DESKTOP_FOLDERS(Desktop);
	     (window != NULL) && (window->shell != shell);
	     window = window->next)
	    ;

    return (window);
}				/* end of DmFindFolderWindow */

/****************************procedure*header*****************************
    DmOpenFolderWindow-
*/
DmFolderWinPtr
DmOpenFolderWindow(char * path, DtAttrs attrs, char * geom_str, Boolean iconic)
{
    static int		first = 1;
    DtAttrs		geom_options;
    DtAttrs		layout_options;
    DmFolderWinPtr	window;
    Dimension		view_width;
    BaseWindowGizmo *	base;
    MenuGizmo *		menu;
    char		str_buff[32];
    static char *	icon_pixmaps[][2] = {
	TXT_SYSTEM_SETUP_FOLDER,	"sysfldr48.icon",
	TXT_PREFERENCES_FOLDER,		"prefldr48.icon",
	TXT_APPLICATIONS_FOLDER,	"appfldr48.icon",
	TXT_UTILITIES_FOLDER,		"util48.icon",
	TXT_RESOURCES_FOLDER,		"recfldr48.icon",
    };
    static char *	(*icon_pixmap)[2];

    /* If $HOME is '/', then DESKTOPDIR will be "//desktop" which will
       confuse things like the Folder menu.  Fix this here.
    */
    if ((path[0] == '/') && (path[1] == '/'))
	path++;

    if (access(path, F_OK) != 0)
	return (NULL);

    if (window = DmQueryFolderWindow(path))
    {
	DmMapWindow((DmWinPtr)window);
	return(window);
    }

    if (first)
    {
	/* Set defaults */
	ViewMenu.default_item = 1;	/* "Sort" */

	/* Build sub menus that are shared by all folder windows. */

	CreateGizmo(DESKTOP_SHELL(Desktop),MenuGizmoClass, &FileMenu, NULL, 0);
	CreateGizmo(DESKTOP_SHELL(Desktop),MenuGizmoClass, &ExitMenu, NULL, 0);
	CreateGizmo(DESKTOP_SHELL(Desktop),MenuGizmoClass, &ViewMenu, NULL, 0);
	CreateGizmo(DESKTOP_SHELL(Desktop),MenuGizmoClass, &EditMenu, NULL, 0);
	CreateGizmo(DESKTOP_SHELL(Desktop),MenuGizmoClass, &FolderMenu,NULL,0);
	CreateGizmo(DESKTOP_SHELL(Desktop),MenuGizmoClass, &HelpMenu, NULL, 0);
	CreateGizmo(DESKTOP_SHELL(Desktop),MenuGizmoClass, &DesktopHelpMenu,
		 NULL, 0);
    }

    window = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));
    window->cp = DmOpenDir(path, attrs | DM_B_TIME_STAMP | DM_B_SET_TYPE | DM_B_INIT_FILEINFO);

    if (window->cp == NULL)
    {
	Dm__VaPrintMsg(TXT_OPENDIR, errno, path);
	FREE((void *)window);
	return (NULL);
    }

    window->attrs	= DM_B_FOLDER_WIN; /* identify it as a Folder window */
    window->view_type	= DM_ICONIC;
    window->sort_type	= DM_BY_TYPE;

    /* Make a copy of the base window gizmo */
    window->gizmo_shell = base = CopyGizmo(BaseWindowGizmoClass, &FolderWindow);
    menu = base->menu;
    menu->client_data = (XtPointer)window;

    /* Copy the shared menu gizmos pointers into the new gizmo struct. */
					/* First window gets the Exit button */
    menu->items[0].mod.nextTier = first ? &ExitMenu : &FileMenu;
    menu->items[1].mod.nextTier = &EditMenu;
    menu->items[2].mod.nextTier = &ViewMenu;
    menu->items[3].mod.nextTier = &FolderMenu;
    if (first)
    	menu->items[4].mod.nextTier = &DesktopHelpMenu;
    else
    	menu->items[4].mod.nextTier = &HelpMenu;

    if (first)
    {
	/* Activate "sync" timer when 1st folder is created */
	if (SYNC_INTERVAL(Desktop) > 0)
	    Dm__AddSyncTimer(Desktop);

	/* CAUTION: visited folders will not be updated (below) for first
	   folder so must make the folder menu here the first time.
	*/
	MakeFolderMenu(&FolderMenu);

	/* Initialize the "head" of the visited folder array */
	VISITED_HEAD(Desktop) = VISITED_FOLDERS(Desktop);

    } else
    {
	/* Add folder (path) to the list of visited folders.
	   See CAUTION above.
	*/
	Dm__UpdateVisitedFolders(NULL, DM_WIN_PATH(window));
    }


    /* Set base window title, icon_name and icon_pixmap.
       The first time thru, initialize the table of "special" folder names.
       This way it's only done ONCE.  After the first time, compare the
       folder path with the folder names in the table to extract its pixmap.
    */
    base->title = strdup(DmMakeFolderTitle(window));
    if (first)
    {
	base->icon_name = strdup(base->title);
	base->icon_pixmap = "dtprop48.icon";

	for(icon_pixmap = icon_pixmaps;
	    icon_pixmap < icon_pixmaps + XtNumber(icon_pixmaps); icon_pixmap++)
	    (*icon_pixmap)[0] = Dm__gettxt((*icon_pixmap)[0]);
    } else
    {
	base->icon_name = strdup(ROOT_DIR(DM_WIN_PATH(window)) ?
				 DM_WIN_PATH(window) :
				 basename(DmGetFolderName(window)));

	for(icon_pixmap = icon_pixmaps;
	    icon_pixmap < icon_pixmaps + XtNumber(icon_pixmaps); icon_pixmap++)
	{
	    /* Compare path with the name of one of the "special" folders */
	    if (strcmp(DmMakePath(DESKTOP_DIR(Desktop), (*icon_pixmap)[0]),
		       DM_WIN_PATH(window)) == 0)
	    {
		base->icon_pixmap = (*icon_pixmap)[1]; /* icon pixmap name */
		break;
	    }
	}
	/* If not a special folder name, assign "default" folder icon_pixmap */
	if (base->icon_pixmap == NULL) {
	    char *icon = DtGetProperty(&(window->cp->plist), FOLDERICON, NULL);

	    if (icon)
	    	base->icon_pixmap = icon;
	    else
	        base->icon_pixmap = "dir48.icon";
	}
    }

    if (!geom_str && (geom_str = DtGetProperty(&(window->cp->plist),
			 FOLDERSIZE, NULL))) {
	/* make sure it is less than the screen size */
	int sw = WidthOfScreen(XtScreen(DESKTOP_SHELL(Desktop)));
	int sh = HeightOfScreen(XtScreen(DESKTOP_SHELL(Desktop)));
	int w = 0, h = 0;
	int changed = 0;

	if ((sscanf(geom_str, "%dx%d", &w, &h) != 2) || !w || !h ||
	    (w > sw) || (h > sh))
		geom_str = NULL;
    }

    XtSetArg(Dm__arg[0], XtNiconic, iconic);
    XtSetArg(Dm__arg[1], XtNgeometry, geom_str);

    window->shell = CreateGizmo(NULL, BaseWindowGizmoClass, base, Dm__arg, 2);
    window->swin  = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
				 GetGizmoWidget, "swin");

    if (geom_str == NULL)
    {
	Dimension margin = Ol_PointToPixel(OL_HORIZONTAL, ICON_MARGIN) * 2;
	OlSWGeometries swin_geom =
	    GetOlSWGeometries((ScrolledWindowWidget)(window->swin));

    	/* Compute the view width and set the scrolled window to it */
    	view_width = GRID_WIDTH(Desktop) * FOLDER_COLS(Desktop) +
	    swin_geom.vsb_width + margin;

	XtSetArg(Dm__arg[0], XtNviewWidth, view_width);
	XtSetArg(Dm__arg[1], XtNviewHeight,
		 GRID_HEIGHT(Desktop)*FOLDER_ROWS(Desktop) + margin);
    	XtSetValues(window->swin, Dm__arg, 2);
    }

    DetermineLayoutAttributes(window, &geom_options, &layout_options);

    XtSetArg(Dm__arg[0], XtNdropProc,		Dm__FolderDropProc);
    XtSetArg(Dm__arg[1], XtNpostSelectProc,	DmButtonSelectProc);
    XtSetArg(Dm__arg[2], XtNpostAdjustProc,	DmButtonSelectProc);
    XtSetArg(Dm__arg[3], XtNdblSelectProc,	Dm__FolderSelect2Proc);
    XtSetArg(Dm__arg[4], XtNmenuProc,		DmIconMenuProc);
    XtSetArg(Dm__arg[5], XtNclientData,		window);
    XtSetArg(Dm__arg[6], XtNtriggerMsgProc,	DmFolderTriggerNotify);
    XtSetArg(Dm__arg[7], XtNfont,		DESKTOP_FONT(Desktop));
    XtSetArg(Dm__arg[8], XtNmovableIcons,	window->view_type == DM_ICONIC);
    XtSetArg(Dm__arg[9], XtNdrawProc, 
	     (window->view_type == DM_LONG) ?	DmDrawLongIcon :
	     (window->view_type == DM_NAME) ?	DmDrawNameIcon :
	     /* else */				DmDrawLinkIcon);

    window->nitems = window->cp->num_objs + EXTRA_ICON_SLOTS;
    window->box = DmCreateIconContainer(window->swin, geom_options, 
					Dm__arg, 10,
			  		window->cp->op, 
					window->cp->num_objs,
			  		&(window->itp), 
					window->nitems,
			  		NULL,
					DESKTOP_FONTLIST(Desktop),
					DESKTOP_FONT(Desktop),
					DM_FontHeight(Desktop));

    /* make the iconbox the initial focus widget */
    XtSetArg(Dm__arg[0], XtNfocusWidget, window->box);
    XtSetValues(window->shell, Dm__arg, 1);

    /* insert new window into the list */
    DmAddWindow((DmWinPtr *)&DESKTOP_FOLDERS(Desktop), (DmWinPtr)window);

    /* add callback for WM */
    OlAddCallback(window->shell, XtNwmProtocol, FolderWinWMCB,
		  (XtPointer)window);

    /* put something in the status area */
    DmDisplayStatus((DmWinPtr)window);

    XtRealizeWidget(window->shell);

    if (geom_str == NULL)
    {
	XtSetArg(Dm__arg[0], XtNvStepSize, GRID_HEIGHT(Desktop));
	XtSetArg(Dm__arg[1], XtNhStepSize, GRID_WIDTH(Desktop));
    	XtSetValues(window->swin, Dm__arg, 2);
    }

    if (first)
    {
        Session.SessionLeader = XtWindow(base->shell);
        Session.SessionLeaderIcon = XtWindow(base->icon_shell);
    }

    /* FORMAT VIEW after window has been realized so geometries have been
       negotiated.  Pass the view_type in 'type' and override the window
       view_type with 'no value'.  SPECIAL: if ICONIC and no ".dtinfo",
       don't pass any view_type; DmFormatView understands.
    */
    {
	DmViewFormatType type =
	    ((window->view_type == DM_ICONIC) &&
	     (window->cp->attrs & DM_B_NO_INFO)) ?
		 (DmViewFormatType)-1 : window->view_type;
	window->view_type = (DmViewFormatType)-1;
	DmFormatView(window, type);
    }

    /* register for context-sensitive help for all folders */
    if (first)
    {
	first = 0;			/* no longer the first time */

	/*
	 * Register Destiny window for help separately from other
	 * folders so that help for the Destiny window can be accessed
	 * from its Help button.
	 */
	DESKTOP_HELP_ID(Desktop) =
	    DmNewHelpAppID(DESKTOP_SCREEN(Desktop),
			   XtWindow(window->shell),
			   Dm__gettxt(TXT_DESKTOP_MGR),
			   Dm__gettxt(TXT_PRODUCT_NAME),
			   DESKTOP_NODE_NAME(Desktop),
			   NULL, "dtprop.icon")->app_id;

	DestinyWinHelp.app_title = Dm__gettxt(TXT_PRODUCT_NAME);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)(window->shell), NULL,
		       OL_DESKTOP_SOURCE, (XtPointer)&DestinyWinHelp);
    } else
    {
	FolderWinHelp.app_title = Dm__gettxt(TXT_FOLDER_TITLE);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)(window->shell), NULL,
		       OL_DESKTOP_SOURCE, (XtPointer)&FolderWinHelp);
    }

    return(window);
}				/* end of DmOpenFolderWindow */

/****************************procedure*header*****************************
    Dm__UpdateVisitedFolder- update the list of visited folders.
	If the list changes, rebuild the Folder Menu.  (Optimally, the menu
	would only be (re)made when the menu is popped up.)
	This routine is here so it can access the FolderMenu gizmo.

	old_path == NULL  :	add 'new_path'
	new_path == NULL  :	delete 'old_path'
	else			modify 'old_path' to 'new_path'

	For deletions and modifications, descendant paths must also be
	processed.

	Note that deletions do not occur each time a folder is closed but
	rather only when a directory becomes invalid (ie. is deleted).
*/
void
Dm__UpdateVisitedFolders(char * old_path, char * new_path)
{
    char **	visited = VISITED_FOLDERS(Desktop);
    char **	current = VISITED_HEAD(Desktop);
    char **	path;
    int		old_len;
    Boolean	changed = False;

    if (old_path == NULL)			/* ie. ADDITION */
    {
	/* Ensure unique entries */
	for (path = visited; path <= TAIL_(visited); path++)
	{
	    if (*path == NULL)
		break;

	    if (strcmp(*path, new_path) == 0)
		return;				/* ignore duplicates */
	}

	XtFree(*current);	/* free any path before replacing */
	*current = strdup(new_path);
	VISITED_HEAD(Desktop) = NEXT_(current, visited);
	changed = True;

    } else if (new_path == NULL)		/* ie. DELETION */
    {
	old_len = strlen(old_path);
	path = visited;
	while (path <= TAIL_(visited))
	    if ((*path != NULL) &&
		DmSameOrDescendant(old_path, *path, old_len))
	    {
		char **	next_path;
		int	cnt;

		FREE(*path);
		*path = NULL;
		changed = True;

		PlugRing(visited, &VISITED_HEAD(Desktop), path);

	    } else
	       path++;

    } else					/* ie. MODIFICATION */
    {
	old_len = strlen(old_path);
	for (path = visited; path <= TAIL_(visited); path++)
	{
	    int result = DmSameOrDescendant(old_path, *path, old_len);

	    if ((*path != NULL) && (result != 0))
	    {
		char * save_path = *path;

		*path = strdup((result < 0) ? new_path :	/* same */
			       DmMakePath(new_path, save_path + old_len + 1));

		changed = True;
		FREE(save_path);
	    }
	}
    }

    if (changed)
	MakeFolderMenu(&FolderMenu);
}					/* end of Dm__UpdateVisitedFolders */
