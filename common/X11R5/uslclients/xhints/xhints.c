/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xhints:xhints.c	1.3"
#include <stdio.h>
#include <Xlib.h>
#include <Xutil.h>
#include <Xatom.h>
#include <StringDefs.h>
#include <cursorfont.h>
#include <Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>

#define ARGLISTSIZE	50
#define INIT_ARGS	SetArgCount = 0;
#define SET_ARGS(R, A)	XtSetArg(SetArgList[SetArgCount], R, A); ++SetArgCount;
#define SET_VALUES(W)	XtSetValues(W, SetArgList, SetArgCount);
#define GET_VALUES(W)	XtGetValues(W, SetArgList, SetArgCount);

#define CREATE_MANAGED(N, C, P) \
			XtCreateManagedWidget(N,C,P,SetArgList,SetArgCount);

/*
	globals
*/
static Window		root;
static Display		*dpy;
static GC		gc;
static Cursor		arrow;

static char		*selectprompt = "Select Target Window";

static Bool		input = True;			/* passive model */
static Bool		take_focus = False;		/* by default */


Window	get_window();
void	set_current();
void	display_info();
void	display_protocols();
void	display_WMHints();
void	display_sizehints();
void	change_current();
void	do_window();
void	do_parent();
void	do_quit();
void	AddButtons();

void	GetPointer();
Window	GetWindow();
void	FlashWindow();


typedef struct Command {
	char	*name;
	void	(*func)();
} Command;

typedef struct EventWindow {
	Window	window;
	char	*name;
} EventWindow;


static Command commands[] =
{
	{ "Window",	do_window   },
	{ "Parent",	do_parent   },
	{ "Quit",	do_quit     },
	{ (char *)0,	(void(*)())0 },
};


static EventWindow current =
{
	(Window) 0,
	(char *) 0,
};


static Arg	SetArgList[ARGLISTSIZE];
static int	SetArgCount;

Widget	Toplevel;


void
main (argc, argv)
int argc;
char **argv;
{
	Widget		control_area;
	XGCValues	gcv;
	Atom		stuff;

	Toplevel = OlInitialize("X Hints", "X Hints", NULL, NULL, &argc, argv);
	_OlSetApplicationTitle("X Hints");

	dpy = XtDisplay(Toplevel);
	root = RootWindowOfScreen(XtScreen(Toplevel));
	arrow = XCreateFontCursor(dpy, XC_arrow);

	gcv.function = GXequiv;
	gcv.foreground = (unsigned long) 0;
	gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);

	set_current(get_window(selectprompt));
	display_info();

	INIT_ARGS;
	control_area = CREATE_MANAGED("control_area", controlAreaWidgetClass,
								Toplevel);
	AddButtons(control_area);
	XtRealizeWidget(Toplevel);

	stuff = XA_OL_DECOR_RESIZE(dpy);
	XChangeProperty(dpy, XtWindow(Toplevel), XA_OL_DECOR_DEL(dpy),
			XA_ATOM, 32,
			PropModeReplace, (unsigned char *) &stuff, 1);

	XtMainLoop();
}


void
AddButtons(parent)
Widget parent;
{
	Widget	button;
	int	i;

	for (i = 0; commands[i].name != (char *) 0; ++i)
	{
		INIT_ARGS;
		SET_ARGS(XtNlabelType, OL_STRING);
		SET_ARGS(XtNlabel, commands[i].name);
		SET_ARGS(XtNrecomputeSize, True);

		button = CREATE_MANAGED(commands[i].name,
					oblongButtonWidgetClass, parent);
		XtAddCallback(button, XtNselect, commands[i].func, (caddr_t) 0);
	}
}


Window
get_window(tag)
char *tag;
{
	printf ("%s\n", tag);
	return GetWindow(arrow);
}


void
set_current(w)
Window w;
{
	current.window = w;
	FlashWindow(w);
	XFetchName(dpy, w, &current.name);
}


void
display_info()
{
	display_WMHints();
	display_protocols();
	display_sizehints();
}


