/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:main.c	1.2.1.80"
#endif

/*
 main.c (C source file)
	Acc: 601053441 Tue Jan 17 10:17:21 1989
	Mod: 601054081 Tue Jan 17 10:28:01 1989
	Sta: 601054081 Tue Jan 17 10:28:01 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#ifdef lint
static char rcs_id[] = "$Header: main.c,v 1.23 88/02/26 09:14:01 swick Exp $";
#endif	/* lint */

/*
 * WARNING:  This code (particularly, the tty setup code) is a historical
 * relic and should not be confused with a real toolkit application or a
 * an example of how to do anything.  It really needs a rewrite.  Badly.
 */
#include <sys/termio.h>		/* for TIOCSWINSZ */

#include "xterm.h"	/* ehr3 - for DUP2 */
#include "error.h"
#include "ptyx.h"
#include "data.h"

#include <errno.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <utmp.h>
#include <sys/wait.h>

#include "Strings.h"
#include "messages.h"
/* #ifdef	SYSV */
#if defined(SYSV) || defined(SVR4)
char    my_DISPLAY[40];
char    my_WINDOWID[40];
char 	my_GUIMODE[20];

# ifndef SVR4
# include	<sys/stream.h>	/* ehr3 - for typedef used in ptem.h */
# include	<sys/ptem.h>	/* ehr3 - for struct winsize */
# endif /* SVR4 */

# ifdef JOBCONTROL
# include <sys/bsdtty.h>
# endif	/* JOBCONTROL */
#endif	/* SYSV */

#include <X11/copyright.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Shell.h>
					/* SS-ioctl */
#define XK_MISCELLANY
#include <X11/keysymdef.h>		/* SS-ioctl-end */
#include <Xol/OpenLook.h>
#include <X11/StringDefs.h>

extern Pixmap make_gray();
#ifndef MEMUTIL
extern char *malloc();
extern char *calloc();
#endif
extern char *ttyname();
extern void exit();
#ifndef SVR4
extern void sleep();
#endif /* SVR4 */
extern void bcopy();
extern void vhangup();
extern long lseek();
extern	int	get_vtname();

int switchfb[] = {0, 2, 1, 3};

static void reapchild ();

static void SetupUsageStrings();

/*
 * USL SVR4: need to define SYSV
 */
#define SYSV YES

static char **command_to_exec;

/* The following structures are initialized in main() in order
** to eliminate any assumptions about the internal order of their
** contents.
*/
struct termios d_tio;

char   *tty_modes;
static int parse_tty_modes ();
struct _xttymodes {
    char *name;
    int len;
    int set;
    char value;
} ttymodelist[] = {
{ "intr", 4, 0, '\0' },			/* tchars.t_intrc ; VINTR */
#define XTTYMODE_intr 0
{ "quit", 4, 0, '\0' },			/* tchars.t_quitc ; VQUIT */
#define XTTYMODE_quit 1
{ "erase", 5, 0, '\0' },		/* sgttyb.sg_erase ; VERASE */
#define XTTYMODE_erase 2
{ "kill", 4, 0, '\0' },			/* sgttyb.sg_kill ; VKILL */
#define XTTYMODE_kill 3
{ "eof", 3, 0, '\0' },			/* tchars.t_eofc ; VEOF */
#define XTTYMODE_eof 4
{ "eol", 3, 0, '\0' },			/* VEOL */
#define XTTYMODE_eol 5
{ "eol2", 4, 0, '\0' },			/* VEOL2 */
#define XTTYMODE_eol2 6
{ "swtch", 5, 0, '\0' },		/* VSWTCH */
#define XTTYMODE_swtch 7
{ "start", 5, 0, '\0' },		/* VSTART */
#define XTTYMODE_start 8
{ "stop", 4, 0, '\0' },			/* VSTOP */
#define XTTYMODE_stop 9
{ "susp", 4, 0, '\0' },			/* ltchars.t_suspc ; VSUSP */
#define XTTYMODE_susp 10
{ "dsusp", 5, 0, '\0' },		/* ltchars.t_dsuspc ; VDSUSP */
#define XTTYMODE_dsusp 11
{ "rprnt", 5, 0, '\0' },		/* ltchars.t_rprntc ; VREPRINT */
#define XTTYMODE_rprnt 12
{ "flush", 5, 0, '\0' },		/* ltchars.t_flushc ; VDISCARD */
#define XTTYMODE_flush 13
{ "weras", 5, 0, '\0' },		/* ltchars.t_werasc ; VWERASE */
#define XTTYMODE_weras 14
{ "lnext", 5, 0, '\0' },		/* ltchars.t_lnextc ; VLNEXT */
#define XTTYMODE_lnext 15
{ NULL, 0, 0, '\0' },			/* end of data */
};

char *ProgramName;

extern struct utmp *getutent();
extern struct utmp *getutid();
extern struct utmp *getutline();
extern void setutent();
extern void endutent();

#ifndef __STDC__
extern void pututline();
extern void utmpname();
#endif /* __STDC__ */

extern struct passwd *getpwent();
extern struct passwd *getpwuid();
extern struct passwd *getpwnam();
extern void 	      setpwent();
extern void 	      endpwent();
extern struct passwd *fgetpwent();

static char *get_ty;
static int inhibit;
static int log_on;

static int loginpty;
#ifdef TIOCCONS
static int Console;
#endif	/* TIOCCONS */

static char *icon_geometry;
static char *icon_name;
static Pixmap *icon_pixmap;
static Pixmap *icon_mask;
static char *default_title = NULL;

/* ROSS */
static void	IgnoreWarnings();
static Boolean	false = False;
static Boolean	warnings = False;
/* ROSS-end */

static Boolean	report_child_status = False;

/* used by VT (charproc.c) */

