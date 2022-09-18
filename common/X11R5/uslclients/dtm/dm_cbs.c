/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:dm_cbs.c	1.103"

/******************************file*header********************************

    Description:
	This file contains the source code for callback functions
	which are shared among components of the desktop manager.
*/
						/* #includes go here	*/
#include <stdio.h>
#include <stdlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlCursors.h>
#include <memutil.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

static Boolean	BuildIconMenu(DmWinPtr, DmItemPtr, MenuItems **, Cardinal *);
static void	DeleteOneFile(DmFolderWindow, DmItemPtr);
static void	IconMenuCB(Widget, XtPointer, XtPointer);
static void	I18NMenuItems(MenuItems * items);
static void	Print(DmWinPtr wp, DmObjectPtr obj);
static void	SelectUnselectAll(DmWinPtr, Boolean);
static void	SetIconMenuClientData(MenuItems * items, XtPointer data);

					/* public procedures		*/

void		DmEditSelectAllCB(Widget, XtPointer, XtPointer);
void		DmEditUnselectAllCB(Widget, XtPointer, XtPointer);
XtPointer	DmGetWinPtr(Widget w);
void		DmHelpDeskCB(Widget, XtPointer, XtPointer);
void		DmHelpSpecificCB(Widget, XtPointer, XtPointer);
void		DmHelpTOCCB(Widget, XtPointer, XtPointer);
void		DmIconMenuProc (Widget, XtPointer, XtPointer);
void		DmMenuSelectCB(Widget, XtPointer, XtPointer);
void		DmPrintCB(Widget, XtPointer, XtPointer);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/
/* Define Folder icon menu items */
static MenuItems FolderMenuItems[] = {
  { True, TXT_FILE_OPEN,	TXT_M_FILE_OPEN,	NULL, IconMenuCB, },
  { True, TXT_FILE_PROP,	TXT_M_FILE_PROP,	NULL, IconMenuCB, },
  { True, TXT_FILE_DELETE,	TXT_M_FILE_DELETE,	NULL, IconMenuCB, },
  { True, TXT_FILE_PRINT,	TXT_M_FILE_PRINT,	NULL, IconMenuCB, },
  { NULL },
};

static char *FolderLabels[] = { "Open", "Properties...", "Delete", "Print" };

/* Define Help Desk icon menu items */
static MenuItems HDMenuItems[] = {
  { True, TXT_FILE_OPEN, TXT_M_FILE_OPEN, NULL, DmHDIMOpenCB, },
  { NULL },
};

/* Define Tree view icon menu items */
static MenuItems TreeMenuItems[] = {
  { True, TXT_FILE_OPEN,     TXT_M_FILE_OPEN,	NULL, IconMenuCB, },
  { True, TXT_FILE_PROP,     TXT_M_FILE_PROP,	NULL, IconMenuCB, },
  { True, TXT_FILE_DELETE,   TXT_M_FILE_DELETE,	NULL, IconMenuCB, },
  { True, TXT_TREE_SHOW,     TXT_M_TREE_SHOW,	NULL, TreeIconMenuCB, },
  { True, TXT_TREE_HIDE,     TXT_M_TREE_HIDE,	NULL, TreeIconMenuCB, },
  { True, TXT_TREE_SHOW_ALL, TXT_M_TREE_SHOW_ALL,NULL,TreeIconMenuCB, },
  { True, TXT_TREE_HERE,     TXT_M_TREE_HERE,	NULL, TreeIconMenuCB, },
  { NULL },
};

/* Define Wastebasket icon menu items */
static MenuItems WBMenuItems[] = {
  { True, TXT_IM_WB_FILEPROP, TXT_M_WB_FILEPROP,NULL, DmWBIMFilePropCB, },
  { True, TXT_IM_WB_PUTBACK,  TXT_M_WB_PUTBACK,	NULL, DmWBIMPutBackCB, },
  { True, TXT_IM_WB_DELETE,   TXT_M_WB_DELETE,	NULL, DmWBIMDeleteCB, },
  { NULL },
};

