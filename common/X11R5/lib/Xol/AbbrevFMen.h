/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)abbrevstack:AbbrevFMen.h	1.2"
#endif

#ifndef _Ol_AbbrevFMen_h
#define _Ol_AbbrevFMen_h

/*
 *************************************************************************
 *
 * Description:
 *		This is the "public" include file for the
 *	AbbreviatedFlatMenuButton Widget.
 *
 ******************************file*header********************************
 */

#include <Xol/Primitive.h>		/* include superclasses' header */

extern WidgetClass				abbreviatedButtonWidgetClass;

typedef struct _AbbreviatedButtonClassRec *	AbbreviatedButtonWidgetClass;
typedef struct _AbbreviatedButtonRec *		AbbreviatedButtonWidget;

#endif /* _Ol_AbbrevFMen_h */
