/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xlock:life.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)life.c 22.1 89/09/20";
#endif
/*-
 * life.c - Conway's game of Life for the xlock X11 terminal locker.
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 *
 * This file is provided AS IS with no warranties of any kind.	The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Comments and additions should be sent to the authors:
 *
 *		       flar@sun.com or naughton@sun.com
 *
 *		       James A. Graham
 *		       Patrick J. Naughton
 *		       Window Systems Group, MS 14-40
 *		       Sun Microsystems, Inc.
 *		       2550 Garcia Ave
 *		       Mountain View, CA  94043
 *
 * Revision History:
 * 20-Sep-89: Written.
 */

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "lifeicon.bit"

static XImage logo = {
    0, 0,			/* width, height */
    0, XYBitmap, 0,		/* xoffset, format, data */
    LSBFirst, 8,		/* byte-order, bitmap-unit */
    LSBFirst, 8, 1		/* bitmap-bit-order, bitmap-pad, depth */
};

static Display *Dsp;
static Window Win;
static GC   Gc;
static GC   eraseGC = (GC) 0;
static int  timeout;
static int  shooter;
static int  delay;
static int  color;
static int  pixels;
static int  width,
            height;
static int  xs,
            ys,
            xb,
            yb;
static long startTime;

#define	NROWS	30
#define NCOLS	38

/* Buffer stores the data for each cell.  Each cell is stored as
 * 8 bits representing the presence of a critter in each of it's
 * surrounding 8 cells.	 There is an empty row and column around
 * the whole array to allow stores without bounds checking as well
 * as an extra row at the end for the fetches into tempbuf.
 */
#define	UPLT	0x01
#define UP	0x02
#define UPRT	0x04
#define LT	0x08
#define RT	0x10
#define DNLT	0x20
#define DN	0x40
#define DNRT	0x80
static unsigned char buffer[(NROWS + 2) * (NCOLS + 2) + 2];
static unsigned char agebuf[(NROWS + 2) * (NCOLS + 2)];

/* Tempbuf stores the data for the next two rows so that we know
 * the state of those critter before he was modified by the fate
 * of the critters that have already been processed.
 */
static unsigned char tempbuf[NCOLS * 2];

/* Fates is a lookup table for the fate of a critter.  The 256
 * entries represent the 256 possible combinations of the 8
 * neighbor cells.  Each entry is one of BIRTH (create a cell
 * or leave one alive), SAME (leave the cell alive or dead),
 * or DEATH (kill anything in the cell).
 */
#define BIRTH	0
#define SAME	1
#define DEATH	2
static unsigned char fates[256];


