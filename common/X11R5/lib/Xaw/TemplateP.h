/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:TemplateP.h	1.2"
/* $XConsortium: TemplateP.h,v 1.6 91/03/13 20:12:07 rws Exp $ */

/* Copyright	Massachusetts Institute of Technology	1987, 1988
 *
 *
 *
 */

#ifndef _TemplateP_h
#define _TemplateP_h

#include <X11/Xaw/Template.h>
/* include superclass private header file */
#include <X11/CoreP.h>

/* define unique representation types not found in <X11/StringDefs.h> */

#define XtRTemplateResource "TemplateResource"

typedef struct {
    int empty;
} TemplateClassPart;

typedef struct _TemplateClassRec {
    CoreClassPart	core_class;
    TemplateClassPart	template_class;
} TemplateClassRec;

extern TemplateClassRec templateClassRec;

typedef struct {
    /* resources */
    char* resource;
    /* private state */
} TemplatePart;

typedef struct _TemplateRec {
    CorePart		core;
    TemplatePart	template;
} TemplateRec;

#endif /* _TemplateP_h */
