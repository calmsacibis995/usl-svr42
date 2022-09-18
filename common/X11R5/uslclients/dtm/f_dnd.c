/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:f_dnd.c	1.15" */

#include <libgen.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <Xol/OpenLook.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

static void
GotSrcFiles(Widget w, XtPointer client_data, XtPointer call_data)
{
    DtDnDInfoPtr	dnd_info = (DtDnDInfoPtr)call_data;
    DmDnDFileOpPtr	file_info;
    DmWinPtr		dst_win;
    int			x, y;
    Window		dummy_win;
    Cardinal		dst_idx;
    char *		target;
    DmFileOpInfoPtr	opr_info;

    if (dnd_info->error || (dnd_info->nitems == 0))
	goto quit;

    file_info = (DmDnDFileOpPtr)client_data;
    dst_win = file_info->wp;

    if (IS_WB_WIN(Desktop, (DmFolderWindow)dst_win))
    {
	if (file_info->type != OlDnDTriggerMoveOp)
	{
	    DmVaDisplayStatus(dst_win, True,
			      (file_info->type == OlDnDTriggerLinkOp) ?
			      TXT_CANT_WB_LINK : TXT_CANT_WB_COPY);
	    goto quit;
	}

	target = strdup( DM_WB_PATH(Desktop) );
	goto drop;
    }

    XTranslateCoordinates(XtDisplay(w), RootWindowOfScreen(XtScreen(w)),
			  XtWindow(w), (int)file_info->root_x,
			  (int)file_info->root_y, &x, &y, &dummy_win);

    dst_idx = OlFlatGetItemIndex(dst_win->box, (Position)x, (Position)y);

    if (dst_idx == OL_NO_ITEM)		/* dropping in an empty space? */
    {
	/* Dropping onto found window or tree view has no meaning */
	if (dst_win->attrs & (DM_B_TREE_WIN & DM_B_FOUND_WIN))
	{
	    DmVaDisplayStatus(dst_win, True,
			      IS_TREE_WIN(Desktop, (DmFolderWindow)dst_win) ?
			      TXT_CANT_TREE_DROP : TXT_CANT_FOUND_DROP);
	    goto quit;
	}

	target = strdup(dst_win->cp->path);

    } else				/* dropping onto an icon */
    {
	DmObjectPtr	dst_op = ITEM_OBJ(DM_WIN_ITEM(dst_win, dst_idx));

	/* If there is a DROPCMD property, use it and return */
	if (DmGetObjProperty(dst_op, DROPCMD, NULL) != NULL)
	{
	    char ** p;
	    for (p = dnd_info->files; *p != NULL; p++)
		DmDropObject(dst_win, dst_idx, NULL, (DmObjectPtr)*p);

	    goto quit;

	} else if (!OBJ_IS_DIR(dst_op))
	    goto quit;		/* dropping onto non-dir w/no property */

	target = strdup( DmObjPath(dst_op) );

	/* Not dropping on bg so can't use dst_win */
	dst_win = NULL;

	/* Make position unspecified so item can be correctly positioned
	   in the dest folder.
	*/
	x = UNSPECIFIED_POS;
	y = UNSPECIFIED_POS;
    }

    /* Falling thru to here means the item(s) will be mv/cp/ln'ed */
 drop:
    dnd_info->attrs |= DT_B_STATIC_LIST;	/* keep src_list around */

    opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
    opr_info->target_path	= target;	/* already dup'ed above */
    opr_info->src_path		= NULL;		/* src's have full paths */
    opr_info->src_list		= dnd_info->files;
    opr_info->src_cnt		= dnd_info->nitems;
    opr_info->src_win		= NULL;
    opr_info->dst_win		= (DmFolderWindow)dst_win;
    opr_info->x			= x;
    opr_info->y			= y;

    /* Establish file operation type based on drop type */
    opr_info->type =
	(file_info->type == OlDnDTriggerMoveOp)	? DM_MOVE :
	(file_info->type == OlDnDTriggerLinkOp)	? DM_SYMLINK :
	    /* OL_DUPLICATE */			  DM_COPY;

    /* Options depend on whether caller needs notification when done.
       Note that src list may have multi-paths.
    */
    if (dnd_info->send_done)
    {
	opr_info->options = MULTI_PATH_SRCS | EXTERN_DND | EXTERN_SEND_DONE;
	dnd_info->send_done = False;

    } else
	opr_info->options = MULTI_PATH_SRCS | EXTERN_DND;

    /* Return now without freeing client_data yet if file op accepted and
       caller needs notification when done.  Task info is freed then, too.
    */
    if ((DmDoFileOp(opr_info, DmFolderFMProc, (XtPointer)file_info) != NULL) &&
	(opr_info->options & EXTERN_SEND_DONE))
	return;

 quit:
    FREE(client_data);
}					/* end of GotSrcFiles */

Boolean
DmFolderTriggerNotify(Widget			w,
		      Window			win,
		      Position			root_x,
		      Position			root_y,
		      Atom			selection,
		      Time			timestamp,
		      OlDnDDropSiteID		drop_site_id,
		      OlDnDTriggerOperation	op,
		      Boolean			send_done,
		      Boolean			forwarded, /* not used */
		      XtPointer			closure)
{
    DmDnDFileOpPtr file_info = (DmDnDFileOpPtr)MALLOC(sizeof(DmDnDFileOpRec));

    file_info->type	= op;
    file_info->wp	= (DmWinPtr)closure;
    file_info->root_x	= root_x;
    file_info->root_y	= root_y;
    file_info->selection= selection;

    DtGetFileNames(w, selection, timestamp, send_done,
		   GotSrcFiles, (XtPointer)file_info);

}					/* end of DmFolderTriggerNotify */
