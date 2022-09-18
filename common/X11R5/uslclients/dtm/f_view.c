/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_view.c	1.84"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-view related functions.
*/
						/* #includes go here	*/
#include <libgen.h>
#include <stdlib.h>
#include <locale.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/STextGizmo.h>
#include "Dtm.h"
#include "extern.h"
#include "CListGizmo.h"
#include "SpaceGizmo.h"
#include "dm_strings.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static int	AddToView(DmFolderWindow, DmObjectPtr, Dimension);
static int	I18NByName(DmItemPtr n1, DmItemPtr n2);
static int	ByName(DmItemPtr n1, DmItemPtr n2);
static int	ByPosition(DmItemPtr n1, DmItemPtr n2);
static int	BySize(DmItemPtr n1, DmItemPtr n2);
static int	ByTime(DmItemPtr n1, DmItemPtr n2);
static int	ByType(DmItemPtr n1, DmItemPtr n2);
static void	FilterView(DmFolderWindow);
static Boolean	FindFileMatch(char * objname, char * pattern);
static void	ShowCB(Widget, XtPointer, XtPointer);

					/* public procedures		*/
void		DmFormatView(DmFolderWindow, DmViewFormatType);
void		DmSortItems(DmFolderWindow,
			    DmViewSortType, DtAttrs, DtAttrs, Dimension);
void		DmViewAlignCB(Widget, XtPointer, XtPointer);
void		DmViewCustomizedCB(Widget, XtPointer, XtPointer);
void		DmViewFormatCB(Widget, XtPointer, XtPointer);
void		DmViewSortCB(Widget, XtPointer, XtPointer);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* Make table of compare func's global */
PFI compare_func[] = { ByType, ByName, BySize, ByTime, ByPosition };

static Setting filepattern;

static SpaceGizmo customspace3mm = {
	1, 2
};

static InputGizmo pattern = {
	NULL, "pattern", TXT_FILE_PATTERN, NULL, &filepattern, 24
};

static StaticTextGizmo type_list_label = {
	NULL,			/* help */
	"type_list",		/* name */
	TXT_IB_FILE_TYPE,	/* string */
	WestGravity		/* gravity */
};

static CListGizmo custom_clist = {
	"customlist",		/* name */
	4,			/* view width */
	NULL,			/* required property */
	False,		        /* file */
	False,			/* sys class */
	True,			/* xenix class */
	False,			/* usr class */
 	True,			/* overridden class */
	False,			/* exclusives behavior */
	False,			/* noneset behaviour */
	NULL,			/* select proc */
};


static GizmoRec CustomGiz[] = {
	{SpaceGizmoClass,	&customspace3mm},
	{InputGizmoClass,	&pattern},
	{SpaceGizmoClass,	&customspace3mm},
	{StaticTextGizmoClass,	&type_list_label},
	{CListGizmoClass,	&custom_clist},
};

/* Define the Show menu */
static MenuItems ShowMenubarItems[] = {
	MENU_ITEM( TXT_APPLY,	TXT_M_APPLY,	NULL ),
	MENU_ITEM( TXT_RESET,	TXT_M_RESET,	NULL ),
	MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
	MENU_ITEM( TXT_P_HELP,	TXT_M_HELP,	NULL ),
	{ NULL }
};
MENU_BAR("ShowMenubar", ShowMenubar, ShowCB, 0);	/* default: Apply */

static HelpInfo CustomWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "740", NULL };

