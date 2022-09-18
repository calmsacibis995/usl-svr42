#pragma ident	"@(#)train:passenger.c	1.4"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include	<stdio.h>
#include	<X11/Xlib.h>
#include	<X11/Xutil.h>
#include	<X11/Xatom.h>
#include	<X11/cursorfont.h>
#include	<Xos.h>
#include	<X11/keysym.h>
#include	<setjmp.h>
#include	"train.h"

static int	passengers_delivered;
int		passenger_src = PASSENGER_NONE;
int		passenger_dst = PASSENGER_NONE;

static int	wave = WAVE_IN;
static unsigned long	track_color = 0L;
static int	starting_time = 60;
static XSizeHints	size_hints;
static Colormap		cmap;

extern int	train_pos[];
extern int	train_len[];
extern int	level;
extern int	num_active_trains;
extern Window	train_windows [NUM_TRAINS] [MAX_TRAIN_LEN];
extern jmp_buf	env;
extern Window	window1, window2, score_window;
extern GC	stn_GC, pas_GC;
extern Display	*dpy;
extern int	the_screen;
extern Window	root_window;
extern XColor	my_gray, my_red, my_black, my_blue, my_green, my_yellow;
extern int	t_pause;
extern Pixmap	pas1_pixmap;
extern Pixmap	pas2_pixmap;
extern Pixmap	arm_pixmap;
extern station_type	stn_list[];
extern int	time_count;
extern int	passenger_train;
extern int	monochrome_mode, demo_mode;

TryToCreateRandomPassenger()
{
	int i;
	if (passenger_src != PASSENGER_NONE)
		return;

AGAIN:
	passenger_src = GetRandom(4);
	passenger_dst = GetRandom(4);
	passenger_train = -1;
	if (passenger_src == passenger_dst)
		goto AGAIN;
	for (i = 0; i < num_active_trains; i++) {
		if (train_pos[i] == stn_list[passenger_src].tile)
			goto AGAIN;
	}
	time_count = starting_time;

	if (monochrome_mode) 
		DrawStation(window1,passenger_dst);
}

TryToPickUpPassenger(num)
int num;
{
	int i;

	if (	passenger_src < 0 || 
		train_pos[num] != stn_list[passenger_src].tile) {
		return;
	}

	UndrawPassenger();
	for (i = 0; i < train_len[num]; i++) {
		XSetWindowBackground(
				dpy,
				train_windows[num][i],
				stn_list[passenger_dst].pixel);
		XClearWindow(
				dpy,
				train_windows[num][i]);
	}
	passenger_src = PASSENGER_RIDING;
	passenger_train = num;
}


TryToDropOffPassenger(num)
int num;
{
	XEvent ev;
	int i;

	if (	passenger_src != PASSENGER_RIDING ||
		passenger_train != num ||
		train_pos[num] != stn_list[passenger_dst].tile) {

		return;
	}


	for (i = 0; i < train_len[num]; i++) {
		XSetWindowBackground(
				dpy,
				train_windows[num][i],
				my_black.pixel);
		XClearWindow(
				dpy,
				train_windows[num][i]);
	}
	if (monochrome_mode) 
		UnDrawStation(window1,passenger_dst);
	passenger_src = PASSENGER_NONE;
	passenger_dst = PASSENGER_NONE;
	passenger_train = -1;
	AddToPassengerArea();
	UpdateScore((time_count > 0) ? time_count + level*PASSENGER_VALUE : 0);

	if (++passengers_delivered == PASSENGERS_FOR_NEXT_LEVEL)
	{
		passengers_delivered = 0;
		starting_time -= TIME_DECREASE;
		if (starting_time < MIN_TIME)
			starting_time = MIN_TIME;

		if (!demo_mode) {
			t_pause -= TIME_DECREASE;
			if (t_pause < MIN_PAUSE)
				t_pause = MIN_PAUSE;
		}


		FreeSubWindows();

		level++;
		if (monochrome_mode) {
			track_color = my_black.pixel;
		}

		else {
			track_color = (5 * level) % 16;
			while (	track_color == my_gray.pixel ||
				track_color == WhitePixel(dpy,the_screen)) {
				track_color = (track_color + 1) % 16;
			}
		}


		MakeSubWindows(	window2, 
				track_color,
				WhitePixel(dpy,the_screen),
				my_gray.pixel);

		DisplayLevel(level);

		XMapSubwindows(dpy, window2);

		if (!monochrome_mode) {
			for (i = 0; i < 4; i++) {
				DrawStation(window1,i);
			}
		}
	}
}

