/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_cbs.c	1.140"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-related callbacks.
*/
						/* #includes go here	*/
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/OlCursors.h>

#include <Gizmos.h>
#include <BaseWGizmo.h>
#include <ChoiceGizm.h>
#include <LabelGizmo.h>
#include <MenuGizmo.h>
#include <FileGizmo.h>
#include <ModalGizmo.h>
#include <PopupGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "SWinGizmo.h"
#include "StatGizmo.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void	CancelCB(Widget, XtPointer, XtPointer);
static void	CopyCB(Widget, XtPointer, XtPointer);
static FileGizmo * CreateFilePrompt(DmFolderWindow folder, FileGizmo * gizmo);
static char *	GetTarget(DmFolderWindow win, Widget text_field, char * cwd);
static void	LinkCB(Widget, XtPointer, XtPointer);
static void	MoveCB(Widget, XtPointer, XtPointer);
static void	OpenOtherCB(Widget, XtPointer, XtPointer);
static void	RenameCB(Widget, XtPointer, XtPointer);
static int	SetupFileOp(DmFolderWindow, DmFileOpType, char * target);
static void	Undo(DmFolderWinPtr window);

					/* public procedures		*/
void		DmEditUndoCB(Widget, XtPointer, XtPointer);
void		DmFileCopyCB(Widget, XtPointer, XtPointer);
void		DmFileDeleteCB(Widget, XtPointer, XtPointer);
void		DmFileLinkCB(Widget, XtPointer, XtPointer);
void		DmFileMoveCB(Widget, XtPointer, XtPointer);
void		DmFileNewCB(Widget, XtPointer, XtPointer);
void		DmFileOpenCB(Widget, XtPointer, XtPointer);
void		DmFilePrintCB(Widget, XtPointer, XtPointer);
void		DmFilePropCB(Widget, XtPointer, XtPointer);
void		DmFileRenameCB(Widget, XtPointer, XtPointer);
void		DmFolderOpenDirCB(Widget, XtPointer, XtPointer);
void		DmFolderOpenOtherCB(Widget, XtPointer, XtPointer);
void		DmFolderOpenParentCB(Widget, XtPointer, XtPointer);
void		DmStopCB(Widget, XtPointer, XtPointer);
int		DmValidatePrompt(Widget, char *, char **, struct stat *);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* Menu Items for "Copy", "Rename" "Move", "Link" & "Open" prompts */

static MenuItems  FileCopyMenuItems[] = {
  { True, TXT_COPY,	TXT_M_COPY,	NULL, CopyCB, },
  { True, TXT_CANCEL,	TXT_M_CANCEL,	NULL, CancelCB, },
  { True, TXT_P_HELP,	TXT_M_HELP,	NULL, CopyCB, },
  { NULL }
};

static MenuItems  FileRenameMenuItems[] = {
  { True, TXT_RENAME,	TXT_M_RENAME,	NULL, RenameCB, },
  { True, TXT_CANCEL,	TXT_M_CANCEL,	NULL, CancelCB, },
  { True, TXT_P_HELP,	TXT_M_HELP,	NULL, RenameCB, },
  { NULL }
};

static MenuItems  FileMoveMenuItems[] = {
  { True, TXT_MOVE,	TXT_M_MOVE,	NULL, MoveCB, },
  { True, TXT_CANCEL,	TXT_M_CANCEL,	NULL, CancelCB, },
  { True, TXT_P_HELP,	TXT_M_HELP,	NULL, MoveCB, },
  { 0 }
};

static MenuItems  FileLinkMenuItems[] = {
  { True, TXT_LINK,	TXT_M_LINK,	NULL, LinkCB, },
  { True, TXT_CANCEL,	TXT_M_CANCEL,	NULL, CancelCB, },
  { True, TXT_P_HELP,	TXT_M_HELP,	NULL, LinkCB, },
  { 0 }
};