static PopupGizmo CustomWindow = {
	&CustomWinHelp,	/* help */
	"custom",			/* widget name */
	TXT_CUSTOM_TITLE,	/* title */
	&ShowMenubar,		/* menu */
	CustomGiz,		/* gizmo array */
	XtNumber(CustomGiz),	/* number of gizmos */
};

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    AddToView - Add The Object To The View.
*/
static int
AddToView(DmFolderWindow folder, DmObjectPtr objptr, Dimension width)
{
    DmItemPtr	ip;

    /* If an Item with this object pointer is already in the
       item list, just reuse it.
    */
    if ( (ip = DmObjectToItem((DmWinPtr)folder, objptr)) != NULL )
    {
	if (ITEM_MANAGED(ip))
	    return(0);			/* new item not added to view */

    } else
    {
	/* No Item with this object exists so get a free item from the list. */
	(void)Dm__GetFreeItems(&folder->itp,
			       &folder->nitems, 1, &ip);

	ip->object_ptr	= (XtArgVal)objptr;
	ip->label	= (XtArgVal)
	    strdup(Dm__MakeItemLabel(ip, folder->view_type, 0));

	DmComputeItemSize(ip, folder->view_type,
			  (Dimension *)&ip->icon_width,
			  (Dimension *)&ip->icon_height);
    }

    /* Get available position if ICONIC view */
    if (folder->view_type == DM_ICONIC)
	DmGetAvailIconPos(folder->itp, folder->nitems,
			  ITEM_WIDTH(ip), ITEM_HEIGHT(ip),
			  width, GRID_WIDTH(Desktop), GRID_HEIGHT(Desktop),
			  (Position *)&ip->x, (Position *)&ip->y);

    ip->managed		= True;
    ip->select		= False;
    ip->busy		= False;
    ip->client_data	= NULL;

    return(1);
}					/* AddToView */

/****************************procedure*header*****************************
    AlignView-
*/
static void
AlignView(DmFolderWindow folder)
{
    OlSWGeometries	swin_geom;
    Dimension		wrap_width;
    DmItemPtr		item;

    /* they are already arranged in other views, not enough reason to
       dim 'Align' button, I guess !
    */
    if (folder->view_type == DM_LONG)
	return;

    swin_geom = GetOlSWGeometries((ScrolledWindowWidget)(folder->swin));
    wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;
    if (folder->view_type == DM_ICONIC)
    {
	Dimension quad_grid_height = GRID_HEIGHT(Desktop) / 4;

    	/* Fix for sorting by position: normalize 'y' to disambiguate */
    	for (item = folder->itp; item < folder->itp + folder->nitems; item++)
		if (ITEM_MANAGED(item))
	    		item->y = (int)(ITEM_Y(item) + quad_grid_height) /
				  (int)GRID_HEIGHT(Desktop);
	
    	DmSortItems(folder,
		DM_BY_POSITION,	DM_B_CALC_SIZE | DM_B_CALC_POS, 0, wrap_width);

	return;
    }
    if (folder->view_type == DM_NAME)
    	DmSortItems(folder, 
		folder->sort_type, DM_B_CALC_SIZE | DM_B_CALC_POS, 0, wrap_width);
    return;
}					/* End of AlignView */

/****************************procedure*header*****************************
    I18NByName-
*/
static int 
I18NByName(DmItemPtr n1, DmItemPtr n2)
{
    if (!ITEM_MANAGED(n2))
	return(0);

    if (!ITEM_MANAGED(n1))
	return (1);

    return (strcoll((char *)(n1->label), (char *)(n2->label)));

}				/* end of ByName */

/****************************procedure*header*****************************
    ByName-
*/
static int 
ByName(DmItemPtr n1, DmItemPtr n2)
{
    if (!ITEM_MANAGED(n2))
	return(0);

    if (!ITEM_MANAGED(n1))
	return (1);

    return (strcmp((char *)(n1->label), (char *)(n2->label)));

}				/* end of ByName */

/****************************procedure*header*****************************
    ByPosition-
*/
static int
ByPosition(DmItemPtr n1, DmItemPtr n2)
{
    if (!ITEM_MANAGED(n2))
	return(-1);

    if (!ITEM_MANAGED(n1))
	return (1);

    if (n1->y != n2->y)
	return( (int)n1->y - (int)n2->y );

    /* Items in same row: return their horiz relationship */
    return( (int)n1->x - (int)n2->x );

}				/* end of ByPosition */

/****************************procedure*header*****************************
    BySize-
*/
static int
BySize(DmItemPtr n1, DmItemPtr n2)
{

    if (!ITEM_MANAGED(n2))
	return(0);

    if (!ITEM_MANAGED(n1))
	return (1);

    return (FILEINFO_PTR(n1)->size - FILEINFO_PTR(n2)->size);

}				/* end of BySize */

