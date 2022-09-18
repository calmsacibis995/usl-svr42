/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_update.c	1.121"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-releated callbacks.
*/
						/* #includes go here	*/
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/TextField.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <FileGizmo.h>
#include <ModalGizmo.h>
#include <PopupGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

static void	AddNameToParent(char *, char *, Position, Position, Dimension);
static Cardinal	AddObjToFolder(DmFolderWindow, DmObjectPtr,
			       Position, Position, Dimension);
static void	CloseDir(char * dir);
static void	FileOpDone(DmFileOpInfoPtr opr_info);
static DmWinPtr	GetParentWin(char * target_path);
static void	OverwriteCB(Widget, XtPointer, XtPointer);
static void	RmItemFromFolder(DmFolderWindow folder, DmItemPtr item);
static void	RmNameFromParent(char * dir, char * name);

					/* public procedures		*/
Cardinal	DmAddNameToFolder(DmFolderWindow, char *,
				  Position, Position, Dimension);
Cardinal	DmAddObjToFolder(DmFolderWindow, DmObjectPtr,
				 Position, Position, Dimension);
void 		DmAllBusyWindow(DmFolderWindow, Boolean);
void		DmFolderFMProc(DmProcReason, XtPointer client_data,
			       XtPointer call_data, char * str1, char * str2);
void		DmFolderPathChanged(char * dir1, char * dir2);
void		DmPromptOverwrite(DmFolderWindow win, char * file);
void		DmRmItemFromFolder(DmFolderWindow folder, DmItemPtr item);
void		DmUpdateWindow(DmFileOpInfoPtr, DtAttrs);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/
/* Define gizmo for Overwrite Notice */

static MenuItems menubarItems[] = {
    MENU_ITEM( TXT_OVERWRITE,		TXT_M_OVERWRITE,	NULL ),
    MENU_ITEM( TXT_DONT_OVERWRITE,	TXT_M_DONT_OVERWRITE,	NULL ),
    { NULL }				/* NULL terminated */
};
					/* default: Cancel */
MENU_BAR("overwriteNoticeMenubar", menubar, OverwriteCB, 1);

static ModalGizmo notice_gizmo = {
    NULL,				/* help info */
    "overwriteNotice",		/* shell name */
    TXT_G_FOLDER_TITLE,		/* title */
    &menubar,			/* menu */
    NULL,			/* message (run-time) */
    NULL, 0,			/* gizmos, num_gizmos */
};

typedef enum { Overwrite, DontOverwrite } OverwriteType;


/***************************private*procedures****************************

    Private Procedures
*/
/****************************procedure*header*****************************
    AddNameToParent-
*/
static void
AddNameToParent(char * dir, char * name, Position x, Position y, Dimension wrap_width)
{
    register DmFolderWindow folder;

    if ( (folder = (DmFolderWindow)GetParentWin(dir)) != NULL ) {
	(void)DmAddNameToFolder(folder, name, x, y, wrap_width);
	if ((folder->view_type != DM_ICONIC) &&
	    !IS_TREE_WIN(Desktop, folder))
	    DmSortItems(folder, folder->sort_type, 0, 0, 0);
    }

}					/* end of AddNameToParent */

/****************************procedure*header*****************************
    AddObjToFolder- this does all the work for DmAddObjToFolder except it
	doesn't attempt to update tree view.  It is broken out so that
	UpdateWindow can use it when it know tree view has already be
	updated.
*/
static Cardinal
AddObjToFolder(DmFolderWindow win, DmObjectPtr obj,
	       Position x, Position y, Dimension pane_width)
{
    DmItemPtr	item;

    if (obj->attrs & DM_B_HIDDEN)
	return (OL_NO_ITEM);

    if ((win->filter_state) && !DmObjMatchFilter(win, obj))
    {
	Dm__AddToObjList(win->cp, obj, 0);
	if (win->view_type == DM_ICONIC)
	    DmInitObjType(win->box, obj);
	else
	    DmRetypeObj(obj);

	return(OL_NO_ITEM);
    }
	
    if (win->view_type == DM_ICONIC)
	return(Dm__AddObjToIcontainer(win->box, &win->itp, &win->nitems,
				      win->cp, obj, x, y, DM_B_CALC_SIZE,
				      DESKTOP_FONTLIST(Desktop),
				      DESKTOP_FONT(Desktop),
				      pane_width,
				      GRID_WIDTH(Desktop),
				      GRID_HEIGHT(Desktop)));

    /* The rest of this emulates Dm__AddObjToIcontainer for non-ICONIC
       views.  Note that for non-ICONIC views, the caller will recompute the
       layout so that the new item appears in it's ordered position.  The
       size must be computed so item can be positioned during layout.
       'managed' can be set directly.
    */
    /* obj may be already attached but this will ignore it if so */
    Dm__AddToObjList(win->cp, obj, 0);
    DmRetypeObj(obj);

    /* Get "free" item from FIconBox */
    (void)Dm__GetFreeItems(&win->itp, &win->nitems, 1, &item);

    /* Generate the item's label (based on view-type) and size it */
    item->object_ptr = (XtArgVal)obj;
    item->label = (XtArgVal)strdup(Dm__MakeItemLabel(item, win->view_type, 0));

    DmComputeItemSize(item, win->view_type, (Dimension *)&item->icon_width,
			(Dimension *)&item->icon_height);

    item->x		= 0;		/* generated during layout */
    item->y		= 0;		/* generated during layout */
    item->managed	= True;
    item->select	= False;
    item->busy		= False;
    item->client_data	= NULL;
    item->object_ptr	= (XtArgVal)obj;

    return( (Cardinal)(item - win->itp) );
}

