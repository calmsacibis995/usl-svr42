/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:gfx.c	1.1"
/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)gfx.c	2.1	90/04/25
 *
 */

/*
 * Spider card drawing routines
 */
#include	"defs.h"
#include	"globals.h"

#ifdef ROUND_CARDS
#include	<X11/Xmu/Drawing.h>

#define	ROUND_W	7
#define	ROUND_H	7

Bool	round_cards = True;
#endif

static GC	cardgc;		/* gc in use when drawing cards */

#ifdef	KITLESS
static int	message_y;
static GC	textgc;
#endif	/* KITLESS */


/* substitue gray1 for Green on mono */
#define gray1_width 16
#define gray1_height 16
static unsigned char gray1_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa};

/* logo for card backs */
/* any size logo can be handled.  */

#define logo_width 64
#define logo_height 64
static unsigned char logo_bits[] = {
 0x77,0xd5,0xd7,0xdd,0x77,0xd5,0xd7,0xdd,0xbb,0xea,0xae,0xbb,0xbb,0xea,0xae,
 0xbb,0x5d,0x75,0x5d,0x77,0x5d,0x75,0x5d,0x77,0xee,0xba,0xba,0xee,0xae,0xba,
 0xba,0xee,0x77,0x5d,0x75,0xdd,0x77,0x5d,0x75,0xdd,0xbb,0xae,0xea,0xba,0xbb,
 0xae,0xea,0xba,0x5d,0x57,0xd5,0x75,0x5d,0x57,0xd5,0x75,0xae,0xab,0xaa,0xeb,
 0xae,0xab,0xaa,0xeb,0xd7,0x55,0x55,0xd7,0xd7,0x51,0x55,0xd7,0xeb,0xae,0xaa,
 0xae,0xeb,0xa2,0xaa,0xae,0x75,0x5d,0x55,0x5d,0x75,0x44,0x15,0x5d,0xba,0xbb,
 0xea,0xba,0xba,0x88,0x8a,0xba,0x5d,0x77,0x75,0x75,0x1d,0x11,0x45,0x75,0xee,
 0xee,0xba,0xea,0x2e,0x22,0xa2,0xea,0xd7,0xdd,0x5d,0xd5,0x57,0x44,0x10,0xd5,
 0xab,0xbb,0xef,0xaa,0xab,0x88,0x88,0xaa,0x57,0x77,0x77,0xd5,0x57,0x11,0x45,
 0xd5,0xae,0xee,0xba,0xea,0xae,0x22,0xa2,0xea,0x5d,0xdd,0x5d,0x75,0x5d,0x45,
 0x10,0x75,0xba,0xba,0xef,0xba,0xba,0x8a,0x88,0xba,0x75,0x75,0x77,0x5d,0x75,
 0x15,0x45,0x5d,0xea,0xea,0xba,0xae,0xea,0x2a,0xa2,0xae,0xdd,0xd5,0x5d,0x77,
 0xdd,0x55,0x50,0x77,0xae,0xab,0xaf,0xeb,0xae,0xab,0xa8,0xeb,0xd7,0x55,0x57,
 0xd7,0xd7,0x55,0x55,0xd7,0xeb,0xaa,0xaa,0xae,0xeb,0xaa,0xaa,0xae,0x75,0x57,
 0x55,0x5d,0x75,0x57,0x55,0x5d,0xba,0xae,0xea,0xba,0xba,0xae,0xaa,0xba,0xdd,
 0x5d,0x75,0x75,0xdd,0x5d,0x75,0x75,0xae,0xbb,0xba,0xea,0xae,0xbb,0xba,0xea,
 0x77,0x77,0x5d,0xd5,0x77,0x77,0x5d,0xd5,0xeb,0xee,0xee,0xaa,0xeb,0xee,0xae,
 0xaa,0xd7,0xdd,0x77,0xd5,0xd7,0xdd,0x77,0xd5,0xae,0xbb,0xbb,0xea,0xae,0xbb,
 0xbb,0xea,0x5d,0x77,0x5d,0x75,0x5d,0x77,0x5d,0x75,0xba,0xee,0xee,0xba,0xba,
 0xee,0xae,0xba,0x75,0xdd,0x77,0x5d,0x75,0xdd,0x77,0x5d,0xea,0xba,0xbb,0xae,
 0xea,0xba,0xbb,0xae,0xd5,0x75,0x5d,0x57,0xd5,0x75,0x5d,0x57,0xaa,0xeb,0xae,
 0xab,0xaa,0xeb,0xae,0xab,0x55,0xd7,0xd7,0x55,0x55,0xd7,0xd7,0x55,0xaa,0xae,
 0xeb,0xae,0xaa,0xae,0xeb,0xae,0x55,0x5d,0x75,0x5d,0x55,0x5d,0x75,0x5d,0xea,
 0xba,0xba,0xbb,0xea,0xba,0xba,0xbb,0x75,0x75,0x5d,0x77,0x75,0x75,0x5d,0x77,
 0xba,0xea,0xee,0xee,0xba,0xea,0xee,0xee,0x5d,0xd5,0xd7,0xdd,0x5d,0xd5,0xd7,
 0xdd,0xef,0xaa,0xab,0xbb,0xef,0xaa,0xab,0xbb,0x77,0xd5,0x57,0x77,0x77,0xd5,
 0x57,0x77,0xba,0xea,0xae,0xee,0xba,0xea,0xae,0xee,0x5d,0x75,0x5d,0xdd,0x5d,
 0x75,0x5d,0xdd,0xef,0xba,0xba,0xba,0xef,0xba,0xba,0xba,0x77,0x5d,0x75,0x75,
 0x77,0x5d,0x75,0x75,0xba,0xae,0xea,0xea,0xba,0xae,0xea,0xea,0x5d,0x77,0xdd,
 0xd5,0x5d,0x77,0xdd,0xd5,0xaf,0xeb,0xae,0xab,0xaf,0xeb,0xae,0xab,0x57,0xd7,
 0xd7,0x55,0x57,0xd7,0xd7,0x55,0xaa,0xae,0xeb,0xaa,0xaa,0xae,0xeb,0xaa,0x55,
 0x5d,0x75,0x57,0x55,0x5d,0x75,0x57,0xaa,0xba,0xba,0xae,0xaa,0xba,0xba,0xae,
 0x75,0x75,0xdd,0x5d,0x55,0x75,0xdd,0x5d,0xba,0xea,0xae,0xbb,0xba,0xea,0xae,
 0xbb,0x5d,0xd5,0x77,0x77,0x5d,0xd5,0x77,0x77,0xae,0xaa,0xeb,0xee,0xae,0xaa,
 0xeb,0xee};

