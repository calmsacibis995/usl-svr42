/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:olwsm/wsm.c	1.88"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>

#include <Xol/OpenLookP.h>
#include <Xol/WSMcomm.h>
#include <Xol/Category.h>

#include "error.h"  
#include <misc.h>
#include <node.h>
#include <list.h>
#include <menu.h>
#include <xutil.h>  
#include <wsm.h>

#include "../Dtm.h"		/* for Desktop structure */

#if	!defined(SYSV) && !defined(SVR4)
#include <sys/wait.h>
#endif

extern String		getenv();
extern String		CurrentLocale;
extern String		PrevLocale;
Widget			InitShell;

/*
 * Convenient macros:
 */

#define PUSH(p, data)		dring_push(p, alloc_DNODE((ADDR)data))
#define PULL(p, node)		free_DNODE(dring_delete(p, node))
#define MY_RESOURCE(x) \
	( \
		x >= DISPLAY->resource_base && \
		x <= (DISPLAY->resource_base | DISPLAY->resource_mask) \
	)
#define RC			"/.olinitrc"

#define WSMCantTouchThis	0		/* -- M.C. Hammer */
#define WSMDeleteWindow		1
#define WSMXKillClient		2
#define WSMSaveYourself		3

/*
 * Special types:
 */

typedef struct WinInfo {
	Window			win;
	short			fate;
}			WinInfo, *WinInfoPtr;

/*
 * Global data:
 */

WSMresources		wsm;

Widget			handleRoot;
Widget			workspaceMenu;
Widget			programsMenu;

/*
 * Local routines:
 */

static String		WindowPosition OL_ARGS((
	Widget			w
));
static void		IgnoreWarnings OL_ARGS((
	String			message
));
static int		IgnoreErrors OL_ARGS((
	Display *		dpy,
	XErrorEvent *		event
));
static int		IgnoreClosedConnection OL_ARGS((
	Display *		dpy
));
static int		ErrorHandler OL_ARGS((
	Display *		dpy,
	XErrorEvent *		event
));
Widget			CreateHandleRoot OL_ARGS((
	void
));
static void		RegisterHandleRoot OL_ARGS((
	Widget			handle
));
static void		CheckRequestQueue OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XEvent *		event
));
static void		TerminateClient OL_ARGS((
	Window			client,
	short			action
));
static void		ServiceRequest OL_ARGS((
	void
));
static DNODE *		GetWindow OL_ARGS((
	DNODE **		root,
	Window			window
));
static void		WSMExec OL_ARGS((
	Window			client,
	int			serial,
	String			command
));
void			ExecRC OL_ARGS((
	String			path
));
static void		SetEnvironment OL_ARGS((
	Widget			w
));
void			SetWSMBang OL_ARGS((
	Display *		display,
	Window			send_to,
	unsigned long		mask
));

#ifdef DONT_USE
static int		CatchButtonPressErrors OL_ARGS((
	Display *		dpy,
	XErrorEvent *		event
));
static void		CatchKillSignal OL_ARGS((
	int			sig
));
static void		MouselessCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XEvent *		event
));
static void		ReapChild OL_ARGS((
	void
));
#endif

/*
 * Local data:
 */

static DNODE *		save_pending = NULL;

static XtResource		resources[] = {
#define offset(F) XtOffsetOf(WSMresources,F)

    {
	"workspace", "Workspace",
	XtRPixel, sizeof(Pixel), offset(workspace.pixel),
	XtRString, (XtPointer)XtDefaultBackground
    },
#ifdef DONT_USE
    {
	"menuPinned", "MenusPinned",
	XtRBoolean, sizeof(Boolean), offset(menu_pinned),
	XtRImmediate, (XtPointer)False
    },
    {
	"programsMenuPinned", "MenusPinned",
	XtRBoolean, sizeof(Boolean), offset(programs_menu_pinned),
	XtRImmediate, (XtPointer)False
    },
#endif
    {
	"warnings", "Warnings",
	XtRBoolean, sizeof(Boolean), offset(warnings),
	XtRImmediate, (XtPointer)False
    },
    {
	"depthThreshold", "DepthThreshold",
	XtRCardinal, sizeof(Cardinal), offset(depth_threshold),
	XtRImmediate, (XtPointer)5
    },
#if	defined(FACTORY_LIST)
    {
	"factoryColorLists", "FactoryColorLists",
	"ColorLists", sizeof(ColorLists *), offset(factory_color_lists),
	XtRImmediate, (XtPointer)0
    },
#endif
#ifdef DONT_USE
    {
	"initialSheet", "InitialSheet",
	XtRString, sizeof(String), offset(initial_sheet),
	XtRImmediate, (XtPointer)0
    },
    {
	"startChildren", "StartChildren",
	XtRBoolean, sizeof(Boolean), offset(start_children),
	XtRImmediate, (XtPointer)True
    },
#endif

#undef	offset
};

