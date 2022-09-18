/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:xaw_stubs.c	1.2"

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

/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)xaw_stubs.c	2.2	90/04/27
 *
 */

/*
 * Athena Widget interface to Spider
 */
#include	"defs.h"
#include	"globals.h"
#include	"xaw_ui.h"

Bool	show_play_events();

static	Widget	help = (Widget) NULL;
char	helpfiles[6][256];
static int	confirmer_state;
Bool		write_confirmer();
Bool		confirm;

/*
 *  CHANGE # UNKNOWN
 *  FILE # xaw_stubs.c
 *  All functions to be used with XtAddCallback() and XtAddEventHandler() are
 *  void .
 *  ENDCHANGE # UNKNOWN
 */

/* ARGSUSED */
void
score_handler(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
char	buf[128];

	(void) sprintf(buf, "Current position scores %d out of 1000.",
		compute_score());
	show_message(buf);
}

/* ARGSUSED */
void
newgame_handler(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
	if (newgame_confirmer())	{
		clear_message();
		shuffle_cards();
	}
}

/* ARGSUSED */
void
backup_handler(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
	switch ((int) call_data)	{
		case	0:
			undo();
			break;
		case	1:
			(void) replay();
			init_cache();	/* reset move cache */
			break;
		case	2:
			/* show all events */
			show_play(0, 0, show_play_events, delay);
			break;
	}
}

/* ARGSUSED */
void
expand_handler(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
	do_expand();
}

/* ARGSUSED */
void
help_handler(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
	if (help == (Widget) NULL)	{
		help = create_help_popup(helpfiles[0]);
	}
	XtPopup(help, XtGrabNone);
}

/* ARGSUSED */
void
change_help(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
Arg	args[1];
char	*fname;

	switch ((int) call_data)	{
		case	1:
		case	2:
		case	3:
		case	4:
		case	5:
		case	6:
			fname = helpfiles[(int)call_data - 1];
			break;
		case	7:
			XtPopdown(help);
			return;
		default:
			assert(0);
			break;
	}

	/* have to duplicate the name cause the widget clobbers it */
	XtSetArg(args[0], XtNstring, strdup(fname));
	XtSetValues(helptext, args, ONE);
}

/* ARGSUSED */
void
locate_handler(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
Arg	args[1];
char	*fname = NULL;

	XtSetArg(args[0], XtNstring, &fname);
	XtGetValues(file, args, ONE);

	/* nothing in the field doesn't return a NIL pointer */
	if (strlen(fname))	{
		/* remove the leading whitespace */
		fname = remove_newlines(fname);
		locate(fname);
	}
}

/* ARGSUSED */
void
file_handler(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
Arg	args[1];
char	*fname;

	XtSetArg(args[0], XtNstring, &fname);
	XtGetValues(file, args, ONE);
	switch ((int) call_data)	{
		case	0:
			if (fname)	{
				write_file(fname, write_confirmer);
			} else	{
				show_message("Bogus filename.");
			}
			return;
		case	1:
			if (fname)	{
				read_file(fname);
			} else	{
				show_message("Bogus filename.");
			}
			break;
		case	2:
			fname = get_selection();
			if (fname)	{
				read_selection(fname);
			} else	{
				show_message("Bogus primary selection.");
			}
			break;
	}
	force_redraw();
}


/* event handlers */

/* ARGSUSED */
void
xaw_redraw_table(w, data, xev, ctd)
Widget	w;
XtPointer	data;
XEvent	*xev;
Boolean	*ctd;
{
	assert (XtWindow(w) == table);

	redraw_table(xev);
	*ctd = False;
}

/* ARGSUSED */
void
xaw_button_press(w, data, xev, ctd)
Widget	w;
XtPointer	data;
XEvent	*xev;
Boolean	*ctd;
{
	assert (XtWindow(w) == table);

	button_press(xev);
	*ctd = False;
}

/* ARGSUSED */
void
xaw_button_release(w, data, xev, ctd)
Widget	w;
XtPointer	data;
XEvent	*xev;
Boolean	*ctd;
{
	assert (XtWindow(w) == table);

	button_release(xev);
	*ctd = False;
}

/* ARGSUSED */
void
xaw_resize(w, data, xev, ctd)
Widget	w;
XtPointer	data;
XEvent	*xev;
Boolean	*ctd;
{
int	i;

	assert (XtWindow(w) == table);

	if (xev->type != ConfigureNotify)
		return;

	table_height = ((XConfigureEvent *)xev)->height;
	table_width = ((XConfigureEvent *)xev)->width;
	for (i = 0; i < NUM_STACKS; i++)	{
		if (stack[i])
			recompute_list_deltas(stack[i]);
	}
	*ctd = False;
}

/* ARGSUSED */
void
xaw_key_press(w, data, xev, ctd)
Widget	w;
XtPointer	data;
XEvent	*xev;
Boolean	*ctd;
{
	assert (XtWindow(w) == table);

	key_press(xev);

	if (restart)	{
		shuffle_cards();
	}

	*ctd = False;
}

Bool
show_play_events()
{
XEvent	xev;

	while (XPending(XtDisplay(toplevel)))	{
		XNextEvent(XtDisplay(toplevel), &xev);

		/* any key or button will stop it */
		switch(xev.type)	{
			default:
				XtDispatchEvent(&xev);
				break;

			case	KeyPress:
			case	KeyRelease:
			case	ButtonPress:
			case	ButtonRelease:
				return False;
		}
	}
	return True;
}

void
confirm_callback(w, call_data, client_data)
Widget	w;
XtPointer	call_data, client_data;
{
	if ((int)call_data == 1)
		confirmer_state = True;
	else
		confirmer_state = False;

	XtPopdown(confirm_box);
}

static Bool
do_confirmer(label)
String	label;
{
Arg	args[2];
XEvent	event;
Dimension	height, width;
Position	x, y;
	
	/* set the location */
	XtSetArg(args[0], XtNwidth, &width);
	XtSetArg(args[1], XtNheight, &height);
	XtGetValues(toplevel, args, TWO);
	XtTranslateCoords(toplevel, (Position)(width/2), 
		(Position)(height/2), &x, &y);
	XtSetArg(args[0], XtNx, x);
	XtSetArg(args[1], XtNy, y);
	XtSetValues(confirm_box, args, TWO);

	/* set the label */
	XtSetArg(args[0], XtNlabel, label);
	XtSetValues(confirm_label, args, ONE);
	confirmer_state = -1;
	XtPopup(confirm_box, XtGrabExclusive);

	/* await the confirmation/denial */
	while (confirmer_state == -1)	{
		XNextEvent(XtDisplay(toplevel), &event);
		XtDispatchEvent(&event);
	}

	return (confirmer_state);
}

Bool
write_confirmer()
{
	if (confirm)
		return (do_confirmer("File already exists -- overwrite it?"));
	else
		return True;
}

Bool
newgame_confirmer()
{
	if (confirm)
		return(do_confirmer("Really discard current game?"));
	else
		return True;
}
