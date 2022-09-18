/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:PortholeP.h	1.2"
/*
 * $XConsortium: PortholeP.h,v 1.1 90/02/28 18:07:32 jim Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#ifndef _XawPortholeP_h
#define _XawPortholeP_h

#include <X11/CompositeP.h>
#include <X11/Xaw/Porthole.h>


typedef struct {			/* new fields in widget class */
    int dummy;
} PortholeClassPart;

typedef struct _PortholeClassRec {	/* Porthole widget class */
    CoreClassPart core_class;
    CompositeClassPart composite_class;
    PortholeClassPart porthole_class;
} PortholeClassRec;


typedef struct {			/* new fields in widget */
    /* resources... */
    XtCallbackList report_callbacks;	/* callback/Callback */
    /* private data... */
} PortholePart;

typedef struct _PortholeRec {
    CorePart core;
    CompositePart composite;
    PortholePart porthole;
} PortholeRec;


/*
 * external declarations
 */
extern PortholeClassRec portholeClassRec;


#endif /* _XawPortholeP_h */
