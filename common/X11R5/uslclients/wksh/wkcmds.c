/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:wkcmds.c	1.6"

/* X includes */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#define malloc do_not_redeclare_malloc
#include "name.h"
#undef malloc
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include "wksh.h"


#define MAXARGS 128
#define SLISTITEMSIZE	16

int ConvertStringToType(), ConvertTypeToString();
Widget Toplevel;
void Translation_ksh_eval(), stdCB(), stdInputCB(), stdTimerCB();

static const XtActionsRec Ksh_actions[] = {
	{ "ksh_eval",	Translation_ksh_eval }
};

extern void (*toolkit_addcallback)(), (*toolkit_callcallbacks)();

#ifndef MEMUTIL
extern char *strdup();
#endif /* MEMUTIL */
extern char e_colon[];

#ifdef MOOLIT
#define str_XtRString XtRString
#else
static const char str_XtRString[] = XtRString;
#endif

const char str_wksh[] = "wksh";
const char str_s_eq_s[] = "%s=%s";
const char str_s_eq[] = "%s=";
const char str_nill[] = "";
const char str_bad_argument_s[] = "Bad argument: ";
const char str_unknown_pointer_type[] = "Unknown pointer type";
const char str_Pointer[] = "Pointer";
const char str_cannot_parse_s[] = "Cannot parse value, unmatched parentheses, skipped";
const char str_0123456789[] = "0123456789";
const char str_left_over_points_ignored[] = "Draw: left over points ignored";
const char str_polygon_not_supported[] = "Draw: POLYGON not yet supported";
const char str_opt_rect[] = "-rect";
const char str_opt_fillrect[] = "-fillrect";
const char str_opt_fillpoly[] = "-fillpoly";
const char str_opt_line[] = "-line";
const char str_opt_text[] = "-text";
const char str_opt_arc[] = "-arc";
const char str_opt_fillarc[] = "-fillarc";
const char str_opt_point[] = "-point";
const char usage_draw[] = "usage: Draw -<object> handle [args ...]";

/*
 * simply do a sprintf and return a static buffer, a convenient way
 * to format usage messages.
 */

char *
usagemsg(fmt, cmd)
char *fmt, *cmd;
{
	static char out[256];

	sprintf(out, fmt, cmd);
	return(out);
}

static void
wtab_destroy(w, wtab, callData)
Widget w;
wtab_t *wtab;
caddr_t callData; /* not used */
{
	extern int Wtab_free;

	XtFree(wtab->wname);
	XtFree(wtab->widid);
	XtFree(wtab->envar);
	if (wtab->info)
		XtFree(wtab->info);
	wtab->type = TAB_EMPTY;
	Wtab_free++;
}

wtab_t *
set_up_w(wid, parent, var, name, class)
Widget wid;
wtab_t *parent;
char *var, *name;
classtab_t *class;
{
	char widid[8];
	static wtab_t *w;

	get_new_wtab(&w, widid);
	if (var) {
		env_set_var(var, widid);
		w->envar = strdup(var);
	} else {
		w->envar = strdup("none");
	}
	w->type = TAB_WIDGET;
	w->wname = name ? strdup(name) : strdup(CONSTCHAR "anonymous");
	w->wclass = class;
	w->parent = parent;
	w->widid = strdup(widid);
	w->w = wid;
	w->info = NULL;
	(*toolkit_addcallback)(wid, XtNdestroyCallback, wtab_destroy, (caddr_t)w);
	return(w);
}

static short Needfree[MAXARGS];

void
parse_args(arg0, argc, argv, w, parent, class, n, args)
char *arg0;
int argc;
char **argv;
wtab_t *w;
wtab_t *parent;
classtab_t *class;
int  *n;
Arg *args;
{
	register int i;
	register char *colon, *resource, *val, *p;
	XtArgVal argval, flatargval;
	int freeflag, len;
	char *flatres;

	*n = 0;
	for (i = 0; i < argc; i++) {
		if (i == MAXARGS) {
			printerr(arg0, CONSTCHAR "Too many arguments, skipping:", argv[*n]);
			continue;
		}
		if ((colon = strchr(argv[i], ':')) == NULL && (colon = strchr(argv[i], '=')) == NULL) {
			printerr(arg0, CONSTCHAR "Bad resource, should be of form 'name:value' : ", argv[i]);
			continue;
		}
		if (*colon == '=') {
			printerr(arg0, CONSTCHAR "WARNING: resource separator is ':' but you used '=' : ", argv[i]);
		}
		val = &colon[1];
		len = colon - argv[i];
		resource = XtMalloc(len + 1);
		strncpy(resource, argv[i], len);
		resource[len] = '\0';
		flatres = NULL;
		if (ConvertStringToType(arg0, w, parent, class, resource, val, &argval, &flatargval, &flatres, &freeflag, !i) != FAIL) {
			XtSetArg(args[*n], resource, argval);
			Needfree[*n] = freeflag;
			(*n)++;
			/* Necessary hack for flats */
			if (flatres != NULL) {
				XtSetArg(args[*n], flatres, flatargval);
				Needfree[*n] = FALSE;
				(*n)++;
			}
		}
	}
}

