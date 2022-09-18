/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:init.c	1.1"
#endif

#include "defs.h"
#include "icon"
#include <X11/Xos.h>
#include <pwd.h>

initialise()
{
        struct passwd *who;
        char   *getenv();
	int i;
	Arg args[20];

        srandom((int) time((time_t *) 0));
        define_shapes();
        if ((name = getenv("TETRIS")) == NULL) {
                who = getpwuid(getuid());
                name = who->pw_name;
        }

        init_all();
        read_high_scores();

	/* Make the icon. */

        tetris_icon = XCreateBitmapFromData(XtDisplay(frame),
                                            XtWindow(frame),
                                      icon_bits, icon_width, icon_height);
        i=0;
        XtSetArg(args[i], XtNiconPixmap, tetris_icon); i++; 
        XtSetValues(toplevel, args, i);
}

init_all()
{
        int     i, j;
        Arg args[20];

	score_position = -1;
        end_of_game = 0;
        rows = score = shape_no = rot = xpos = ypos = 0;

        for (i = 0; i < UWIDTH; i++)
	  for (j = 0; j < UHEIGHT; j++)
	    grid[i][j] = NULL;

        create_shape();         /* Set up 1st shape */
        create_shape();         /* Set up next shape */
        XClearArea(XtDisplay(canvas), XtWindow(canvas), 0, 0, 0, 0, False );
	i=0;
	XtSetArg(args[i], XtNlabel, "         "); i++;
	XtSetValues(game_over, args, i);
        show_score();
        show_next();
        draw_shadow(shape_no, xpos, rot );
}
