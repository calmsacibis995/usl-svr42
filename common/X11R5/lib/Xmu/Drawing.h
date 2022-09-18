/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:Drawing.h	1.2"
/* $XConsortium: Drawing.h,v 1.10 91/07/22 23:45:48 converse Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 * The X Window System is a Trademark of MIT.
 *
 * The interfaces described by this header file are for miscellaneous utilities
 * and are not part of the Xlib standard.
 */

#ifndef _XMU_DRAWING_H_
#define _XMU_DRAWING_H_

#include <X11/Xfuncproto.h>

#if NeedFunctionPrototypes
#include <stdio.h>
#if ! defined(_XtIntrinsic_h) && ! defined(PIXEL_ALREADY_TYPEDEFED)
typedef unsigned long Pixel;
#endif
#endif

_XFUNCPROTOBEGIN

extern void XmuDrawRoundedRectangle(
#if NeedFunctionPrototypes
    Display*	/* dpy */,
    Drawable 	/* draw */,
    GC 		/* gc */,
    int		/* x */,
    int		/* y */,
    int		/* w */,
    int		/* h */,
    int		/* ew */,
    int		/* eh */
#endif
);

extern void XmuFillRoundedRectangle(
#if NeedFunctionPrototypes
    Display*	/* dpy */,
    Drawable 	/* draw */,
    GC 		/* gc */,
    int		/* x */,
    int		/* y */,
    int		/* w */,
    int		/* h */,
    int		/* ew */,
    int		/* eh */
#endif
);

extern void XmuDrawLogo(
#if NeedFunctionPrototypes
    Display*	/* dpy */,
    Drawable 	/* drawable */,
    GC		/* gcFore */,
    GC		/* gcBack */,
    int		/* x */,
    int		/* y */,
    unsigned int /* width */,
    unsigned int /* height */
#endif
);

extern Pixmap XmuCreatePixmapFromBitmap(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Drawable 		/* d */,
    Pixmap 		/* bitmap */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int	/* depth */,
    unsigned long	/* fore */,
    unsigned long	/* back */
#endif
);

extern Pixmap XmuCreateStippledPixmap(
#if NeedFunctionPrototypes
    Screen*		/* screen */,
    Pixel		/* fore */,
    Pixel		/* back */,
    unsigned int	/* depth */
#endif
);

extern void XmuReleaseStippledPixmap(
#if NeedFunctionPrototypes
    Screen*		/* screen */,
    Pixmap 		/* pixmap */
#endif
);

extern Pixmap XmuLocateBitmapFile(
#if NeedFunctionPrototypes
    Screen*		/* screen */,
    _Xconst char*	/* name */,
    char*		/* srcname_return */,
    int 		/* srcnamelen */,
    int*		/* width_return */,
    int*		/* height_return, */,
    int*		/* xhot_return */,
    int*		/* yhot_return */
#endif
);

extern Pixmap XmuLocatePixmapFile(
#if NeedFunctionPrototypes
    Screen*		/* screen */,
    _Xconst char*	/* name */,
    unsigned long	/* fore */,
    unsigned long	/* back */,
    unsigned int	/* depth */,
    char*		/* srcname_return */,
    int 		/* srcnamelen */,
    int*		/* width_return */,
    int*		/* height_return, */,
    int*		/* xhot_return */,
    int*		/* yhot_return */
#endif
);

extern int XmuReadBitmapData(
#if NeedFunctionPrototypes
    FILE*		/* fstream */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */,
    unsigned char**	/* datap_return */,
    int*		/* xhot_return */,
    int*		/* yhot_return */
#endif
);

extern int XmuReadBitmapDataFromFile(
#if NeedFunctionPrototypes
    _Xconst char*	/* filename */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */,
    unsigned char**	/* datap_return */,
    int*		/* xhot_return */,
    int*		/* yhot_return */
#endif
);

_XFUNCPROTOEND

#endif /* _XMU_DRAWING_H_ */