/* Define gizmo for Icon Menu */
static MenuGizmo icon_menu = {
    NULL,				/* help info */
    "icon_menu",			/* shell name */
    NULL,				/* menu title */
    NULL,				/* menu items (run time) */
    NULL,				/* call back (in items) */
    NULL,				/* client data (in items) */
    CMD,				/* button type */
    OL_FIXEDCOLS,			/* layout type */
    1,					/* measure */
    0,					/* index of default item */
};

/* Save these in statics.  They cannot be passed conveniently to the File
   Icon menu callback since the menu is shared.

    'icon_item'	points to the item overwhich MENU was pressed.
    'icon_win'	points to the folder/toolbox window which contains the item.
*/
static DmItemPtr	icon_item;
static DmWinPtr		icon_win;
static DmWinPtr		iconmenu_for;


/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    BuildIconMenu-
*/
static Boolean
BuildIconMenu(DmWinPtr window, DmItemRec * item,
	      MenuItems ** ret_menu_items, Cardinal * ret_num_items)
{
    Boolean	items_touched = False;

    /* Save the window and item.  These are needed by the certain button
       callbacks in the menu but can't be passed conveniently since menu is
       shared.  If this is the 1st time, I18n'ize menu items.
    */
    if (icon_win == NULL)
    {
	I18NMenuItems(FolderMenuItems);
	I18NMenuItems(HDMenuItems);
	I18NMenuItems(TreeMenuItems);
	I18NMenuItems(WBMenuItems);
    }
    icon_item	= item;
    icon_win	= window;

    if (window->attrs & DM_B_HELPDESK_WIN)
    {
	SetIconMenuClientData(HDMenuItems, icon_item);
	*ret_menu_items	= HDMenuItems;
	*ret_num_items	= XtNumber(HDMenuItems) - 1;

    } else if (window->attrs & DM_B_WASTEBASKET_WIN)
    {
	SetIconMenuClientData(WBMenuItems, icon_item);
	*ret_menu_items	= WBMenuItems;
	*ret_num_items	= XtNumber(WBMenuItems) - 1;

    } else if (IS_TREE_WIN(Desktop, (DmFolderWindow)window))
    {
	/* CAUTION: '3' used here to jump over Open, Properties, & Delete */
	SetIconMenuClientData(TreeMenuItems + 3, icon_item);
	*ret_menu_items	= TreeMenuItems;
	*ret_num_items	= XtNumber(TreeMenuItems) - 1;

    } else					/* Folder window */
    {
	static MenuItems *	menu_items = NULL;
	static Cardinal		num_alloced = 0;
	MenuItems *		menu_item;
	DtPropPtr		prop;
	int			num_props;
	DmObjectPtr		obj;
	int			num_dflt_items;

	/* Get the object data. */
	obj = ITEM_OBJ(item);

	/* The number of properties computed here is a conservative number
	   which may count duplicate entries: class and instance properties
	   (if specified) may be double-counted.  ('1' added for delimiter)
	*/
	num_props = XtNumber(FolderMenuItems) +
	    obj->plist.count + obj->fcp->plist.count;

	if (num_alloced < num_props)
	{
	    menu_items = (MenuItems *)REALLOC((void *)menu_items,
					      num_props * sizeof(MenuItems));

	    /* First time thru, copy in "fixed" buttons */
	    if (num_alloced == 0)
		(void)memcpy(menu_items,
			     FolderMenuItems, sizeof(FolderMenuItems));

	    num_alloced = num_props;

	} else
	{
	    menu_items[0].client_data = NULL;
	    menu_items[1].client_data = NULL;
	    menu_items[2].client_data = NULL;
	    menu_items[3].client_data = NULL;
	}

	/* Put 'end' after "fixed" buttons.  The "Print" button only
	   appears as a default button for files that type as data-files.
	*/
	menu_item = menu_items + XtNumber(FolderMenuItems) - 2;
	if (obj->ftype == DM_FTYPE_DATA)
	    *menu_item++ = FolderMenuItems[XtNumber(FolderMenuItems) - 2];
	num_dflt_items = (int)(menu_item - menu_items) + 1;

	for (prop = DmFindObjProperty(obj, DT_PROP_ATTR_MENU);
	     prop != NULL; prop = DmFindObjProperty(NULL, DT_PROP_ATTR_MENU))
	{
	    char *	name;
	    char *	value;
	    MenuItems *	ip;

	    /* Get name and value of property.  Skip any leading '_' in
	       property name.  Pass value of property as client_data (though
	       NULL for Print).
	    */
	    name = (prop->name[0] == '_') ? prop->name + 1 : prop->name;
	    value = (strcmp(name, "Print") == 0) ? NULL : prop->value;

	    for (ip = menu_items; ip < menu_item; ip++) {
		/* compare label with localized string */
		if (!strcmp(ip->label, name))
			break;

		/* For system names, compare label with "English" string */
		if ((prop->name != name) &&
		    ((ip - menu_items) < num_dflt_items) &&
		    (!strcmp(FolderLabels[ip - menu_items], name)))
			break;
	    }

	    if (ip < menu_item)		/* ie, duplicate name */
	    {
		if (value != NULL)
		    ip->client_data = value;
		continue;
	    }

	    /* Create new button using property name: button label is
	       property name; pass value as client_data.  (Make sure
	       client_data for "Print" button is NULL.)
	    */

	    menu_item->sensitive	= True;
	    menu_item->label		= name;
	    menu_item->mnemonic		= NULL;
	    menu_item->mod.nextTier	= NULL;
	    menu_item->function		= IconMenuCB;
	    menu_item->client_data	= (char *)value;
	    menu_item->set		= False;
	    menu_item->button		= NULL;
	    menu_item->real_mnemonic	= NULL;

	    menu_item++;
	    items_touched = True;
	}

	/* MenuGizmo needs terminated list of items */
	menu_item->label = NULL;

	*ret_menu_items	= menu_items;
	*ret_num_items	= menu_item - menu_items;
    }

    return(items_touched);
}					/* end of BuildIconMenu */

