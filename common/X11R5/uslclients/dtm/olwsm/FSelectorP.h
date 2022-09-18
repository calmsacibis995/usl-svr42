/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/FSelectorP.h	1.4"
#endif

#ifndef _OL_FSELECTORP_H
#define _OL_FSELECTORP_H

#include <Xol/FButtonsP.h>	/* superclass' header */
#include "FSelector.h"		/* public header */

#include <Xol/ChangeBar.h>


/*
 * Class structure:
 */

typedef struct _FlatSelectorClassPart {
    char no_class_fields;			/* Makes compiler happy */
}			FlatSelectorClassPart;

typedef struct _FlatSelectorClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	FlatClassPart		flat_class;
	FlatRowColumnPart	row_column_class;
	FlatButtonsClassPart	exclusives_class;
	FlatSelectorClassPart	selector_class;
}			FlatSelectorClassRec;

extern FlatSelectorClassRec	flatSelectorClassRec;

/*
 * Item structure:
 */

typedef struct _FlatSelectorItemPart {
	OlDefine		change_bar;
	Dimension		chip_height;
	Dimension		chip_width;
	Dimension		chip_space;
	Pixel			chip_color;
}			FlatSelectorItemPart;

typedef struct _FlatSelectorItemRec {
	FlatItemPart		flat;
	FlatRowColumnItemPart	row_column;
	FlatButtonsItemPart	exclusives;
	FlatSelectorItemPart	selector;
}			FlatSelectorItemRec, *FlatSelectorItem;

/*
 * Instance structure:
 */

typedef struct _FlatSelectorPart {
	/*
	 * Public:
	 */
	Boolean			allow_change_bars;

	/*
	 * Private:
	 */
	ChangeBar *		cb;
}			FlatSelectorPart;

typedef struct _FlatSelectorRec {
	CorePart		core;
	PrimitivePart		primitive;
	FlatPart		flat;
	FlatRowColumnPart	row_column;
	FlatButtonsPart		exclusives;
	FlatSelectorPart	selector;

			/* Hide the default item in the widget instance */
	FlatSelectorItemRec	default_item;
}			FlatSelectorRec;

#endif /* _OL_FSELECTORP_H */