static int  patterns[][200] = {
    {				/* EIGHT */
	-3, -3, -2, -3, -1, -3,
	-3, -2, -2, -2, -1, -2,
	-3, -1, -2, -1, -1, -1,
	0, 0, 1, 0, 2, 0,
	0, 1, 1, 1, 2, 1,
	0, 2, 1, 2, 2, 2,
	99
    },
    {				/* PULSAR */
	1, 1, 2, 1, 3, 1, 4, 1, 5, 1,
	1, 2, 5, 2,
	99
    },
    {				/* BARBER */
	-7, -7, -6, -7,
	-7, -6, -5, -6,
	-5, -4, -3, -4,
	-3, -2, -1, -2,
	-1, 0, 1, 0,
	1, 2, 3, 2,
	3, 4, 5, 4,
	4, 5, 5, 5,
	99
    },
    {				/* HERTZ */
	-2, -6, -1, -6,
	-2, -5, -1, -5,
	-7, -3, -6, -3, -2, -3, -1, -3, 0, -3, 1, -3, 5, -3, 6, -3,
	-7, -2, -5, -2, -3, -2, 2, -2, 4, -2, 6, -2,
	-5, -1, -3, -1, -2, -1, 2, -1, 4, -1,
	-7, 0, -5, 0, -3, 0, 2, 0, 4, 0, 6, 0,
	-7, 1, -6, 1, -2, 1, -1, 1, 0, 1, 1, 1, 5, 1, 6, 1,
	-2, 3, -1, 3,
	-2, 4, -1, 4,
	99
    },
    {				/* TUMBLER */
	-6, -6, -5, -6, 6, -6, 7, -6,
	-6, -5, -5, -5, 6, -5, 7, -5,
	-5, 5, 6, 5,
	-7, 6, -5, 6, 6, 6, 8, 6,
	-7, 7, -5, 7, 6, 7, 8, 7,
	-7, 8, -6, 8, 7, 8, 8, 8,
	99
    },
    {				/* PERIOD4 */
	-5, -8, -4, -8,
	-7, -7, -5, -7,
	-8, -6, -2, -6,
	-7, -5, -3, -5, -2, -5,
	-5, -3, -3, -3,
	-4, -2,
	99
    },
    {				/* PERIOD5 */
	-5, -8, -4, -8,
	-6, -7, -3, -7,
	-7, -6, -2, -6,
	-8, -5, -1, -5,
	-8, -4, -1, -4,
	-7, -3, -2, -3,
	-6, -2, -3, -2,
	-5, -1, -4, -1,
	99
    },
    {				/* PERIOD6 */
	-4, -8, -3, -8,
	-8, -7, -7, -7, -5, -7,
	-8, -6, -7, -6, -4, -6, -1, -6,
	-3, -5, -1, -5,
	-2, -4,
	-3, -2, -2, -2,
	-3, -1, -2, -1,
	99
    },
    {				/* PINWHEEL */
	-4, -8, -3, -8,
	-4, -7, -3, -7,
	-4, -5, -3, -5, -2, -5, -1, -5,
	-5, -4, -3, -4, 0, -4, 2, -4, 3, -4,
	-5, -3, -1, -3, 0, -3, 2, -3, 3, -3,
	-8, -2, -7, -2, -5, -2, -2, -2, 0, -2,
	-8, -1, -7, -1, -5, -1, 0, -1,
	-4, 0, -3, 0, -2, 0, -1, 0,
	-2, 2, -1, 2,
	-2, 3, -1, 3,
	99
    },
    {				/* ] */
	-1, -1, 0, -1, 1, -1,
	0, 0, 1, 0,
	-1, 1, 0, 1, 1, 1,
	99
    },
    {				/* cc: */
	-3, -1, -2, -1, -1, -1, 1, -1, 2, -1, 3, -1,
	-3, 0, -2, 0, 1, 0, 2, 0,
	-3, 1, -2, 1, -1, 1, 1, 1, 2, 1, 3, 1,
	99
    },
    {				/* DOLBY */
	-3, -1, -2, -1, -1, -1, 1, -1, 2, -1, 3, -1,
	-3, 0, -2, 0, 2, 0, 3, 0,
	-3, 1, -2, 1, -1, 1, 1, 1, 2, 1, 3, 1,
	99
    },
    {				/* HORIZON */
	-15, 0, -14, 0, -13, 0, -12, 0, -11, 0,
	-10, 0, -9, 0, -8, 0, -7, 0, -6, 0,
	-5, 0, -4, 0, -3, 0, -2, 0, -1, 0,
	4, 0, 3, 0, 2, 0, 1, 0, 0, 0,
	9, 0, 8, 0, 7, 0, 6, 0, 5, 0,
	14, 0, 13, 0, 12, 0, 11, 0, 10, 0,
	99
    },
    {				/* SHEAR */
	-7, -2, -6, -2, -5, -2, -4, -2, -3, -2,
	-2, -2, -1, -2, 0, -2, 1, -2, 2, -2,
	-5, -1, -4, -1, -3, -1, -2, -1, -1, -1,
	0, -1, 1, -1, 2, -1, 3, -1, 4, -1,
	-3, 0, -2, 0, -1, 0, 0, 0, 1, 0,
	2, 0, 3, 0, 4, 0, 5, 0, 6, 0,
	-10, 1, -9, 1, -8, 1, -7, 1, -6, 1,
	-5, 1, -4, 1, -3, 1, -2, 1, -1, 1,
	-10, 2, -9, 2, -8, 2, -7, 2, -6, 2,
	-5, 2, -4, 2, -3, 2, -2, 2, -1, 2,
	99
    },
    {				/* VERTIGO */
	0, -7,
	0, -6,
	0, -5,
	0, -4,
	0, -3,
	0, -2,
	0, -1,
	0, 0,
	0, 7,
	0, 6,
	0, 5,
	0, 4,
	0, 3,
	0, 2,
	0, 1,
	99
    },
    {				/* CROSSBAR */
	-5, 0, -4, 0, -3, 0, -2, 0, -1, 0, 4, 0, 3, 0, 2, 0, 1, 0, 0, 0,
	99
    },
    {				/* GOALPOSTS */
	-8, -7, 8, -7,
	-8, -6, 8, -6,
	-8, -5, 8, -5,
	-8, -4, 8, -4,
	-8, -3, 8, -3,
	-8, -2, 8, -2,
	-8, -1, 8, -1,
	-8, 0, 8, 0,
	-8, 1, 8, 1,
	-8, 2, 8, 2,
	-8, 3, 8, 3,
	-8, 4, 8, 4,
	-8, 5, 8, 5,
	-8, 6, 8, 6,
	-8, 7, 8, 7,
	99
    },
    {				/* \ */
	-8, -8, -7, -8,
	-7, -7, -6, -7,
	-6, -6, -5, -6,
	-5, -5, -4, -5,
	-4, -4, -3, -4,
	-3, -3, -2, -3,
	-2, -2, -1, -2,
	-1, -1, 0, -1,
	0, 0, 1, 0,
	1, 1, 2, 1,
	2, 2, 3, 2,
	3, 3, 4, 3,
	4, 4, 5, 4,
	5, 5, 6, 5,
	6, 6, 7, 6,
	7, 7, 8, 7,
	99
    },
    {				/* LABYRINTH */
	-4, -4, -3, -4, -2, -4, -1, -4, 0, -4, 1, -4, 2, -4, 3, -4, 4, -4,
	-4, -3, 0, -3, 4, -3,
	-4, -2, -2, -2, -1, -2, 0, -2, 1, -2, 2, -2, 4, -2,
	-4, -1, -2, -1, 2, -1, 4, -1,
	-4, 0, -2, 0, -1, 0, 0, 0, 1, 0, 2, 0, 4, 0,
	-4, 1, -2, 1, 2, 1, 4, 1,
	-4, 2, -2, 2, -1, 2, 0, 2, 1, 2, 2, 2, 4, 2,
	-4, 3, 0, 3, 4, 3,
	-4, 4, -3, 4, -2, 4, -1, 4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4,
	99
    }
};

