/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcalc:dtcalc.c	1.6"

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

/*
 * Define FIX_ERRNO as 0 to avoid compiling code that saves and restores
 * errno. Define it as 1 to compile this code. "Undefine" it to have it
 * track the IEEE flag.
 */
#if	!defined(FIX_ERRNO)
# if	!defined(IEEE)
#  define FIX_ERRNO 1
# else
#  define FIX_ERRNO 0
# endif
#endif

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "signal.h"
#if	FIX_ERRNO
#include "errno.h"
#endif

#include "X11/Intrinsic.h"
#include "X11/StringDefs.h"
#include "X11/Shell.h"		/* for XtNminWidth, etc. */

#include "Xol/OpenLook.h"
#include "Xol/Dynamic.h"	/* for OlReplayBtnEvent */
#include "Xol/FButtons.h"
#include "Xol/Panes.h"
#include "Xol/StaticText.h"
#include "Xol/OlCursors.h"

#include "xcalc.h"
#include "actions.h"

#ifndef IEEE
extern signal_t		fperr();
extern signal_t		illerr();
#endif

extern Widget
OlCreateIconWidget OL_ARGS((
      Widget                  shell,
      Pixmap                  pixmap
));

/*
 * Convenient macros:
 */

#define CString OLconst char * OLconst

#define ClassName	(CString)"DtCalc"
#define HelpFile	(CString)"/calc.hlp"
#define Usage		(CString)"usage: %s\n"
#define Missing		(CString)"No app-defaults resources; look for $XWINHOME/lib/app-defaults/%s\n"

	/*
	 * Stolen from Xol/ConvertersI.h:
	 */
#define ConversionDone(type,value) {					\
	if (to->addr) {							\
		if (to->size < sizeof(type)) {				\
			to->size = sizeof(type);			\
			return (False);					\
		}							\
		*(type *)(to->addr) = (value);				\
	} else {							\
		static type static_value;				\
		static_value = (value);					\
		to->addr = (XtPointer)&static_value;			\
	}								\
	to->size = sizeof(type);					\
} return (True)

/*
 * Public routines:
 */

extern void		draw OL_ARGS((
	String			string
));
extern void		setflag OL_ARGS((
	int			indicator,
	Boolean			on
));
extern void		ringbell OL_ARGS((
	void
));
extern void		Quit OL_ARGS((
	void
));
extern void		do_select OL_ARGS((
	Time			time
));

/*
 * Public data:
 */

	/*
	 * These are here to avoid having to touch math.c and actions.c
	 */
Atom			wm_delete_window;
int			rpn = 0;
#define LCD_STR_LEN	32
char			dispstr[LCD_STR_LEN];

/*
 * Private routines:
 */

static Widget		CreateIndicator OL_ARGS((
	String			name,
	Widget			parent
));
static void		Activate OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		Consume OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static Boolean		CvtStringToPointer OL_ARGS((
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data	/*NOTUSED*/
));

/*
 * Private data:
 */

static Display *	dpy;

static Widget		top;
static Widget		LCD;
static Widget		ind[6];
static Widget		buttons;

static XtAppContext	app;

static String		fallback_resources = "*appDefaultsMissing:true";

static struct app_defaults {
	Pixmap			icon_pixmap;
	Boolean			missing;
	Boolean			usage;
}			app_defaults;

static XtResource	resources[] = {
#define offset(F) XtOffsetOf(struct app_defaults, F)

    {	/* GI */
	"iconPixmap", "IconPixmap",
	XtRPixmap, sizeof(Pixmap), offset(icon_pixmap),
	XtRImmediate, (XtPointer)None
    }, 
    {	/* GI */
	"appDefaultsMissing", "AppDefaultsMissing",
	XtRBoolean, sizeof(Boolean), offset(missing),
	XtRImmediate, (XtPointer)False
    }, 
    {	/* GI */
	"usage", "Usage",
	XtRBoolean, sizeof(Boolean), offset(usage),
	XtRImmediate, (XtPointer)False
    },

#undef	offset
};

static XrmOptionDescRec		options[] = {
	{ "-?", "usage", XrmoptionNoArg, (XtPointer)"on" },
};

static OLconst XtCallbackRec	consume[] = {
	Consume,           (XtPointer)0,
	(XtCallbackProc)0, (XtPointer)0
};

/**
 ** main()
 **/

int
#if	OlNeedFunctionPrototypes
main (
	int			argc,
	char *			argv[]
)
#else
main (argc, argv)
	int			argc;
	char *			argv[];
