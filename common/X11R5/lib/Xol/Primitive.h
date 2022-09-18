/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)primitive:Primitive.h	1.3"
#endif


#ifndef _OlPrimitive_h
#define _OlPrimitive_h


#include <X11/Core.h>


/* Class record constants */

extern WidgetClass	primitiveWidgetClass;

typedef struct _PrimitiveClassRec	*PrimitiveWidgetClass;
typedef struct _PrimitiveRec		*PrimitiveWidget;

#endif	/* _OlPrimitive_h */
