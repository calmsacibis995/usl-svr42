/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Vendor.c	1.2"
/* $XConsortium: Vendor.c,v 1.21 91/07/30 15:29:56 rws Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/*
 * This is a copy of Xt/Vendor.c with an additional ClassInitialize
 * procedure to register Xmu resource type converters.
 *
 */

/* Make sure all wm properties can make it out of the resource manager */

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <X11/Vendor.h>
#include <X11/VendorP.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/Editres.h>

static XtResource resources[] = {
  {XtNinput, XtCInput, XtRBool, sizeof(Bool),
		XtOffsetOf(VendorShellRec, wm.wm_hints.input),
		XtRImmediate, (XtPointer)True}
};

/***************************************************************************
 *
 * Vendor shell class record
 *
 ***************************************************************************/

static void XawVendorShellClassInitialize();
static void XawVendorShellInitialize();
static void ChangeManaged();

#define SuperClass (&wmShellClassRec)
externaldef(vendorshellclassrec) VendorShellClassRec vendorShellClassRec = {
  {
    /* superclass	  */	(WidgetClass)SuperClass,
    /* class_name	  */	"VendorShell",
    /* size		  */	sizeof(VendorShellRec),
    /* class_initialize	  */	XawVendorShellClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed ?	  */	FALSE,
    /* initialize	  */	XawVendorShellInitialize,
    /* initialize_hook	  */	NULL,		
    /* realize		  */	XtInheritRealize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources	  */	resources,
    /* resource_count	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	FALSE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave*/	FALSE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	NULL,
    /* resize		  */	XtInheritResize,
    /* expose		  */	NULL,
    /* set_values	  */	NULL,
    /* set_values_hook	  */	NULL,			
    /* set_values_almost  */	XtInheritSetValuesAlmost,  
    /* get_values_hook	  */	NULL,
    /* accept_focus	  */	NULL,
    /* intrinsics version */	XtVersion,
    /* callback offsets	  */	NULL,
    /* tm_table		  */	NULL,
    /* query_geometry	  */	NULL,
    /* display_accelerator*/	NULL,
    /* extension	  */	NULL
  },{
    /* geometry_manager	  */	XtInheritGeometryManager,
    /* change_managed	  */	ChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */	NULL
  },{
    /* extension	  */	NULL
  },{
    /* extension	  */	NULL
  },{
    /* extension	  */	NULL
  }
};

externaldef(vendorshellwidgetclass) WidgetClass vendorShellWidgetClass =
	(WidgetClass) (&vendorShellClassRec);

static void XawVendorShellClassInitialize()
{
    static XtConvertArgRec screenConvertArg[] = {
        {XtWidgetBaseOffset, (XtPointer) XtOffsetOf(WidgetRec, core.screen),
	     sizeof(Screen *)}
    };

    XtAddConverter(XtRString, XtRCursor, XmuCvtStringToCursor,      
		   screenConvertArg, XtNumber(screenConvertArg));

    XtAddConverter(XtRString, XtRBitmap, XmuCvtStringToBitmap,
		   screenConvertArg, XtNumber(screenConvertArg));
}

/* ARGSUSED */
static void XawVendorShellInitialize(req, new)
	Widget req, new;
{
    XtAddEventHandler(new, (EventMask) 0, TRUE, _XEditResCheckMessages, NULL);
}

static void ChangeManaged(wid)
	Widget wid;
{
	ShellWidget w = (ShellWidget) wid;
	Widget* childP;
	int i;

	(*SuperClass->composite_class.change_managed)(wid);
	for (i = w->composite.num_children, childP = w->composite.children;
	     i; i--, childP++) {
	    if (XtIsManaged(*childP)) {
		XtSetKeyboardFocus(wid, *childP);
		break;
	    }
	}
}
