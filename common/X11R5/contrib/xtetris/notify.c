/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:notify.c	1.2"
#endif

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

#include "defs.h"


void show_score(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
        char    buf[BUFSIZ], buf1[BUFSIZ], buf2[BUFSIZ];
	int j;
	Arg args[20];

        sprintf(buf, "Score: %d", score);
        j=0;
        XtSetArg(args[j], XtNlabel, buf); j++;
        XtSetValues(score_item,args,j);
        sprintf(buf1, "Level: %d", rows / 10);
        j=0;
        XtSetArg(args[j], XtNlabel, buf1); j++;
        XtSetValues(level_item,args,j);
/*
 *	CHANGE # UNKNOWN
 *	FILE # notify.c
 *  Set the label of the Rows widget properly ( only one gap).
 *	ENDCHANGE # UNKNOWN
 */
        sprintf(buf2, "Rows: %d", rows);
        j=0;
        XtSetArg(args[j], XtNlabel, buf2); j++;
        XtSetValues(rows_item,args,j);
}

void quit_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
        clear_events();
        stop_timer();
	XtDestroyWidget(toplevel);
	exit(0);
}

void end_game(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
	int j;
	Arg args[20];

        end_of_game = 1;
        clear_events();
        stop_timer();
	XtUnmapWidget( start_bt );
	XtUnmapWidget( pause_bt );
        j=0;
        XtSetArg(args[j], XtNlabel, "Game Over"); j++;
        XtSetValues(game_over,args,j);
        update_highscore_table();
	print_high_scores();
}

void restart_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
        clear_events();
        stop_timer();
        init_all();
}

void start_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
  if (running || end_of_game) return;
  set_events();
  start_timer();
}

void pause_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
  if (!running) return;
  clear_events();
  stop_timer();
}

void drop_block(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
  start_timer();
  if (block_can_drop(shape_no, xpos, ypos, rot))
    print_shape( canvas, shape_no, xpos, ypos++, rot, True );
  else {
    if (ypos < 0)
      end_game();
    else {
      score += shape[shape_no].forms[rot].points;
      store_shape(shape_no, xpos, ypos, rot);
      remove_full_lines(ypos);
      create_shape();
      show_score();
      show_next();
      draw_shadow( shape_no, xpos, rot );
    }
  }
  print_shape( canvas, shape_no, xpos, ypos, rot, False );
}

void left_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
  if (!running) return;
  
  if (block_can_left(shape_no, xpos, ypos, rot)) {
    print_shape( canvas, shape_no, xpos, ypos, rot, True );
    xpos--;
    print_shape( canvas, shape_no, xpos, ypos, rot, False );
    draw_shadow(shape_no, xpos, rot );
  }
}

void right_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
  if (!running) return;
  
        if (block_can_right(shape_no, xpos, ypos, rot)) {
                print_shape( canvas, shape_no, xpos, ypos, rot, True );
                xpos++;
                print_shape( canvas, shape_no, xpos, ypos, rot, False );
                draw_shadow(shape_no, xpos, rot );
        }
}

void anti_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
        int     newrot;

  if (!running) return;
  
        newrot = (rot + 3) % 4;
        if (check_rot(shape_no, xpos, ypos, newrot)) {
                print_shape( canvas, shape_no, xpos, ypos, rot, True );
                rot = newrot;
                print_shape( canvas, shape_no, xpos, ypos, rot, False );
                draw_shadow(shape_no, xpos, rot );
        }
}

void clock_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
        int     newrot;

  if (!running) return;
  
        newrot = (rot + 1) % 4;
        if (check_rot(shape_no, xpos, ypos, newrot)) {
                print_shape( canvas, shape_no, xpos, ypos, rot, True );
                rot = newrot;
                print_shape( canvas, shape_no, xpos, ypos, rot, False );
                draw_shadow(shape_no, xpos, rot );
        }
}

void fast_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
  if (!running) return;
  
        while (block_can_drop(shape_no, xpos, ypos, rot)) {
                print_shape( canvas, shape_no, xpos, ypos, rot, True );
                ypos++;
                print_shape( canvas, shape_no, xpos, ypos, rot, False );
        }
}

void done_proc(w, event, pars, npars)
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
	XtPopdown(score_frame);
}
