#pragma ident	"@(#)train:train.c	1.2"
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

/*
**
**  Software Engineer, aka "The Train Game"
**
**  By Andy Oakland and Bruce Barnett, AT&T, Summit NJ
*/

/*
**  The three different types of tiles, with a list of train positions
**  as it moves through the tile from each entrance point, and a list
**  of exit points.
*/

static tile_type tile_defs[3] =
	{	{{{{32,16}, {32,32}, {32,48}, {32,64}},
		  {{48,32}, {32,32}, {16,32}, { 0,32}},
		  {{32,48}, {32,32}, {32,16}, {32, 0}},
		  {{16,32}, {32,32}, {48,32}, {64,32}}},
		{2,3,0,1}},

		{{{{28,16}, {22,22}, {16,28}, { 0,32}},
		  {{48,36}, {42,42}, {36,48}, {32,64}},
		  {{36,48}, {42,42}, {48,36}, {64,32}},
		  {{16,28}, {22,22}, {28,16}, {32, 0}}},
		{3,2,1,0}},

		{{{{32,12}, {42,22}, {52,30}, {64,32}},
		  {{52,30}, {42,22}, {34,12}, {32, 0}},
		  {{30,52}, {22,42}, {12,34}, {0, 32}},
		  {{12,34}, {22,42}, {30,52}, {32,64}}},
		{1,0,3,2}},
	};

/*
**  Which tiles the stations are on, and where the station and passenger
**  should be displayed relative to these tiles.
*/

station_type stn_list[4] = 
{	{2,	20,	8,	10,	16},
	{17,	-32,	20,	-24,	8},
	{32,	20,	-32,	10,	-26},
	{12,	8,	20,	16,	8}
};

int	train_pos[NUM_TRAINS] = {15,4,17,18};
int	train_len[NUM_TRAINS] = {5,5,5,5};
int	level = 1;
int	num_active_trains = 1;
Window	train_windows [NUM_TRAINS] [MAX_TRAIN_LEN];
jmp_buf	env;
Window	window1, window2, score_window;
GC	stn_GC, pas_GC;
Display	*dpy;
int	the_screen;
Window	root_window;
XColor	my_gray, my_red, my_black, my_blue, my_green, my_yellow;
int	t_pause = ORIG_PAUSE;
Pixmap	pas1_pixmap;
Pixmap	pas2_pixmap;
Pixmap	arm_pixmap;
int	time_count;
int	passenger_train = -1;
int	demo_mode = False;
int	monochrome_mode = False;

extern int	passenger_src, passenger_dst;

static int	hole_pos;
static int	train_head[NUM_TRAINS];
static unsigned	seed = 1231;

Pixmap MakePixmap();

main(argc, argv)
int argc;
char **argv;
{
	XEvent	the_event;
	Timeval	t;
	int	new_time, last_time = 0;
	int	i, jump_status;
	static	timer = 0;



	InitializeOneTime(argc,argv);

	XMapSubwindows(dpy, window1);
	XMapWindow(dpy, window1);

	XWindowEvent(dpy, score_window, ExposureMask, &the_event);

	jump_status = setjmp(env);

	while (jump_status != 0) {
		jump_status = setjmp(env);
	}

	InitializeEachGame();

	TitleScreen();

	InitializeScore();

	XMapSubwindows(dpy, window2);

	gettimeofday(&t);
	seed = t.tv_sec;

	if (!monochrome_mode) {
		for (i = 0; i < 4; i++)
			DrawStation(window1,i);
	}
	else {
		DrawStation(window1,passenger_dst);
	}

	UpdateScore(0);

	while (1) {
		for (i = 0; i < num_active_trains; i++) {


			if (	XPending(dpy) && 
				XCheckMaskEvent(
					dpy,
					ButtonPressMask | ExposureMask,
					&the_event)) {
				EventHandler(dpy, window2, &the_event);
			}

			if (i == 0)
				timer++;

			if ((timer % t_pause) == 0) {

				MoveTrain(i);
				gettimeofday(&t);
				if (last_time != t.tv_sec)
				{
					DecrementTime();
					last_time = t.tv_sec;
				}
			}
		}
	}
}

