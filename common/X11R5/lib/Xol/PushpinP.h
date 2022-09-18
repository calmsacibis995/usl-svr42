/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)pushpin:PushpinP.h	1.14"
#endif

#ifndef _PushpinP_h
#define _PushpinP_h

/*
 ************************************************************************
 *
 * Description:
 *		"Private" include file for the Pushpin Widget.
 *
 *****************************file*header********************************
 */

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Pushpin.h>
#include <Xol/Olg.h>

/*
 ************************************************************************
 *
 * Define the Pushpin's Class Part and then the Class Record
 *
 ************************************************************************
 */

typedef struct _PushpinClass  {
    char no_class_fields;		/* Makes compiler happy */
} PushpinClassPart;

typedef struct _PushpinClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	PushpinClassPart	pushpin_class;
} PushpinClassRec;

		/* Declare the public hook to the Pushpin Class Record	*/
			
extern PushpinClassRec pushpinClassRec;

/*************************************************************************
 *
 * Define the widget instance structure for the pushpin
 *
 ************************************************************************/

typedef struct {
					/* Public Resources		*/

    int			scale;		/* size of this pushpin		*/
    XtCallbackList	in_callback;	/* Pinning callback list	*/
    XtCallbackList	out_callback;	/* UnPinning callback list	*/
    Boolean		is_default;	/* Is pushpin a default ??	*/

					/* Private Resources		*/

    ShellBehavior	shell_behavior;	/* behavior of pushpin's shell	*/
    Widget		preview_widget;	/* Widget to preview pushpin in	*/
    XtPointer		preview_cache;	/* private preview cache pointer*/
    OlgAttrs   		*pAttrs;	/* Graphics attributes		*/
    Boolean		selected;	/* has Pushpin been selected ??	*/
    OlDefine		pin_state;	/* binary pin state: OL_IN or
					 * OL_OUT			*/
} PushpinPart;

				/* Full Record Declaration		*/

typedef struct _PushpinRec {
	CorePart	core;
	PrimitivePart	primitive;
	PushpinPart	pushpin;
} PushpinRec;

#endif /* _PushpinP_h */