/*
**	Update a waving passenger
*/

UpdatePassenger()
{
	if (passenger_src == PASSENGER_RIDING)
		return;

	if (passenger_src < 0)
		return;

	XSetStipple(dpy, pas_GC, arm_pixmap);
	XSetForeground(dpy, pas_GC, my_gray.pixel);
	XSetTSOrigin(dpy, pas_GC, 
		stn_list[passenger_src].pasx + XOFWIN(stn_list[passenger_src].tile), 
		stn_list[passenger_src].pasy + YOFWIN(stn_list[passenger_src].tile));
	XFillRectangle(dpy, window1, pas_GC, 
		stn_list[passenger_src].pasx + XOFWIN(stn_list[passenger_src].tile), 
		stn_list[passenger_src].pasy + YOFWIN(stn_list[passenger_src].tile),
		arm_width, arm_height);

	if (wave == WAVE_IN)
	{
		XSetStipple(dpy, pas_GC, pas1_pixmap);
		wave = WAVE_OUT;
	}
	else if (wave == WAVE_OUT)
	{
		XSetStipple(dpy, pas_GC, pas2_pixmap);
		wave = WAVE_IN;
	}

	XSetForeground(dpy, pas_GC, stn_list[passenger_dst].pixel);
	XFillRectangle(dpy, window1, pas_GC, 
		stn_list[passenger_src].pasx + XOFWIN(stn_list[passenger_src].tile), 
		stn_list[passenger_src].pasy + YOFWIN(stn_list[passenger_src].tile),
		pas1_width, pas1_height);
}


UndrawPassenger()
{
	XSetTSOrigin(dpy, pas_GC, 
		stn_list[passenger_src].pasx + 
				XOFWIN(stn_list[passenger_src].tile), 
		stn_list[passenger_src].pasy + 
				YOFWIN(stn_list[passenger_src].tile));

	XSetForeground(dpy, pas_GC, my_gray.pixel);

	XSetStipple(dpy, pas_GC, arm_pixmap);

	XFillRectangle(dpy, window1, pas_GC, 
		stn_list[passenger_src].pasx + 
				XOFWIN(stn_list[passenger_src].tile), 
		stn_list[passenger_src].pasy + 
				YOFWIN(stn_list[passenger_src].tile),
		arm_width, arm_height);

	XSetStipple(dpy, pas_GC, pas1_pixmap);

	XFillRectangle(dpy, window1, pas_GC, 
		stn_list[passenger_src].pasx + 
				XOFWIN(stn_list[passenger_src].tile), 
		stn_list[passenger_src].pasy + 
				YOFWIN(stn_list[passenger_src].tile),
		pas1_width, pas1_height);
}



/*
**	Initialize GCs
*/
InitializeGCs()
{
	XGCValues	values;
	Pixmap	stn_pixmap;

#ifdef __STDC__
	stn_pixmap = XCreateBitmapFromData(
			dpy,
			root_window,
			(const char *)stn_bits, stn_width, stn_height);

	pas1_pixmap = XCreateBitmapFromData(
			dpy,
			root_window,
			(const char *) pas1_bits, pas1_width, pas1_height);

	pas2_pixmap = XCreateBitmapFromData(
			dpy,
			root_window,
			(const char *) pas2_bits, pas2_width, pas2_height);

	arm_pixmap = XCreateBitmapFromData(
			dpy,
			root_window,
			(const char *)arm_bits, arm_width, arm_height);

#else
	stn_pixmap = XCreateBitmapFromData(
			dpy,
			root_window,
			stn_bits, stn_width, stn_height);

	pas1_pixmap = XCreateBitmapFromData(
			dpy,
			root_window,
			pas1_bits, pas1_width, pas1_height);

	pas2_pixmap = XCreateBitmapFromData(
			dpy,
			root_window,
			pas2_bits, pas2_width, pas2_height);

	arm_pixmap = XCreateBitmapFromData(
			dpy,
			root_window,
			(char *)arm_bits, arm_width, arm_height);
#endif

	values.fill_style = FillStippled;
	values.stipple = stn_pixmap;
	values.subwindow_mode = IncludeInferiors;

	stn_GC = XCreateGC(
			dpy,
			root_window,
			GCFillStyle | GCStipple | GCSubwindowMode,
			&values);

	values.stipple = pas1_pixmap;

	pas_GC = XCreateGC(
			dpy,
			root_window,
			GCFillStyle | GCStipple | GCSubwindowMode,
			&values);
}