static MenuItems  FileOpenOtherMenuItems[] = {
  { True, TXT_OPEN,	TXT_M_OPEN,	NULL, OpenOtherCB, },
  { True, TXT_CANCEL,	TXT_M_CANCEL,	NULL, CancelCB, },
  { True, TXT_P_HELP,	TXT_M_HELP,	NULL, OpenOtherCB, },
  { 0 }
};

/* Menus for "Copy", "Move", "Link" & "Open" */
MENU_BAR("CopyMenuBar",		FileCopyMenu,		NULL, 0 );
MENU_BAR("RenameMenuBar",	FileRenameMenu,		NULL, 0 );
MENU_BAR("MoveMenuBar",		FileMoveMenu,		NULL, 0 );
MENU_BAR("LinkMenuBar",		FileLinkMenu,		NULL, 0 );
MENU_BAR("OpenOtherMenuBar",	FileOpenOtherMenu,	NULL, 0 );

/* Flat Exclusive definition for the Link Type: "Hard" "Symbolic" */
static MenuItems LinkTypeItems[] = {
  { True, TXT_LINK_SOFT, TXT_M_LINK_SOFT, NULL, NULL, (char *)DM_SYMLINK },
  { True, TXT_LINK_HARD, TXT_M_LINK_HARD, NULL, NULL, (char *)DM_HARDLINK },
  { NULL }
};

static MenuGizmo LinkTypeMenu =
  { NULL, "TypeMenu", NULL, LinkTypeItems, NULL, NULL, EXC, OL_FIXEDROWS };

/* Define LinkType ChoiceGizmo ie. a caption with an Exclusive attached. */
static ChoiceGizmo LinkTypeChoice =
  { NULL, "TypeChoice", TXT_LINK_LABEL, &LinkTypeMenu };
static GizmoRec LinkTypeGiz[] = {
        {ChoiceGizmoClass,       &LinkTypeChoice},
};

static LabelGizmo RenameCaption =
  { NULL, "renameCaption", TXT_RENAME_CAPTION, NULL, 0, OL_FIXEDCOLS, 1, };

static GizmoRec RenameGizmos[] = {
  { LabelGizmoClass, &RenameCaption, },
};

/* Prompts for "Copy", "Rename", "Move", "Link" & "Open" */

static HelpInfo CopyWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "470", NULL };

static FileGizmo FileCopyPrompt = {
	&CopyWinHelp, "filecopy", TXT_COPY_TITLE, &FileCopyMenu, "", "", ".",
	FOLDERS_ONLY
};

static HelpInfo RenameWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "510", NULL };

static PopupGizmo FileRenamePrompt = {
	&RenameWinHelp, "filerename", TXT_RENAME_TITLE, &FileRenameMenu,
	RenameGizmos, XtNumber(RenameGizmos),
};

static HelpInfo MoveWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "550", NULL };

static FileGizmo FileMovePrompt = {
	&MoveWinHelp, "filemove", TXT_MOVE_TITLE, &FileMoveMenu, "", "", ".",
	FOLDERS_ONLY
};

static HelpInfo LinkWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "590", NULL };

static FileGizmo FileLinkPrompt = {
	&LinkWinHelp, "filelink", TXT_LINK_TITLE, &FileLinkMenu, "", "", ".",
	FOLDERS_ONLY, LinkTypeGiz, XtNumber(LinkTypeGiz)
};

static HelpInfo OpenWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "860", NULL };

static FileGizmo FileOpenPrompt = {
	&OpenWinHelp, "fileopen", TXT_OPEN_TITLE, &FileOpenOtherMenu, "", "",
	".", FOLDERS_AND_FOLDERS
};

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    CancelCB- common callback to popdown prompt
*/
static void
CancelCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown( (Widget)_OlGetShellOfWidget(widget) );
}

