/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:SmeBSB.h	1.2"
/*
 * $XConsortium: SmeBSB.h,v 1.5 89/12/11 15:20:14 kit Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */

/*
 * SmeBSB.h - Public Header file for SmeBSB object.
 *
 * This is the public header file for the Athena BSB Sme object.
 * It is intended to be used with the simple menu widget.  This object
 * provides bitmap - string - bitmap style entries.
 *
 * Date:    April 3, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _SmeBSB_h
#define _SmeBSB_h

#include <X11/Xmu/Converters.h>

#include <X11/Xaw/Sme.h>

/****************************************************************
 *
 * SmeBSB object
 *
 ****************************************************************/

/* BSB Menu Entry Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 callback            Callback           Callback        NULL
 destroyCallback     Callback		Pointer		NULL
 font                Font               XFontStruct *   XtDefaultFont
 foreground          Foreground         Pixel           XtDefaultForeground
 height		     Height		Dimension	0
 label               Label              String          Name of entry
 leftBitmap          LeftBitmap         Pixmap          None
 leftMargin          HorizontalMargins  Dimension       4
 rightBitmap         RightBitmap        Pixmap          None
 rightMargin         HorizontalMargins  Dimension       4
 sensitive	     Sensitive		Boolean		True
 vertSpace           VertSpace          int             25
 width		     Width		Dimension	0
 x		     Position		Position	0n
 y		     Position		Position	0

*/

typedef struct _SmeBSBClassRec    *SmeBSBObjectClass;
typedef struct _SmeBSBRec         *SmeBSBObject;

extern WidgetClass smeBSBObjectClass;

#define XtNleftBitmap "leftBitmap"
#define XtNleftMargin "leftMargin"
#define XtNrightBitmap "rightBitmap"
#define XtNrightMargin "rightMargin"
#define XtNvertSpace   "vertSpace"

#define XtCLeftBitmap "LeftBitmap"
#define XtCHorizontalMargins "HorizontalMargins"
#define XtCRightBitmap "RightBitmap"
#define XtCVertSpace   "VertSpace"

#endif /* _SmeBSB_h */