static DmWinPtr
DmGetIconMenuFor(shell)
Widget *shell;
{
	*shell = icon_menu.parent;
	return(iconmenu_for);
}

void
DmBringDownIconMenu(folder)
DmWinPtr folder;
{
	Widget shell;

	/*
	 * If the icon menu is currently associated with this
	 * base window, pop it down too.
	 */
	if (folder == DmGetIconMenuFor(&shell)) {
		OlUnpostPopupMenu(shell);
	}
}

/****************************procedure*header*****************************
    DeleteOneFile-
*/
static void
DeleteOneFile(DmFolderWindow folder, DmItemPtr item)
{
    DmItemPtr	iplist[2];
    char 	*filename;
    DmFileOpInfoPtr	opr_info =
	(DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));

    iplist[0] = item;
    iplist[1] = NULL;
    if (filename = DmHasSystemFiles((void **)iplist)) {
	DmVaDisplayStatus((DmWinPtr)folder, True, TXT_DEL_SYSTEM_FILE,filename);
	return;
    }

    /* load parameters into opr_info struct */
    opr_info->type		= DM_MOVE;
    opr_info->options		=
	(folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN)) ?
	    REPORT_PROGRESS | OVERWRITE | OPRBEGIN | MULTI_PATH_SRCS :
	    REPORT_PROGRESS | OVERWRITE | OPRBEGIN;
    opr_info->target_path	= strdup( DM_WB_PATH(Desktop) );
    opr_info->src_path		= strdup( DM_WIN_PATH(folder) );
    opr_info->src_list		= (char **)MALLOC(sizeof(char *));
    opr_info->src_cnt		= 1;
    opr_info->src_win		= folder;
    opr_info->dst_win		= DESKTOP_WB_WIN(Desktop);
    opr_info->x			= opr_info->y = UNSPECIFIED_POS;

    opr_info->src_list[0] = strdup( ITEM_OBJ_NAME(item) );

    DmFreeTaskInfo(folder->task_id);
    folder->task_id =
        DmDoFileOp(opr_info, DmFolderFMProc, NULL);

}					/* end of DeleteOneFile */

