/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xplaces:xplaces.c	1.3"
/*copyright	"%c%*/
/*
 * xplaces goes through the windows on the screen trying to determine the
 * command used to start each one up in the same state as it is when xplaces is
 * run. It doesn't quite meet such a stringent spec, thanks to complete
 * non-cooperation from clients, and marginal cooperation from window managers.
 * It will, however, give a window manager that obeys at least the WM_STATE
 * directive, come up with a reasonable guess at geometry, icon state, and
 * icon geometry. It outputs a Bourne shell script that may be used to
 * recreate the window setting.
 */
/*
 * Rewritten from scratch based on Ken Yap's manual page -- the old version
 * was just unable to cope with the ICCCM
 */
/*
 * It assumes that the rcmd script is available to run a command in the
 * background on a remote machine -- a script that works on systems with the
 * Berkeley remote shell (rsh) command is provided in thsi directory.
 */
/*
 * Note that the output of xplaces may need to be ordered by hand. xplaces is
 * a fairly low level tool -- shell/awk/perl/C commands should be wrapped
 * around it to match your environment.
 */
/*
 * !! Won't deal with applications that start up on one display/screen and
 * change to another...
 */
/*
 * !! It doesn't free any of the WM_COMMAND strings because it allows the
 * Option procs to modify argvp. This shouldn't be a problem -- if you have
 * that many commands around, chances are your X server will die first...
 */
#include	<stdio.h>
#include	<X11/Xlib.h>
#include	<X11/Xutil.h>
#include 	<X11/Xatom.h>
#include	<X11/Xos.h>

#ifndef ICCCM_XLIB
  /* If ICCCM_XLIB isn't defined, try to intuit whether we're ICCCM or not. */
# ifdef WithdrawnState
#  define ICCCM_XLIB
# endif
#endif

#ifndef MEMUTIL
extern char *malloc();
#endif /* MEMUTIL */

static Atom wmStateAtom;
static Window rootw;
static Display *d;
static int scr;
static Bool iconic;
static char hostname[256];
static char *progname = "xplaces";

/* forward declarations */
static XID *GetWMState();
static Bool argmatch();
static char *savestr();
static void dogeom();
static void doiconic();
static void printargs();
static int scanoptions();

typedef struct {
	int valid;
	int width, height, x, y;
} Geom;

static Geom geom;
static Geom igeom;

/* !! Wish the X toolkit would expand -g to -geometry, and so on */
/*
 * xplaces parsing only deals with xplaces style options. It uses the Option
 * struct and the scanoptions procedure to implement a fairly general way of
 * searching WM_COMMAND and changing them if necessary to match reality. Other
 * toolkits (eg) XView, have different ways of specifying geometry. The only
 * way I can see out of it is to have each toolkit indicate it's style of
 * options in a special property, and make xplaces have a different set of
 * Option structs for each toolkit's options style. The Option struct is a
 * descriptor that describes what to match and how to modify the options. We
 * scan through the arguments in WM_COMMAND looking for things that match
 * 'option', (these arguments must be a minimum of 'atleast' long to be
 * considered a hit -- see argmatch()) -- at least 'numargs' args are expected
 * after the option for a hit, and 'proc' is called as proc(char **argv, int
 * argc, int i, Option *option) where argv is the entire argv array, argc is
 * the number of elements in argv, i is the option in argv which matched, and
 * option is what triggered the match. The proc may modify the strings in argv
 * just so long as it does not increase their length, proc may change any of
 * the pointers in argv except argv[0] change any of the argv pointers.
 */
typedef struct {
	char *option;
	int atleast;
	int numargs;
	void (* proc)();
	char *arg;
} Option;

/*
 * At CSRI, we hack the toolkit to provide a -ig option. The -xrm stuff is too
 * much work.
 */
#ifdef CSRI
# define NICON 3
  static Option opt_ig = {"-igeometry", NICON, 1, dogeom, (char *) &igeom};