/****************************procedure*header*****************************
    CopyCB- callback for Copy operation.
*/
static void
CopyCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	p = (OlFlatCallData *)call_data;
    DmFolderWindow	folder;
    DmHelpAppPtr	help_app;
    char *		target;

    switch(p-> item_index)
    {
    case FilePromptTask:
	folder = (DmFolderWindow)client_data;
	target = GetTarget(folder, folder->copy_prompt->textFieldWidget,
			   folder->copy_prompt->directory);
	if ((target != NULL) && (SetupFileOp(folder, DM_COPY, target) == 0))
	    BringDownPopup( (Widget)_OlGetShellOfWidget(widget) );
	break;

    case FilePromptHelp:
	help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));
	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
		NULL, "DesktopMgr/folder.hlp", "470", UNSPECIFIED_POS,
		UNSPECIFIED_POS);
	break;
    }
}				/* End of CopyCB */

/****************************procedure*header*****************************
    CreateFilePrompt-
*/
static FileGizmo *
CreateFilePrompt(DmFolderWindow folder, FileGizmo * gizmo)
{
    FileGizmo * prompt = CopyGizmo(FileGizmoClass, gizmo);

    prompt->menu->client_data =(char *)folder;
    prompt->directory =
	strdup(IS_FOUND_WIN(Desktop, folder) ? DESKTOP_DIR(Desktop) :
	       IS_TREE_WIN(Desktop, folder) ?
	       DmObjPath(ITEM_OBJ(folder->itp)) : folder->cp->path );

    (void)CreateGizmo(folder->shell, FileGizmoClass, prompt, NULL, 0);

    return(prompt);
}					/* end of CreateFilePrompt */

/****************************procedure*header*****************************
    GetTarget- get target from "prompt", validate it and display error msg if
	error.
*/
static char *
GetTarget(DmFolderWindow folder, Widget text_field, char * cwd)
{
    char *	target = NULL;
    int		status;

    status = DmValidatePrompt(text_field, cwd, &target, NULL);

    if ((status != 0) && (status != ENOENT))
    {
	DmVaDisplayStatus((DmWinPtr)folder, True,
			  TXT_INVALID_FILE_OR_FOLDER, target);
	if (target != NULL)
	{
	    FREE(target);
	    target = NULL;
	}
    }
    return(target);
}					/* end of GetTarget */

/****************************procedure*header*****************************
    LinkCB- callback for Link operation.
*/
static void
LinkCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	p = (OlFlatCallData *)call_data;
    DmFolderWindow	folder;
    DmHelpAppPtr	help_app;
    char *		target;
    Boolean		set;
    DmFileOpType	link_type;
    XtPointer		link_data;
    ChoiceGizmo *	choice;

   switch(p-> item_index)
   {
   case FilePromptTask:
       folder = (DmFolderWindow)client_data;
       choice = (ChoiceGizmo *)QueryGizmo(FileGizmoClass,
					  (FileGizmo *)folder->link_prompt,
					  GetGizmoGizmo, "TypeChoice");
       set = False;
       link_data = NULL;
       XtSetArg(Dm__arg[0], XtNset, &set);
       XtSetArg(Dm__arg[1], XtNclientData, &link_data);
       OlFlatGetValues(choice->buttonsWidget, 0, Dm__arg, 2);

       link_type = set ? (DmFileOpType)link_data :
	   ((DmFileOpType)link_data == DM_HARDLINK) ? DM_SYMLINK : DM_HARDLINK;

       target =  GetTarget(folder, folder->link_prompt->textFieldWidget,
			   folder->link_prompt->directory);
	if ((target != NULL) && (SetupFileOp(folder, link_type, target) == 0))
	    BringDownPopup( (Widget)_OlGetShellOfWidget(widget) );
       break;

   case FilePromptHelp:
       help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));
       DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id, NULL,
		"DesktopMgr/folder.hlp", "590", UNSPECIFIED_POS, UNSPECIFIED_POS);
       break;

   }
}				/* end of LinkCB */