void
display_WMHints()
{
	XWMHints	*xwmh;
	char		buf[100],
			fields[500],
			tmp[50];

	if (current.name)
		printf ("\nWindow = 0x%lx (%s)\n", current.window,current.name);
	else
		printf ("\nWindow = 0x%lx\n", current.window);

	printf("XWMHints:  ");
	fields[0] = (char) 0;
	if ((xwmh = XGetWMHints(dpy, current.window)) != (XWMHints *) 0)
	{
		strcpy(buf, "\n\tflags =   ");

		if (xwmh->flags & InputHint)
		{
			strcat(buf, "InputHint | ");
			input = xwmh->input;
			sprintf(tmp, "\tinput = %s\n",
				(input == True) ? "True" : "False");
			strcat(fields, tmp);
		}

		if (xwmh->flags & StateHint)
		{
			strcat(buf, "StateHint | ");
			strcat(fields, "\tinitial_state = ");
			if (xwmh->initial_state == WithdrawnState)
				strcat(fields, "WithdrawnState\n");
			if (xwmh->initial_state == NormalState)
				strcat(fields, "NormalState\n");
			if (xwmh->initial_state == ZoomState)
				strcat(fields, "ZoomState\n");
			if (xwmh->initial_state == IconicState)
				strcat(fields, "IconicState\n");
			if (xwmh->initial_state == InactiveState)
				strcat(fields, "InactiveState\n");
		}

		if (xwmh->flags & IconPixmapHint)
		{
			strcat(buf, "IconPixmapHint | ");
		}

		if (xwmh->flags & IconWindowHint)
		{
			strcat(buf, "IconWindowHint | ");
			strcat(fields, "\ticon window id = ");
			sprintf(tmp, "0x%x\n", xwmh->icon_window);
			strcat(fields, tmp);
		}

		if (xwmh->flags & IconPositionHint)
		{
			strcat(buf, "IconPositionHint | ");
			strcat(fields, "\ticon x = ");
			sprintf(tmp, "%d\n", xwmh->icon_x);
			strcat(fields, tmp);
			strcat(fields, "\ticon y = ");
			sprintf(tmp, "%d\n", xwmh->icon_y);
			strcat(fields, tmp);
		}

		if (xwmh->flags & IconMaskHint)
		{
			strcat(buf, "IconMaskHint | ");
		}

		if (xwmh->flags & WindowGroupHint)
		{
			strcat(buf, "WindowGroupHint | ");
			strcat(fields, "\twindow group id = ");
			sprintf(tmp, "0x%x\n", xwmh->window_group);
			strcat(fields, tmp);
		}

#ifndef SVR4	/* One really shouldn't check the existence of a variable */
		if (buf)
#endif /* SVR4 */
			buf[strlen(buf) - 3] = (char) 0;

		printf("%s\n", buf);
		printf("%s", fields);
		XFree((char *) xwmh);
	}
	else
		printf("\n\tNone\n");
}


void
display_protocols()
{
	Atom	*atoms;
	int	num_atoms,
		i;
	char	buf[100];
	Window	parent,
		r,
		*c;
	int	n;

	buf[0] = (char) 0;
	printf("WM_PROTOCOLS used:\n");

	if ((atoms = GetAtomList(dpy, current.window,
			XA_WM_PROTOCOLS(dpy), &num_atoms, False)) != (Atom *) 0)
	{
		for (i = 0; i < num_atoms; ++i)
		{
			if (atoms[i] == XA_WM_TAKE_FOCUS(dpy))
			{
				take_focus = True;
				strcat(buf, "\tWM_TAKE_FOCUS\n");
			}
			else if (atoms[i] == XA_WM_SAVE_YOURSELF(dpy))
			{
				strcat(buf, "\tWM_SAVE_YOURSELF\n");
			}
			else if (atoms[i] == XA_WM_DELETE_WINDOW(dpy))
			{
				strcat(buf, "\tWM_DELETE_WINDOW\n");
			}
		}

		if (!buf)
			strcat(buf, "\tNone\n");

		XFree((char *) atoms);
	}
	else
		strcat(buf, "\tNone\n");

	printf("%s", buf);

	printf("Input Model:\n");

	if (input != True && take_focus != True)
		printf("\tNO_INPUT\n");
	else if (input == True && take_focus != True)
		printf("\tPASSIVE\n");
	else if (input == True && take_focus == True)
		printf("\tLOCALLY_ACTIVE\n");
	else if (input != True && take_focus == True)
		printf("\tGLOBALLY_ACTIVE\n");
	else
		printf("\tInconsistent Information!\n");

	input = True;			/* back to default */
	take_focus = False;
}


