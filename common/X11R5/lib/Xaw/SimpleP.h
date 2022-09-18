/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:SimpleP.h	1.2"
/***********************************************************

 $XConsortium: SimpleP.h,v 1.10 90/03/05 17:47:33 kit Exp $ 

Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _SimpleP_h
#define _SimpleP_h

#include <X11/Xaw/Simple.h>
/* #include <X11/CoreP.h> */

typedef struct {
    Boolean	(*change_sensitive)(/* widget */);
} SimpleClassPart;

#define XtInheritChangeSensitive ((Boolean (*)())_XtInherit)

typedef struct _SimpleClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
} SimpleClassRec;

extern SimpleClassRec simpleClassRec;

typedef struct {
    /* resources */
    Cursor	cursor;
    Pixmap	insensitive_border;
    String      cursor_name;	/* cursor specified by name. */

    Pixel       pointer_fg, pointer_bg;	/* Pointer colors. */
    /* private state */
} SimplePart;

typedef struct _SimpleRec {
    CorePart	core;
    SimplePart	simple;
} SimpleRec;

#endif /* _SimpleP_h */
