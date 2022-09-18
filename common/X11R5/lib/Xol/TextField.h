/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)textfield:TextField.h	2.6"
#endif

#ifndef _TEXTFIELD_H
#define _TEXTFIELD_H

#include "Xol/TextEdit.h"

extern WidgetClass			textFieldWidgetClass;

typedef struct _TextFieldClassRec *	TextFieldWidgetClass;
typedef struct _TextFieldRec *		TextFieldWidget;

/*
 * Public types:
 */

typedef enum {
	OlTextFieldReturn,
	OlTextFieldPrevious,
	OlTextFieldNext,
	OlTextFieldStep
}			OlTextVerifyReason;

typedef struct {
	String			string;
	Boolean			ok;
	OlTextVerifyReason	reason;
}			OlTextFieldVerify,
		      * OlTextFieldVerifyPointer;

typedef enum		OlSteppedReason {
	OlSteppedNotAtAll,
	OlSteppedIncrement,
	OlSteppedDecrement,
	OlSteppedToMaximum,
	OlSteppedToMinimum
}			OlSteppedReason;

/*
 * Public functions:
 */

OLBeginFunctionPrototypeBlock

extern Cardinal		OlTextFieldCopyString OL_ARGS((
	Widget			w,
	String			string
));
extern String		OlTextFieldGetString OL_ARGS((
	Widget			w,
	Cardinal *		size
));

OLEndFunctionPrototypeBlock

#endif