static String		SHELL;
static String		DEFAULT_SHELL	= "/bin/sh";

static Cardinal		Argc;
static String *		Argv;
static String		GuardArg	= "-GuardArg";

static jmp_buf		PreClosedConnectionEnvironment;

/**
 ** DmInitWSM()
 **/

int
#if	OlNeedFunctionPrototypes
DmInitWSM (
	int			argc,
	String			argv[]
)
#else
DmInitWSM (argc, argv)
	int			argc;
	String			argv[];
#endif
{
	Cardinal		n;
	Cardinal		k;

#ifdef NOT_USE
	(void)signal (SIGHUP, CatchKillSignal);
	(void)signal (SIGTERM, CatchKillSignal);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal (SIGINT, CatchKillSignal);
#endif

#if	defined(SYSV) || defined(SVR4)
	(void)setpgrp();			/* Begin new process group */
	(void)signal(SIGCLD, SIG_IGN);		/* To prevent zombies */
#else
	(void)setpgrp(0, getpid());		/* Begin new process group */
	(void)signal(SIGCLD, ReapChild);	/* To prevent zombies */
#endif

	/*
	 * Save the argument list and count, in case we have to
	 * be restarted. Since the toolkit initialization may
	 * clobber the argument list, we have to copy the strings.
	 * Don't copy the arguments that we've fed ourselves in
	 * a restart. Finally, get rid of the guard argument that
	 * separates the real args from the ones we fed ourself.
	 */
	for (Argc = 0; Argc < argc; Argc++)
		if (MATCH(argv[Argc], GuardArg))
			break;
	Argv = (String *)XtMalloc(sizeof(String) * (Argc+1));
	for (n = 0; n < Argc; n++)
		Argv[n] = XtNewString(argv[n]);
	for (n = k = 0; n < argc; n++)
		if (!MATCH(argv[n], GuardArg))
			argv[k++] = argv[n];

	InitShell = DESKTOP_SHELL(Desktop);
        SetEnvironment (InitShell);

#if	defined(FACTORY_LIST)
	XtSetTypeConverter (
		XtRString,
		"ColorLists",
		StringToColorLists,
		(XtConvertArgList)0,
		0,
		XtCacheByDisplay,
		(XtDestructor)0
	);
#endif
	OlGetApplicationResources(
		InitShell,
		(XtPointer)&wsm,
		resources,
		XtNumber(resources),
		(Arg *)0,
		0
	);
	if (wsm.depth_threshold > 16)
		wsm.depth_threshold = 16;

#ifdef WSM_MENU
	XSync (DISPLAY, False);
	XSetErrorHandler (ErrorHandler);
#endif
	/* This is now done in dtm:main() */
#ifdef DONT_USE
	handleRoot = CreateHandleRoot();
#endif

	RegisterHandleRoot (handleRoot);

#ifdef WSM_MENU
	XSync (DISPLAY, False);
	XSetErrorHandler (CatchButtonPressErrors);
#endif
	InitProperty (DISPLAY);
#ifdef WSM_MENU
	CreateWorkSpaceMenu (handleRoot, &workspaceMenu, &programsMenu);
#endif

	XSync (DISPLAY, False);
	XSetErrorHandler (ErrorHandler);

	ClearWSMQueue (DISPLAY);

	XSetWindowBackground (DISPLAY, ROOT, wsm.workspace.pixel);
	XClearWindow (DISPLAY, ROOT);
	XSync (DISPLAY, False);

	/* This is now done in dtm:main() */
#ifdef DONT_USE
	if (wsm.start_children)
		ExecRC (GetPath(RC));
#endif

#ifdef WSM_MENU
	OlGrabVirtualKey (
		handleRoot, OL_WORKSPACEMENU,
		False, GrabModeAsync, GrabModeSync
	);
	XtAddEventHandler(
		handleRoot, KeyPressMask, False, MouselessCB, NULL
	);
#endif

#ifdef DONT_USE
	if (wsm.initial_sheet)
		PropertySheetByName (wsm.initial_sheet);
	BusyPeriod (handleRoot, False);
#endif

	return(0);		/* Always??!! */

} /* DmInitWSM */

/**
 ** RestartWorkspaceManager()
 **/