#else
# define NICON 2
  static Option opt_ig  = {"-xrm", 2, 1, dogeom, (char *) &igeom};
#endif

static Option opt_geom = {"-geometry", 2, 1, dogeom, (char *)  &geom};
static Option opt_icon = {"-iconic", NICON, 0, doiconic, (char *) &iconic};

#ifndef ICCCM_XLIB
/*
 * All this stuff is a (somewhat sloppy) implementation of some of the
 * proposed Xlib support for the ICCCM. Probably won't be needed in R4.
 */

typedef struct {
	unsigned char *value;	/* property data */
	Atom encoding;		/* type of property */
	int format;		/* 8, 16, or 32 */
	unsigned long nitems;	/* number of items in value */
} XTextProperty;
	
static Status
XGetTextProperty(dpy, w, text_prop_ret, property)
Display *dpy;
Window w;
XTextProperty *text_prop_ret;
Atom property;
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	Status status;
	unsigned char *propdata;

	text_prop_ret->value = NULL;
	text_prop_ret->encoding = None;
	text_prop_ret->format = 0;
	text_prop_ret->nitems = 0;
	
	status = XGetWindowProperty(dpy, w, property, 0L, 4096L, 
	 False, AnyPropertyType, &actual_type, &actual_format, 
	 &nitems, &bytes_after, &propdata);
	if (status == BadWindow) {
		(void) fprintf(stderr, "Window 0x%lx does not exist!\n", w);
		return 0;
	}
	if (status != Success) {
		(void) fprintf(stderr,
		 "XGetWindowProperty failed (status %d)!\n", status);
		return 0;
	}
	if (actual_type == None) {
		return 0;
	}
	if (propdata == NULL) {
		(void) fprintf(stderr, "NULL property data!\n");
		return 0;
	}
	if (bytes_after != 0) {
		/* !! Sleazy! We should loop and get it in chunks */
		(void) fprintf(stderr,
		 "Only got the first %d items. %d bytes remain\n", 
		 nitems, bytes_after);
	}
	text_prop_ret->value = propdata;
	text_prop_ret->encoding = actual_type;
	text_prop_ret->format = actual_format;
	text_prop_ret->nitems = nitems;
	return 1;
}
	
static Status
XGetCommand(dpy, w, argvp, argcp)
Display *dpy;
Window w;
char ***argvp;
int *argcp;
{
	int i, len;
	char **argv;
	char *cp;
	XTextProperty tp;
	Status status;
	
	status = XGetTextProperty(dpy, w, &tp, XA_WM_COMMAND);
	if (status == 0 || tp.value == NULL)
		return 0;
	for(i = 0, len = 0, cp = (char *) tp.value; i < tp.nitems; i++, cp++)
		if (*cp == '\0')
			len++;
	if (len == 0) {
		XFree((char *) tp.value);
		(void) fprintf(stderr, "No data in WM_COMMAND\n");
		return 0;
	}
	if ((argv = (char **) malloc(sizeof(char *) * len)) == NULL) {
		XFree((char *) tp.value);
		(void) fprintf(stderr, "Not enough memory for string list\n");
		return 0;
	}
	i = 0;
	cp = (char *) tp.value;
	do {
		argv[i++] = cp;
		while(*cp++ != '\0')
			;
	} while (i < len);
	*argcp = len;
	*argvp = argv;
	return 1;
}

static Status
XGetWMClientMachine(dpy, w, text_prop_ret)
Display *dpy;
Window w;
XTextProperty *text_prop_ret;
{
	return XGetTextProperty(dpy, w, text_prop_ret, XA_WM_CLIENT_MACHINE);
}

static Status
XGetWMName(dpy, w, text_prop_ret)
Display *dpy;
Window w;
XTextProperty *text_prop_ret;
{
	return XGetTextProperty(dpy, w, text_prop_ret, XA_WM_NAME);
}