#define NPATS	(sizeof(patterns)/sizeof(patterns[0]))

static void
drawcell(row, col, age)
    unsigned char age;
{
    if (color)
	XSetForeground(Dsp, Gc, (unsigned long) age);
    if (pixels)
	XFillRectangle(Dsp, Win, Gc,
		       xb + xs * col, yb + ys * row, xs, ys);
    else
	XPutImage(Dsp, Win, Gc, &logo,
		  0, 0, xb + xs * col, yb + ys * row,
		  lifeicon_width, lifeicon_height);
}

static void
erasecell(row, col)
{
    XFillRectangle(Dsp, Win, eraseGC,
		   xb + xs * col, yb + ys * row, xs, ys);
}

static void
spawn(loc)
    unsigned char *loc;
{
    *(loc - NCOLS - 2 - 1) |= UPLT;
    *(loc - NCOLS - 2) |= UP;
    *(loc - NCOLS - 2 + 1) |= UPRT;
    *(loc - 1) |= LT;
    *(loc + 1) |= RT;
    *(loc + NCOLS + 2 - 1) |= DNLT;
    *(loc + NCOLS + 2) |= DN;
    *(loc + NCOLS + 2 + 1) |= DNRT;
    *(agebuf + (loc - buffer)) = 0;
}

static void
kill(loc)
    unsigned char *loc;
{
    *(loc - NCOLS - 2 - 1) &= ~UPLT;
    *(loc - NCOLS - 2) &= ~UP;
    *(loc - NCOLS - 2 + 1) &= ~UPRT;
    *(loc - 1) &= ~LT;
    *(loc + 1) &= ~RT;
    *(loc + NCOLS + 2 - 1) &= ~DNLT;
    *(loc + NCOLS + 2) &= ~DN;
    *(loc + NCOLS + 2 + 1) &= ~DNRT;
}

static void
setcell(row, col)
{
    register unsigned char *loc;

    loc = buffer + ((row + 1) * (NCOLS + 2)) + col + 1;
    spawn(loc);
    drawcell(row, col, 0);
}