EventHandler(display, window, pevent)
Display	*display;
Window	window;
XEvent	*pevent;
{
	XButtonEvent *button_event;
	XEvent	the_event;
	static int	started = 0;
	int	i;

	if (pevent->type == ButtonPress) {
		seed = pevent->xbutton.time;
		button_event = (XButtonEvent *)pevent;
		SlideOver(button_event);
	}
	if (pevent->type == Expose) {
		if (!monochrome_mode) {
			for (i = 0; i < 4; i++)
				DrawStation(window1,i);
		}
		else {
			DrawStation(window1,passenger_dst);
		}
		UpdateScore(0);
	}
}

SlideOver(button_event)
XButtonEvent *button_event;
{
	int	i,j;
	int	ok_flag = False;
	XEvent	junk_event;

/*
** Eat multiple clicks
*/
	i = 0;
	while (XCheckWindowEvent(	dpy,
					button_event->window,
					ButtonPressMask,
					&junk_event) == True) {
		XFlush(dpy);
	}

	for (i = 0; i < NUM_TILES; i++) {
		if (window_list[i].wid == button_event->window)
			break;
	}

	if (i >= NUM_TILES) {
		return(0);
	}

	for (j = 0; j < NUM_TRAINS; j++) {
		if (i == train_pos[j])
			return(0);
	}

	for (j = 0; j < 4; j++) {
		if (GetTileInDirection(i,j) == hole_pos)
			ok_flag = True;
	}

	if (ok_flag != True) {
		return (0);
	}

	XMoveWindow(	dpy, button_event->window, 
			(XOFWIN(hole_pos) + XOFWIN(i)) >> 1, 
			(YOFWIN(hole_pos) + YOFWIN(i)) >> 1);

	window_list[hole_pos].wid = button_event->window;
	window_list[hole_pos].type = window_list[i].type;
	window_list[i].wid = 0;
	XMoveWindow(	dpy, button_event->window, 
			XOFWIN(hole_pos), YOFWIN(hole_pos));
	hole_pos = i;
}

/*
**  Rudimentary auto-play mode:  Move a random tile into the hole,
**  but not the tile that the train's about to move onto or the one
**  that it's currently on.  If the train's headed to the right station,
**  don't do anything.
*/

MoveSemiRandomTile(next_train_pos)
{

	static int	direction = 0;
	static int	last_moved_tile = 0;
	int	moved_tile = 0;
	int	j;


/*
**  Find a random tile bordering the hole.
*/


GETONE: if (GetRandom(3) == 0)  {
		direction = GetRandom(4);
	}
	else
		direction = (direction + 1) % 4;

	moved_tile = GetTileInDirection(hole_pos,direction);

/*
**  Ensure it's not on the grayed-out border 
*/

	if (moved_tile < GRID_SIZE)
		goto GETONE;

	if ((moved_tile % GRID_SIZE) == 0)
		goto GETONE;

	if (((moved_tile + 1) % GRID_SIZE) == 0)
		goto GETONE;

	if (moved_tile >= (NUM_TILES - GRID_SIZE))
		goto GETONE;

	if (moved_tile == next_train_pos)
		goto GETONE;

	for (j = 0; j < NUM_TRAINS; j++) {
		if (moved_tile == train_pos[j])
			goto GETONE;
	}

	if (window_list[moved_tile].wid == window_list[last_moved_tile].wid) {
/*
**  Let the train hit the hole sometimes.  Looks good.
*/
		return;
	}

	XMoveWindow(	dpy, window_list[moved_tile].wid,
			(XOFWIN(hole_pos) + XOFWIN(moved_tile)) >> 1, 
			(YOFWIN(hole_pos) + YOFWIN(moved_tile)) >> 1);

	XMoveWindow(	dpy, window_list[moved_tile].wid,
			XOFWIN(hole_pos), YOFWIN(hole_pos));

	window_list[hole_pos].wid = window_list[moved_tile].wid;
	window_list[hole_pos].type = window_list[moved_tile].type;
	window_list[moved_tile].wid = 0;

	last_moved_tile = hole_pos;
	hole_pos = moved_tile;
}

