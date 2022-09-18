/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Extern.c	1.2.1.30"
#endif

/*
 ************************************************************************	
 * Description:
 *	This file contains the external data definition.  This is
 * accomplished by setting the macro EXTERN before including the
 * header files.
 ************************************************************************	
 */

#ifndef EXTERN
#define EXTERN(t,var,val)	t var = val
#endif

				/* Include files.  Note, some of these
				 * headers don't have to be included by
				 * all source files since Extern.h
				 * conditionally includes global
				 * variables.  Such headers are marked
				 * with a comment.
				 */
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/WSMcomm.h>		/* for conditional inclusion	*/
#include <Xol/Olg.h>			/* for conditional inclusion	*/
#include <wm.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>
#include <Strings.h>
#include <Extern.h>
#include <limits.h>

/* String definitions */
#include <olwmmsgs>

		/* Define a structure for holding resources that are global
		 * to the application.
		 */
static WMResources	wmrcsRec;
WMResources *	wmrcs = &wmrcsRec;


/* mlp - what is the parentRelative resource for?? ( boolean )
 * default appears to be false.
 */

#define OFFSET(f)	XtOffsetOf(WMResources, f)
XtResource	global_resources[] = {
   { XtNpassKeys, XtCPassKeys, XtRBoolean, sizeof(Boolean),
     OFFSET(pass_keys), XtRImmediate, (XtPointer)False},
   { XtNdoFork, XtCDoFork, XtRBoolean, sizeof(Boolean),
     OFFSET(do_fork), XtRImmediate, (XtPointer)True },
   { "iconForeground", XtCForeground, XtRPixel, sizeof(Pixel),
     OFFSET(iconForeground), XtRString, "Black" },
   { "iconBackground", XtCBackground, XtRPixel, sizeof(Pixel),
     OFFSET(iconBackground), XtRString, "White" },
   { "inputWindowHeader", "InputWindowHeader", XtRPixel, sizeof(Pixel),
     OFFSET(inputFocusColor), XtRString, "White" },
   { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
     OFFSET(foregroundColor), XtRImmediate, (XtPointer)ULONG_MAX },
   { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
     OFFSET(backgroundColor), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNwindowFrameColor, XtCWindowFrameColor, XtRPixel, sizeof(Pixel),
     OFFSET(windowFrameColor), XtRString, "Black" },
   { XtNiconGravity, XtCIconGravity, XtRWMIconGravity, sizeof(WMIconGravity),
     OFFSET(iconGravity), XtRImmediate, (caddr_t)WMSouthGravity },
   { "parentRelative", "parentRelative", XtRBoolean, sizeof(Boolean),
     OFFSET(parentRelative), XtRImmediate, (caddr_t)0 },
   { "moveOpaque", "moveOpaque", XtRBoolean, sizeof(Boolean),
     OFFSET(moveOpaque), XtRImmediate, (caddr_t)0 },
   { XtNpointerFocus, XtCPointerFocus, XtRBoolean, sizeof(Boolean),
     OFFSET(pointerFocus), XtRImmediate, (caddr_t)0 },
   { XtNselectDoesPreview, XtCSelectDoesPreview, XtRBoolean, sizeof(Boolean),
     OFFSET(selectDoesPreview), XtRImmediate, (caddr_t)1 },
   { XtNiconBorder, XtCIconBorder, XtRBoolean, sizeof(Boolean),
     OFFSET(iconBorder), XtRImmediate, (caddr_t)1 },
   { XtNiconGrid, XtCIconGrid, XtRBoolean, sizeof(Boolean),
     OFFSET(iconGrid), XtRImmediate, (caddr_t)1 },
   { XtNiconGridSize, XtCIconGridSize, XtRInt, sizeof(int),
     OFFSET(iconGridSize), XtRImmediate, (caddr_t)13 },
   { XtNwindowLayering, XtCWindowLayering, XtRBoolean, sizeof(Boolean),
     OFFSET(windowLayering), XtRImmediate, (caddr_t)1 },
   { XtNscale, XtCScale, XtRInt, sizeof(int),
     OFFSET(scale), XtRImmediate, (caddr_t)12},
  { XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *),
    OFFSET(font_list), XtRImmediate, (XtPointer)NULL },
  { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
    OFFSET(font), XtRImmediate, (XtPointer)NULL },
  { XtNiconFont, XtCIconFont, XtRFontStruct, sizeof(XFontStruct *),
    OFFSET(iconFont), XtRImmediate, (XtPointer)NULL },
  { XtNxnlLanguage, XtCXnlLanguage, XtRString, sizeof(String),
    OFFSET(xnlLanguage), XtRString, (XtPointer) "C" },
  { XtNiconParentRelative, XtCIconParentRelative, XtRBoolean, sizeof(Boolean),
    OFFSET(iconParentRelative), XtRImmediate, (XtPointer) False },
	/* if pointerColormapFocus is true, then the colormap (installed)
	 * focus follows the pointer; otherwise it follows focus (explicitly)
	 */
  { XtNpointerColormapFocus, XtCPointerColormapFocus, XtRBoolean,
    sizeof(Boolean), OFFSET(pointerColormapFocus), XtRImmediate, (XtPointer) 0},
};

Cardinal	num_global_resources = XtNumber(global_resources);

#undef OFFSET

Widget	menushells_posted[MAX_MENUSHELLS];

/* A Buffer type is defined in <Xol/buffutil.h> - {int size, used, esize;
 * BufferElement *p;}.  Bufferof(type) is defined for {int size, used, esize;
 * type *p; } - replacing the word type with Widget, and we can maintain a
 * list of these types in a buffer;
 *  - size can denote the length of the list;
 *  - used can denote how many elts. in the list are used;
 *  - esize will be the size of the type itself (sizeof(Widget)).
 *  - p is a pointer to one of the type elements;
 */

static WidgetBuffer	windowListRec = {0, 0, sizeof(Widget), (Widget *)NULL};
WidgetBuffer *	window_list = &windowListRec;

static WidgetBuffer	groupListRec = {0, 0, sizeof(Widget), (Widget *)NULL};
WidgetBuffer *	group_list = &groupListRec;

WMMenuInfo *	WMCombinedMenu = (WMMenuInfo *) 0;