#ifndef SMALL_CARDS
#include	"rank.bm"
#include	"face.bm"
#include	"suit.bm"

static Pixmap	rank_map[NUM_RANKS],	rank_r_map[NUM_RANKS];
static Pixmap	rank_map_red[NUM_RANKS],	rank_r_map_red[NUM_RANKS];
static Pixmap	suit_map[NUM_SUITS],	suit_r_map[NUM_SUITS];
static Pixmap	suit_sm_map[NUM_SUITS],	suit_sm_r_map[NUM_SUITS];
static Pixmap	suit_lg_map[NUM_SUITS];
static Pixmap	jack_map[NUM_SUITS], queen_map[NUM_SUITS], king_map[NUM_SUITS];

#else	/* SMALL_CARDS */
#include	"cards.bm"

static Pixmap	card_bitmaps[CARDS_PER_DECK];
#endif /* !SMALL_CARDS */

/* clipping rectangles */
static XRectangle	cliprects[1] = { 0, 0, CARD_WIDTH + 1, 0};

static GC	redgc;
static GC	blackgc;
static GC	whitegc;
static GC	backgc;

static int	back_delta_x, back_delta_y; /* how much to modify the TS origin by */

#ifndef SMALL_CARDS
static Bool	card_is_clipped;	/* optimizer for card drawing */
#endif

gfx_init(d, scr)
Display	*d;
int	scr;
{
XGCValues	gcv;
long		gcflags;
XColor		color;
Colormap	cmap;
Pixmap		tmpmap;
GC		logogc;
unsigned long	redpixel;

	/* save these off */
	dpy = d;
	screen = scr;

	if (DisplayCells(dpy, screen) > 2)	{
		is_color = True;
	}	else	{
		is_color = False;
	}

	blackpixel = BlackPixel(dpy, screen);
	whitepixel = WhitePixel(dpy, screen);

	/* make gc for white */
	gcv.foreground = WhitePixel(dpy, screen);
	gcv.background = BlackPixel(dpy, screen);
	gcv.graphics_exposures = False;
	gcflags = GCForeground | GCBackground | GCGraphicsExposures;

	whitegc = XCreateGC(dpy, RootWindow(dpy, screen), gcflags, &gcv);

	/* make gc for black */
	gcv.foreground = BlackPixel(dpy, screen);
	gcv.background = WhitePixel(dpy, screen);
	gcflags = GCForeground | GCBackground | GCGraphicsExposures;

	blackgc = XCreateGC(dpy, RootWindow(dpy, screen), gcflags, &gcv);

#ifdef	KITLESS
	/* add on to blackgc */
	if ((message_font = XLoadQueryFont(dpy, MESSAGE_FONT)) == NULL)	{
		(void) fprintf(stderr,"can't get font %s\n", MESSAGE_FONT);
		exit(0);
	}
	message_y = message_font->ascent;
	gcv.font = message_font->fid;
	gcflags |= GCFont;

	textgc = XCreateGC(dpy, RootWindow(dpy, screen), gcflags, &gcv);
#endif /*KITLESS*/

	tmpmap = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		(char *)logo_bits, logo_width, logo_height);

	logomap = XCreatePixmap(dpy, RootWindow(dpy, screen), logo_width,
		logo_height, DefaultDepth(dpy, screen));

	back_delta_x = (CARD_WIDTH - logo_width)/2;
	back_delta_y = (CARD_HEIGHT - logo_height)/2;

	if (is_color)	{
		cmap = DefaultColormap(dpy, screen);
		XAllocNamedColor(dpy, cmap, "Sea Green", &color, &color);
		gcv.foreground = color.pixel;
		gcv.background = WhitePixel(dpy, screen);
		gcflags = GCForeground | GCBackground;
		logogc = XCreateGC(dpy, RootWindow(dpy, screen), gcflags, &gcv);
		XCopyPlane(dpy, tmpmap, logomap, logogc, 0, 0, 
			logo_width, logo_height, 0, 0, 1);
		XFreeGC(dpy, logogc);
	} else	{
		XCopyPlane(dpy, tmpmap, logomap, whitegc, 0, 0, 
			logo_width, logo_height, 0, 0, 1);
	}
	XFreePixmap(dpy, tmpmap);

	gcv.tile = logomap;
	gcv.fill_style = FillTiled;
	gcflags |= GCTile | GCFillStyle | GCGraphicsExposures;

	backgc = XCreateGC(dpy, RootWindow(dpy, screen), gcflags, &gcv);

	borderpixel = blackpixel;

	if (is_color)	{
		cmap = DefaultColormap(dpy, screen);

		color.flags = DoRed | DoGreen | DoBlue;

		/*
		 * color levels are the NeWS RGB values
		 */
		color.red = 13107;	/* 0.2 */
		color.green = 52428;	/* 0.8 */
		color.blue = 39321;	/* 0.6 */
		XAllocColor(dpy, cmap, &color);
		greenpixel = color.pixel;

		color.red = 52428;	/* 0.8 */
		color.green = color.blue = 0;
		XAllocColor(dpy, cmap, &color);
		redpixel = color.pixel;

		gcv.foreground = redpixel;
		gcv.background = WhitePixel(dpy, screen);
		gcflags = GCForeground | GCBackground | GCGraphicsExposures;

		redgc = XCreateGC(dpy, RootWindow(dpy, screen), gcflags, &gcv);

	} else	{
		greenmap = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
			(char *) gray1_bits, gray1_width, gray1_height);

		redmap = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
			(char *) gray1_bits, gray1_width, gray1_height);

		gcv.tile = redmap;
		gcv.fill_style = FillTiled;

		gcv.foreground = BlackPixel(dpy, screen);
		gcv.background = WhitePixel(dpy, screen);

		gcflags = GCTile | GCForeground | GCBackground |
			GCFillStyle | GCGraphicsExposures;

		redgc = XCreateGC(dpy, RootWindow(dpy, screen), gcflags, &gcv);
	}
	make_card_maps();
}

