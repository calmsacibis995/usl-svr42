/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xlock:hopalong.c	1.1"
/*-
 * hopalong.c - Real Plane Fractals for the xlock X11 terminal locker.
 *
 * Copyright (c) 1988-89 by Patrick Naughton and Sun Microsystems, Inc.
 *
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Comments and additions should be sent to the author:
 *
 *		       naughton@sun.com
 *
 *		       Patrick J. Naughton
 *		       Window Systems Group, MS 14-40
 *		       Sun Microsystems, Inc.
 *		       2550 Garcia Ave
 *		       Mountain View, CA  94043
 *
 * Revision History:
 * 20-Sep-89: Lint.
 * 31-Aug-88: Forked from xlock.c for modularity.
 * 23-Mar-88: Coded HOPALONG routines from Scientific American Sept. 86 p. 14.
 */

#include <math.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static int  centerx,
	    centery;		/* center of the screen */
static double a,
	    b,
	    c,
	    i,
	    j;			/* hopalong parameters */
static int  color;
static unsigned long pix = 0;
static Display *Dsp;
static Window Win;
static GC   Gc;
static XPoint *pointBuffer = 0;	/* pointer for XDrawPoints */
static int  Npoints = 0;
static long startTime;
static int  timeout;

static long
seconds()
{
    struct timeval foo;

    gettimeofday(&foo);
    return (foo.tv_sec);
}

void
inithop(d, w, g, c, t, n, p, x, y, A, B, C)
    Display    *d;
    Window	w;
    GC		g;
    int		c,
		t,
		n,
		p,
		x,
		y;
    double	A,
		B,
		C;
{
    i = j = 0.0;
    startTime = seconds();

    if ((pointBuffer) || (n != Npoints)) {
	if (pointBuffer)
	    free((char *) pointBuffer);
	pointBuffer = (XPoint *) malloc(n * sizeof(XPoint));
	Npoints = n;
    }
    Dsp = d;
    Win = w;
    Gc = g;
    color = c;
    timeout = t;
    if (p >= 0)
	pix = (unsigned long) p;
    centerx = x;
    centery = y;
    a = A;
    b = B;
    c = C;
    XClearWindow(Dsp, Win);
}


void
randomInithop(d, w, g, c, t, n)
    Display    *d;
    Window	w;
    GC		g;
    int		c,
		t,
		n;
{
    int		range;
    XWindowAttributes xgwa;
    double	A,
		B,
		C;
    int		x,
		y;

    XGetWindowAttributes(d, w, &xgwa);
    x = xgwa.width / 2;
    y = xgwa.height / 2;
    range = (int) sqrt((double) x * x + (double) y * y);
    A = random() % (range * 100) * (random() % 2 ? -1.0 : 1.0) / 100.0;
    B = random() % (range * 100) * (random() % 2 ? -1.0 : 1.0) / 100.0;
    C = random() % (range * 100) * (random() % 2 ? -1.0 : 1.0) / 100.0;

    if (!(random() % 3))
	a /= 10.0;
    if (!(random() % 2))
	b /= 100.0;

    inithop(d, w, g, c, t, n, -1, x, y, A, B, C);
}

int
hopdone()
{
    return (seconds() - startTime > timeout);
}


void
hop()
{
    register double oldj;
    register int k = Npoints;
    register XPoint *xp = pointBuffer;

    if (color) {
	XSetForeground(Dsp, Gc, pix++);
	pix %= 254;
    }
    while (k--) {
	oldj = j;
	j = a - i;
	i = oldj + (i < 0 ? sqrt(fabs(b * i - c)) : -sqrt(fabs(b * i - c)));
	xp->x = centerx + (int) (i + j);
	xp->y = centery - (int) (i - j);
	xp++;
    }
    XDrawPoints(Dsp, Win, Gc, pointBuffer, Npoints, CoordModeOrigin);
}
