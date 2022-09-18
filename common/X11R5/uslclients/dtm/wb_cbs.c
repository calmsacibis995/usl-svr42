/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:wb_cbs.c	1.152"

/******************************file*header********************************

    Description:
     This file contains the source code for callbacks for buttons
	in the Wastebasket window.
*/
                              /* #includes go here     */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>

#include <Xol/OpenLook.h>
#include <Xol/Dynamic.h>
#include <Xol/Notice.h>
#include <Xol/Caption.h>
#include <Xol/Modal.h>
#include <Xol/Form.h>
#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/PopupWindo.h>
#include <Xol/ScrolledWi.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"
#include "error.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static void	CreateFilePropSheet(DmItemPtr);
static void	FilePropBtnCB(Widget, XtPointer, XtPointer);
static void	FreeWBProps(DmItemPtr);
static char 	*GetFilePerms(mode_t mode, int type);
static void	VerifyPopdnCB(Widget, XtPointer, XtPointer);
static void	PopdownCB(Widget, XtPointer, XtPointer);
static int	PutBack(DmItemPtr item);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* used in file instance property sheet */
#define OWNER	1
#define GROUP	2
#define OTHER	3

#define NameToItem(name) DmObjNameToItem((DmWinPtr) \
					 DESKTOP_WB_WIN(Desktop), name)

/* global variables */
static Boolean	popdown = False;

static OlDtHelpInfo help_info;

/* Define gizmo for empty wastebasket notice */
static MenuItems menubarItems[] = {
    MENU_ITEM ( TXT_YES,	TXT_M_YES,	NULL ),
    MENU_ITEM ( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
    MENU_ITEM ( TXT_P_HELP,	TXT_M_HELP,	NULL ),
    { NULL }					/* NULL terminated */
};

MENU_BAR("emptyNoticeMenubar", menubar, DmEmptyWB, 1);	/* default: Cancel */

static HelpInfo EmptyNoticeHelp =
{ TXT_WB_TITLE, NULL, "DesktopMgr/wb.hlp", "70", NULL };

static ModalGizmo emptyGizmo = {
	&EmptyNoticeHelp,  /* help info */
	"emptyNotice",     /* shell name */
	TXT_WB_TITLE,      /* title */
	&menubar,          /* menu */
	TXT_EMPTY_WB,      /* message */
	NULL, 0,           /* gizmos, num_gizmos */
};

static String	btn_fields[] = {
	XtNlabel, XtNmnemonic, XtNselectProc, XtNdefault, XtNclientData,
	XtNsensitive
};

/***************************private*procedures****************************

    Private Procedures
*/


/****************************procedure*header*****************************
 * Calls DmDoFileOp() to undelete a file in the wastebasket.
 */
static int
PutBack(DmItemPtr item)
{
	DmWBCPDataPtr   cpdp;
	DmFileOpInfoPtr opr_info;
	char *real_path;
	char *path;

	if (ITEM_OBJ(item)->objectdata == NULL) {
		/* initialize file's op->objectdata to its stat info */
		path = DmMakePath(DM_WB_PATH(Desktop), ITEM_OBJ_NAME(item));
		ITEM_OBJ(item)->objectdata = (void *)WBGetFileInfo(path);
	}

	cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
	cpdp->op_type = DM_PUTBACK;
	cpdp->itp     = item;

	real_path = DmGetObjProperty(ITEM_OBJ(item), REAL_PATH, NULL);
	cpdp->target = strdup(real_path);

	opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
	opr_info->type		     = DM_RENAME;	/* re-maps to DM_MOVE */
	opr_info->options	     = DONT_OVERWRITE;

	/* Need to remove the one '/' from real_path if target is root
	 * directory.
	 */
	if (real_path[0] == '/' && real_path[1] == '/')
		opr_info->target_path = strdup(real_path + 1);
	else
		opr_info->target_path = strdup(real_path);

	opr_info->src_path	     = strdup(DM_WB_PATH(Desktop));
	opr_info->src_list	     = (char **)MALLOC(sizeof(char *));
	opr_info->src_cnt	     = 1;
	opr_info->dst_win	     = NULL;
	opr_info->src_win	     = DESKTOP_WB_WIN(Desktop);
	opr_info->x		     = opr_info->y = UNSPECIFIED_POS;
	opr_info->src_list[0]	= strdup(ITEM_OBJ_NAME(item));

	return(DmDoFileOp(opr_info, DmWBClientProc, (XtPointer)cpdp) == NULL);

}					/* end of PutBack */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Returns all selected files in wastebasket to where they were
 * deleted from.  Files are put back one at a time because they can
 * be targeted for different folders - can't specify more than one
 * target in call to DmDoFileOp(). 
 *
 * (NOTE: In all callbacks in Edit menu, can't reply on client_data
 * being NULL; client_data is used to set/unset sensitivity.)
 *
 */
void
DmWBEMPutBackCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmItemPtr	itp;

	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), False, NULL);

	for (itp = DESKTOP_WB_WIN(Desktop)->itp;
	     itp < DESKTOP_WB_WIN(Desktop)->itp +
	     DESKTOP_WB_WIN(Desktop)->nitems; itp++)
		if (ITEM_MANAGED(itp) && ITEM_SELECT(itp) && !ITEM_BUSY(itp))
			if (PutBack(itp) != 0)
				break;
}					/* end of DmWBEMPutBackCB */