static XtResource application_resources[] = {
    {XtNname, "Name", XtRString, sizeof(char *),
	(Cardinal)&xterm_name, XtRString, "xterm"},
    {"iconGeometry", "IconGeometry", XtRString, sizeof(char *),
	(Cardinal)&icon_geometry, XtRString, (XtPointer) NULL},
    {"procIcon", "ProcIcon", XtRPixmap, sizeof (Pixmap),
        (Cardinal)&icon_pixmap, XtRString, "term48.icon"},
    {"procMask", "ProcMask", XtRBitmap, sizeof (Pixmap),
        (Cardinal)&icon_mask, XtRString, "term48.mask"},
    {XtNtitle, XtCTitle, XtRString, sizeof(char *),
	(Cardinal)&title, XtRString, (XtPointer) NULL},
    {XtNiconName, XtCIconName, XtRString, sizeof(char *),
        (Cardinal)&icon_name, XtRString, (XtPointer) NULL},
/* ROSS */
    {"warnings", "Warnings", XtRBoolean, sizeof(Boolean),
	(Cardinal)&warnings, XtRBoolean, (XtPointer) &false },
/* ROSS-end */
    {"ttyModes", "TtyModes", XtRString, sizeof(char *),
        (Cardinal)&tty_modes, XtRString, (XtPointer) NULL},
};

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec optionDescList[] = {
#ifndef SYSV
{"-132",	"*c132",	XrmoptionNoArg,		(XtPointer) "on"},
{"+132",	"*c132",	XrmoptionNoArg,		(XtPointer) "off"},
#endif

#ifdef TIOCCONS
{"-e",		NULL,		XrmoptionSkipLine,	(XtPointer) NULL},
#endif	/* TIOCCONS */

{"-b",		"*internalBorder",XrmoptionSepArg,	(XtPointer) NULL},
#ifdef SVR4
{"-C",	"*console", XrmoptionNoArg,	(XtPointer) "on"},
#endif
{"-cr",		"*inputFocusColor",	XrmoptionSepArg,	(XtPointer) NULL},

{"-e",		NULL,		XrmoptionSkipLine,	(XtPointer) NULL},
{"-E",		NULL,		XrmoptionSkipLine,	(XtPointer) NULL},
{"-fb",		"*boldFont",	XrmoptionSepArg,	(XtPointer) NULL},

/* fontColor replaces foreground resource */
{"-fg",		"*fontColor",	XrmoptionSepArg,	(XtPointer) NULL},

/* FLH resize */
{"-rs",		"*cursesResize", XrmoptionNoArg,		(XtPointer) "on"},
{"+rs",		"*cursesResize", XrmoptionNoArg,		(XtPointer) "off"},
/* FLH resize-end */

{"-j",		"*jumpScroll",	XrmoptionNoArg,		(XtPointer) "on"},
{"+j",		"*jumpScroll",	XrmoptionNoArg,		(XtPointer) "off"},

{"-l",		"*logging",	XrmoptionNoArg,		(XtPointer) "on"},
{"+l",		"*logging",	XrmoptionNoArg,		(XtPointer) "off"},
{"-lf",		"*logFile",	XrmoptionSepArg,	(XtPointer) NULL},

/* #ifndef SYSV */
{"-ls",		"*loginShell",	XrmoptionNoArg,		(XtPointer) "on"},
{"+ls",		"*loginShell",	XrmoptionNoArg,		(XtPointer) "off"},
/* #endif */

{"-mb",		"*marginBell",	XrmoptionNoArg,		(XtPointer) "on"},
{"+mb",		"*marginBell",	XrmoptionNoArg,		(XtPointer) "off"},
{"-ml",		"*mouseless",	XrmoptionNoArg,		(XtPointer) "on"},
{"+ml",		"*mouseless",	XrmoptionNoArg,		(XtPointer) "off"},
{"-ms",		"*pointerColor",XrmoptionSepArg,	(XtPointer) NULL},
{"-nb",		"*nMarginBell",	XrmoptionSepArg,	(XtPointer) NULL},
{"-rw",		"*reverseWrap",	XrmoptionNoArg,		(XtPointer) "on"},
{"+rw",		"*reverseWrap",XrmoptionNoArg,		(XtPointer) "off"},

#ifndef SYSV
{"-s",		"*multiScroll",	XrmoptionNoArg,		(XtPointer) "on"},
{"+s",		"*multiScroll", XrmoptionNoArg,	(XtPointer) "off"},
#endif
{"-sb",		"*scrollBar",	XrmoptionNoArg,		(XtPointer) "on"},
{"+sb",		"*scrollBar",	XrmoptionNoArg,		(XtPointer) "off"},

#ifdef XTERM_COMPAT
{"-si",		"*scrollInput",	XrmoptionNoArg,		(XtPointer) "off"},
{"+si",		"*scrollInput", XrmoptionNoArg,		(XtPointer) "on"},
{"-sk",		"*scrollKey",	XrmoptionNoArg,		(XtPointer) "on"},
{"+sk",		"*scrollKey",	XrmoptionNoArg,		(XtPointer) "off"},
#endif

{"-sl",		"*saveLines",	XrmoptionSepArg,	(XtPointer) NULL},

#ifdef TEK
{"-t",		"*tekStartup",	XrmoptionNoArg,		(XtPointer) "on"},
{"+t",		"*tekStartup",	XrmoptionNoArg,		(XtPointer) "off"},
#endif /* TEK */

{"-vb",		"*visualBell",	XrmoptionNoArg,		(XtPointer) "on"},
{"+vb",		"*visualBell",	XrmoptionNoArg,		(XtPointer) "off"},
/* bogus old compatibility stuff for which there are
   standard XtInitialize options now */
#ifndef TRASHEQUALGEOMETRY
{"=",		"*xterm.geometry",XrmoptionStickyArg,	(XtPointer) NULL},
#endif

/* MORE: why not on SYSV? */
#ifndef SYSV
{"#",		".iconGeometry",XrmoptionStickyArg,	(XtPointer) NULL},
#endif

#ifdef TEK
{"%",		"*tekGeometry", XrmoptionStickyArg,	(XtPointer) NULL},
#endif /* TEK */

{"-T",		"*title",	XrmoptionSepArg,	(XtPointer) NULL},
#ifndef TEK
{"-t",		"*title",	XrmoptionSepArg,	(XtPointer) NULL},
#endif /* TEK */
{"-n",		"*iconName",	XrmoptionSepArg,	(XtPointer) NULL},
{"-r",		"*reverseVideo",XrmoptionNoArg,		(XtPointer) "on"},
{"+r",		"*reverseVideo",XrmoptionNoArg,		(XtPointer) "off"},
{"-rv",		"*reverseVideo",XrmoptionNoArg,		(XtPointer) "on"},
{"+rv",		"*reverseVideo",XrmoptionNoArg,		(XtPointer) "off"},
{"-w",		".borderWidth", XrmoptionSepArg, 	(XtPointer) NULL},
};

