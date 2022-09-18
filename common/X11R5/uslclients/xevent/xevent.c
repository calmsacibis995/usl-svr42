/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xevent:xevent.c	1.1"
#endif
/*
 xevent.c (C source file)
	Acc: 575326450 Fri Mar 25 15:54:10 1988
	Mod: 575321189 Fri Mar 25 14:26:29 1988
	Sta: 575570330 Mon Mar 28 11:38:50 1988
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
		AT&T 12/02/87
************************************************************************/

#include <stdio.h>
#include <Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "Xprint.h"
#include "Xinput.h"
#include "Xargs.h"
#include "matrix.h"


#ifdef SVR4
#define SELECT  select
#else
#define SELECT  pollselect
#endif

typedef struct
{
	int x, y;
}
	Point;

typedef struct
{
	int x, y, width, height;
}
	Rectangle;

typedef struct
{
	char *		name[2];
	int		value;
	void		(*f)();
	Point		offset[2];
	Point		origin;
}
	Command;

typedef struct
{
	char *		name;
	unsigned long	mask;
	Point		offset;
	Point		origin;
}
	Item;

typedef struct
{
	Window				window;
	char *				name;
	unsigned long			mask;
}
	EventWindow;

#define Addr(x)		((char*)&x)
/*
	parameters
*/
static int		abbrev;		/* abbreviated items?		*/
static int		center;		/* center items?		*/
static int		rowmajor;	/* row major display?		*/
static int		columns;	/* number of columns of items	*/
static int		borderwidth;	/* width of window border	*/
static int		padding;	/* padding for items/commands	*/
static XFontStruct *	fs;		/* font for items/commands	*/
static FILE *		logfile = NULL;	/* log file			*/
static unsigned long	foreground;	/* foreground pixel		*/
static unsigned long	background;	/* background pixel		*/
static unsigned long	border;		/* border pixel			*/

static Option opts[] =
{
"Abbreviate",	"-abbrev",	"no",	Addr(abbrev),		OptBoolean,	NULL,
"Normal",	"-normal",	NULL,	Addr(abbrev),		OptInverse,	NULL,
"Center",	"-center",	"no",	Addr(center),		OptBoolean,	NULL,
"Left",		"-left",	NULL,	Addr(center),		OptInverse,	NULL,
"RowMajor",	"-rowmajor",	"no",	Addr(rowmajor),		OptBoolean,	NULL,
"ColMajor",	"-colmajor",	NULL,	Addr(rowmajor),		OptInverse,	NULL,
"Columns",	"-columns:",	"2",	Addr(columns),		OptInt,		"1:",
NULL,		"-c:",		NULL,	Addr(columns),		OptInt,		"1:",
"BorderWidth",	"-bw:",		"4",	Addr(borderwidth),	OptInt,		"0:",
"Padding",	"-pad:",	"3",	Addr(padding),		OptInt,		"1:",
"Font",		"-fn:",		"8x13",	Addr(fs),		OptFont,	NULL,
"Log",		"-log:",	NULL,	Addr(logfile),		OptFILE,	"w",
"Foreground",	"-fg:",		"black",Addr(foreground),	OptColor,	NULL,
"Background",	"-bg:",		"white",Addr(background),	OptColor,	NULL,
"Border",	"-bd:",		"black",Addr(border),		OptColor,	NULL,
NULL,		NULL,		NULL,	NULL,			NULL,		NULL,
};
/*
	globals
*/
static Window		root;
static Window		win;
static Display *	dpy;
static int		scr;
static Colormap		cmap;
static GC		gc;
static GC		gc_inverse;
static Cursor		arrow;
static Cursor		hand;

static char *		PGM;
static char *		selectprompt = "Select Target Window";
static int		EXIT = 0;

static int		command_count = 0;
static int		command_width = 0;
static int		command_height = 0;
static int		command_area = 0;

static int		item_count = 0;
static int		item_width = 0;
static int		item_height = 0;
static MatrixAttributes	item_matrix;