/****************************procedure*header*****************************
    DmAllBusyWindow- [un]busy folder window at start or end of file op.
	Also busy any popups in an attempt to "serialize" file op requests
	from user.  Only single file op can occur at a time for a given src
	window.
*/
void
DmAllBusyWindow(DmFolderWindow folder, Boolean busy)
{
    if ((folder->copy_prompt != NULL) && (folder->copy_prompt->shell != NULL))
	DmBusyWindow(folder->copy_prompt->shell, busy, DmStopCB, folder);

    if ((folder->move_prompt != NULL) && (folder->move_prompt->shell != NULL))
	DmBusyWindow(folder->move_prompt->shell, busy, DmStopCB, folder);

    if ((folder->link_prompt != NULL) && (folder->link_prompt->shell != NULL))
	DmBusyWindow(folder->link_prompt->shell, busy, DmStopCB, folder);

    if ((folder->rename_prompt != NULL) && (folder->rename_prompt->shell != NULL))
	DmBusyWindow(folder->rename_prompt->shell, busy, DmStopCB, folder);

    DmBusyWindow(folder->shell, busy, DmStopCB, folder);

    if (busy)
	folder->attrs |= DM_B_FILE_OP_BUSY;
    else
	folder->attrs &= ~DM_B_FILE_OP_BUSY;

}					/* end of DmAllBusyWindow */

/****************************procedure*header*****************************
    CloseDir-
*/
static void
CloseDir(register char * dir)
{
    register DmFolderWindow folder;

    if ( (folder = DmQueryFolderWindow(dir)) != NULL )
	DmCloseFolderWindow(folder);
}

/****************************procedure*header*****************************
    FileOpDone- do whatever's necessary following a completed file operation
*/
static void
FileOpDone(DmFileOpInfoPtr opr_info)
{
    switch(opr_info->type)
    {
    case DM_DELETE:			/* for undo-ing move, copy or links */
	DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
	break;

    case DM_COPY:
    case DM_HARDLINK:
    case DM_SYMLINK:
	/* If the target is an existing dir, update it if it's open.
	   Otherwise, if a directory was created, display the glyph in the
	   parent folder if open.
	*/
	if (opr_info->attrs & DM_B_DIR_EXISTS)
	    DmUpdateWindow(opr_info, DM_UPDATE_DSTWIN);

	else
	{
	    if ( !(opr_info->attrs & DM_B_DIR_NEEDED_4FILES) )
	    {
		/* Target is a file and not multi-srcs */
		if (opr_info->src_info[0] & SRC_B_IGNORE)
		    break;

		if ((opr_info->src_info[0] & SRC_TYPE_MASK) == DM_IS_DIR)
		    RmNameFromParent(opr_info->target_path,
				     basename(opr_info->target_path));
	    }

	    AddNameToParent(opr_info->target_path,
			    basename(opr_info->target_path),
			    opr_info->x, opr_info->y, 0);
	}
	break;
	
    case DM_MOVE:
	if (IS_WB_PATH(Desktop, opr_info->target_path))
	{
	    DmMoveFileToWB(opr_info, False);
	    break;
	}

	/* Update src and dst windows */
	DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN | DM_UPDATE_DSTWIN);

	/* If new dir was created, display it in parent (if any) */
	if (opr_info->attrs & DM_B_DIR_NEEDED_4FILES)
	    AddNameToParent(opr_info->target_path,
			    basename(opr_info->target_path),
			    opr_info->x, opr_info->y, 0);
	break;

    default:
	break;
    }

    /* For UNDO, remove the dir icon from the parent dir if one was created
       originally on behalf of the user (the mutli-src case).
    */
    if (opr_info->options & RMNEWDIR)
    {
	/* Close folder if one is open on this dir */
	CloseDir(opr_info->src_path);		/* ie. original target */

	/* Remove folder glyph from parent if open */
	RmNameFromParent(opr_info->src_path, basename(opr_info->src_path));
    }
}					/* end of FileOpDone */