/****************************procedure*header*****************************
 * This routine is called when Delete is selected from the icon menu.
 * It moves an item in the wastebasket back to where it was deleted from.
 * client_data contains pointer to item to be put back.
 */
void
DmWBIMPutBackCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), False, NULL);

	(void)PutBack((DmItemPtr)client_data);
}

/****************************procedure*header*****************************
 * Requests user confirmation to empty the wastebasket.
 */
void
DmConfirmEmptyCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	if (WB_IS_EMPTY(Desktop))
		/* this should never happen */
		return;

	DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));

	/* Create the modal gizmo */
	if (emptyGizmo.shell == NULL)
		CreateGizmo(XtParent(DESKTOP_WB_WIN(Desktop)->box),
			ModalGizmoClass, &emptyGizmo, NULL, 0);

	MapGizmo(ModalGizmoClass, &emptyGizmo);

} /* end of DmConfirmEmptyCB */


/****************************procedure*header*****************************
 * Calls DmDoFileOp() to delete a file in the wastebasket.
 */
void
DmWBDelete(char ** src_list, int src_cnt, DmWBCPDataPtr cpdp)
{
    DmFileOpInfoPtr	opr_info =
	(DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));

    opr_info->type		= DM_DELETE;
    opr_info->options		= 0;
    opr_info->target_path	= NULL;
    opr_info->src_path		= strdup(DM_WB_PATH(Desktop));
    opr_info->src_list		= src_list;
    opr_info->src_cnt		= src_cnt;
    opr_info->src_win		= DESKTOP_WB_WIN(Desktop);
    opr_info->dst_win		= NULL;
    opr_info->x			= opr_info->y = UNSPECIFIED_POS;

    (void)DmDoFileOp(opr_info, DmWBClientProc, (XtPointer)cpdp);
}					/* end of DmWBDelete */

/****************************procedure*header*****************************
 * Permanently removes the contents of the wastebasket.
 */
void
DmEmptyWB(Widget w, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData *fcd = (OlFlatCallData *)call_data;
	DmItemPtr      itp;
	DmWBCPDataPtr  cpdp;
	char           **src_list;
	char           **src;

	if (fcd->item_index == 2) {
	    /* Help */
		DmHelpAppPtr help_app = DmGetHelpApp(WB_HELP_ID(Desktop));

		DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
			NULL, "DesktopMgr/wb.hlp", "70", UNSPECIFIED_POS,
			UNSPECIFIED_POS);
		XtAddGrab(help_app->hlp_win.shell, False, False);
		return;
	}

	XtPopdown(emptyGizmo.shell);

         /* Cancel or no items */
	if ((fcd->item_index == 1) || WB_IS_EMPTY(Desktop))
	    return;

	src_list = src = (char **)
	    MALLOC(DESKTOP_WB_WIN(Desktop)->nitems * sizeof(char *));

	for (itp = DESKTOP_WB_WIN(Desktop)->itp;
	     itp < DESKTOP_WB_WIN(Desktop)->itp +
	     DESKTOP_WB_WIN(Desktop)->nitems; itp++)
	{
		if (ITEM_MANAGED(itp) && !ITEM_BUSY(itp))
			*src++ = strdup(ITEM_OBJ_NAME(itp));
	}

	cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
	cpdp->op_type = DM_EMPTY;

	DmWBDelete(src_list, src - src_list, cpdp);

}					/* end of DmEmptyWB */