void
#if	OlNeedFunctionPrototypes
RestartWorkspaceManager (
	void
)
#else
RestartWorkspaceManager ()
#endif
{
	Widget			property_window	= 0;

	String			property_sheet	= 0;

	Cardinal		argc;
	Cardinal		nargs;
	Cardinal		n;

	Window			window = 0;
	char			buffer[PATH_MAX];

	static String *		argv;	/* because of longjmp (ick) */ 

	static struct menu {
		Widget *		menu;
		String			resource;
	}			menus[] = {
		{ &workspaceMenu, "menuPinned"         },
		{ &programsMenu,  "programsMenuPinned" },
	};

#define PINNED(W) ((ShellWidget)W)->shell.popped_up

	DmPrepareForRestart();

	/* Execute locale change script */
	sprintf(buffer,"%s/lib/locale/%s/init.rc", getenv("XWINHOME"),
		CurrentLocale); /* $XWINHOME is always set (in dtm.c) */
	if (!access(buffer, F_OK | X_OK)) {
		strcat(buffer, " ");
		strcat(buffer, PrevLocale);
		system(buffer);
	}

	/*
	 * Accumulate number of arguments needed.
	 */

	nargs = Argc;
	nargs++;	/* guard arg */

	PropertySheetStatus (&property_window, &property_sheet);
	if (property_window && property_sheet)
		nargs += 6;

	for (n = 0; n < XtNumber(menus); n++) {
		struct menu *	p      = &menus[n];

		nargs += 2;	 /* state */
		if (PINNED(*p->menu))
			nargs += 2; /* position */
	}

	nargs += 2;	/* don't start children */

	/*
	 * Allocate space for the arguments and construct the
	 * argument list.
	 */

	argv = (String *)XtMalloc(sizeof(String) * (nargs+1));
	for (argc = 0; argc < Argc; argc++)
		argv[argc] = Argv[argc];
	argv[argc++] = GuardArg;

	if (property_window && property_sheet) {
		String			window_state;
		String			sheet_state;
		char 			window_name[2048];
		char *			p;
		Widget			widget;

		for (p = window_name, widget = property_window; 
			widget; 
			widget = XtParent(widget))
			p += sprintf(p, "%s.", XtName(widget));
/*
		window_name = _OlFullName(property_window);
*/
		window_state = XtMalloc(1+strlen(window_name)+14+1);
		sprintf (window_state, "*%s.pushpin: in", window_name);

		sheet_state = XtMalloc(15+strlen(property_sheet)+1);
		sprintf (sheet_state, "*initialSheet: %s", property_sheet);

		argv[argc++] = "-xrm";
		argv[argc++] = window_state;
		argv[argc++] = "-xrm";
		argv[argc++] = WindowPosition(property_window);
		argv[argc++] = "-xrm";
		argv[argc++] = sheet_state;
	}

	for (n = 0; n < XtNumber(menus); n++) {
		struct menu *	p      = &menus[n];
		String		state;

		state = XtMalloc(1+strlen(p->resource)+2+5+1);
		sprintf (
			state,
			"*%s: %s",
			p->resource,
			(PINNED(*p->menu)? "true" : "false")
		);
		argv[argc++] = "-xrm";
		argv[argc++] = state;
		if (PINNED(*p->menu)) {
			argv[argc++] = "-xrm";
			argv[argc++] = WindowPosition(*p->menu);
		}

	}

	argv[argc++] = "-xrm";
	argv[argc++] = "*startChildren: false";

	argv[argc]   = 0;

	BusyPeriod (handleRoot, True);

	/*
	 * Destroy everything, to get the X server to forget we
	 * ever existed. This prevents the failure of our restarting
	 * and attempting to get sole access to special resources
	 * (e.g. ButtonPress on root).
	 *
	 * NOTE: I've tried all sorts of things to get the server
	 * to forget about all our current resources:
	 *
	 *	- XtDestroyWidget on all top-level widgets,
	 *	  adding a XtNdestroyWidget callback to know
	 *	  when Xt has finished cleaning up and we can
	 *	  close the display. This doesn't work, because
	 *	  Xt never seems to get around to the phase 2
	 *	  destruction of key widgets.
	 *
	 *	- XtDestroyApplicationContext.
	 *
	 *	- Making sure the close-down-mode is DestroyAll before
	 *	  closing the display.
	 *
	 *	- Physically closing the FD associated with the
	 *	  display: close(ConnectionNumber(DISPLAY)).
	 *
	 *	- Closing every darned FD that's open except stdin,
	 *	  stdout, stderr.
	 *
	 * For all but the first, the pinned windows don't go away, even
	 * across the re-exec!! Of course, the windows are dumb
	 * (no events are processed, e.g. no exposures), but they
	 * hang around. Note that on *subsequent* restarts the pinned
	 * windows go away.
	 *
	 * The following seems to work. If you have a better idea,
	 * I'd be real interested in hearing it.
	 *
	 * Note on the following method: We need any X resource
	 * owned by us. The window of handleRoot won't work, because
	 * we don't own the root window. The window of InitShell
	 * won't work, because that widget hasn't been realized (no
	 * window). Also, if none of the following windows exist,
	 * then we can't ``kill'' ourselves but we still have to
	 * remove our preemptive access to the root window.
	 */
	if (property_window)
		window = XtWindowOfObject(property_window);

	if (workspaceMenu && !window)
		window = XtWindowOfObject(workspaceMenu);

	if (programsMenu && !window)
		window = XtWindowOfObject(programsMenu);

	if (window) {
		XSetIOErrorHandler (IgnoreClosedConnection);
		XKillClient (DISPLAY, window);
		if (setjmp(PreClosedConnectionEnvironment) == 0)
			XSync (DISPLAY, False);
		/*
		 * Nothing X-ish can be used between here and the
		 * exec below!!! Furthermore, any values we
		 * need after the longjmp is called must be in
		 * static (or external) variables!!
		 */
	} else {
		EventMask	mask	= XtBuildEventMask(handleRoot);

		XSelectInput (DISPLAY, ROOT, mask & ~ButtonPressMask);
		OlUngrabVirtualKey (handleRoot, OL_WORKSPACEMENU);
		XSync (DISPLAY, False);
	}

	/*
	 * We've cleaned up our X server resources, so now we
	 * can start again.
	 */
	execvp (argv[0], argv);
	/*NOTREACHED*/

#undef	PINNED
} /* RestartWorkspaceManager */