/****************************procedure*header*****************************
    MoveCB- callback for Move operation.
*/
static void
MoveCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	p = (OlFlatCallData *)call_data;
    DmFolderWindow	folder;
    DmHelpAppPtr	help_app;
    char *		target;

    switch(p->item_index)
    {
    case FilePromptTask:
        folder = (DmFolderWindow)client_data;
	target = GetTarget(folder, folder->move_prompt->textFieldWidget,
			   folder->move_prompt->directory);
	if ((target != NULL) && (SetupFileOp(folder, DM_MOVE, target) == 0))
	    BringDownPopup( (Widget)_OlGetShellOfWidget(w) );
	break;

    case FilePromptHelp:
	help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));
	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id, NULL,
		"DesktopMgr/folder.hlp", "550", UNSPECIFIED_POS, UNSPECIFIED_POS);
	break;
    }
}				/* end of MoveCB */

/****************************procedure*header*****************************
    OpenOtherCB- this is called from the Open Folder popup which occurs
	when the user asks to open an "other" folder.  The user has typed in
	a folder name.

	It can be static since it is referenced only by the Prompt struct
	for the popup in this file.
*/
static void
OpenOtherCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	p = (OlFlatCallData *)call_data;
    DmFolderWindow	folder;
    char * 		foldername;
    FileGizmo *		prompt;
    DmHelpAppPtr	help_app;
    int			status;
    struct stat		stat_buf;

    switch(p-> item_index)
    {
    case FilePromptTask:
	folder = (DmFolderWindow)client_data;
	prompt = folder->folder_prompt;
	status = DmValidatePrompt(prompt->textFieldWidget, prompt->directory,
				  &foldername, &stat_buf);

	/* Even when no error, check for 'dir' type */
	if ((status == 0) && S_ISDIR(stat_buf.st_mode))
	    if (IS_WB_PATH(Desktop, foldername))
	    {
		/* From wbReqProc.c:DisplayWBProc
		   
		   This code assume that the wastebasket window is always
		   created up front.
		*/
		DmMapWindow((DmWinPtr)DESKTOP_WB_WIN(Desktop));
		DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));

	    } else
	    {
		/* This could take awhile so entertain the user */
		BUSY_CURSOR(folder->box);

		if (DmOpenFolderWindow(foldername, NULL, NULL, False) != NULL)
		    BringDownPopup( (Widget)_OlGetShellOfWidget(w) );
		else
		    DmVaDisplayStatus((DmWinPtr)folder, True,
				      TXT_OpenErr, foldername); 
	    }
	else
	    DmVaDisplayStatus((DmWinPtr)folder, True, TXT_NOT_DIR, foldername);

	if (foldername != NULL)
	    FREE(foldername);
	break;

    case FilePromptHelp:
	help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));
	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id, NULL,
		"DesktopMgr/folder.hlp", "860", UNSPECIFIED_POS, UNSPECIFIED_POS);
	break;
    }
}				/* End of OpenOtherCB */