static Status
XGetWMIconName(dpy, w, text_prop_ret)
Display *dpy;
Window w;
XTextProperty *text_prop_ret;
{
	return XGetTextProperty(dpy, w, text_prop_ret, XA_WM_ICON_NAME);
}

static void
XFreeStringList(list)
char **list;
{
	XFree(*list);
	free((char *) list);
}

#endif /* ICCCM_XLIB */

void
fatal(message)
char *message;
{
	(void) fprintf(stderr, "%s: %s\n", progname, message);
	exit(1);
	/*NOTREACHED*/
}

void
usage()
{
	fatal("Usage: xplaces [-display displayname] [-id WindowId]\n");
	/*NOTREACHED*/
}

/*
 *	FILE # xplaces.c
 *	Usage includes the id option now.
 *	ENDCHANGE # UNKNOWN
 */
static char *
nextarg(i, argv, argc)
int i;
char **argv;
int argc;
{
	if (++i >= argc) {
		(void) fprintf(stderr, "%s: %s must be followed by argument\n",
		 progname, argv[i-1]);
		usage();
	}
	return argv[i];
}

int
main(argc, argv)
int argc;
char *argv[];
{
	char *dpyname = NULL;
	int i;
	extern char *getenv();
	extern long atol();
	unsigned int nchildren;
	Window parent_win, *child_list;
	Window topwin = None;
	
	for(i = 1; i < argc;) {
		if (argmatch(argv[i], "-display", 2)) {
			dpyname = nextarg(i, argv, argc);
			i += 2;
		} else if (argmatch(argv[i], "-id", 3)) {
			char *cp = nextarg(i, argv, argc);

			if (strncmp(cp, "0x", 2) == 0) {
				(void) sscanf(cp+2, "%x", &topwin);
			} else {
				(void) sscanf(cp, "%d", &topwin);
			}
			i += 2;
		} else {
			(void) fprintf(stderr, "%s: Bad option -- %s\n",
			 progname, argv[i]);
			usage();
			/*NOTREACHED*/
		}
	}
	if ((d = XOpenDisplay(dpyname)) == NULL) {
		if (dpyname == NULL)
			dpyname = getenv("DISPLAY");
		if (dpyname == NULL)
			dpyname = "(NULL)";
		(void) fprintf(stderr, "Can't open display %s\n", dpyname);
		exit(-1);
		/*NOTREACHED*/
	}
   	(void) XmuGetHostname(hostname, sizeof(hostname));
	(void) printf("DISPLAY=%s; export DISPLAY\n", DisplayString(d));
	wmStateAtom = XInternAtom (d, "WM_STATE", False);
	scr = DefaultScreen(d);
	if (topwin == None)
		topwin = RootWindow(d, scr);
	if (XQueryTree(d, topwin, &rootw, &parent_win, &child_list,
	 &nchildren) == 0) {
		(void) fprintf(stderr,
		 "%s: Couldn't get children for top window 0x%lx\n", progname,
		 topwin);
		exit(-1);
		/*NOTREACHED*/
	}
	/* scan list */

/*
 *	FILE # xplaces.c
 *	If a window id is specified, the search should begin with that window itself
 *	and not from it's children.
 *	ENDCHANGE # UNKNOWN
 */
	if(topwin!= RootWindow(d,scr))
		(void)searchtree(parent_win,topwin);
	else
	for ( ; nchildren-- > 0; child_list++)
		(void) searchtree(topwin, *child_list);
	return 0;
}

/*
 * Searchtree returns non-zero if w, or one of w's descendants was found
 * carrying a WM_COMMAND property. It attempts to find all top level windows
 * that are descendants of w, and print the command strings for them if they
 * have a WM_COMMAND property. Non-conformant low-down weaselly windows that
 * don't set WM_COMMAND for their top level windows are ignored. (Hmm, might
 * be a better idea to just XKillClient them:-) If a WM_COMMAND property was
 * found on a toplevel window, it prints out the command string for that
 * window and returns. A toplevel window is a window which has
 * override_redirect set to false, and has either WM_STATE set, or is a mapped
 * child of the root window with no descendants that have WM_STATE set,
 * according to Section 5.1.1.3 of the X Penal Code (er, the ICCCM, that is)
 * Sheesh!
 */