extern WidgetClass xtermWidgetClass;

Arg ourTopLevelShellArgs[] = {
	{ XtNallowShellResize, (XtArgVal) TRUE },	
	{ XtNinput, (XtArgVal) FALSE },
/* XtNinput is actually set to FALSE by default anyway (in OpenLook)*/
/*
	{ XtNinput, (XtArgVal) TRUE },
FLH mouseless
*/
};
int number_ourTopLevelShellArgs = 2;
	
Widget toplevel;
OlDtHelpInfo help_info[] = {NULL, NULL, "xterm/term.hlp", NULL, NULL};

main (argc, argv)
int argc;
char **argv;
{
	register TScreen *screen;
	register int i, pty;
	int Xsocket, mode;
#ifndef MEMUTIL
	char *malloc();
#endif
	char *basename();
	int xerror(), xioerror();
	int fd1 = -1;
	int fd2 = -1;
	int fd3 = -1;
	int	lastfd = 0;	/* ehr3 */


#ifdef MEMUTIL
	InitializeMemutil();
#endif

	ProgramName = argv[0];

	/* close any extra open (stray) file descriptors */
	lastfd = ulimit(4, (long) 0);

	for (i = 3; i < lastfd; i++)
		(void) close(i);

#if !defined(SVR4)
        d_tio.c_line = 0;
#endif
	/* initialize the c_cc array: first try to inherit tty settings.  */
	/* if this will fail, use the default values		   	  */

	{
	    int i;

	    for (i = 0; i <= 2; i++) {
		if (ioctl (i, TCGETS, &d_tio) == 0) {

		    /* if started from dtm there is no controlling tty, so */
		    /* all values are set to defaults.  in this case use   */
		    /* CTRL-H for erase					   */

		    if (getppid() == 1)
        	        d_tio.c_cc[VERASE] = 'H' & 0x3f;    /* '^H' */

		    /* if start or stop are not set, set them	    */

		    if (d_tio.c_cc[VSTART] == 0)
        	        d_tio.c_cc[VSTART]  = 'Q' & 0x3f;   /* '^Q' */
		    if (d_tio.c_cc[VSTOP] == 0)
        	        d_tio.c_cc[VSTOP]   = 'S' & 0x3f;   /* '^S' */

		    break;
		}
	    }

	    /* could not inherit, use the default values, except for erase  */

	    if (i == 3) {
        	d_tio.c_cc[VINTR]  = CINTR;
        	d_tio.c_cc[VQUIT]  = CQUIT;
        	d_tio.c_cc[VERASE] = 'H' & 0x3f;    /* '^H' */
        	d_tio.c_cc[VKILL]  = CKILL;
        	d_tio.c_cc[VEOF]   = CEOF;
        	d_tio.c_cc[VEOL]   = CNUL;
        	d_tio.c_cc[VEOL2]  = CNUL;
#ifdef VSWITCH
        	d_tio.c_cc[VSWTCH] = CNUL;
#endif
		d_tio.c_cc[VSTART]   = CSTART;
		d_tio.c_cc[VSTOP]    = CSTOP;
#ifdef SVR4
        	d_tio.c_cc[VSUSP]    = CSUSP;
        	d_tio.c_cc[VDSUSP]   = CDSUSP;
        	d_tio.c_cc[VREPRINT] = CNUL;
        	d_tio.c_cc[VDISCARD] = CNUL;
        	d_tio.c_cc[VWERASE]  = CNUL;
        	d_tio.c_cc[VLNEXT]   = CNUL;
#endif
	    }
	}

	/* Change some of the the modes for the child pty  */

        /* input: nl->nl, don't ignore cr, cr->nl */
        d_tio.c_iflag &= ~(INLCR|IGNCR|ISTRIP);
        d_tio.c_iflag |= ICRNL|IXON;

        /* ouput: cr->cr, nl is not return, no delays, ln->cr/nl */
        d_tio.c_oflag &= ~(OCRNL|ONLRET|NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
        d_tio.c_oflag |= (ONLCR|OPOST|TAB3);

        d_tio.c_cflag &= ~(CBAUD|PARENB);
        d_tio.c_cflag |= B9600|CS8|CREAD|HUPCL;

        /* enable signals, canonical processing (erase, kill, etc), echo */

        d_tio.c_lflag |= ISIG|ICANON|ECHO|ECHOE|ECHOK;

	/* Init the Toolkit. */

#ifdef MEMUTIL
	InitializeMemutil();
#endif
	/* enable GUI switch */

	OlToolkitInitialize(&argc, argv, NULL);

	toplevel = XtInitialize("main", "XTerm",
		optionDescList, XtNumber(optionDescList), &argc, argv);


	XtGetApplicationResources( toplevel, 0, application_resources,
				   XtNumber(application_resources), NULL, 0 );

 	/*
 	 *	Register the Process Icon
 	 */
 	XtVaSetValues(toplevel,
 		      XtNiconPixmap, icon_pixmap,
		      XtNiconMask, icon_mask,
 		      NULL);
 
	/* Make an internal copy of title.
	 * It may be freed later by ChangeTitle
	 */
	if (title != NULL)
		title = XtNewString(title);

/* ROSS */
	if (warnings == False)
	{
	    XtSetWarningHandler(IgnoreWarnings);
	    OlSetWarningHandler(IgnoreWarnings);
	}
/* ROSS-end */

	if (strcmp(xterm_name, "-") == 0)
		xterm_name = "xterm";

#if defined(I18N)
	/* in the non-internationalized env. the title will be "xterm"	*/
	/* unless specified otherwise.  In the internationalized case	*/
	/* we would like to get the correct title.			*/

	if (title == NULL)
	{
		static Arg args[1];
		

		default_title = OlGetMessage(XtDisplay(toplevel),
							 NULL, 0,
				 			 OleNtitle, OleTxterm,
							 OleCOlClientXtermMsgs,
				 			 OleMtitle_xterm,NULL);
                XtSetArg (args[0], XtNtitle, default_title);
                XtSetValues (toplevel, args, 1);
	}
#endif

/* SS-title: there seems to be a problem in XtInitialize.  If the iconName */
/*	     resource is specified, while title resource is not specified, */
/*	     both the iconName and the title of the toplevel widget are set*/
/*	     to the iconName.  Since this is not hapenning for the 	   */
/*	     Application Resources, we can detect and correct it here	   */

	if (title == NULL && icon_name != NULL)
	{
		static Arg args[1];
	
                XtSetArg (args[0], XtNtitle, xterm_name);
                XtSetValues (toplevel, args, 1);
	}
/* SS-title-end */

	if (icon_geometry != NULL) {
	    int scr, junk;
	    Arg args[2];

	    for(scr = 0;	/* yyuucchh */
		XtScreen(toplevel) != ScreenOfDisplay(XtDisplay(toplevel),scr);
		scr++);

	    args[0].name = XtNiconX;
	    args[1].name = XtNiconY;
	    XGeometry(XtDisplay(toplevel), scr, icon_geometry, "", 0, 0, 0,
		      0, 0, (int *) &args[0].value, (int *) &args[1].value, &junk, &junk);
	    XtSetValues( toplevel, args, 2);
	}

	XtSetValues (toplevel, ourTopLevelShellArgs, 
		     number_ourTopLevelShellArgs);
/* SS-fix */
	{
	    int bw = toplevel[0].core.border_width;

	    if (bw < 1)
		toplevel[0].core.border_width = 1;
	    else if (bw > 40)
		toplevel[0].core.border_width = 40;
	}
/* SS-fix-end */

	/* Now that we are in control again, close any uglies. */
	if (fd1 >= 0)
	    (void)close(fd1);
	if (fd2 >= 0)
	    (void)close(fd2);
	if (fd3 >= 0)
	    (void)close(fd3);


	/* Parse the rest of the command line.  We need the #if statement */
	/* to prevent compiler warning: otherwise we'll have a loop which */
	/* never reaches the bottom '{', causing compiler warning	  */

#if defined(TIOCCONS) || !defined(SYSV) || defined(DEBUG)
	for (argc--, argv++ ; argc > 0 ; argc--, argv++) {
#else
	if (argc > 1) {
	    argv++;
#endif
	    if (**argv != '-')
		Syntax (*argv);

	    switch(argv[0][1]) {
#ifdef TIOCCONS
	     case 'C':
		Console = TRUE;
		continue;
#endif	/* TIOCCONS */

#ifdef DEBUG
	     case 'D':
		debug = TRUE;
		continue;
#endif	/* DEBUG */
	     case 'e':
	     case 'E':
		if (argc <= 1) Syntax (*argv);
		if (argv[0][1] == 'E')
		    report_child_status = TRUE;
		command_to_exec = ++argv;
		break;
	     default:
		Syntax (*argv);
	    }
#if defined(TIOCCONS) || !defined(SYSV) || defined(DEBUG)
	    break;
#endif
	}

/* FLH Dynamic */
				/*
				 *		Make xterm and scrollbar (peer) children
				 *		of a rubbertile widget.
				 *
				 *		Scrollbar will appear outside of text window, and
				 *		will pick up regular (shell) background, rather
				 *		than text background
				 */
		{
			Arg args[5];
			int n;
				
				/* tell rubbertile (container) to layout the
				 * text window and scrollbar horizontally
				 */
			n = 0;
			XtSetArg(args[n],XtNorientation,OL_HORIZONTAL); n++;
			XtSetArg(args[n],XtNshadowThickness,0); n++;
			container = (RubberTileWidget) XtCreateManagedWidget(
							"container", 
							rubberTileWidgetClass,
							toplevel, 
							args, n);

				/* 
				 *	set xterm weight to 1 so 100% of a resize is
				 * absorbed by the xterm virtual screen 
				 */
			n = 0;
			XtSetArg(args[n],XtNweight,1); n++;
			XtSetArg(args[n],XtNshadowThickness,0); n++;
				/*
				 * Create a footer panel to hold the xterm screen
				 * and a status field.
				 */
			footerpane = (FooterPanelWidget) XtCreateManagedWidget(
								"footer", 
								footerPanelWidgetClass, 
								(Widget) container, 
								args, n);
        	term = (XtermWidget) XtCreateManagedWidget(
							   "xterm", 
							   xtermWidgetClass, 
							   (Widget) footerpane, 
							   NULL, 0);
		}
            /* this causes the initialize method to be called */

        screen = &term->screen;

	/* fill in terminal modes.  we could not do this earlier, because */
	/* screen, which we is needed in OlVaDisplayErrorMsg() was not set*/

        if (tty_modes) {
            int n = parse_tty_modes (tty_modes, ttymodelist);
            if (n < 0) {
#if !defined(I18N)
                fprintf (stderr, "%s:  bad tty modes \"%s\"\n",
                         ProgramName, tty_modes);
#else
        	OlVaDisplayErrorMsg(screen->display, OleNtty, OleTbadTty,
				OleCOlClientXtermMsgs, OleMtty_badTty,
				ProgramName, tty_modes, NULL);
#endif
            } else if (n > 0) {

#define TMODE(ind,var) if (ttymodelist[ind].set) var = ttymodelist[ind].value;

		TMODE (XTTYMODE_intr, d_tio.c_cc[VINTR]);
		TMODE (XTTYMODE_quit, d_tio.c_cc[VQUIT]);
		TMODE (XTTYMODE_erase, d_tio.c_cc[VERASE]);
		TMODE (XTTYMODE_kill, d_tio.c_cc[VKILL]);
		TMODE (XTTYMODE_eof, d_tio.c_cc[VEOF]);
		TMODE (XTTYMODE_eol, d_tio.c_cc[VEOL]);
#ifdef VSWTCH
		TMODE (XTTYMODE_swtch, d_tio.c_cc[VSWTCH]);
#endif
		TMODE (XTTYMODE_start, d_tio.c_cc[VSTART]);
		TMODE (XTTYMODE_stop,  d_tio.c_cc[VSTOP]);
#ifdef SVR4
		TMODE (XTTYMODE_susp,  d_tio.c_cc[VSUSP]);
		TMODE (XTTYMODE_dsusp, d_tio.c_cc[VDSUSP]);
		TMODE (XTTYMODE_rprnt, d_tio.c_cc[VREPRINT]);
		TMODE (XTTYMODE_flush, d_tio.c_cc[VDISCARD]);
		TMODE (XTTYMODE_weras, d_tio.c_cc[VWERASE]);
		TMODE (XTTYMODE_lnext, d_tio.c_cc[VLNEXT]);
#endif
	    }
#undef TMODE
	}

	/* Get Hypertext help from Desktop Manager */
#ifdef DTM_HELP
	if (title)
	    help_info->app_title = title;
	else
	    help_info->app_title = default_title;
 	OlRegisterHelp (OL_WIDGET_HELP, term, "xterm", OL_DESKTOP_SOURCE, &help_info);
#else
	OlRegisterHelp (OL_WIDGET_HELP, term, "xterm", OL_STRING_SOURCE,
		OlGetMessage(screen->display, NULL, 0,
				OleNhelp, OleThelpString, OleCOlClientXtermMsgs,
				OleMhelp_helpString, NULL));
#endif /* DTM_HELP */

	term->flags = WRAPAROUND | AUTOREPEAT;
	if (!screen->jumpscroll)	term->flags |= SMOOTHSCROLL;
	if (term->misc.reverseWrap)	term->flags |= REVERSEWRAP;
	if (term->misc.re_verse)	term->flags |= REVERSE_VIDEO;

	inhibit = 0;
	if (term->misc.logInhibit)	inhibit |= I_LOG;
	if (term->misc.signalInhibit)	inhibit |= I_SIGNAL;
#ifdef TEK
	if (term->misc.tekInhibit)	inhibit |= I_TEK;
#endif /* TEK */

	if (term->misc.scrollbar){
			if (screen->scrollWidget)
				XtVaGetValues(screen->scrollWidget, XtNwidth, &screen->scrollbar, NULL);
			else
				screen->scrollbar = SCROLLBARWIDTH;
		}

	term->initflags = term->flags;

	if (get_ty || command_to_exec) {
	    char window_title[1024];
	    static Arg args[2];

	    if(!title)
	    {
	    	if (get_ty) {

			/* for gethostname (see Xt:Initalize.c)  MichaelZ */
#ifdef USE_UNAME
                    struct utsname name;    /* crock for hpux 8 char names */
                    uname(&name);
#if !defined(I18N)
                    strcpy (window_title, "login(");
#else
			strcpy(window_title, OlGetMessage(screen->display, NULL, 0,
				OleNtitle, OleTwindow, OleCOlClientXtermMsgs,
				OleMtitle_window, NULL));
#endif
                    strcpy (window_title+6, name.nodename);
                    strcat (window_title, ")");
#else
#if !defined(I18N)
                    strcpy (window_title, "login(");
#else
			strcpy(window_title, OlGetMessage(screen->display, NULL, 0,
				OleNtitle, OleTwindow, OleCOlClientXtermMsgs,
				OleMtitle_window, NULL));
#endif
                    (void) gethostname(window_title+6, sizeof(window_title)-6);
                    strcat (window_title, ")");
#endif
			/* end of  gethostname patch */

                    title = strdup (window_title);
                } else if (command_to_exec) {
                    title = strdup (basename (command_to_exec[0]));
                } /* else not reached */
            }

            if (!icon_name)
              icon_name = title;
            XtSetArg (args[0], XtNtitle, title);
            XtSetArg (args[1], XtNiconName, icon_name);

            XtSetValues (toplevel, args, 2);
        }

#ifdef TEK
	if(inhibit & I_TEK)
		screen->TekEmu = FALSE;

	if(screen->TekEmu && !TekInit())
		exit(ERROR_INIT);
#endif /* TEK */

	/* set up stderr properly */
	i = -1;
#ifdef DEBUG
	if(debug)
		i = open ("xterm.debug.log", O_WRONLY | O_CREAT | O_TRUNC,
		 0666);
	else
#endif	/* DEBUG */
	if(get_ty)
		i = open("/dev/console", O_WRONLY, 0);
	if(i >= 0)
		fileno(stderr) = i;
	if(fileno(stderr) != (lastfd - 1)) {
#if defined(SYSV) && !defined(SVR4)
		/* SYSV has another pointer which should be part of the
		** FILE structure but is actually a seperate array.
		*/
		unsigned char *old_bufend;

		if (dup2(fileno(stderr), (lastfd - 1)) == -1) {
		    char	debug_buffer[256];
#if !defined(I18N)
		    (void) fprintf(stderr, "Error: Failed to dup2(%d, %d)\r\n", fd1, fd2);
#else
        	OlVaDisplayErrorMsg(screen->display, OleNdup, OleTbadDup2,
				OleCOlClientXtermMsgs, OleMdup_badDup2, fd1,
				fd2, NULL);
#endif
		    (void) fflush(stderr);
		    (void) sprintf(debug_buffer, "echo \"dup2(%d, %d) failed errno = %d\" > CHECK", fd1, fd2, errno);
		    system(debug_buffer);
		    exit(1);
		}

		old_bufend = (unsigned char *) _bufend(stderr);
		if(fileno(stderr) >= 3)
			close(fileno(stderr));
		fileno(stderr) = (lastfd - 1);
		(unsigned char *) _bufend(stderr) = old_bufend;
#else	/* !SYSV */
		dup2(fileno(stderr), (lastfd - 1));
		if(fileno(stderr) >= 3)
			close(fileno(stderr));
		fileno(stderr) = (lastfd - 1);
#endif	/* !SYSV */
	}

	(void) signal (SIGCHLD, reapchild);

	/* open a terminal for client */
	get_terminal ();
	spawn ();

	Xsocket = screen->display->fd;
	pty = screen->respond;

	if (am_slave) { /* Write window id so master end can read and use */
#ifdef TEK
	    write(pty, screen->TekEmu ? (char *)&TWindow(screen) :
             (char *)&VWindow(screen), sizeof(Window));
#else
	    write(pty, (char *)&VWindow(screen), sizeof(Window));
#endif /* TEK */
	    write(pty, "\n", 1);
	}

	if(log_on) {
		log_on = FALSE;
		StartLog(screen);
	}
	screen->inhibit = inhibit;
	screen->grabbedKbd = FALSE;	/* RJK (secure keyboard) */

	if (0 > (mode = fcntl(pty, F_GETFL, 0)))
		Error();
	mode |= O_NDELAY;
	if (fcntl(pty, F_SETFL, mode))
		Error();
	
	pty_mask = 1 << pty;
	X_mask = 1 << Xsocket;
	Select_mask = pty_mask | X_mask;
	max_plus1 = (pty < Xsocket) ? (1 + Xsocket) : (1 + pty);

#ifdef DEBUG
	if (debug) printf ("debugging on\n");
#endif	/* DEBUG */
	XSetErrorHandler(xerror);
	XSetIOErrorHandler(xioerror);

	for( ; ; )
#ifdef TEK
	    if(screen->TekEmu)
               TekRun();
	    else
#endif /* TEK */
	       VTRun();
}

char *basename(name)
char *name;
{
	register char *cp;
	char *rindex();

	return((cp = rindex(name, '/')) ? cp + 1 : name);
}

/* ustring - an array of strings for the "usage" statement */

#if !defined(MAXUSAGE)
#define MAXUSAGE 50
#endif

/* For I18N, don't define the array of strings here, simply set
 * to NULL.
 */
#if !defined(I18N)
static char *ustring[] = 
{
#ifdef SYSV
"Usage: xterm [-b inner_border_width] [-bd border_color] \\\n",
#else
"Usage: xterm [-132] [-b inner_border_width] [-bd border_color] \\\n",
#endif

#ifdef TIOCCONS
" [-bg backgrnd_color] [-bw border_width] [-C] [-cr cursor_color] \\\n",
#else	/* TIOCCONS */
" [-bg backgrnd_color] [-bw border_width] [-cr cursor_color] \\\n",
#endif	/* TIOCCONS */

#ifdef SVR4
" [-C] [-display display] [-fb bold_font] [-fg foregrnd_color] [-fn norm_font] \\\n",

#else

" [-display display] [-fb bold_font] [-fg foregrnd_color] [-fn norm_font] \\\n",
#endif

/*NOTE: -i option used to be commented out in XWIN 1.0 */
" [-i] [-j] [+j] [-l] [+l] [-lf logfile] [-ls] [-mb] [+mb] \\\n",
" [-ms mouse_color] [-n icon name] [-name application name] \\\n",
" [-nb bell_margin] [-rv|-r] [-rs] [+rs] [-rw] [+rw] \\\n",

#ifdef SYSV
/* ehr3: No -sn -tb option, while -t is for title in SYSV */
#ifdef XTERM_COMPAT
#ifdef TEK
" [-sb] [+sb] [-si] [-sk] [-sl save_lines] [-t] [-T title] \\\n",
#else  /* TEK */
" [-sb] [+sb] [-si] [-sk] [-sl save_lines] [-T|t title] \\\n",
#endif /* TEK */
#else
#ifdef TEK
" [-sb] [+sb] [-sl save_lines] [-t] [-T title] \\\n",
#else /* TEK */
" [-sb] [+sb] [-sl save_lines] [-T|t title] \\\n",
#endif /* TEK */
#endif /*XTERM_COMPAT */
#else
#ifdef TEK
" [-s] [-sb] [-si] [-sk] [-sl save_lines] [-sn] [-st] \\\n",
" [-T title] [-t]  [-tb] \\\n",
#else /* TEK */
" [-s] [-sb] [-si] [-sk] [-sl save_lines] [-sn] [-st] [-T|t title] [-tb] \\\n",
#endif /*TEK */
#endif

#ifdef SYSV
" [-vb] [+vb] [-geometry [columns][xlines][[+-]xoff[[+-]yoff]]] \\\n",
#else
" [-vb] [+vb] [=[width]x[height][[+-]xoff[[+-]yoff]]] \\\n",
#endif

#ifndef SYSV
" [%[width]x[height][[+-]xoff[[+-]yoff]]] [#[+-]xoff[[+-]yoff]] \\\n",
#endif

" [-w border_width] [-e|E command_to_exec] [-xrm resource string]\n\n",
"Fonts must be of fixed width and of same size;\n",
"If only one font is specified, it will be used for normal and bold text\n",

#ifndef SYSV
"The -132 option allows 80 <-> 132 column escape sequences\n",
#endif

#ifdef TIOCCONS
"The -C option forces output to /dev/console to appear in this window\n",
#endif	/* TIOCCONS */

"The -i  option enables iconic startup\n",

"The -j  option enables jump scroll\n",

"The -l  option enables logging\n",

"The -ls option makes the shell a login shell\n",
"The -mb option turns the margin bell on\n",
"The -ml option turns mouseless mode on\n",
"The +rs option disables window resizing in curses mode\n",
"The -rs option allows window resizing in curses mode\n",
"The -rv option turns reverse video on\n",
"The -rw option turns reverse wraparound on\n",

#ifndef SYSV
"The -s  option enables asynchronous scrolling\n",
#endif

"The -sb option enables the scrollbar\n",

#ifdef XTERM_COMPAT
"The -si option disables re-positioning the scrollbar at the bottom on input\n",
"The -sk option causes the scrollbar to position at the bottom on a key\n",
#endif

#ifdef TEK
"The -t  option starts Tektronix mode\n",
#endif /* TEK */

"The -vb option enables visual bell\n",
0
};
#else
static char *ustring[MAXUSAGE];
#endif


Syntax (badOption)
char	*badOption;
{
	register char **us = ustring;

	fprintf(stderr, "Unknown option \"%s\"\n", badOption);
	if (*us == NULL)
		SetupUsageStrings();
	while (*us) fputs(*us++, stderr);
	exit (1);
}

#define INSERT_MESSAGE(type, msg)  OlGetMessage(XtDisplay(toplevel), NULL, 0, \
			OleNusage, type, OleCOlClientXtermMsgs, msg, NULL);
static void
SetupUsageStrings()
{
	int cnt = 0;

#ifdef SYSV
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg1, OleMusage_msg1)
#else
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg1a, OleMusage_msg1a)
#endif
#ifdef TIOCCONS
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg2, OleMusage_msg2)
#else
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg2a, OleMusage_msg2a)
#endif
#ifdef SVR4
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg3, OleMusage_msg3)
#else
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg3a, OleMusage_msg3a)
#endif

	ustring[cnt++] = INSERT_MESSAGE(OleTmsg4, OleMusage_msg4)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg5, OleMusage_msg5)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg6, OleMusage_msg6)

