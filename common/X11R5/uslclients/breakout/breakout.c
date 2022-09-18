#ifndef NOIDENT
#ident	"@(#)breakout:breakout.c	1.1"
#endif
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Breakout--classic game, conceived by someone else, adapted for X: 7/89
 *           This Version (2.0) adapted 10/90.  Some game concepts based
 *           slightly on Arkanoid arcade game.
 *
 * Invocation string: breakout [-m] [-l level]		(-m for monochrome)
 *
 * Object: Knock out all the blocks in the wall for points.
 *	   Knocking out all blocks brings up a new wall.
 *
 * Directions: Click any button to get out of the demo.
 * 	       Btn 1 to move paddle to click position (horizontal only),
 *			start ball, and start new level;
 * 	       btn 2 to pause the ball (resume with btn 1);
 *	       btn 3 to quit the game.
 *
 * Progression of the Game:
 *	The ball gets faster the longer you play it.  Needless to say,
 *	this is faster and faster as you progress through levels.
 *	The player gets a new ball every fourth level.
 *	Points are 5 times row (bottom is 1) times level.
 *	After every level, the background, ball, and paddle colors change.
 *
 * Special instructions:
 *	Some bricks are special: Silver (grey) bricks take 2 or more hits
 *	to destroy (another after each 16 levels).  Gold (orange) bricks
 *	are virtually indestructible (10,000 hits to destroy).  The gold
 *	bricks do not need to be destroyed in order to finish a level.
 *
 *	breakout saves 5 high scores.  The default file for
 *	these scores is brkout.scr in the user's home
 *	directory.  The user can change this by having a
 *	line in the .Xdefaults file of the home directory
 *	that looks like this:
 *
 *	breakout.scoreFile:	/usr2/games/brk/SCORES  (or some other file).
 *
 * Notes: The demo will not play forever.  Occasionally, it misses the ball.
 *	It WILL restart when it's done.
 *
 *	This game can be compiled with OPEN LOOK 2.0 or earlier.  If
 *	later, the PIXMAP flag should be included with the CFLAGS in
 *	the Makefile.  Also, the OPEN LOOK client lib will be needed.
 *      This will use the new pixmap icon instead of the bitmap icon.
 *
 * Known bugs: The algorithm for figuring out if the ball hit the SIDE of a 
 *	       brick is flawed.  Bug fixes are welcomed and encouraged.
 *	       Email to scottn@usl.com.
 */

#include <stdio.h>
#include <ctype.h>  
#include <math.h>
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include "misc.h"  

#ifdef PIXMAP
#include "brkout.xpm"
#else  
#include "brkout.icon"
#endif  

#define START_X	10
#define	START_Y	20
#define WINDOW_WIDTH	400
#define WINDOW_HEIGHT	300
#define	BORDER_WIDTH	1
#define OVER "GAME OVER"
#define FONT "-sony-fixed-medium-r-normal--24-170-100-100-c-120-jisx0201.1976-0"
#define BRICKS 30
#define PADDLE_HALF 25
#define FACTOR	1
#define GOLD -9999

static int SILVER = -1;
static int CurrentBallSpeed = 3;	/* speed factor for balls */
static int StartBallSpeed = 3;		/* speed at start of level */
static int ballcount = 5;	/* # of balls */
static int brickcount = BRICKS;	/* # of bricks */
static int points = 0;		/* points value */
static short multiplier = 1;	/* points multiplier */
static int demo = 1;		/* demo mode flag */
static int mono = 0;		/* monochrome option */
static Display *the_display;
static Window the_window;
static int the_screen;
static Window root_window;
static XSizeHints size_hints;
static XEvent the_event;
static XButtonEvent *btn_ev;
static GC the_GC, erase, blue_GC, red_GC, green_GC, white_GC, black_GC,
	  back_GC, gray_GC, gold_GC;
static XGCValues values;
static XFontStruct *fs;
static XWindowAttributes xwa;
static unsigned long valuemask, inputmask, BGColor;
static Window final, paddle, score, hiscore;
static int gone[BRICKS],		/* array of bricks gone */
  reverse = 0,			/* flag on ball direction */
  hit = 1;			/* flag on whether bricks can be hit */
static int numhits;		/* number of hits on current ball at speed */
static int angle, angle2, angle3,
  angle4, angle5; 		/* integers for trig routines */
static int m = FACTOR, n = FACTOR,
  posx = 1, posy = 150, indx = 0;
static int paddlex,		/* paddle x-coordinate */
  bzzt,				/* flag if ball missed */
  go = 0,			/* flag to move the ball */
  nasty = 0;			/* flag if nasty angles to use */
struct scrfile {		/* structure for high scores */
  char name[20];		/* initials */
  int scr;			/* points */
  int lvl;			/* screen level */
} hiscr[5];			/* 5 entries total */
static char shn[20];		/* score names holder*/
static int scr[5];			/* high scores holder*/
static int hix, finx;		/* center point in various windows */
static int offset = 0;		/* offset for demo */
static int sign = 1;
static unsigned long backg[16], foreg[16];
static int currentx = 0, currenty = 0;
static XRectangle rects[3] = {
  {  0,  0, 40, 20 },
  { 10, 20, 40, 10 },
  { 40, 10, 10, 10 }
};

/* function prototypes */

static long t_get_pixel ();
static int getoffset();
static Pixmap t_iconic();
static void DrawBall();
static void DrawPaddle();
static void DrawBrick();
static void reset();
static int NewGrays();
static void CreateGCs();
static void event_handler();
static void RedrawPlayfield();
static void torpedo();
static void move_ball();
static void init_colors();
static void change_bg();
static int new_high();
static void bsort();
static void outcode();
static int nobrick();
static void collision();


/* functions to do trig and other math functions with integers */

#define msin(x) mcos(x-90)
#define muldiv(a, b, c)	((long)((a)*((long)(b))/(c)))

static int
getoffset(off)
  int off;
{
  if ((off < PADDLE_HALF-4) && (off > -PADDLE_HALF+4))
    sign *= -1;
  return(sign - off);
}

/* Bring in icon bitmap */