/****************************procedure*header*****************************
 * Submits a list of files in the wastebasket to be deleted.
 */
static void
Delete(Cardinal item_index)
{
     DmWBCPDataPtr cpdp;
     void          **item_list;
     char          **src_list;
     int           src_cnt;

	item_list = DmGetItemList((DmWinPtr)DESKTOP_WB_WIN(Desktop), item_index);
	src_list  = DmItemListToSrcList(item_list, &src_cnt);
	FREE((void *)item_list);

	if (src_cnt == 0)
	    return;

	cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
	cpdp->op_type = DM_WBDELETE;
	DmWBDelete(src_list, src_cnt, cpdp);

}					/* end of Delete */

/****************************procedure*header*****************************
 * Permanently removes selected files in wastebasket.
 */
void
DmWBEMDeleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), False, NULL);
	Delete(OL_NO_ITEM);
}

/****************************procedure*header*****************************
 * Permanently removes item from which the icon menu is obtained.
 */
void
DmWBIMDeleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmItemPtr	itp = (DmItemPtr)client_data;

	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), False, NULL);
	Delete(itp - DESKTOP_WB_WIN(Desktop)->itp);

}					/* end of DmWBIMDeleteCB */

/****************************procedure*header*****************************
 * Handles dropping files in the wastebasket outside of wastebasket window.
 */
void
DmWBDropProc(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatDropCallData *d = (OlFlatDropCallData *)call_data;
    void               **list;
    DmFolderWindow     dst_win;
    DmObjectPtr        dst_obj;
    Cardinal           dst_indx;
    DmFolderWindow     src_win;
    char               **src;
    char               **src_list;
    int                src_cnt;

    /* Can't drop on icon within WB */
    if (DmIsWB(d->dst_info->window))
    {
	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			  True, TXT_WB_INVALID_DROP);
	return;
    }

	/* can't create a link or make a copy from wastebasket */ 
	if ((d->ve->virtual_name != OL_SELECT) &&
		(d->ve->virtual_name != OL_DRAG)) {
		DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), True,
			((d->ve->virtual_name == OL_LINK) ||
			 (d->ve->virtual_name == OL_LINKKEY)) ?
			 TXT_WB_CANT_LINK : TXT_WB_CANT_COPY);
		return;
	}

    src_win = (DmFolderWindow)client_data;
    dst_win = ((TREE_WIN(Desktop) != NULL) &&
	      (d->dst_info->window == XtWindow(TREE_WIN(Desktop)->box))) ?
		  TREE_WIN(Desktop) : DmIsFolderWin(d->dst_info->window);

    /* Can't drop outside of dtm */
    if (dst_win == NULL)
    {
	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			  True, TXT_WB_DROP_ON_UNK_WIN);
	return;
    }

    /* Getting here means it's a drop within dtm but not within WB */
    DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));

    /* Get type of dst object (if any).  Only valid to drop on background
       or folder icon
    */
    dst_indx = OlFlatGetItemIndex(dst_win->box,d->dst_info->x, d->dst_info->y);

    if ((dst_indx != OL_NO_ITEM) &&
	!OBJ_IS_DIR((dst_obj = ITEM_OBJ(DM_WIN_ITEM(dst_win, dst_indx)))))
    {
	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			  True, TXT_WB_DROP_FAILED);
	return;
    }

    /* get the list of items to be operated on */
    list = DmGetItemList((DmWinPtr)src_win, d->item_data.item_index);
    src_list = DmItemListToSrcList(list, &src_cnt);
    FREE((void *)list);

    /* Since file op's are generated in a loop but they are executed async
       later, we cannot handle overwrites.
    */
    for (src = src_list; src < src_list + src_cnt; src++)
    {
	char *real_path;
	char *bname;
	char *target;
	DmWBCPDataPtr   cpdp;
	DmFileOpInfoPtr opr_info;
	DmItemPtr       src_item = NameToItem(*src);

	if (src_item == NULL)			/* shouldn't be */
	    continue;

	/* Initialize file's op->objectdata to its stat info if it's NULL */
	if (ITEM_OBJ(src_item)->objectdata == NULL) {
		char *path;

		path = DmMakePath(DM_WB_PATH(Desktop), *src);
		ITEM_OBJ(src_item)->objectdata = (void *)WBGetFileInfo(path);
	}
	
	real_path = DmGetObjProperty(ITEM_OBJ(src_item), REAL_PATH, NULL);
	bname = basename(real_path);

	/* If dropping on folder icon, target is that path (and can't use
	   drop coord's.  Otherwise it's the path of the container.
	   */
	if (dst_indx == OL_NO_ITEM) {
		/* dropping on background */
		target = DmMakePath(dst_win->cp->path, bname);
		if (src_cnt > 1) {
			d->dst_info->x = UNSPECIFIED_POS;
			d->dst_info->y = UNSPECIFIED_POS;
		}

	} else {
	    char buf[PATH_MAX];

	    target = Dm_MakePath(DmObjPath(dst_obj), bname, buf);
	    d->dst_info->x = UNSPECIFIED_POS;
	    d->dst_info->y = UNSPECIFIED_POS;
	}	    

	cpdp	= (DmWBCPDataPtr)CALLOC(1,sizeof(DmWBCPDataRec));
	cpdp->itp      = src_item;
	cpdp->op_type  = DM_DROP;
	cpdp->target   = strdup(target);

	opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
	opr_info->type        = DM_RENAME;	/* re-maps to DM_MOVE */
	opr_info->options     = DONT_OVERWRITE;
	opr_info->target_path = strdup(target);
	opr_info->src_path    = strdup(DM_WB_PATH(Desktop));
	opr_info->src_list    = (char **)MALLOC(sizeof(char *));
	opr_info->src_cnt     = 1;
	opr_info->src_win     = DESKTOP_WB_WIN(Desktop);
	opr_info->dst_win     = NULL;
	opr_info->x           = d->dst_info->x;
	opr_info->y           = d->dst_info->y;
	opr_info->src_list[0] = strdup(*src);

	DmDoFileOp(opr_info, DmWBClientProc, (XtPointer)cpdp);
    }
}					/* end of DmWBDropProc */

