/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:GetResList.c	1.1"
/* $XConsortium: GetResList.c,v 1.2 91/01/06 13:32:20 rws Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "IntrinsicI.h"

/*
 * XtGetResourceList(), XtGetConstraintResourceList()
 */

void XtGetResourceList(widget_class, resources, num_resources)
	WidgetClass widget_class;
	XtResourceList *resources;
	Cardinal *num_resources;
{
	int size = widget_class->core_class.num_resources * sizeof(XtResource);
	register int i, dest = 0;
	register XtResourceList *list, dlist;

	*resources = (XtResourceList) XtMalloc((unsigned) size);

	if (!widget_class->core_class.class_inited) {
	    /* Easy case */

	    bcopy((char *)widget_class->core_class.resources,
		    (char *) *resources, size);
	    *num_resources = widget_class->core_class.num_resources;
	    return;
	}

	/* Nope, it's the hard case */

	list = (XtResourceList *) widget_class->core_class.resources;
	dlist = *resources;
	for (i = 0; i < widget_class->core_class.num_resources; i++) {
	    if (list[i] != NULL) {
		dlist[dest].resource_name = (String)
			XrmQuarkToString((XrmQuark) list[i]->resource_name);
		dlist[dest].resource_class = (String) 
			XrmQuarkToString((XrmQuark) list[i]->resource_class);
		dlist[dest].resource_type = (String)
			XrmQuarkToString((XrmQuark) list[i]->resource_type);
		dlist[dest].resource_size = list[i]->resource_size;
		dlist[dest].resource_offset = -(list[i]->resource_offset + 1);
		dlist[dest].default_type = (String)
			XrmQuarkToString((XrmQuark) list[i]->default_type);
		dlist[dest].default_addr = list[i]->default_addr;
		dest++;
	    }
	}
	*num_resources = dest;
}


static Boolean ClassIsSubclassOf(class, superclass)
    WidgetClass class, superclass;
{
    for (; class != NULL; class = class->core_class.superclass) {
	if (class == superclass) return True;
    }
    return False;
}

void XtGetConstraintResourceList(widget_class, resources, num_resources)
	WidgetClass widget_class;
	XtResourceList *resources;
	Cardinal *num_resources;
{
	int size;
	register int i, dest = 0;
	register XtResourceList *list, dlist;
	ConstraintWidgetClass class = (ConstraintWidgetClass)widget_class;

	if (   (class->core_class.class_inited &&
		!(class->core_class.class_inited & ConstraintClassFlag))
	    || (!class->core_class.class_inited &&
		!ClassIsSubclassOf(widget_class, constraintWidgetClass))
	    || class->constraint_class.num_resources == 0) {

	    *resources = NULL;
	    *num_resources = 0;
	    return;
	}

	size = class->constraint_class.num_resources * sizeof(XtResource);
	*resources = (XtResourceList) XtMalloc((unsigned) size);

	if (!class->core_class.class_inited) {
	    /* Easy case */

	    bcopy((char *)class->constraint_class.resources,
		    (char *) *resources, size);
	    *num_resources = class->constraint_class.num_resources;
	    return;
	}

	/* Nope, it's the hard case */

	list = (XtResourceList *) class->constraint_class.resources;
	dlist = *resources;
	for (i = 0; i < class->constraint_class.num_resources; i++) {
	    if (list[i] != NULL) {
		dlist[dest].resource_name = (String)
			XrmQuarkToString((XrmQuark) list[i]->resource_name);
		dlist[dest].resource_class = (String) 
			XrmQuarkToString((XrmQuark) list[i]->resource_class);
		dlist[dest].resource_type = (String)
			XrmQuarkToString((XrmQuark) list[i]->resource_type);
		dlist[dest].resource_size = list[i]->resource_size;
		dlist[dest].resource_offset = -(list[i]->resource_offset + 1);
		dlist[dest].default_type = (String)
			XrmQuarkToString((XrmQuark) list[i]->default_type);
		dlist[dest].default_addr = list[i]->default_addr;
		dest++;
	    }
	}
	*num_resources = dest;
}
