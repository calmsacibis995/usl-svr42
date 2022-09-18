/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:tb_proc.c	1.22"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <FIconBox.h>

#include "Dtm.h"
#include "strings.h"
#include "extern.h"

void
Dm__ToolboxDropProc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	OlFIconBoxDropCD *d = (OlFIconBoxDropCD *)call_data;
	DmToolboxWinPtr dst_twp;
	DmToolboxWinPtr src_twp;
	DmFolderWinPtr dst_fwp;
	DmItemPtr ip = ITEM_CD(d->item_data);
	DmObjectPtr op;
	Dimension icon_width;
	Dimension icon_height;

	/* Can't drag and drop a busy item */
	if (ITEM_BUSY(ip))
		return;

	if (d->drop_status == OlDnDDropCanceled) {
printf("doesn't handle old protocol yet.\n");
		return;
	}

	XtSetArg(Dm__arg[0], XtNobjectData, &op);
	XtSetArg(Dm__arg[1], XtNwidth, &icon_width);
	XtSetArg(Dm__arg[2], XtNheight, &icon_height);
	OlFlatGetValues(w, d->item_data.item_index, Dm__arg, 3);
	src_twp = DmIsToolboxWin(XtWindow(w));

	/* clear footer first */
	DmVaDisplayMsg((DmWinPtr)src_twp, Dm__TBErrorMessages, TB_ERR_CLEAR);

	if (dst_twp = DmIsToolboxWin(d->drop_data.window)) {
		/* dropped onto a toolbox window */
		int idx = OlFlatGetItemIndex(dst_twp->box, d->drop_data.x,
						 d->drop_data.y);

		if (idx == OL_NO_ITEM) {
			/* copy/move to another toolbox */

			/*
			 * First, check if dropping a toolbox icon
			 * onto itself.
			 */
			if ((op->ftype == DM_FTYPE_TOOLBOX) &&
			    !strcmp(op->name, dst_twp->cp->path)) {
				DmVaDisplayMsg((DmWinPtr)src_twp,
						Dm__TBErrorMessages,
						d->drop_data.ve->virtual_name
						== OL_SELECT ?
						TB_ERR_MOVE_SELF :
						TB_ERR_COPY_SELF,
						dst_twp->cp->path);
				return;
			}

			if (d->drop_data.ve->virtual_name == OL_SELECT) {
				/* move */
				DmDelObjectFromContainer(src_twp->cp, op);
				d->drop_data.x -= icon_width / 2;
				d->drop_data.y -= op->fcp->glyph->height / 2;
				if (DmAddShortcut(dst_twp, op, d->drop_data.x,
					d->drop_data.y, 
					DM_B_DUPCHECK | DM_B_MOVE)
					!= (int)OL_NO_ITEM) {
					DmUnmanageIcon(src_twp,
						d->item_data.item_index);
				}
				else {
					DmAddObjectToContainer(src_twp->cp,
								op,0);
				}
			}
			else {
				/* copy */
				int x, y;

				x = d->drop_data.x - icon_width / 2;
				y = d->drop_data.y - op->fcp->glyph->height / 2;
				if (!DmTBCopyShortcut(dst_twp->cp,
						      src_twp->cp, op, x, y)) {
					DmVaDisplayMsg((DmWinPtr)src_twp,
							Dm__TBErrorMessages,
							TB_ERR_TBCOPY,
							op->name);
				}
			}
		}
		else {
			/* drop onto an icon in a toolbox */
			DmObjectPtr dst_op;

			XtSetArg(Dm__arg[0], XtNobjectData, &dst_op);
			OlFlatGetValues(dst_twp->box, idx, Dm__arg, 1);
			if (dst_op->ftype == DM_FTYPE_TOOLBOX) {
				int x, y;
				Dimension width;
				DmContainerPtr dst_cp;
				
				/* move into a sub-toolbox */
				/* check if icon onto the same toolbox. */
				if (!strcmp(dst_op->name, src_twp->cp->path))
					return;

				dst_cp = DmGetToolboxInfo(dst_op->name);
				if (DmTBMoveShortcut(dst_cp,
						     src_twp->cp, op)) {
					DmUnmanageIcon(src_twp,
						       d->item_data.item_index);
				}
			}
			else if (dst_op->ftype == DM_FTYPE_DIR) {
				DmVaDisplayMsg((DmWinPtr)src_twp,
						Dm__TBErrorMessages,
						TB_ERR_DROPDIR);
			}
			else {
				if (op->ftype == DM_FTYPE_TOOLBOX) {
					/* can't drop toolbox into shortcut */
					DmVaDisplayMsg((DmWinPtr)src_twp,
							Dm__TBErrorMessages,
							TB_ERR_TBDROP);
				}
				else {
					/*drop a shortcut on another shortcut*/
					DmDropObject(dst_twp->box, idx,
							TB_REALOP(dst_op),
							(DmWinPtr)src_twp,
							TB_REALOP(op));
				}
			}
		}
	}
	else if (DmIsWB(d->drop_data.window)) {
		/* dropped onto the waste basket */
		if (DmDelShortcut(src_twp->cp, op)) {
			DmVaDisplayMsg((DmWinPtr)src_twp,
					Dm__TBErrorMessages,
					TB_ERR_TBDEL, op->name);
			return;
		}
	}
	else if (dst_fwp = DmIsFolderWin(d->drop_data.window)) {
		/* dropped onto a folder window */
		int idx;

		if (op->ftype == DM_FTYPE_TOOLBOX) {
err_drop:
			/* can't drop toolbox into folder window */
			DmVaDisplayMsg((DmWinPtr)src_twp,
					Dm__TBErrorMessages,
					TB_ERR_DROPDIR);
			return;
		}

		idx = OlFlatGetItemIndex(dst_fwp->box, d->drop_data.x,
						 d->drop_data.y);
		if (idx == OL_NO_ITEM)
			goto err_drop;
		else {
			DmObjectPtr dst_op;

			XtSetArg(Dm__arg[0], XtNobjectData, &dst_op);
			OlFlatGetValues(dst_fwp->box, idx, Dm__arg, 1);
			if (dst_op->ftype == DM_FTYPE_DIR)
				goto err_drop;
			else
				/*drop a shortcut on another shortcut*/
				DmDropObject(dst_fwp->box, idx,
						dst_op,
						(DmWinPtr)src_twp,
						TB_REALOP(op));
		}
	}
	else {
		/* dropped onto somewhere (may be outside dtm) */
		(void)DmDnDNewTransaction((DmWinPtr)src_twp,
					       TB_REALOP(op),
					       0,
					       d->root_info,
					       0,
					       OL_DUPLICATE,
					       DmConvertSelectionProc,
					       DmTransactionStateProc);
	}
}

void
Dm__ToolboxSelect2Proc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	OlFIconBoxButtonCD *d = (OlFIconBoxButtonCD *)call_data;
	DmObjectPtr op = OBJECT_CD(d->item_data);
	Boolean busy;
	DmToolboxWinPtr src_twp = DmIsToolboxWin(XtWindow(w));

	/* clear footer first */
	DmVaDisplayMsg((DmWinPtr)src_twp, Dm__TBErrorMessages, TB_ERR_CLEAR);

	XtSetArg(Dm__arg[0], XtNbusy, True);
	OlFlatSetValues(w, d->item_data.item_index, Dm__arg, 1);
	OlFlatRefreshItem(w, d->item_data.item_index, True);
	if (op->objectdata)
		DmOpenObject((DmWinPtr)src_twp, (DmObjectPtr)(op->objectdata));
	else
		DmOpenObject((DmWinPtr)src_twp, op);
	XtSetArg(Dm__arg[0], XtNbusy, False);
	OlFlatSetValues(w, d->item_data.item_index, Dm__arg, 1);
	OlFlatRefreshItem(w, d->item_data.item_index, True);
}