/****************************procedure*header*****************************
 * Called when timer expires.  Checks for files to be deleted based on
 * their time stamps and the current time, then restarts the timer.
 * If the wastebasket is empty, call DmWBRestartTimer() to "do the right
 * thing", although this function should never be called if the wastebasket
 * is empty.
 */
void
DmWBTimerProc(client_data, timer_id)
XtPointer    client_data;
XtIntervalId timer_id;
{
	Boolean reset = False;
	DmItemPtr     itp;
	DmWBCPDataPtr cpdp;

	time_t time_stamp;
	time_t current_time;
	time_t interval_unit;

	char **nsrc_list;
	char **src_list = NULL;

	int  i;
	int  src_cnt = 0;
	int  alloc = NUM_ALLOC;

	/* this should never be used */
	if (WB_IS_EMPTY(Desktop) && WB_BY_TIMER(wbdp)) {
		if (wbdp->suspend == False)
			wbdp->restart = True;
		DmWBRestartTimer();
		return;
	}

	src_list = (char **)MALLOC(NUM_ALLOC * sizeof(char *));

	time(&(current_time));
	/* get interval unit in seconds */
	interval_unit = (time_t)(wbdp->time_unit / 1000);

	for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->itp;
	     i < DESKTOP_WB_WIN(Desktop)->nitems; i++, itp++) {

		if (src_cnt == alloc) {
			nsrc_list = (char **)REALLOC((void *)src_list,
				(alloc + NUM_ALLOC) * sizeof(char *));
			alloc += NUM_ALLOC;
			src_list = nsrc_list;
		}

		if (ITEM_MANAGED(itp)) {
			time_stamp = atol(DmGetObjProperty(ITEM_OBJ(itp),
						TIME_STAMP, NULL));

			if (((current_time - time_stamp) / interval_unit)
				>= (time_t)(wbdp->interval)) {

				/* don't delete busy files and reset their time stamp
				 * to current time to avoid an infinite loop.
				 */
				if (ITEM_BUSY(itp)) {
					char buf[16];
					sprintf(buf, "%ld", current_time);
					DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP, buf,NULL);
					reset = True;
					continue;
				}

				src_list[src_cnt] = strdup(ITEM_OBJ_NAME(itp));
				++src_cnt;
			} 
		}
	}

	if (reset)
		DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->cp, wb_dtinfo, 0);

	if (src_cnt == 0) {
		DmWBRestartTimer();
		if (src_list)
			FREE((void *)src_list);
		return;
	}

	cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
	cpdp->op_type = DM_TIMER;

	DmWBDelete(src_list, src_cnt, cpdp);

} /* end of DmWBTimerProc */

