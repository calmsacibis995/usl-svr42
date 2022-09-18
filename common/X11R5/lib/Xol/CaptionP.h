/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)caption:CaptionP.h	2.3"
#endif

#ifndef _CAPTIONP_H
#define _CAPTIONP_H

#include "Xol/ManagerP.h"
#include "Xol/Caption.h"
#include "Xol/LayoutExtP.h"
#include "Xol/Olg.h"

/*
 * Class structure:
 */

typedef struct _CaptionClassPart {
	XtPointer		extension;
}			CaptionClassPart;

typedef struct _CaptionClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	CaptionClassPart	caption_class;
}			CaptionClassRec;

extern CaptionClassRec	captionClassRec;

#define CAPTION_C(WC) ((CaptionWidgetClass)(WC))->caption_class

/*
 * Instance structure:
 */

typedef struct _CaptionPart {
	/*
	 * Public:
	 */
	Pixel			font_color;
	XFontStruct *		font;
#if	defined(I18N)
	OlFontList *		font_list;
#endif
	String			label;
	XImage *		label_image;
	Pixmap			label_pixmap;
	OlDefine		position;
	OlDefine		alignment;
	Dimension		space;
	OlMnemonic		mnemonic;
	OlLayoutResources	layout;

	/*
	 * Private:
	 */
	GC			normal_gc;
	GC			inverse_gc;
	OlgAttrs *		attrs;
	XRectangle		label_geometry;
	unsigned char		dynamics;
}			CaptionPart;

#define _CAPTION_B_DYNAMIC_FONTCOLOR	0x001
#define _CAPTION_B_DYNAMIC_FONT		0x002

typedef struct _CaptionRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	CaptionPart		caption;
}			CaptionRec;

#define CAPTION_P(W) ((CaptionWidget)(W))->caption

#endif
