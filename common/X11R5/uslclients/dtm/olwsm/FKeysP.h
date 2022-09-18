/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/FKeysP.h	1.4"
#endif

#ifndef _OL_FKEYSP_H
#define _OL_FKEYSP_H

#include <Xol/FRowColumP.h>	/* superclass' header */
#include "FKeys.h"		/* public header */
#include <Xol/ChangeBar.h>

/*
 * Widget class record:
 */

typedef struct {
    char no_class_fields;		/* Makes compiler happy */
}			FlatKeysClassPart;

typedef struct _FlatKeysClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	FlatClassPart		flat_class;
	FlatRowColumnClassPart	row_column_class;
	FlatKeysClassPart	keys_class;
}			FlatKeysClassRec;

extern FlatKeysClassRec	flatKeysClassRec;

/*
 * Widget sub-object record:
 */

typedef struct {
	Boolean			is_header;
	OlDefine		change_bar;
	String			caption;
	Modifiers		modifiers;
	KeySym			keysym;
}			FlatKeysItemPart;

typedef struct {
	FlatItemPart		flat;
	FlatRowColumnItemPart	row_column;
	FlatKeysItemPart	keys;
}			FlatKeysItemRec;

typedef FlatKeysItemRec * FlatKeysItem;

/*
 * Widget instance record:
 */

typedef struct {
	/*
	 * Public:
	 */
	Boolean			allow_change_bars;
	XFontStruct *		caption_font;
	Dimension		caption_gap;
	XtCallbackList		key_changed;

	/*
	 * Private:
	 */
	XModifierKeymap *	modifiers_map;
	Dimension		caption_width;
	Dimension		modifiers_width;
	Dimension		detail_width;
	KeyCode			consume_keycode;
	ChangeBar *		cb;
	Cardinal		quoting;
	Cardinal		old_quoting;
	Modifiers		old_modifiers;
	KeySym			old_keysym;
	unsigned int		flags;
	GC			caption_GC;
	/*
	 * WARNING: Don't forget to initialize new private
	 * fields in the Initialize() method!!!
	 */
}			FlatKeysPart;
#define _FLATKEYS_SET_MAX_WIDTHS	0x0001

typedef struct _FlatKeysRec {
	CorePart		core;
	PrimitivePart		primitive;
	FlatPart		flat;
	FlatRowColumnPart	row_column;
	FlatKeysPart		keys;

			/* Hide the default item in the widget instance */
	FlatKeysItemRec		default_item;
}			FlatKeysRec;

#endif /* _OL_FKEYSP_H */
