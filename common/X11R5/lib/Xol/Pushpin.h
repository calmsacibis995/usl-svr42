/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)pushpin:Pushpin.h	1.8"
#endif

#ifndef _Pushpin_h
#define _Pushpin_h

/*
 *************************************************************************
 *
 * Description:
 *		"Public" include file for the Pushpin Widget.
 *
 ******************************file*header********************************
 */

#include <Xol/Primitive.h>		/* include superclasses' header */

extern WidgetClass			pushpinWidgetClass;
typedef struct _PushpinClassRec *	PushpinWidgetClass;
typedef struct _PushpinRec *		PushpinWidget;

#endif /* _Pushpin_h */