/****************************procedure*header*****************************
    RenameCB- callback for Rename operation.
*/
static void
RenameCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	p = (OlFlatCallData *)call_data;
    DmFolderWindow	folder;
    DmHelpAppPtr	help_app;
    char *		target;
    PopupGizmo *	prompt;
    Widget		text_field;
    char *		cwd;

    switch(p->item_index)
    {
    case FilePromptTask:
        folder = (DmFolderWindow)client_data;
	prompt = folder->rename_prompt;
	text_field = NULL;
	XtSetArg(Dm__arg[0], XtNuserData, &text_field);
	XtGetValues(prompt->shell, Dm__arg, 1);

	/* Must generate correct dir-name: container path for folders,
	   obj-path for other windows (must get src obj).
	*/
	if (folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
	{
	    DmItemPtr	item;

	    for (item = folder->itp;
		 item < folder->itp + folder->nitems; item++)
		if (ITEM_MANAGED(item) && ITEM_SELECT(item) && !ITEM_BUSY(item))
		    break;

	    /* *Must* find an item since button would be insens otherwise */
	    cwd = strdup( DmObjDir(ITEM_OBJ(item)) );

	} else
	    cwd = folder->cp->path;

	target = GetTarget(folder, text_field, cwd);
	if (target != NULL)
	{
	    char * name = target + strlen(cwd);

	    if (folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
		FREE(cwd);

	    /* Check for NULL input and don't allow paths (no slash) */
	    if ((name[0] == '\0') || (strchr(name + 1, '/') != NULL))
	    {
		DmVaDisplayStatus((DmWinPtr)folder, True, (name[0] == '\0') ?
				  TXT_NO_PATH_ALLOWED : TXT_NO_PATH_ALLOWED);
		FREE(target);
		return;
	    }
	    /* Will free target as necessary: */
	    if (SetupFileOp(folder, DM_RENAME, target) == 0)
		BringDownPopup( (Widget)_OlGetShellOfWidget(w) );
	}
	break;

    case FilePromptHelp:
	help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));
	/* NOTE: change section tag when help on Rename is available */
	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id, NULL,
		"DesktopMgr/folder.hlp", "510", UNSPECIFIED_POS, UNSPECIFIED_POS);
	break;
    }
}				/* end of RenameCB */