/****************************procedure*header*****************************
    GetParentWin-
*/
static DmWinPtr
GetParentWin(char * path)
{
    DmFolderWindow	folder;
    char *		dup_path = strdup(path);

    folder = DmQueryFolderWindow(dirname(dup_path));
    FREE((void *)dup_path);

    return((DmWinPtr)folder);
}

/****************************procedure*header*****************************
  OverwriteCB-
*/
static void
OverwriteCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	d = (OlFlatCallData *)call_data;
    DmFolderWindow	folder = (DmFolderWindow)client_data;
    DmTaskInfoListPtr	tip = (DmTaskInfoListPtr)
	(folder->overwriteNotice->menu->client_data);

    /* (Taken from DmBusyWindow) */
    OlRemoveCallback(folder->overwriteNotice->shell,
		     XtNconsumeEvent, DmStopCB, folder);
    OlRemoveCallback(folder->overwriteNotice->menu->child,
		     XtNconsumeEvent, DmStopCB, folder);

    XtPopdown(folder->overwriteNotice->shell);
    Dm__Overwrite(tip, d->item_index == Overwrite);
}

/****************************procedure*header*****************************
    RmItemFromFolder-
*/
static void
RmItemFromFolder(DmFolderWindow folder, DmItemPtr item)
{
    /* Unmanage the item from view */
    XtSetArg(Dm__arg[0], XtNmanaged, False);
    OlFlatSetValues(folder->box, item - folder->itp, Dm__arg, 1);

    XtFree(ITEM_LABEL(item));

    DmDelObjectFromContainer(folder->cp, ITEM_OBJ(item));

}					/* end of RmItemFromFolder */

/****************************procedure*header*****************************
    RmNameFromParent-
*/
static void
RmNameFromParent(char * dir, char * name)
{
    register DmFolderWindow	folder;
    register DmItemPtr		item;

    if (((folder = (DmFolderWindow)GetParentWin(dir)) != NULL) &&
	((item = DmObjNameToItem((DmWinPtr)folder, name)) != NULL)) {
	DmRmItemFromFolder(folder, item);
	if ((folder->view_type != DM_ICONIC) &&
	    !IS_TREE_WIN(Desktop, folder))
	    DmSortItems(folder, folder->sort_type, 0, 0, 0);
    }
}				/* end of RmNameFromParent */

/***************************private*procedures****************************

    Public Procedures
*/
/****************************procedure*header*****************************
    DmAddNameToFolder-
*/
Cardinal
DmAddNameToFolder(DmFolderWindow folder, char * name,
		  Position x, Position y, Dimension wrap_width)
{
    DmObjectPtr obj;

    obj = Dm__CreateObj(folder->cp, name, DM_B_SET_TYPE | DM_B_INIT_FILEINFO);

    if (obj == NULL)
	return(OL_NO_ITEM);

    /* As a convenience, if wrap width is "unspecified" and this is
       ICONIC, get the pane width.
    */
    if ((folder->view_type == DM_ICONIC) && (wrap_width == 0))
    {
	XtSetArg(Dm__arg[0], XtNwidth, &wrap_width);
	XtGetValues(folder->box, Dm__arg, 1);
    }

    return( DmAddObjToFolder(folder, obj, x, y, wrap_width) );

}				/* end of DmAddNameToFolder */

/****************************procedure*header*****************************
    DmAddObjToFolder-
*/
Cardinal
DmAddObjToFolder(DmFolderWindow win, DmObjectPtr obj,
		   Position x, Position y, Dimension pane_width)
{
    Cardinal	indx = AddObjToFolder(win, obj, x, y, pane_width);

    if (OBJ_IS_DIR(obj))
    {
	char buf[PATH_MAX];
	Dm__UpdateTreeView(NULL, Dm_ObjPath(obj, buf));
    }

    return(indx);
}					/* end of DmAddObjToFolder */