static Pixmap
t_iconic (dpy)
  Display *dpy;
{
  Pixmap pid;
  Window   root = XDefaultRootWindow (dpy);
  int    screen = XDefaultScreen(dpy);
  Colormap cmap = XDefaultColormap (dpy, screen);
  int     depth = DefaultDepth(dpy, screen);

#ifdef PIXMAP
  pid = XCreatePixmapFromData(dpy, root, cmap, brkout_width, brkout_height,
			      depth, brkout_ncolors,
			      brkout_chars_per_pixel, brkout_colors,
			      brkout_pixels);
#else
  pid = XCreateBitmapFromData(dpy, root, (char *)brkout_bits, brkout_width, brkout_height);
#endif 

  return (pid);
}

static void
DrawBall(d, w, gc, x, y)
  Display *d;
  Window w;
  GC gc;
  int x, y;
{
  XFillRectangle(d, w, gc, x-2, y-2, 3, 3);
}

static void
DrawPaddle(d, w, gc, x, y)
  Display *d;
  Window w;
  GC gc;
  int x, y;
{
  XFillRectangle(d, w, gc, x-PADDLE_HALF, y-3, PADDLE_HALF*2, 3);
}

/* Draw a brick */

static void
DrawBrick(d, w, gc, num, row, shadow)
  Display *d;
  Window w;
  GC gc;
  int num, row, shadow;
{
  int x = (40*num);
  int y = 40 + (20*row);
  int h = 20;
  int wd = 40;

  if (gc != erase) {
    if (shadow) XFillRectangle(d, w, black_GC, x+10, y+10, wd, h);
    XFillRectangle(d, w, gc, x+1, y+1, wd-2, h-2);
    XFillRectangle(d, w, white_GC, x, y, wd, 1);
    XFillRectangle(d, w, white_GC, x, y, 1, h);
    XFillRectangle(d, w, black_GC, x, y+h-1, wd, 1);
    XFillRectangle(d, w, black_GC, x+wd-1, y, 1, h);
  }
  else {
    if (!mono) XFillRectangle(d, w, back_GC, x+10, y+10, wd, h);
    XFillRectangle(d, w, back_GC, x, y, wd, h);
  }
}

static void 
reset(start)
  int start;
{
  int i;
  char str[10];
  
  CurrentBallSpeed = StartBallSpeed = 3 + ((start+1)/4);
  numhits = 0;
  ballcount = 5;
  brickcount = BRICKS;
  points = 0;
  multiplier = 1+start;
  demo = 1;
  reverse = 0;
  hit = 1;
  m = FACTOR; n = FACTOR; posx = 1; posy = 150; indx = 0;
  
  sprintf(str, "%4d", points);
  XDrawString(the_display, score, the_GC, 0, 20, str, strlen(str));
  
  for (i = 0; i< BRICKS; i++)
    gone[i] = 0;

  if (!mono) NewGrays(gone, start);

  /* Draw the wall */
  
  paddlex = 180;

  if (start != 0) change_bg(the_display, start);

  RedrawPlayfield(the_display, the_window, 1);
}

static int
NewGrays(g, lvl)
  int g[], lvl;
{
  int i;

  for (i = 0; i < BRICKS; i++)
    gone[i] = 0;

  switch((lvl+1)%16) {
  case 1:  /* introduction of silver blocks (> 1 hit to destroy) */
    gone[4] = gone[5] = SILVER; 
    break;
  case 2:  /* two columns of silver */
    gone[3] = gone[13] = gone[23] = gone[6] = gone[16] = gone[26] = SILVER;
    break;
  case 3:  /* top row silver */
    for(i = 0; i < 10; i++) gone[i] = SILVER; 
    break;
  case 4:  /* smiley I */
    gone[3] = gone[6] = gone[22] = gone[27] = SILVER;
    gone[11] = gone[18] = gone[23] = gone[24] = gone[25] = gone[26] = SILVER;
    break;
  case 5:  /* bottom row sivler */
    for (i = 20; i < 30; i++) gone[i] = SILVER; 
    break;
  case 6:  /* big 'O' silver */
    for(i = 0; i < 10; i++) gone[i] = gone[20+i] = SILVER;
    gone[10] = gone[19] = SILVER; 
    break;
  case 7:  /* every other silver */
    for(i = 0; i < 10; i+=2) gone[i] = gone[11+i] = gone[20+i] = SILVER;
    break;
  case 8:  /* ALL silver */
    for(i = 0; i < 30; i++) gone[i] = SILVER; 
    break;
  case 9:  /* introduction of gold (indestructible) bricks */
    gone[4] = gone[5] = GOLD; brickcount -= 2; 
    break;
  case 10: /* middle row gold and silver */
    gone[14] = gone[15] = SILVER;
    gone[11] = gone[12] = gone[13] = gone[16] = gone[17] = gone[18] = GOLD;
    brickcount -= 6;
    break;
  case 11: /* smiley II */
    gone[3] = gone[6] = gone[23] = gone[24] = gone[22] = GOLD;
    gone[25] = gone[26] = gone[11] = gone[18] = gone[27] = GOLD;
    gone[14] = gone[15] = SILVER;
    brickcount -= 10;
    break;
  case 12: /* most of bottom row gold */
    for (i = 21; i < 29; i++) gone[i] = GOLD; brickcount -= 8; 
    break;
  case 13: /* not all bricks exist at start.  Only 6 bricks to hit. */
    for (i = 0; i < 5; i+=4) {  /* remove bricks */
      gone[i] = gone[10+i] = gone[20+i] = 1;
      gone[i+5] = gone[15+i] = gone[25+i] = 1;
    }
    gone[1] = gone[3] = gone[6] = gone[8] = GOLD;
    gone[11] = gone[13] = gone[16] = gone[18] = GOLD;
    gone[21] = gone[22] = gone[23] = GOLD;
    gone[26] = gone[27] = gone[28] = GOLD;
    gone[4] = gone[5] = GOLD;
    gone[24] = gone[25] = SILVER;
    brickcount -= 24;
    break;
  case 14: /* most of top and bottom rows gold (tough level) */
    for (i = 1; i < 9; i++) gone[i] = gone[20+i] = GOLD; 
    brickcount -= 16;
    break;
  case 15: /* the "corridor" */
    for (i = 0; i < 10; i++) gone[i] = gone[20+i] = GOLD;
    gone[29] = gone[0] = SILVER;
    brickcount -= 18;
    break;
  default: /* no weird bricks...just a faster ball */
    break;
  }
}
  