/****************************procedure*header*****************************
    IconMenuCB- this callback services the buttons in the Icon menu
	used in Folder and Toolbox windows.  Callbacks for the icon
	menu in the wastebasket window are handled elsewhere.
*/
static void
IconMenuCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	data = (OlFlatCallData *)call_data;

    DmClearStatus(icon_win);			/* Clear footer first */

    switch(data->item_index)
    {
    case 0:					/* "Open" button */
	/* This could take awhile so entertain the user */
	BUSY_CURSOR(icon_win->box);
	DmOpenObject(icon_win, ITEM_OBJ(icon_item));
	break;

    case 2:					/* "Delete" button */
	DeleteOneFile((DmFolderWindow)icon_win, icon_item);
	break;

    case 1:					/* "Property" button */
	if ((icon_win->attrs & DM_B_FOLDER_WIN) && (client_data == NULL))
	{
	    Dm__PopupFilePropSheet((DmFolderWinPtr)icon_win, icon_item);
	    break;
	}
	/* otherwise, FALLTHROUGH */
    default:
	if (client_data == NULL)		/* Must be "Print" button */
	{
	    Print(icon_win, ITEM_OBJ(icon_item));
	} else
	{
	    DmObjectPtr	obj = ITEM_OBJ(icon_item);
	    char *	cmd;

	    cmd =
		Dm__expand_sh((char *)client_data, DmObjProp, (XtPointer)obj);
	    DmExecuteShellCmd(icon_win, obj, cmd, False);
	    FREE(cmd);
	}
	break;
    }
}	/* end of IconMenuCB */

/****************************procedure*header*****************************
    I18NMenuItems-
*/
static void
I18NMenuItems(MenuItems * items)
{
    for ( ; items->label != NULL; items++)
    {
	items->label = GetGizmoText(items->label);

	if (items->mnemonic == NULL)
	    items->real_mnemonic = 0;
	else
	{
	    char * mnemonic = GetGizmoText(items->mnemonic);
	    items->real_mnemonic = (XtArgVal)mnemonic[0];
	}
    }
}

/****************************procedure*header*****************************
    Print-
*/
static void
Print(DmWinPtr wp, DmObjectPtr obj)
{
    char *	default_printer;
    char *	prt_cmd;

    /* Get print command from property (if any) */
    prt_cmd = DmGetObjProperty(obj, PRINTCMD, NULL);

    if (prt_cmd == NULL)
    {
	if (obj->ftype != DM_FTYPE_DATA)
	{
	    DmVaDisplayStatus(wp, 1, TXT_NO_PRINT, obj->name);
	    return;
	}

	/* This is the default for all data files */
	prt_cmd = "cat \"%F\" | $XWINHOME/bin/wrap | $XWINHOME/bin/PrtMgr -p %_DEFAULT_PRINTER &";
    }

    /* If print command uses default printer but property is not
     * defined, alert user.
     */
    default_printer = "%" _DEFAULT_PRINTER;
    if((strstr(prt_cmd, default_printer) != 0) &&
       (DmGetDTProperty(_DEFAULT_PRINTER, NULL) == NULL))
    {
	DmVaDisplayStatus(wp, 1, TXT_NO_DFLT_PRT, obj->name);
	return;
    }

    /* Expand command (returns malloc'ed string) */
    prt_cmd = Dm__expand_sh(prt_cmd, DmObjProp, (XtPointer)obj);
    DmExecuteShellCmd(wp, obj, prt_cmd, False);
    FREE(prt_cmd);
}
	
