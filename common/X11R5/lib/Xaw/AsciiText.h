/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:AsciiText.h	1.2"
/*
 * $XConsortium: AsciiText.h,v 1.16 91/01/21 12:39:04 swick Exp $ 
 */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/***********************************************************************
 *
 * AsciiText Widget
 *
 ***********************************************************************/

/*
 * AsciiText.c - Public header file for AsciiText Widget.
 *
 * This Widget is intended to be used as a simple front end to the 
 * text widget with an ascii source and ascii sink attached to it.
 *
 * Date:    June 29, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _AsciiText_h
#define _AsciiText_h

/****************************************************************
 *
 * AsciiText widgets
 *
 ****************************************************************/

#include <X11/Xaw/Text.h>		/* AsciiText is a subclass of Text */
#include <X11/Xaw/AsciiSrc.h>

/* Resources:

 Name		     Class		RepType		  Default Value
 ----		     -----		-------		  -------------
 autoFill	     AutoFill		Boolean		  False
 background	     Background		Pixel		  XtDefaultBackground
 border		     BorderColor	Pixel		  XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	  1
 bottomMargin	     Margin		Position	  2
 cursor		     Cursor		Cursor		  xterm
 destroyCallback     Callback		Pointer		  NULL
 displayCaret	     Output		Boolean		  True
 displayPosition     TextPosition	int		  0
 editType	     EditType		XawTextEditType	  XawtextRead
 font		     Font		XFontStruct*	  Fixed
 foreground	     Foreground		Pixel		  Black
 height		     Height		Dimension	  font height
 insertPosition	     TextPosition	int		  0
 leftMargin	     Margin		Position	  2
 mappedWhenManaged   MappedWhenManaged	Boolean		  True
 resize		     Resize		XawTextResizeMode XawtextResizeNever
 rightMargin	     Margin		Position	  4
 scrollHorizontal    Scroll		XawTextScrollMode XawtextScrollNever
 scrollVertical	     Scroll		XawTextScrollMode XawtextScrollNever
 selectTypes	     SelectTypes	Pointer		  pos/word/line/par/all
 selection	     Selection		Pointer		  (empty selection)
 sensitive	     Sensitive		Boolean		  True
 sink		     TextSink		Widget		  (none)
 source		     TextSource		Widget		  (none)
 string		     String		String		  NULL
 topMargin	     Margin		Position	  2
 width		     Width		Dimension	  100
 wrap		     Wrap		XawTextWrapMode	  XawtextWrapNever
 x		     Position		Position	  0
 y		     Position		Position	  0

 (see also *Src.h and *Sink.h)
*/

/*
 * Everything else we need is in StringDefs.h or Text.h
 */

typedef struct _AsciiTextClassRec	*AsciiTextWidgetClass;
typedef struct _AsciiRec	        *AsciiWidget;

extern WidgetClass asciiTextWidgetClass;

/************************************************************
 *
 * Disk and String Emulation Info.
 * 
 ************************************************************/

#ifdef ASCII_STRING
extern WidgetClass asciiStringWidgetClass;
#endif

#ifdef ASCII_DISK
extern WidgetClass asciiDiskWidgetClass;
#endif

#endif /* _AsciiText_h */
