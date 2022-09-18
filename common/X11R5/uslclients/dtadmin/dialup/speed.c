/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/speed.c	1.9"
#endif

/* ############################  Port Speed ############################ */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FButtons.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

static DeviceItems *	FindSpeed();
extern void		Warning();
extern Widget		CreateCaption();
extern void		CBselect();
extern void		CBunselect();

DeviceItems	speeds[] = {
	{ label_any, "Any" },
	{ label_b38400, "38400" },
	{ label_b19200, "19200" },
	{ label_b9600, "9600" },
	{ label_b4800, "4800" },
	{ label_b2400, "2400" },
	{ label_b1200, "1200" },
	{ label_b300, "300" },
};
/*
 * itemFields[] -  array of choice item resource names
 */
static String  itemFields[] = {
        XtNlabel,
	XtNclientData,
};

#define NR_ITEM_FIELDS  (sizeof(itemFields) / sizeof(itemFields[0]))
#define NR_SPEEDS	(sizeof(speeds) / sizeof(DeviceItems))

caddr_t
PortSpeed(message, param)
int	message;
caddr_t	param;
{
	int			i;
	static DeviceItems	*current = &speeds[0];
	static Boolean		first_time = True;

	if (first_time) {
		first_time = False;

		for (i = 0; i < NR_SPEEDS; i++)
			speeds[i].label = GGT(speeds[i].label);
	}


	switch (message) {
	case NEW: {
		/*
		 * NEW: create the object from scratch
		 *
		 *	INPUT: param == parent widget
		 *	RETURN: parent's child (highest level created here)
		 */
		 
		Widget			w_parent = (Widget) param;


		/*
		 * Add the exclusives area as the caption's child
		 */
		sf->w_class = 
			XtVaCreateWidget(
				"speeds",
				flatButtonsWidgetClass,
				CreateCaption(w_parent, GGT(label_class)),
				XtNlayoutType,	        OL_FIXEDROWS,
				XtNmeasure,	        2,
				XtNitems,               speeds,
				XtNnumItems,            NR_SPEEDS,
				XtNitemFields,          itemFields,
				XtNnumItemFields,       NR_ITEM_FIELDS,
				XtNlabelJustify,        OL_LEFT,
				XtNselectProc,          CBselect,
				XtNunselectProc,        CBunselect,
				XtNbuttonType,		OL_RECT_BTN,
				XtNexclusives,		TRUE,
				0
			);

		/*
		 * Manage the choice widget and return
		 */
		XtManageChild(sf->w_class);
		return((caddr_t) sf->w_class);
	}

	case SET: {
		/*
		 * SET: set the value of the object.  NOTE that this is
		 * a forced set by our parent, the popup window object,
		 * as opposed to a direct use selection.
		 *
		 *	INPUT: param == value to set
		 *	RETURN: value actually set
		 */
		String		value = (String) param;
		DeviceItems	*pItem;
		int		index;

		if (strcmp(current->value, value) == 0) {
			return((caddr_t) value);	/* already set */
		} 

		pItem = FindSpeed(value, speeds, NR_SPEEDS, &index);
		if (pItem != NULL) {
			OlVaFlatSetValues(
				sf->w_class,
                		index,   /* sub-item to modify */
				XtNset,     TRUE,
				0
        		);

			current = pItem;		/* remember new */
		}
		return((caddr_t) current->value);    /* return current value */
	}

	case GET: {
		/*
		 * GET: return the current value of the object
		 *
		 *	INPUT: param N/A
		 *	RETURN: current value
		 */
		return((caddr_t) current->value);
	}

	case SELECT: {
		/*
		 * SELECT: One of our widgets become selected, so remember
		 * which one it was so we can return the value on GET.
		 * NOTE that this message is for use by the XtNselect callback
		 * associated with the button widgets.
		 *
		 *	INPUT: param == pSizeItem of object
		 *	RETURN: pSizeItem of object actually set.
		 */
		current = (DeviceItems *) param;
		return((caddr_t) current);
	}

	default:
		Warning("SpeedSpeed: Unknown message %d\n", message);
		return((caddr_t) NULL);
	}
} /* PortSpeed */

static DeviceItems *
FindSpeed(value, list, count, index)
String		value;
DeviceItems	*list;
register	count;
int*		index;
{
	register DeviceItems	*pItem = list;
	register i;

	for (i=0; i < count; i++, pItem++) {
		if (!strcmp(pItem->value, value)) {
			*index = i;
			return(pItem);
		}
	}
	Warning("FindSpeed: couldn't find value %s\n", value);
	return(NULL);
} /* FindSpeed */