#ifdef SYSV
/* ehr3: No -sn -tb option, while -t is for title in SYSV */
#ifdef XTERM_COMPAT
#ifdef TEK
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg7, OleMusage_msg7)
#else  /* TEK */
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg7a, OleMusage_msg7a)
#endif /* TEK */
#else
#ifdef TEK
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg7b, OleMusage_msg7b)
#else /* TEK */
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg7c, OleMusage_msg7c)
#endif /* TEK */
#endif /*XTERM_COMPAT */
#else
#ifdef TEK
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg7d, OleMusage_msg7d)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg7e, OleMusage_msg7e)
#else /* TEK */
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg7f, OleMusage_msg7f)
#endif /*TEK */
#endif

	ustring[cnt++] = INSERT_MESSAGE(OleTmsg8, OleMusage_msg8)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg8a, OleMusage_msg8a)

	ustring[cnt++] = INSERT_MESSAGE(OleTmsg9, OleMusage_msg9)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg10, OleMusage_msg10)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg11, OleMusage_msg11)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg12, OleMusage_msg12)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg13, OleMusage_msg13)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg14, OleMusage_msg14)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg15, OleMusage_msg15)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg16, OleMusage_msg16)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg17, OleMusage_msg17)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg18, OleMusage_msg18)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg19, OleMusage_msg19)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg19a, OleMusage_msg19a)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg20, OleMusage_msg20)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg21, OleMusage_msg21)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg22, OleMusage_msg22)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg23, OleMusage_msg23)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg24, OleMusage_msg24)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg25, OleMusage_msg25)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg26, OleMusage_msg26)
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg27, OleMusage_msg27)
#ifdef TEK
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg28, OleMusage_msg28)
#endif
	ustring[cnt++] = INSERT_MESSAGE(OleTmsg29, OleMusage_msg29)
	ustring[cnt] = NULL;
	return;

}



