/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Form.c	1.2"
#endif

/*  Example of the Form widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>
#include <Xol/OblongButt.h>
#include <Xol/Form.h>

#include "WidgetTree.h"

/*ARGSUSED*/
void
FormCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, caption, form, ok;

        popup = XtVaCreatePopupShell("transientShell",
                transientShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: Form",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, popup,
                XtNlabel, "Form:",
		XtNposition, OL_TOP,
		XtNalignment, OL_LEFT,
                (String) 0);

        form = XtVaCreateManagedWidget("form", formWidgetClass, caption,
		XtNwidth, 150,
		XtNheight, 150,
		XtNshadowThickness, 0,
                (String) 0);

        XtVaCreateManagedWidget("AttachLeft", staticTextWidgetClass, form,
                XtNstring, "Attach Left",
		XtNyOffset, 50,
		(String) 0);

        XtVaCreateManagedWidget("AttachTop", staticTextWidgetClass, form,
                XtNstring, "Attach Top",
		XtNxRefName, "AttachLeft",
		XtNxAddWidth, True,
		(String) 0);

        XtVaCreateManagedWidget("AttachRight", staticTextWidgetClass, form,
                XtNstring, "Attach Right",
		XtNxAttachRight, True,
		XtNxAddWidth, True,
		XtNxVaryOffset, True,
		XtNyOffset, 50,
		XtNxRefName, "AttachTop",
		(String) 0);

        ok = XtVaCreateManagedWidget("OK", oblongButtonGadgetClass, form,
		XtNyAttachBottom, True,
		XtNyRefName, "AttachTop",
		XtNyAddHeight, True,
		XtNyVaryOffset, True,
		XtNxOffset, 85,
		(String) 0);
	XtAddCallback(ok, XtNselect, PopdownCB, popup);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "Form.c");

        XtPopup(popup, XtGrabNone);
}  /* end of FormCB() */