/**
 ** WindowPosition()
 **/

static String
#if	OlNeedFunctionPrototypes
WindowPosition (
	Widget			w
)
#else
WindowPosition (w)
	Widget			w;
#endif
{
	int			x;
	int			y;

	Dimension		width  = w->core.width;
	Dimension		height = w->core.height;
	Dimension		header;
	Dimension		side;

	Window			junk;

	String			geometry;
/*
	String			name   = _OlFullName(w);
*/
	char 			name[2048];
	char *			p;
	Widget			widget;

	for (p = name, widget = w; widget; widget = XtParent(widget))
		p += sprintf(p, "%s.", XtName(widget));

	XTranslateCoordinates (
		XtDisplayOfObject(w),
		XtWindowOfObject(w),
		RootWindowOfScreen(XtScreenOfObject(w)),
		0, 0,
		&x, &y,
		&junk
	);

	/*
	 * Foo on the ICCC--the window manager positions the *decorated*
	 * window, not the client window.
	 */
	_OlWMFrameDimensions (w, &header, &side, (Dimension *)0);
	x -= side;
	y -= header;

#define SIGN(x) (x < 0? "" : "+")
#define N 20
	geometry = XtMalloc(1+strlen(name)+11+N+1+N+1+N+1+N+1);
	sprintf (
		geometry,
		"*%s.geometry: %ldx%ld%s%ld%s%ld",
		name,
		width, height,
		SIGN(x), x, SIGN(y), y
	);
#undef	N
#undef	SIGN

	return (geometry);
} /* WindowPosition */

/**
 ** GetPath()
 **/

String
#if	OlNeedFunctionPrototypes
GetPath (
	String			name
)
#else
GetPath (name)
	String			name;
#endif
{
	String			ptr;

	struct passwd *		pw;

	int			uid;

#ifndef SVR4
	extern unsigned short	getuid();
#endif /* SVR4 */


	if ((ptr = getenv("HOME")))
		return (CONCAT(ptr, name));

	if ((ptr = getenv("USER")))
		pw = getpwnam(ptr);
	else {
		uid = (int)getuid();
		pw = getpwuid(uid);
	}
	if (pw)
		return (CONCAT(pw->pw_dir, name));

	return (0);
} /* GetPath */

/**
 ** FooterMessage()
 **/

void
#if	OlNeedFunctionPrototypes
FooterMessage (
	Widget			w,
	String			message,
	OlDefine		side,
	Boolean			beep
)
#else
FooterMessage (w, message, side, beep)
	Widget			w;
	String			message;
	OlDefine		side;
	Boolean			beep;
