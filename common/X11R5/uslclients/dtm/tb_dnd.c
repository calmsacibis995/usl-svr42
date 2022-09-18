/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:tb_dnd.c	1.2"
#endif

#include <libgen.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <Xol/OpenLook.h>
#include "Dtm.h"
#include "extern.h"

Boolean
DmTBTriggerNotify(Widget			w,
		  Window			win,
		  Position			x,
		  Position			y,
		  Atom				selection,
		  Time				timestamp,
		  OlDnDDropSiteID		drop_site_id,
		  OlDnDTriggerOperation		op,
		  Boolean			send_done,
		  Boolean			forwarded, /* not used */
		  XtPointer			closure)
{
	DmFolderWinPtr fwp = (DmFolderWinPtr)closure;
	DmDnDInfoPtr   dip;

	/* allocate DnD transaction structure */
	if ((dip = (DmDnDInfoPtr)malloc(sizeof(DmDnDInfoRec))) == NULL)
		return(False); /* Is this right? */

	/* initialize structure */
	dip->wp		= (DmWinPtr)fwp;
	dip->x		= x;
	dip->y		= y;
	dip->selection	= selection;
	dip->timestamp	= timestamp;
	dip->attrs	= (op == OlDnDTriggerCopyOp) ?
				 (DM_B_COPY_OP | DM_B_TRANS_IN) : DM_B_TRANS_IN;
	dip->user_data	= NULL;

	
printf("tb_trigger: x=%d y=%d oper=%s\n", x, y, op == OlDnDTriggerMoveOp ? "Move" : "Copy");

} /* end of DmTBTriggerNotify */