#ifndef SMALL_CARDS
/*
 * make a 'red' pixmap by setting the clipmask to the desired shape and 
 * pushing 'red' through it
 */

static Pixmap
make_red_map(bits, width, height)
char	*bits;
int	width, height;
{
Pixmap	tmpmap, newmap;
static GC	cleargc = (GC) 0;
XGCValues	xgcv;


	if (cleargc == (GC) 0)	{
		xgcv.function = GXclear;
		cleargc = XCreateGC(dpy, RootWindow(dpy, screen), GCFunction, 
								&xgcv);
	}

	tmpmap = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		bits, width, height);

	newmap = XCreatePixmap(dpy, RootWindow(dpy, screen), width, height, 1);

	/* clear pixmap */
	XFillRectangle(dpy, newmap, cleargc, 0, 0, width, height);

	XSetClipMask(dpy, redgc, tmpmap);
	XFillRectangle(dpy, newmap, redgc, 0, 0, width, height);
	XSetClipMask(dpy, redgc, None);
	XFreePixmap(dpy, tmpmap);

	return (newmap);
}

make_card_maps()
{
unsigned char	*new_bits;
Rank	r;
int	i;

	for (r = Ace; r <= King; r++)	{
		rank_map[(int)r] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			rank_bits[(int)r], rank_width, rank_height);

		new_bits = (unsigned char *) calloc(sizeof(rank_bits[(int)r]),
							1);
		rot_180((unsigned char *)rank_bits[(int)r], new_bits, 
			rank_width, rank_height);
		rank_r_map[(int)r] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			new_bits, rank_width, rank_height);
		free((char *)new_bits);
	}

	for (r = Ace; r <= King; r++)	{
		new_bits = (unsigned char *) calloc(sizeof(rank_bits[(int)r]),
							1);
		rot_180((unsigned char *)rank_bits[(int)r], new_bits, 
				rank_width, rank_height);
		if (is_color)	{
			rank_map_red[(int)r] = XCreateBitmapFromData(dpy, 
				RootWindow(dpy, screen),
				rank_bits[(int)r], rank_width, rank_height);

			rank_r_map_red[(int)r] = XCreateBitmapFromData(dpy, 
				RootWindow(dpy, screen),
				new_bits, rank_width, rank_height);
		} else	{
			rank_map_red[(int)r] = make_red_map(rank_bits[(int)r],
						rank_width, rank_height);

			rank_r_map_red[(int)r] = make_red_map((char *)new_bits, 
						rank_width, rank_height);
		}
		free((char *)new_bits);
	}

	i = (int)Spade;
	/* make all the card bitmaps */
	suit_map[i] = XCreateBitmapFromData(dpy, 
		RootWindow(dpy, screen),
		spade_bits, spade_width, spade_height);

	new_bits = (unsigned char *) calloc(sizeof(spade_bits), 1);
	flip_bits((unsigned char *)spade_bits, new_bits, spade_width, 
				spade_height);
	suit_r_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		new_bits, spade_width, spade_height);
	free((char *)new_bits);

	suit_sm_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		spade_sm_bits, spade_sm_width, spade_sm_height);

	new_bits = (unsigned char *) calloc(sizeof(spade_sm_bits), 1);
	flip_bits((unsigned char *)spade_sm_bits, new_bits, spade_sm_width,
			spade_sm_height);
	suit_sm_r_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		new_bits, spade_sm_width, spade_sm_height);
	free((char *)new_bits);

	suit_lg_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		spade_lg_bits, spade_lg_width, spade_lg_height);

	jack_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		jack_s_bits, jack_s_width, jack_s_height);

	queen_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		queen_s_bits, queen_s_width, queen_s_height);

	king_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		king_s_bits, king_s_width, king_s_height);

	i = (int)Heart;
	/* make all the card bitmaps */
	new_bits = (unsigned char *) calloc(sizeof(heart_bits), 1);
	flip_bits((unsigned char *)heart_bits, new_bits, heart_width, 
					heart_height);

	if (is_color)	{
		suit_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			heart_bits, heart_width, heart_height);
		suit_r_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			new_bits, heart_width, heart_height);
	} else	{
		suit_map[i] = make_red_map(heart_bits, heart_width, 
						heart_height);
		suit_r_map[i] = make_red_map((char *)new_bits, heart_width, 
						heart_height);
	}

	free((char *)new_bits);

	new_bits = (unsigned char *) calloc(sizeof(heart_sm_bits), 1);
	flip_bits((unsigned char *)heart_sm_bits, new_bits, heart_sm_width, 
		heart_sm_height);
	suit_sm_r_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		new_bits, heart_sm_width, heart_sm_height);

	if (is_color)	{
		suit_sm_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			heart_sm_bits, heart_sm_width, heart_sm_height);
		suit_sm_r_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			new_bits, heart_sm_width, heart_sm_height);
	} else	{
		suit_sm_map[i] = make_red_map(heart_sm_bits, heart_sm_width, 
						heart_height);
		suit_sm_r_map[i] = make_red_map((char *)new_bits, 
			heart_sm_width, heart_sm_height);
	}
	free((char *)new_bits);

	suit_lg_map[i] = suit_map[i];

	if (is_color)	{
		jack_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			jack_h_bits, jack_h_width, jack_h_height);

		queen_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			queen_h_bits, queen_h_width, queen_h_height);

		king_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			king_h_bits, king_h_width, king_h_height);
	} else	{
		jack_map[i] = make_red_map(jack_h_bits, jack_h_width, 
							jack_h_height);

		queen_map[i] = make_red_map(queen_h_bits, queen_h_width, 
							queen_h_height);

		king_map[i] = make_red_map(king_h_bits, king_h_width, 
							king_h_height);
	}


	i = (int)Diamond;
	/* make all the card bitmaps */
	new_bits = (unsigned char *) calloc(sizeof(diamond_bits), 1);
	flip_bits((unsigned char *)diamond_bits, new_bits, diamond_width, 
		diamond_height);

	if (is_color)	{
		suit_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			diamond_bits, diamond_width, diamond_height);
		suit_r_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			new_bits, diamond_width, diamond_height);
	} else	{
		suit_map[i] = make_red_map(diamond_bits, diamond_width, 
						diamond_height);
		suit_r_map[i] = make_red_map((char *)new_bits, diamond_width, 
						diamond_height);
	}

	free((char *)new_bits);

	new_bits = (unsigned char *) calloc(sizeof(diamond_sm_bits), 1);
	flip_bits((unsigned char *)diamond_sm_bits, new_bits, 
				diamond_sm_width, diamond_sm_height);
	suit_sm_r_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		new_bits, diamond_sm_width, diamond_sm_height);

	if (is_color)	{
		suit_sm_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			diamond_sm_bits, diamond_sm_width, diamond_sm_height);
		suit_sm_r_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			new_bits, diamond_sm_width, diamond_sm_height);
	} else	{
		suit_sm_map[i] = make_red_map(diamond_sm_bits, diamond_sm_width, 
						diamond_height);
		suit_sm_r_map[i] = make_red_map((char *)new_bits, 
				diamond_sm_width, diamond_sm_height);
	}
	free((char *)new_bits);

	suit_lg_map[i] = suit_map[i];

	if (is_color)	{
		jack_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			jack_d_bits, jack_d_width, jack_d_height);

		queen_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			queen_d_bits, queen_d_width, queen_d_height);

		king_map[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen),
			king_d_bits, king_d_width, king_d_height);
	} else	{
		jack_map[i] = make_red_map(jack_d_bits, jack_d_width, 
							jack_d_height);

		queen_map[i] = make_red_map(queen_d_bits, queen_d_width, 
							queen_d_height);

		king_map[i] = make_red_map(king_d_bits, king_d_width, 
							king_d_height);
	}

	i = (int)Club;
	/* make all the card bitmaps */
	suit_map[i] = XCreateBitmapFromData(dpy, 
		RootWindow(dpy, screen),
		club_bits, club_width, club_height);

	new_bits = (unsigned char *) calloc(sizeof(club_bits), 1);
	flip_bits((unsigned char *)club_bits, new_bits, club_width, 
		club_height);
	suit_r_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		new_bits, club_width, club_height);
	free((char *)new_bits);

	suit_sm_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		club_sm_bits, club_sm_width, club_sm_height);

	new_bits = (unsigned char *) calloc(sizeof(club_sm_bits), 1);
	flip_bits((unsigned char *)club_sm_bits, new_bits, club_sm_width, 
		club_sm_height);
	suit_sm_r_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		new_bits, club_sm_width, club_sm_height);
	free((char *)new_bits);

	suit_lg_map[i] = suit_map[i];


	jack_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		jack_c_bits, jack_c_width, jack_c_height);

	queen_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		queen_c_bits, queen_c_width, queen_c_height);

	king_map[i] = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		king_c_bits, king_c_width, king_c_height);
}