static void
CreateGCs()
{
  /* Create all GC's: bricks in their colors, rest in white */
  
  valuemask = GCFont|GCForeground|GCGraphicsExposures|GCBackground;
  values.font = fs->fid;
  values.foreground = foreg[0];
  values.background = backg[0];
  values.graphics_exposures = FALSE;
  the_GC = XCreateGC(the_display, the_window, valuemask, &values);
  white_GC = XCreateGC(the_display, the_window, valuemask, &values);
  XSetFunction(the_display, the_GC, GXxor);
  values.foreground = t_get_pixel(the_display, "blue");
  blue_GC = XCreateGC(the_display, the_window, valuemask, &values);
  values.foreground = t_get_pixel(the_display, "red");
  red_GC = XCreateGC(the_display, the_window, valuemask, &values);
  values.foreground = t_get_pixel(the_display, "forest green");
  green_GC = XCreateGC(the_display, the_window, valuemask, &values);
  valuemask = GCForeground|GCBackground|GCGraphicsExposures;
  values.background = WhitePixel(the_display, the_screen);
  values.foreground = BlackPixel(the_display, the_screen);
  values.graphics_exposures = FALSE;
  erase = XCreateGC(the_display, the_window, valuemask, &values);
  black_GC = XCreateGC(the_display, the_window, valuemask, &values);
  back_GC = XCreateGC(the_display, the_window, valuemask, &values);
  XSetFunction(the_display, black_GC, GXcopy);
  XSetFunction(the_display, back_GC, GXcopy);
  values.foreground = t_get_pixel(the_display, "gray");
  gray_GC = XCreateGC(the_display, the_window, valuemask, &values);
  XSetFunction(the_display, gray_GC, GXcopy);
  values.foreground = t_get_pixel(the_display, "gold");
  gold_GC = XCreateGC(the_display, the_window, valuemask, &values);
  XSetFunction(the_display, gold_GC, GXcopy);
}

static void
event_handler(d, w, ev)
  Display *d;
  Window w;
  XEvent *ev;
{
  XButtonEvent *btn, *dumbtn;
  XExposeEvent *expo;
  XKeyEvent *key;
  XEvent dummy;
  int j;
  char a_char[1];

  btn = (XButtonEvent *) ev;
  key = (XKeyPressedEvent *) ev;
  expo = (XExposeEvent *) ev;

  if (ev->type == ButtonPress) {
    if (demo) {
      bzzt = 1;
      go = 0;
    }
    else if ( btn->button == 3) { /* exit */
      exit(0);
    }
    else if ( btn->button == 2) { /* pause */
      go = 0;
    }
    else if (btn->button == 1) { /* move paddle */
      go = 1;
      DrawPaddle(d, w, the_GC, paddlex, WINDOW_HEIGHT);
      paddlex = btn->x;
      DrawPaddle(d, w, the_GC, paddlex, WINDOW_HEIGHT);
    }
    else fprintf(stderr, "oops.\n");
  }
  else if (ev->type == Expose) {
    if (!expo->count)
      RedrawPlayfield(the_display, the_window, 1);
  }
#ifdef OLD  
  else if (ev->type == KeyPress) {
    KeySym sym;
    
    XLookupString(key, a_char, 1, &sym, NULL);
    if (!IsModifierKey(sym)) {
      hiscr[4].name[indx] = a_char[0];
      XDrawString(the_display, hiscore, the_GC, 80+20*indx, 100,
		  &(hiscr[4].name[indx]), 1);
    }
    else {
      XWindowEvent(the_display, hiscore, inputmask, &the_event);
      event_handler(the_display, hiscore, &the_event);
    }
  }
#endif  
}

static void
RedrawPlayfield(d, w, all)
  Display *d;
  Window w;
  int all;
{
  int i;
  char str[10];

  /* redraw existing bricks */

  if (all) {
    XClearWindow(d, w);
    XClearWindow(d, score);
  }
  else {
    if (mono)
      XSetClipRectangles(d,the_GC,currentx,currenty, rects, 3, Unsorted);
    else {
      XSetClipRectangles(d,red_GC,currentx,currenty, rects, 3, Unsorted);
      XSetClipRectangles(d,blue_GC,currentx,currenty, rects, 3, Unsorted);
      XSetClipRectangles(d,green_GC,currentx,currenty, rects, 3, Unsorted);
      XSetClipRectangles(d,gray_GC,currentx,currenty, rects, 3, Unsorted);
      XSetClipRectangles(d,gold_GC,currentx,currenty, rects, 3, Unsorted);
      XSetClipRectangles(d,black_GC,currentx,currenty, rects, 3, Unsorted);
      XSetClipRectangles(d,white_GC,currentx,currenty, rects, 3, Unsorted);
    }
  }

  for (i=0; i < 30; i++) {
    if (gone[i] < -100) {
      if (mono) DrawBrick(d, w, the_GC, i%10, i/10, !mono);
      else DrawBrick(d, w, gold_GC, i%10, i/10, !mono);
    }
    else if (gone[i] < 0) {
      if (mono) DrawBrick(d, w, the_GC, i%10, i/10, !mono);
      else DrawBrick(d, w, gray_GC, i%10, i/10, !mono);
    }
    else if (!gone[i]) {
      if (mono) DrawBrick(d, w, the_GC, i%10, i/10, !mono);
      else if (i/10 == 0) DrawBrick(d, w, red_GC, i%10, i/10, !mono);
      else if (i/10 == 1) DrawBrick(d, w, blue_GC, i%10, i/10, !mono);
      else if (i/10 == 2) DrawBrick(d, w, green_GC, i%10, i/10, !mono);
    }
  }
  if (!all) {
    if (mono)
      XSetClipMask(d, the_GC, None);
    else {
      XSetClipMask(d, red_GC, None);
      XSetClipMask(d, green_GC, None);
      XSetClipMask(d, blue_GC, None);
      XSetClipMask(d, gold_GC, None);
      XSetClipMask(d, gray_GC, None);
      XSetClipMask(d, black_GC, None);
      XSetClipMask(d, white_GC, None);
    }
  }
  else {
  /* redraw ball, paddle, score */
    DrawPaddle(d, w, the_GC, paddlex, WINDOW_HEIGHT);
    if (bzzt != 1)
      DrawBall(d, w, the_GC, posx, posy);
    sprintf(str, "%4d", points);
    XDrawString(the_display, score, the_GC, 0, 20, str, strlen(str));
  }
}