/****************************procedure*header*****************************
 * Called when the Suspend/Resume Timer button is selected.
 * Suspends the timer if it's currently on; otherwise, turn
 * it on again.  Update wbSuspend resource in .Xdefaults
 * so that it is always up-to-date.  Note that this resource
 * is otherwise only updated when the Wastebasket properties
 * are changed.
 */
void
DmWBTimerCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	extern void MergeResource();
	char buf[32];
	
	if (!WB_BY_TIMER(wbdp) || WB_IS_EMPTY(Desktop))
		return;

	if (wbdp->suspend == True)
		DmWBResumeTimer();
	else {
		DmWBSuspendTimer();
		wbdp->restart = False;
	}
	 
	/* update wbSuspend resource in .Xdefaults file */
	sprintf(buf, "dtm.wbSuspend:%d\n", wbdp->suspend);
	MergeResources(buf);

} /* end of DmWBTimerCB */


#include "error.h"	/* For overwrite error below */

/****************************procedure*header*****************************
 * Client proc called after file manipulation is completed.
 */
void
DmWBClientProc(DmProcReason reason, XtPointer client_data, XtPointer call_data,
char *src_file, char *dst_file)
{
	DmTaskInfoListPtr tip = (DmTaskInfoListPtr)call_data;
	DmFileOpInfoPtr   opr_info = tip->opr_info;
	DmWBCPDataPtr     cpdp = (DmWBCPDataPtr)client_data;
	char *name;
	char *tmp;

	switch(reason) {
	case DM_DONE:
		switch(cpdp->op_type) {
		case DM_PUTBACK:
		case DM_DROP:

			if (opr_info->src_info[0] & SRC_B_IGNORE)
				break;

			/* opr_info->target_path_path must always be a full path */
			tmp = strdup(opr_info->target_path);
			name = strdup(dirname(tmp));

			DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), 0,
				TXT_WB_PUTBACK_SUCCESS, cpdp->itp->label, basename(name));

			FREE(ITEM_OBJ_NAME(cpdp->itp));
			ITEM_OBJ_NAME(cpdp->itp) =
				strdup(basename(opr_info->target_path));

			FREE(opr_info->src_list[0]);
			opr_info->src_list[0] = strdup(ITEM_OBJ_NAME(cpdp->itp));

			FreeWBProps(cpdp->itp);

			if (opr_info->dst_win == NULL)
				opr_info->dst_win = DmQueryFolderWindow(name);
			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN | DM_UPDATE_DSTWIN);
			FREE(name);
			FREE(tmp);
			break;

		case DM_WBDELETE:
		case DM_TIMERCHG:

			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
			if (opr_info->error == 0) {
				DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
			}
			break;

		case DM_EMPTY:

			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);

			if (opr_info->error == 0)
				DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
			break;

		case DM_TIMER:

			/* Temporarily deal with error condition: if timer failed
			 * to remove a file, remove the item and obj anyway so
			 * timer doesn't continue to try to delete it.
			 */
			if (opr_info->error != 0) {
				DmItemPtr item =
					NameToItem(opr_info->src_list[(opr_info->cur_src == 0)
					? 0 : opr_info->cur_src - 1]);

				DmRmItemFromFolder(DESKTOP_WB_WIN(Desktop), item);
			} else
				DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
			break;

		case DM_IMMEDDELETE:

			if (opr_info->src_win == NULL)
				opr_info->src_win =
					DmQueryFolderWindow(opr_info->src_path);
			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
			if (opr_info->error == 0)
				DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
			break;

		default:
			break;
		}
		DmSwitchWBIcon();
		DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->cp, wb_dtinfo, 0);

		/* If the wastebasket is now empty but the timer is still
		 * running, suspend it and set wbdp->restart to True so
		 * that when it is non-empty again, the timer will be
		 * resumed for the user. If By Timer is selected but
		 * wbdp->suspend is True, make the Resume Timer button
		 * insensitive. If reason is DM_TIMER, just restart
		 * the timer.
		 */
		if (WB_BY_TIMER(wbdp)) {
			if (WB_IS_EMPTY(Desktop)) {
				if (wbdp->suspend == False) {
					wbdp->restart = True;
					DmWBRestartTimer();
				} else {
					wbdp->restart = False;
					/* make Suspend/Timer Button insensitive */
					DmWBToggleTimerBtn(Dm__gettxt(TXT_RESUME_LBL),
						*Dm__gettxt(TXT_M_RESUME_LBL), False);
				}
			} else if (cpdp->op_type == DM_TIMER) {
				DmWBRestartTimer();
			}
		}
		DmFreeTaskInfo(tip);

		if (cpdp->target)
			FREE(cpdp->target);

		if (cpdp)
			FREE((void *)cpdp);
		break;

	case DM_ERROR:
		switch(cpdp->op_type) {
		case DM_PUTBACK:
		case DM_DROP:
			name = (char *)DmGetObjProperty(ITEM_OBJ(cpdp->itp),
					REAL_PATH, NULL);

			if (opr_info->error == ERR_IsAFile)
				DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), True,
					TXT_WB_CANT_OVERWRITE, basename(name));
			else
				DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), True,
					TXT_WB_PUTBACK_FAILED, basename(name));
			break;

		case DM_WBDELETE:
		case DM_TIMERCHG:
			if (opr_info->cur_src == 0 || opr_info->cur_src == 1)
				DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), 1,
					TXT_WBDELETE_FAILED);
			else
				DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), 1,
					TXT_PARTIAL_DELETE);
			break;

		case DM_EMPTY:
			if (opr_info->cur_src == 0 || opr_info->cur_src == 1)
				DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), 1,
					TXT_EMPTY_FAILED);
			else
				DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), 1,
					TXT_PARTIAL_EMPTY);
			break;

		case DM_TIMER:
			/* Temporarily ignore any errors during timer deletion */
			break;

		case DM_IMMEDDELETE:
			if (opr_info->cur_src == 0 || opr_info->cur_src == 1)
				DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
					1, TXT_IMMEDDEL_FAILED);
			else
				DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
					1, TXT_PARTIAL_DELETE);
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
}					/* end of DmWBClientProc */

