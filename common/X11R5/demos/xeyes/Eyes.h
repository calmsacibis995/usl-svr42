/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xeyes:Eyes.h	1.1"
/*
* $XConsortium: Eyes.h,v 1.7 90/12/01 13:04:51 rws Exp $
*/

#ifndef _XtEyes_h
#define _XtEyes_h

/***********************************************************************
 *
 * Eyes Widget
 *
 ***********************************************************************/

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 background	     Background		pixel		White
 border		     BorderColor	pixel		Black
 borderWidth	     BorderWidth	int		1
 foreground	     Foreground		Pixel		Black
 outline	     Outline		Pixel		Black
 height		     Height		int		120
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 reverseVideo	     ReverseVideo	Boolean		False
 width		     Width		int		120
 x		     Position		int		0
 y		     Position		int		0

*/

#define XtNoutline	"outline"
#define XtNcenterColor	"center"

#define XtNshapeWindow	"shapeWindow"
#define XtCShapeWindow	"ShapeWindow"

typedef struct _EyesRec *EyesWidget;  /* completely defined in EyesPrivate.h */
typedef struct _EyesClassRec *EyesWidgetClass;    /* completely defined in EyesPrivate.h */

extern WidgetClass eyesWidgetClass;

#endif /* _XtEyes_h */
/* DON'T ADD STUFF AFTER THIS #endif */