void
free_args(n, args)
int n;
Arg *args;
{
	register int i;

	/*
	 * Free up argument pointers
	 */
	for (i = 0; i < n; i++) {
		XtFree(args[i].name);
		if (Needfree[i]) {
			XtFree((String)args[i].value);
		}
	}
}

do_XtAppInitialize(argc, argv)
int argc;
char *argv[];
{
	int ret;
	char *wkdb_hook, *env_get();

	ret = toolkit_initialize(argc, argv);
	XtAddActions((XtActionList)Ksh_actions, XtNumber(Ksh_actions));
	if ((wkdb_hook = env_get(CONSTCHAR "WKDB_HOOK")) != NULL) {
		ksh_eval(wkdb_hook);
	}
	return(ret);
}

static Widget
WkshCreateShell(name, class, parent, args, nargs)
String name;
WidgetClass class;
Widget parent;
ArgList args;
Cardinal nargs;
{
	return(XtCreateApplicationShell(name, class, args, nargs));
}

do_XtAppCreateShell(argc, argv)
int argc;
char *argv[];
{
	return(_CreateWidget(WkshCreateShell, argc, argv));
}

do_XtCreatePopupShell(argc, argv)
int argc;
char *argv[];
{
	return(_CreateWidget(XtCreatePopupShell, argc, argv));
}

do_XtCreateManagedWidget(argc, argv)
int argc;
char *argv[];
{
	return(_CreateWidget(XtCreateManagedWidget, argc, argv));
}

do_XtCreateWidget(argc, argv)
int argc;
char *argv[];
{
	return(_CreateWidget(XtCreateWidget, argc, argv));
}

static int
_CreateWidget(func, argc, argv)
Widget (*func)();
int argc;
char *argv[];
{
	Widget widget;
	classtab_t *class;
	char *arg0 = argv[0];
	wtab_t *w, *pw, *wtab;
	char *wname, *wclass, *parentid, *var;
	Arg	args[MAXARGS];
	register int	i;
	int n;

	if (argc < 5) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s var name class parent [arg:val ...]\n", argv[0]), NULL);
		return(1);
	}
	var = argv[1];
	wname = argv[2];
	wclass = argv[3];
	parentid = argv[4];
	pw = str_to_wtab(argv[0], parentid);
	if (pw == NULL) {
		printerr(argv[0], CONSTCHAR "Could not find parent\n", NULL);
		return(1);
	}
	argv += 5;
	argc -= 5;
	if ((class = str_to_class(arg0, wclass)) == NULL) {
		return(1);
	}
	n = 0;
	parse_args(arg0, argc, argv, NULL, pw, class, &n, args);
	widget = func(wname, class->class, pw->w, args, n);
	if (widget != NULL) {
		wtab = set_up_w(widget, pw, var, wname, class);

		if (class->initfn)
			class->initfn(arg0, wtab, var);
	} else {
		printerr(argv[0], usagemsg(CONSTCHAR "Creation of widget '%s' failed\n", wname), NULL);
		env_blank(argv[1]);
	}
	free_args(n, args);

	return(0);
}

do_XtPopup(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	XtGrabKind grab;

	if (argc < 2 || argc > 3) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid [GrabNone|GrabNonexclusive|GrabExclusive]\n", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	} else {
		grab = XtGrabNone;
		if (argc < 3 || strcmp(argv[2], CONSTCHAR "GrabNone") == 0) {
			grab = XtGrabNone;
		} else if (strcmp(argv[2], CONSTCHAR "GrabNonexclusive") == 0) {
			grab = XtGrabNonexclusive;
		} else if (strcmp(argv[2], CONSTCHAR "GrabExclusive") == 0) {
			grab = XtGrabExclusive;
		} else {
			printerr(argv[0], CONSTCHAR "Warning: Grab type unknown, using GrabNone", NULL);
			grab = XtGrabNone;
		}
		XtPopup(w->w, grab);
	}
	return(0);
}