/****************************procedure*header*****************************
    SetupFileOp- this function performs common functionality of callback
	routines (CopyCB, MoveCB, etc) to set up parameters appropriately and
	call DmDoFileOp().
*/
static int
SetupFileOp(DmFolderWindow folder, DmFileOpType type, char * target)
{
    char **		src_list;
    void **		item_list;
    int			src_cnt;
    DmFileOpInfoPtr	opr_info;

    /* get list of selected items to be operated on */
    item_list = DmGetItemList((DmWinPtr)folder, OL_NO_ITEM);

    /* check if deleting "system" files */
    if ((type == DM_DELETE) ||
	(target && !strcmp(target, DM_WB_PATH(Desktop)))) {
	char *file;

	if (file = DmHasSystemFiles(item_list)) {
		DmVaDisplayStatus((DmWinPtr)folder, True,
				  TXT_DEL_SYSTEM_FILE, file);
    		FREE((void *)item_list);
		XtFree(target);
		return(1);
	}
    }

    src_list = DmItemListToSrcList(item_list, &src_cnt);
    FREE((void *)item_list);

    if (src_cnt == 0)
    {
	DmVaDisplayStatus((DmWinPtr)folder, True, TXT_NONE_SELECTED);
	XtFree(target);
	return(1);
    }

    /* load parameters into opr_info struct */
    opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
    opr_info->type		= type;
    opr_info->target_path	= target;
    opr_info->src_path		= strdup(folder->cp->path);
    opr_info->src_list		= src_list;
    opr_info->src_cnt		= src_cnt;
    opr_info->src_win		= folder;
    opr_info->dst_win		=
	((type == DM_RENAME) &&
	 !(folder->attrs & (DM_B_FOUND_WIN | DM_B_TREE_WIN))) ? folder : NULL;
    opr_info->x			= opr_info->y = UNSPECIFIED_POS;

    /* Determine options.  For Tree view and Found window, srcs can
       have multi-paths.
    */
    opr_info->options = REPORT_PROGRESS | OVERWRITE | OPRBEGIN;
    if (folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
	opr_info->options |= MULTI_PATH_SRCS;

    /* free previous task info structure so that now we are about to do 
       new operation. The reason to keep info. until next opr. is started,
       is; to facilitate undo. We need the info. in TaskInfo structure
       for being able to do undo
    */
    DmFreeTaskInfo(folder->task_id);
    folder->task_id = DmDoFileOp(opr_info, DmFolderFMProc, NULL);

    return(folder->task_id == NULL);
}					/* End of SetupFileOp */

/****************************procedure*header*****************************
    Undo - implements undo of last operation. The undo functionality is
	also implemented with asynchronous processing mechanism.  If the
	previous operation was a DELETE operation then the wastebasket
	function DmMoveFile FromWB()  is called which does in turn calls
	DmDoFileOp() to 'undo' asynchrounously.
*/
static void
Undo(DmFolderWinPtr window)
{
    DmTaskInfoListPtr	tip;
    DmFileOpInfoPtr	opr_info;
    char *		tmp_path;
    DmFolderWindow	tmp_win;

    /* See if anything to undo */
    if ( (tip = window->task_id) == NULL)
	return;

    opr_info = tip->opr_info;

    if ((opr_info->cur_src == 0) || (opr_info->task_cnt == 0))
	goto done;

    /* WB will look at options so adjust them now */
    opr_info->options |= UNDO;
    opr_info->options &= ~OVERWRITE;	/* ignore overwrites */

    /* Undo'ing from WB */
    if (IS_WB_PATH(Desktop, opr_info->target_path))
    {
	DmMoveFileFromWB(NULL, opr_info, False);
	goto done;
    }

    if (opr_info->x != UNSPECIFIED_POS)
    {
	opr_info->x = UNSPECIFIED_POS;		/* UNDO can't specify pos. */
	opr_info->y = UNSPECIFIED_POS;
    }
    
    /* Establish new 'src_path' and 'target_path' */
    if (opr_info->attrs & (DM_B_DIR_EXISTS | DM_B_DIR_NEEDED_4FILES))
    {
	tmp_path = opr_info->src_path;
	opr_info->src_path = opr_info->target_path;

	if (opr_info->attrs & DM_B_DIR_NEEDED_4FILES)
	{
	    opr_info->options |= RMNEWDIR;

	    /* Attempt to establish new dst_win when the original target
	       was created by the operation.
	    */
	    opr_info->dst_win = DmQueryFolderWindow(opr_info->target_path);
	}
	else
	    /* The dst win may not exist anymore */
	    opr_info->dst_win = NULL;

	/* since 'target_path' will be existing dir: */
	opr_info->attrs = DM_B_DIR_EXISTS | DM_B_TARGET_ADJUSTED;

    } else
    {
	tmp_path =strdup(DmMakePath(opr_info->src_path,opr_info->src_list[0]));

	FREE(opr_info->src_list[0]);
	FREE(opr_info->src_path);
	opr_info->src_list[0] = strdup(basename(opr_info->target_path));
	opr_info->src_path = strdup(dirname(opr_info->target_path));
	FREE(opr_info->target_path);

	/* since 'target_path' will NOT be existing dir */
	opr_info->attrs = 0;
    }
    opr_info->target_path = tmp_path;

    /* Switch src & dst win's. */
    tmp_win = opr_info->src_win;
    opr_info->src_win = opr_info->dst_win;
    opr_info->dst_win = tmp_win;

    (void)DmUndoFileOp(tip);
    return;			/* can't free task info until done */

 done:
    DmFreeTaskInfo(window->task_id);
    window->task_id = NULL;
}					/* end of Undo */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmEditUndoCB-
*/
void
DmEditUndoCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Undo((DmFolderWindow)DmGetWinPtr(w));

}				/* End of DmEditUndoCB */

/****************************procedure*header*****************************
    DmFileCopyCB-
*/
void
DmFileCopyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);

    if (folder->copy_prompt == NULL)
	folder->copy_prompt = CreateFilePrompt(folder, &FileCopyPrompt);
    MapGizmo(FileGizmoClass, folder->copy_prompt);
}

/****************************procedure*header*****************************
    DmFileDeleteCB-
*/
void
DmFileDeleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmFolderWindow folder = (DmFolderWindow)DmGetWinPtr(w);
	
	/* If destination is Wastebasket and Immediate Delete mode is set,
	 * make this a DM_DELETE operation.
	 */
	if (DM_WBIsImmediate(Desktop))
		(void)SetupFileOp(folder, DM_DELETE, NULL);
	else
		(void)SetupFileOp(folder, DM_MOVE, strdup(DM_WB_PATH(Desktop)));

}					/* end of DmFileDeleteCB */

