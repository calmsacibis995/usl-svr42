/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/port.c	1.10"
#endif

/* ############################  Port Number  ############################ */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

static DeviceItems *	FindPort();
extern Widget		CreateCaption();
extern void		Warning();

DeviceItems	ports[] = {
	{ label_com1, "com1" },
	{ label_com2, "com2" },
	{ label_other, "other" },
};
/*
 * itemFields[] -  array of choice item resource names
 */
static String  itemFields[] = {
        XtNlabel,
	XtNclientData,
};

#define NR_ITEM_FIELDS  (sizeof(itemFields) / sizeof(itemFields[0]))
#define NR_PORTS	(sizeof(ports) / sizeof(DeviceItems))

caddr_t
PortNumber(message, param)
int	message;
caddr_t	param;
{
	int			i;
	static DeviceItems	*current = &ports[0];
	static Boolean 		manage_extra = True;
	static Boolean		first_time = True;

	if (first_time) {
		first_time = False;

		for (i = 0; i < NR_PORTS; i++)
			ports[i].label = GGT(ports[i].label);
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
		Widget			w_control;
		Widget			CreateCaption();
		void			CBselect(), CBunselect();

		w_control = XtVaCreateManagedWidget(
				"control",
				controlAreaWidgetClass,
				w_parent,
				XtNlayoutType,	(XtArgVal) OL_FIXEDROWS,
				XtNcenter,	(XtArgVal) FALSE,
				XtNalignCaptions,	(XtArgVal) TRUE,
				XtNshadowThickness,	(XtArgVal) 0,
				(String)0
			);
		/*
		 * Add the exclusives area as the caption's child
		 */
		df->w_port = 
			XtVaCreateManagedWidget(
				"ports",
				flatButtonsWidgetClass,
				CreateCaption(w_control, GGT (label_port)),
				XtNlayoutType,	        OL_FIXEDROWS,
				XtNmeasure,	        1,
				XtNitems,               ports,
				XtNnumItems,            NR_PORTS,
				XtNitemFields,          itemFields,
				XtNnumItemFields,       NR_ITEM_FIELDS,
				XtNlabelJustify,        OL_LEFT,
				XtNselectProc,          CBselect,
				XtNunselectProc,        CBunselect,
				XtNbuttonType,		OL_RECT_BTN,
				XtNexclusives,		TRUE,
				0
			);

		df->w_extra = XtVaCreateWidget(
				"extra",
				textFieldWidgetClass,
				w_control,
				XtNcharsVisible, MAXNAMESIZE,
				XtNmaximumSize, MAXNAMESIZE,
				XtNtraversalOn,	True,
				XtNstring, holdData.portNumber,
				(String)0
			);
		return((caddr_t) df->w_port);
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
		String	value = (String) param;
		DeviceItems	*pItem;
		int		index;

		if (strcmp(current->value, value) == 0) {
			return((caddr_t) value);	/* already set */
		}
		if (!strcmp(current->value, "other") &&
		   (strcmp(value, "com1") && strcmp(value, "com2"))) {
			SetValue( df->w_extra, XtNstring, value, NULL);
			return((caddr_t) value);	/* already set */
		}

		if (strcmp(value, "com1") && strcmp(value, "com2"))
			pItem = FindPort("other", ports, NR_PORTS, &index);
		else
			pItem = FindPort(value, ports, NR_PORTS, &index);
		if (pItem != NULL) {
			OlVaFlatSetValues(
				df->w_port,
                		index,   /* sub-item to modify */
				XtNset,     TRUE,
				0
        		);

			current = pItem;		/* remember new */
		}
		if (strcmp(current->value, "other") == 0) {
			if (manage_extra) {
				manage_extra = False;
				XtManageChild(df->w_extra);
			}
			SetValue( df->w_extra, XtNmappedWhenManaged, True, NULL);
			SetValue( df->w_extra, XtNstring, value, NULL);
			OlMoveFocus( df->w_extra, OL_IMMEDIATE, 0);
		} else
			SetValue( df->w_extra, XtNmappedWhenManaged, False, NULL);
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
		if (strcmp(current->value, "other") == 0) {
			if (manage_extra) {
				manage_extra = False;
				XtManageChild(df->w_extra);
			}
	
			SetValue( df->w_extra, XtNmappedWhenManaged, True, NULL);
			SetValue( df->w_extra, XtNstring, "", NULL);
			OlMoveFocus( df->w_extra, OL_IMMEDIATE, 0);
		} else
			SetValue( df->w_extra, XtNmappedWhenManaged, False, NULL);
		return((caddr_t) current);
	}

	default:
		Warning("ObjPortNumber: Unknown message %d\n", message);
		return((caddr_t) NULL);
	}
} /* PortNumber */

static DeviceItems *
FindPort(value, list, count, index)
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
	Warning("FindPort: couldn't find value %s\n", value);
	return(NULL);
} /* FindPort */
