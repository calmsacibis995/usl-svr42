/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:window.c	1.1"
#endif

#include "defs.h"

static XtIntervalId timer;

start_timer()
{
	unsigned long interval;
        int     level;

        level = 2500 - rows * 6;
        if (level < 0)
                level = 0;

	interval = level / (int)resources.speed;
	timer = XtAppAddTimeOut( context, interval, drop_block, NULL);
}

stop_timer()
{
	if( timer )
	{
		XtRemoveTimeOut(timer);
		timer = NULL;
	}
}

set_events()
{
  running = True;
  XtUnmapWidget( start_bt );
  XtMapWidget( pause_bt );
}

clear_events()
{
  running = False;
  XtUnmapWidget( pause_bt );
  XtMapWidget( start_bt );
}

void restore_canvas(w, event, pars, npars )
  Widget w;
  XEvent *event;
  String *pars;
  Cardinal *npars;
{
  int x, y;

  if (w == canvas) 
    {
      for(x=0; x<UWIDTH; x++)
	for(y=0; y<UHEIGHT; y++)
	  if (grid[x][y] != NULL) {
	    XFillRectangle(XtDisplay(w), XtWindow(w), grid[x][y]->gc,
			   x * resources.boxsize, y * resources.boxsize, resources.boxsize, resources.boxsize);
	  }
      print_shape( canvas, shape_no, xpos, ypos, rot, False );
    }
  else if (w == shadow)
    draw_shadow(shape_no, xpos, rot );
  else if (w == nextobject)
    show_next();
  else
    fprintf( stderr, "Hmm. I got a Refresh() for an unrecognized window!\n" );
}