#else

make_card_maps()
{
int	i;

	for (i = 0; i < CARDS_PER_DECK; i++)	{
		card_bitmaps[i] = XCreateBitmapFromData(dpy, 
			RootWindow(dpy, screen), (char *)card_bits[i], 
			card_width, card_height);
	}
}
#endif /* !SMALL_CARDS */

void
force_redraw()
{
	XClearArea(dpy, table, 0, 0, 0, 0, True);
}

/*
 * only bother to show one card, since they're all stacked up
 *
 * REMIND -- spread the deck a bit to show that there's more
 */
redraw_deck(x, y, w, h)
int	x, y, w, h;
{
CardPtr	tmp;

	/* trivial reject */
	if ((y > (DECK_Y + CARD_HEIGHT)) || ((y + h) < (DECK_Y)) ||
	    (x > (DECK_X + CARD_WIDTH))  || ((x + w) < DECK_X))
		return;

	if (deck->cards == CARDNULL)	{
		XClearArea(dpy, table, deck->x, deck->y, CARD_WIDTH,
			CARD_HEIGHT, False);
#ifdef ROUND_CARDS
		if (round_cards)	{
			XmuDrawRoundedRectangle(dpy, table, blackgc, 
				deck->x, deck->y, CARD_WIDTH, CARD_HEIGHT,
				ROUND_W, ROUND_H);
		} else
#endif
		XDrawRectangle(dpy, table, blackgc, deck->x, deck->y,
			CARD_WIDTH, CARD_HEIGHT);
	} else	{
		/* only show topmost card */
		tmp = last_card(deck);
		if (tmp->draw_count != draw_count)	{
			show_card(tmp);
			tmp->draw_count = draw_count;
		}
	}
}