/****************************procedure*header*****************************
    DmFolderFMProc- A client_proc for the folder window file manipulation.
	It is called when an operation begins, error occurs, successful
	completion, to report progress of the operation.
*/
void 
DmFolderFMProc(DmProcReason reason, XtPointer client_data, XtPointer call_data,
	       char * src_file, char * dst_file)
{
    DmTaskInfoListPtr	tip = (DmTaskInfoListPtr)call_data;
    DmFileOpInfoPtr	opr_info = tip->opr_info;
    DmFolderWindow *	src_win;
    DmFolderWindow *	dst_win;

    if (opr_info->options & UNDO)
    {
	src_win = &opr_info->dst_win;
	dst_win = &opr_info->src_win;
    } else
    {
	src_win = &opr_info->src_win;
	dst_win = &opr_info->dst_win;
    }

    switch(reason)
    {
    case DM_OPRBEGIN:
	/* operation is about to begin, busy out src_win & dst_win as appro. */
	if ((*src_win != NULL) ||
	    (!(opr_info->options & MULTI_PATH_SRCS) &&
	     ((*src_win = DmQueryFolderWindow(opr_info->src_path)) != NULL)))
	{
	    DmAllBusyWindow(*src_win, True);
	}

	/* If the target is an existing dir, busy it and use it during
	   UpdateWindow (this is where the new icons will go).  If the target
	   doesn't exist, there is definitely no window for the new icons.
	   Still the parent window can be busied but do not attempt to use
	   this is in UpdateWindow.
	*/
	if (opr_info->attrs & DM_B_DIR_EXISTS)
	{
	    if ((*dst_win == NULL) && (opr_info->type != DM_DELETE))
		*dst_win = DmQueryFolderWindow(opr_info->target_path);
	}

	if ((*dst_win != NULL) && (*dst_win != *src_win))
	    DmAllBusyWindow(*dst_win, True);

	break;

    case DM_DONE:
	/* operation successfully completed.
	   Note: DM_DONE is always called (no option is needed) so must be
	   careful to test for NULL src_win & dst_win.
	*/

	if (*src_win != NULL)		/* Unbusy src_win if any */
	    DmAllBusyWindow(*src_win, False);

	/* Unbusy dst_win if any and if !src_win */
	if ((*dst_win != NULL) && (*dst_win != *src_win))
	    DmAllBusyWindow(*dst_win, False);

	FileOpDone(opr_info);		/* Update folder(s) */

	if (*src_win != NULL)
	    if (opr_info->attrs & DM_B_FILE_OP_STOPPED)
		DmVaDisplayStatus((DmWinPtr)*src_win, True,
				  TXT_FILE_OP_STOPPED);

	    else if (opr_info->error == 0)
		DmVaDisplayStatus((DmWinPtr)*src_win, False,
			(opr_info->type == DM_DELETE	? TXT_DELETE_DONE :
			 opr_info->type == DM_COPY	? TXT_COPY_DONE :
			 opr_info->type == DM_HARDLINK	? TXT_LINK_DONE :
			 opr_info->type == DM_SYMLINK	? TXT_LINK_DONE :
			 opr_info->type == DM_MKDIR	? TXT_MKDIR_DONE :

			 /* This is ugly.  If this is a move and the
			    target is the WB, it's really a delete.
			 */
			 opr_info->type == DM_MOVE	?
			 (IS_WB_PATH(Desktop, opr_info->target_path)
							? TXT_DELETE_DONE :
							  TXT_MOVE_DONE) :
			/* else */			  NULL),

			src_file);

	/* Free task info now for UNDO and external DnD's or when moving
	   to the WB and its clean-up method is immediate.
	*/
	if ((opr_info->options & (EXTERN_DND | UNDO)) ||
	    ((opr_info->type == DM_MOVE) && DM_WBIsImmediate(Desktop) &&
	     IS_WB_PATH(Desktop, opr_info->target_path)) ||
		(opr_info->src_win != DESKTOP_WB_WIN(Desktop) &&
		 opr_info->type == DM_DELETE && DM_WBIsImmediate(Desktop)))
	{
	    /* For external DnD's, signal completion */
	    if (opr_info->options & EXTERN_SEND_DONE)
	    {
		DmDnDFileOpPtr dnd_info = (DmDnDFileOpPtr)client_data;

		OlDnDDragNDropDone(dnd_info->wp->box, dnd_info->selection,
				   dnd_info->timestamp, NULL, NULL);
		FREE((void *)dnd_info);
	    }

	    if (*src_win != NULL)
		(*src_win)->task_id = 0;
	    DmFreeTaskInfo(tip);
	}
	break;

    case DM_REPORT_PROGRESS:
	if (*src_win == NULL)
	    break;

	/* report progress info. about the operation.
	   Note: using the REPORT_PROGRESS option assumes src_win != NULL
	*/
#define MSG_(type_, msg_)	(opr_info->type == type_) ? msg_

	DmVaDisplayStatus((DmWinPtr)*src_win, False,
			  MSG_(DM_DELETE,	TXT_DELETE_IN_PROGRESS) :
			  MSG_(DM_COPY,		TXT_COPY_IN_PROGRESS) :
			  MSG_(DM_HARDLINK,	TXT_LINK_IN_PROGRESS) :
			  MSG_(DM_SYMLINK,	TXT_LINK_IN_PROGRESS) :
			  MSG_(DM_MKDIR,	TXT_MKDIR_IN_PROGRESS) :
			  MSG_(DM_MOVE,		TXT_MOVE_IN_PROGRESS) :
			   /* else */		NULL,
			  basename(src_file));
#undef MSG_
	break;

    case DM_OVRWRITE:
	/* An overwrite condition occurred.
	   Note: using the OVERWRITE option assumes src_win != NULL
	*/
	DmPromptOverwrite(*src_win, dst_file);
	break;

    case DM_ERROR:
	/* Error occurred during file op: display approp message.
	   Note: DM_ERROR is always called (no option is needed) so must be
	   careful to test for NULL src_win & dst_win.
	*/
	if ((opr_info->error != 0) && (*src_win != NULL))
	    DmVaDisplayStatus((DmWinPtr)*src_win, True,
			      StrError(opr_info->error), src_file,
			      IS_WB_PATH(Desktop, opr_info->target_path) ?
			      GetGizmoText(TXT_WB_TITLE) : dst_file);
	break;

    default: 
	break;
    }
}				/* end of DmFolderFMProc */