int
do_XtDestroyWidget(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_arg_func(XtDestroyWidget, argc, argv));
}

static int
do_single_widget_arg_func(func, argc, argv)
int (*func)();
int argc;
char **argv;
{
	wtab_t *w;
	register int i;

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid ...\n", argv[0]), NULL);
		return(1);
	}
	for (i = 1; i < argc; i++) {
		w = str_to_wtab(argv[0], argv[i]);
		if (w != NULL) {
			func(w->w);
		}
	}
	return(0);
}

static int
do_single_widget_test_func(func, argc, argv)
int (*func)();
int argc;
char **argv;
{
	wtab_t *w;
	register int i;

	if (argc != 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid\n", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w != NULL) {
		return(!func(w->w));
	}
	return(255);
}

int
do_XtIsSensitive(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_test_func(XtIsSensitive, argc, argv));
}

int
do_XtIsManaged(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_test_func(XtIsManaged, argc, argv));
}

int
do_XtIsRealized(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_test_func(XtIsRealized, argc, argv));
}

int
do_XtRealizeWidget(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_arg_func(XtRealizeWidget, argc, argv));
}

int
do_XtUnrealizeWidget(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_arg_func(XtUnrealizeWidget, argc, argv));
}

/*
 * XtMapWidget() is a macro, so can't use do_single_widget_arg_func()
 */

int
do_XtMapWidget(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	register int i;

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid ...\n", argv[0]));
		return(1);
	}
	for (i = 1; i < argc; i++) {
		w = str_to_wtab(argv[0], argv[i]);
		if (w != NULL) {
			XtMapWidget(w->w);
		}
	}
	return(0);
}

int
do_XtUnmapWidget(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	register int i;

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid ...\n", argv[0]), NULL);
		return(1);
	}
	for (i = 1; i < argc; i++) {
		w = str_to_wtab(argv[0], argv[i]);
		if (w != NULL) {
			XtUnmapWidget(w->w);
		}
	}
	return(0);
}

do_XtPopdown(argc, argv)
int argc;
char **argv;
{
	return(do_single_widget_arg_func(XtPopdown, argc, argv));
}

static jmp_buf MainLoopJump;

static void
mainloopsighandler(sig)
int sig;
{
	longjmp(MainLoopJump, 1);
}

do_XtMainLoop(argc, argv)
int argc;
char **argv;
{
	extern void sh_fault();
	void (*old)();
	/*
	 * This will continue until the user hits the <DEL> key
	 */

	if (setjmp(MainLoopJump) == 1) {
		signal(SIGINT, sh_fault);
		sh_fault(SIGINT);
		return(1);
	}
	old = signal(SIGINT, mainloopsighandler);
	XtMainLoop();
	signal(SIGINT, old);
	printerr(argv[0], CONSTCHAR "MainLoop failed\n", NULL);
	return(1);
}

static int
XtAddCallback_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-n] widget callbackname ksh-command", arg0), NULL);
	return(1);
}

static int
XtCallCallbacks_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s Widgetid Callbackname", arg0), NULL);
	return(1);
}


int
do_XtCallCallbacks(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	char *arg0 = argv[0];

	if (argc != 3) {
		return(XtCallCallbacks_usage(arg0));
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	} else {
		(*toolkit_callcallbacks)(w->w, argv[2], NULL);
	}
	return(0);
}

int
do_XtAddCallback(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	int nocalldata = 0;
	char *arg0 = argv[0];

	if (argc != 4 && argc != 5) {
		return(XtAddCallback_usage(arg0));
	}
	if (argc == 5) {
		if (strcmp(argv[1], CONSTCHAR "-n") == 0) {
			nocalldata++;
			argv++;
			argc--;
		} else
			return(XtAddCallback_usage(arg0));
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	} else {
		wksh_client_data_t *cdata;
		const callback_tab_t *cbtab;

		cdata = (wksh_client_data_t *)XtMalloc(sizeof(wksh_client_data_t));
		cdata->ksh_cmd = strdup(argv[3]);
		cdata->w = w;

		if (nocalldata) {
			cbtab = NULL;
		} else {
			for (cbtab = w->wclass->cbtab; cbtab != NULL && cbtab->resname != NULL; cbtab++) {
				if (strcmp(cbtab->resname, argv[2]) == 0)
					break;
			}
		}
		if (cbtab != NULL && cbtab->resname != NULL)
			cdata->cbtab = cbtab;
		else
			cdata->cbtab = NULL;
		(*toolkit_addcallback)(w->w, argv[2], stdCB, (XtPointer)cdata);
	}
	return(0);
}