void			Error ();
Window			get_window ();
void			set_current ();
void			print_event ();
void			flush_expose ();
void			service_events ();
void			calculate_size ();
void			display_monitor ();
void			display_attributes ();
void			service_button ();
void			service_item ();
void			service_command ();
int			PointInCommand ();
void			change_current ();
void			do_window ();
void			do_parent ();
void			do_clear ();
void			do_power ();
void			do_format ();
void			do_quit ();
void			toggle_command ();
void			draw_item ();
int			flip_mask ();
void			draw_outline ();
void			xe_pause ();
/*
	event mask items
*/
static Item items[] =
{
	{ "ButtonPress",		(unsigned long) ButtonPressMask },
	{ "ButtonRelease",		(unsigned long) ButtonReleaseMask },
	{ "ButtonMotion",		(unsigned long) ButtonMotionMask },
	{ "Button1Motion",		(unsigned long) Button1MotionMask },
	{ "Button2Motion",		(unsigned long) Button2MotionMask },
	{ "Button3Motion",		(unsigned long) Button3MotionMask },
	{ "Button4Motion",		(unsigned long) Button4MotionMask },
	{ "Button5Motion",		(unsigned long) Button5MotionMask },
	{ "ColormapChange",		(unsigned long) ColormapChangeMask },
	{ "EnterWindow",		(unsigned long) EnterWindowMask },
	{ "Exposure",			(unsigned long) ExposureMask },
	{ "FocusChange",		(unsigned long) FocusChangeMask },
	{ "KeyPress",			(unsigned long) KeyPressMask },
	{ "KeyRelease",			(unsigned long) KeyReleaseMask },
	{ "KeymapState",		(unsigned long) KeymapStateMask },
	{ "LeaveWindow",		(unsigned long) LeaveWindowMask },
	{ "OwnerGrabButton",		(unsigned long) OwnerGrabButtonMask },
	{ "PointerMotion",		(unsigned long) PointerMotionMask },
	{ "PointerMotionHint",		(unsigned long) PointerMotionHintMask },
	{ "PropertyChange",		(unsigned long) PropertyChangeMask },
	{ "ResizeRedirect",		(unsigned long) ResizeRedirectMask },
	{ "StructureNotify",		(unsigned long) StructureNotifyMask },
	{ "SubstructureNotify",		(unsigned long) SubstructureNotifyMask },
	{ "SubstructureRedirect",	(unsigned long) SubstructureRedirectMask },
	{ "VisibilityChange",		(unsigned long) VisibilityChangeMask },
	{ (char *) 0,			(unsigned long) 0 },
};
static char * short_items[] =
{
	"ButPress",
	"ButRelse",
	"ButMot",
	"But1Mot",
	"But2Mot",
	"But3Mot",
	"But4Mot",
	"But5Mot",
	"ClrmpChg",
	"EnterWin",
	"Exposure",
	"FocusChg",
	"KeyPress",
	"KeyRelse",
	"KeymapSt",
	"LeaveWin",
	"OwnrGrbB",
	"PtrMot",
	"PtrMotHt",
	"PropChg",
	"RszRedir",
	"StrNotfy",
	"SubNotfy",
	"SubRedir",
	"VisChg",
	(char *) 0,
};
/*
	command buttons
*/
static Command commands[] =
{
	{ { "Window",	(char *)0 },	0,	do_window   },
	{ { "Parent",	(char *)0 },	0,	do_parent   },
	{ { "Clear",	(char *)0 },	0,	do_clear    },
	{ { "LogOn",	"LogOff"  },	1,	do_power    },
	{ { "Long",	"Short"   },	1,	do_format   },
	{ { "Quit",	(char *)0 },	0,	do_quit     },
	{ { (char *)0,	(char *)0 },	0,	(void(*)())0 },
};
#define Power		3
#define Format		4
#define POWER		(commands[Power].value)
#define FORMAT		(commands[Format].value)

static char * short_commands[] =
{
	"Win",		(char *)0,
	"Par",		(char *)0,
	"Clr",		(char *)0,
	"On",		"Off",
	"Lg",		"Sh",
	"Q",		(char *)0,
	(char *)0,	(char *)0,
};
/*
	current window information
*/
static EventWindow current =
{
	(Window) 0,
	(char *) 0,
	(unsigned long) 0
};
/*
	special events that can only be selected once
*/
static unsigned long	special =	ButtonPressMask |
					SubstructureRedirectMask |
					ResizeRedirectMask;