int
searchtree(parent, w)
Window parent;
Window w;
{
	unsigned int nchildren;
	int ret = 0;
	int donegeometry = 0;
	int doneigeom = 0;
	int doneiconic = 0;
	int gotcommand;
	int argc;
	Window root_win, parent_win, *child_list, junk;
	XWindowAttributes win_info;
	XSizeHints hints;
	XID *state;
	XTextProperty tp;
	char **argv;

	/* what do we know about the window? */
	if (!XGetWindowAttributes(d, w, &win_info)) {
		(void) fprintf(stderr,
		 "XGetWindowAttributes failed on window 0x%lx", w);
		return 0;
	}

	if (win_info.override_redirect)
		return 0;	/* ICCCM 5.1.1.3 */

	state = GetWMState(w);
	/*
	 * If we get here, then we're a top level window. Question is, are we
	 * the top level window with the WM_COMMAND? We only return non-zero
	 * if we're a WM_COMMAND type... Ideally, we should try for a command
	 * only if WM_STATE is set. Alas, with older non-ICCCM-conformant
	 * window managers still in existence, WM_STATE may not be set, but
	 * WM_COMMAND might be.
	 */
	gotcommand = XGetCommand(d, w, &argv, &argc);
	
	if (state == 0 && gotcommand == 0) {
		/*
		 * We don't have a WM_STATE property, maybe one of our kids
		 * does, so recurse first */
		if (XQueryTree(d, w, &root_win, &parent_win,
		 &child_list, &nchildren) == 0)
			return 0;
		
		for ( ; nchildren-- > 0; child_list++) {
			ret = searchtree(w, *child_list);
			if (ret != 0) {
				/*
				 * One of our children is a toplevel window
				 * with WM_COMMAND.  Good for them -- we can't
				 * be one, nor can any of our other children
				 * have the WM_COMMAND even if they have
				 * WM_STATE.
				 */
				return ret;
			}
		}
		/* We don't have a WM_STATE, none of the kids does */
		/* Are we a Mapped root window descendant? */
		if (parent == rootw && win_info.map_state != IsUnmapped)
			/* yep, so we're top level */
			ret++;
		else
			/* nope. keep looking */
			return 0;
	} else {
		/* We have a WM_STATE or a WM_COMMAND. We're top level */
		ret++;
	}

	/*
	 * Ideally, we'd get the command here instead of earlier, but it ain't
	 * an ICCCM world yet... We print a warning if we don't get a
	 * WM_COMMAND from a top level window.
	 */
	if (gotcommand == 0) {
		XTextProperty text_prop;
		Status status;
		char *wm_name;
		char *wm_class;
		char *wm_client_machine;

		status = XGetTextProperty(d, w, &text_prop,
		 XA_WM_NAME);
		if (status == 0 || text_prop.value == NULL)
			wm_name = "";
		else
			wm_name = (char *) text_prop.value;
		status = XGetTextProperty(d, w, &text_prop,
		 XA_WM_CLASS);
		if (status == 0 || text_prop.value == NULL)
			wm_class = "";
		else
			wm_class = (char *) text_prop.value;

		status =  XGetTextProperty(d, w, &text_prop,
		 XA_WM_CLIENT_MACHINE);
		if (status == 0 || text_prop.value == NULL)
			wm_client_machine = "";
		else
			wm_client_machine = (char *) text_prop.value;

		(void) fprintf(stderr,
		 "%s: Top level window 0x%lx has no WM_COMMAND string\n",
		 progname, w);
		(void) fprintf(stderr,
		 "\t\t(name \"%s\", class \"%s\", client machine \"%s\")\n",
		 wm_name, wm_class, wm_client_machine);
		return 0;
	}

	/* Ok. We have a WM_COMMAND. Let's get the size hints */
	/* !! Should use XGetWMNormalHints? */
	if (!XGetNormalHints(d, w, &hints)) {
		(void) fprintf(stderr,
		 "XGetNormalHints on Window 0x%lx failed.\n", w);
		hints.flags = 0;	/* can't give up now...*/
	}

	/* if no increment then assume 1 */
	if (!(hints.flags & PResizeInc))
		hints.width_inc = hints.height_inc = 1;

	/* if no position then assume +0+0 */
	if (!(hints.flags & (USPosition || PPosition)))
		hints.x = hints.y = 0;

	/* if no min width or height then assume 0 */
	if (!(hints.flags & PMinSize))
		hints.min_width = hints.min_height = 0;

	/*
	 * terminal windows sizes are in characters so this will compute the
	 * geometry correctly. Use the win_info stuff, not the hints, since
	 * almost no window manager or client updates the hints. Grump.
	 */
	if (hints.width_inc > 1 && hints.height_inc > 1) {
		geom.width = (win_info.width - hints.min_width) /
			hints.width_inc;
		geom.height = (win_info.height - hints.min_height) /
			hints.height_inc;
	} else {
		geom.width = win_info.width;
		geom.height = win_info.height;
	}
	geom.valid = True;
	if (! XTranslateCoordinates(d, w, RootWindow(d, scr),
	 win_info.x, win_info.y, &(geom.x), &(geom.y), &junk)) {
		/*
		 * only supposed to fail if w and RootWindow
		 * are on different screens. Can't happen!
		 */
		(void) fprintf(stderr,
		 "XTranslateCoordinates on Window 0x%lx failed!\n", w);
		geom.x = geom.y = 0;
	}
	
	if (XGetWMClientMachine(d, w, &tp)) {
		if (strcmp((char *) tp.value, hostname) != 0) {
			(void) printf("rcmd %s ", (char *) tp.value);
		}
		XFree((char *) tp.value);
	}
	/*
	 * Finally, there's the problem of state - iconic, etc? And
	 * icon geometry. We flagrantly violate the ICCCM and rely on
	 * WM_STATE containing a CARD32 with the state, and a WINDOW
	 * with the icon window. (well, we have the weak excuse that
	 * xplaces is a poor man's session manager:-)
	 */
	iconic = False;
	igeom.valid = False;
	if (state != 0) {
		if (state[0] == IconicState) {
			iconic = True;
		}
		if (state[1] != None) {
			if (!XGetWindowAttributes(d,
			 (Window) state[1], &win_info)) {
			    (void) fprintf(stderr,
			     "XGetWindowAttr. failed on window 0x%lx",
			     state[1]);
			} else {
			    igeom.valid = True;
			    igeom.width = win_info.width;
			    igeom.height = win_info.height;
			    igeom.x = win_info.x;
			    igeom.y = win_info.y;
			}
		}
	}
	/*
	 * Now we have all the info we need. We first call scanoptions
	 * to substitute any arguments in WM_COMMAND that may have
	 * changed. We don't perform this substitution in the actual
	 * WM_COMMAND properties, attractive as that idea seems.
	 */
	donegeometry = scanoptions(argv, argc, &opt_geom);
	doneigeom = scanoptions(argv, argc, &opt_ig);
	doneiconic = scanoptions(argv, argc, &opt_icon);
	/*
	 * scanoptions doesn't add the options to the end if they
	 * don't exist. (!! Maybe it should? We should generalize the
	 * Option structs into a table, have scanoptions accept that
	 * table, then we could get rid of the multiple calls to
	 * scanoptions and remove all the straight line code below for
	 * adding options in) We do it by hand here for each option.
	 */
	(void) printf("%s ", argv[0]);
	/* Safest to print options immediately after argv[0] */
	if (!donegeometry) {
		char *targv[2];
		int targc;
		
		targv[0] = opt_geom.option;
		targv[1] = "";
		targc = 2;
		dogeom(targv, targc, 0, &opt_geom);
		printargs(targv, targc);
	}
	if (!doneigeom) {
		char *targv[2];
		int targc;
		
		targv[0] = opt_ig.option;
		targv[1] = "";
		targc = 2;
		dogeom(targv, targc, 0, &opt_ig);
		printargs(targv, targc);
	}
	if (!doneiconic) {
		char *targv[2];
		int targc;
		
		targv[0] = opt_icon.option;
		targc = 1;
		doiconic(targv, targc, 0, &opt_icon);
		printargs(targv, targc);
	}
	/* print the rest of the arguments */
	printargs(&argv[1], argc-1);
	putc('\n', stdout);
	fflush(stdout);
	return ret;
}