int
do_XtGetValues(argc, argv)
int argc;
char **argv;
{
	register int i, j;
	int n;
	char *arg0 = argv[0];
	char *val, *p, *str;
	Arg args[MAXARGS];
	char *envar[MAXARGS];
	wtab_t *w;

	if (argc < 3) {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s Widgetid resource:envar ...", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	argv += 2;
	argc -= 2;
	if (w == NULL) {
		return(1);
	}
	/*
	 * Arguments are of the form:
	 *
	 *     resource:envar
	 */

	for (i = 0, n = 0; i < argc; i++) {
		if ((p = strchr(argv[i], ':')) == NULL) {
			printerr(arg0, CONSTCHAR "Bad argument: ", argv[i]);
			continue;
		}
		*p = '\0';
		args[n].name = strdup(argv[n]);
		envar[n] = &p[1];
		*p = ':';
		args[n].value = (XtArgVal)XtMalloc(256);
		n++;
	}
	XtGetValues(w->w, args, n);
	for (i = 0; i < n; i++) {
		if (ConvertTypeToString(arg0, w->wclass, w, w->parent, args[i].name, args[i].value, &str) != FAIL) {
			env_set_var(envar[i], str);
		}
		XtFree(args[i].name);
		XtFree((String)args[i].value);
	}
	return(0);
}

int
do_XtSetValues(argc, argv)
int argc;
char **argv;
{
	int n;
	char *arg0 = argv[0];
	Arg args[MAXARGS];
	wtab_t *w;

	if (argc < 3) {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s Widgetid arg:val ...", arg0), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	argv += 2;
	argc -= 2;
	if (w == NULL) {
		return(1);
	} else {
		n = 0;
		parse_args(arg0, argc, argv, w, w->parent, w->wclass, &n, args);
		if (n > 0) {
			XtSetValues(w->w, args, n);
			free_args(n, args);
		} else {
			printerr(arg0, CONSTCHAR "Nothing to set", NULL);
			return(1);
		}
	}
	return(0);
}

static void
parse_indexlist(argc, argv, max, indices, numindices)
int argc;
char *argv[];
int max;
int *indices;
int *numindices;
{
	register int i;

	*numindices = 0;
	for (i = 0; i < argc; i++) {
		if (!isdigit(argv[i][0])) {
			continue;
		}
		indices[*numindices] = atoi(argv[i]);
		if (indices[*numindices] < 0 || indices[*numindices] > max) {
			continue;
		}
		(*numindices)++;
	}
}

int
do_XtAddTimeOut(argc, argv)
int argc;
int **argv;
{
	unsigned long milliseconds = 0;
	wtab_t *w;
	char *cmd;

	if (argc != 4) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s toplevel milliseconds command", argv[0]), NULL);
		return(1);
	}

	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	}
	if ((milliseconds = atoi(argv[2])) <= 0) {
		printerr(argv[0], CONSTCHAR "Milliseconds must be greater than zero", NULL);
		return(1);
	}
	cmd = strdup((char *)argv[3]);
	(void)XtAddTimeOut(milliseconds, stdTimerCB, (XtPointer)cmd);
	return(0);
}

do_XtUnmanageChildren(argc, argv)
int argc;
char *argv[];
{
	return(do_managelist_func(argc, argv, XtUnmanageChildren));
}

do_XtManageChildren(argc, argv)
int argc;
char *argv[];
{
	return(do_managelist_func(argc, argv, XtManageChildren));
}

int
do_managelist_func(argc, argv, func)
int argc;
char *argv[];
int (*func)();
{
	wtab_t *w;
	register int i;
	Widget widgets[MAXARGS];
	Cardinal nwidgets;

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid ...\n", argv[0]), NULL);
		return(1);
	}
	for (nwidgets = 0, i = 1; i < argc && nwidgets < MAXARGS; i++) {
		w = str_to_wtab(argv[0], argv[i]);
		if (w != NULL) {
			widgets[nwidgets++] = w->w;
		}
	}
	func(widgets, nwidgets);
	return(0);
}

#define PARSE_POINTLIST (-1)

