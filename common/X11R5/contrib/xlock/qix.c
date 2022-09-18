/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xlock:qix.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)qix.c 22.4 89/09/23";
#endif
/*-
 * qix.c - The old standby vector swirl for the xlock X11 terminal locker.
 *
 * Copyright (c) 1989 by Sun Microsystems Inc.
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
 * 23-Sep-89: Switch to random() and fixed bug w/ less than 4 lines.
 * 20-Sep-89: Lint.
 * 24-Mar-89: Written.
 */

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static Display *Dsp;
static Window Win;
static GC   Gc,
	    eraseGC = (GC) 0;
static int  timeout;
static int  color;
static unsigned long pix = 0;
static long startTime;
static int  first,
	    last,
	    dx1,
	    dy1,
	    dx2,
	    dy2,
	    x1,
	    y1,
	    x2,
	    y2,
	    offset,
	    delta,
	    width,
	    height;

typedef struct {
    int		x,
		y;
}	    point;

static int  Nlines = 0;
static point *lineq = (point *) 0;

static long
seconds()
{
    struct timeval foo;

    gettimeofday(&foo);
    return (foo.tv_sec);
}

void
initqix(d, w, g, c, t, n)
    Display    *d;
    Window	w;
    GC		g;
    int		c,
		t,
		n;
{
    XWindowAttributes xgwa;
    XGCValues	xgcv;

    startTime = seconds();
    if (n < 4)
	n = 4;

    if (n != Nlines) {
	if (lineq)
	    free((char *) lineq);
	lineq = (point *) malloc(n * sizeof(point));
	Nlines = n;
    }
    Dsp = d;
    Win = w;
    Gc = g;
    color = c;
    timeout = t;

    if (eraseGC == (GC) 0) {
	xgcv.foreground = BlackPixel(Dsp, 0);
	eraseGC = XCreateGC(Dsp, Win, GCForeground, &xgcv);
    }
    if (!color)
	XSetForeground(Dsp, Gc, WhitePixel(Dsp, 0));

    XGetWindowAttributes(Dsp, Win, &xgwa);
    width = xgwa.width;
    height = xgwa.height;

    if (width < 100)		/* icon window */
	delta = 2;
    else
	delta = 15;
    offset = delta / 3;
    last = 0;

    srandom(time((long *) 0));
    dx1 = random() & (width - 1) + 50;
    dy1 = random() & (height - 1) + 50;
    dx2 = random() & (width - 1) + 50;
    dy2 = random() & (height - 1) + 50;
    x1 = random() & width;
    y1 = random() & height;
    x2 = random() & width;
    y2 = random() & height;
    XFillRectangle(Dsp, Win, eraseGC, 0, 0, width, height);
}

int
qixdone()
{
    return (seconds() - startTime > timeout);
}

void
drawqix()
{
    register int n = Nlines;

    while (n--) {
	first = (last + 2) % Nlines;

	x1 += dx1;
	y1 += dy1;
	x2 += dx2;
	y2 += dy2;
	check_bounds_x(x1, &dx1);
	check_bounds_y(y1, &dy1);
	check_bounds_x(x2, &dx2);
	check_bounds_y(y2, &dy2);
	if (color) {
	    XSetForeground(Dsp, Gc, pix++);
	    if (pix > 253)
		pix = 0;
	}
	XDrawLine(Dsp, Win, eraseGC,
		  lineq[first].x, lineq[first].y,
		  lineq[first + 1].x, lineq[first + 1].y);
	XDrawLine(Dsp, Win, Gc, x1, y1, x2, y2);

	lineq[last].x = x1;
	lineq[last].y = y1;
	last += 1;
	if (last >= Nlines)
	    last = 0;

	lineq[last].x = x2;
	lineq[last].y = y2;
	last += 1;
	if (last >= Nlines)
	    last = 0;
    }
}

static
check_bounds_y(y, dy)
    int		y,
	       *dy;
{
    if (y < 0) {
	*dy = (random() & delta) + offset;
    } else if (y > height) {
	*dy = -(random() & delta) - offset;
    }
}

static
check_bounds_x(x, dx)
    int		x,
	       *dx;
{
    if (x < 0) {
	*dx = (random() & delta) + offset;
    } else if (x > width) {
	*dx = -(random() & delta) - offset;
    }
}
