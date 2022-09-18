/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:GenKey.c	1.2"
/*
 * $XConsortium: GenKey.c,v 1.3 91/01/23 22:13:42 gildea Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xos.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xdmcp.h>

static getbits (data, dst)
    long	    data;
    unsigned char   *dst;
{
    dst[0] = (data      ) & 0xff;
    dst[1] = (data >>  8) & 0xff;
    dst[2] = (data >> 16) & 0xff;
    dst[3] = (data >> 24) & 0xff;
}

#if defined(SYSV) || defined(SVR4)
#define srandom srand48
#define random lrand48
#endif

long random();

void
XdmcpGenerateKey (key)
    XdmAuthKeyPtr   key;
{
    long    lowbits, highbits;

    srandom (getpid() ^ time (0));
    lowbits = random ();
    highbits = random ();
    getbits (lowbits, key->data);
    getbits (highbits, key->data + 4);
}