redraw_card_piles(x, y, w, h)
int	x, y, w, h;
{
int	i;
CardPtr	tmp;

	/* trivial reject */
	if ((y > (PILE_LOC_Y + CARD_HEIGHT)) || (y + h) < (PILE_LOC_Y))
		return;

	/* figure out which piles to show */
	for (i = 0; i < NUM_PILES; i++)	{
		if ((x <= (CARD_WIDTH + PILE_LOC_X(piles[i]->place))) && 
		    ((x + w) >= PILE_LOC_X(piles[i]->place)))	{
			if (tmp = piles[i]->cards)	{
				/* display the last card */
				while (tmp->next)	{
					tmp = tmp->next;
				}
				if (tmp->draw_count != draw_count)	{
					show_card(tmp);
					tmp->draw_count = draw_count;
				}
			} else	{
#ifdef ROUND_CARDS
				if (round_cards)
				    XmuDrawRoundedRectangle(dpy, table, blackgc,
					piles[i]->x, piles[i]->y,
					CARD_WIDTH, CARD_HEIGHT,
					ROUND_W, ROUND_H);
				else
#endif
				    XDrawRectangle(dpy, table, blackgc, 
					piles[i]->x, piles[i]->y,
					CARD_WIDTH, CARD_HEIGHT);
			}
			
		}
	}
}


redraw_card_stacks(x, y, w, h)
int	x, y, w, h;
{
int	i;
CardPtr	tmp;

	/* trivial reject */
	if ((y + h) < (STACK_LOC_Y))
		return;

	/* figure out which stacks to show */
	for (i = 0; i < NUM_STACKS; i++)	{
		if ((x <= (CARD_WIDTH + STACK_LOC_X(stack[i]->place))) && 
		    ((x + w) >= STACK_LOC_X(stack[i]->place))) {
			if (tmp = stack[i]->cards)	{
				while (tmp)	{
					int	d;

					if (tmp->next)
						d = tmp->list->card_delta;
					else
						d = CARD_HEIGHT;
					if ((y <= (tmp->y + d)) && 
					    ((y + h) >= tmp->y)) {
#ifdef ROUND_CARDS
/* for round cards, we just paint the whole stack if any card is damaged.
 * this avoids getting into trouble when unsorted exposure lists come back
 * and we screw up overlaps
 */
						if (round_cards)	{
						    tmp = stack[i]->cards;
						    /* skip if this stack's been
						     * painted already */
						    if (tmp->draw_count == draw_count)	{
							tmp = CARDNULL; /* so we pop out */
							continue;
							}
                                                    while (tmp)     {
							show_card(tmp);
							tmp->draw_count = draw_count;
                                                        tmp = tmp->next;
                                                    }
						    continue;
						} else
#endif /* ROUND_CARDS */
						{
						    if (tmp->draw_count != draw_count)	{
							show_card(tmp);
							tmp->draw_count = draw_count;
						    }
						}
					}
					tmp = tmp->next;
				}
			} else	{
#ifdef ROUND_CARDS
				if (round_cards)
				    XmuDrawRoundedRectangle(dpy, table, blackgc,
					stack[i]->x, stack[i]->y,
					CARD_WIDTH, CARD_HEIGHT,
					ROUND_W, ROUND_H);
				else
#endif
				    XDrawRectangle(dpy, table, blackgc, 
					stack[i]->x, stack[i]->y,
					CARD_WIDTH, CARD_HEIGHT);
			}
		}
	}
}

/*
 * clears out and repaints list from card down
 */
show_list(list, card)
CardList	list;
CardPtr		card;
{
CardPtr	tmp;
int	h;
	
	if (list->place >= STACK_1)	{
		h = table_height - ((card) ? card->y : STACK_LOC_Y);
	} else	{
		h = CARD_HEIGHT;
	}

#ifdef ROUND_CARDS
	/* don't want to blast the card above us that we overwrite */
	if (round_cards)
	    XClearArea(dpy, table, list->x, 
		((card) ? card->y + ROUND_H : list->y), 
		CARD_WIDTH + 1, 
		h - (IS_PILE(list) ? 0 : ROUND_H),
		False);
	else
#endif
	    XClearArea(dpy, table, list->x, ((card) ? card->y : list->y), 
		CARD_WIDTH + 1, h, False);

	tmp = card;
	if (tmp == CARDNULL)	{
#ifdef ROUND_CARDS
		if (round_cards)
		    XmuDrawRoundedRectangle(dpy, table, blackgc, 
			list->x, list->y,
			CARD_WIDTH, CARD_HEIGHT, ROUND_W, ROUND_H);
		else
#endif
		    XDrawRectangle(dpy, table, blackgc, list->x, list->y,
			CARD_WIDTH, CARD_HEIGHT);
	}
	while (tmp)	{
		show_card(tmp);
		tmp = tmp->next;
	}

}

/*
 * paints individual card
 */
show_card(card)
CardPtr	card;
{
int	delta	= 0;

	if (((card->next == CARDNULL) || ((card->list->card_delta != 0))) && 
	    (card->type == Faceup))	{
		if (card->next)
			delta = card->list->card_delta;
		paint_card(card->x, card->y, card->rank, card->suit, delta);
	}
	if (card->type == Facedown)	{
		/* paint all of back if end of stack */
		if (card->next == CARDNULL) 	{
			paint_cardback(card->x, card->y, CARD_HEIGHT);
		/* paint delta bits of it otherwise */
		} else if (card->list->card_delta)	{
			paint_cardback(card->x, card->y, 
				card->list->card_delta);
		}
	}
}

paint_cardback(x, y, delta)
int	x, y, delta;
{
#define	INSET	2
	/* change the origin so cards will have the same back anywhere
	 * on the table
	 */
	/*
	 * there should be a tile centered in the card, with the
	 * surrounding tiles being partial
	 */
#ifdef ROUND_CARDS
	if (round_cards)
	    XmuFillRoundedRectangle(dpy, table, blackgc, x, y, CARD_WIDTH, 
		(delta == CARD_HEIGHT) ? delta : delta + ROUND_H * 2,
		ROUND_W, ROUND_H);
	else
#endif
	    XFillRectangle(dpy, table, blackgc, x, y, CARD_WIDTH, delta);
	XSetTSOrigin(dpy, backgc, x + back_delta_x, y + back_delta_y);

#ifdef ROUND_CARDS
	if (round_cards)
	    XmuFillRoundedRectangle(dpy, table, backgc, x + INSET, y + INSET, 
		CARD_WIDTH - 2*INSET,
		(delta == CARD_HEIGHT) ? 
			(CARD_HEIGHT - 2*INSET) : delta + ROUND_H * 2,
		ROUND_W, ROUND_H);
	else
#endif
	    XFillRectangle(dpy, table, backgc, x + INSET, y + INSET, 
		CARD_WIDTH - 2*INSET,
		(delta == CARD_HEIGHT) ? 
			(CARD_HEIGHT - 2*INSET) : delta);
}

