#ident	"@(#)xinfo:xinfo.c	1.1"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 xinfo.c (C source file)
	Acc: 575327074 Fri Mar 25 16:04:34 1988
	Mod: 575321475 Fri Mar 25 14:31:15 1988
	Sta: 575570341 Mon Mar 28 11:39:01 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	author:
		Ross Hilbert
		AT&T 12/09/87
************************************************************************/

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include "Xprint.h"
#include "Xinput.h"
#include "Xargs.h"

#define Addr(x)		((char*)&x)
/*
	parameters
*/
static int			verbose;
static int			display;
static int			window;
static int			fontlist;
static int			columns;
static char *			fontpat = (char*)0;
static char *			font = (char*)0;

static Option opts[] =
{
"Verbose",	"-v",		"yes",		Addr(verbose),	OptBoolean,	NULL,
"Terse",	"-t",		"no",		Addr(verbose),	OptInverse,	NULL,
"Columns",	"-c:",		"1",		Addr(columns),	OptInt,		"1:",
NULL,		"-d",		"no",		Addr(display),	OptBoolean,	NULL,
NULL,		"-w",		"no",		Addr(window),	OptBoolean,	NULL,
NULL,		"-f",		"no",		Addr(fontlist),	OptBoolean,	NULL,
NULL,		"-fp:",		NULL,		Addr(fontpat),	OptString,	NULL,
NULL,		"-fn:",		NULL,		Addr(font),	OptString,	NULL,
NULL,		NULL,		NULL,		NULL,		NULL,		NULL
};

static Display *		dpy;
static int			scr;
static Cursor			arrow;

static char *			PGM = (char *) 0;

void Error (message)
char *message;
{
    fprintf (stderr, "%s: %s\n", PGM, message);
    exit (1);
}

syntax ()
{
	fprintf (stderr, "%s\n",
	"usage: xinfo [-v | -t] [-d] [-w] [-f] [-fp pattern]");
	fprintf (stderr, "%s\n",
	"             [-c cols] [-fn name] [-display host:display]");
	exit (1);
}

void main (argc, argv)
int argc;
char **argv;
{
	int	count = 0;
	char *	displayname = ExtractDisplay (&argc, argv);

	PGM = argv[0];

	if (!(dpy = XOpenDisplay(displayname)))
		Error ("can't open display");

	/* The following call is being made to initialize */
	/* dpy->keysyms_per_keycode. Otherwise, it's a NOP. */
	(void)XKeysymToKeycode (dpy, XK_BackSpace);

	if (ExtractOptions (dpy, opts, &argc, argv))
		syntax ();

	if (argc != 1)
		syntax ();

	scr = DefaultScreen (dpy);
	arrow = XCreateFontCursor (dpy, XC_arrow);

	if (window)
	{
		do_window ();
		++count;
	}
	if (font)
	{
		do_font ();
		++count;
	}
	if (fontlist || fontpat)
	{
		do_fontlist ();
		++count;
	}
	if (display || !count)
		do_display ();
}

do_window ()
{
	char *			name;
	XWindowAttributes	x;
	Window			w;

	fprintf (stderr, "Select Target Window\n");
	w = GetWindow (dpy, scr, arrow);
	FlashWindow (dpy, w);
	XGetWindowAttributes (dpy, w, &x);
	XFetchName (dpy, w, &name);

	if (name)
		printf ("Window = 0x%lx (%s)\n", w, name);
	else
		printf ("Window = 0x%lx\n", w);

	pXWindowAttributes (&x, verbose);
}

do_font ()
{
	XFontStruct * fs = XLoadQueryFont (dpy, font);

	if (fs)
	{
		printf ("%s\n", font);
		pXFontStruct (fs, verbose);
	}
	else
		Error ("bad font name");
}

do_fontlist ()
{
	int		i, j, k, n, w, x;
	char *		pattern = fontpat ? fontpat : "*";
	char **		names = XListFonts (dpy, pattern, 999, &n);

	if (!names)
		Error ("bad return from XListFonts");

	if (columns == 1)
		for (i = 0; i < n; ++i)
			printf ("%s\n", *names++);
	else
	{
		int rows = (n-1)/columns + 1;

		for (i = 0, w = 0; i < n; ++i)
			if ((x = strlen(names[i])) > w)
				w = x;

		for (i = 0; i < rows; ++i)
		{
			for (j = 0; j < columns; ++j)
				if ((k = i+j*rows) < n)
					printf ("%*s", -(w+1), names[k]);
			printf ("\n");
		}
	}
}

do_display ()
{
	printf ("Display Attributes\n");
	pDisplay (dpy, verbose);
}

