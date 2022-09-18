/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)stub:Stub.h	1.6"
#endif

#ifndef _Stub_h
#define _Stub_h

/*
 *************************************************************************
 *
 * Description:
 *		"Public" include file for the Stub Widget.
 *
 ******************************file*header********************************
 */


#include <Xol/Primitive.h>  	/* include superclasses' header */

extern WidgetClass		stubWidgetClass;
typedef struct _StubClassRec *	StubWidgetClass;
typedef struct _StubRec *	StubWidget;

#endif /* _Widget_h */
