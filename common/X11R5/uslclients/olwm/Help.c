/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Help.c	1.19"
#endif

/*
 ************************************************************************
 * Description:
 *	This file contains procedures used to implement the window
 * manager's help facility.
 ************************************************************************
 */

#include <sys/utsname.h>
#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/HelpP.h>
#include <Xol/Stub.h>
#include <Xol/VendorI.h>
#include <Xol/Olg.h>
#include <wm.h>
#include <WMStepP.h>
#include <Extern.h>

/*
 *************************************************************************
 *
 * Forward Procedure Declarations
 *
 **************************forward*declarations***************************
 */

static char *	cat OL_ARGS((char*, char*, char*, char*));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static HelpWidget	help_widget;

/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * cat
 ****************************procedure*header*****************************
 */
static char *
cat(s, s1, s2, s3)
	char * s;
	char * s1;
	char * s2;
	char * s3;
{
	(void)strcpy(s, s1);
	(void)strcat(s, s2);
	(void)strcat(s, s3);
	return (s);
} /* end of cat */

/*
 * CreateHelpTree
 *
 * The \fICreateHelpTree\fR procedure is used to override the builtin
 * help subsystem.  The window manager cannot use the existing help
 * subsystem since it needs to reparent the shell and will not be told
 * about the window map.
 * mlp - called from SetupWindowManager().
 */

extern void
CreateHelpTree(display)
	Display *	display;
{

	extern Widget		help_shell;

	help_shell = XtVaAppCreateShell(
		"helpShell",
		"OLwm",
		transientShellWidgetClass,
		display,
		XtNmappedWhenManaged,	(XtArgVal)False,
		XtNpushpin,		currentGUI == OL_OPENLOOK_GUI ?
					  (XtArgVal)OL_IN : (XtArgVal)OL_NONE,
		XtNwinType,		(XtArgVal)OL_WT_HELP,
		XtNwindowHeader,	(XtArgVal)True,
		XtNmenuButton,		(XtArgVal)False,
		(String)0
	);
	help_widget = (HelpWidget)XtVaCreateManagedWidget(
		"help",
		helpWidgetClass,
		help_shell,
		(String)0
	);

	XtRealizeWidget(help_shell);

	XtAddCallback(help_shell, XtNpopupCallback, WMReparent, NULL);

} /* end of CreateHelpTree */

/*
 *************************************************************************
 * SetHelpMsg - this routine is called to set the message for the help
 * widget.
 ****************************procedure*header*****************************
 */
extern void
SetHelpMsg(help_shell, event, text, title_prefix, title)
	Widget		help_shell;
	XEvent *	event;
	char *		text;
	char *		title_prefix;
	char *		title;
{
	char		s[1024];


	cat(s, title_prefix, ": ", title);
	XtVaSetValues (
		help_shell,
		XtNtitle,           (XtArgVal)s,
		(String)0
	);

	XtVaSetValues (
		help_widget->help.text_widget,
	    	XtNsourceType,      (XtArgVal)OL_DISK_SOURCE,
		XtNsource,	    (XtArgVal)text,       
	    	XtNeditType,        (XtArgVal)OL_TEXT_READ,
	    	XtNdisplayPosition, (XtArgVal)0,
	    	XtNcursorPosition,  (XtArgVal)0,
	    	XtNselectStart,     (XtArgVal)0,
	    	XtNselectEnd,       (XtArgVal)0,
		(String)0
	);

	if (currentGUI == OL_OPENLOOK_GUI)	/* for Motif, mag is empty */
		XtVaSetValues (
			help_widget->help.mag_widget,
			XtNmouseX,          (XtArgVal)event->xclient.data.l[3],
			XtNmouseY,          (XtArgVal)event->xclient.data.l[4],
			(String)0
		);

	XtPopup (help_shell, XtGrabNone);
	return;
} /* end of SetHelpMsg() */

/*
 * This routine is called by the main KeyPressHandler procedure, and it
 * is responsible for forwarding a user's help request.
 */