GC Standard_GC;

int
create_standard_gc(toplevel)
Widget toplevel;
{
	Standard_GC = XCreateGC(XtDisplay(toplevel), XtWindow(toplevel), 0, NULL);
	return(0);
}

do_XBell(argc, argv)
int argc;
char *argv[];
{
	int volume;

	if (Toplevel == NULL) {
		printerr(argv[0], CONSTCHAR "can't ring bell: toolkit not initialized.", NULL);
		return(1);
	}
	if (argc == 1) {
		volume = 0;
	} else if (argc > 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "Usage: %s volume (volume must be between -100 and 100", argv[0]), NULL);
	} else
		volume = atoi(argv[2]);
	if (volume < -100)
		volume = -100;
	else if (volume > 100)
		volume = 100;

	XBell(XtDisplay(Toplevel), volume);
	return(0);
}

int
do_XtRemoveAllCallbacks(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	register int i;

	if (argc < 3) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid callbackname\n", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w != NULL) {
		XtRemoveAllCallbacks(w->w, argv[2]);
		return(0);
	} else
		return(1);
}

int
do_XDraw(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	char *arg0 = argv[0];
	register int i;
	int mode, parse;
	int (*func)();
	GC  gc = NULL;
#define MAXDRAWARGS 6
	int p[MAXDRAWARGS];
	int XDrawRectangle(), XFillRectangle(), XFillPolygon(),
		XDrawLine(), XDrawText(), XDrawSegment(), XDrawArc(),
		XFillArc(), XDrawPoint();

	if (argc < 5) {
		printerr(argv[0], usage_draw, NULL);
		return(1);
	}
	if (strcmp(argv[1], str_opt_rect) == 0) {
		parse = 4;
		func = XDrawRectangle;
	} else if (strcmp(argv[1], str_opt_fillrect) == 0) {
		parse = 2;
		func = XFillRectangle;
	} else if (strcmp(argv[1], str_opt_fillpoly) == 0) {
		parse = PARSE_POINTLIST;
		func = XFillPolygon;
	} else if (strcmp(argv[1], str_opt_line) == 0) {
		parse = 4;
		func = XDrawLine;
	} else if (strcmp(argv[1], str_opt_text) == 0) {
		parse = 3;
		func = XDrawText;
	/***********************************************
	} else if (strcmp(argv[1], "-segment") == 0) {
		parse = 2;
		func = XDrawSegment;
	***********************************************/
	} else if (strcmp(argv[1], str_opt_arc) == 0) {
		parse = 6;
		func = XDrawArc;
	} else if (strcmp(argv[1], str_opt_fillarc) == 0) {
		parse = 6;
		func = XFillArc;
	} else if (strcmp(argv[1], str_opt_point) == 0) {
		parse = 2;
		func = XDrawPoint;
	}
	if ((w = str_to_wtab(argv[0], argv[2])) == NULL)
		return(1);
	if (Standard_GC == NULL)
		create_standard_gc(w->w);
	if (gc == NULL)
		gc = Standard_GC;
	argc -= 3;
	argv += 3;
	if (parse == PARSE_POINTLIST) {
		printerr(arg0, str_polygon_not_supported, NULL);
	} else {
		while (argc >= parse) {
			for (i = 0; i < parse; i++) {
				/*
				 * check for legal int here!
				 */
				p[i] = atoi(argv[i]);
			}
			func(XtDisplay(w->w), XtWindow(w->w), gc, p[0], p[1], p[2], p[3], p[4], p[5]);
			argc -= parse;
			argv += parse;
		}
	}
	if (argc != 0) {
		printerr(arg0, str_left_over_points_ignored, NULL);
		return(1);
	}
	return(0);
}

