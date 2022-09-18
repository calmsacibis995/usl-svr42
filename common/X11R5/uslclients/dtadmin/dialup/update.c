/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/update.c	1.9"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <Xol/FList.h>
#include "uucp.h"

extern void SetFields();
extern void ResetFields();

/* 
 * Do the following:
 * Select "new" in the scrolling list.
 * Position to "new" in the scrolling list.
 * Set the "new" data in the selected field.
 */

void
UnselectSelect()
{
	HostData *hp = sf->flatItems[sf->currentItem].pField;
	
	/* Select the new item */
	OlVaFlatSetValues (
		sf->scrollingList,
		sf->currentItem,
		XtNset,	True,
		(String)0
	);
	XtVaSetValues (
		sf->scrollingList,
		XtNviewItemIndex, sf->currentItem,
		(String)0
	);
	/* Reset the cusor to the first text field */
	AcceptFocus(sf->w_name);
	/* clear up the footer msg on the base window */
	CLEARMSG();
	ResetFields(hp);
} /* UnselectSelect */