/****************************procedure*header*****************************
    DmFileLinkCB-
*/
void
DmFileLinkCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);

    if (folder->link_prompt == NULL)
	folder->link_prompt = CreateFilePrompt(folder, &FileLinkPrompt);
    MapGizmo(FileGizmoClass, folder->link_prompt);
}

/****************************procedure*header*****************************
    DmFileMoveCB-
*/
void
DmFileMoveCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);

    if (folder->move_prompt == NULL)
	folder->move_prompt = CreateFilePrompt(folder, &FileMovePrompt);
    MapGizmo(FileGizmoClass, folder->move_prompt);
}

/****************************procedure*header*****************************
    DmFileNewCB-
*/
void
DmFileNewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);

    DmPromptCreateFile(folder);
}

/****************************procedure*header*****************************
    DmFileOpenCB-
*/
void
DmFileOpenCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    register DmItemPtr	item;
    register int	i;
    register Boolean	busy_cursor = False;

    for (i = folder->nitems, item = folder->itp; i > 0; i--, item++)
	if (ITEM_MANAGED(item) && ITEM_SELECT(item) && !ITEM_BUSY(item))
	{
	    if (!busy_cursor)
	    {
		/* Opens can take awhile so entertain user */
		BUSY_CURSOR(folder->box);
		busy_cursor = True;
	    }
	    DmOpenObject((DmWinPtr)folder, ITEM_OBJ(item));
	}
}					/* end of DmFileOpenCB */

/****************************procedure*header*****************************
    DmFilePrintCB-
*/
void
DmFilePrintCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmPrintCB(w, DmGetWinPtr(w), call_data);
}

/****************************procedure*header*****************************
    DmFilePropCB-
*/
void
DmFilePropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Dm__PopupFilePropSheets( (DmFolderWindow)DmGetWinPtr(w) );
}

/****************************procedure*header*****************************
    DmFileRenameCB-
*/
void
DmFileRenameCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(widget);
    PopupGizmo *	prompt;

    if ( (prompt = folder->rename_prompt) == NULL )
    {
	Widget	w;
	LabelGizmo * l;

	prompt = folder->rename_prompt =
	    CopyGizmo(PopupGizmoClass, &FileRenamePrompt);
	((MenuGizmo *)(prompt->menu))->client_data =(char *)folder;

	CreateGizmo(folder->shell, PopupGizmoClass, prompt, NULL, 0);

	l = (LabelGizmo *)(prompt->gizmos[0].gizmo);

	/* Set the font on the caption and create the text field */
	XtSetArg(Dm__arg[0], XtNfont,
		 _OlGetDefaultFont(prompt->shell, OlDefaultBoldFont));
	XtSetValues(l->captionWidget, Dm__arg, 1);
	w = XtCreateManagedWidget("renameInput", textFieldWidgetClass,
				  l->controlWidget, NULL, 0);

	/* Stash the textField widget on the shell for easier access */
	XtSetArg(Dm__arg[0], XtNuserData, w);
	XtSetValues(prompt->shell, Dm__arg, 1);
    }
    MapGizmo(PopupGizmoClass, prompt);
}

/****************************procedure*header*****************************
    DmFolderOpenDirCB- callback when user presses button on item in
	Folder menu.
*/
void
DmFolderOpenDirCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	d = (OlFlatCallData *)call_data;
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    MenuItems *		button = (MenuItems *)(d->items) + d->item_index;
    
    BUSY_CURSOR(folder->box);
    if (DmOpenFolderWindow((char *)button->client_data, 0, 0, False) == NULL)
	DmVaDisplayStatus((DmWinPtr)folder,
			  True, TXT_OpenErr, button->client_data);

}				/* End of DmFolderOpenDirCB */

