/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_proc.c	1.50"

#include <libgen.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OlCursors.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

DmFolderWinPtr
DmIsFolderWin(Window win)
{
    register DmFolderWinPtr fwp;

    for (fwp = DESKTOP_FOLDERS(Desktop); fwp != NULL; fwp = fwp->next)
	if (XtWindow(fwp->box) == win)
	    return(fwp);

    return(NULL);
}

void
Dm__FolderDropProc(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatDropCallData *d = (OlFlatDropCallData *)call_data;
    DmItemPtr		src_item = ITEM_CD(d->item_data);
    DmFolderWinPtr	src_win;	/* source folder window */
    DmFolderWinPtr	dst_win;	/* dest folder window */
    void **		list;

#ifdef NOT_USE
    if (d->drop_status == OlDnDDropCanceled) {
printf("doesn't handle old protocol yet.\n");
        return;
    }
#endif

    /* Can't drag and drop a busy item */
    if (ITEM_BUSY(src_item))
	return;

    src_win	= (DmFolderWinPtr)client_data;
    dst_win	= (DmIsWB(d->dst_info->window)) ? DESKTOP_WB_WIN(Desktop) :
	((TREE_WIN(Desktop) != NULL) &&
	 (d->dst_info->window == XtWindow(TREE_WIN(Desktop)->box))) ?
	     TREE_WIN(Desktop) : DmIsFolderWin(d->dst_info->window);

    /* get the list of items to be operated on */
    list = DmGetItemList((DmWinPtr)src_win, d->item_data.item_index);

    /* Process a drop on the WasteBasket or a Folder window */
    if (dst_win != NULL)
    {
	DmFileOpInfoPtr	opr_info;
	DmObjectPtr	src_op;
	Cardinal	dst_idx;
	char *		target;
	int		status;

	if (IS_WB_WIN(Desktop, dst_win))
	{
	    if ((d->ve->virtual_name != OL_SELECT) &&
		(d->ve->virtual_name != OL_DRAG))
	    {
		DmVaDisplayStatus((DmWinPtr)src_win, True,
				  ((d->ve->virtual_name == OL_LINK) ||
				   (d->ve->virtual_name == OL_LINKKEY)) ?
				  TXT_CANT_WB_LINK : TXT_CANT_WB_COPY);
		return;
	    }

	    if (target = DmHasSystemFiles(list)) {
		DmVaDisplayStatus((DmWinPtr)src_win, True,
				  TXT_DEL_SYSTEM_FILE, target);
		FREE((void *)list);
		return;
	    }
	    target = strdup( DM_WB_PATH(Desktop) );
	    goto drop;
	}

	/* Process a drop on a FolderWindow */

	dst_idx = OlFlatGetItemIndex(dst_win->box,
				     d->dst_info->x, d->dst_info->y);

	if (dst_idx == OL_NO_ITEM)	/* dropping in an empty space? */
	{
	    /* Dropping onto found window or tree view has no meaning */
	    if (dst_win->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
	    {
		DmVaDisplayStatus((DmWinPtr)dst_win, True,
				  (dst_win->attrs & DM_B_TREE_WIN) ?
				  TXT_CANT_TREE_DROP : TXT_CANT_FOUND_DROP);
		return;
	    }
	    target = strdup(dst_win->cp->path);

	} else				/* dropping onto an icon */
	{
	    char *dropcmd;
	    DmObjectPtr	dst_op = ITEM_OBJ(DM_WIN_ITEM(dst_win, dst_idx));

	    /* If there is a DROPCMD property, use it and return */
	    if ((dropcmd = DmGetObjProperty(dst_op, DROPCMD, NULL)) != NULL)
	    {
		if (!strcmp(dropcmd, "##DELETE()") &&
		    (target = DmHasSystemFiles(list))) {
		    DmVaDisplayStatus((DmWinPtr)src_win, True,
				      TXT_DEL_SYSTEM_FILE, target);
		    FREE((void *)list);
		    return;
	        }

		while (*list != NULL)
		    DmDropObject((DmWinPtr)dst_win, dst_idx, (DmWinPtr)src_win,
				 ITEM_OBJ((DmItemPtr)(*list++)));
		return;

	    } else if (!OBJ_IS_DIR(dst_op))
		return;		/* dropping onto non-dir w/no property */

	    /* Getting here means dropping onto folder icon */

	    target = strdup( DmObjPath(dst_op) );

	    /* Not dropping on bg so can't use dst_win or x & y */
	    dst_win = NULL;
	    d->dst_info->x = UNSPECIFIED_POS;
	    d->dst_info->y = UNSPECIFIED_POS;
	}

	/* Falling thru to here means the item will be mv/cp/ln'ed */
    drop:
	opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
	opr_info->target_path	= target;	/* already dup'ed above */
	opr_info->src_path	= strdup(src_win->cp->path);
	opr_info->src_list	= DmItemListToSrcList(list,
						      &(opr_info->src_cnt));
	opr_info->src_win	= src_win;
	opr_info->dst_win	= dst_win;

	/* If Wastebasket is iconized, set opr_info-> and opr_info->y
	 * to UNSPECIFIED_POS so that icons won't be stacked upon each other.
	 */
	if (dst_win == DESKTOP_WB_WIN(Desktop) && 
		GetWMState(XtDisplay(DESKTOP_WB_WIN(Desktop)->shell),
			XtWindow(DESKTOP_WB_WIN(Desktop)->shell)) == IconicState) {
		opr_info->x = opr_info->y = UNSPECIFIED_POS;
	} else {
		opr_info->x		= d->dst_info->x;
		opr_info->y		= d->dst_info->y;
	}

	FREE((void *)list);

	/* Establish file operation type based on button type */
	opr_info->type =
	    ((d->ve->virtual_name == OL_SELECT) ||
	     (d->ve->virtual_name == OL_DRAG)) ?	DM_MOVE :
	    ((d->ve->virtual_name == OL_LINK) ||
	     (d->ve->virtual_name == OL_LINKKEY)) ?	DM_SYMLINK :
		/* OL_DUPLICATE */			DM_COPY;

	/* Determine options.  If this is a "special" window (Tree view or
	   Found window), tag the list of srcs as special.
	*/
	opr_info->options = REPORT_PROGRESS | OVERWRITE | OPRBEGIN;
	if (IS_TREE_WIN(Desktop, src_win) || IS_FOUND_WIN(Desktop, src_win))
	    opr_info->options |= MULTI_PATH_SRCS;

	DmFreeTaskInfo(src_win->task_id);

	/* If destination is Wastebasket and Immediate Delete mode is set,
	 * make this a DM_DELETE operation.
	 */ 
	if (opr_info->dst_win == DESKTOP_WB_WIN(Desktop)
	    && DM_WBIsImmediate(Desktop)) {
		opr_info->type        = DM_DELETE;
		if (opr_info->target_path)
			free(opr_info->target_path);
		opr_info->target_path = NULL;
		opr_info->dst_win     = NULL;
	}
	src_win->task_id = DmDoFileOp(opr_info, DmFolderFMProc, NULL);

    } else
    {
	/* dropped onto somewhere (may be outside dtm) */
	(void)DmDnDNewTransaction((DmWinPtr)src_win,
				  (DmItemPtr *)list,	/* freed after trans */
				  0,
				  d->root_info,
				  0,
				  d->ve->virtual_name,
				  DmConvertSelectionProc,
				  DmTransactionStateProc);
    }
}

void
Dm__FolderSelect2Proc(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFIconBoxButtonCD *d = (OlFIconBoxButtonCD *)call_data;

    BUSY_CURSOR(w);
    DmOpenObject((DmWinPtr)client_data, OBJECT_CD(d->item_data));
}