/*
	write an error message and exit
*/
void Error (message)
char *message;
{
    fprintf (stderr, "%s: %s\n", PGM, message);
    exit (1);
}
/*
	put up a arrow cursor and return the window selected
*/
Window get_window (tag)
char * tag;
{
	Window w;

	printf ("%s\n", tag);
	w = GetWindow (dpy, scr, arrow);
	return w == win ? current.window : w;
}
/*
	set the current window to w
*/
void set_current (w)
Window w;
{
	XWindowAttributes x;

	current.window = w;
#ifdef NCR
	XGetWindowAttributes (dpy, w, &x);
	if (x.class != InputOnly) FlashWindow (dpy, w);
#else
	FlashWindow (dpy, w);
#endif
	XFetchName (dpy, w, &current.name);
#ifndef NCR
	XGetWindowAttributes (dpy, w, &x);
#endif
	current.mask = x.your_event_mask;
}
/*
	print event structure with optional log
*/
void print_event (p)
XEvent * p;
{
	if (POWER)
	{
		printf ("xevent\n");
		pXEvent (p, FORMAT);

		if (logfile)
		{
			fprintf (logfile, "xevent\n");
			fpXEvent (logfile, p, FORMAT);
		}
	}
}
/*
	flush remaining exposure events
*/
void flush_expose ()
{
	XEvent e;
	while (XCheckWindowEvent (dpy, win, Expose, &e))
	;
}
/*
	service pending events
*/
void service_events ()
{
	XEvent e;

	while (XPending (dpy))
	{
		XNextEvent (dpy, &e);

		if (e.xany.type == MappingNotify)
		{
			XRefreshKeyboardMapping ((XMappingEvent *) &e);
			e.xany.window = (Window) 0;
		}
		if (e.xany.window == win)
		{
			switch (e.xany.type)
			{
				case Expose:
					if (e.xexpose.count == 0)
					{
						flush_expose ();
						display_monitor ();
					}
					break;
				case ButtonPress:
					service_button (&e);
					break;
				case ButtonRelease:
					if (EXIT) exit (0);
					break;
				default:
					print_event (&e);
					break;
			}
		}
		else
			print_event (&e);
	}
}
static char * syntax_msg[] =
{
	"usage: xevent [-fg foreground] [-bg background] [-bd border]",
	"              [-bw borderwidth] [-fn fontname] [-c columns]",
	"              [-pad padding] [-log logfile] [-colmajor | -rowmajor]",
	"              [-normal | -abbrev] [-left | -center]",
	"              [-geometry geometry] [-display host:display]",
	(char *) 0,
};

syntax ()
{
	int i;

	for (i = 0; syntax_msg[i]; ++i)
		fprintf (stderr, "%s\n", syntax_msg[i]);
	exit (1);
}