/****************************procedure*header*****************************
    DmFolderOpenOtherCB-
*/
void
DmFolderOpenOtherCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);

    if (folder->folder_prompt== NULL)
    {
	folder->folder_prompt = CreateFilePrompt(folder, &FileOpenPrompt);
	SetFileGizmoMessage(folder->folder_prompt, TXT_ENTER_PATH_MSG);
    }
    MapGizmo(FileGizmoClass, folder->folder_prompt);

}				/* End of DmFolderOpenOtherCB */

/****************************procedure*header*****************************
    DmFolderOpenParentCB-
*/
void
DmFolderOpenParentCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    char *		path;

    path = strdup( DM_WIN_PATH(folder) );
    (void)DmOpenFolderWindow(dirname(path), 0, NULL, False);
    FREE(path);
}

/****************************procedure*header*****************************
    DmStopCB- callback called when user presses STOP key during file operation.
	Popdown any overwrite notice, close any open file descriptors, and
	removes all un-executed tasks from the list, effectively marking the
	end of the operation. Note, Undo operation will undo only what was
	done before the STOP key was entered. There may be some
	synchronization problem here, if an undo is invoked right after STOP.
*/
void
DmStopCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlVirtualEvent	event = (OlVirtualEvent)call_data;
    DmFolderWindow	window;
    DmTaskInfoListPtr	tip;

    /* only interested in OL_STOP virtual event */
    if (event->virtual_name != OL_STOP)
    {
	event->consumed = False;
	return;
    }

    event->consumed = True;

    window	= (DmFolderWinPtr)client_data;
    tip		= window->task_id;

    tip->opr_info->attrs |= DM_B_FILE_OP_STOPPED;

    /* Popdown overwrite notice if one is up */
    if (window->overwriteNotice != NULL)
	XtPopdown(window->overwriteNotice->shell);

    DmStopFileOp(tip);		/* clean up tasks after "stopping" */
    tip->opr_info->src_info[(tip->opr_info->cur_src == 0) ? 0 :
			tip->opr_info->cur_src - 1] |= SRC_B_ERROR;


    (*tip->client_proc)(DM_DONE, tip->client_data,
				 (XtPointer)tip, NULL, NULL);
}					/* end of DmStopCB */

/****************************procedure*header*****************************
    DmValidatePrompt-  This function validates the user input in prompt
	boxes popped up for the user to perform file manipulation via menu
	interface in folder window.
	Extract input from 'widget' using XtNstring resources (ie, widget is
	textFieldWidget, etc)
*/
int
DmValidatePrompt(Widget text_field, char * cwd,
		 char ** ret_input, struct stat * stat_buf)
{
    char *		filename;
    int			status;
    struct stat		my_stat_buf;

    /* Get user input */
    filename = NULL;
    XtSetArg(Dm__arg[0], XtNstring, &filename);
    XtGetValues(text_field, Dm__arg, 1);

    /* Deal with empty string input */
    if ((filename == NULL) || (filename[0] == '\0'))
    {
	if (filename != NULL)
	    FREE(filename);

	filename = strdup(cwd);

    } else if (filename[0] != '/')
    {
	char * save = filename;

	filename = strdup( DmMakePath(cwd, filename) );
	FREE(save);
    }

    /* 'filename' is now full path so "stat" the file.  Note that a
       non-existent file may be valid for some prompts but not for others so
       must return this condition.  Also, note that file names or paths that
       are too long can be detected by caller as ENAMETOOLONG.
    */
    if (stat_buf == NULL)
	stat_buf = &my_stat_buf;
    errno = 0;
    status = (stat(filename, stat_buf) == 0) ? 0 : errno;

    if (ret_input != NULL)
	*ret_input = filename;

    else if (filename != NULL)
	FREE(filename);

    return(status);
}				/* end of DmValidatePrompt */