static void
torpedo(d, w, gc, x, y)
  Display *d;
  Window w;
  GC gc;
  int x, y;
{
  int i, j, k, l;
  
  angle = matan2(x - 1, y - 250);

  angle2 = (x-1 >= 0) ? muldiv(3, mcos(angle), 1024) :
    muldiv(-3, mcos(angle), 1024);
  angle3 = (y-250 >= 0) ? muldiv(3, msin(angle), 1024) :
    muldiv(-3, msin(angle), 1024);
  
  angle = matan2(2*x - 1, y - 250);
  angle4 = (x-1 >= 0) ? muldiv(3, mcos(angle), 1024) :
    muldiv(-3, mcos(angle), 1024);
  angle5 = (y-250 >= 0) ? muldiv(3, msin(angle), 1024) :
    muldiv(-3, msin(angle), 1024);
}

/* absolute value function */
#define abso(x)	((x) > 0 ? (x) : -(x))

static void
move_ball(d, w, gc, startlevel)
  Display *d;
  Window w;
  GC gc;
  int startlevel;
{
  int k, l, prevx = posx, prevy = posy;
  long q;
  char str[30];

  for (q=4500*FACTOR;q>0;q--);
  DrawBall(d, w, the_GC, posx, posy);
  if (demo)
    DrawPaddle(d, w, gc, paddlex, WINDOW_HEIGHT);
  if (!nasty) {
    posx += (angle2 - ((CurrentBallSpeed - 2)/2)) * n;
    posy += (angle3 + ((CurrentBallSpeed - 2)/2)) * m;
  }
  else {
    posx += (angle4 - ((CurrentBallSpeed - 2)/2)) * n;
    posy += (angle5 + ((CurrentBallSpeed - 2)/2)) * m;
  }
  k = posx;
  l = posy;

#ifdef OLD  
  if (l <= 100 && l >= 40)
    for (q=0; q < BRICKS/10; q++)
      collision(&q, &k, &l, d, w, &reverse, &hit, &m, &n, prevx, prevy);
#else
  if (l < 100 && l >= 40) {
    int tmp = (l - 40)/20;
    collision(&tmp, &k, &l, d, w, &reverse, &hit, &m, &n, prevx, prevy);
  }
#endif  
  
  if (l < 0 && q < 10) {
    l = -l;
    m = -m;
    reverse = 1;
  }
  if (k > WINDOW_WIDTH - 2) {
    k = WINDOW_WIDTH-2 - (k - (WINDOW_WIDTH-2));
    n = FACTOR;
  }
  else if (k < 0) {
    k = -k + 1;
    n = -FACTOR;
  }
  if (l >= WINDOW_HEIGHT - 4 && (abso(k - paddlex) <= PADDLE_HALF)) {
    reverse = 0;
    l = WINDOW_HEIGHT - (l - WINDOW_HEIGHT);
    m = -FACTOR;
    hit = 1;
    numhits++;
    offset = getoffset(offset);
    if (abso(k - paddlex) > (PADDLE_HALF*2/3))
      nasty = 1;
    else nasty = 0;
    if (k < paddlex) {
      if (n < 0) {
	n = -n;
	k = prevx;
      }
    }
    else
      if (n > 0) {
	n = -n;
	k = prevx;
      }
    if ((CurrentBallSpeed - StartBallSpeed <= multiplier/2) && 
	(numhits > BRICKS/multiplier)) {
      numhits = 0;
      CurrentBallSpeed++;
    }
  }
  else if (l > WINDOW_HEIGHT && (abso(k - paddlex) > PADDLE_HALF)) {
    bzzt = 1;
    CurrentBallSpeed = StartBallSpeed;
    numhits = 0;
    for (q = 6; q > 0; q--) {
      int qq;
      DrawPaddle(d, w, the_GC, paddlex, WINDOW_HEIGHT);
      XSync(the_display, True);
      for (qq=100000; qq > 0; qq--);
    }
    DrawPaddle(d, w, the_GC, paddlex, WINDOW_HEIGHT);
    XSync(the_display, True);
    for (q = 100000; q > 0; q--);
    XMapWindow(d, final);

    if (ballcount - 1 != 0) {
      if (ballcount == 2) 
        sprintf(str, "Last ball");
      else
        sprintf(str, "%d balls left", ballcount - 1);
      XGetWindowAttributes(the_display, final, &xwa);
      finx = (xwa.width - XTextWidth(fs, str, strlen(str)))/2;
      XDrawString(the_display, final, the_GC, finx,20, str, strlen(str));
    }
    XSync(the_display, True);
    for (q = 600000; q > 0; q--);
    XUnmapWindow(d, final);
  }
  DrawBall(d, w, gc, posx, posy);
  if (demo) {
    paddlex = posx + offset;
    DrawPaddle(d, w, gc, paddlex, WINDOW_HEIGHT);
  }
  if (brickcount == 0) {
    for (q = 0; q< BRICKS; q++)
      gone[q] = 0;
    brickcount = BRICKS;
    if (!mono) {
      NewGrays(gone, multiplier);
      change_bg(the_display, multiplier);
    }
    RedrawPlayfield(the_display, the_window, 1);
    hit = 0;
    numhits = 0;
    if (multiplier % 4 == 0) {
      StartBallSpeed++;
      CurrentBallSpeed = StartBallSpeed;
      ballcount++;
    }
    multiplier++;
    if (multiplier%16 == 0) SILVER--;
    XMapWindow(the_display, final);
    sprintf(str, "Level%s%d", (multiplier < 10) ? "  " : " ", multiplier);
    XGetWindowAttributes(the_display, final, &xwa);
    finx = (xwa.width - XTextWidth(fs, str, strlen(str)))/2;
    XDrawString(the_display, final, the_GC, finx, 20, str, strlen(str));
  go = demo;
  while (!go)
    if (XPending(the_display)) {
      XNextEvent(the_display, &the_event);
      event_handler(the_display, the_window, &the_event);
    }
    XUnmapWindow(the_display, final);
  }
}

static long 
t_get_pixel (dpy, color_name)
  Display *dpy;
  char	*color_name;
{
  Colormap cmap = XDefaultColormap (dpy, the_screen);
  XColor d1, d2;

  if (XAllocNamedColor (dpy, cmap, color_name, &d1, &d2) == False)
  {
    char buff [100];

    sprintf (buff, "bad color name: %s", color_name);
  }
  return (d2.pixel);
}