void
HandleHelpKeyPress OLARGLIST((w, event))
	OLARG( Widget,		w)
	OLGRA( XEvent *,	event)
{
	XEvent		send_event;
	Widget		widget;
	Window		target;
	Window		child_of_root;
	Window		parent		= event->xkey.root;
	Window		grandparent	= event->xkey.root;
	Display *	dpy		= event->xkey.display;
	int		x		= event->xkey.x_root;
	int		y		= event->xkey.y_root;

	XTranslateCoordinates(dpy, grandparent, parent, x, y,
					&x, &y, &child_of_root);

		/* if 'child_of_root' is not None, i.e., the pointer
		 * isn't directly over the RootWindow, find the
		 * smallest enclosing window and set parent, x, and y
		 * according to that window.
		 */
	if (child_of_root != None)
	{
		Window	child = child_of_root;

		while (child != None)
		{
			grandparent	= parent;
			parent		= child;
			XTranslateCoordinates(dpy, grandparent, parent,
						x, y, &x, &y, &child);
		}
	}
	
	if (parent == event->xkey.root)
	{
		target = parent;
		FPRINTF((stderr,"target is root\n"));
	}
	else
	{
		WMStepWidget	wm = (WMStepWidget)
					XtWindowToWidget(dpy, child_of_root);

		if (wm == (WMStepWidget)NULL) /* another's override shell */
		{
			target = child_of_root;
			FPRINTF((stderr,
				"target is another's override-redirect\n"));
		}
		else
		if (child_of_root == parent)	/* wm decoration */
		{
			target = child_of_root;
			FPRINTF((stderr, "target is wm decoration\n"));
		}
		else
		if (XtIsSubclass((Widget)wm, shellWidgetClass))
			/* wm popup shell */
		{
			target = child_of_root;
			FPRINTF((stderr, "target is wm popup shell\n"));
		}
		else
		{
			target = wm->wmstep.window;
			FPRINTF((stderr, "target is wm or another's shell\n"));
		}
	}
	
	send_event.xclient.type         = ClientMessage;
	send_event.xclient.display      = dpy;
	send_event.xclient.window       = target;
	send_event.xclient.message_type = XA_OL_HELP_KEY(dpy);
	send_event.xclient.format       = 32;
	send_event.xclient.data.l[0]    = parent;
	send_event.xclient.data.l[1]    = x;
	send_event.xclient.data.l[2]    = y;
	send_event.xclient.data.l[3]    = event->xkey.x_root;
	send_event.xclient.data.l[4]    = event->xkey.y_root;
	
	if (target == event->xkey.root)
	{
		XSendEvent(dpy, target, False, ButtonPressMask, &send_event);
		FPRINTF((stderr,
			"sending help client message to RootWindow\n"));
	}
	else
	{
		XSendEvent(dpy, target, False, NoEventMask, &send_event);
		FPRINTF((stderr,
			"sending help client message to window %x\n", target));
	}

	widget = XtWindowToWidget(dpy, target);
	if (help_parent && widget && !XtIsSubclass(widget, stubWidgetClass))
	{
		MoveWindow(help_parent, WMRaise);
		XRaiseWindow(XtDisplay(help_parent), XtWindow(help_parent));
	}
} /* end of HandleHelpKeyPress() */

/*
 * This routine sends a DT_OL_DISPLAY_HELP request to the Desktop Metaphor's
 * help manager to display help on the window manager.  It is called when
 * the Help key is pressed and the help manager is running (i.e. if
 * XGetSelectionOwner() does not return None for the _HELP_QUEUE atom.)
 * It is called from ClientNonMaskable() in Event.c.
 */
void
GetWMDesktopHelp(display, wm_win, event, text, title, app_title)
Display *display;
Window  wm_win;
XEvent  *event;
char    *text;
char    *title;
char    *app_title;
{
	char *app_name;
	char *app_class;
	char *buf;
	int  size;

	/*
	 * When and if the window manager's help files are converted to
	 * hypertext format, this should be changed to OL_DESKTOP_SOURCE.
	 */
	OlDefine source_type = OL_DISK_SOURCE;

	XtGetApplicationNameAndClass(display, &app_name, &app_class);
	size = strlen(title) + strlen(text) + strlen(app_name)
			+ strlen(app_title) + 300;
	buf = (char *)malloc(size * sizeof(char));

	sprintf(buf, "@OL_DISPLAY_HELP: SERIAL=-3 VERSION=0 CLIENT=%lu "
		"HELPTYPE=%d APPNAME=\"%s\" APPTITLE=\"%s\" TITLE=\"%s\" "
		"FILENAME=\"%s\" XPOS=%d YPOS=%d", wm_win, source_type,
		app_name, app_title, title, text, event->xclient.data.l[3],
		event->xclient.data.l[4]);

	XChangeProperty(display, wm_win, XA_WM_HELP_QUEUE(display), XA_STRING,
			8, PropModeAppend, (unsigned char *)buf, strlen(buf));

	XConvertSelection(display, XA_WM_HELP_QUEUE(display),
		XA_WM_HELP_QUEUE(display), XA_WM_HELP_QUEUE(display),
		wm_win, CurrentTime);

	free(buf);

} /* end of GetWMDesktopHelp() */
