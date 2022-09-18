/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:LogoP.h	1.2"
/*
* $XConsortium: LogoP.h,v 1.9 90/10/22 14:45:51 converse Exp $
*/

/*
Copyright 1988 by the Massachusetts Institute of Technology

*/

#ifndef _XawLogoP_h
#define _XawLogoP_h

#include <X11/Xaw/Logo.h>
#include <X11/Xaw/SimpleP.h>

typedef struct {
	 Pixel	 fgpixel;
	 GC	 foreGC;
	 GC	 backGC;
	 Boolean shape_window;
	 Boolean need_shaping;
   } LogoPart;

typedef struct _LogoRec {
   CorePart core;
   SimplePart simple;
   LogoPart logo;
   } LogoRec;

typedef struct {int dummy;} LogoClassPart;

typedef struct _LogoClassRec {
   CoreClassPart core_class;
   SimpleClassPart simple_class;
   LogoClassPart logo_class;
   } LogoClassRec;

extern LogoClassRec logoClassRec;

#endif /* _XawLogoP_h */