#endif
{
	if (beep && message)
		_OlBeepDisplay (w, 1);
	if (!message)
		message = "";

	/*
	 * Walk up the widget tree to the CategoryWidget--it owns the
	 * footer space.
	 */
	while (w && XtClass(w) != categoryWidgetClass)
		w = XtParent(w);

	if (w)
		XtVaSetValues (
			w,
			(side == OL_RIGHT? XtNrightFoot : XtNleftFoot),
			(XtArgVal)message,
			(String)0
		);

	return;
} /* FooterMessage */

/**
 ** WSMExit()
 **/
void
WSMExit (void)
{
    extern int DtmExitCode;
    static Boolean first_time = True;

    /* After (normal) exit below, this will be called again from AtExit
       in dtm.c - ignore it.
    */
    if (!first_time)
	return;
    first_time = False;

    if (save_pending) {
	debug((stderr,
	       "WSMExit: exiting with client-saves pending\n"));
	/*
	 *	Should we do something intelligent here
	 *	like post a notice?
	 */
    }

    (void)signal (SIGHUP, SIG_IGN);
    (void)signal (SIGTERM, SIG_IGN);
    (void)signal (SIGINT, SIG_IGN);

    XCloseDisplay (DISPLAY);
    kill (0, SIGTERM);			/* kill all processes in our group */

    exit (DtmExitCode);
    /*NOTREACHED*/
}					/* WSMExit */

/**
 ** IgnoreWarnings()
 **/

static void
#if	OlNeedFunctionPrototypes
IgnoreWarnings (
	String			message
)
#else
IgnoreWarnings (message)
	String			message;
#endif
{
	return;
} /* IgnoreWarnings */

/**
 ** IgnoreErrors()
 **/

static int
#if	OlNeedFunctionPrototypes
IgnoreErrors (
	Display *		dpy,
	XErrorEvent *		event
)
#else
IgnoreErrors(dpy, event)
	Display *		dpy;
	XErrorEvent *		event;
#endif
{
	return (0);
} /* IgnoreErrors */

/**
 ** IgnoreClosedConnection()
 **/

static int
#if	OlNeedFunctionPrototypes
IgnoreClosedConnection (
	Display *		dpy
)
#else
IgnoreClosedConnection (dpy)
	Display *		dpy;
#endif
{
	longjmp (PreClosedConnectionEnvironment, 1);
	/*NOTREACHED*/
} /* IgnoreClosedConnection */

#ifdef WSM_MENU
/**
 ** CatchButtonPressErrors()
 **/

static int
#if	OlNeedFunctionPrototypes
CatchButtonPressErrors (
	Display *		dpy,
	XErrorEvent *		event
)
#else
CatchButtonPressErrors (dpy, event)
	Display *		dpy;
	XErrorEvent *		event;
#endif
{
	extern int		_XDefaultError();

 	trace("CatchButtonPressErrors");
	if (event->error_code == BadAccess) {
		Dm__VaPrintMsg(TXT_errorMsg_otherControl);
	}
	return (_XDefaultError(dpy, event));
} /* CatchButtonPressErrors */
#endif /* WSM_MENU */

/**
 ** ErrorHandler()
 **/

static int
#if	OlNeedFunctionPrototypes
ErrorHandler (
	Display *		dpy,
	XErrorEvent *		event
)
#else
ErrorHandler (dpy, event)
	Display *		dpy;
	XErrorEvent *		event;
#endif
{
	extern int		_XDefaultError();

	return (_XDefaultError(dpy, event));
} /* ErrorHandler */

#ifdef DONT_USE
/**
 ** CatchKillSignal()
 **/

static void
#if	OlNeedFunctionPrototypes
CatchKillSignal (
	int			sig
)
#else
CatchKillSignal (sig)
	int			sig;
#endif
{
	WSMExit();
	/*NOTREACHED*/
} /* CatchKillSignal */
#endif

/**
 ** ReapChild()
 **/

#if	!defined(SVR4)

static void
#if	OlNeedFunctionPrototypes
ReapChild (
	void
)
#else
ReapChild ()
#endif
{
	union wait		status;

	int			pid;


	pid = wait3(&status, WNOHANG, NULL);
	if (pid < 0)
		perror ("fruitless wait3");
	return;
} /* ReapChild */

#endif

/**
 ** CreateHandleRoot()
 **/

Widget
#if	OlNeedFunctionPrototypes
CreateHandleRoot (
	void
)
#else
CreateHandleRoot ()
#endif
{
	Widget			w = XtWindowToWidget(DISPLAY, ROOT);

	if (!w) {
		Dm__VaPrintMsg(TXT_errorMsg_noWidget);
	}
	return (w);
} /* CreateHandleRoot */