static void
init_colors(d)
  Display *d;
{
  int w = t_get_pixel(d, "white");
  int b = t_get_pixel(d, "black");
  int i;

  backg[0] = b;
  backg[1] = w;
  backg[2] = t_get_pixel(d, "gray");
  backg[3] = t_get_pixel(d, "navy");
  backg[4] = t_get_pixel(d, "blue");
  backg[5] = t_get_pixel(d, "skyblue");
  backg[6] = t_get_pixel(d, "cyan");
  backg[7] = t_get_pixel(d, "forest green");
  backg[8] = t_get_pixel(d, "green");
  backg[9] = t_get_pixel(d, "light green");
  backg[10] = t_get_pixel(d, "brown");
  backg[11] = t_get_pixel(d, "orange");
  backg[12] = t_get_pixel(d, "yellow");
  backg[13] = t_get_pixel(d, "violet");
  backg[14] = t_get_pixel(d, "magenta");
  backg[15] = t_get_pixel(d, "red");
  for(i = 0; i < 16; i++)
    switch(i) {
    case 0:
    case 3:
    case 4:
    case 7:
    case 10:
    case 13:
    case 15:
      foreg[i] = backg[i] ^ w; break;
    default:
      foreg[i] = backg[i] ^ b; break;
    }
}

static void
change_bg(d, num)
  Display *d;
  int num;
{
  BGColor = backg[num%16];

  XSetBackground(d, the_GC, BGColor);
  XSetBackground(d, red_GC, BGColor);
  XSetBackground(d, blue_GC, BGColor);
  XSetBackground(d, green_GC, BGColor);
  XSetForeground(d, erase, BGColor);
  XSetForeground(d, back_GC, BGColor);
  XSetForeground(d, the_GC, foreg[num%16]);
  
  XSetWindowBackground(d, the_window, BGColor);
  XSetWindowBackground(d, score, BGColor);
  XSetWindowBackground(d, final, BGColor);
  XSetWindowBackground(d, hiscore, BGColor);
  
  XClearWindow(d, the_window);
  XClearWindow(d, score);
}

static int
new_high(hi, pts)
  struct scrfile hi[5];
  int pts;
{
  int i, j, k;
  char msg[15], msg2[20];
  static int firsttime = 1;
  static char *login = NULL;
  int flag = 0;

#ifndef OLD  
  if (firsttime) {
    login = (char *)getenv("LOGNAME");
    firsttime = 0;
  }
  for (i = 0; i < 5; i++) {
    if (!strcmp(hi[i].name, login))
      if (hi[i].scr >= pts)
	return -1;
    else {
      hi[i].scr = pts;
      hi[i].lvl = multiplier;
      strcpy(hi[i].name, login);
      flag = 1;
    }
  }
  if (!flag) {
    strcpy(hi[4].name, login);
#endif    
  hi[4].scr = pts;
  hi[4].lvl = multiplier;
#ifdef OLD 
  XMapWindow(the_display, hiscore);
  XSetInputFocus(the_display, hiscore, RevertToParent, CurrentTime);
  XGetWindowAttributes(the_display, hiscore, &xwa);
  strcpy(msg, "HIGH SCORE!!");
  strcpy(msg2, "Enter 3 initials");
  hix = (xwa.width - XTextWidth(fs, msg, strlen(msg)))/2;
  XDrawString(the_display, hiscore, the_GC, hix,25, msg, strlen(msg));
  hix = (xwa.width - XTextWidth(fs, msg2, strlen(msg2)))/2;
  XDrawString(the_display, hiscore, the_GC, hix,50, msg2, strlen(msg2));
  inputmask = KeyPressMask;
  XSelectInput(the_display, hiscore, inputmask);
  for (i = 0; i < 3; i++) {
    XWindowEvent(the_display, hiscore, inputmask, &the_event);
    event_handler(the_display, hiscore, &the_event);
    if (indx == 2) {
      indx = 0;
      inputmask = NoEventMask;
      XSelectInput(the_display, hiscore, inputmask);
   }
    else indx++;
  }
#endif
#ifndef OLD
  }
#endif
  bsort(hi);
 
  XClearWindow(the_display, hiscore);
  XUnmapWindow(the_display, hiscore);
}

static void
bsort(hi)
  struct scrfile hi[5];
{
  int tmp, i;
  char ctmp[4];

  for (i = 4; i >0; i--) {
    if (hi[i].scr > hi[i-1].scr) {
      tmp = hi[i-1].scr;
      hi[i-1].scr = hi[i].scr;
      hi[i].scr = tmp;
      tmp = hi[i-1].lvl;
      hi[i-1].lvl = hi[i].lvl;
      hi[i].lvl = tmp;
      strcpy(ctmp, hi[i-1].name);
      strcpy(hi[i-1].name, hi[i].name);
      strcpy(hi[i].name, ctmp);
    }
    else continue;
  }
}

