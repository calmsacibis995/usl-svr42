/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)textfield:IntegerFie.h	1.3"
#endif

#ifndef _INTEGERFIELD_H
#define _INTEGERFIELD_H

#include "Xol/TextField.h"

extern WidgetClass			integerFieldWidgetClass;

typedef struct _IntegerFieldClassRec *	IntegerFieldWidgetClass;
typedef struct _IntegerFieldRec *	IntegerFieldWidget;

typedef struct OlIntegerFieldChanged {
	int			value;
	Boolean			changed;
	OlTextVerifyReason	reason;
}			OlIntegerFieldChanged;

#endif
