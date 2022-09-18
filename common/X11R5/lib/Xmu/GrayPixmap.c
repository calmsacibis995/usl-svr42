/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:GrayPixmap.c	1.2"
/* static char Xrcsid[] = "$XConsortium: GrayPixmap.c,v 1.6 90/12/01 12:58:06 rws Exp $"; */


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include <stdio.h>
#include <X11/IntrinsicP.h>

typedef struct _PixmapCache {
    Screen *screen;
    Pixmap pixmap;
    Pixel foreground, background;
    unsigned int depth;
    int ref_count;
    struct _PixmapCache *next;
  } CacheEntry;

static CacheEntry *pixmapCache = NULL;



Pixmap XmuCreateStippledPixmap(screen, fore, back, depth)
    Screen *screen;
    Pixel fore, back;
    unsigned int depth;
/*
 *	Creates a stippled pixmap of specified depth
 *	caches these so that multiple requests share the pixmap
 */
{
    register Display *display = DisplayOfScreen(screen);
    CacheEntry *cachePtr;
    Pixmap stippled_pixmap;
    static unsigned char pixmap_bits[] = {
	0x02, 0x01,
    };

/*
 *	Creates a stippled pixmap of depth DefaultDepth(screen)
 *	caches these so that multiple requests share the pixmap
 */

#define pixmap_width 2
#define pixmap_height 2

    /* see if we already have a pixmap suitable for this screen */
    for (cachePtr = pixmapCache; cachePtr; cachePtr = cachePtr->next) {
	if (cachePtr->screen == screen && cachePtr->foreground == fore &&
	    cachePtr->background == back && cachePtr->depth == depth)
	    return( cachePtr->ref_count++, cachePtr->pixmap );
    }

    stippled_pixmap = XCreatePixmapFromBitmapData (display,
			RootWindowOfScreen(screen), (char *)pixmap_bits, 
			pixmap_width, pixmap_height, fore, back, depth);

    /* and insert it at the head of the cache */
    cachePtr = XtNew(CacheEntry);
    cachePtr->screen = screen;
    cachePtr->foreground = fore;
    cachePtr->background = back;
    cachePtr->depth = depth;
    cachePtr->pixmap = stippled_pixmap;
    cachePtr->ref_count = 1;
    cachePtr->next = pixmapCache;
    pixmapCache = cachePtr;

    return( stippled_pixmap );
}

void XmuReleaseStippledPixmap(screen, pixmap)
    Screen *screen;
    Pixmap pixmap;
{
    register Display *display = DisplayOfScreen(screen);
    CacheEntry *cachePtr, **prevP;
    for (prevP = &pixmapCache, cachePtr = pixmapCache; cachePtr;) {
	if (cachePtr->screen == screen && cachePtr->pixmap == pixmap) {
	    if (--cachePtr->ref_count == 0) {
		XFreePixmap( display, pixmap );
		*prevP = cachePtr->next;
		XtFree( (char*)cachePtr );
		break;
	    }
	}
	prevP = &cachePtr->next;
	cachePtr = *prevP;
    }
}
