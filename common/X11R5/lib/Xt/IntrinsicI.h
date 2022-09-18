/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:IntrinsicI.h	1.1"
/* $XConsortium: IntrinsicI.h,v 1.48 91/06/27 13:24:18 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XtintrinsicI_h
#define _XtintrinsicI_h

#include "Xtos.h"
#include "IntrinsicP.h"
#include <X11/Xos.h>

#include "Object.h"
#include "RectObj.h"
#include "ObjectP.h"
#include "RectObjP.h"

#include "TranslateI.h"
#include "CallbackI.h"
#include "ConvertI.h"
#include "EventI.h"
#include "PassivGraI.h"
#include "InitialI.h"
#include "ResourceI.h"

#define RectObjClassFlag	0x02
#define WidgetClassFlag		0x04
#define CompositeClassFlag	0x08
#define ConstraintClassFlag	0x10
#define ShellClassFlag		0x20
#define WMShellClassFlag	0x40
#define TopLevelClassFlag	0x80

/*
 * The following macros, though very handy, are not suitable for
 * IntrinsicP.h as they violate the rule that arguments are to
 * be evaluated exactly once.
 */

#define XtDisplayOfObject(object) \
    ((XtIsWidget(object) ? (object) : _XtWindowedAncestor(object)) \
     ->core.screen->display)

#define XtScreenOfObject(object) \
    ((XtIsWidget(object) ? (object) : _XtWindowedAncestor(object)) \
     ->core.screen)

#define XtWindowOfObject(object) \
    ((XtIsWidget(object) ? (object) : _XtWindowedAncestor(object)) \
     ->core.window)

#define XtIsManaged(object) \
    (XtIsRectObj(object) ? (object)->core.managed : False)

#define XtIsSensitive(object) \
    (XtIsRectObj(object) ? ((object)->core.sensitive && \
			    (object)->core.ancestor_sensitive) : False)


/****************************************************************
 *
 * Byte utilities
 *
 ****************************************************************/

#define _XBCOPYFUNC _XtBCopy
#include <X11/Xfuncs.h>

/* If the alignment characteristics of your machine are right, these may be
   faster */

#ifdef UNALIGNED

#define XtBCopy(src, dst, size)				    \
    if (size == sizeof(int))				    \
	*((int *) (dst)) = *((int *) (src));		    \
    else if (size == sizeof(char))			    \
	*((char *) (dst)) = *((char *) (src));		    \
    else if (size == sizeof(short))			    \
	*((short *) (dst)) = *((short *) (src));	    \
    else						    \
	bcopy((char *) (src), (char *) (dst), (int) (size));

#define XtBZero(dst, size)				    \
    if (size == sizeof(int))				    \
	*((int *) (dst)) = 0;				    \
    else						    \
	bzero((char *) (dst), (int) (size));

#define XtBCmp(b1, b2, size)				    \
    (size == sizeof(int) ?				    \
	*((int *) (b1)) != *((int *) (b2))		    \
    :   bcmp((char *) (b1), (char *) (b2), (int) (size))    \
    )

#else

#define XtBCopy(src, dst, size)		\
	bcopy((char *) (src), (char *) (dst), (int) (size));

#define XtBZero(dst, size) bzero((char *) (dst), (int) (size));

#define XtBCmp(b1, b2, size) bcmp((char *) (b1), (char *) (b2), (int) (size))

#endif


/****************************************************************
 *
 * Stack cache allocation/free
 *
 ****************************************************************/

#define XtStackAlloc(size, stack_cache_array)     \
    ((size) <= sizeof(stack_cache_array)	  \
    ?  (XtPointer)(stack_cache_array)		  \
    :  XtMalloc((unsigned)(size)))

#define XtStackFree(pointer, stack_cache_array) \
    if ((pointer) != ((XtPointer)(stack_cache_array))) XtFree(pointer); else

/***************************************************************
 *
 * Filename defines
 *
 **************************************************************/

/* used by XtResolvePathname */
#ifndef XFILESEARCHPATHDEFAULT
#define XFILESEARCHPATHDEFAULT "/usr/lib/X11/%L/%T/%N%S:/usr/lib/X11/%l/%T/%N%S:/usr/lib/X11/%T/%N%S"
#endif

/* the following two were both "X Toolkit " prior to R4 */
#ifndef XTERROR_PREFIX
#define XTERROR_PREFIX ""
#endif

#ifndef XTWARNING_PREFIX
#define XTWARNING_PREFIX ""
#endif

#ifndef ERRORDB
#define ERRORDB "/usr/lib/X11/XtErrorDB"
#endif

extern String XtCXtToolkitError;

extern void _XtAllocError(
#if NeedFunctionPrototypes
    String	/* alloc_type */
#endif
);

extern void _XtCompileResourceList(
#if NeedFunctionPrototypes
    XtResourceList 	/* resources */,
    Cardinal 		/* num_resources */
#endif
);

extern XtGeometryResult _XtMakeGeometryRequest(
#if NeedFunctionPrototypes
    Widget 		/* widget */,
    XtWidgetGeometry*	/* request */,
    XtWidgetGeometry*	/* reply_return */,
    Boolean*		/* clear_rect_obj */
#endif
);

#endif /* _XtintrinsicI_h */
/* DON'T ADD STUFF AFTER THIS #endif */