#ifndef SMALL_CARDS
/*
 * delta is 0 if the card is fully showing
 */
paint_card(x, y, rank, suit, delta)
int	x,y;
Rank	rank;
Suit	suit;
int	delta;
{
	if (suit == Spade || suit == Club)	{
		cardgc = blackgc;
	} else	{
		cardgc = redgc;
	}

	if (delta)	{
#ifdef ROUND_CARDS
		if (round_cards)
			cliprects[0].height = delta + ROUND_H;
		else
#endif
			cliprects[0].height = delta;
		XSetClipRectangles(dpy, cardgc, x, y, cliprects, 1, Unsorted);
#ifdef ROUND_CARDS
		if (round_cards)	{
		    XSetClipRectangles(dpy, whitegc, x, y, cliprects, 1, 
						Unsorted);
		    if (cardgc != blackgc)
			XSetClipRectangles(dpy, blackgc, x, y, cliprects, 1,
						Unsorted);

		    /* add in ROUND_H to height so only the top is rounded */
		    /* fill the background */
		    XmuFillRoundedRectangle(dpy, table, whitegc, x, y, 
			CARD_WIDTH, delta + ROUND_H, ROUND_W, ROUND_H);
		    /* draw border on card */
		    XmuDrawRoundedRectangle(dpy, table, blackgc, x, y, 
			CARD_WIDTH, delta + ROUND_H, ROUND_W, ROUND_H);
		} else	
#endif
		{
		    /* fill the background */
		    XFillRectangle(dpy, table, whitegc, x, y, 
					CARD_WIDTH, delta);
		    /* draw border on card */
		    XDrawRectangle(dpy, table, blackgc, x, y, 
					CARD_WIDTH, delta);
		}
		card_is_clipped = True;
	} else	{	/* fill all the card */
#ifdef ROUND_CARDS
		if (round_cards)	{
		    /* fill the background */
		    XmuFillRoundedRectangle(dpy, table, whitegc, x, y, 
			CARD_WIDTH, CARD_HEIGHT,
			ROUND_W, ROUND_H);
		    /* draw border on card */
		    XmuDrawRoundedRectangle(dpy, table, blackgc, x, y, 
			CARD_WIDTH, CARD_HEIGHT,
			ROUND_W, ROUND_H);
		} else
#endif
		{
		    /* fill the background */
		    XFillRectangle(dpy, table, whitegc, x, y, 
			CARD_WIDTH, CARD_HEIGHT);
		    /* draw border on card */
		    XDrawRectangle(dpy, table, blackgc, x, y, 
			CARD_WIDTH, CARD_HEIGHT);
		}
		card_is_clipped = False;
	}

	switch (rank)	{
	case	King:
		draw_king(suit, x, y);
		break;
	case	Queen:
		draw_queen(suit, x, y);
		break;
	case	Jack:
		draw_jack(suit, x, y);
		break;

	case	Ten:
		draw_pip(suit, MID_CARD_X + x, CARD_TEN_Y1 + y);
		draw_did(suit, MID_CARD_X + x, CARD_TEN_Y2 + y);
		draw_eight_pips(suit, x, y);
		break;

	case	Nine:
		draw_pip(suit, x + MID_CARD_X, y + MID_CARD_Y);
		draw_eight_pips(suit, x, y);
		break;

	case	Eight:
		draw_did(suit, x + MID_CARD_X, y + CARD_EIGHT_Y);
		/* fall thru */
	case	Seven:
		draw_pip(suit, MID_CARD_X + x, CARD_SEVEN_Y + y);
		/* fall thru */
	case	Six:
		draw_six_pips(suit, x, y);
		break;

	case	Five:
		draw_pip(suit, x + MID_CARD_X, y + MID_CARD_Y);
		/* fall thru */
	case	Four:
		draw_four_pips(suit, x, y);
		break;

	case	Three:
		draw_pip(suit, x + MID_CARD_X, y + MID_CARD_Y);
		/* fall thru */
	case	Deuce:
		draw_two_pips(suit, x, y);
		break;
	case	Ace:
		draw_center_pip(suit, x + MID_CARD_X, y + MID_CARD_Y);
		break;
	default:
		assert(0);
	}

	draw_rank(x, y, rank, suit);

	/* clear the clip mask */
	XSetClipMask(dpy, cardgc, None);
#ifdef ROUND_CARDS
	if (round_cards)	{
	    XSetClipMask(dpy, whitegc, None);
	    if (cardgc != blackgc)
		XSetClipMask(dpy, blackgc, None);
	}
#endif
}

/*
 * NOTE -- for all the pip drawers except the one that actually plots the
 * bits, the location is the card's location.  the drawer's take the
 * pip's center as location.
 */

/*
 * draws right-side-up pip
 *
 * location is for center of pip
 */
draw_pip(suit, x, y)
Suit	suit;
int	x, y;
{
int	w, h;

	switch(suit)	{
	case	Spade:
		w = spade_width;
		h = spade_height;
		break;
	case	Diamond:
		x++;
		w = diamond_width;
		h = diamond_height;
		break;
	case	Heart:
		y++;
		w = heart_width;
		h = heart_height;
		break;
	case	Club:
		y++;
		w = club_width;
		h = club_height;
		break;
	default:
		assert(0);
	}
	XCopyPlane(dpy, suit_map[suit], table, cardgc, 
		0, 0, w, h,
		x - w/2, y - h/2, 1);
}

/*
 * draws upside-down pip
 *
 * location is for center of pip
 */