GoingRightWay(tile,exit_direction)
int	tile;
int	exit_direction;
{
	int	i, type;

	for (i = 0; i < 35; i++) {
		tile = GetTileInDirection(tile, exit_direction);

		if (tile == hole_pos) {
			return (False);
		}

		if (passenger_src == PASSENGER_NONE)
			continue;

		if (passenger_src == PASSENGER_RIDING &&
				tile == stn_list[passenger_dst].tile) {
			return (True);
		}
		if (passenger_src != PASSENGER_RIDING &&
				tile == stn_list[passenger_src].tile) {
			return (True);
		}

		type = window_list[tile].type;
		exit_direction = ((tile_defs[type].exits[exit_direction]) + 2) % 4;
	}
	return (False);
}

GetTileInDirection(tile,direction)
int tile;
int direction;
{
	int foo;
	switch (direction) {

	case 0:				/* Look up */
		foo = tile - GRID_SIZE;
		foo = (foo < 0) ? -1 : foo;
		break;

	case 1:				/* Look right */
		foo = tile + 1;
		foo = (foo % GRID_SIZE == 0) ? -1 : foo;
		break;

	case 2:				/* Look down */
		foo = tile + GRID_SIZE;
		foo = (foo > NUM_TILES) ? -1 : foo;
		break;

	case 3:				/* Look left */
		foo = tile - 1;
		foo = ((foo + 1) % GRID_SIZE == 0) ? -1 : foo;
		break;
	}

/*
** Special case for introducing extra trains
*/
	if (foo == -1) {
		foo = 8;
	}

	return (foo);
}

/*
**	Move train.
*/

MoveTrain(num)
int num;
{
	static int train_state[NUM_TRAINS];	/* which position in window */
	static int train_side[NUM_TRAINS];	/* which side it entered from */
	static int	train_cars[4][2];
	static int	initialized = 0;

	int foo, bar;
	int type;
	int exit_direction;
	int x,y;

	if (!initialized) {
		for (foo = 0; foo < NUM_TRAINS; foo++) {
			train_head[foo] = 0;
			train_side[foo] = 0;
			train_state[foo] = 0;
		}
		initialized = 1;
	}


/*
** is a passenger waiting?  If not, make one.
*/
	TryToCreateRandomPassenger();

	if (train_head[num] == 0)
		UpdatePassenger();

/*
** Is train about to leave a window?
*/
	type = window_list[train_pos[num]].type;


	if (train_state[num] >= NUM_POSES) {
		exit_direction = tile_defs[type].exits[train_side[num]];

		bar = GetTileInDirection(train_pos[num],exit_direction);

		if (	(demo_mode == True) &&
			(GoingRightWay(	train_pos[num],
					exit_direction) == False)) {
				MoveSemiRandomTile(bar);
				bar = GetTileInDirection(train_pos[num],exit_direction);
		}

		if (bar == hole_pos) {
			FlashHole();
			RemoveCarFromTrain(num);
			train_state[num] = 0;
			train_side[num] = exit_direction;
			return;
		}
			
		train_pos[num] = bar;
		train_state[num] = 0;
		train_side[num] = (exit_direction + 2) % 4;
		type = window_list[train_pos[num]].type;
	}

	x = tile_defs[type].position_list[train_side[num]][train_state[num]][0];
	y = tile_defs[type].position_list[train_side[num]][train_state[num]][1];

	x += XOFWIN(train_pos[num]);
	y += YOFWIN(train_pos[num]);

	XMoveWindow(dpy, train_windows[num][train_head[num]], x-5, y-5);

/*
** have we picked up or dropped off a passenger?
*/
	TryToPickUpPassenger(num);

	TryToDropOffPassenger(num);

	train_head[num] = (++train_head[num]) % train_len[num];

	train_state[num]++;
}