int
ConvertTypeToString(arg0, class, w, parent, resource, val, ret)
char *arg0;
classtab_t *class;
wtab_t *w;
wtab_t *parent;
char *resource;
XtArgVal val;
char **ret;
{
	char *from_type;
	XtResourceList res;
	XrmValue    fr_val, to_val;
	struct namnod *nam;

	if ((nam = nam_search(resource, class->res, 0)) == NULL) {
		/* If we didn't find it in this widget's class record,
		 * see if the parent is a constraint widget class, and
		 * if so then see if we can find the class there.
		 */
		if (parent == NULL || parent->wclass == NULL ||
			parent->wclass->con == NULL ||
			(nam = nam_search(resource, parent->wclass->con, 0)) == NULL) {
			printerr(arg0, usagemsg(
				CONSTCHAR "No such resource for '%s' widget: ", 
				class->cname), resource);
			return(FAIL);
		}
	}
	res = (XtResourceList)(nam->value.namval.cp);

	/*
	 * unfortunately, we have to have a special case for String
	 * type resources, since their size may vary.
	 */
	if (strcmp(res->resource_type, str_XtRString) == 0) {
#ifndef MEMUTIL
		char *strccpy();
		char *strdup();
#endif /* MEMUTIL */

		*ret = ((String *)val)[0];
		return(0);
	}
	fr_val.size = res->resource_size;
	fr_val.addr = (caddr_t)val;
	to_val.size = 0;
	to_val.addr = NULL;

	XtConvert(
	    w ? w->w : Toplevel,
	    res->resource_type,	/* from type */
	    &fr_val,	/* from value */
	    str_XtRString,	/* to type */
	    &to_val	/* the converted value */
	);
	if (to_val.addr) {
		*ret = to_val.addr;
	} else {
	    printerr(arg0, usagemsg(CONSTCHAR "Failed conversion from '%s' to 'String'", res->resource_type), NULL);
		return(FAIL);
	}
	return(SUCCESS);
}

wtab_t *WKSHConversionWidget;
classtab_t *WKSHConversionClass;
char *WKSHConversionResource;

int
ConvertStringToType(arg0, w, parent, class, resource, val, ret, flatret, flatres, freeit, firstcall)
char *arg0;
wtab_t *w;
wtab_t *parent;
classtab_t *class;
char *resource;
char *val;
XtArgVal *ret;
XtArgVal *flatret;
char **flatres;
int *freeit;
int firstcall;	/* nonzero means this is the first call within a widget */
{
	char *to_type;
	XtResourceList res;
	XrmValue    fr_val, to_val;
	struct namnod *nam;

	WKSHConversionClass = class;	/* needed by callback converter */
	WKSHConversionResource = resource;	/* needed by callback converter */
	WKSHConversionWidget = w;	/* needed by callback converter */

	if ((nam = nam_search(resource, class->res, 0)) == NULL) {
		/* If we didn't find it in this widget's class record,
		 * see if the parent is a constraint widget class, and
		 * if so then see if we can find the class there.
		 */
		if (parent == NULL || parent->wclass == NULL ||
			parent->wclass->con == NULL ||
			(nam = nam_search(resource, parent->wclass->con, 0)) == NULL) {
			printerr(arg0, usagemsg(
				CONSTCHAR "No such resource for '%s' widget: ", 
				class->cname), resource);
			return(FAIL);
		}
	}
	res = (XtResourceList)nam->value.namval.cp;

	/*
	 * Unfortunately, because String types can be variable in size,
	 * we have to handle this as a special case.
	 */
	if (strcmp(res->resource_type, str_XtRString) == 0) {
		*ret = (XtArgVal)strdup(val);
		*freeit = TRUE;
		return(SUCCESS);
	}

	fr_val.size = strlen(val) + 1;
	fr_val.addr = (caddr_t)val;
	to_val.size = 0;
	to_val.addr = NULL;

	/*
	 * Hook to allow the toolkit to do something special
	 * with a resource.  For example, OPEN LOOK has to
	 * do some special things to convert flats.
	 */
	if (toolkit_special_resource(arg0, res, w, parent, 
		class, resource, val, ret, flatret, 
		flatres, freeit, firstcall)) {
		return(SUCCESS);
	}
	XtConvert(
	    w ? w->w : Toplevel,
	    str_XtRString,	/* from type */
	    &fr_val,	/* from value */
	    res->resource_type,	/* to type */
	    &to_val	/* the converted value */
	);
	/*
	 * PORTABILITY NOTE: the following code assumes that
	 * sizeof(int) is equal to either sizeof(short) or
	 * sizeof(long).
	 */
	if (to_val.size && to_val.addr) {
		switch(to_val.size) {
		case sizeof(char):
		    *ret = ((char *)to_val.addr)[0];
		    *freeit = FALSE;
		    break;
		case sizeof(short):
		    *ret = (XtArgVal)((short *)to_val.addr)[0];
		    *freeit = FALSE;
		    break;
		case sizeof(long):
		    *ret = (XtArgVal)((long *)to_val.addr)[0];
		    *freeit = FALSE;
		    break;
		default:
		    /*
		     * There is a possibility that some
		     * coverters will return malloc'ed space and this
		     * is really unnecessary and will leak memory.  About
		     * the only way to handle this is to handle such types as
		     * special cases.  Maybe we need a hash table that
		     * contains the names of types that need the malloc?
		     * The X specs should really have some mechanism for
		     * knowing when to free the results of a conversion.
		     */
		    *ret = (XtArgVal)XtMalloc(to_val.size);
		    memcpy((char *)ret, to_val.addr, to_val.size);
		    *freeit = TRUE;
		}
	} else {
	    printerr(arg0, usagemsg(CONSTCHAR "Failed string conversion to %s", res->resource_type), NULL);
		return(FAIL);
	}
	return(SUCCESS);
}