draw_did(suit, x, y)
Suit	suit;
int	x,y;
{
int	w, h;

	if (card_is_clipped)	/* a clipped card never shows any did's */
		return;

	switch(suit)	{
	case	Spade:
		w = spade_width;
		h = spade_height;
		break;
	case	Diamond:
		x++;
		w = diamond_width;
		h = diamond_height;
		break;
	case	Heart:
		y++;
		w = heart_width;
		h = heart_height;
		break;
	case	Club:
		y++;
		w = club_width;
		h = club_height;
		break;
	default:
		assert(0);
	}
	XCopyPlane(dpy, suit_r_map[suit], table, cardgc, 
		0, 0, w, h,
		x - w/2, y - h/2, 1);
}

/*
 * draws big center pip
 */
draw_center_pip(suit, x, y)
Suit	suit;
int	x,y;
{
int	w, h;

	if (card_is_clipped)
		return;

	switch(suit)	{
	case	Spade:
		w = spade_lg_width;
		h = spade_lg_height;
		break;
	case	Diamond:
		w = diamond_width;
		h = diamond_height;
		break;
	case	Heart:
		w = heart_width;
		h = heart_height;
		break;
	case	Club:
		w = club_width;
		h = club_height;
		break;
	default:
		assert(0);
	}
	XCopyPlane(dpy, suit_lg_map[suit], table, cardgc, 
		0, 0, w, h,
		x - w/2, y - h/2, 1);
}

/* 
 * draw_two_pips
 */
draw_two_pips(suit, x, y)
Suit	suit;
int	x,y;
{
	draw_pip(suit, x + MID_CARD_X, y + CARD_ROW1_Y);
	draw_did(suit, x + MID_CARD_X, y + CARD_ROW5_Y);
}

/*
 * draw_four_pips
 */
draw_four_pips(suit, x, y)
Suit	suit;
int	x,y;
{
	draw_pip(suit, x + CARD_COL1_X, y + CARD_ROW1_Y);
	draw_did(suit, x + CARD_COL1_X, y + CARD_ROW5_Y);

	draw_pip(suit, x + CARD_COL3_X, y + CARD_ROW1_Y);
	draw_did(suit, x + CARD_COL3_X, y + CARD_ROW5_Y);
}

draw_six_pips(suit, x, y)
Suit	suit;
int	x, y;
{
	draw_pip(suit, x + CARD_COL1_X, y + CARD_ROW1_Y);

	draw_pip(suit, x + CARD_COL3_X, y + CARD_ROW1_Y);

	if (card_is_clipped)
		return;

	/* these are only visible when its not clipped */
	draw_pip(suit, x + CARD_COL1_X, y + CARD_ROW3_Y);
	draw_did(suit, x + CARD_COL1_X, y + CARD_ROW5_Y);

	draw_pip(suit, x + CARD_COL3_X, y + CARD_ROW3_Y);
	draw_did(suit, x + CARD_COL3_X, y + CARD_ROW5_Y);
}

draw_eight_pips(suit, x, y)
Suit	suit;
int	x,y;
{
	draw_pip(suit, x + CARD_COL1_X, y + CARD_ROW1_Y);

	draw_pip(suit, x + CARD_COL3_X, y + CARD_ROW1_Y);

	if (card_is_clipped)
		return;

	/* these are only visible when its not clipped */
	draw_pip(suit, x + CARD_COL1_X, y + CARD_ROW2_Y);
	draw_did(suit, x + CARD_COL1_X, y + CARD_ROW4_Y);
	draw_did(suit, x + CARD_COL1_X, y + CARD_ROW5_Y);

	draw_pip(suit, x + CARD_COL3_X, y + CARD_ROW2_Y);
	draw_did(suit, x + CARD_COL3_X, y + CARD_ROW4_Y);
	draw_did(suit, x + CARD_COL3_X, y + CARD_ROW5_Y);
}

draw_jack(suit, x, y)
Suit	suit;
int	x,y;
{
	XCopyPlane(dpy, jack_map[suit], table, cardgc, 
		0, 0, FACECARD_WIDTH, FACECARD_HEIGHT,
		x + (CARD_WIDTH - FACECARD_WIDTH)/2, 
		y + (CARD_HEIGHT - FACECARD_HEIGHT)/2, 1);

	XDrawRectangle(dpy, table, cardgc,
		x + (CARD_WIDTH - FACECARD_WIDTH)/2, 
		y + (CARD_HEIGHT - FACECARD_HEIGHT)/2,
		FACECARD_WIDTH, FACECARD_HEIGHT);
}

draw_queen(suit, x, y)
Suit	suit;
int	x,y;
{
	XCopyPlane(dpy, queen_map[suit], table, cardgc,
		0, 0, FACECARD_WIDTH, FACECARD_HEIGHT,
		x + (CARD_WIDTH - FACECARD_WIDTH)/2, 
		y + (CARD_HEIGHT - FACECARD_HEIGHT)/2, 1);

	XDrawRectangle(dpy, table, cardgc,
		x + (CARD_WIDTH - FACECARD_WIDTH)/2, 
		y + (CARD_HEIGHT - FACECARD_HEIGHT)/2,
		FACECARD_WIDTH, FACECARD_HEIGHT);
}

draw_king(suit, x, y)
Suit	suit;
int	x,y;
{
	XCopyPlane(dpy, king_map[suit], table, cardgc,
		0, 0, FACECARD_WIDTH, FACECARD_HEIGHT,
		x + (CARD_WIDTH - FACECARD_WIDTH)/2, 
		y + (CARD_HEIGHT - FACECARD_HEIGHT)/2, 1);

	XDrawRectangle(dpy, table, cardgc,
		x + (CARD_WIDTH - FACECARD_WIDTH)/2, 
		y + (CARD_HEIGHT - FACECARD_HEIGHT)/2,
		FACECARD_WIDTH, FACECARD_HEIGHT);
}