AddCarToTrain(num)
int num;
{
	unsigned long	valuemask;
	XSetWindowAttributes	attributes;

	valuemask = CWBackPixel;
	attributes.background_pixel = my_black.pixel;

	train_windows[num][train_len[num]] = XCreateWindow(
		dpy, window2,
		-1000,-1000,
		TRAIN_WIDTH, TRAIN_HEIGHT,
		0,
		CopyFromParent,
		CopyFromParent,
		CopyFromParent,
		valuemask,
		&attributes);

	XSync(dpy, False);
	XClearWindow(	dpy,
			train_windows[num][train_len[num]]);

	XMapWindow(dpy, train_windows[num][train_len[num]]);
	train_len[num]++;
}



/*
** Draw train station
*/

DrawStation(loc_window, i)
Window	loc_window;
int i;
{

	if (i < 0)
		return;

	XSetForeground(dpy, stn_GC, stn_list[i].pixel);
	XSetTSOrigin(dpy, stn_GC, 
		stn_list[i].stnx + XOFWIN(stn_list[i].tile), 
		stn_list[i].stny + YOFWIN(stn_list[i].tile));
	XFillRectangle(dpy, loc_window, stn_GC, 
		stn_list[i].stnx + XOFWIN(stn_list[i].tile), 
		stn_list[i].stny + YOFWIN(stn_list[i].tile),
		stn_width, stn_height);
}

UnDrawStation(loc_window, i)
Window	loc_window;
int i;
{

	XSetForeground(dpy, stn_GC, my_gray.pixel);
	XSetTSOrigin(dpy, stn_GC, 
		stn_list[i].stnx + XOFWIN(stn_list[i].tile), 
		stn_list[i].stny + YOFWIN(stn_list[i].tile));
	XFillRectangle(dpy, loc_window, stn_GC, 
		stn_list[i].stnx + XOFWIN(stn_list[i].tile), 
		stn_list[i].stny + YOFWIN(stn_list[i].tile),
		stn_width, stn_height);
}


