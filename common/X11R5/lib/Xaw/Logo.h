/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Logo.h	1.2"
/*
* $XConsortium: Logo.h,v 1.11 90/10/22 14:45:11 converse Exp $
*/

/*
Copyright 1988 by the Massachusetts Institute of Technology

*/

#ifndef _XawLogo_h
#define _XawLogo_h

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 background	     Background		Pixel		XtDefaultBackground
 border		     BorderColor	Pixel		XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	1
 destroyCallback     Callback		Pointer		NULL
 foreground	     Foreground		Pixel		XtDefaultForeground
 height		     Height		Dimension	100
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 shapeWindow	     ShapeWindow	Boolean		False
 width		     Width		Dimension	100
 x		     Position		Position	0
 y		     Position		Position	0

*/

#define XtNshapeWindow "shapeWindow"
#define XtCShapeWindow "ShapeWindow"

typedef struct _LogoRec *LogoWidget;
typedef struct _LogoClassRec *LogoWidgetClass;

extern WidgetClass logoWidgetClass;

#endif /* _XawLogo_h */
