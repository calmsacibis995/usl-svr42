/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:dnd_util.c	1.8" */

#include <libgen.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <Xol/OpenLook.h>
#include "Dtm.h"
#include "extern.h"

/*
 * ConvertSelectionProc -
 *
 *	This is one of the important parts of drag-and-drop.
 *	This operation is going thru the selection mechanism.
 *	This routine converts the target into
 *	the actual data when a request is received from "destination".
 *
 */
Boolean
DmConvertSelectionProc( Widget		w,
			Atom *		selection,
			Atom *		target,
			Atom *		type_rtn,
			XtPointer *	val_rtn,
			unsigned long *	length_rtn,
			int *		format_rtn)
{
	Boolean			ret_val = False;
	Display *		dpy = XtDisplay(w);
	String			stuff;
	DmDnDInfoPtr		dip;
	char			*ret_str = NULL; /* return string */

	if ((dip = DtGetData(XtScreen(w), DM_CACHE_DND, (void *)*selection,
				sizeof(*selection))) == NULL)
		/*
		 * A selection conversion request that is not part
		 * of any outstanding transactions.
		 */
		return(False);

	if (*target == OL_XA_TARGETS(dpy)) {
		Atom *		everything;

#define N_ATOMS		3

		everything = (Atom *)malloc(N_ATOMS * sizeof(Atom));
		everything[0] = OL_XA_FILE_NAME(dpy);
		everything[1] = OL_XA_HOST_NAME(dpy);
		everything[2] = XA_STRING;
#ifdef NOT_USE
		everything[3] = OL_USL_OBJECT_CLASS(dpy);
		everything[4] = OL_USL_HOTSPOT_TO_OBJECT_POSITION(dpy);
		everything[5] = OL_USL_OBJECT_PROPERTIES(dpy);
#endif

		*format_rtn = 32;
		*length_rtn = (unsigned long) N_ATOMS;
		*val_rtn    = (XtPointer)everything;
		*type_rtn   = XA_ATOM;
		ret_val = True;
#undef N_ATOMS
	}
	else if (*target == OL_XA_FILE_NAME(dpy) || *target == XA_STRING) {
		if (dip->ip)
			ret_str = DmObjPath(ITEM_OBJ(dip->ip));
	}
	else if (*target == OL_XA_HOST_NAME(dpy))
		ret_str = DESKTOP_NODE_NAME(Desktop);
	else if (*target == OL_USL_NUM_ITEMS(dpy)) {
		int cnt;
		DmItemPtr *lp;

		/* count the # of items */
		for (cnt=0, lp=dip->ilist; *lp; lp++, cnt++) ;

		*format_rtn = 32;
		*length_rtn = 1;
		*val_rtn    = (XtPointer)malloc(sizeof(long));
		*type_rtn   = *target;
		*(long *)(*val_rtn) = cnt;
		ret_val = True;
	}
	else if (*target == OL_USL_ITEM(dpy)) {
		if (*(dip->list_idx))
			dip->ip = *(++(dip->list_idx));
		else {
			/* reset to the beginning */
			dip->list_idx = dip->ilist;
			dip->ip = *(dip->ilist);
		}
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = NULL;
		*type_rtn   = *target;
		ret_val = True;
	}
	else if (*target == OL_XA_DELETE(dpy)) {
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = NULL;
		*type_rtn   = *target;

printf("delete\n");
		ret_val = True;
	}
#ifdef FUTURE
	else if (*target == OL_USL_OBJECT_CLASS(dpy)) {
		if (dip->ip)
			ret_str = OBJ_CLASS_NAME(ITEM_OBJ(dip->ip));
	}
	else if (*target == OL_USL_OBJECT_PROPERTIES(dpy)) {
		/* to be implemented */
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = (XtPointer)malloc(*length_rtn);
		*type_rtn   = *target;

		ret_val = True;
	}
	else if (*target == OL_USL_HOTSPOT_TO_OBJECT_POSITION(dpy)) {
		*format_rtn = 32;
		*length_rtn = 2;
		*val_rtn    = (XtPointer)malloc(sizeof(long) * 2);
		*type_rtn   = *target;
		*(long *)(*val_rtn) = (long)((dip->wp->itp + dip->idx)->
						icon_width) / 2;
		*((long *)(*val_rtn) + 1) = (long)(dip->op->fcp->glyph->height)
						/ 2;
		ret_val = True;
	}
#endif
	if (ret_str) {
		*format_rtn = 8;
		*length_rtn = strlen(ret_str) + 1;
		*val_rtn    = (XtPointer)malloc(*length_rtn);
		*type_rtn   = *target;
		strcpy((char *)*val_rtn, ret_str);
		ret_val = True;
	}

	return(ret_val);
} /* end of DmConvertSelectionProc */