/****************************procedure*header*****************************
    ByTime-
*/
static int
ByTime(DmItemPtr n1, DmItemPtr n2)
{
    if (!ITEM_MANAGED(n2))
	return(0);

    if (!ITEM_MANAGED(n1))
	return (1);

    return (FILEINFO_PTR(n2)->mtime - FILEINFO_PTR(n1)->mtime);

}				/* end of ByTime */

/****************************procedure*header*****************************
    ByType-
*/
static int
ByType(DmItemPtr n1, DmItemPtr n2)
{
    int ret;

    if (!ITEM_MANAGED(n2))
	return(-1);

    if (!ITEM_MANAGED(n1))
	return (1);

    /* dir types gets sorted first */
    if (ret = ((int)(ITEM_OBJ(n1)->ftype) - (int)(ITEM_OBJ(n2)->ftype)))
	;
    else {
	if (ret = strcmp(((DmFnameKeyPtr)(FCLASS_PTR(n1))->key)->name,
		 ((DmFnameKeyPtr)(FCLASS_PTR(n2))->key)->name))
		;
    	else
        	ret = strcmp(ITEM_LABEL(n1), ITEM_LABEL(n2));
    }

    return(ret);
}				/* end of ByType */

/****************************procedure*header*****************************
    FilterView - Filter the view according to the filter info
		   stored in the folder structure.

*/
static void
FilterView(DmFolderWindow folder)
{
    int			i;
    DmItemPtr		ip;
    DmObjectPtr		objptr;
    Dimension		width;
    Boolean		allset;
    CListGizmo *	clist;
    Boolean		touched = False;

    /* Get the width of the icon box to pass to GetAvailIconPos */
    XtSetArg(Dm__arg[0], XtNwidth, &width);
    XtGetValues(folder->box, Dm__arg, 1);

    /* Get the char * pattern entered by the user.
       It is up to dtm to free already allocated patterns.
    */
    if (folder->filter.pattern)
	FREE((void *)(folder->filter.pattern));

    folder->filter.pattern = GetInputText(folder->customWindow, 1);

    if (folder->filter.pattern[0] == '\0')
    {
	FREE(folder->filter.pattern);
	folder->filter.pattern = strdup("*");
    }

    /* Get The Clist Gizmo from the gizmo array */
    clist = (CListGizmo *)folder->customWindow->gizmos[4].gizmo;

    /* Assume at least one of the types has to be set */
    allset = True;
    for(i = 0; i < clist->cp->num_objs ; i++)
	if (ITEM_SELECT(clist->itp + i))
	{
	    folder->filter.type[i] = True;
	} else
	{
	    folder->filter.type[i] = False;
	    allset = False;
	}

    /* Filter is off if all types set AND "no pattern" set */
    folder->filter_state = !allset || strcmp(folder->filter.pattern, "*");

    for (i = 0, objptr = folder->cp->op;
	 i < folder->cp->num_objs; i++, objptr = objptr->next)
    {
	if ((objptr->name == NULL) || (objptr->attrs & DM_B_HIDDEN))
	    continue;

	if (DmObjMatchFilter(folder, objptr))
	    touched |= AddToView(folder, objptr, width);

	else if ((ip = DmObjectToItem((DmWinPtr)folder, objptr)) != NULL)
	{
	    touched = True;
	    ip->managed = False;
	    objptr->x = 0;
	    objptr->y = 0;
	}
    }

    /* Re-sort if items "touched" regardless of whether state of filter
       has changed.  (Just re-align in iconic view?)
    */
    if (touched)
    {
	if (folder->view_type == DM_ICONIC)
	    AlignView(folder);
	else
	    DmSortItems(folder, folder->sort_type,
			DM_B_CALC_SIZE | DM_B_CALC_POS, 0, 0);
	DmDisplayStatus((DmWinPtr)folder);
    }

    DmVaDisplayState((DmWinPtr)folder,
		     folder->filter_state ? TXT_FILTER_ON : NULL);

}					/* end of FilterView */

/****************************procedure*header*****************************
    FindFileMatch(char * objname, char * pattern)-
*/
static Boolean
FindFileMatch(char * objname, char * pattern)
{
        char * tmpstring;
        char * localstring;

        localstring = strdup(pattern);
        tmpstring = strtok(localstring, " \t\n");
        while (tmpstring) {
                if (gmatch(objname, tmpstring)) {
                        FREE(localstring);
                        return(True);
                }
                else{
                        tmpstring = strtok(NULL, " \t\n");
                }
        }
        FREE(localstring);
        return(False);
}