/*
	event moitor
*/
void main (argc, argv)
int argc;
char **argv;
{
	XSizeHints	hints;
	char *		display = ExtractDisplay (&argc, argv);
	char *		geometry = ExtractGeometry (&argc, argv);

	PGM = argv[0];
	/*
	 *	open display
	 */
	if (!(dpy = XOpenDisplay(display)))
		Error ("can't open display");
	/*
	 *	extract and validate arguments
	 */
	if (ExtractOptions (dpy, opts, &argc, argv))
		syntax ();

	if (argc != 1)
		syntax ();
	/*
	 *	set global variables
	 */
	scr = DefaultScreen (dpy);
	cmap = DefaultColormap (dpy, scr);
	root = DefaultRootWindow (dpy);
	arrow = XCreateFontCursor (dpy, XC_arrow);
	hand = XCreateFontCursor (dpy, XC_hand1);
	gc = DefaultGC (dpy, scr);
	XSetFont (dpy, gc, fs->fid);
	XSetForeground (dpy, gc, foreground);
	XSetBackground (dpy, gc, background);
	gc_inverse = XCreateGC (dpy, root, (unsigned long) 0, (XGCValues *)0);
	XSetFont (dpy, gc_inverse, fs->fid);
	XSetForeground (dpy, gc_inverse, background);
	XSetBackground (dpy, gc_inverse, foreground);
	/*
	 *	ask for selection window and display attributes
	 */
	set_current (get_window(selectprompt));
	display_attributes ();
	/*
	 *	calculate sizes for everything and get window size
	 */
	calculate_size (&hints.width, &hints.height);
	/*
	 *	set up hints
	 */
	hints.x = 0;
	hints.y = 0;
	hints.min_width = hints.max_width = hints.width;
	hints.min_height = hints.max_height = hints.height;
	hints.flags = PPosition | PSize | PMinSize | PMaxSize;
	/*
	 *	check for geometry
	 */
	if (geometry)
	{
		int t;
		int flags = XParseGeometry (geometry,&hints.x,&hints.y,(unsigned int *)&t,(unsigned int *)&t);

		if ((flags & XValue) && (flags & YValue))
		{
			hints.flags &= ~PPosition;
			hints.flags |= USPosition;
			hints.flags &= ~PSize;
			hints.flags |= USSize;

			if (flags & XNegative)
				hints.x += DisplayWidth (dpy, scr) -
					hints.width - 2*borderwidth;
			if (flags & YNegative)
				hints.y += DisplayHeight (dpy, scr) -
					hints.height - 2*borderwidth;
		}
	}
	/*
	 *	create window, set properties, select input, and map window
	 */
	win = XCreateSimpleWindow (dpy, root,
		hints.x, hints.y, hints.width, hints.height,
		borderwidth, border, background);
	XSetStandardProperties (dpy, win, "event monitor", "event monitor",
		None, argv, argc, &hints);
	XSelectInput (dpy, win, ButtonPressMask|ButtonReleaseMask|ExposureMask);
	XDefineCursor (dpy, win, hand);
	XMapWindow (dpy, win);
	/*
	 *	service loop
	 */
	while (1) {
		service_events ();
		xe_pause ();
	}
}
/*
	calculate sizes for item/command buttons
*/
void calculate_size (window_width, window_height)
int * window_width;
int * window_height;
{
	int i, row, col, x, y, w;
	int commands_per_row, command_rows;
	int xstart, ystart, pad;
	char * p;

	if (abbrev)
	{
		for (i = 0; items[i].name; ++i)
			items[i].name = short_items[i];

		for (i = 0; commands[i].name[0]; ++i)
		{
			commands[i].name[0] = short_commands[2*i];
			commands[i].name[1] = short_commands[2*i+1];
		}
	}
	item_height = command_height = fs->ascent + fs->descent + padding*2;

	for (i = 0; p = items[i].name; ++i)
	{
		w = XTextWidth (fs, p, strlen(p));
		if (item_width < w)
			item_width = w;
	}
	item_count = i;
	item_width += padding*2;

        for (i = 0; p = commands[i].name[0]; ++i)
        {
                w = XTextWidth (fs, p, strlen(p));
                if (command_width < w) command_width = w;
		
		if (p = commands[i].name[1])
		{
			w = XTextWidth (fs, p, strlen(p));
			if (command_width < w) command_width = w;
		}
        }
	command_count = i;
	command_width += (padding*2 + command_height);
	commands_per_row = (item_width*columns)/command_width;

	if (! commands_per_row)
	{
		commands_per_row = 1;
		item_width = (command_width-1)/columns+1;
	}
	command_rows = (command_count-1)/commands_per_row+1;
	commands_per_row = (command_count-1)/command_rows+1;
	command_area = (command_height+padding)*((command_count-1)/commands_per_row+1);

	item_matrix.rows = (item_count - 1)/columns + 1;
	item_matrix.cols = columns;
	item_matrix.width = item_width;
	item_matrix.height = item_height;
	item_matrix.pad = padding;
	item_matrix.order = rowmajor ? ROW_MAJOR : COL_MAJOR;

	for (i = 0; p = items[i].name; ++i)
	{
		w = XTextWidth (fs, p, strlen(p));

		if (center)
			items[i].offset.x = item_width/2 - w/2;
		else
			items[i].offset.x = padding;

		GetCellAddress (&item_matrix, i, &row, &col);
		GetCellOrigin (&item_matrix, row, col, &x, &y);
		items[i].offset.y = padding + fs->ascent;
		items[i].origin.x = x;
		items[i].origin.y = y + command_area;
	}
	GetMatrixDimensions (&item_matrix, &x, &y);
	*window_width = x;
	*window_height = y + command_area;

	for (i = 0; p = commands[i].name[0]; ++i)
	{
		w = XTextWidth (fs, p, strlen(p));
		commands[i].offset[0].x = command_width/2 - w/2;
		commands[i].offset[0].y = padding + fs->ascent;
		
		if (p = commands[i].name[1])
		{
			w = XTextWidth (fs, p, strlen(p));
			commands[i].offset[1].x = command_width/2 - w/2;
			commands[i].offset[1].y = padding + fs->ascent;
		}
	}
	if (commands_per_row > 1)
	{
		pad = (x-padding*2-command_width*commands_per_row)/(commands_per_row-1);
		xstart = padding;
		ystart = padding;
	}
	else
	{
		pad = 0;
		xstart = padding + (x-padding*2-command_width)/2;
		ystart = padding;
	}
	x = xstart;
	y = ystart;

	for (i = 0; i < command_count; ++i)
	{
		commands[i].origin.x = x;
		commands[i].origin.y = y;
		x += (command_width + pad);

		if (((i+1) % commands_per_row) == 0)
		{
			x = xstart;
			y += (command_height + padding);
		}
	}
}
/*
	display the monitor window
*/
void display_monitor ()
{
	int i;
	int v, x, y;

	XClearWindow (dpy, win);

	for (i = 0; i < command_count; ++i)
	{
		v = commands[i].value;
		x = commands[i].origin.x + commands[i].offset[v].x;
		y = commands[i].origin.y + commands[i].offset[v].y;
		XDrawString (dpy, win, gc, x, y,
			commands[i].name[v], strlen(commands[i].name[v]));
		draw_outline (commands[i].origin.x, commands[i].origin.y,
			command_width, command_height);
	}
	for (i = 0; i < item_count; ++i)
		draw_item (i);
}
/*
	print the current window attributes
*/
void display_attributes ()
{
	char buf[128];

	XWindowAttributes x;
	XGetWindowAttributes (dpy, current.window, &x);

	if (current.name)
		sprintf (buf, "Window = 0x%lx (%s)", current.window, current.name);
	else
		sprintf (buf, "Window = 0x%lx", current.window);

	printf ("%s\n", buf);
	pXWindowAttributes (&x, FORMAT);

	if (logfile)
	{
		fprintf (logfile, "%s\n", buf);
		fpXWindowAttributes (logfile, &x, FORMAT);
	}
}
/*
	service a button press on the monitor window
*/
void service_button (event)
XEvent * event;
{
	XButtonEvent * e = (XButtonEvent *) event;

	if (e->y < command_area)
		service_command (e->x, e->y);
	else
		service_item (e->x, e->y-command_area);
}
/*
	service a mask toggle request
*/
void service_item (x, y)
int x, y;
{
	int	i, row, col;

	GetCellAtPoint (&item_matrix, x, y, &row, &col);
	GetCellIndex (&item_matrix, row, col, &i);

	if (flip_mask (i))
		draw_item (i);
	else
		XBell (dpy, 30);
}
/*
	service a command request
*/
void service_command (x, y)
int x, y;
{
	int i;

	for (i = 0; i < command_count; ++i)
		if (PointInCommand (x, y, i))
			break;

	if (commands[i].f)
		(*commands[i].f)();
}
/*
	check if point (x, y) is in command i
*/
int PointInCommand (x, y, i)
int x, y;
int i;
{
	if (	x < commands[i].origin.x
	||	x > commands[i].origin.x+command_width
	||	y < commands[i].origin.y
	||	y > commands[i].origin.y+command_height)
		return 0;
	return 1;
}
/*
	change current to new window
*/
void change_current (w)
Window w;
{
	int mask = current.mask;
	int i;

	if (current.name)
		XFree (current.name);
	current.mask = 0;

	for (i = 0; i < item_count; ++i)
		if (mask & items[i].mask)
			draw_item (i);
	set_current (w);
	for (i = 0; i < item_count; ++i)
		if (current.mask & items[i].mask)
			draw_item (i);
	display_attributes ();
}
/*
	select window to modify
*/
void do_window ()
{
	change_current (get_window (selectprompt));
}
/*
	set current window to parent
*/
void do_parent ()
{
	Window		parent;
	Window		r;
	Window *	c;
	int		n;

	if (current.window == root)
	{
		XBell (dpy, 30);
		return;
	}
	XQueryTree (dpy, current.window, &r, &parent, &c, (unsigned int *)&n);
	if (c) XFree ((char  *) c);
	change_current (parent);
}
/*
	clear events command
*/
void do_clear ()
{
	int mask = current.mask;
	int i;

	current.mask = 0;
	XSelectInput (dpy, current.window, current.mask);

	for (i = 0; i < item_count; ++i)
		if (mask & items[i].mask)
			draw_item (i);
}
/*
	toggle power switch
*/
void do_power ()
{
	toggle_command (Power);
}
/*
	toggle format switch
*/
void do_format ()
{
	toggle_command (Format);
}
/*
	quit command
*/
void do_quit ()
{
	EXIT = 1;
}
/*
	visually toggle command i
*/
void toggle_command (i)
int i;
{
	int v = commands[i].value;
	int x, y;

	v = v ? 0 : 1;
	x = commands[i].origin.x + commands[i].offset[v].x;
	y = commands[i].origin.y + commands[i].offset[v].y;
	XClearArea (dpy, win,
		commands[i].origin.x, commands[i].origin.y,
		command_width, command_height, False);
	XDrawString (dpy, win, gc, x, y,
		commands[i].name[v], strlen(commands[i].name[v]));
	draw_outline (commands[i].origin.x, commands[i].origin.y,
		command_width, command_height);
	commands[i].value = v;
}
/*
	draw item i
*/
void draw_item (i)
int		i;
{
	int	flag = (current.mask & items[i].mask);
	GC	rgc = flag ? gc : gc_inverse;
	GC	tgc = flag ? gc_inverse : gc;

	XFillRectangle (dpy, win, rgc,
		items[i].origin.x, items[i].origin.y,
		item_width, item_height);

	XDrawString (dpy, win, tgc,
		items[i].origin.x + items[i].offset.x,
		items[i].origin.y + items[i].offset.y,
		items[i].name, strlen(items[i].name));
}
/*
	add/delete item i's mask from current window's mask
*/
int flip_mask (i)
int i;
{
	XWindowAttributes x;

	if (i >= item_count)
		return 0;

	if (current.mask & items[i].mask)
		current.mask &= ~items[i].mask;
	else
	{
		if (items[i].mask & special)
		{
			XGetWindowAttributes (dpy, current.window, &x);

			if (x.all_event_masks & items[i].mask)
				return 0;
		}
		current.mask |= items[i].mask;
	}
	XSelectInput (dpy, current.window, current.mask);
	return 1;
}
/*
	draw recatngle with rounded corners
*/
void draw_outline (x, y, w, h)
int x, y, w, h;
{
	int r = h/2;
	XDrawArc (dpy, win, gc, x, y, h, h, 90*64, 180*64);
	XDrawLine (dpy, win, gc, x+r, y+h, x+w-r, y+h);
	XDrawArc (dpy, win, gc, x+w-h, y, h, h, -90*64, 180*64);
	XDrawLine (dpy, win, gc, x+w-r, y, x+r, y);
}
/*
	wait for event from server
*/
void xe_pause ()
{
	int maxfds = ConnectionNumber(dpy)+1;
	int readfds = 1 << ConnectionNumber(dpy);

	if (SELECT (maxfds, &readfds, NULL, NULL, NULL) == -1)
		Error ("select error");
}