/**
 ** RegisterHandleRoot()
 **/

static void
#if	OlNeedFunctionPrototypes
RegisterHandleRoot (
	Widget			handle
)
#else
RegisterHandleRoot (handle)
	Widget			handle;
#endif
{
	/*
	 * Set up service on workspace manager queue.
	 */
	XtAddEventHandler (
		handle, PropertyChangeMask, False,
		(XtEventHandler)CheckRequestQueue, NULL
	);

	return;
} /* RegisterHandleRoot */

/**
 ** CheckRequestQueue()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckRequestQueue (
	Widget			w,
	XtPointer		client_data,
	XEvent *		event
)
#else
CheckRequestQueue (w, client_data, event)
	Widget			w;
	XtPointer		client_data;
	XEvent *		event;
#endif
{
	XPropertyEvent *	p = (XPropertyEvent *)event;

	/*
	 * Check for workspace manager request and service:
	 */
	if (p->atom == XA_OL_WSM_QUEUE(DISPLAY) && p->state == PropertyNewValue)
		ServiceRequest ();
	return;
} /* CheckRequestQueue */

#ifdef WSM_MENU
/**
 ** MouselessCB()
 **/

static void
#if	OlNeedFunctionPrototypes
MouselessCB (
	Widget			w,
	XtPointer		client_data,
	XEvent *		event
)
#else
MouselessCB (w, client_data, event)
	Widget			w;
	XtPointer		client_data;
	XEvent *		event;
#endif
{
	Display *		display	= XtDisplayOfObject(w);

	OlVirtualEventRec       ve;
	Time			time = CurrentTime;


	OlLookupInputEvent(w, event, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_WORKSPACEMENU:
                XAllowEvents (display, AsyncKeyboard, CurrentTime);
			/* Call XtUngrabKeyboard here since calling
			 * XUngrabKeyboard prevents the next keypress
			 * from finding the right destination widget.
			 */
                XtUngrabKeyboard (handleRoot, CurrentTime);
		XtCallAcceptFocus (workspaceMenu, &time);
                break;

	default:
                break;
	}

	return;
} /* MouselessCB */
#endif /* WSM_MENU */

/**
 ** TerminateClient()
 **/

static void
#if	OlNeedFunctionPrototypes
TerminateClient (
	Window			client,
	short			action
)
#else
TerminateClient (client, action)
	Window			client;
	short			action;
#endif
{
	debug ((
		stderr,
		"TerminateClient: action=%s clients=%x\n",
		(action == WSMDeleteWindow? "DeleteWindow":
		(action == WSMXKillClient ? "KillClient": "UNKNOWN")),
		client
	));

	switch (action) {

	case WSMDeleteWindow:
		debug ((stderr, "SendProtocolMessage\n"));
		SendProtocolMessage(
			DISPLAY, client,
			XA_WM_DELETE_WINDOW(DISPLAY), CurrentTime
		);
		break;

	case WSMXKillClient:
		/*
		 * Don't kill our windows!
		 */
		if (!MY_RESOURCE(client)) {
			debug ((stderr, "XKillClient\n"));
			XSync (DISPLAY, False);
			XKillClient (DISPLAY, client);
			XSync (DISPLAY, False);
		} else
			debug ((stderr, "No XKillClient, WSM client!\n"));
		break;

	default:
		debug((stderr, "TerminateClient: unknown action\n"));
		break;
	}

	return;
} /* TerminateClient */

/**
 ** ServiceEvent()
 **/

void 
#if	OlNeedFunctionPrototypes
ServiceEvent (
	XEvent *		event
)
#else
ServiceEvent (event)
	XEvent *		event;
#endif
{
	if (event->type == PropertyNotify) {
		XPropertyEvent *	p = (XPropertyEvent *)event;
		DNODE *			q;

		/*
		 * Check for client notification of SAVE_YOURSELF done:
		 */
		if (p->window != ROOT && p->atom == XA_WM_COMMAND) {
			if (q = GetWindow(&save_pending, p->window)) {
				short		action;

				action = ((WinInfoPtr)DNODE_data(q))->fate;
				FREE (DNODE_data(q));
				PULL (&save_pending, q);

				/*
				 * Is client now due for termination?
				 */
				if (action != WSMCantTouchThis) {
					XSetErrorHandler (IgnoreErrors);
					XSync (DISPLAY, False);
					TerminateClient (p->window, action);
					XSync (DISPLAY, False);
					XSetErrorHandler (ErrorHandler);
				}
			} else {
				debug((stderr,
					"ServiceEvent: unknown window\n"));
			}
			return;
		}
	}
	XtDispatchEvent (event); /* normal Xt dispatch */

	return;
} /* ServiceEvent */

