/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)button:RectButtoP.h	1.8"
#endif

/*
 ************************************************************
 *
 *  Date:	August 12, 1988
 *
 *  Description:
 *	This file contains the private definitions for the
 *	OPEN LOOK(tm) RectButton widget.
 *
 ************************************************************
 */


#ifndef _OlRectButtonP_h
#define _OlRectButtonP_h

#include <Xol/ButtonP.h>	/* include superclasses's header */
#include <Xol/RectButton.h>

/*
 *  There are no new fields in the RectButton class
 */
typedef struct _RectButtonClass {
    char no_class_fields;		/* Makes compiler happy */
} RectButtonClassPart;

/*
 *  declare the class record for the RectButton widget
 */
typedef struct _RectButtonClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	 primitive_class;
	ButtonClassPart		button_class;
	RectButtonClassPart	rect_button_class;
	} RectButtonClassRec;

/*
 *  rectButtonClassRec is defined in RectButton.c
 */
extern RectButtonClassRec rectButtonClassRec;

/*
 *  declaration of the RectButton widget fields
 */
typedef struct {
	Boolean		parent_reset;
	} RectButtonPart;

/*
 *  RectButton widget record declaration
 */
typedef struct _RectButtonRec {
	CorePart	core;
	PrimitivePart	primitive;
	ButtonPart	button;
	RectButtonPart	rect;
} RectButtonRec;

#endif /* _OlRectButtonP_h */
