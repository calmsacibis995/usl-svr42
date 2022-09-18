/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)abbrevstack:AbbrevFMeP.h	1.3"
#endif

#ifndef _Ol_AbbrevFMeP_h
#define _Ol_AbbrevFMeP_h

/*
 *************************************************************************
 *
 * Description:
 *		Private ".h" file for the AbbreviatedFlatMenuButton Widget
 ******************************file*header********************************
 */

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/AbbrevFMen.h>
#include <Xol/Olg.h>

/*
 *************************************************************************
 *
 * Widget Private Data
 *
 *************************************************************************
 */


/* New fields for the widget class record	*/

typedef struct _AbbreviatedButtonClass {
    char no_class_fields;		/* Makes compiler happy */
} AbbreviatedButtonClassPart;

/* Full class record declaration 	*/

typedef struct _AbbreviatedButtonClassRec {
    CoreClassPart			core_class;
    PrimitiveClassPart			primitive_class;
    AbbreviatedButtonClassPart		abbreviated_button_class;
} AbbreviatedButtonClassRec;

extern AbbreviatedButtonClassRec abbreviatedButtonClassRec;

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

    Widget		preview_widget; /* Id where abbrev. does its
    					 * its previewing.		*/
    int			scale;
    Widget		popup;	 	/* Submenu 			*/

    /* Private data	*/

    Boolean		set;
    Boolean		previewing;
    Boolean		err_flg;	/* menu is invalid flag		*/
    Widget		flat;		/* "pane" of submenu		*/
    Cardinal		dflt_index;	/* index of default item	*/
    OlgAttrs		*pAttrs;	/* GCs and what not		*/
} AbbreviatedButtonPart;

/* Full widget declaration	*/

typedef struct _AbbreviatedButtonRec {
    CorePart			core;
    PrimitivePart		primitive;
    AbbreviatedButtonPart	abbreviated_button;
} AbbreviatedButtonRec;

#endif /* _Ol_AbbrevFMeP_h */