/**
 ** ServiceRequest()
 **/

static void
#if	OlNeedFunctionPrototypes
ServiceRequest (
	void
)
#else
ServiceRequest ()
#endif
{
	unsigned char		type;
	Window			client;
	int			serial;
	String			name;
	String			command;
	WSM_Request		request;
	short			action;
	DNODE *			p;
	WinInfoPtr		info;


	trace("ServiceRequest: IN");

	XSync (DISPLAY, False);
	XSetErrorHandler (IgnoreErrors);

	/*
	 * Service all queued workspace manager requests:
	 */
	while (DequeueWSMRequest(DISPLAY, &client, &type, &request)
		== GOTREQUEST) {

		serial  = request.serial;
		name    = request.name;
		command = request.command;

		switch (type) {

		case WSM_EXECUTE:
			debug((stderr, "%s: WSM_EXECUTE\n", name));
			WSMExec (client, serial, command);
			break;

		case WSM_TERMINATE:
			debug ((stderr, "%s: WSM_TERMINATE %s\n", name, command));
			if (command && MATCH(command, "WM_DELETE_WINDOW"))
			        action = WSMDeleteWindow;
			else
				action = WSMXKillClient;

			if ((p = GetWindow(&save_pending, client))) {
				/*
				 * Postpone death until client is
				 * through saving itself.
				 */
				((WinInfoPtr)DNODE_data(p))->fate = action;
			} else {
			        TerminateClient (client, action);
			}
			break;

		case WSM_SAVE_YOURSELF:
			debug((stderr, "%s: WSM_SAVE_YOURSELF\n", name));
			XSelectInput (DISPLAY, client, PropertyChangeMask);
			/*
			 * Old protocol...
			 */
			SetWSMBang (DISPLAY, client, NoEventMask);
			/*
			 * New protocol...
			 */
			SendProtocolMessage (
			    DISPLAY, client,
			    XA_WM_SAVE_YOURSELF(DISPLAY), CurrentTime
			);
			info = ELEMENT(WinInfo);
			info->win = client;
			info->fate = WSMCantTouchThis;
			PUSH (&save_pending, info);
			break;

		case WSM_EXIT:
			debug((stderr, "%s: WSM_EXIT\n", name));
			WSMExit ();
			/*NOTREACHED*/

		case WSM_MERGE_RESOURCES:
			debug((stderr, "%s: WSM_MERGE_RESOURCES\n", name));
			MergeResources (command);
			break;

		case WSM_DELETE_RESOURCES:
			debug((stderr, "%s: WSM_DELETE_RESOURCES\n", name));
			DeleteResources (command);
			break;

		default:
			debug((stderr, "%s: UNKNOWN REQUEST\n", name));
			break;
		}
	}
	XSync (DISPLAY, False);
	XSetErrorHandler (ErrorHandler);
	trace("ServiceRequest: OUT");

	return;
} /* ServiceRequest */

/**
 ** GetWindow()
 **/

static DNODE *
#if	OlNeedFunctionPrototypes
GetWindow (
	DNODE **		root,
	Window			window
)
#else
GetWindow (root, window)
	DNODE **		root;
	Window			window;
#endif
{
	DNODE *			p;
	dring_ITERATOR		I;


	I = dring_iterator(root);
	while (p = dring_next(&I)) {
		if (window == ((WinInfoPtr)DNODE_data(p))->win) {
			return (p);
		}
	}
	return (0);
} /* GetWindow */

/**
 ** WSMExec()
 **/

static void
#if	OlNeedFunctionPrototypes
WSMExec (
	Window			client,
	int			serial,
	String			command
)
#else
WSMExec (client, serial, command)
	Window			client;
	int			serial;
	String			command;
#endif
{
	int			pid;
	static WSM_Reply	reply;


	reply.serial = serial;

	switch (pid = fork()) {
	case 0:
		signal (SIGCLD, SIG_DFL);
		(void)execl (SHELL, "sh", "-c", command, (String)0);
		(void)execl (DEFAULT_SHELL, "sh", "-c", command, (String)0);
		reply.detail = errno;
		SendWSMReply (DISPLAY, client, WSM_EXEC_FAILURE, &reply);
		exit (-1);
		/*NOTREACHED*/
	case -1:
		reply.detail = errno;
		SendWSMReply (DISPLAY, client, WSM_FORK_FAILURE, &reply);
		break;
	default:
		reply.detail = pid;
		SendWSMReply (DISPLAY, client, WSM_SUCCESS, &reply);
		break;
	}

	return;
} /* WSMExec */

