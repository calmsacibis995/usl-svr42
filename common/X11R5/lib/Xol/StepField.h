/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)textfield:StepField.h	1.3"
#endif

#ifndef _STEPFIELD_H
#define _STEPFIELD_H

#include "Xol/TextField.h"

extern WidgetClass			stepFieldWidgetClass;

typedef struct _StepFieldClassRec *	StepFieldWidgetClass;
typedef struct _StepFieldRec *		StepFieldWidget;

typedef struct OlTextFieldStepped {
	OlSteppedReason		reason;
	Cardinal		count;
}			OlTextFieldStepped,
		      * OlTextFieldSteppedPointer;

#endif