#endif
{
	Widget			base;
	Widget			frame;
	Widget			icon = 0;

	Dimension		width;
	Dimension		height;

	String			help;


	OlToolkitInitialize (&argc, argv, (XtPointer)0);
	top = XtVaAppInitialize(
		&app,
		ClassName,
		options, XtNumber(options),
		&argc, argv,
		&fallback_resources,
		XtNconsumeEvent, (XtArgVal)&consume,
		(String)0
	);

	dpy = XtDisplay(top);
	wm_delete_window = XA_WM_DELETE_WINDOW(dpy); /* see actions.c */

	XtGetApplicationResources (
		top,
		(XtPointer)&app_defaults,
		resources, XtNumber(resources),
		(ArgList)0, (Cardinal)0
	);
	if (app_defaults.missing)
		fprintf (stderr, Missing, ClassName);
	if (app_defaults.usage)
		fprintf (stderr, Usage, argv[0]);
	if (app_defaults.missing)
		exit (1);
	if (app_defaults.usage)
		exit (0);

	if (app_defaults.icon_pixmap != None)
		icon = OlCreateIconWidget(top, app_defaults.icon_pixmap);

	XtSetTypeConverter (
		XtRString, XtRPointer, CvtStringToPointer,
		(XtConvertArgList)0, 0, XtCacheNone, (XtDestructor)0
	);

	base = XtVaCreateManagedWidget(
		"base", panesWidgetClass, top,
		XtNconsumeEvent, (XtArgVal)&consume,
		(String)0
	);

	frame = XtVaCreateManagedWidget(
		"frame", panesWidgetClass, base,
		XtNconsumeEvent, (XtArgVal)&consume,
		(String)0
	);

	LCD = XtVaCreateManagedWidget(
		"LCD", staticTextWidgetClass, frame,
		XtNconsumeEvent, (XtArgVal)&consume,
		(String)0
	);

	ind[XCalc_MEMORY]  = CreateIndicator("M", frame);
	ind[XCalc_INVERSE] = CreateIndicator("INV", frame);
	ind[XCalc_DEGREE]  = CreateIndicator("DEG", frame);
	ind[XCalc_RADIAN]  = CreateIndicator("RAD", frame);
	ind[XCalc_GRADAM]  = CreateIndicator("GRAD", frame);
	ind[XCalc_PAREN]   = CreateIndicator("PAREN", frame);

	buttons = XtVaCreateManagedWidget(
		"buttons", flatButtonsWidgetClass, base,
		XtNselectProc,   (XtArgVal)Activate,
		XtNconsumeEvent, (XtArgVal)&consume,
		(String)0
	);

	XtAppAddActions (app, Actions, XtNumber(Actions));
	XtSetKeyboardFocus (top, buttons);

	XtRealizeWidget (top);

	help = XtResolvePathname(
		dpy, "help", (String)0, HelpFile,
		(String)0, (Substitution)0, (Cardinal)0,
		(XtFilePredicate)0
	);
	if (help) {
		OlRegisterHelp (
			OL_WIDGET_HELP, (XtPointer)top, (String)0,
			OL_DISK_SOURCE, (XtPointer)help
		);
		if (icon)
			OlRegisterHelp (
				OL_WIDGET_HELP, (XtPointer)icon, (String)0,
				OL_DISK_SOURCE, (XtPointer)help
			);
	}

	XtVaGetValues (
		top,
		XtNwidth,  (XtArgVal)&width,
		XtNheight, (XtArgVal)&height,
		(String)0
	);
	XtVaSetValues (
		top,
		XtNminWidth,  (XtArgVal)width,
		XtNminHeight, (XtArgVal)height,
		XtNmaxWidth,  (XtArgVal)width,
		XtNmaxHeight, (XtArgVal)height,
		(String)0
	);

#ifndef IEEE
	signal(SIGFPE,fperr);
	signal(SIGILL,illerr);
#endif
	ResetCalc();

	XtAppMainLoop (app);
	/*NOTREACHED*/
}

/**
 ** draw() - See math.c
 **/

void
#if	OlNeedFunctionPrototypes
draw (
	String		string
)
#else
draw (string)
	String		string;
#endif
{
#if	FIX_ERRNO
	int save_errno = errno;
#endif
	XtVaSetValues (LCD, XtNstring, string, (String)0);
	OlUpdateDisplay (LCD);
#if	FIX_ERRNO
	errno = save_errno;
#endif
	return;
} /* draw */

/**
 ** setflag() - See math.c
 **/

void
#if	OlNeedFunctionPrototypes
setflag (
	int			indicator,
	Boolean			on
)
#else
setflag (indicator, on)
	int			indicator;
	Boolean			on;