/****************************procedure*header*****************************
 * Frees all wastebasket-specific properties. 
 * This routine is called when a file is undeleted.
 */
static void
FreeWBProps(DmItemPtr itp)
{
	DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP, NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), VERSION, NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), ICONFILE, NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), REAL_PATH, NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), "f", NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), CLASS_NAME, NULL, NULL);
} /* end of FreeWBProps */

/****************************procedure*header*****************************
 * Callback for Properties button in icon menu to show file properties
 * of item from which icon menu is popped up.
 */
void
DmWBIMFilePropCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmItemPtr itp = (DmItemPtr)client_data;
	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), False, NULL);
	CreateFilePropSheet(itp);

} /* end of DmWBIMFilePropCB */

/****************************procedure*header*****************************
 * Callback for Properties button in Edit menu to show file properties
 * of all selected items in wastebasket window.
 */
void
DmWBEMFilePropCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmItemPtr itp;
	int       i;

	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), False, NULL);

	for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->itp;
		i < DESKTOP_WB_WIN(Desktop)->nitems; itp++, i++) {

		if (ITEM_MANAGED(itp) && ITEM_SELECT(itp) && !ITEM_BUSY(itp))
			CreateFilePropSheet(itp);
	}
} /* end of DmWBEMFilePropCB */

/****************************procedure*header*****************************
 * Creates a property sheet for item pointed to by itp.  The item is set
 * to busy state while the property sheet is up.  The property sheet is
 * destroyed when it is dismissed.
 */
