/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:StripChart.h	1.2"
/* $XConsortium: StripChart.h,v 1.5 91/07/26 22:50:35 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XawStripChart_h
#define _XawStripChart_h

/***********************************************************************
 *
 * StripChart Widget
 *
 ***********************************************************************/

/* StripChart resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 accelerators	     Accelerators	AcceleratorTable NULL
 ancestorSensitive   AncestorSensitive	Boolean		True 
 background	     Background		Pixel		XtDefaultBackground
 backgroundPixmap    Pixmap		Pixmap		XtUnspecifiedPixmap
 borderColor	     BorderColor	Pixel		XtDefaultForeground
 borderPixmap	     Pixmap		Pixmap		XtUnspecifiedPixmap
 borderWidth	     BorderWidth	Dimension	1
 colormap	     Colormap		Colormap	parent's colormap
 cursor		     Cursor		Cursor		None
 cursorName	     Cursor		String		NULL
 depth		     Depth		int		parent's depth
 destroyCallback     Callback		XtCallbackList	NULL
 foreground	     Foreground		Pixel		XtDefaultForeground
 getValue	     Callback		XtCallbackList	NULL
 height		     Height		Dimension	120
 highlight	     Foreground		Pixel		XtDefaultForeground
 insensitiveBorder   Insensitive	Pixmap		GreyPixmap
 jumpScroll	     JumpScroll		int		1/2 width
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 minScale	     Scale		int		1
 pointerColor	     Foreground		Pixel		XtDefaultForeground
 pointerColorBackground Background	Pixel		XtDefaultBackground
 screen		     Screen		Screen		parent's screen
 sensitive	     Sensitive		Boolean		True
 translations	     Translations	TranslationTable NULL
 update		     Interval		int		10 (seconds)
 width		     Width		Dimension	120
 x		     Position		Position	0
 y		     Position		Position	0

*/

#define DEFAULT_JUMP -1

#ifndef _XtStringDefs_h_
#define XtNhighlight "highlight"
#define XtNupdate "update"
#endif

#define XtCJumpScroll "JumpScroll"
#define XtCScale "Scale"

#define XtNgetValue "getValue"
#define XtNjumpScroll "jumpScroll"
#define XtNminScale "minScale"
#define XtNscale "scale"
#define XtNvmunix "vmunix"
 
typedef struct _StripChartRec *StripChartWidget;
typedef struct _StripChartClassRec *StripChartWidgetClass;

extern WidgetClass stripChartWidgetClass;

#endif /* _XawStripChart_h */
/* DON'T ADD STUFF AFTER THIS #endif */