#ifdef OLD
collision(row, x, y, d, w, rev, hit, m,n, prevx, prevy)
  int *row, *x, *y, *rev, *hit, *m,*n;
  int prevx, prevy;
  Display *d;
  Window w;
{
  int pos = (*x)/40;
  int nextpos = ((*x)+1)/40;
  int prevpos = ((*x)-1)/40;
  int brk = pos + (10*(*row));
  int max = 60 + ((*row) * 20);
  int min = 40 + ((*row) * 20);
  char str[10];
  GC tmp_GC;

  if (pos == 0) prevpos = 0;
  else if (pos == 9) nextpos = 9;

  switch (*row) {
  case '0': tmp_GC = red_GC; break;
  case '1': tmp_GC = blue_GC; break;
  case '2': tmp_GC = green_GC; break;
  }
  
  if ((*y < max) && (*y >= min) && (gone[brk] != 1) && 
      (*hit) && (!(*rev)) && (*x < WINDOW_WIDTH)) {
    gone[brk]++;
    if (gone[brk] == 1) {
      DrawBrick(d, w, erase, pos, *row, !mono); 
      brickcount--;
/*    RedrawNeighbors(the_display, the_window, brk);*/
      sprintf(str, "%4d", points);
      XDrawString(the_display, score, the_GC, 0, 20, str, strlen(str));
      points += (20 - (((*row)+1)*5)) * multiplier;
      sprintf(str, "%4d", points);
      XDrawString(the_display, score, the_GC, 0, 20, str, strlen(str));
    }

    if (!mono && gone[brk] >= 0) RedrawPlayfield(the_display, the_window, 0);

    if ((prevx < (pos+1)*40 && prevx >= pos*40 &&
	(prevy > max || prevy < min)) ||
	(prevx > (pos+1*40) && (brk/10 != 9) && (gone[brk+1] != 1)) ||
	(prevx < (pos*40) && (brk/10 != 0) && (gone[brk-1] != 1))){
      *y = max + (max - *y);
      *m = -(*m);
      *rev = 1;
    }
    else {
      if ((prevx >= (pos+1)*40) && (brk/10 != 9) && (gone[brk+1] == 1))
	*x = (pos+1)*40 + ((pos+1)*40 - *x);
      else if ((brk/10 != 0) && (gone[brk-1] == 1))
	*x = pos*40 - (pos*40 - *x);
      *n = -(*n);
    }
    *row = 10;
  }
  else if ((*y > min) && (*y < max) && (*hit) && (*x < WINDOW_WIDTH) &&
	     (gone[brk] != 1) && (*rev)) {
      gone[brk]++;
      if (gone[brk] == 1) {
        DrawBrick(d, w, erase, pos, *row, !mono); 
        brickcount--;
     /* RedrawNeighbors(the_display, the_window, brk);
 */
        sprintf(str, "%4d", points);
        XDrawString(the_display, score, the_GC, 0, 20, str, strlen(str));
        points += (20 - (((*row)+1)*5)) * multiplier;
        sprintf(str, "%4d", points);
        XDrawString(the_display, score, the_GC, 0, 20, str, strlen(str));
      }

      if (!mono && gone[brk] >= 0) RedrawPlayfield(the_display, the_window, 0);

      if ((prevx <= (pos+1)*40 && prevx >= pos*40) ||
	  (prevx > (pos+1)*40 && (brk/10 != 9) && (gone[brk+1] != 1)) ||
	  (prevx < (pos*40) && (brk/10 != 0) && (gone[brk-1] != 1))) {
	*y = min - (*y - min);
	*m = -(*m);
	*rev = 0;
      }
      else {
	if ((prevx > (pos+1)*40) && (brk/10 != 9) && (gone[brk+1] == 1))
	  *x = (pos+1)*40 + (pos+1)*40 - *x;
	else if ((brk/10 != 0) && (gone[brk-1] == 1))
	  *x = pos*40 - (pos*40 - *x);
	*n = -(*n);
      }
      *row = 10;
    }
}
#else
#define TOP	0x08
#define BOTTOM	0x04
#define LEFT	0x02
#define RIGHT	0x01
/*#define CORNER	0x10*/
#define top(x)		((x & TOP) == TOP)
#define bottom(x)	((x & BOTTOM) == BOTTOM)
#define left(x)		((x & LEFT) == LEFT)
#define right(x)	((x & RIGHT) == RIGHT)
/*#define corner(x)	(x & CORNER == CORNER)*/
#define betw(a,b,c)	(((a >= b) && (a <= c)) || ((a <= b) && (a >= c)))

static void
outcode(x, y, xmin, xmax, ymin, ymax, code)
  int x, y, xmin, xmax, ymin, ymax;
  unsigned short *code;
{
  unsigned short tmp = 0x0;
  
  *code = 0x0;

  /* establish where point is in relation to brick */
  if (y <= ymin) *code |= TOP;
  if (y >= ymax) *code |= BOTTOM;
  if (x <= xmin) *code |= LEFT;
  if (x >= xmax) *code |= RIGHT;

  /* can also be ON brick */
/*  if (y == ymin) tmp |= TOP;
  if (y == ymax) tmp |= BOTTOM;
  if (x == xmin) tmp |= LEFT;
  if (x == xmax) tmp |= RIGHT;*/

  /* may be a corner or side hit, if ON brick */
  
  if (*code == 0) {		/* on brick */
/*    if (tmp > 1)		/* corner hit */
/*      *code |= CORNER;*/
/*    if (y == ymin) *code |= TOP;
    if (y == ymax) *code |= BOTTOM;
    if (x == xmin) *code |= LEFT;
    if (x == xmax) *code |= RIGHT;*/
    *code |= tmp;
  }
}

static int
nobrick(direction, brick)
  short direction;
  int brick;
{
  switch (direction) {
  case TOP:
    if (brick/10 == 0) return 1;
    return (gone[brick-10] == 1);
    break;
  case BOTTOM:
    if (brick/10 == 2) return 1;
    return(gone[brick+10] == 1);
    break;
  case LEFT:
    if (brick%10 == 0) return 1;
    return(gone[brick-1] == 1);
    break;
  case RIGHT:
    if (brick%10 == 9) return 1;
    return(gone[brick+1] == 1);
    break;
  }
}