FlashHole()
{
	int i;

	for (i = 0; i < 10; i++) {
		if (!monochrome_mode)
			XSetWindowBackground( dpy, window2, my_red.pixel);
		else
			XSetWindowBackground( dpy, window2, WhitePixel(dpy,the_screen));
		XClearWindow( dpy, window2);
	
		XSetWindowBackground( dpy, window2, my_black.pixel);
		XClearWindow( dpy, window2);
	}

}



Pixmap
MakePixmap(root, bits, width, height, foreground, background)
Window root;
unsigned int bits;
unsigned int width;
unsigned int height;
unsigned long foreground;
unsigned long background;
{
	Pixmap		bitmap;
	Pixmap		pixmap;
	GC		DrawGC;
	XGCValues	gcv;

	gcv.foreground = foreground;
	gcv.background = background;
	gcv.graphics_exposures = False;
	gcv.function = GXcopy;
	gcv.subwindow_mode = IncludeInferiors;
	DrawGC = XCreateGC(dpy, root, GCForeground | GCBackground | GCFunction |
				GCGraphicsExposures | GCSubwindowMode, &gcv);


#ifdef __STDC__
	bitmap = XCreateBitmapFromData(dpy, root, (const char *) bits, width, height);
#else

	bitmap = XCreateBitmapFromData(dpy, root, (char *) bits, width, height);
#endif
	pixmap = XCreatePixmap(dpy, root, (unsigned int) width, (unsigned int) height,
						DefaultDepth(dpy, the_screen));
	XCopyPlane(dpy, bitmap, pixmap, DrawGC, 0, 0, width, height, 0, 0, 1);

	XFreePixmap(dpy, bitmap);
	XFreeGC (dpy, DrawGC);

	return pixmap;
}

/*
** Return a random number between 0 and limit-1
*/

GetRandom(limit)
int limit;
{
	int i;
	seed =  ((seed * 1297) >> 5);
	i = seed % limit;
	return(i);
}



Spin(x)
int x;
{
	int j;
	for (j = 0; j < x * t_pause; j++)
			;
}

