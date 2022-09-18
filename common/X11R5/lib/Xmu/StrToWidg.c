/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StrToWidg.c	1.2"
/* $XConsortium: StrToWidg.c,v 1.8 91/07/23 15:19:48 converse Exp $ */

/* Copyright 1988, 1991 Massachusetts Institute of Technology
 *
 *
 *
 *
 * XmuCvtStringToWidget
 *
 *   static XtConvertArgRec parentCvtArgs[] = {
 *	{XtBaseOffset, (caddr_t)XtOffset(Widget, core.parent), sizeof(Widget)},
 *   };
 *
 * matches the string against the name of the immediate children (normal
 * or popup) of the parent.  If none match, compares string to classname
 * & returns first match.  Case is significant.
 */
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ObjectP.h>

#define	done(address, type) \
	{ toVal->size = sizeof(type); \
	  toVal->addr = (XPointer) address; \
	  return; \
	}

/* ARGSUSED */
void XmuCvtStringToWidget(args, num_args, fromVal, toVal)
    XrmValuePtr args;		/* parent */
    Cardinal    *num_args;      /* 1 */
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
{
    static Widget widget, *widgetP, parent;
    XrmName name = XrmStringToName(fromVal->addr);
    int i;

    if (*num_args != 1)
	XtErrorMsg("wrongParameters", "cvtStringToWidget", "xtToolkitError",
		   "StringToWidget conversion needs parent arg", NULL, 0);

    parent = *(Widget*)args[0].addr;
    /* try to match names of normal children */
    if (XtIsComposite(parent)) {
	i = ((CompositeWidget)parent)->composite.num_children;
	for (widgetP = ((CompositeWidget)parent)->composite.children;
	     i; i--, widgetP++) {
	    if ((*widgetP)->core.xrm_name == name) {
		widget = *widgetP;
		done(&widget, Widget);
	    }
	}
    }
    /* try to match names of popup children */
    i = parent->core.num_popups;
    for (widgetP = parent->core.popup_list; i; i--, widgetP++) {
	if ((*widgetP)->core.xrm_name == name) {
	    widget = *widgetP;
	    done(&widget, Widget);
	}
    }
    /* try to match classes of normal children */
    if (XtIsComposite(parent)) {
	i = ((CompositeWidget)parent)->composite.num_children;
	for (widgetP = ((CompositeWidget)parent)->composite.children;
	     i; i--, widgetP++) {
	    if ((*widgetP)->core.widget_class->core_class.xrm_class == name) {
		widget = *widgetP;
		done(&widget, Widget);
	    }
	}
    }
    /* try to match classes of popup children */
    i = parent->core.num_popups;
    for (widgetP = parent->core.popup_list; i; i--, widgetP++) {
	if ((*widgetP)->core.widget_class->core_class.xrm_class == name) {
	    widget = *widgetP;
	    done(&widget, Widget);
	}
    }
    XtStringConversionWarning(fromVal->addr, XtRWidget);
    toVal->addr = NULL;
    toVal->size = 0;
}

#undef done

#define	newDone(type, value) \
	{							\
	    if (toVal->addr != NULL) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (XtPointer)&static_val;		\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}


/*ARGSUSED*/
Boolean XmuNewCvtStringToWidget(dpy, args, num_args, fromVal, toVal, 
				converter_data)
     Display *dpy;
     XrmValue *args;		/* parent */
     Cardinal *num_args;	/* 1 */
     XrmValue *fromVal;
     XrmValue *toVal;
     XtPointer *converter_data;
{
    Widget *widgetP, parent;
    XrmName name = XrmStringToName(fromVal->addr);
    int i;

    if (*num_args != 1)
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
			"wrongParameters","cvtStringToWidget","xtToolkitError",
			"String To Widget conversion needs parent argument",
			(String *)NULL, (Cardinal *)NULL);

    parent = *(Widget*)args[0].addr;
    /* try to match names of normal children */
    if (XtIsComposite(parent)) {
	i = ((CompositeWidget)parent)->composite.num_children;
	for (widgetP = ((CompositeWidget)parent)->composite.children;
	     i; i--, widgetP++) {
	    if ((*widgetP)->core.xrm_name == name)
		newDone(Widget, *widgetP);
	}
    }
    /* try to match names of popup children */
    i = parent->core.num_popups;
    for (widgetP = parent->core.popup_list; i; i--, widgetP++) {
	if ((*widgetP)->core.xrm_name == name)
	    newDone(Widget, *widgetP);
    }
    /* try to match classes of normal children */
    if (XtIsComposite(parent)) {
	i = ((CompositeWidget)parent)->composite.num_children;
	for (widgetP = ((CompositeWidget)parent)->composite.children;
	     i; i--, widgetP++) {
	    if ((*widgetP)->core.widget_class->core_class.xrm_class == name)
		newDone(Widget, *widgetP);
	}
    }
    /* try to match classes of popup children */
    i = parent->core.num_popups;
    for (widgetP = parent->core.popup_list; i; i--, widgetP++) {
	if ((*widgetP)->core.widget_class->core_class.xrm_class == name)
	    newDone(Widget, *widgetP);
    }
    XtDisplayStringConversionWarning(dpy, (String)fromVal->addr, XtRWidget);
    return False;
}
