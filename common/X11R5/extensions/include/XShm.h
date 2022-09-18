/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:include/XShm.h	1.1"
/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology


********************************************************/

/* THIS IS NOT AN X CONSORTIUM STANDARD */

/* $XConsortium: XShm.h,v 1.5 91/07/12 09:54:34 rws Exp $ */

#ifndef _XSHM_H_
#define _XSHM_H_

#include <X11/Xfuncproto.h>

#define X_ShmQueryVersion		0
#define X_ShmAttach			1
#define X_ShmDetach			2
#define X_ShmPutImage			3
#define X_ShmGetImage			4
#define X_ShmCreatePixmap		5

#define ShmCompletion			0
#define ShmNumberEvents			(ShmCompletion + 1)

#define BadShmSeg			0
#define ShmNumberErrors			(BadShmSeg + 1)

typedef unsigned long ShmSeg;

#ifndef _XSHM_SERVER_
typedef struct {
    int	type;		    /* of event */
    unsigned long serial;   /* # of last request processed by server */
    Bool send_event;	    /* true if this came frome a SendEvent request */
    Display *display;	    /* Display the event was read from */
    Drawable drawable;	    /* drawable of request */
    int major_code;	    /* ShmReqCode */
    int minor_code;	    /* X_ShmPutImage */
    ShmSeg shmseg;	    /* the ShmSeg used in the request */
    unsigned long offset;   /* the offset into ShmSeg used in the request */
} XShmCompletionEvent;

typedef struct {
    ShmSeg shmseg;	/* resource id */
    int shmid;		/* kernel id */
    char *shmaddr;	/* address in client */
    Bool readOnly;	/* how the server should attach it */
} XShmSegmentInfo;

_XFUNCPROTOBEGIN

Bool XShmQueryVersion(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */,
    Bool*		/* sharedPixmaps */
#endif
);

int XShmPixmapFormat(
#if NeedFunctionPrototypes
    Display*		/* dpy */
#endif
);

Status XShmAttach(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    XShmSegmentInfo*	/* shminfo */
#endif
);

Status XShmDetach(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    XShmSegmentInfo*	/* shminfo */
#endif
);

Status XShmPutImage(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Drawable		/* d */,
    GC			/* gc */,
    XImage*		/* image */,
    int			/* src_x */,
    int			/* src_y */,
    int			/* dst_x */,
    int			/* dst_y */,
    unsigned int	/* src_width */,
    unsigned int	/* src_height */,
    Bool		/* send_event */
#endif
);

Status XShmGetImage(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Drawable		/* d */,
    XImage*		/* image */,
    int			/* x */,
    int			/* y */,
    unsigned long	/* plane_mask */
#endif
);

XImage *XShmCreateImage(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Visual*		/* visual */,
    unsigned int	/* depth */,
    int			/* format */,
    char*		/* data */,
    XShmSegmentInfo*	/* shminfo */,
    unsigned int	/* width */,
    unsigned int	/* height */
#endif
);

Pixmap XShmCreatePixmap(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Drawable		/* d */,
    char*		/* data */,
    XShmSegmentInfo*	/* shminfo */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int	/* depth */
#endif
);

_XFUNCPROTOEND

#endif

#endif