/****************************procedure*header*****************************
    ShowCB-
*/
static void
ShowCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    int			i;
    OlFlatCallData *	cd = (OlFlatCallData *)call_data;
    DmFolderWindow	folder = (DmFolderWindow) client_data;
    Widget		shell = GetPopupGizmoShell(folder->customWindow);
    CListGizmo *	clist;
    DmHelpAppPtr    help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));

    switch(cd-> item_index)
    {
    case DM_SHOW_CV:
	FilterView(folder);
	BringDownPopup(shell);
        break;

    case DM_RESET_CV:
	SetInputText(folder->customWindow, 1, "", 0);
	folder->filter.pattern = GetInputText(folder->customWindow, 1);
	clist = (CListGizmo *)folder->customWindow->gizmos[4].gizmo;
	for(i = 0; i < clist->cp->num_objs ; i++) 
	{
	    XtSetArg(Dm__arg[0], XtNset, True);
	    OlFlatSetValues(clist->boxWidget, i, Dm__arg, 1);
	}
        break;

    case DM_CANCEL_CV:
        XtPopdown( (Widget)_OlGetShellOfWidget(w) );
        break;

    case DM_HELP_CV:
     DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
                    NULL, "DesktopMgr/folder.hlp", "740", -1000, -1000);

        break;
    }
}

/***************************public*procedures*****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmFormatView- format the view according to 'type'.

	Assumes 'type' is not the current view type.
*/
void
DmFormatView(DmFolderWindow folder, DmViewFormatType type)
{
    Dimension	wrap_width;
    DtAttrs	layout_options;

    layout_options =
	/* When changing FROM iconic, save icon positions */
	(folder->view_type == DM_ICONIC) ? SAVE_ICON_POS :

    /* When changing FROM long view, restore labels */
    (folder->view_type == DM_LONG) ? UPDATE_LABEL : 0;

    /* When changing TO iconic:
	    Restore icon positions
	    Use width of icon box for wrapping
	    Just "ComputeLayout".
       Otherwise:
	    Use the width of the view.
	    Must sort items.
    */
    if (type == DM_ICONIC)
    {
	layout_options |= RESTORE_ICON_POS;
	
	XtSetArg(Dm__arg[0], XtNwidth, &wrap_width);
	XtGetValues(folder->box, Dm__arg, 1);

	DmComputeLayout(folder->box, folder->itp, folder->nitems, type,
			wrap_width, DM_B_CALC_SIZE, layout_options);

    } else
    {
	OlSWGeometries swin_geom =
	    GetOlSWGeometries((ScrolledWindowWidget)(folder->swin));

	wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;

	/* SPECIAL: when a folder is brought up initially in ICONIC view
	   and there is no .dtinfo, 'type' is passed in as '-1'.
	   */
	if (type == (DmViewFormatType)-1)
	    type = DM_ICONIC;

	folder->view_type = type;	/* DmSortItems needs this */

	DmSortItems(folder, folder->sort_type, DM_B_CALC_SIZE,
		    layout_options, wrap_width);
    }

    /* remember view type in folder */
    folder->view_type = type;

    /* set drawProc */
    XtSetArg(Dm__arg[0], XtNdrawProc,
	     (folder->view_type == DM_LONG) ?	DmDrawLongIcon :
	     (folder->view_type == DM_NAME) ?	DmDrawNameIcon :
	     /* else */				DmDrawLinkIcon);

    /* Icons are only movable in ICONIC view */
    XtSetArg(Dm__arg[1], XtNmovableIcons, folder->view_type == DM_ICONIC);

    DmTouchIconBox((DmWinPtr)folder, Dm__arg, 2);

    /* Now set the granularity of the scrollbar(s) */
    XtSetArg(Dm__arg[0], XtNhStepSize, (type == DM_LONG) ?
	     DM_FontWidth(Desktop) : GRID_WIDTH(Desktop) / 2);
    XtSetArg(Dm__arg[1], XtNvStepSize,
	     (type == DM_ICONIC) ? GRID_HEIGHT(Desktop) / 2 :
	     (type == DM_LONG) ? DM_LongRowHeight(Desktop) :
	     DM_FontHeight(Desktop));
    XtSetValues(folder->swin, Dm__arg, 2);

}					/* end of DmFormatView() */