void
display_sizehints()
{
	XSizeHints	xsh;
	char		buf[100],
			fields[500],
			tmp[50];

	printf("XSizeHints: ");
	fields[0] = (char) 0;
	if (XGetSizeHints(dpy, current.window, &xsh, XA_WM_NORMAL_HINTS) !=  0)
	{
		strcpy(buf, "\n\tflags =   ");

		if (xsh.flags & USPosition || xsh.flags & PPosition)
		{
			if (xsh.flags & USPosition)
				strcat(buf, "USPosition | ");
			if (xsh.flags & PPosition)
				strcat(buf, "PPosition | ");

			sprintf(tmp, "\tx = %d\n\ty = %d\n", xsh.x, xsh.y);
			strcat(fields, tmp);
		}

		if (xsh.flags & USSize || xsh.flags & PSize)
		{
			if (xsh.flags & USSize)
				strcat(buf, "USSize | ");
			if (xsh.flags & PSize)
				strcat(buf, "PSize | ");

			sprintf(tmp, "\twidth = %d\n\theight = %d\n",
						xsh.width, xsh.height);
			strcat(fields, tmp);
		}

		if (xsh.flags & PMinSize)
		{
			strcat(buf, "PMinSize | ");
			sprintf(tmp, "\tmin width = %d\n\tmin height = %d\n",
					xsh.min_width, xsh.min_height);
			strcat(fields, tmp);
		}

		if (xsh.flags & PMaxSize)
		{
			strcat(buf, "PMaxSize | ");
			sprintf(tmp, "\tmax width = %d\n\tmax height = %d\n",
					xsh.max_width, xsh.max_height);
			strcat(fields, tmp);
		}

		if (xsh.flags & PResizeInc)
		{
			strcat(buf, "PResizeInc | ");
			sprintf(tmp,
				"\twidth increment = %d\n\theight increment = %d\n",
				xsh.width_inc, xsh.height_inc);
			strcat(fields, tmp);
		}

		if (xsh.flags & PAspect)
		{
			strcat(buf, "PAspect | ");
			sprintf(tmp, "\tmin aspect numerator = %d\n\tmin aspect denominator = %d\n\tmax aspect numerator = %d\n\tmax aspect denominator = %d\n",
				xsh.min_aspect.x, xsh.min_aspect.y,
				xsh.max_aspect.x, xsh.max_aspect.y);
			strcat(fields, tmp);
		}

#ifndef SVR4	/* One really shouldn't check the existence of a variable */
		if (buf)
#endif
			buf[strlen(buf) - 3] = (char) 0;
#ifndef SVR4	/* One really shouldn't check the existence of a variable */
		if (fields)
#endif
			strcat(fields, "\n");

		printf("%s\n", buf);
		printf("%s", fields);
	}
	else
		printf("\n\tNone\n");
}


void
change_current(w)
Window w;
{
	if (current.name)
		XFree((char *) current.name);

	set_current(w);
	display_info();
}


void
do_window(wid, data, ev)
Widget wid;
caddr_t data;
XEvent *ev;
{
	change_current(get_window(selectprompt));
}


void
do_parent(wid, data, ev)
Widget wid;
caddr_t data;
XEvent *ev;
{
	Window		parent,
			r,
			*c;
	int		n;

	if (current.window == root)
	{
		XBell (dpy, 30);
		return;
	}

	XQueryTree(dpy, current.window, &r, &parent, &c, &n);

	if (c)
		XFree((char *) c);

	change_current(parent);
}


void
do_quit(wid, data, ev)
Widget wid;
caddr_t data;
XEvent *ev;
{
	exit(0);
}


Window
GetWindow(cursor)
Cursor cursor;
{
	XEvent	event;
	Window	w = root,
		s,
		t;
	int	x,
		y,
		dx,
		dy;

	GetPointer(w, cursor, &event);
	s = event.xbutton.subwindow;

	if (s)
	{
		x = event.xbutton.x;
		y = event.xbutton.y;
		XTranslateCoordinates(dpy, w, s, x, y, &dx, &dy, &t);

		while (t)
		{
			w = s;
			s = t;
			x = dx;
			y = dy;
			XTranslateCoordinates(dpy, w, s, x, y, &dx, &dy, &t);
		}
		return s;
	}
	else
		return w;
}


void
FlashWindow(win)
Window win;
{
	XWindowAttributes	w;

	XGrabServer(dpy);
	XGetWindowAttributes(dpy, win, &w);
	XFillRectangle(dpy, win, gc, 0, 0, w.width, w.height);
	XFlush(dpy);
	sleep(1);
	XFillRectangle(dpy, win, gc, 0, 0, w.width, w.height);
	XUngrabServer(dpy);
}


void
GetPointer(win, cursor, event)
Window win;
Cursor cursor;
XEvent *event;
{
	XEvent	up;

	while (XGrabPointer(dpy, win, False,
		ButtonPressMask|ButtonReleaseMask, GrabModeAsync,
		GrabModeAsync, None, cursor, CurrentTime) != GrabSuccess)
			;
	XWindowEvent(dpy, win, ButtonPressMask, event);
	XWindowEvent(dpy, win, ButtonReleaseMask, &up);
	while (up.xbutton.time < event->xbutton.time)
		XWindowEvent(dpy, win, ButtonReleaseMask, &up);
	XUngrabPointer(dpy, CurrentTime);
}