/****************************procedure*header*****************************
    DmFolderPathChanged- called when a window's path is changed or removed:
	Update visited folders array (if path is found there).
	Update path of "root" folder (if open) and any descendants.
*/
void
DmFolderPathChanged(char * old_path, char * new_path)
{
    register DmFolderWindow	folder;
    int				old_len;

    Dm__UpdateVisitedFolders(old_path, new_path);
    Dm__UpdateTreeView(old_path, new_path);
	
    old_len = strlen(old_path);		/* do it once outside of loop */
    for (folder = DESKTOP_FOLDERS(Desktop);
	folder != NULL; folder = folder->next)
    {
	int status =
	    DmSameOrDescendant(old_path, folder->cp->path, old_len);

	if (status != 0)
	{
	    char *		dir_path;

	    /* open folder found... close it if its path is 'old_path' (or
	       a descendant) and 'old_path' is being deleted.
	    */
	    if (new_path == NULL)
	    {
		DmCloseFolderWindow(folder);
		continue;
	    }

	    dir_path =
		strdup((status < 0) ? new_path :	/* exact match */
		       DmMakePath(new_path, folder->cp->path + old_len + 1));

	    /* Update the path in the container cache */
	    DtDelData(NULL, DM_CACHE_FOLDER,
		      (void *)folder->cp->path, strlen(folder->cp->path) + 1);

	    XtFree(folder->cp->path);
	    folder->cp->path = dir_path;
	    DtPutData(NULL, DM_CACHE_FOLDER, folder->cp->path,
		      strlen(dir_path) + 1, folder->cp);

	    DmUpdateFolderTitle(folder);
	}
    }
}					/* end of DmFolderPathChanged */

/****************************procedure*header*****************************
    DmPromptOverwrite- popup overwrite notice.
	The round-about way of passing the current task ptr to the callback
	is to first register src_win as the client_data before CreateGizmo.
	This will be passed to the widget when created.  Then client_data is
	set to DESKTOP_CUR_TASK afterwards which is not set on the widget but
	can be accessed from the notice callbacks.
*/
void
DmPromptOverwrite(DmFolderWindow win, char * file)
{
    ModalGizmo *	notice;

    /* Create the Notice gizmo (once per folder) */
    if ((notice = win->overwriteNotice) == NULL)
    {
	notice = win->overwriteNotice =
	    CopyGizmo(ModalGizmoClass, &notice_gizmo);
	notice->menu->client_data = (char *)win;
	CreateGizmo(XtParent(win->box), ModalGizmoClass, notice, NULL, 0);
    }

    /* Construct I18N Notice text with file name, etc */
    sprintf(Dm__buffer, Dm__gettxt(TXT_OVERWRITE_NOTICE), file);
    SetModalGizmoMessage(notice, Dm__buffer);
    notice->menu->client_data = (char *)DESKTOP_CUR_TASK(Desktop);
    MapGizmo(ModalGizmoClass, notice);

    /* Should this be done in general or just for file op's between folders?
       (Taken from DmBusyWindow)
    */
    OlAddCallback(notice->shell, XtNconsumeEvent, DmStopCB, win);
    OlAddCallback(notice->menu->child, XtNconsumeEvent, DmStopCB, win);

}					/* end of DmPromptOverwrite */


/****************************procedure*header*****************************
    DmRmItemFromFolder-
*/
void
DmRmItemFromFolder(DmFolderWindow folder, DmItemPtr item)
{
    RmItemFromFolder(folder, item);

    if (ITEM_IS_DIR(item))
    {
	char buf[PATH_MAX];
	DmFolderPathChanged(Dm_ObjPath(ITEM_OBJ(item), buf), NULL);
    }
}					/* end of DmRmItemFromFolder */

