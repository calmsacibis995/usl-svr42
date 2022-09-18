/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)abbrevstack:AbbrevMenP.h	1.6"
#endif

#ifndef _Ol_AbbrevMenP_h
#define _Ol_AbbrevMenP_h

/*
 *************************************************************************
 *
 * Description:
 *		Private ".h" file for the AbbreviatedMenuButton Widget
 ******************************file*header********************************
 */

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/AbbrevStac.h>
#include <Xol/Olg.h>

/*
 *************************************************************************
 *
 * Widget Private Data
 *
 *************************************************************************
 */


			/* New fields for the widget class record	*/

typedef struct _AbbrevMenuButtonClass {
    char no_class_fields;		/* Makes compiler happy */
} AbbrevMenuButtonClassPart;

				/* Full class record declaration 	*/

typedef struct _AbbrevMenuButtonClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    AbbrevMenuButtonClassPart	menubutton_class;
} AbbrevMenuButtonClassRec;

extern AbbrevMenuButtonClassRec abbrevMenuButtonClassRec;

/*
 *************************************************************************
 *
 * Widget Instance Data
 *
 *************************************************************************
 */

				/* New fields for the widget record	*/
typedef struct {
							/* Public data	*/

    int			scale;
    Boolean		busy;
    Boolean		set;
    Widget		preview_widget; /* Id where abbrev. does its
    					 * its previewing.		*/

    						/* Private data	*/

    ShellBehavior	shell_behavior;	/* Behavior of Abbrev.'s shell	*/
    Widget		submenu;	/* Submenu for AbbrevMenuButton	*/
    Boolean		previewing_default;
    OlgAttrs		*pAttrs;	/* GCs and what not		*/
    XtCallbackList	post_select;
    Position		root_x;		/* Temporary cache		*/
    Position		root_y;		/* Temporary cache		*/
} AbbrevMenuButtonPart;

   					/* Full widget declaration	*/
typedef struct _AbbrevMenuButtonRec {
	CorePart		core;
	PrimitivePart		primitive;
	AbbrevMenuButtonPart	menubutton;
} AbbrevMenuButtonRec;

#endif /* _Ol_AbbrevMenP_h */