/* An attempt to minimize contention at startup:  in the olinitrc file, DO
 * NOT start the window manager or the file manager in the background.  This
 * way, the initialization code for the two applications is run sequentially.
 */

#define WAIT_FOR_RC  (unsigned) 240

static int child_exited = 0;

static void
do_nothing(int signal_fodder)
{
	if (signal_fodder == SIGCLD) {
		child_exited++;
	}
}

/**
 ** ExecRC()
 **/
void
ExecRC (String path)
{
void (*orig_sigcld)(int);

    if ((path == NULL) || (access(path, 4) != 0))
    {
	Dm__VaPrintMsg(TXT_warningMsg_noFile, path);
	return;
    }

    /*
     * Need to install the signal handler here. Otherwise, the shell
     * process may exit before the signal handler is installed, and then
     * dtm will be sitting in pause() forever...
     */
    orig_sigcld = sigset (SIGCLD, do_nothing);

    if (fork() == 0)
    {
	(void)signal(SIGCLD, orig_sigcld);
	(void)execl (SHELL, "sh", path, (String)0);
	(void)execl (DEFAULT_SHELL, "sh", path, (String)0);
	_exit(0);

    } else 
    {
	void (*orig_sigalrm)(int);

	/* This is used after starting RC file execution   */
	/* to signal completion.  It starts olwm and dsdm, */
	/* which go into background when about to start    */
	/* MainLoop.  An alarm is used so we don't wait    */
	/* forever if someone forgot to put "&" in  */
	/* something other than dsdm or olwm. */
	if (!child_exited) {
		alarm (WAIT_FOR_RC);
		orig_sigalrm = sigset (SIGALRM, do_nothing);
		pause();	/* In parent, wait for startup child shell */

		alarm(0);	/* Child started so disable alarm */

		/* Restore signal handlers */
		(void)signal(SIGCLD, orig_sigcld);
		(void)signal (SIGALRM, orig_sigalrm);
	}
    }
}					/* end of ExecRC */

#ifdef WSM_MENU
/**
 ** ExecCommand()
 **/

int
#if	OlNeedFunctionPrototypes
ExecCommand (
	String			command
)
#else
ExecCommand (command)
	String			command;
#endif
{
	int			pid;


	debug((stderr, "ExecCommand: command = \"%s\"\n", command));

	switch (pid = fork()) {
	case 0:
		signal (SIGCLD, SIG_DFL);
		(void)execl (SHELL, "sh", "-c", command, (String)0);
		(void)execl (DEFAULT_SHELL, "sh", "-c", command, (String)0);
		exit (-1);
		/*NOTREACHED*/
	case -1:
		Dm__VaPrintMsg(TXT_warningMsg_forkFailed);
		break;
	default:
		debug ((stderr, "ExecCommand: pid = %d\n", pid));
		break;
	}
	return (pid);
} /* ExecCommand */
#endif /* WSM_MENU */

/**
 ** SetEnvironment()
 **/

static void
#if	OlNeedFunctionPrototypes
SetEnvironment (
	Widget			w
)
#else
SetEnvironment (w)
	Widget			w;
#endif
{
#define FORMAT	"DISPLAY=%s"

	static char		buf[128];

	String			p;
	String			sdisplay;

	Cardinal		len;


	if (!(SHELL = getenv("SHELL")) || !*SHELL)
		SHELL = DEFAULT_SHELL;

	sdisplay = XDisplayString(XtDisplayOfObject(w));

	len = sizeof(FORMAT) + strlen(sdisplay) + 1;
	if (len > XtNumber(buf))
		p = XtMalloc(len);
	else
		p = buf;

	sprintf (p, FORMAT, sdisplay);
	putenv (p);

	/*
	 * Don't free "p", as the environment is still using it!
	 */

	return;
}

/**
 ** SetWSMBang()
 **/

void
#if	OlNeedFunctionPrototypes
SetWSMBang (
	Display *		display,
	Window			send_to,
	unsigned long		mask
)
#else
SetWSMBang (display, send_to, mask)
	Display *		display;
	Window			send_to;
	unsigned long		mask;
#endif
{
	XEvent			sev;

	sev.xclient.type = ClientMessage;
	sev.xclient.display = display;
	sev.xclient.window = send_to;
	sev.xclient.message_type = XA_BANG(display);
	sev.xclient.format = 8;
	XSendEvent (display, send_to, False, mask, &sev);

	return;
} /* SetWSMBang */