/****************************procedure*header*****************************
    SelectUnselectAll- this routine selects or unselects all the items in
	the folder.  'select' determines setting.
*/
static void
SelectUnselectAll(DmWinPtr window, Boolean select)
{
    int			i;
    DmItemPtr		ip;
    Boolean		touched = False;
    int			cnt;	/* # of items affected */

    for (i = 0, cnt = 0, ip = window->itp; i < window->nitems; i++, ip++)
	if (ITEM_MANAGED(ip))
	{
	    cnt++;
	    if (ITEM_SELECT(ip) != select)
	    {
		ip->select = select;
		touched	= True;
	    }
	}

    if (touched)
    {
	XtSetArg(Dm__arg[0], XtNselectCount, (select ? cnt : 0));
	DmTouchIconBox(window, Dm__arg, 1);
	DmDisplayStatus(window);
    }
}				/* End of SelectUnselectAll */

/****************************procedure*header*****************************
    SetIconMenuClientData-
*/
static void
SetIconMenuClientData(MenuItems * items, XtPointer data)
{
    while (items->label != NULL)
	items++->client_data = (char *)data;
}

/***************************private*procedures****************************

    Public Procedures
*/

/*************************************************************************
	Edit menu item callbacks

    These callbacks handle (common) menu items from the Edit menu.
*/

/****************************procedure*header*****************************
    DmEditSelectAllCB-
*/
void
DmEditSelectAllCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    SelectUnselectAll((DmWinPtr)DmGetWinPtr(w), True);

}				/* End of DmEditSelectAllCB */

/****************************procedure*header*****************************
    DmEditUnselectAllCB-
*/
void
DmEditUnselectAllCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    SelectUnselectAll((DmWinPtr)DmGetWinPtr(w), False);

}				/* End of DmEditUnselectAllCB */

/*************************************************************************
	Help menu item callbacks

    Each Desktop window (Toolbox, Folder, Wastebasket) has a help menu.  Menu
    items on that menu call these callbacks.  There is one Help menu shared
    between all Folder windows and one Help menu shared between all Toolbox
    windows.
*/
/****************************procedure*header*****************************
    DmHelpDeskCB- called to bring up the Help Desk.
*/

void
DmHelpDeskCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (DESKTOP_HELP_DESK(Desktop) == NULL)
		DmInitHelpDesk(NULL, False, True);
	else {
		DmMapWindow((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
		DmClearStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
		XRaiseWindow(XtDisplay(DESKTOP_HELP_DESK(Desktop)->shell),
			XtWindow(DESKTOP_HELP_DESK(Desktop)->shell));
	}
}

/****************************procedure*header*****************************
    DmHelpSpecificCB- called for specfic (window-dependent) help (usually the
	1st menu item).

	This callback uses the 'attrs' field in the Window struct to identifiy
	which window invoked the callback.  Normally this would be done by
	passing client data that would identify the Window type but this is not
	possible when the menus are shared.
*/
void
DmHelpSpecificCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmWinPtr		wp = (DmWinPtr)DmGetWinPtr(w);

    if (wp->attrs & DM_B_TREE_WIN) {
    		DmHelpAppPtr	help_app = DmGetHelpApp(FMAP_HELP_ID(Desktop));

          DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
               NULL, "DesktopMgr/fmap.hlp", "10", UNSPECIFIED_POS,
			UNSPECIFIED_POS);
		
    } else if (wp->attrs & DM_B_FOLDER_WIN) {
		if (strcmp(wp->cp->path, DESKTOP_DIR(Desktop)) == 0) {
    			DmHelpAppPtr help_app = DmGetHelpApp(DESKTOP_HELP_ID(Desktop));

          	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
               	NULL, "DesktopMgr/desktop.hlp", "10", UNSPECIFIED_POS,
				UNSPECIFIED_POS);
		} else {
    			DmHelpAppPtr help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));

          	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
               	NULL, "DesktopMgr/folder.hlp", "10", UNSPECIFIED_POS,
				UNSPECIFIED_POS);
		}
		
    } else if (wp->attrs & DM_B_WASTEBASKET_WIN) {
    		DmHelpAppPtr	help_app = DmGetHelpApp(WB_HELP_ID(Desktop));

          DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
               NULL, "DesktopMgr/wb.hlp", "10", UNSPECIFIED_POS,
			UNSPECIFIED_POS);
    }
}

