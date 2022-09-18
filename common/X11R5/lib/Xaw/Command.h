/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Command.h	1.2"
/* $XConsortium: Command.h,v 1.27 91/07/28 18:51:35 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XawCommand_h
#define _XawCommand_h

#include <X11/Xaw/Label.h>

/* Command widget resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 accelerators	     Accelerators	AcceleratorTable NULL
 ancestorSensitive   AncestorSensitive	Boolean		True
 background	     Background		Pixel		XtDefaultBackground
 backgroundPixmap    Pixmap		Pixmap		XtUnspecifiedPixmap
 bitmap		     Pixmap		Pixmap		None
 borderColor	     BorderColor	Pixel		XtDefaultForeground
 borderPixmap	     Pixmap		Pixmap		XtUnspecifiedPixmap
 borderWidth	     BorderWidth	Dimension	1
 callback	     Callback		XtCallbackList	NULL
 colormap	     Colormap		Colormap	parent's colormap
 cornerRoundPercent  CornerRoundPercent	Dimension	25
 cursor		     Cursor		Cursor		None
 cursorName	     Cursor		String		NULL
 depth		     Depth		int		parent's depth
 destroyCallback     Callback		XtCallbackList	NULL
 encoding	     Encoding		UnsignedChar	XawTextEncoding8bit
 font		     Font		XFontStruct*	XtDefaultFont
 foreground	     Foreground		Pixel		XtDefaultForeground
 height		     Height		Dimension	text height
 highlightThickness  Thickness		Dimension	0 if shaped, else 2
 insensitiveBorder   Insensitive	Pixmap		Gray
 internalHeight	     Height		Dimension	2
 internalWidth	     Width		Dimension	4
 justify	     Justify		XtJustify	XtJustifyCenter
 label		     Label		String		NULL
 leftBitmap	     LeftBitmap		Pixmap		None
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 pointerColor	     Foreground		Pixel		XtDefaultForeground
 pointerColorBackground Background	Pixel		XtDefaultBackground
 resize		     Resize		Boolean		True
 screen		     Screen		Screen		parent's Screen
 sensitive	     Sensitive		Boolean		True
 shapeStyle	     ShapeStyle		ShapeStyle	Rectangle
 translations	     Translations	TranslationTable see doc or source
 width		     Width		Dimension	text width
 x		     Position		Position	0
 y		     Position		Position	0

*/

#define XtNhighlightThickness "highlightThickness"

#define XtNshapeStyle "shapeStyle"
#define XtCShapeStyle "ShapeStyle"
#define XtRShapeStyle "ShapeStyle"
#define XtNcornerRoundPercent "cornerRoundPercent"
#define XtCCornerRoundPercent "CornerRoundPercent"

#define XawShapeRectangle XmuShapeRectangle
#define XawShapeOval XmuShapeOval
#define XawShapeEllipse XmuShapeEllipse
#define XawShapeRoundedRectangle XmuShapeRoundedRectangle

extern WidgetClass     commandWidgetClass;

typedef struct _CommandClassRec   *CommandWidgetClass;
typedef struct _CommandRec        *CommandWidget;

#endif /* _XawCommand_h */
/* DON'T ADD STUFF AFTER THIS */
