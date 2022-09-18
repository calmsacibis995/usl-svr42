/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:ConstrainP.h	1.1"
/* $XConsortium: ConstrainP.h,v 1.14 89/10/04 12:22:33 swick Exp $ */
/* $oHeader: ConstrainP.h,v 1.2 88/08/18 15:54:15 asente Exp $ */
/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XtConstraintP_h
#define _XtConstraintP_h

#include <X11/Constraint.h>

typedef struct _ConstraintPart {
    XtPointer   mumble;		/* No new fields, keep C compiler happy */
} ConstraintPart;

typedef struct _ConstraintRec {
    CorePart	    core;
    CompositePart   composite;
    ConstraintPart  constraint;
} ConstraintRec, *ConstraintWidget;

typedef struct _ConstraintClassPart {
    XtResourceList resources;	      /* constraint resource list	     */
    Cardinal   num_resources;         /* number of constraints in list       */
    Cardinal   constraint_size;       /* size of constraint record           */
    XtInitProc initialize;            /* constraint initialization           */
    XtWidgetProc destroy;             /* constraint destroy proc             */
    XtSetValuesFunc set_values;       /* constraint set_values proc          */
    XtPointer	    extension;		/* pointer to extension record      */
} ConstraintClassPart;

typedef struct {
    XtPointer next_extension;	/* 1st 4 mandated for all extension records */
    XrmQuark record_type;	/* NULLQUARK; on ConstraintClassPart */
    long version;		/* must be XtConstraintExtensionVersion */
    Cardinal record_size;	/* sizeof(ConstraintClassExtensionRec) */
    XtArgsProc get_values_hook;
} ConstraintClassExtensionRec, *ConstraintClassExtension;

typedef struct _ConstraintClassRec {
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
} ConstraintClassRec;

externalref ConstraintClassRec constraintClassRec;

#define XtConstraintExtensionVersion 1L

#endif /* _XtConstraintP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
