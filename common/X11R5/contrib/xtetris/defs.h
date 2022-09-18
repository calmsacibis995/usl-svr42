/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:defs.h	1.1"
#endif

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#define UWIDTH          10      /* canvas size in units */
#ifdef i386
#define UHEIGHT         21
#else
#define UHEIGHT         30
#endif

#define HIGH_TABLE_SIZE 10      /* size of high score table */
#ifndef HIGH_SCORE_TABLE
#define HIGH_SCORE_TABLE	"/usr/games/lib/tetris_scores"
#endif

static Arg args[20];

Boolean running;

Widget	 toplevel;
Widget   frame, score_frame;
Widget   panel, score_panel;
Widget   canvas, shadow, nextobject, start_bt, pause_bt, newgame_bt;

Widget   high_score_item[HIGH_TABLE_SIZE+1], score_item, level_item, rows_item, game_over;
GC       gc, erasegc, cleargc;
Pixmap	 tetris_icon;

int	end_of_game, score_position;
int     shape_no, xpos, ypos, rot, score, rows;
int	next_no, next_rot;
char   *name;

struct resource_struct
{
  Pixel foreground;
  Pixel background;
  Boolean usescorefile;
  Pixmap erasestipple;
  Dimension boxsize;
  Dimension speed;
  char *scorefile;
}
resources;

struct score_table {
        char    name[BUFSIZ];
        int     score;
        int     rows;
        int     level;
		char hostname[BUFSIZ];
        char    date[BUFSIZ];
}       high_scores[HIGH_TABLE_SIZE];

typedef struct
{
  unsigned long unitson;  /* an array of 4x4 = 16 bits, indicating the on 
			     units in this order:

    X11 coordinates	     <0,0> <1,0> <2,0> <3,0>   <0,1> <1,1> <2,1> <3,1>  <0,2> <1,2> <2,2> <3,2> ... <3,3>*/


  int points;             /* Points for acceptance in this position. */
  XRectangle urect[2];    /* Rectangles to draw in unit form */
  XRectangle rect[2];     /* Rectangles to draw in pixel form */
  short nrect;

  short shadowx;
  unsigned short shadowwidth;
  
  short highesty[4];      /* highest non-zero y in unitson, for each x */
  short highestx[4];      /* highest non-zero x in unitson, for each y */
  short lowestx[4];       /* lowest non-zero y in unitson, for each y */
}
rotshape_type, *rotshape_ptr;

typedef struct shape_table {
  rotshape_type forms[4];
  Pixel   foreground;
  Pixel   background;
  Pixmap  stipple;
  GC      gc;
} shape_type, *shape_ptr;

shape_ptr grid[UWIDTH][UHEIGHT];

extern shape_type shape[7];

void    print_high_scores(), done_proc(), quit_proc(), start_proc(), pause_proc(), restart_proc();
void    drop_block(), restore_canvas(), show_score(), end_game(), left_proc(), right_proc();
void    clock_proc(), anti_proc(), fast_proc();

  XtAppContext context;
