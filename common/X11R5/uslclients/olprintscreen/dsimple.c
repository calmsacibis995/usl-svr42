/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:dsimple.c	1.7"
#endif

/*
 * Based on code written by Mark Lillibridge
 */

#include "main.h"
#include "externs.h"

/*
 * Standard fatal error routine - call like printf but maximum of 7 arguments.
 * Does not require dpy or screen defined.
 */
void Fatal_Error(msg, arg0,arg1,arg2,arg3,arg4,arg5,arg6)
char *msg;
char *arg0, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, msg, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	fprintf(stderr, "\n");
	exit(1);
}


/*
 * Routine to let user select a window using the mouse
 */

Window Select_Window(dpy)
Display *dpy;
{
	XEvent event;
	Window target_win = None;
	/*  Pixmap pix1, pix2;*/
	int f_pressed = False;
	int f_released = False;

	int last_root_x, last_root_y;

	OlVirtualEventRec	ve;

	/* Grab the pointer using target cursor, letting it room all over */
	while (XGrabPointer(dpy, RootWindow(dpy, screen), False,
	    ButtonPressMask|ButtonReleaseMask, GrabModeAsync,
	    GrabModeAsync, None, sw_cursor, CurrentTime)
	    != GrabSuccess);

	/* Grab the Keyboard to allow for mouseless operation */
	if(XGrabKeyboard(dpy, RootWindow(dpy, screen), False, GrabModeAsync,
		GrabModeAsync, CurrentTime) != GrabSuccess) {
		fprintf(stderr, "Cannot capture Keyboard\n");
	}

	/* Let the user select a window 
	(Select ButtonPress and Select ButtonRelease)... */
	while (f_released == False) {

		if (XCheckMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|KeyPressMask,
				    &event) == True)
		{
				/* get the virtual binding */
			OlLookupInputEvent(toplevel, &event, &ve,
					   OL_DEFAULT_IE);
			switch (event.type) {
			case ButtonPress:
				/* make sure it's select button
                           and not pressed already */
				if (ve.virtual_name == OL_SELECT
				    && (f_pressed == False)) {
					f_pressed = True;
				}
				break;
			case ButtonRelease:
				/* make sure it's select button
                           and pressed already */
				if (ve.virtual_name == OL_SELECT
				    && (f_pressed == True)) {
					/* window selected */
					target_win = event.xbutton.subwindow;
					if (target_win == None)
						target_win = rootwindow;
					f_released = True;
				}
				break;

			case KeyPress:
				if (ve.virtual_name == OL_SELECTKEY) {
					/* window selected */
					target_win = event.xbutton.subwindow;
					if (target_win == None)
						target_win = rootwindow;
					f_released = True;
				}
				if (!(ve.virtual_name == OL_SELECT)) {
					last_root_x = event.xbutton.x_root;
					last_root_y = event.xbutton.y_root;
					HandleCursor(ve.virtual_name, 
						&last_root_x, &last_root_y);
					XWarpPointer(dpy, None, 
							RootWindow(dpy, screen),
							0, 0, (unsigned int) 0,
							(unsigned int) 0,
							last_root_x, last_root_y);
				}
			}
		}
	}

	XUngrabPointer(dpy, CurrentTime);	 /* Done with pointer */
	XUngrabKeyboard(dpy, CurrentTime);	 /* Done with keyboard */
	return(target_win);
}

