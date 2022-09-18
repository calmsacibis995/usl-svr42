/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xlock:HSBmap.c	1.1"
/*-
 * HSBmap.c - Create an HSB ramp.
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * Author: Patrick J. Naughton
 * naughton@sun.com
 *
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 */

#include <sys/types.h>
#include <math.h>

void
hsb2rgb(H, S, B, r, g, b)
    double      H,
                S,
                B;
    unsigned char     *r,
               *g,
               *b;
{
    register int i;
    register double f,
                bb;
    register unsigned char p,
                q,
                t;

    H -= floor(H);		/* remove anything over 1 */
    H *= 6.0;
    i = floor(H);		/* 0..5 */
    f = H - (float) i;		/* f = fractional part of H */
    bb = 255.0 * B;
    p = (unsigned char) (bb * (1.0 - S));
    q = (unsigned char) (bb * (1.0 - (S * f)));
    t = (unsigned char) (bb * (1.0 - (S * (1.0 - f))));
    switch (i) {
    case 0:
	*r = (unsigned char) bb;
	*g = t;
	*b = p;
	break;
    case 1:
	*r = q;
	*g = (unsigned char) bb;
	*b = p;
	break;
    case 2:
	*r = p;
	*g = (unsigned char) bb;
	*b = t;
	break;
    case 3:
	*r = p;
	*g = q;
	*b = (unsigned char) bb;
	break;
    case 4:
	*r = t;
	*g = p;
	*b = (unsigned char) bb;
	break;
    case 5:
	*r = (unsigned char) bb;
	*g = p;
	*b = q;
	break;
    }
}


void
HSBramp(h1, s1, b1, h2, s2, b2, start, end, red, green, blue)
    double      h1,
                s1,
                b1,
                h2,
                s2,
                b2;
    int         start,
                end;
    unsigned char     *red,
               *green,
               *blue;
{
    double      dh,
                ds,
                db;
    register unsigned char *r,
               *g,
               *b;
    register int i;

    r = red;
    g = green;
    b = blue;
    dh = (h2 - h1) / 255.0;
    ds = (s2 - s1) / 255.0;
    db = (b2 - b1) / 255.0;
    for (i = start; i <= end; i++) {
	hsb2rgb(h1, s1, b1, r++, g++, b++);
	h1 += dh;
	s1 += ds;
	b1 += db;
    }
}