#ifndef	SYSV		/* ehr3 - SYSV uses openpty() */
get_pty (pty, tty)
/*
   opens a pty, storing fildes in pty and tty.
 */
int *pty, *tty;
{
	int devindex, letter = 0;

	while (letter < 11) {
	    ttydev [strlen(ttydev) - 2]  = ptydev [strlen(ptydev) - 2] =
		    PTYCHAR1 [letter++];
	    devindex = 0;

	    while (devindex < 16) {
		ttydev [strlen(ttydev) - 1] = ptydev [strlen(ptydev) - 1] =
			PTYCHAR2 [devindex++];
		if ((*pty = open (ptydev, O_RDWR)) < 0)
			continue;
		if ((*tty = open (ttydev, O_RDWR)) < 0) {
			close(*pty);
			continue;
		}
		return;
	    }
	}
	
#if !defined(I18N)
	fprintf (stderr, "%s: Not enough available pty's\n", xterm_name);
#else
        OlVaDisplayErrorMsg(XtDisplay(toplevel), OleNpty, OleTnoAvail,
				OleCOlClientXtermMsgs, OleMpty_noAvail,
				xterm_name, NULL);
#endif
	exit (ERROR_PTYS);
}
#endif

get_terminal ()
/* 
 * sets up X and initializes the terminal structure except for term.buf.fildes.
 */
{
	register TScreen *screen = &term->screen;

	screen->graybordertile = make_gray(term, term->core.border_pixel,
		screen->background,
		DefaultDepth(screen->display, DefaultScreen(screen->display)));


	{
	    unsigned long fg, bg;

	    fg = screen->mousecolor;
	    bg = (screen->mousecolor == screen->background) ?
		screen->foreground : screen->background;

	    screen->arrow = make_arrow (term, fg, bg);
	}
	XAutoRepeatOn(screen->display);
}