void
CreateFilePropSheet(itp)
DmItemPtr itp;
{
	struct stat    ms;
	struct passwd  *pw;
	struct group   *gr;
	time_t         tstamp;
	Widget         fp_shell;
	Widget         uca;
	Widget         lca;
	Widget         buttons;
	XtArgVal       *btn_items;
	XtArgVal       *p;
	int            len;
	char           str[256];
	char           *pval;
	char           *tpath;

	/* busy the item; unbusy it when property sheet is dismissed */
	OlVaFlatSetValues(DESKTOP_WB_WIN(Desktop)->box,
		itp - DESKTOP_WB_WIN(Desktop)->itp,
		XtNbusy, True, NULL);

	/* to be used to unbusy item */
	XtSetArg(Dm__arg[0], XtNuserData, ITEM_OBJ_NAME(itp));
	fp_shell = XtCreatePopupShell(Dm__gettxt(TXT_WB_FILEPROP_TITLE),
				popupWindowShellWidgetClass,
				DESKTOP_WB_WIN(Desktop)->shell, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNupperControlArea, &uca);
	XtSetArg(Dm__arg[1], XtNlowerControlArea, &lca);
	XtGetValues(fp_shell, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNlayoutType, OL_FIXEDCOLS);
	XtSetArg(Dm__arg[1], XtNmeasure,    1);
	XtSetValues(uca, Dm__arg, 2);

	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_FP_FILE_NAME),
		(char *)(itp->label));

	pval = DmMakePath(DESKTOP_WB_WIN(Desktop)->cp->path, ITEM_OBJ(itp)->name);
	/* Should make use of file's stat info stored in its op->objectdata
	 * instead, if it's initialized.
	 */
	stat(pval, &ms);

	/* display link info */
	if (ITEM_OBJ(itp)->attrs & DM_B_SYMLINK) {
		/* display symbolic link info */
		int len;
		char buffer[BUFSIZ];

		len = readlink(pval, buffer, BUFSIZ);
		buffer[len] = '\0'; /* readlink doesn't do this */
		(void)DmCreateStaticText(uca, Dm__gettxt(TXT_FPROP_SYMLINK),buffer);
	}
	else if (ITEM_OBJ(itp)->ftype != DM_FTYPE_DIR) {
		/* display hard link info */
		char nlinks[32];

		sprintf(nlinks, "%d", ms.st_nlink);
		(void)DmCreateStaticText(uca, Dm__gettxt(TXT_FPROP_HARDLINK),nlinks);
	}

	pval =  DmGetObjProperty(ITEM_OBJ(itp), REAL_PATH, NULL);
	tpath = strdup(pval);
	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_WB_ORIG_LOC),dirname(tpath));
	free(tpath);

	pw = getpwuid(ms.st_uid);
	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_OWNER), pw->pw_name);

	gr = getgrgid(ms.st_gid);
	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_GROUP), gr->gr_name);

	(void)strftime(str, sizeof(str), TIME_FORMAT,localtime(&(ms.st_mtime)));
	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_MODTIME), str);

	tstamp = atol(DmGetObjProperty(ITEM_OBJ(itp), TIME_STAMP, NULL));
	(void)strftime(str, sizeof(str), TIME_FORMAT, localtime(&tstamp));
	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_WB_TIME_DELETED), str);

	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_OWNER_ACCESS),
			GetFilePerms(ms.st_mode, OWNER));

	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_GROUP_ACCESS),
			GetFilePerms(ms.st_mode, GROUP));

	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_OTHER_ACCESS),
			GetFilePerms(ms.st_mode, OTHER));

	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_ICON_CLASS),
		DmGetObjProperty(ITEM_OBJ(itp), CLASS_NAME, NULL));

	pval =  DmGetObjProperty(ITEM_OBJ(itp), "Comment", NULL);
	(void)DmCreateStaticText(uca, Dm__gettxt(TXT_COMMENTS), pval);

	btn_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 6 * 2);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_CANCEL_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_CANCEL_STR);
	*p++ = (XtArgVal)FilePropBtnCB;
	*p++ = (XtArgVal)True;
	*p++ = (XtArgVal)fp_shell;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HELP_ELLIPSIS));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HELP_STR);
	*p++ = (XtArgVal)FilePropBtnCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)NULL;
	*p++ = (XtArgVal)True;

	XtSetArg(Dm__arg[0], XtNitems,         btn_items);
	XtSetArg(Dm__arg[1], XtNnumItems,      2);
	XtSetArg(Dm__arg[2], XtNitemFields,    btn_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(btn_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNclientData,    fp_shell);
	XtSetArg(Dm__arg[7], XtNselectProc,    FilePropBtnCB);

	buttons = XtCreateManagedWidget("buttons", flatButtonsWidgetClass,
				lca, Dm__arg, 8);
	XtAddCallback(fp_shell, XtNverify, VerifyPopdnCB, (XtPointer)popdown);
	XtAddCallback(fp_shell, XtNpopdownCallback, PopdownCB,ITEM_OBJ_NAME(itp));

	XtPopup(fp_shell, XtGrabNone);

	/* register help */
     help_info.app_title = Dm__gettxt(TXT_WASTEBASKET);
     help_info.filename  = "DesktopMgr/wb.hlp";
     help_info.section   = "220";
     help_info.path      = NULL;
     help_info.title     = NULL;

     OlRegisterHelp(OL_WIDGET_HELP, fp_shell, NULL, OL_DESKTOP_SOURCE,
          (XtPointer)&help_info);

} /* end of CreateFilePropSheet */