draw_rank(x, y, rank, suit)
int	x, y;
Rank	rank;
Suit	suit;
{
int	w, h;

	if (suit == Heart || suit == Diamond)	{
		XCopyPlane(dpy, rank_map_red[rank], table, cardgc,
			0, 0, RANK_WIDTH, RANK_HEIGHT,
			x + RANK_LOC_X, y + RANK_LOC_Y, 1);

		if (!card_is_clipped)
		    XCopyPlane(dpy, rank_r_map_red[rank], table, cardgc,
			0, 0, RANK_WIDTH, RANK_HEIGHT,
			x + (CARD_WIDTH - RANK_WIDTH - RANK_LOC_X), 
			y + (CARD_HEIGHT - RANK_HEIGHT - RANK_LOC_Y), 1);
	} else	{
		XCopyPlane(dpy, rank_map[rank], table, cardgc,
			0, 0, RANK_WIDTH, RANK_HEIGHT,
			x + RANK_LOC_X, y + RANK_LOC_Y, 1);

		if (!card_is_clipped)
		    XCopyPlane(dpy, rank_r_map[rank], table, cardgc,
			0, 0, RANK_WIDTH, RANK_HEIGHT,
			x + (CARD_WIDTH - RANK_WIDTH - RANK_LOC_X), 
			y + (CARD_HEIGHT - RANK_HEIGHT - RANK_LOC_Y), 1);
	}

	switch (suit)	{
		case	Spade:
			w = spade_sm_width;
			h = spade_sm_height;
			break;
		case	Heart:
			w = heart_sm_width;
			h = heart_sm_height;
			break;
		case	Diamond:
			x++;	/* offset the smaller width */
			w = diamond_sm_width;
			h = diamond_sm_height;
			break;
		case	Club:
			w = club_sm_width;
			h = club_sm_height;
			break;
		default:
			assert(0);
	}
	XCopyPlane(dpy, suit_sm_map[suit], table, cardgc,
		0, 0, w, h,
		x + SMALL_LOC_X, y + SMALL_LOC_Y, 1);

	if (!card_is_clipped)
	    XCopyPlane(dpy, suit_sm_r_map[suit], table, cardgc,
		0, 0, w, h,
		x + (CARD_WIDTH - w - SMALL_LOC_X), 
		y + (CARD_HEIGHT - h - SMALL_LOC_Y), 1);
}

#else	/* SMALL_CARDS */

paint_card(x, y, rank, suit, delta)
int	x, y;
Rank	rank;
Suit	suit;
int	delta;
{
int	card_number;

	if (suit == Spade || suit == Club)      {
		cardgc = is_color ? blackgc: whitegc;
	} else  {
		cardgc = redgc;
	}

	if (delta)	{
		cliprects[0].height = delta;
		XSetClipRectangles(dpy, cardgc, x, y, cliprects, 1, Unsorted);
	}

	/* this is messy cause these are just straight xsol cards */
	switch (suit)	{
		case	Spade:
			card_number = 3 * NUM_RANKS;
			break;
		case	Heart:
			card_number = 0;
			break;
		case	Club:
			card_number = 2 * NUM_RANKS;
			break;
		case	Diamond:
			card_number = NUM_RANKS;
			break;
	}
	card_number += rank;

	XCopyPlane(dpy, card_bitmaps[card_number], table, cardgc, 0, 0, 
		CARD_WIDTH, CARD_HEIGHT, x, y, 1);

	if (delta)
		XSetClipMask(dpy, cardgc, None);
}
#endif	/* SMALL_CARDS */


/*
 * message handling
 */

#ifdef KITLESS
void
show_message(str)
char	*str;
{
static	char last_message[512];

	if (str)	{
		(void)strcpy(last_message, str);
	}
	XClearWindow(dpy, message_win);
	XDrawImageString(dpy, message_win, textgc, MESSAGE_X, message_y,
		last_message, strlen(last_message));
}
#endif	/*KITLESS*/

void
card_message(str, card)
char	*str;
CardPtr	card;
{
char	buf[512];

	(void)sprintf(buf, "%s %s of %s", 
		str, rank_name(card->rank), suit_name(card->suit));
	show_message(buf);
}

void
card2_message(str1, card1, str2, card2)
char	*str1, *str2;
CardPtr	card1, card2;
{
char	buf[512];

	(void)sprintf(buf, "%s %s of %s %s %s of %s", 
		str1, rank_name(card1->rank), suit_name(card1->suit),
		str2, rank_name(card2->rank), suit_name(card2->suit));
	show_message(buf);
}

void
clear_message()
{
	show_message(" ");
}

#ifndef SMALL_CARDS
static unsigned char _reverse_byte[0x100] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

#define S(x,y) src[(H-1-(y))*W+(x)]
#define D(x,y) dst[(H-1-(y))*W+(x)]

flip_bits(src, dst, W, H)
unsigned char	*src, *dst;
int	W, H;
{
int	x, y;

	W = (W + 7)/8;
	for (y = 0; y < H; y++)	{
		for (x = 0; x < W; x++)	{
			D (x, y) = S (x, H - 1 - y);
		}
	}
}

rot_180(src, dst, W, H)
unsigned char   *src, *dst;
int	W, H;
{
int     x, y;
int	width = W;
unsigned char	*new;
int	bit;

	W = (W + 7)/8;
	for (y = 0; y < H; y++) {
		for (x = 0; x < W; x++) {
			D (x, y) = _reverse_byte[S (W - 1 - x, H - 1 - y)];
		}
	}

	/* shift it over */
	new = (unsigned char *)calloc((unsigned)W*H, (unsigned)1);
	for (y = 0; y < H; y++)	{
		for (x = 0; x < W*8; x++)	{
			bit = (*(dst + (x + (W*8 - width))/8 + y * W)
				& (1 << ((x + (W*8 - width)) % 8))) ? 1 : 0;
			*(new + x/8 + y*W) = (bit << (x%8)) | 
				(*(new + x/8 + y*W) & ~(1 << (x%8)));
		}
	}
	bcopy((char *)new, (char *)dst, W*H);
	free((char *)new);
}
#endif	/* !SMALL_CARDS */
