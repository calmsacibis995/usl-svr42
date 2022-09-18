/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)textfield:StepFieldP.h	1.1"
#endif

#ifndef _STEPFIELDP_H
#define _STEPFIELDP_H

#include "Xol/TextFieldP.h"
#include "Xol/StepField.h"

/*
 * Class structure:
 */

typedef struct _StepFieldClassPart {
	XtPointer		extension;
}			StepFieldClassPart;

typedef struct _StepFieldClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	TextEditClassPart	textedit_class;
	TextFieldClassPart	textfield_class;
	StepFieldClassPart	stepfield_class;
}			StepFieldClassRec;

extern StepFieldClassRec	stepFieldClassRec;

#define STEPFIELD_C(WC) ((StepFieldWidgetClass)(WC))->stepfield_class

/*
 * Instance structure:
 */

typedef struct _StepFieldPart {
	/*
	 * Public:
	 */
	XtCallbackList		stepped;

	/*
	 * Private:
	 */
}			StepFieldPart;

typedef struct _StepFieldRec {
	CorePart		core;
	PrimitivePart		primitive;
	TextEditPart		textedit;
	TextFieldPart		textfield;
	StepFieldPart		stepfield;
}			StepFieldRec;

#define STEPFIELD_P(W) ((StepFieldWidget)(W))->stepfield

#endif