/****************************procedure*header*****************************
    DmHelpTOCCB- called to bring up Help Table of Contents
*/
void
DmHelpTOCCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmWinPtr		wp = (DmWinPtr)DmGetWinPtr(w);

    if (wp->attrs & DM_B_TREE_WIN) {
    		DmHelpAppPtr	help_app = DmGetHelpApp(FMAP_HELP_ID(Desktop));

		DmDisplayHelpTOC(w, &(help_app->hlp_win), NULL,
			"DesktopMgr/fmap.hlp", help_app->app_id);

    } else if (wp->attrs & DM_B_FOLDER_WIN) {

		if (strcmp(wp->cp->path, DESKTOP_DIR(Desktop)) == 0) {
         		DmHelpAppPtr help_app = DmGetHelpApp(DESKTOP_HELP_ID(Desktop));

			DmDisplayHelpTOC(w, &(help_app->hlp_win), NULL,
				"DesktopMgr/desktop.hlp", help_app->app_id);
		} else {
    			DmHelpAppPtr help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));

			DmDisplayHelpTOC(w, &(help_app->hlp_win), NULL,
				"DesktopMgr/folder.hlp", help_app->app_id);
		}

    } else if (wp->attrs & DM_B_WASTEBASKET_WIN) {
         DmHelpAppPtr	help_app = DmGetHelpApp(WB_HELP_ID(Desktop));

	    DmDisplayHelpTOC(w, &(help_app->hlp_win), NULL,
			"DesktopMgr/wb.hlp", help_app->app_id);
    } else {
            /* do nothing for now */
    }
} /* end of DmHelpTOCCB */

/****************************procedure*header*****************************
    DmIconMenuProc-
*/
void
DmIconMenuProc(Widget widget, XtPointer client_data, XtPointer call_data)
{
    static Cardinal		num_items = 0;
    DmWinPtr			window = (DmWinPtr)client_data;
    OlFIconBoxButtonCD *	data = (OlFIconBoxButtonCD *)call_data;
    DmItemPtr			item = ITEM_CD(data->item_data);
    Position			root_x, root_y;
    Boolean			items_touched;
    MenuItems *			menu_items;
    Cardinal			count;

    /*
     * Remember for which base window the icon menu pop up, so that if
     * the base window goes away, say alt-f4 was pressed, before the icon
     * menu was pop down, we need to pop down the menu ourselves.
     */
    iconmenu_for = window;

    /* Build menu */
    items_touched = BuildIconMenu(window, item, &menu_items, &count);

    /* If the gizmo hasn't been created, create it */
    if (icon_menu.child == NULL)
    {
	icon_menu.items = menu_items;

	/* Use Desktop Shell since it won't go away */
	(void)CreateGizmo(DESKTOP_SHELL(Desktop), MenuGizmoClass,
			  &icon_menu, NULL, 0);

    } else if ((items_touched) ||
	       (icon_menu.items != menu_items) || (num_items != count))
    {
	icon_menu.items	= menu_items;
	num_items	= count;

	XtVaSetValues(icon_menu.child,
		      XtNitemsTouched, True,
		      XtNitems, (num_items == 0) ? NULL : icon_menu.items,
		      XtNnumItems, num_items,
		      NULL);
    }

    /* Post the menu near the pointer */
    XtTranslateCoords(widget, data->x, data->y, &root_x, &root_y);
    OlPostPopupMenu(widget, icon_menu.parent, data->reason, NULL,
		    root_x, root_y, data->x, data->y);

}				/* End of DmIconMenuProc */