InitializeOneTime(argc,argv)
int argc;
char **argv;
{
	XSetWindowAttributes	attributes;
	unsigned long	valuemask = NULL;
	XWMHints	xwmh;
	Atom	WM_PROTOCOLS, WM_TAKE_FOCUS;
	int	i;

	for (i = 1; i < argc; i++) {

		if (!strcmp(argv[i],"-d")) {
			printf ("DEMO MODE\n");
			demo_mode = True;
		}

		else if (!strcmp(argv[i],"-m")) {
			printf ("MONOCHROME MODE\n");
			monochrome_mode = True;
		}

		else {
			printf ("Usage:	%s [-d] [-m]\n",argv[0]);
			exit (-1);
		}
	}

	if ((dpy = XOpenDisplay("")) == NULL) {
		printf ("can't open display\n");
		exit(-1);
	}

	the_screen = DefaultScreen(dpy);
	root_window = DefaultRootWindow(dpy);
	cmap = DefaultColormap(dpy, DefaultScreen(dpy));
	monochrome_mode = monochrome_mode | (DisplayCells(dpy,the_screen) == 2);

	XParseColor(dpy, cmap, "black", &my_black);
	XAllocColor(dpy, cmap, &my_black);

	if (monochrome_mode) {
		XParseColor(dpy, cmap, "white", &my_gray);
		XAllocColor(dpy, cmap, &my_gray);

		XParseColor(dpy, cmap, "black", &my_red);
		XAllocColor(dpy, cmap, &my_red);
		stn_list[0].pixel = my_red.pixel;

		XParseColor(dpy, cmap, "black", &my_blue);
		XAllocColor(dpy, cmap, &my_blue);
		stn_list[1].pixel = my_blue.pixel;

		XParseColor(dpy, cmap, "black", &my_green);
		XAllocColor(dpy, cmap, &my_green);
		stn_list[2].pixel = my_green.pixel;

		XParseColor(dpy, cmap, "black", &my_yellow);
		XAllocColor(dpy, cmap, &my_yellow);
		stn_list[3].pixel = my_yellow.pixel;
	}

	else {
		XParseColor(dpy, cmap, "gray", &my_gray);
		XAllocColor(dpy, cmap, &my_gray);

		XParseColor(dpy, cmap, "red", &my_red);
		XAllocColor(dpy, cmap, &my_red);
		stn_list[0].pixel = my_red.pixel;

		XParseColor(dpy, cmap, "blue", &my_blue);
		XAllocColor(dpy, cmap, &my_blue);
		stn_list[1].pixel = my_blue.pixel;

		XParseColor(dpy, cmap, "darkgreen", &my_green);
		XAllocColor(dpy, cmap, &my_green);
		stn_list[2].pixel = my_green.pixel;

		XParseColor(dpy, cmap, "yellow", &my_yellow);
		XAllocColor(dpy, cmap, &my_yellow);
		stn_list[3].pixel = my_yellow.pixel;
	}

	window1 = XCreateWindow(
			dpy, root_window,
			0, 0,
			TILE_SIZE * (GRID_SIZE - 1), 
			TILE_SIZE * (GRID_SIZE - 1) + PANEL_HEIGHT,
			BORDER_WIDTH, 
			CopyFromParent,
			CopyFromParent,
			CopyFromParent,
			NULL, NULL);

	xwmh.flags = InputHint | IconPixmapHint;
	xwmh.input = False;
#ifdef __STDC__
	xwmh.icon_pixmap = XCreateBitmapFromData (
			dpy,
			window1,
			(const char *)train_bits,
			train_width,
			train_height);
#else
	xwmh.icon_pixmap = XCreateBitmapFromData (
			dpy,
			window1,
			train_bits,
			train_width,
			train_height);
#endif
	XSetWMHints( dpy, window1, &xwmh);

	WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
	WM_TAKE_FOCUS = XInternAtom(dpy, "WM_TAKE_FOCUS", False);

#ifdef __STDC__
	XChangeProperty(dpy, window1, WM_PROTOCOLS, XA_ATOM, 32,
				PropModeReplace, (const unsigned char *) &WM_TAKE_FOCUS, 1L);
#else

	XChangeProperty(dpy, window1, WM_PROTOCOLS, XA_ATOM, 32,
				PropModeReplace, (char *) &WM_TAKE_FOCUS, 1L);

#endif
	size_hints.x = 0 - (TILE_SIZE >> 1);
	size_hints.y = 0 - (TILE_SIZE >> 1);
	size_hints.width = TILE_SIZE * (GRID_SIZE - 1);
	size_hints.height = TILE_SIZE * (GRID_SIZE - 1) + PANEL_HEIGHT;
	size_hints.max_width = size_hints.min_width = size_hints.width;
	size_hints.max_height = size_hints.min_height = size_hints.height;
	size_hints.flags = PSize | PPosition | PMinSize | PMaxSize;

	XSetStandardProperties(
			dpy, window1,
			/* "Software Engineer", "Ding!",  ul92-05857 */
                        "Train Game", "Train",
			None, argv, argc, &size_hints);
	
	XSelectInput(
			dpy, window1,
			ButtonPressMask);

	valuemask = CWBackPixel;
	attributes.background_pixel = my_black.pixel;

	window2 = XCreateWindow(
			dpy, window1,
			0 - (TILE_SIZE >> 1),
			0 - (TILE_SIZE >> 1),
			TILE_SIZE * GRID_SIZE,
			TILE_SIZE * GRID_SIZE,
			BORDER_WIDTH, 
			CopyFromParent,
			CopyFromParent,
			CopyFromParent,
			valuemask,
			&attributes);

	XSelectInput( dpy, window2, ButtonPressMask);

	valuemask = CWBackPixel;
	attributes.background_pixel = my_black.pixel;

	score_window = XCreateWindow(
			dpy, window1,
			0,
			TILE_SIZE * (GRID_SIZE - 1),
			TILE_SIZE * (GRID_SIZE - 1), 
			PANEL_HEIGHT,
			0, 
			CopyFromParent,
			CopyFromParent,
			CopyFromParent,
			valuemask,
			&attributes);

	XSelectInput( dpy, score_window, ExposureMask);


	InitializeGCs();
	InitializeFont();


}


InitializeEachGame()
{
	train_pos[0] = 8; train_pos[1] = 4;
	train_pos[2] = 17; train_pos[3] = 18;

	train_len[0] = 5; train_len[1] = 5;
	train_len[2] = 5; train_len[3] = 5;

	starting_time = 60;
	level = 1;
	num_active_trains = 1;
	if (!demo_mode) {
		t_pause = ORIG_PAUSE;
	}
	else {
		t_pause = 1;
	}

	passenger_src = PASSENGER_NONE;
	passenger_dst = PASSENGER_NONE;
	passenger_train = -1;
	passengers_delivered = 0;

	MakeSubWindows(	window2, 
			BlackPixel(dpy,the_screen),
			WhitePixel(dpy,the_screen),
			my_gray.pixel);

}