int
do_XtAddInput(argc, argv)
int argc;
char *argv[];
{
	register int i;
	int fd;
	char *arg0 = argv[0];
	char *cmd;
	inputrec_t *inp;

	if (argc < 2) {
		printerr(arg0, CONSTCHAR "usage: XtAddInput [ filename | -d descriptor ] [cmd ...]\n", NULL);
		return(1);
	}
	if (strncmp(argv[1], "-d", 2) == 0) {
		if (isdigit(argv[1][2])) {
			fd = atoi(&argv[1][2]);
			argv++;
			argc--;
		} else if (argv[1][2] != '\0' || argc < 3 || 
			!isdigit(argv[2][0])) {
			printerr(arg0, CONSTCHAR "usage: XtAddInput [ filename | -d descriptor ] [cmd ...]\n", NULL);
			return(1);
		} else {
			fd = atoi(argv[2]);
			argv += 2;
			argc -= 2;
		}
	} else {
		if ((fd = open(argv[1], O_RDONLY)) < 0) {
			printerr(arg0, CONSTCHAR "Could not open input source\n", argv[1]);
			return(1);
		}
		argv += 1;
		argc -= 1;
	}
	inp = (inputrec_t *)XtMalloc(sizeof(inputrec_t));
	memset(inp, '\0', sizeof(inputrec_t));
	inp->fd = fd;
	if (argc > 1) {
		inp->cmds = (char **)XtMalloc(sizeof(char *) * argc);
		for (i = 1; i < argc; i++) {
			inp->cmds[i-1] = strdup(argv[i]);
		}
		inp->cmds[i-1] = NULL;
		inp->ncmds = argc - 1;
	} else {
		/*
		 * With no arguments, the default is to just call
		 * "eval" on its args, which only requires a nil command
		 */
		inp->cmds = (char **)XtMalloc(sizeof(char *));
		inp->cmds[0] = (char *)(CONSTCHAR "");
		inp->ncmds = 1;
	}
	XtAddInput(fd, (XtPointer)XtInputReadMask, stdInputCB, (caddr_t)inp);
	return(0);
}

#ifdef MALDEBUG
int
do_malctl(argc, argv)
int argc;
char *argv[];
{
	if (argc == 2) {
		extern int Mt_trace, Mt_certify;

		Mt_trace = atoi(argv[1]);
		if (Mt_trace >= 0) {
			Mt_certify = 1;
		} else
			Mt_certify = 0;
		return(0);
	} else {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s fd\n", argv[0]), NULL);
		return(1);
	}
}
#endif

void
Translation_ksh_eval(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	register int i;

	for (i = 0; i < *num_params; i++) {
		ksh_eval(params[i]);
	}
}

/*
 * stdCB() is the central routine from which all callback
 * functions are dispatched (specified by clientData).  The
 * variable "CBW" will be placed in the environment to represent 
 * the CallBackWidget handle.  Certain widgets may have also
 * registered functions to be called before and/or after the callback
 * in order to set up environment variables with callData parameters.
 */

void
stdCB(widget, clientData, callData)
void  *widget;
caddr_t clientData, callData;
{
	wtab_t *widget_to_wtab();

	wksh_client_data_t *cdata = (wksh_client_data_t *)clientData;

	if (cdata->cbtab != NULL && cdata->cbtab->precb_proc != NULL)
		(cdata->cbtab->precb_proc)(cdata->w, callData);
	/*
	 * The wtab_t entry of the cdata need not be filled in since
	 * it could have been set via direct resource setting at widget
	 * creation time, and the converter for string to callback would
	 * not have had access to this information (since the widget
	 * was not created yet.
	 * Thus, we set it here.  Note that this will happen at most
	 * one time, since we are modifying the cdata structure.
	 */
	if (cdata->w == NULL) {
		cdata->w = widget_to_wtab(widget);
	}
	env_set_var(CONSTCHAR "CB_WIDGET", cdata->w->widid);

	ksh_eval((char *)cdata->ksh_cmd);
	if (cdata->cbtab != NULL && cdata->cbtab->postcb_proc != NULL)
		(cdata->cbtab->postcb_proc)(cdata->w, callData);
	return;
}