void
DmButtonSelectProc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    DmWinPtr window = (DmWinPtr)client_data;

    DmDisplayStatus(window);
}

/****************************procedure*header*****************************
   The functions DmMenuSelectCB and DmGetWinPtr are used to
   facilitate shared menus among base windows. Before a button
   in a menu is referenced by a menu button, the menu button will
   invoke the Select callback, which is DmMenuButtonSelectCB. This
   remembers which base window is invoked from. Then when any of
   the select callback for buttons in the menu is invoked, it will
   fetch the information by calling DmGetWinPtr.
*/

static void * win_ptr;
static Widget menu_shell;

void *
DmGetLastMenuShell(shell)
Widget *shell;
{
	*shell = menu_shell;
	return(win_ptr);
}

void
DmMenuSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    MenuItems *	item;
    Arg		args[1];
    int		nselected;
    int		item_idx = ((OlFlatCallData *)call_data)->item_index;
    extern	Widget _OlGetShellOfWidget();

    win_ptr = (void *)client_data;

    item = ((OlFlatCallData *)call_data)->items;

    /*
     * Remember the popup menu shell, so that the menu can be popdown if
     * for any reason the base window is unmapped later before menu was
     * pop down.
     */
    menu_shell = _OlGetShellOfWidget(item[item_idx].mod.nextTier->parent);

    /* Get to the submenu's item list */
    item = item[item_idx].mod.nextTier->items;

    XtSetArg(args[0], XtNselectCount, &nselected);
    XtGetValues(((DmWinPtr)win_ptr)->box, args, 1);

    /* convert # of selected items to DM_B_ANY, DM_B_ONE, or DM_B_MORE */
    switch(nselected)
    {
    case 0:
	nselected = DM_B_ANY;
	break;

    case 1:
	nselected = DM_B_ONE;
	break;

    default:
	nselected = DM_B_ONE_OR_MORE;
	break;
    }
	
    for (;item->label; item++)
    {
	DmMenuItemCDType type = (DmMenuItemCDType)item->client_data;

	/* This comparison will only work if the table is true:
	 *
	 *	ANY	> 1	ONE
	 *	-------------------
	 * ANY | OK	-	 -
	 * > 1 | OK	OK	 -
	 * ONE | OK	OK	OK
	 *
	 * Also, if client_data is ANY, don't change item's sensitivity.
	 * Because it may be forced to True or False.
	 */
	switch (type)
	{
	case DM_B_ANY:
	    /* do nothing */
	    break;

	case DM_B_UNDO:
	    item->sensitive = (((DmFolderWinPtr)win_ptr)->task_id != NULL);
	    break;

	case DM_B_ONE:
	case DM_B_ONE_OR_MORE:
	    item->sensitive = (nselected >= (int)(item->client_data));
	default:
	    break;
	}
    }
}

XtPointer
DmGetWinPtr(Widget w)
{
	if (win_ptr)
		DmVaDisplayStatus((DmWinPtr)win_ptr , 0, NULL);
	return(win_ptr);
}

/****************************procedure*header*****************************
    DmPrintCB- called by pressing "Print" on File menu.
*/
void
DmPrintCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmWinPtr	window = (DmWinPtr)client_data;
    DmItemPtr	item;
    int		i;

    for (i = 0, item = window->itp; i < window->nitems; i++, item++)
	if (ITEM_MANAGED(item) && ITEM_SELECT(item))
	    Print(window, ITEM_OBJ(item));
} /* End of DmPrintCB */

