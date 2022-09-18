/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XIMWrap.c	1.1"
/*
 * $XConsortium: XIMWrap.c,v 11.4 91/04/07 18:59:21 rws Exp $
 */

/*
 * Copyright 1991 by the Massachusetts Institute of Technology
 * Copyright 1991 by the Open Software Foundation
 *
 *
 * OPEN SOFTWARE FOUNDATION AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD TO
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * 
 *		 M. Collins		OSF  
 */				

#include "Xlibint.h"
#include "Xlcint.h"

/*
 * Compile the resource list. (XIMResourceList ---> XIMrmResourceList)
 */
void
_XIMCompileResourceList(res, num_res)
    register XIMResourceList res;
    unsigned int num_res;
{
    register unsigned int count;

#define	xrmres	((XIMrmResourceList) res)

    for (count = 0; count < num_res; res++, count++) {
	xrmres->xrm_name = XrmPermStringToQuark(res->resource_name);
	xrmres->xrm_offset = -res->resource_offset - 1;
    }
#undef	xrmres
}

void
_XCopyToArg(src, dst, size)
    XPointer src;
    XPointer *dst;
    register unsigned int size;
{
    if (!*dst) {
	union {
	    long	longval;
	    short	shortval;
	    char	charval;
	    char*	charptr;
	    XPointer	ptr;
	} u;
	if (size <= sizeof(XPointer)) {
	    bcopy((char *)src, (char *)&u, (int)size);
	    if (size == sizeof(long))	       *dst = (XPointer)u.longval;
	    else if (size == sizeof(short))    *dst = (XPointer)u.shortval;
	    else if (size == sizeof(char))     *dst = (XPointer)u.charval;
	    else if (size == sizeof(char*))    *dst = (XPointer)u.charptr;
	    else if (size == sizeof(XPointer)) *dst = (XPointer)u.ptr;
	    else bcopy( (char*)src, (char*)dst, (int)size );
	} else {
	    bcopy( (char*)src, (char*)dst, (int)size );
	}
    } else {
	bcopy( (char*)src, (char*)*dst, (int)size );
    }
}

/*
 * Connects to an input method matching current locale specification, creates
 * a XIM object and return a pointer the newly created XIM back to the caller.
 */

XIM 
XOpenIM(display, rdb, res_name, res_class)
    Display *display;
    XrmDatabase rdb;
    char *res_name;
    char *res_class;
{
    XLCd lcd = _XlcCurrentLC();

    if (!lcd)
	return (XIM) NULL;
    return (*lcd->methods->open_im) (lcd, display, rdb, res_name, res_class);
}

/*
 * Close the connection to the input manager, and free the XIM structure
 */
Status
XCloseIM(im)
    XIM im;
{
    Status s;
    XIC ic;
  
    s = (im->methods->close) (im);
    for (ic = im->core.ic_chain; ic; ic = ic->core.next)
	ic->core.im = (XIM)NULL;
    Xfree ((char *) im);
    return (s);
}

/*
 * Return the Display associated with the input method.
 */
Display *
XDisplayOfIM(im)
    XIM im;
{
    return im->core.display;
}

/*
 * Return the Locale associated with the input method.
 */
char *
XLocaleOfIM(im)
    XIM im;
{
    return im->core.lcd->core.name;
}