/*
 * DmTransactionStateProc -
 *
 *	This should be the only place to disown/free "transient"
 *	because this is the responsibility of "resource". This also
 *	implies all relevant info/resources should also be freed here
 *	(included the icon etc.). Without the assumption above (i.e.,
 *	freeing the transient), the comments about "transient" in the
 *	DroppedOnSomething() is not hold.
 *
 */
void
DmTransactionStateProc(Widget			w,
		     Atom			selection,
		     OlDnDTransactionState	state,
		     Time			timestamp,
		     XtPointer			closure)
{
	DmDnDInfoPtr dip;

	if ((dip = DtGetData(XtScreen(w), DM_CACHE_DND, (void *)selection,
				sizeof(selection))) == NULL)
		/*
		 * A selection conversion request that is not part
		 * of any outstanding transactions.
		 */
		return;

	switch(state) {
	case OlDnDTransactionDone:
	case OlDnDTransactionRequestorError:
	case OlDnDTransactionRequestorWindowDeath:
		DmDnDFreeTransaction(dip);
		break;
	}
} /* end of DmTransactionStateProc */

DmDnDInfoPtr
DmDnDNewTransaction(DmWinPtr				wp,
		    DmItemPtr				*ilist,
		    DtAttrs				attrs,
		    OlDnDDragDropInfoPtr		root_info,
		    Window				dst_win,
		    OlVirtualName			virtual_name,
		    XtConvertSelectionProc		convert_proc,
		    OlDnDTransactionStateCallback	trans_state_proc)
{
	DmDnDInfoPtr dip;

	if ((dip = (DmDnDInfoPtr)malloc(sizeof(DmDnDInfoRec))) == NULL)
		return(NULL);

	if ((dip->selection = OlDnDAllocTransientAtom(wp->box)) == (Atom)None) {
		/* couldn't allocate a free transient atom */
		free(dip);
		return(NULL);
	}

	/* own the transient id */
	if (OlDnDOwnSelection(wp->box, dip->selection,
			      root_info->drop_timestamp,
			      convert_proc,
			      (XtLoseSelectionProc)NULL,
			      (XtSelectionDoneProc)NULL,
			      trans_state_proc,
			      (XtPointer)dip) == False) {
		/* failed to own selection */
		DmDnDFreeTransaction(dip);
		return(NULL); /* for now */
	}

	dip->opcode = (virtual_name == OL_DUPLICATE) ?
				OlDnDTriggerCopyOp :
				OlDnDTriggerMoveOp;

	/*
	 * Must initialize and add dip to the cache first!
	 * You see, OlDnDDeliverTriggerMessage() will shortcircuit transactions
	 * that go to windows in the same process space.
	 */

	/* initialize structure */
	dip->wp		= wp;
	dip->ilist	= ilist;
	dip->x		= root_info->root_x;
	dip->y		= root_info->root_y;
	dip->timestamp	= root_info->drop_timestamp;
	dip->attrs	= attrs;
	dip->user_data	= NULL;

	/* set default item to the first item in the list */
	dip->list_idx = ilist;
	dip->ip = *ilist;

	DtPutData(XtScreen(wp->box), DM_CACHE_DND, (void *)(dip->selection),
		  sizeof(dip->selection), dip);

	if (attrs & DM_B_SEND_EVENT) {
		if (OlDnDSendTriggerMessage(wp->box,
			root_info->root_window,
			dst_win,
			dip->selection,
			dip->opcode,
			root_info->drop_timestamp) == False) {
				/* failed to deliver trigger message */
				DmDnDFreeTransaction(dip);
				return(NULL); /* for now */
		}
	}
	else {
		if (OlDnDDeliverTriggerMessage(wp->box,
		       root_info->root_window,
		       root_info->root_x,
		       root_info->root_y,
		       dip->selection,
		       dip->opcode,
		       root_info->drop_timestamp) == False) {
			/* failed to deliver trigger message */
			DmDnDFreeTransaction(dip);
			return(NULL); /* for now */
		}
	}

	return(dip);
} /* end of DmDnDNewTransaction */

void
DmDnDFreeTransaction(dip)
DmDnDInfoPtr dip;
{

	/* remove it from cache */
	DtDelData(XtScreen(dip->wp->box), DM_CACHE_DND,
		  (void *)(dip->selection), sizeof(dip->selection));

	OlDnDDisownSelection(dip->wp->box, dip->selection, CurrentTime);
	OlDnDFreeTransientAtom(dip->wp->box, dip->selection);

	free(dip->ilist);
	free(dip);
} /* end of DmDnDFreeTransaction */