void
garp(dummy)
int dummy;
{

#if !defined(I18N)
	printf("sighup\n");
#else
	printf(
          OlGetMessage(XtDisplay(toplevel), NULL, 0, OleNsignal, OleTsighup,
			OleCOlClientXtermMsgs, OleMsignal_sighup, NULL) );
#endif
	kill(getpid(),SIGQUIT);
}

spawn()
{
	register TScreen *screen = &term->screen;
	int Xsocket = screen->display->fd;
/* ehr */
	int     i;
	extern  char    **environ;
	char    **envnew;
/* ehr-end */

	screen->uid = getuid();
	screen->gid = getgid();

#ifdef TEK
	if(!(screen->TekEmu ? TekInit() : VTInit()))
#else
	if (!VTInit())
#endif 
		exit(ERROR_INIT);

	/* ehr3 - for setting the icon */
/* FLH dynamic
 *
 * term is now 2 levels below the shell
 */
	/* if (getenv("ICONIFY"))
		set_icon(XtDisplay(term), VShellWindow, "/usr/X/icons/xterm1"); */
/* FLH dynamic */

	(void) signal(SIGHUP,garp);
/* ehr3 */

        /* copy the environment before Setenving */
        for (i = 0 ; environ [i] != NULL ; i++) ;
        /*
         * The `4' is the number of Setenv() calls which may add                         * a new entry to the environment.  The `1' is for the
         * NULL terminating entry.
         */
        envnew = (char **) calloc ((unsigned) i + (4 + 1), sizeof(char *));
        bcopy((char *)environ, (char *)envnew, i * sizeof(char *));                     environ = envnew;

	get_vtname(screen);	/* we set $TERM in here, because we use
				different values for color and monochrome */

#ifdef TEK
	sprintf(my_WINDOWID, "WINDOWID=%d", screen->TekEmu ? (int)TWindow(screen) : (int)VWindow(screen));
#else
	sprintf(my_WINDOWID, "WINDOWID=%d", (int)VWindow(screen));
#endif /* TEK */
        putenv(my_WINDOWID);
        /* put the display into the environment of the shell*/
        sprintf(my_DISPLAY, "DISPLAY=%s", XDisplayString(screen->display));
	putenv(my_DISPLAY);

	/* announce the GUI mode to the shell environment */
	sprintf(my_GUIMODE, "XGUI=%s", (OlGetGui() == OL_OPENLOOK_GUI)?
		"OPEN_LOOK" : "MOTIF");
	putenv(my_GUIMODE);
/* ehr-end */

	/* ehr3 - added check for failure */
	if ((screen->respond = openpty(command_to_exec, screen)) < 0) {
#if !defined(I18N)
		(void) fprintf(stderr, "Error: Open of pseudo-tty failed\n");
		perror("	Reason");
#else
		perror(OlGetMessage(screen->display,NULL, 0, OleNperror, OleTreason,
			OleCOlClientXtermMsgs, OleMperror_reason, NULL));
        	OlVaDisplayErrorMsg(screen->display, OleNpty, OleTopen,
				OleCOlClientXtermMsgs, OleMpty_open, NULL);
#endif
		exit(1);
	}

	if ( screen->respond != Xsocket + 1)
	{
		if (dup2(screen->respond, Xsocket + 1) == -1) {
#if !defined(I18N)
			(void) fprintf(stderr, "Error: dup2(screen->respond), Xsocket + 1) failed\n");
			(void) perror("	Reason");
			(void) fflush(stderr);
#else
		perror(OlGetMessage(screen->display, NULL, 0, OleNperror, OleTreason,
			OleCOlClientXtermMsgs, OleMperror_reason, NULL));
        	OlVaDisplayErrorMsg(screen->display, OleNdup2, OleTbadDup2Msg2,
				OleCOlClientXtermMsgs, OleMdup2_badDup2Msg2, NULL);
#endif
			exit(1);
		}
		close(screen->respond);
		screen->respond = Xsocket + 1;
	}
}


