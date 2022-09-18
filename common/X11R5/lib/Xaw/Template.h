/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Template.h	1.2"
/* $XConsortium: Template.h,v 1.5 90/12/19 18:46:00 converse Exp $ */

/* Copyright	Massachusetts Institute of Technology	1987, 1988
 *
 *
 *
 */

#ifndef _Template_h
#define _Template_h

/****************************************************************
 *
 * Template widget
 *
 ****************************************************************/

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 background	     Background		Pixel		XtDefaultBackground
 border		     BorderColor	Pixel		XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	1
 destroyCallback     Callback		Pointer		NULL
 height		     Height		Dimension	0
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	0
 x		     Position		Position	0
 y		     Position		Position	0

*/

/* define any special resource names here that are not in <X11/StringDefs.h> */

#define XtNtemplateResource "templateResource"

#define XtCTemplateResource "TemplateResource"

/* declare specific TemplateWidget class and instance datatypes */

typedef struct _TemplateClassRec*	TemplateWidgetClass;
typedef struct _TemplateRec*		TemplateWidget;

/* declare the class constant */

extern WidgetClass templateWidgetClass;

#endif /* _Template_h */