static void
collision(row, x, y, d, w, rev, hit, m,n, prevx, prevy)
  int *row, *x, *y, *rev, *hit, *m,*n;
  int prevx, prevy;
  Display *d;
  Window w;
{
  int pos = (*x)/40;
  int nextpos = ((*x)+1)/40;
  int prevpos = ((*x)-1)/40;
  int brk = pos + (10*(*row));
  int ymax = 59 + ((*row) * 20);
  int ymin = 40 + ((*row) * 20);
  int xmin = (*x)/40*40;
  int xmax = xmin + 39;
  char str[10];
  GC tmp_GC;
  unsigned short code;
  float slope, intercept;
  double tmp;
  int min, max;

  currentx = 40*(brk%10);
  currenty = 40 + 20*(brk/10);

  if (pos == 0) prevpos = 0;
  else if (pos == 9) nextpos = 9;

  /* If brick hit, destroy it, update points */
  
  if ((*y <= ymax) && (*y >= ymin) && (gone[brk] != 1) && (*hit) &&
      (*x < WINDOW_WIDTH)) {
    gone[brk]++;
    if (gone[brk] == 1) {
      XSetClipRectangles(d, back_GC, currentx, currenty, rects, 3,
			 Unsorted);
      DrawBrick(d, w, erase, pos, *row, !mono); 
      brickcount--;
      sprintf(str, "%4d", points);
      XDrawString(the_display, score, the_GC, 0, 20, str, strlen(str));
      points += (20 - (((*row)+1)*5)) * multiplier;
      sprintf(str, "%4d", points);
      XDrawString(the_display, score, the_GC, 0, 20, str, strlen(str));
      XSetClipMask(d, back_GC, None);
    }
    
    if (!mono && gone[brk] >= 0) RedrawPlayfield(the_display, the_window, 0);

    /* Now figure out which way to bounce the ball */
    
    outcode(prevx, prevy, xmin, xmax, ymin, ymax, &code);
#ifdef DEBUG    
    fprintf(stderr, "code is %o.  x= %d y=%d, prev x %d y %d\n", code, *x, *y,
	   prevx, prevy);
    fprintf(stderr, "\txmin %d xmax %d ymin %d ymax %d\n", xmin, xmax,
	    ymin, ymax);
#endif    
    
    if (code == TOP) {
      *rev = 0;
      *y = prevy;
      *m = -(*m);
    }
    else if (code == BOTTOM) {
      *rev = 1;
      *y= prevy;
      *m = -(*m);
    }
    else if ((code == LEFT) || (code == RIGHT)) {
      *x = prevx;
      *n = -(*n);
    }
    else {
      slope = (float)(*y - prevy)/(float)(*x - prevx);
      intercept = -slope * (float)*x + (float)*y;
      tmp = rint((double)(slope*(float)xmin + intercept)); min = tmp;
      tmp = rint((double)(slope*(float)xmax + intercept)); max = tmp;
#ifdef DEBUG      
      fprintf(stderr, "min %d, max %d, s %f, i %f, t %f\n", min, max,
	      slope, intercept, tmp);
#endif      
	      
      if ((left(code) && !betw(min, ymin+1, ymax-1) && 
	   ((top(code) && nobrick(TOP, brk)) || (bottom(code) &&
						 nobrick(BOTTOM, brk)))) ||
	  (right(code) && !betw(max, ymin+1, ymax-1) && 
	   ((top(code) && nobrick(TOP, brk)) || (bottom(code) &&
						 nobrick(BOTTOM, brk))))) {
	*y = prevy; *m = -(*m);
      }
      else if ((left(code) && betw(min, ymin, ymax) && nobrick(LEFT, brk)) ||
	       (right(code) && betw(max, ymin, ymax) &&	nobrick(RIGHT, brk))) {
/*      else if ((left(code) && betw((int)(slope*xmin), ymin, ymax) && nobrick(TOP{*/
	*x = prevx; *n = -(*n);
      }
      else if ((top(code) && nobrick(TOP, brk)) ||
	       (bottom(code) && nobrick(BOTTOM, brk))) {
	*y = prevy; *m = -(*m);
      }
      else {
	*y = prevy; *m = -(*m);
	*x = prevx; *n = -(*n);
      }
      *rev = bottom(code);
    }
  }
}
#endif  
      