/****************************procedure*header*****************************
    DmObjMatchFilter-
*/
Boolean
DmObjMatchFilter(DmFolderWindow folder, DmObjectPtr objptr)
{
    return(folder->filter.type[objptr->ftype -1] &&
	   FindFileMatch(objptr->name, folder->filter.pattern));
}

/****************************procedure*header*****************************
    DmSortItems-
*/
void
DmSortItems(DmFolderWindow folder, DmViewSortType sort_type,
	    DtAttrs geom_options, DtAttrs layout_options,
	    Dimension wrap_width)
{
    if ((sort_type == DM_BY_NAME) && strcmp(setlocale(LC_COLLATE, NULL), "C"))
    	qsort(folder->itp, folder->nitems,		/* Sort I18N labels */
	  sizeof(DmItemRec), (int (*)())I18NByName);
    else
    	qsort(folder->itp, folder->nitems,		/* Sort items */
	  sizeof(DmItemRec), compare_func[sort_type]);

    /* As a convenience, if 'wrap_with' is "unspecified", get the width of
       the icon box and use it.
    */
    if (wrap_width == 0)
    {
	XtSetArg(Dm__arg[0], XtNwidth, &wrap_width);
	XtGetValues(folder->box, Dm__arg, 1);
    }

    /* Recompute layout */
    DmComputeLayout(folder->box, folder->itp, folder->nitems,
		    folder->view_type, wrap_width,
		    geom_options, layout_options);

    DmTouchIconBox((DmWinPtr)folder, NULL, 0);		/* Now "touch_items" */

}				/* end of DmSortItems */

/****************************procedure*header*****************************
    DmViewAlignCB-
*/
void 
DmViewAlignCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    AlignView( (DmFolderWindow)DmGetWinPtr(w) );
}

/****************************procedure*header*****************************
    DmViewCustomizedCB-
*/
void 
DmViewCustomizedCB(Widget w, XtPointer client_data, XtPointer call_data)
{

    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    PopupGizmo *	popup;

    if ( (popup = folder->customWindow) == NULL )
    {
	CListGizmo *	clist;
	DmItemPtr	item;

	popup = folder->customWindow =
	    CopyGizmo(PopupGizmoClass, &CustomWindow);

	((MenuGizmo *)popup->menu)->client_data = (char *)folder;
	CreateGizmo(folder->shell, PopupGizmoClass, popup, NULL, 0);

	clist = (CListGizmo *)popup->gizmos[4].gizmo;
	for (item = clist->itp; item < clist->itp + clist->cp->num_objs; item++)
	    item->select = True;

	XtSetArg(Dm__arg[0], XtNitemsTouched, True);
	XtSetArg(Dm__arg[1], XtNselectCount, clist->cp->num_objs);
	XtSetValues(clist->boxWidget, Dm__arg, 2);
    }

    MapGizmo(PopupGizmoClass, popup);
}

/****************************procedure*header*****************************
    DmViewFormatCB-
*/
void 
DmViewFormatCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    OlFlatCallData *	d = (OlFlatCallData *)call_data;
    DmViewFormatType	view_type = (DmViewFormatType)d->item_index;

    if (view_type == folder->view_type)
	return;

    DmFormatView(folder, view_type);
}

/****************************procedure*header*****************************
    DmViewSortCB-
*/
void 
DmViewSortCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    DmViewSortType	sort_type =
	(DmViewSortType)((OlFlatCallData *)call_data)->item_index;
    OlSWGeometries	swin_geom;
    Dimension		wrap_width;

    /* If non-iconic, items are maintained in sorted order */
    if ((folder->view_type != DM_ICONIC) && (folder->sort_type == sort_type))
      return;

    folder->sort_type = sort_type;	/* remember sort type */

    swin_geom = GetOlSWGeometries((ScrolledWindowWidget)(folder->swin));
    wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;
    DmSortItems(folder, sort_type, 0, 0, wrap_width);

}					/* end  of DmViewSortCB */
