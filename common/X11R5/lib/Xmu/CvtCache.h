/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:CvtCache.h	1.2"
/* $XConsortium: CvtCache.h,v 1.6 91/07/22 23:45:42 converse Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * 
 *			       Public Interfaces
 * 
 * XmuCvtCache *XmuCvtCacheLookupDisplay (dpy)
 *     Display *dpy;
 */

#ifndef _XMU_CVTCACHE_H_
#define _XMU_CVTCACHE_H_

#include <X11/Xmu/DisplayQue.h>
#include <X11/Xfuncproto.h>

typedef struct _XmuCvtCache {
    struct {
	char **bitmapFilePath;
    } string_to_bitmap;
    /* add other per-display data that needs to be cached */
} XmuCvtCache;

_XFUNCPROTOBEGIN

extern XmuCvtCache *_XmuCCLookupDisplay(
#if NeedFunctionPrototypes
    Display*	/* dpy */
#endif
);

_XFUNCPROTOEND

#endif /* _XMU_CVTCACHE_H_ */
