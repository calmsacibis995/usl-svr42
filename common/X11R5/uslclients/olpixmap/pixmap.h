/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:pixmap.h	1.17"
#endif

#ifndef _PIXMAP_H
#define _PIXMAP_H


#include <stdio.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include "externs.h"

typedef struct {
        XtArgVal label;
	XtArgVal p;
	XtArgVal data;
	XtArgVal sensitive;
	XtArgVal mapped;
	XtArgVal popup;
	XtArgVal mnemonic;
} MenuItem;
typedef struct Menu {
	String		label;
	MenuItem	*items;
	int		numitems;
	Bool		use_popup;
	int		orientation;	/* OL_FIXEDROWS, OL_FIXEDCOLS */
	int		pushpin;	/* OL_OUT, OL_NONE */
	Widget		widget;		/* Pointer to this menu widget */
} Menu;


#define APP_NAME	"Pixmap Editor"
#define DEFAULT_WIDTH	32
#define DEFAULT_HEIGHT	32

#define USE_SQUARE_PIXELS
#define MIN_PIXELWIDTH	((Dimension)OlPointToPixel(OL_HORIZONTAL, 6))
#define MIN_PIXELHEIGHT	((Dimension)OlPointToPixel(OL_VERTICAL, 6))

#define PREFERRED_WIDTH		OlMMToPixel(OL_HORIZONTAL, 100)
#define PREFERRED_HEIGHT	OlMMToPixel(OL_VERTICAL, 100)

#define MAX_LINEWIDTH	32
#define MAX_COLORS	256

#define PIXEL		0
#define TEXT		1
#define LINE		2
#define SEGMENT		3
#define OVAL		4
#define CIRCLE		5
#define RECTANGLE	6
#define SQUARE		7

#ifndef DESTDIR
#define DESTDIR		"/usr/X"
#endif
#define PIXMAPLOC	"/lib/pixmaps"
#define PATTERN		"*"

#define UNNAMED_FILE	"Untitled.xpm"


#define DISPLAY		XtDisplay(Toplevel)
#define SCREEN		XtScreen(Toplevel)
#define ROOT		RootWindowOfScreen(SCREEN)

#define ARGLISTSIZE	20
#define INIT_ARGS()	{Arg SetArgList[ARGLISTSIZE]; int SetArgCount = 0
#define SET_ARGS(R, A)	XtSetArg(SetArgList[SetArgCount], R, A); ++SetArgCount
#define END_ARGS()	}
#define SET_VALUES(W)	XtSetValues(W, SetArgList, SetArgCount)
#define GET_VALUES(W)	XtGetValues(W, SetArgList, SetArgCount)

/*
 *	For handling the attributes of individual items
 *	in a flattened widget...
 */
#define FSET_VALUES(W, I)	OlFlatSetValues((W), (I), \
						SetArgList, SetArgCount)
#define FGET_VALUES(W, I)	OlFlatGetValues((W), (I), \
						SetArgList, SetArgCount)

#define CREATE_MANAGED(N, C, P) \
			XtCreateManagedWidget(N,C,P,SetArgList,SetArgCount)
#define CREATE_POPUP(N, C, P) \
			XtCreatePopupShell(N,C,P,SetArgList,SetArgCount)

#define ABS(A)		((A) < 0 ? (-(A)) : (A))
#define MIN(A, B)	((A) < (B) ? (A) : (B))
#define MAX(A, B)	((A) > (B) ? (A) : (B))


#if	defined(__STDC__)
# define concat(a,b) a ## b
#else
# define concat(a,b) a/**/b
#endif

			     

#endif /* _PIXMAP_H */