void
stdInputCB(inp, source, id)
inputrec_t *inp;
int *source;
int *id;
{
	char buf[BUFSIZ];
	char cmdbuf[1024];
	int cmd;
	char *p;
	register int i, n;
	static const char str_stdInputCB[] = "stdInputCB";

	/* try to read some input from the fd */

	if ((n = read(inp->fd, buf, sizeof(buf)-1)) <= 0) {
		/*
		 * no input was available, this is not a problem.
		 */
		return;
	}
	/* go through appending to current line, execute line if you 
	 * get an unquoted newline.
	 */

	for (i = 0; i < n; i++) {
		if (inp->lnend >= sizeof(inp->lnbuf)-1) {
			printerr(str_stdInputCB, CONSTCHAR "Line Overflow! Line flushed\n", NULL);
			inp->lnend = 0;
		}
		inp->lnbuf[inp->lnend++] = buf[i];
		if (buf[i] == '\n' && (i == 0 || buf[i-1] != '\\')) {
			inp->lnbuf[inp->lnend] = '\0';
			for (cmd = 0; cmd < inp->ncmds; cmd++) {
				sprintf(cmdbuf, "%s %s", inp->cmds[cmd], inp->lnbuf);
				ksh_eval(cmdbuf);
			}
			inp->lnend = 0;
		}
	}
}

void
stdTimerCB(clientData, id)
char *clientData;
long *id;
{
	ksh_eval((char *)clientData);
	XtFree(clientData);
	return;
}

static void
force()
{
	wk_libinit();
}

static int
VerifyString_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "Usage: %s [-i|-f|-d|-t] [-l low] [-h high] string", arg0), NULL);
	return(255);
}

do_VerifyString(argc, argv)
int argc;
char *argv[];
{
	char *lowrange = NULL, *highrange = NULL;
	char mode;
	register int i;
	int resultint;
	double resultfloat;
	char *arg0 = argv[0];

	if (argc < 3 || argv[1][0] != '-') {
		return(VerifyString_usage(arg0));
	}
	mode = argv[1][1];
	for (i = 2; i < argc && argv[i][0] == '-'; i++) {
		switch (argv[i][1]) {
		case 'l':
			if (argv[i][2])
				lowrange = &argv[i][2];
			else
				lowrange = argv[++i];
			break;
		case 'h':
			if (argv[i][2])
				highrange = &argv[i][2];
			else
				highrange = argv[++i];
		default:
			return(VerifyString_usage(arg0));
		}
	}
	if (argc - i != 1)
		return(VerifyString_usage(arg0));

	switch (mode) {
	case 'i':
		if (strspn(argv[i], CONSTCHAR "-0123456789") != strlen(argv[i])) {
			return(1);
		}
		resultint = atoi(argv[i]);
		if (lowrange) {
			int low = atoi(lowrange);

			if (resultint < low)
				return(2);
		}
		if (highrange) {
			int high = atoi(highrange);

			if (resultint > high)
				return(3);
		}
		return(0);
	case 'f':
	case 'd':
	case 't':
	default:
		return(VerifyString_usage(arg0));
	}
}

do_XFlush(argc, argv)
int argc;
char *argv[];
{
	if (Toplevel == NULL) {
		printerr(argv[0], CONSTCHAR "can't flush: toolkit not initialized.", NULL);
		return(1);
	}
	XFlush(XtDisplay(Toplevel));
	return(0);
}

static int
XtSetSensitive_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s widgetid [true|false]\n", arg0), NULL);
	return(1);
}

int
do_XtSetSensitive(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	Boolean bool;

	if (argc != 3) {
		return(XtSetSensitive_usage(argv[0]));
	}
	bool = (strcmp(argv[2], CONSTCHAR "true") == 0);
	if (!bool && strcmp(argv[2], CONSTCHAR "false") != 0) {
		return(XtSetSensitive_usage(argv[0]));
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w != NULL) {
		XtSetSensitive(w->w, bool);
	} else 
		return(XtSetSensitive_usage(argv[0]));
	return(0);
}
