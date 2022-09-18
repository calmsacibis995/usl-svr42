/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:Functions.c	1.1"
/* $XConsortium: Functions.c,v 1.5 91/05/03 15:31:31 rws Exp $ */

/*

Copyright 1985, 1986, 1987, 1988, 1989 by the
Massachusetts Institute of Technology


*/

#include "IntrinsicI.h"
#include <X11/Shell.h>
#include <X11/Vendor.h>

/*
 * This file defines functional equivalents to all macros defined
 * in Intrinsic.h
 *
 */

#undef XtIsRectObj
Boolean XtIsRectObj(object)
    Widget object;
{
    return _XtCheckSubclassFlag(object, 0x02);
}


#undef XtIsWidget
Boolean XtIsWidget(object)
    Widget object;
{
    return _XtCheckSubclassFlag(object, 0x04);
}


#undef XtIsComposite
Boolean XtIsComposite(object)
    Widget object;
{
    return _XtCheckSubclassFlag(object, 0x08);
}


#undef XtIsConstraint
Boolean XtIsConstraint(object)
    Widget object;
{
    return _XtCheckSubclassFlag(object, 0x10);
}


#undef XtIsShell
Boolean XtIsShell(object)
    Widget object;
{
    return _XtCheckSubclassFlag(object, 0x20);
}


#undef XtIsOverrideShell
Boolean XtIsOverrideShell(object)
    Widget object;
{
    return _XtIsSubclassOf(object, (WidgetClass)overrideShellWidgetClass,
			   (WidgetClass)shellWidgetClass, 0x20);
}


#undef XtIsWMShell
Boolean XtIsWMShell(object)
    Widget object;
{
    return _XtCheckSubclassFlag(object, 0x40);
}


#undef XtIsVendorShell
Boolean XtIsVendorShell(object)
    Widget object;
{
#if defined(AIXSHLIB) && defined(SHAREDCODE)
    return _XtIsSubclassOf(object,
			   transientShellWidgetClass->core_class.superclass,
			   (WidgetClass)wmShellWidgetClass, 0x40);
#else
    return _XtIsSubclassOf(object, (WidgetClass)vendorShellWidgetClass,
			   (WidgetClass)wmShellWidgetClass, 0x40);
#endif
}


#undef XtIsTransientShell
Boolean XtIsTransientShell(object)
    Widget object;
{
    return _XtIsSubclassOf(object, (WidgetClass)transientShellWidgetClass,
			   (WidgetClass)wmShellWidgetClass, 0x40);
}


#undef XtIsTopLevelShell
Boolean XtIsTopLevelShell(object)
    Widget object;
{
    return _XtCheckSubclassFlag(object, 0x80);
}


#undef XtIsApplicationShell
Boolean XtIsApplicationShell(object)
    Widget object;
{
    return _XtIsSubclassOf(object, (WidgetClass)applicationShellWidgetClass,
			   (WidgetClass)topLevelShellWidgetClass, 0x80);
}


#undef XtMapWidget
void XtMapWidget(w)
    Widget w;
{
    XMapWindow(XtDisplay(w), XtWindow(w));
}


#undef XtUnmapWidget
void XtUnmapWidget(w)
    Widget w;
{
    XUnmapWindow(XtDisplay(w), XtWindow(w));
}


#undef XtNewString
String XtNewString(str)
    String str;
{
    if (str == NULL)
	return NULL;
    else
	return strcpy(XtMalloc((unsigned)strlen(str) + 1), str);
}