Exit(n)
int n;
{
	register TScreen *screen = &term->screen;
        int pty = term->screen.respond;  /* file descriptor of pty */
#ifdef DUMMY
#ifdef UTMP
	struct utmp utmp;
	struct utmp *utptr;

	/* cleanup the utmp entry we forged earlier */
	/* unlike BSD, go ahead and cream any entries we didn't make */

	utmp.ut_type = USER_PROCESS;
	(void) strncpy(utmp.ut_id, ttydev + strlen(ttydev) - 2,
	 sizeof(utmp.ut_id));
	(void) setutent();
	utptr = getutid(&utmp);
	if (utptr) {
		utptr->ut_type = DEAD_PROCESS;
		utptr->ut_time = time((long *) 0);
		(void) pututline(utptr);
	}
	(void) endutent();
#endif	/* UTMP */
#endif

        close(pty); /* close explicitly to avoid race with slave side */
	if(screen->logging)
		CloseLog(screen);

	if(!get_ty && !am_slave) {
		/* restore ownership of tty */
		chown (ttydev, 0, 0);

		/* restore modes of tty */
		chmod (ttydev, 0666);
	}
	exit(n);
}


static void reapchild (dummy)
int dummy;
{
#if defined(SYSV) && !defined(JOBCONTROL)
	int status, pid;

	pid = wait(&status);
	if (pid == -1)
		return;
#else	/* defined(SYSV) && !defined(JOBCONTROL) */
	union wait status;
	register int pid;
	
#ifdef DEBUG
	if (debug) fputs ("Exiting\n", stderr);
#endif	/* DEBUG */
	pid  = wait3 (&status, WNOHANG, (struct rusage *)NULL);
	if (!pid) return;
#endif	/* defined(SYSV) && !defined(JOBCONTROL) */
	if (pid != term->screen.pid) return;
	
#if defined(SYSV) && !defined(JOBCONTROL)
	if (report_child_status)
	    Exit(WEXITSTATUS(status));
	else
	    Exit(0);
#else
	Exit(0);
#endif
}


/* ROSS */
static void
IgnoreWarnings(message)
        char *                  message;
{
        /* do nothing */
}
/* ROSS-end */

/*
 * parse_tty_modes accepts lines of the following form:
 *
 *         [SETTING] ...
 *
 * where setting consists of the words in the modelist followed by a character
 * or ^char.
 */
static int parse_tty_modes (s, modelist)
    char *s;
    struct _xttymodes *modelist;
{
    struct _xttymodes *mp;
    int c;
    int count = 0;

    while (1) {
	while (*s && isascii(*s) && isspace(*s)) s++;
	if (!*s) return count;

	for (mp = modelist; mp->name; mp++) {
	    if (strncmp (s, mp->name, mp->len) == 0) break;
	}
	if (!mp->name) return -1;

	s += mp->len;
	while (*s && isascii(*s) && isspace(*s)) s++;
	if (!*s) return -1;

	if (*s == '^') {
	    s++;
	    c = ((*s == '?') ? 0177 : *s & 31);	 /* keep control bits */
	} else {
	    c = *s;
	}
	mp->value = c;
	mp->set = 1;
	count++;
	s++;
    }
}