static XID *
GetWMState(w)
Window w;
{
	int status;
	XTextProperty tp;

	status = XGetTextProperty(d, w, &tp, wmStateAtom);
	if (status == 0)
		return 0;
	if (tp.nitems != 2) {
		(void) fprintf(stderr,
		 "Got %d items for WM_STATE;expected 2\n", tp.nitems);
	}
	return (XID *) tp.value;
}

/*
 * Returns true if s1 and s2 match for the length of s1. s1 must be at least
 * 'atleast' characters long. s1 may at most be as long as s2. Therefore,
 * argmatch("-", "-geometry", 2) will return False, argmatch("-g",
 * "-geometry", 2) will return True, argmatch("-geometry", "-geometry", 2)
 * will return True, and argmatch("-geometryx", "-geometry", 2) will return
 * False.
 */
static Bool
argmatch(s1, s2, atleast)
char *s1;
char *s2;
int atleast;
{
	int l1 = strlen(s1);
	int l2 = strlen(s2);

	if (l1 < atleast)
		return False;
	if (l1 > l2)
		return False;
	return (strncmp(s1, s2, l1) == 0);
}

static char *
savestr(s)
char *s;
{
	char *cp = malloc((unsigned int) (strlen(s) + 1));

	if (cp == NULL)
		fatal("Out of memory\n");
	return strcpy(cp, s);
}


