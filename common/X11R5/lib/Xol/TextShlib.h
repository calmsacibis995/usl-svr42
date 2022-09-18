/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)shlib:TextShlib.h	1.2"
#endif

#ifndef _XtAtom_h_
#define _XtAtom_h_
/*
 * Define _XtAtom_h_ here to prevent the real Intrinsics StringDefs.h
 * header from being pulled into the TextEdit Widget's source file.
 * This is needed for C files that have "Too Much Defining" problems.
 */

#define XtNbackground		"background"
#define XtNbackgroundPixmap     "backgroundPixmap"
#define XtNeditType		"editType"
#define XtNfont			"font"
#define XtNheight		"height"
#define XtNselection		"selection"
#define XtNselectionArray	"selectionArray"
#define XtNwidth		"width"
#define XtCCallback             "Callback"
#define XtCEditType		"EditType"
#define XtCFont			"Font"
#define XtCMargin		"Margin"
#define XtCTextPosition		"TextPosition"
#define XtRBool			"Bool"
#define XtRCallback             "Callback"
#define XtRDimension		"Dimension"
#define XtRFontStruct		"FontStruct"
#define XtRFunction		"Function"
#define XtRInt			"Int"
#define XtRLongBoolean		XtRBool		/* Compatibility */
#define XtRPixel		"Pixel"
#define XtRPointer		"Pointer"
#define XtRString		"String"
#define XtRStringTable		"StringTable"

#endif /* _XtAtom_h_ */