/* Macro to increment drop x & y so that items are cascaded.
   Note that x, y  will be further adjusted when item is added to
   container for upper-left positioning.
*/
#define CascadeItem(win, indx, x, y) \
    if ((indx != OL_NO_ITEM) && \
	(x != UNSPECIFIED_POS) && (win->view_type == DM_ICONIC)) \
	x += GLYPH_PTR(DM_WIN_ITEM(win, indx))->width / 2, \
	y += GLYPH_PTR(DM_WIN_ITEM(win, indx))->height / 2

/****************************procedure*header*****************************
    DmUpdateWindow()- core function to update the window after a File
	manipulation operation is done. 'options' allows caller to indicate
	what kind of updates they are interested in. e.g. a delete operation
	will only be interested in updating src. window, a copy operation
	will only be interested in updating a destination window and move
	opr. will be interested in both. Also, after the files (items) are
	moved we need to keep objectlist in sync., unmange item from the
	itemlist etc. For new files that gets added via operation, we may
	need to get new object structure or in some case (e.g. move), we may
	be able to use the old one (i.e. src obejct).
	
	If either the src or dst windows are not in ICONIC view, their layout
	must be maintained.  This results "holes" being filled in the source
	(when a file is moved or deleted) and new items appearing in the next
	available slot in the destination.

				DM_UPDATE_SRCWIN
				True	False
	DM_UPDATE_DSTWIN
			True	move	copy/create
			False	delete	no-op
*/
void
DmUpdateWindow(DmFileOpInfoPtr opr_info, DtAttrs options)
{
    DmItemPtr		item = NULL;
    Dimension		wrap_width;
    char **		src;
    DtAttrs *		src_info;
    Cardinal		prev_indx;
    Boolean		use_dst_win;

    if (opr_info->cur_src == 0)		/* There is nothing to do */
	return;

    /* Do some work outside of the loop below for the dst_win: Can only use
       dst_win if it's defined and NOT undo'ing multi-path op.  If items will
       be added to dst_win, get the width of the view so items can be
       positioned (wrapped) correctly.
    */
    if ((options & DM_UPDATE_DSTWIN) && (opr_info->dst_win != NULL) &&
	((opr_info->options &
	  (MULTI_PATH_SRCS | UNDO)) != (MULTI_PATH_SRCS | UNDO)))
    {
	use_dst_win = True;

	XtSetArg(Dm__arg[0], XtNwidth, &wrap_width);
	XtGetValues(opr_info->dst_win->box, Dm__arg, 1);

	prev_indx = OL_NO_ITEM;
    } else
	use_dst_win = False;

    for (src = opr_info->src_list, src_info = opr_info->src_info;
	 src < opr_info->src_list + opr_info->cur_src; src++, src_info++)
    {
	char *		src_bname;
	char *		dst_name;
	DmItemPtr	dst_item;

	if (*src_info & SRC_B_IGNORE)
	    continue;

	src_bname = basename(*src);

	/****	Update src_win (DELETE or MOVE)		****/

	if (options & DM_UPDATE_SRCWIN)
	{
	    Boolean undo_multi =
		((opr_info->options &
		  (UNDO | MULTI_PATH_SRCS)) == (UNDO | MULTI_PATH_SRCS));

	    /* Deleteing or moving a dir can affect the visited folders array
	       and other folder descendants.  For example, if /foo/bar is
	       moved to /tmp/bar, the paths of the folder displaying /foo/bar
	       (if any) and its descendants (if any) must be updated.  If
	       /foo/bar is deleted, those folder must be closed.  This is
	       only necessary if the src item is a directory.
	       
	       (src_win can be NULL for an UNDO (where the dtm folder window
	       may not be open) or if a client asks the wastebasket to move a
	       file to the wastebasket (there is no dtm window)).
	    */
	    if ((*src_info & SRC_TYPE_MASK) == DM_IS_DIR)
	    {
		char * old_path;
		char * new_path;

		old_path = strdup(DmMakePath(opr_info->src_path,
					     undo_multi ? src_bname : *src));

		/* If this is a DELETE (no dst_win or dst_win is wastebasket),
		   don't supply a new path since there is none.  Otherwise,
		   construct a new path depending on whether the target was
		   created by this operation or not.
		*/
		if (!(options & DM_UPDATE_DSTWIN) ||
		    IS_WB_WIN(Desktop, opr_info->dst_win))
		{
		    new_path = NULL;

		} else if ((opr_info->attrs &
			    (DM_B_DIR_EXISTS | DM_B_TARGET_ADJUSTED)) ==
			   (DM_B_DIR_EXISTS | DM_B_TARGET_ADJUSTED))
		{
		    new_path =
			strdup(DmMakePath(opr_info->target_path,
					  undo_multi ? *src : src_bname));
		} else
		    new_path = strdup(opr_info->target_path);

		DmFolderPathChanged(old_path, new_path);

		XtFree(old_path);
		XtFree(new_path);

	    }

	    /* Set 'item' so it can be used during dst_win update below.
	       'item' should not be NULL except that the sync timer may have
	       already removed the item (ie, while an overwrite notice is up).
	       Note: 'item' != NULL means src_win != NULL.
	    */
	    
	    if (opr_info->src_win != NULL)
		item = DmObjNameToItem((DmWinPtr)opr_info->src_win,
				       undo_multi ? src_bname : *src);

	    /* Only remove the item if this is a delete or it's a move and
	       the src and dst windows are different.  Moving within the
	       same window (a rename) is handled while updating dst_win.
	       Note: DmFolderPathChanged (above) updates tree view.
	    */
	    if ((item != NULL) && !IS_TREE_WIN(Desktop, opr_info->src_win) &&
		(!(options & DM_UPDATE_DSTWIN) ||
		 (opr_info->src_win != opr_info->dst_win)))
		RmItemFromFolder(opr_info->src_win, item);

	    /* Even if src_win is NULL, if the src_list contains "multi-paths",
	       there may be a folder opened which contains the item.
	       Note: UNDO becomes multi-path "target".
	    */
	    if (opr_info->options & MULTI_PATH_SRCS)
	    {
		DmFolderWindow parent =
		    DmQueryFolderWindow(DmMakeDir((opr_info->options & UNDO) ?
						  opr_info->target_path :
						  opr_info->src_path, *src) );

		if ((parent != NULL) && (parent != opr_info->src_win))
		    Dm__SyncFolder(parent);
	    }
	}

	/* ****  Update dst_win (MOVE, COPY, CREATE or undo MOVE)  ****
	   
	   See if item already exists in dst_win.  This can occur when an
	   item is overwritten or the sync timer may have already added the
	   item while the overwrite notice is up (for a later item).
	   
	   If UPDATE_SRCWIN, then opr is a MOVE so make use of the object
	   from src_win (can't re-use objs from tree view, though). Otherwise
	   (for COPY, etc), just add a new item to dst_win.
	   
	   Note that (for MOVE) 'item' may have already be removed from
	   src_win as by the sync timer example mentioned above.  In all
	   cases, items in the dst_win must be cascaded.
	   
	   For MOVE, if 'item' != NULL, re-use the obj (but only if it's the
	   same file-type): change its name and (must) re-class it.  Finally,
	   add the obj to the dst_win if it is different from src_win.
	   Otherwise (a rename), just update the label.
	*/
	if (!use_dst_win)
	    continue;

	dst_name = ((opr_info->attrs &
		     (DM_B_DIR_EXISTS | DM_B_TARGET_ADJUSTED)) ==
		    (DM_B_DIR_EXISTS | DM_B_TARGET_ADJUSTED)) ?
			src_bname : basename(opr_info->target_path);

	dst_item = DmObjNameToItem((DmWinPtr)opr_info->dst_win, dst_name);

	/* If item already in dst_win, use it instead of item found in
	   src_win (if any) as long as it's the same type.
	*/
	if (dst_item != NULL)
	{
	    if ((item != NULL) &&
		(ITEM_OBJ(item)->ftype == ITEM_OBJ(dst_item)->ftype))
	    {
		/* Use dst_item (just re-position it if necessary) */
		if ((opr_info->x != UNSPECIFIED_POS) &&
		    (opr_info->dst_win->view_type == DM_ICONIC))
		{
		    DmGlyphPtr glyph = GLYPH_PTR(dst_item);
		    
		    prev_indx = (Cardinal)(dst_item - opr_info->dst_win->itp);

		    opr_info->x -= glyph->width / 2;
		    opr_info->y -= glyph->height / 2;

		    XtSetArg(Dm__arg[0], XtNx, opr_info->x);
		    XtSetArg(Dm__arg[1], XtNy, opr_info->y);
		    OlFlatSetValues(opr_info->dst_win->box,
				    prev_indx, Dm__arg, 2);

		    opr_info->x += glyph->width;
		    opr_info->y += glyph->height;
		}

		/* Now deal with the un-needed src-item:
		   If "rename", item hasn't been removed from view yet
		*/
		if (!IS_TREE_WIN(Desktop, opr_info->src_win))
		{
		    if (opr_info->src_win == opr_info->dst_win)
			RmItemFromFolder(opr_info->src_win, item);
		    Dm__FreeObject(ITEM_OBJ(item));
		}

		continue;
	    }
	    /* else: can't make use of dst_item; may be diff type */
	    RmItemFromFolder(opr_info->dst_win, dst_item);
	}

	if ((options & DM_UPDATE_SRCWIN) && (item != NULL) && /* MOVE */
	    !IS_TREE_WIN(Desktop, opr_info->src_win))
	{
	    DmObjectPtr obj = ITEM_OBJ(item);

	    XtFree(obj->name);
	    obj->name = strdup(dst_name);

	    if (opr_info->src_win == opr_info->dst_win) /* ie. RENAME */
	    {
		char * new_label =
		    strdup(Dm__MakeItemLabel(item,
					     opr_info->dst_win->view_type,
					     0));
		char *save_label = ITEM_LABEL(item);
		int save_w = ITEM_WIDTH(item);
		int save_h = ITEM_HEIGHT(item);

		DmRetypeObj(obj);	/* Retype the object */

		item->label = (XtArgVal)new_label;

		/* Now recompute its size */
		DmComputeItemSize(item, opr_info->dst_win->view_type,
				  (Dimension *)&item->icon_width,
				  (Dimension *)&item->icon_height);


		if (opr_info->dst_win->view_type == DM_ICONIC) {
		    Position  new_x;

		    new_x = ITEM_X(item) + (save_w - (int)ITEM_WIDTH(item)) / 2;
		    XtSetArg(Dm__arg[0], XtNlabel, item->label);
		    XtSetArg(Dm__arg[1], XtNwidth, item->icon_width);
		    XtSetArg(Dm__arg[2], XtNheight, item->icon_height);
		    XtSetArg(Dm__arg[3], XtNx, new_x);

		    /* restore dimension */
		    item->icon_width  = (XtArgVal)save_w;
		    item->icon_height = (XtArgVal)save_h;
		    item->label       = (XtArgVal)save_label;

		    OlFlatSetValues(opr_info->dst_win->box, (Cardinal)
				    (item - opr_info->dst_win->itp),
				    Dm__arg, 4);
		}

		/*
		 * Don't free label earlier, because item setvalue needs to
		 * make a comparison of the new and the current.
		 */
		XtFree(save_label);
	    } else			/* MOVE but not RENAME */
	    {
		CascadeItem(opr_info->dst_win, prev_indx,
			    opr_info->x, opr_info->y);

		prev_indx = AddObjToFolder(opr_info->dst_win, obj,
					   opr_info->x, opr_info->y,
					   wrap_width);

		/* Retype object if source window is Wastebasket. */
		if (IS_WB_PATH(Desktop, opr_info->src_path))
			DmRetypeObj(obj);
	    }
	} else				/* COPY, CREATE or Undo MOVE */
	{
	    CascadeItem(opr_info->dst_win, prev_indx,
			opr_info->x, opr_info->y);

	    prev_indx =
		DmAddNameToFolder(opr_info->dst_win, dst_name,
				  opr_info->x, opr_info->y,
				  wrap_width);
	}
    }					/* for all src_list items */

    /* Time stamp src and dst windows and, if their views are not ICONIC,
       keep contents sorted.  Only time-stamp src_win if not already stamped
       (if src_list does not have "multi-paths").
    */
    if ((options & DM_UPDATE_SRCWIN) &&	(opr_info->src_win != NULL))
    {
#ifdef NOT_USE
	if ((opr_info->src_win->attrs & DM_B_FOLDER_WIN) &&
	    !(opr_info->options & MULTI_PATH_SRCS))
	    Dm__StampContainer(opr_info->src_win->cp);
#endif

	if ((opr_info->src_win->view_type != DM_ICONIC) &&
	    !IS_TREE_WIN(Desktop, opr_info->src_win))
	    DmSortItems(opr_info->src_win,
			opr_info->src_win->sort_type, 0, 0, 0);

	DmDisplayStatus((DmWinPtr)opr_info->src_win);
    }

    if ((use_dst_win) && !((options & DM_UPDATE_SRCWIN) &&
			   (opr_info->dst_win == opr_info->src_win)))
    {
#ifdef NOT_USE
	if (opr_info->dst_win->attrs & DM_B_FOLDER_WIN)
	    Dm__StampContainer(opr_info->dst_win->cp);
#endif

	if (opr_info->dst_win->view_type != DM_ICONIC)
	    DmSortItems(opr_info->dst_win,
			opr_info->dst_win->sort_type, 0, 0, wrap_width);

	DmDisplayStatus((DmWinPtr)opr_info->dst_win);
    }
}				/* end of DmUpdateWindow() */