MakeSubWindows(loc_window, track_pixel, bkgnd_pixel, border_pixel)
Window	loc_window;
unsigned long track_pixel, bkgnd_pixel, border_pixel;
{
	Pixmap	pixmapa, pixmapb, pixmapc, pixmapleft, pixmapright;
	int	i,j;
	int	counter;
	int	normal_flag;
	unsigned long	valuemask;
	XSetWindowAttributes	attributes;

	counter = level;

	valuemask = CWBackPixmap;

	pixmapa = MakePixmap (
			root_window,
			traxa_bits, traxa_width, traxa_height,
			track_pixel,
			bkgnd_pixel);

	pixmapb = MakePixmap (
			root_window,
			traxb_bits, traxb_width, traxb_height,
			track_pixel,
			bkgnd_pixel);

	pixmapc = MakePixmap (
			root_window,
			traxc_bits, traxc_width, traxc_height,
			track_pixel,
			bkgnd_pixel);

	pixmapleft = MakePixmap (
			root_window,
			traxb_bits, traxb_width, traxb_height,
			track_pixel,
			border_pixel);

	pixmapright = MakePixmap (
			root_window,
			traxc_bits, traxc_width, traxc_height,
			track_pixel,
			border_pixel);

	hole_pos = NUM_TILES - (GRID_SIZE + 2);


	for (i = 0; i < NUM_TILES; i++) {

		if (i == hole_pos)
			continue;

		normal_flag = False;


/*
** Special cases for border pieces
*/

/*
** first row
*/
		if (i < GRID_SIZE) {
			switch (i % 2) {
			case 0:
				attributes.background_pixmap = pixmapleft;
				window_list[i].type = 1;
				break;
			case 1:
				attributes.background_pixmap = pixmapright;
				window_list[i].type = 2;
				break;
			}
		}

/*
** first column
*/

		else if ((i % GRID_SIZE) == 0) {
			switch (i % (GRID_SIZE * 2)) {
			case 0:
				attributes.background_pixmap = pixmapleft;
				window_list[i].type = 1;
				break;
			default:
				attributes.background_pixmap = pixmapright;
				window_list[i].type = 2;
				break;
			}
		}

/*
** last column
*/
		else if (((i + 1) % GRID_SIZE) == 0) {
			switch ((i + 1) % (GRID_SIZE * 2)) {
			case 0:
				attributes.background_pixmap = pixmapleft;
				window_list[i].type = 1;
				break;
			default:
				attributes.background_pixmap = pixmapright;
				window_list[i].type = 2;
				break;
			}
		}

/*
** last row
*/
		else if (i >= (NUM_TILES - GRID_SIZE)) {
			switch (i % 2) {
			case 1:
				attributes.background_pixmap = pixmapleft;
				window_list[i].type = 1;
				break;
			case 0:
				attributes.background_pixmap = pixmapright;
				window_list[i].type = 2;
				break;
			}
		}

/*
** standard case
*/

		else {
			normal_flag = True;

			switch (level) {
			case 1:
			case 2:
			case 3:
				counter++;
				break;
			case 4:
				counter = (counter == 0 ? 1 : 0);
				break;
			case 5:
				counter = (counter == 1 ? 2 : 1);
				break;
			case 6:
				counter = (counter == 0 ? 2 : 0);
				break;
			case 8:
			case 12:
				counter = GetRandom(2) + 1;
				break;
			case 9:
				counter = (i == (hole_pos-1)) ? 2 : 0;
				break;
			default:
				counter = GetRandom(3);
			}

			switch (counter  % 3) {
			case 0:
				attributes.background_pixmap = pixmapa;
				window_list[i].type = 0;
				break;
			case 1:
				attributes.background_pixmap = pixmapb;
				window_list[i].type = 1;
				break;
			case 2:
				attributes.background_pixmap = pixmapc;
				window_list[i].type = 2;
				break;
			}
		}

		window_list[i].wid = XCreateWindow(
			dpy, loc_window,
			XOFWIN(i), YOFWIN(i),
			TILE_SIZE, TILE_SIZE,
			0,
			CopyFromParent,
			CopyFromParent,
			CopyFromParent,
			valuemask,
			&attributes);

		if (normal_flag == True) {
				XSelectInput(
					dpy, window_list[i].wid,
				 	ButtonPressMask);
		}
	}
/*
**	Make train
*/
	valuemask = CWBackPixel;
	attributes.background_pixel = my_black.pixel;

	for (j = 0; j < NUM_TRAINS; j++) {

		for (i = 0; i < train_len[j]; i++) {
			train_windows[j][i] = XCreateWindow(
				dpy, loc_window,
				-1000,-1000,
				TRAIN_WIDTH, TRAIN_HEIGHT,
				0,
				CopyFromParent,
				CopyFromParent,
				CopyFromParent,
				valuemask,
				&attributes);
		}
	}

}

FreeSubWindows()
{
	int i,j;

	for (j = 0; j < NUM_TRAINS; j++) {
		for (i = 0; i < train_len[j]; i++) {
			XDestroyWindow(dpy,train_windows[j][i]);
		}
	}

	for (i = 0; i < NUM_TILES; i++) {
		if (i != hole_pos) {
			XDestroyWindow(dpy,window_list[i].wid);
		}
	}

}

RemoveCarFromTrain(num)
int num;
{
	train_len[num]--;
	XDestroyWindow(dpy,train_windows[num][train_len[num]]);
	train_head[num] = 0;
	if (train_len[num] <= 0) {
		ExitFunc();
	}
}