/*ARGSUSED*/
static void
dogeom(argv, argc, i, option)
char **argv;
int argc;
int i;
Option *option;
{
	Geom *g = (Geom *) option->arg;
	char tmpstr[256]; /* big enough to hold a large geometry... */

	if (! g->valid) {
		argv[i] = NULL;
		argv[i+1] = NULL;
		return;
	}
	if (strcmp(option->option, "-xrm") == 0) {
		(void) sprintf(tmpstr, "\"*iconGeometry: %dx%d+%d+%d\"",
		 g->width, g->height, g->x, g->y);
	} else {
		(void) sprintf(tmpstr, "%dx%d+%d+%d", g->width, g->height,
		 g->x, g->y);
	}
	argv[i+1] = savestr(tmpstr);
}

/*ARGSUSED*/
static void
doiconic(argv, argc, i, option)
char **argv;
int argc;
int i;
Option *option;
{
	int is_iconic = *((int *) option->arg);

	if (is_iconic)
		return;
	else
		argv[i] = NULL;
}

static void
printargs(argv, argc)
char **argv;
int argc;
{
	int i;

	for(i = 0; i < argc; i++) {
		if (argv[i] != NULL) {
			(void) printf("%s ", argv[i]);
		}
	}
}

static int
scanoptions(argv, argc, option)
char **argv;
int argc;
Option *option;
{
	int i;
	int matched = 0;

	/* never scan argv[0] */
	for(i = 1; i < argc; i++) {
		if (argv[i] == NULL)
			continue;
		if (argmatch(argv[i], option->option, option->atleast)) {
			(*(option->proc))(argv, argc, i, option);
			matched++;
		}
	}
	return matched;
}