/****************************procedure*header*****************************
 * Called when the Properties button in the icon menu of a file in the
 * Wastebasket is selected.  It busies the item for which for as long
 * as its file properties sheet is up.
 */
static void
FilePropBtnCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *fcd = (OlFlatCallData *)call_data;
	Widget         fp_shell = (Widget)client_data;

	if (fcd->item_index == 0) {
		DmItemPtr itp = NULL;
		char *obj_name = NULL;

		/* unbusy the item */
		XtSetArg(Dm__arg[0], XtNuserData, &obj_name);
		XtGetValues(fp_shell, Dm__arg, 1);

		if (obj_name != NULL) {
			itp = NameToItem(obj_name);

			if (itp != NULL) {
				OlVaFlatSetValues(DESKTOP_WB_WIN(Desktop)->box,
					itp - DESKTOP_WB_WIN(Desktop)->itp,
					XtNbusy, False, NULL);
			}
		}

		/* Cancel button selected */
		XtDestroyWidget((Widget)_OlGetShellOfWidget(w));

	} else {
		/* Help button selected */
		DmHelpAppPtr   help_app = DmGetHelpApp(WB_HELP_ID(Desktop));

		popdown = False;
		DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
			NULL, "DesktopMgr/wb.hlp", "220", UNSPECIFIED_POS,
			UNSPECIFIED_POS);
	}

} /* end of FilePropBtnCB */

/****************************procedure*header*****************************
 * Verify pop down callback.
 */
static void
VerifyPopdnCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	*((Boolean *)call_data) &= popdown;
} /* end of VerifyPopdnCB */

/****************************procedure*header*****************************
 * Destroy file properties sheet and unbusy item.
 */
static void
PopdownCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	char *obj_name = (char *)client_data;

	if (obj_name) {
		DmItemPtr itp = NameToItem(obj_name);

		if (itp)
			/* unbusy the item */
			OlVaFlatSetValues(DESKTOP_WB_WIN(Desktop)->box,
				itp - DESKTOP_WB_WIN(Desktop)->itp, XtNbusy, False, NULL);
	}
	XtDestroyWidget(w);

} /* end of PopdownCB */

/****************************procedure*header*****************************
 * Gets file permissions of a file in the wastebasket to be displayed
 * in its properties sheet.
 */
static char *
GetFilePerms(mode_t mode, int type)
{
	Boolean     rp, wp, ep;
	static char buf[128];

	switch(type) {
	case OWNER:
		rp = (mode & S_IRUSR ? True : False);
		wp = (mode & S_IWUSR ? True : False);
		ep = (mode & S_IXUSR ? True : False);
		break;
	case GROUP:
		rp = (mode & S_IRGRP ? True : False);
		wp = (mode & S_IWGRP ? True : False);
		ep = (mode & S_IXGRP ? True : False);
		break;
	case OTHER:
		rp = (mode & S_IROTH ? True : False);
		wp = (mode & S_IWOTH ? True : False);
		ep = (mode & S_IXOTH  ? True : False);
		break;
	}

	sprintf(buf, "%s %s %s", (rp ? Dm__gettxt(TXT_READ_PERM) : ""),
			(wp ? Dm__gettxt(TXT_WRITE_PERM) : ""),
			(ep ? Dm__gettxt(TXT_EXEC_PERM) : "")); 
	return(buf);
} /* end of GetFilePerms */
