/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/update.c	1.5"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <Xol/FList.h>
#include "inet.h"

extern void SetFields();
extern void ResetFields();

/* 
 * Do the following:
 * Select "new" in the scrolling list.
 * Position to "new" in the scrolling list.
 */

void
UnselectSelect()
{
	HostData *hp = hf->flatItems[hf->currentItem].pField;
	/* Select the new item */
	OlVaFlatSetValues (
		hf->scrollingList,
		hf->currentItem,
		XtNset,	True,
		0
	);
	XtVaSetValues (
		hf->scrollingList,
		XtNviewItemIndex, hf->currentItem,
		(String)0
	);

	/* update the property sheet's fields */
	ResetFields(hp);
} /* UnselectSelect */