#endif
{
#if	FIX_ERRNO
	int save_errno = errno;
#endif
	if (ind[indicator])
		if (on)
			XtMapWidget (ind[indicator]);
		else
			XtUnmapWidget (ind[indicator]);
#if	FIX_ERRNO
	errno = save_errno;
#endif
	return;
} /* setflag */

/**
 ** ringbell() - See math.c
 **/

void
#if	OlNeedFunctionPrototypes
ringbell (
	void
)
#else
ringbell ()
#endif
{
#if	FIX_ERRNO
	int save_errno = errno;
#endif
	_OlBeepDisplay (LCD, 1);
#if	FIX_ERRNO
	errno = save_errno;
#endif
	return;
} /* ringbell */

/**
 ** Quit()
 **/

void
#if	OlNeedFunctionPrototypes
Quit (
	void
)
#else
Quit ()
#endif
{
	XtDestroyApplicationContext (app);
	exit (0);
	/*NOTREACHED*/
} /* Quit */

/**
 ** do_select() - See actions.c
 **/

/*ARGSUSED*/
void
#if	OlNeedFunctionPrototypes
do_select (
	Time			time
)
#else
do_select (time)
	Time			time;
#endif
{
#if	FIX_ERRNO
	int save_errno = errno;
#endif
	/*
	 * MORE: Do this.
	 */
#if	FIX_ERRNO
	errno = save_errno;
#endif
	return;
} /* do_select */

static Widget
#if	OlNeedFunctionPrototypes
CreateIndicator (
	String			name,
	Widget			parent
)
#else
CreateIndicator (name, parent)
	String			name;
	Widget			parent;
#endif
{
	Widget			indicator = 0;

	struct str {
		String			string;
	}			str;

	static XtResource	resource = {
		XtNstring, XtCString,
		XtRString, sizeof(String), XtOffsetOf(struct str, string),
		XtRImmediate, (XtPointer)0
	};


	/*
	 * Probe the resource database to see if this widget needs
	 * to be created. We require the XtNstring resource to be set
	 * in the database, as a clue that this indicator is needed.
	 */
	XtGetSubresources (
		parent, &str, name, XtNstring,
		&resource, 1, (Arg *)0, (Cardinal)0
	);
	if (str.string)
		indicator = XtVaCreateManagedWidget(
			name, staticTextWidgetClass, parent,
			XtNconsumeEvent, (XtArgVal)&consume,
			(String)0
		);

	return (indicator);
} /* CreateIndicator */

/**
 ** Activate()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Activate (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data	/*NOTUSED*/
)
#else
Activate (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	String			s = (String)client_data;
	String			action;
	String			param;

	Cardinal		len;

	static String		copy = 0;

	static Cardinal		length = 0;


	/*
	 * Make a copy, since strtok is destructive.
	 */
	if (!(len = strlen(s)))
		return;
	if (len > length)
		copy = XtRealloc(copy, (length = len) + 1);
	strcpy (copy, s);

	action = strtok(copy, "(");
	param = strtok((String)0, ")");

	XtCallActionProc (LCD, action, (XEvent *)0, &param, 1);

	return;
} /* Activate */

/**
 ** Consume()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Consume (
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
)
#else
Consume (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlVirtualEventRec *	ve = (OlVirtualEventRec *)call_data;

#define win XtWindow(top)


	switch (ve->virtual_name) {
	case OL_ADJUST:
		/*
		 * The ADJUST button can do selections in a StaticText,
		 * if in OPEN LOOK mode.
		 */
		if (w != LCD || OlGetGui() != OL_OPENLOOK_GUI) {
			static Cursor		cursor = None;

			switch (ve->xevent->type) {
			case ButtonPress:
				if (cursor == None)
					cursor = OlGetQuestionCursor(top);
				XDefineCursor (dpy, win, cursor);
				break;
			case ButtonRelease:
				XUndefineCursor (dpy, win);
				break;
			}
			ve->consumed = True;
		}
		break;
	case OL_MENU:
		switch (ve->xevent->type) {
		case ButtonPress:
		case ButtonRelease:
			/*
			 * Give these to the window manager.
			 */
			OlReplayBtnEvent (top, (XtPointer)0, ve->xevent);
			break;
		}
		ve->consumed = True;
		break;
	}

#undef	win
	return;
} /* Consume */

/**
 ** CvtStringToPointer()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
CvtStringToPointer (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data	/*NOTUSED*/
)
#else
CvtStringToPointer (display, args, num_args, from, to, converter_data)
	Display *		display;
	XrmValue *		args;
	Cardinal *		num_args;
	XrmValue *		from;
	XrmValue *		to;
	XtPointer *		converter_data;
#endif
{
	String string = XtNewString((String)from->addr);
	ConversionDone (XtPointer, (XtPointer)string);
} /* CvtStringToPointer */