main(argc, argv)
  int argc;
  char *argv[];
{
  int i, q;
  XWMHints *wmhints;
  FILE *fopen(), *fp;
  char *userScorefile, tmp[80], tmpfile[80];
  int c, startlevel = 0;
  extern char *optarg;
  Widget w;

#ifdef OLD  
  if ((the_display = XOpenDisplay("")) == NULL)
#else    
  if ((w = XtInitialize("BReakout", "breakout", NULL, 0, &argc, argv)) ==
      NULL)
#endif    
    {
      fprintf(stderr,"in %s: can't open display\n", argv[0]);
      return(-1);
    }

#ifndef OLD
  the_display = XtDisplay(w);
#endif  

  the_screen = DefaultScreen(the_display);

  root_window = DefaultRootWindow(the_display);

  while ((c = getopt(argc, argv, "l:m")) != -1)
    switch (c) {
    case 'm':
      mono = 1;
      break;
    case 'l':
      startlevel = atoi(optarg) - 1;
      if (startlevel < 1) {
	fprintf(stderr, "%s: Level must be 1 or higher\n", argv[0]);
	fprintf(stderr, "Usage: %s [-m] [-l level]\n", argv[0]);
	exit(1);
      }
      break;
    case '?':
      fprintf(stderr, "Usage: %s [-m] [-l level]\n", argv[0]);
      exit(1);
      break;
    }
  
  /* initalize scores */
  userScorefile = NULL;
  
  for (i=0; i<5; i++) {
    scr[i]=0;
    hiscr[i].lvl=1;
  }

  strcpy(hiscr[0].name, "sCO");
  strcpy(hiscr[1].name, "TTh");
  strcpy(hiscr[2].name, "nOV");
  strcpy(hiscr[3].name, "ACK");
  strcpy(hiscr[4].name, "usl");

  /* get score file, or assign default in $HOME.  Create it if necessary. */ 

  userScorefile = XGetDefault(the_display, "breakout", "scoreFile");
  
  if (userScorefile == NULL) {
    sprintf(tmpfile, "%s/brkout.scr", getenv("HOME"));
    userScorefile = tmpfile;
  }
  
  if ((fp = fopen(userScorefile, "r")) == NULL) {
    fclose(fp);
    if ((fp = fopen(userScorefile, "w")) == NULL) {
      fprintf(stderr,"breakout: Cannot create score file %s.\n",userScorefile);
      exit(1);
    }
    sprintf(tmp, "chmod +w %s 2> /dev/null", userScorefile);

    system(tmp);
    for (i = 0; i < 5; i++)
      fprintf(fp, "%s %d %d\n", hiscr[i].name, hiscr[i].scr, hiscr[i].lvl);
  }
  else 
    for (i=0; i<5; i++)
      fscanf(fp, "%s %d %d", hiscr[i].name, &(hiscr[i].scr), &(hiscr[i].lvl));

  fclose(fp);

  size_hints.x = START_X;
  size_hints.y = START_Y;
  size_hints.width = WINDOW_WIDTH;
  size_hints.height = WINDOW_HEIGHT;
  size_hints.flags = PSize|PPosition;

  /* set original background color */
/*  BGColor = t_get_pixel(the_display, "black");*/
  init_colors(the_display);
  BGColor = backg[startlevel%16];
  
  the_window = XCreateSimpleWindow(the_display, root_window, 
				   size_hints.x, size_hints.y, size_hints.width,
				   size_hints.height, BORDER_WIDTH,
				   WhitePixel(the_display, the_screen),
				   BlackPixel(the_display, the_screen));

  /* various lines to put various things in the title and icon */
  
/*0	XSetStandardProperties(the_display, the_window, "BREAKOUT!", "1234567890123456789012345678901234567890",*/
/*1	XSetStandardProperties(the_display, the_window, NULL, NULL,*/
        XSetStandardProperties(the_display, the_window, "Breakout","BREAKOUT!",
/*3	XSetStandardProperties(the_display, the_window, "BREAKOUT!", NULL,*/
/*4				NULL, argv, argc, &size_hints);*/
				t_iconic(the_display), argv, argc, &size_hints);

  XMapWindow(the_display, the_window);

  /* GAME OVER window */
  final = XCreateSimpleWindow(the_display, the_window, 100,150,200,40,0,
			    WhitePixel(the_display,the_screen),
			    BlackPixel(the_display, the_screen));

  /* score window */
  score = XCreateSimpleWindow(the_display, the_window, 25, 0, 75, 22, 0,
			    WhitePixel(the_display,the_screen),
			    BlackPixel(the_display, the_screen));

  /* high scores */
  hiscore = XCreateSimpleWindow(the_display, the_window, 45, 100, 310, 150, 0,
			    WhitePixel(the_display,the_screen),
			    BlackPixel(the_display, the_screen));

  XMapSubwindows(the_display, the_window);
  XUnmapWindow(the_display, final);
  XUnmapWindow(the_display, hiscore);

  inputmask = ExposureMask|ButtonPressMask;
  XSelectInput(the_display, the_window, inputmask);

  if ((fs = XLoadQueryFont(the_display, FONT)) == NULL) {
    fprintf(stderr, "%s: display %s doesn't know font %s\n",
	    argv[0], DisplayString(the_display), FONT);
    fs = XLoadQueryFont(the_display, "fixed");
  }

  CreateGCs();

  btn_ev = (XButtonEvent *)&the_event;

  reset(startlevel);

  /* play forever */

  while (1) {
      while (ballcount > 0 && brickcount > 0) {
	  
	  /* ready the ball for motion */
	  
	  torpedo(the_display, the_window, the_GC, 100, 85);
	  
	  /* ball ready for play and not moving */
	  
	  bzzt = 0; go = demo;
	  
	  /* Play this ball */
	  
	  while (bzzt == 0 && brickcount > 0) {
	      if (go || demo)
		move_ball(the_display, the_window, the_GC, startlevel);
	      if (XPending(the_display)) {
		  XNextEvent(the_display, &the_event);
/*		  if (the_event.type == ButtonPress)*/
		    event_handler(the_display, the_window, &the_event);
/*		  else if (!the_event.count)*/
/*		    RedrawPlayfield(the_display, the_window, 1);*/
		}
	    }
	  
	  /* ball missed.  Get next ball ready */
	  ballcount--;
	  if (ballcount > 0) {
	      posx = 1;
	      posy = 150;
	      DrawBall(the_display, the_window, the_GC, posx, posy);
	      offset = 0;
	    }
	  if (demo && !go) {
	      XClearWindow(the_display, the_window);
	      XClearWindow(the_display, score);
	      reset(startlevel);
	      demo = 0;
	    }
	}				/* until all balls gone */
      
      /* GAME OVER.  See if high score */
      
      if (points > hiscr[4].scr && !demo)
    	new_high(hiscr, points);
      
      
      /* Show high scores. */
      
      if (!demo) 
	{
	  XMapWindow(the_display, hiscore);
	  XGetWindowAttributes(the_display, hiscore, &xwa);
	  
	  if ((fp = fopen(userScorefile, "w")) == NULL) {
	      fprintf(stderr,"can't write scorefile\n");
	    }
	  else 
	    for (i = 0; i < 5; i++) {
		sprintf(shn, "%d. %5d %8s (%d)", i+1, hiscr[i].scr, hiscr[i].name,
			hiscr[i].lvl);
		fprintf(fp, "%s %d %d\n", hiscr[i].name, hiscr[i].scr,
			hiscr[i].lvl); 
		hix = (xwa.width - XTextWidth(fs, shn, strlen(shn)))/2;
		XDrawString(the_display, hiscore, the_GC, 35,25*(i+1), shn,
			    strlen(shn)); 
	      }
	  fclose(fp);
	  
	  go = 0;  demo = 0;
	  while (!go)
	    if (XPending(the_display)) {
		XNextEvent(the_display, &the_event);
		event_handler(the_display, the_window, &the_event);
	      }
	  
	  /* Say Game Over.  exit on next button press */
	  
	  XUnmapWindow(the_display, hiscore);
	  XMapWindow(the_display, final);
	  XGetWindowAttributes(the_display, final, &xwa);
	  finx = (xwa.width - XTextWidth(fs, OVER, strlen(OVER)))/2;
	  XDrawString(the_display, final, the_GC, finx,20, OVER, strlen(OVER));
	  
	  go = 0;
	  while (!go)
	    if (XPending(the_display)) {
		XNextEvent(the_display, &the_event);
		event_handler(the_display, the_window, &the_event);
	      }
	}
      XClearWindow(the_display, the_window);
      XClearWindow(the_display, score);
      XClearWindow(the_display, final);
      XUnmapWindow(the_display, final);
      demo = 1;
      reset(startlevel);
    }
}

/*RedrawNeighbors(d, w, n)
  Display *d;
  Window w;
  int n;
{
  int i;
  int noright = 0, nobottom = 0, noleft = 0, notop = 0;
  GC tmp_GC1, tmp_GC2;

  if (n/10 == 0) { tmp_GC1 = red_GC; tmp_GC2 = blue_GC;}
  else if (n/10 == 1) { tmp_GC1 = blue_GC; tmp_GC2 = green_GC; }
  else tmp_GC1 = tmp_GC2 = green_GC;

  if (n%10 == 9) noright = 1;
  else if (n%10 == 0) noleft = 1;
  if (n/10 == 2) nobottom = 1;
  else if (n/10 == 0) notop = 1;

  if (!noright && (gone[n+1] != 1)) 
    DrawBrick(d, w, (mono) ? the_GC : tmp_GC1, (n+1)%10, n/10, 0);
  if (!nobottom && (gone[n+10] != 1)) 
    DrawBrick(d, w, (mono) ? the_GC : tmp_GC2, n%10, n/10+1, 0);
  if (!nobottom && !noright && (gone[n+11] != 1)) 
    DrawBrick(d, w, (mono) ? the_GC : tmp_GC2, (n+1)%10, n/10+1, 0);
}*/