static long
seconds()
{
    struct timeval foo;

    gettimeofday(&foo);
    return foo.tv_sec;
}

void
drawlife()
{
    unsigned char *loc,
               *temploc;
    int         row,
                col,
                cells = 0;
    unsigned char fate;


    loc = buffer + NCOLS + 2 + 1;
    temploc = tempbuf;
    /* copy the first 2 rows to the tempbuf */
    bcopy(loc, temploc, NCOLS);
    bcopy(loc + NCOLS + 2, temploc + NCOLS, NCOLS);
    for (row = 0; row < NROWS; ++row) {
	for (col = 0; col < NCOLS; ++col) {
	    fate = fates[*temploc];
	    *temploc = *(loc + (NCOLS + 2) * 2);
	    switch (fate) {
	    case BIRTH:
		if (!(*(loc + 1) & RT)) {
		    spawn(loc);
		}
		/* NO BREAK */
	    case SAME:
		if (*(loc + 1) & RT) {
		    register unsigned char *ageptr;
		    register unsigned char age;

		    ++cells;
		    ageptr = agebuf + (loc - buffer);
		    age = *ageptr;
		    if (++age > 253)
			age = 0;
		    *ageptr = age;
		    drawcell(row, col, age);
		}
		break;
	    case DEATH:
		if (*(loc + 1) & RT) {
		    kill(loc);
		    erasecell(row, col);
		}
		break;
	    }
	    loc++;
	    temploc++;
	}
	loc += 2;
	if (temploc >= tempbuf + NCOLS * 2)
	    temploc = tempbuf;
    }
    XFlush(Dsp);
    usleep(delay * 1000);
    if (!cells)
	startTime = 0;
}

static void
init_fates()
{
    int         i,
                bits,
                neighbors;

    for (i = 0; i < 256; i++) {
	neighbors = 0;
	for (bits = i; bits; bits &= (bits - 1))
	    neighbors++;
	if (neighbors == 3)
	    fates[i] = BIRTH;
	else if (neighbors == 2)
	    fates[i] = SAME;
	else
	    fates[i] = DEATH;
    }
}

void
initlife(d, w, g, c, t, n)
    Display    *d;
    Window      w;
    GC          g;
    int         c,
                t,
                n;
{
    int         row,
                col;
    int        *patptr;
    XWindowAttributes xgwa;

    startTime = seconds();
    shooter = 0;
    Dsp = d;
    Win = w;
    Gc = g;
    color = c;
    timeout = t;
    delay = n;

    if (eraseGC == (GC) 0) {
	XGCValues   xgcv;

	xgcv.foreground = BlackPixel(Dsp, 0);
	eraseGC = XCreateGC(Dsp, Win, GCForeground, &xgcv);

	srandom(time((long *) 0));
	init_fates();
	logo.data = (char*) lifeicon_bits;
	logo.width = lifeicon_width;
	logo.height = lifeicon_height;
	logo.bytes_per_line = 4;
    }
    if (!color)
	XSetForeground(Dsp, Gc, WhitePixel(Dsp, 0));

    XGetWindowAttributes(Dsp, Win, &xgwa);
    width = xgwa.width;
    height = xgwa.height;
    xs = width / NCOLS;
    ys = height / NROWS;
    xb = (width - xs * NCOLS) / 2;
    yb = (height - ys * NROWS) / 2;
    pixels = (xs < lifeicon_width || ys < lifeicon_height);

    XFillRectangle(Dsp, Win, eraseGC, 0, 0, width, height);

    bzero(buffer, sizeof(buffer));
    patptr = &patterns[(random() >> 3) % NPATS][0];
    while ((col = *patptr++) != 99) {
	row = *patptr++;
	col += NCOLS / 2;
	row += NROWS / 2;
	setcell(row, col);
    }
    XFlush(Dsp);
    sleep(1);
}

int
lifedone()
{
    int         elapsedTime = seconds() - startTime;

    if (elapsedTime > timeout)
	return 1;
    if (!shooter && elapsedTime > timeout / 2) {
	setcell(0, 2);
	setcell(1, 2);
	setcell(2, 2);
	setcell(2, 1);
	setcell(1, 0);
	shooter = 1;
    }
    return 0;
}
