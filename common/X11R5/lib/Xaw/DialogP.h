/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:DialogP.h	1.2"
/* $XConsortium: DialogP.h,v 1.12 89/08/25 18:35:37 kit Exp $ */


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* Private definitions for Dialog widget */

#ifndef _DialogP_h
#define _DialogP_h

#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/FormP.h>

typedef struct {int empty;} DialogClassPart;

typedef struct _DialogClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    FormClassPart	form_class;
    DialogClassPart	dialog_class;
} DialogClassRec;

extern DialogClassRec dialogClassRec;

typedef struct _DialogPart {
    /* resources */
    String	label;		/* description of the dialog	*/
    String	value;		/* for the user response	*/
    Pixmap	icon;		/* icon bitmap			*/
    /* private data */
    Widget	iconW;		/* widget to display the icon	*/
    Widget	labelW;		/* widget to display description*/
    Widget	valueW;		/* user response TextWidget	*/
} DialogPart;

typedef struct _DialogRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    FormPart		form;
    DialogPart		dialog;
} DialogRec;

typedef struct {int empty;} DialogConstraintsPart;

typedef struct _DialogConstraintsRec {
    FormConstraintsPart	  form;
    DialogConstraintsPart dialog;
} DialogConstraintsRec, *DialogConstraints;

#endif /* _DialogP_h */
