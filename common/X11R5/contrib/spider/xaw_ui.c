/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:xaw_ui.c	1.2"

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
 *	@(#)xaw_ui.c	2.2	90/04/27
 *
 */

/*
 * Athena Widget interface to Spider
 */

#include	"defs.h"
#include	"globals.h"
#include	"xaw_ui.h"

#include	"spider.bm"

static XtAppContext	spider_con;
Widget		toplevel;
static Widget		table_w;
static Widget		panel;
static Widget		message;
Widget		file;
Widget		helptext = (Widget) 0;	/* catch un-created help 
					 * widget in get_selection() */
Widget		confirm_box;
Widget		confirm_label;

extern int	replayTime;
extern Bool	usebell;
extern Bool	confirm;
#ifdef ROUND_CARDS
extern Bool	round_cards;
#endif
extern int	deltamod;
extern char	*helpDir;

extern char	helpfiles[5][256];

struct	_resources	{
	Bool	confirm;
	Bool	usebell;
#ifdef ROUND_CARDS
	Bool	round_cards;
#endif
	int	replayTime;
	int	deltamod;
	char	*helpDir;
} app_resources;

#define	offset(field)	XtOffset (struct _resources *, field)
/*
 *	WIPRO : Neeti
 *	CHANGE # UNKNOWN
 *	FILE # xaw_ui.c
 * 	Default value of replayTime has been increased to 100000 (0.1  sec)
 *	as a low value can make the program go to sleep forever in case 
 *	the local ulseep fuction is used. (For machines other than SUN) 
 * 	Refer to function usleep in file util.c.
 *	ENDCHANGE # UNKNOWN
 */
static XtResource	resources[] =	{
	{ "confirm", "Confirm", XtRBoolean, sizeof(Boolean),
		offset(confirm), XtRString, "True" },
	{ "bell", "Bell", XtRBoolean, sizeof(Boolean),
		offset(usebell), XtRString, "True" },
	{ "replayTime", "ReplayTime", XtRInt, sizeof(int),
		offset(replayTime), XtRString, "100000" },
#ifdef	ROUND_CARDS
	{ "roundCards", "RoundCards", XtRBoolean, sizeof(Boolean),
		offset(round_cards), XtRString, "True" },
#endif
	{ "deltaMod", "DeltaMod", XtRInt, sizeof(int),
		offset(deltamod), XtRString, "1" },
	{ "helpDir", "HelpDir", XtRString, sizeof(char *),
		offset(helpDir), XtRString, HELPDIR },
};
#undef	offset

/* make the input simple */
static char	file_trans[] =
  "Ctrl<Key>M:		no-op() \n\
   Ctrl<Key>J:		no-op() \n\
   Ctrl<Key>O:		no-op() \n\
   Ctrl<Key>S:		no-op() \n\
   Ctrl<Key>R:		no-op() \n\
   Ctrl<Key>Z:		no-op() \n\
   Meta<Key>I:		no-op() \n\
   Meta<Key>Z:		no-op() \n\
   <Key>Down:		no-op() \n\
   <Key>Up:		no-op() \n\
   <Key>Return:		no-op() \n\
   <Key>Linefeed:	no-op() \n";


static String	default_resources[] = 	{
#ifndef SMALL_CARDS
	"*font:			*helvetica-bold-r-normal--14-140-*",
	"*helptext*font:	*helvetica-bold-r-normal--12-120*",
#else
	"*font:			*helvetica-bold-r-normal--10-100-*",
	"*helptext*font:	*helvetica-bold-r-normal--8-80*",
#endif
	"*input:		True",
	"*newgame.Label:	New Game",
	"*backupMenuButton.Label:	Backup",
	"*onemove.Label:	One Move",
	"*startover.Label:	Start Over",
	"*replay.Label:		Replay",
	"*expand.Label:		Expand",
	"*locate.Label:		Locate",
	"*score.Label:		Score",
	"*help.Label:		Help...",
	"*helpall.Label:	All",
	"*helpintro.Label:	Introduction",
	"*helprules.Label:	Rules",
	"*helpcontrols.Label:	Controls",
	"*helpexamples.Label:	Examples",
	"*helpmisc.Label:	Extras",
	"*helpsummary.Label:	Summary",
	"*helpexit.Label:	Close",
	"*fileMenuButton.Label:		File",
	"*save.Label:		Save in File",
	"*resumeFile.Label:	Resume from File",
	"*resumeSelection.Label:	Resume from Selection",
	"*name.Label:		Name:",
	"*filename*editType:	edit",
	"*confirmcancel.Label:	Cancel",
	"*confirmok.Label:		Ok",
/* this will only work with R4+ Xaw */
	"*panel.MenuButton.leftBitmap:	menu12",
	NULL,
};

static void
usage(arg)
char    *arg;
{
        if (arg)
                (void) fprintf(stderr,"spider: illegal argument %s\n", arg);
        (void) fprintf(stderr,
        "usage: -display <display> -geometry < geometry> -save <save_file>\n");
}

main(argc, argv)
int	argc;
char	**argv;
{
XWMHints	xwmh;
char	*save_file = NULL;
int	i, nargs;

	nargs = xaw_init(argc, argv);

	/* argument processing */
	for (i = 1; i < nargs; i++)      {
		if (strncmp(argv[i], "-s", 2) == 0)      {
			if (argv[i+1])  {
				save_file = argv[++i];
			} else  {
				usage(NULL);
				exit(-1);
			}
		} else	{
			usage(argv[1]);
			exit(-1);
		}
	}

	XtRealizeWidget(toplevel);

	table_init(XtWindow(table_w));
	card_init();

	if (save_file)
		read_file(save_file);

	XtAppMainLoop(spider_con);
}

#define	NUM_ARGS	15

xaw_init(argc, argv)
int	argc;
char	**argv;
{
XtTranslations translations;
Widget	new_button, back_button, expand_button, 
	locate_button, score_button, file_button, help_button;
Widget	outerbox;
Widget	file_menu, back_menu;
Widget	entry;
Arg	args[NUM_ARGS];
int	n;
int	mwin_h, bbox_height;
Pixel	backcolor;
Pixmap	icon_map;
int	scr;


	table_width = TABLE_WIDTH;
	table_height = TABLE_HEIGHT;
	mwin_h = 5;
	bbox_height = 30;

	n = 0;
	XtSetArg(args[n], XtNwidth, table_width);	n++;
	XtSetArg(args[n], XtNheight, table_height + bbox_height + mwin_h); n++;

	toplevel = XtAppInitialize(&spider_con, "Spider",
		NULL, 0,
		&argc, argv, default_resources, args, n);

	XtGetApplicationResources(toplevel, (caddr_t) &app_resources,
			resources, XtNumber(resources),
			NULL, (Cardinal) 0);

	usebell = app_resources.usebell;
	confirm = app_resources.confirm;
	replayTime = app_resources.replayTime;
#ifdef ROUND_CARDS
	round_cards = app_resources.round_cards;
#endif
	deltamod = app_resources.deltamod;
	helpDir = app_resources.helpDir;

	scr = XDefaultScreen(XtDisplay(toplevel));

#ifdef DEBUG
	XSynchronize(XtDisplay(toplevel), True);
#endif

	/* set the icon */
	icon_map = XCreateBitmapFromData(XtDisplay(toplevel),
		RootWindow(XtDisplay(toplevel), scr),
		(char *) spider_bits, spider_width, spider_height);
/*
 *  WIPRO : Neeti
 *  CHANGE # UNKNOWN
 *  FILE # xaw_ui.c
 *  spider_bits array has been type cast to (char *).
 *  
 *  ENDCHANGE # UNKNOWN
 */

	n = 0;
	XtSetArg(args[n], XtNiconPixmap, icon_map);	n++;
	XtSetValues(toplevel, args, n);


	/* get the pixel values, etc */
	gfx_init(XtDisplay(toplevel), scr);


	n = 0;
	XtSetArg(args[n], XtNsensitive, True);	n++;
	outerbox = XtCreateManagedWidget("outerbox", panedWidgetClass,
		toplevel, args, n);

	XtSetArg(args[0], XtNbackground, &backcolor);
	XtGetValues(outerbox, args, ONE);

	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNsensitive, True);	n++;
	XtSetArg(args[n], XtNinternalBorderWidth, 10);	n++;
	XtSetArg(args[n], XtNinternalBorderColor, backcolor);	n++;
	XtSetArg(args[n], XtNorientation, XtorientHorizontal); n++;
	panel = XtCreateManagedWidget("panel", panedWidgetClass, outerbox,
		args, n);

	/* add the buttons */

	/* newgame */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	new_button = XtCreateManagedWidget("newgame", commandWidgetClass,
		panel, args, n);
	
	XtAddCallback(new_button, XtNcallback, newgame_handler, NULL);

	/* backup menu */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	XtSetArg(args[n], XtNmenuName, "backupMenu");	n++;
	back_button = XtCreateManagedWidget("backupMenuButton", 
		menuButtonWidgetClass, panel, args, n);

	back_menu = XtCreatePopupShell("backupMenu", simpleMenuWidgetClass, 
		back_button, NULL, ZERO);

	entry = XtCreateManagedWidget("onemove", smeBSBObjectClass, back_menu,
		NULL, ZERO);
	XtAddCallback(entry, XtNcallback, backup_handler, (XtPointer) 0);
	entry = XtCreateManagedWidget("startover", smeBSBObjectClass, back_menu,
		NULL, ZERO);
	XtAddCallback(entry, XtNcallback, backup_handler, (XtPointer) 1);
	entry = XtCreateManagedWidget("replay", smeBSBObjectClass, back_menu,
		NULL, ZERO);
	XtAddCallback(entry, XtNcallback, backup_handler, (XtPointer) 2);
	
/*
 *  WIPRO : Neeti
 *  CHANGE # UNKNOWN
 *  FILE # xaw_ui.c
 *  The last argument to XtAddCallback() has been casted as XtPointer 
 *  AT ALL PLACES where a constant literal value was being passed .
 *  
 *  ENDCHANGE # UNKNOWN
 */


	/* Expand button */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	expand_button = XtCreateManagedWidget("expand", commandWidgetClass,
		panel, args, n);
	
	XtAddCallback(expand_button, XtNcallback, expand_handler, NULL);

	/* Locate button */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	locate_button = XtCreateManagedWidget("locate", commandWidgetClass,
		panel, args, n);
	
	XtAddCallback(locate_button, XtNcallback, locate_handler, NULL);

	/* Score button */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	score_button = XtCreateManagedWidget("score", commandWidgetClass,
		panel, args, n);

	XtAddCallback(score_button, XtNcallback, score_handler, NULL);

	/* Help button */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	if (!can_get_help_files(helpfiles))	{
		XtSetArg(args[n], XtNsensitive, False);	n++;
	}
	help_button = XtCreateManagedWidget("help", commandWidgetClass,
		panel, args, n);

	XtAddCallback(help_button, XtNcallback, help_handler, NULL);

	/* File menu */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNmenuName, "fileMenu");	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	file_button = XtCreateManagedWidget("fileMenuButton", 
		menuButtonWidgetClass, panel, args, n);

	file_menu = XtCreatePopupShell("fileMenu", simpleMenuWidgetClass, 
		file_button, NULL, ZERO);

	entry = XtCreateManagedWidget("save", smeBSBObjectClass, file_menu,
		NULL, ZERO);
	XtAddCallback(entry, XtNcallback, file_handler, (XtPointer) 0);
	entry = XtCreateManagedWidget("resumeFile", smeBSBObjectClass, 
		file_menu, NULL, ZERO);
	XtAddCallback(entry, XtNcallback, file_handler, (XtPointer) 1);
	entry = XtCreateManagedWidget("resumeSelection", smeBSBObjectClass,
		file_menu, NULL, ZERO);
	XtAddCallback(entry, XtNcallback, file_handler, (XtPointer) 2);

	/* File label */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	file_button = XtCreateManagedWidget("name", 
		labelWidgetClass, panel, args, n);
	
	/* file name entry field */
	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);	n++;
	XtSetArg(args[n], XtNskipAdjust, False);	n++;
	assert (n <= NUM_ARGS);
	file = XtCreateManagedWidget("filename", asciiTextWidgetClass,
		panel, args, n);
	translations = XtParseTranslationTable(file_trans);
	XtOverrideTranslations(file, translations);

	/* build the table itself */
	n = 0;
	XtSetArg(args[n], XtNx, 0);	n++;
	XtSetArg(args[n], XtNy, bbox_height);	n++;
	XtSetArg(args[n], XtNwidth, table_width - 2 * TABLE_BW);	n++;
	XtSetArg(args[n], XtNheight, table_height);	n++;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNskipAdjust, False);	n++;
	assert (n <= NUM_ARGS);
	if (is_color)	{
		XtSetArg(args[n], XtNbackground, greenpixel);	n++;
	} else	{
		XtSetArg(args[n], XtNbackgroundPixmap, greenmap);	n++;
	}
	table_w = XtCreateManagedWidget("table", simpleWidgetClass, outerbox,
		args, n);
	XtAddEventHandler(table_w, ExposureMask, False, xaw_redraw_table, 
					NULL);
	XtAddEventHandler(table_w, ButtonPressMask, False, xaw_button_press, 
					NULL);
	XtAddEventHandler(table_w, ButtonReleaseMask, False, xaw_button_release,
					NULL);
	XtAddEventHandler(table_w, KeyPressMask, False, xaw_key_press,
					NULL);
	XtAddEventHandler(table_w, StructureNotifyMask, False, xaw_resize,
					NULL);

	n = 0;
	XtSetArg(args[n], XtNx, 0);	n++;
	XtSetArg(args[n], XtNy, table_height - 2 * TABLE_BW - mwin_h); n++;
	XtSetArg(args[n], XtNwidth, table_width - 2 * TABLE_BW);	n++;
	XtSetArg(args[n], XtNheight, mwin_h);	n++;
	XtSetArg(args[n], XtNjustify, XtJustifyLeft);	n++;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNskipAdjust, True);	n++;
	assert (n <= NUM_ARGS);
	message = XtCreateManagedWidget("message", labelWidgetClass,
		outerbox, args, n);
	/* set initial contents to version */
	print_version();

	create_confirmer();

	return argc;
}

create_confirmer()
{
Arg	args[NUM_ARGS];
int	n = 0;
Widget	confirmbox, confirmpane;
Widget	button;

	/* create the confirmer box */
	confirm_box = XtCreatePopupShell("prompt", transientShellWidgetClass,
		toplevel, args, n);

	n = 0;
	confirmpane = XtCreateManagedWidget("confirmbox", panedWidgetClass,
		confirm_box, args, n);

	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	confirm_label = XtCreateManagedWidget("confirmlabel", labelWidgetClass,
		confirmpane, args, n);

	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNorientation, XtorientHorizontal);	n++;
	confirmbox = XtCreateManagedWidget("confirmbox", boxWidgetClass,
		confirmpane, args, n);

	n = 0;
	button = XtCreateManagedWidget("confirmok", commandWidgetClass,
		confirmbox, args, n);
	XtAddCallback(button, XtNcallback, confirm_callback, (XtPointer)1);

	n = 0;
	button = XtCreateManagedWidget("confirmcancel", commandWidgetClass,
		confirmbox, args, n);
	XtAddCallback(button, XtNcallback, confirm_callback,(XtPointer) 2);
}


void
show_message(buf)
char	*buf;
{
Arg	args[1];

	XtSetArg(args[0], XtNlabel, buf);
	XtSetValues(message, args, ONE);
}

/*
 * since the Label needs to get events to update, it doesn't update until
 * we hit the event loop.  this forces it out when we have to
 * have it immediatly, like with the Expand feature.
 */
flush_message()
{
XEvent	event;

	/* force the result to be seen immediately */
	while ( !XCheckTypedWindowEvent(XtDisplay(toplevel),
				  XtWindow(message),
				  Expose, &event) );
	XtDispatchEvent( &event );
	XFlush(XtDisplay(toplevel));
}

Widget
create_help_popup(fname)
char	*fname;
{
Widget	help, helpbox, helpform, helpview;
Widget	button;
int	n;
Arg	args[10];

	n = 0;
	help = XtCreatePopupShell("Spider Help", topLevelShellWidgetClass,
		toplevel, args, n);

	n = 0;
	helpbox = XtCreateManagedWidget("helpbox", panedWidgetClass,
		help, args, n);

	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNorientation, XtorientHorizontal);	n++;
	helpform = XtCreateManagedWidget("helpform", boxWidgetClass,
		helpbox, args, n);

	n = 0;
	button = XtCreateManagedWidget("helpintro", commandWidgetClass,
		helpform, args, n);
	XtAddCallback(button, XtNcallback, change_help,(XtPointer) 1);

	n = 0;
	button = XtCreateManagedWidget("helprules", commandWidgetClass,
		helpform, args, n);
	XtAddCallback(button, XtNcallback, change_help, (XtPointer)2);

	n = 0;
	button = XtCreateManagedWidget("helpcontrols", commandWidgetClass,
		helpform, args, n);
	XtAddCallback(button, XtNcallback, change_help,(XtPointer) 3);

	n = 0;
	button = XtCreateManagedWidget("helpexamples", commandWidgetClass,
		helpform, args, n);
	XtAddCallback(button, XtNcallback, change_help, (XtPointer)4);

	n = 0;
	button = XtCreateManagedWidget("helpmisc", commandWidgetClass,
		helpform, args, n);
	XtAddCallback(button, XtNcallback, change_help,(XtPointer) 5);

	n = 0;
	button = XtCreateManagedWidget("helpsummary", commandWidgetClass,
		helpform, args, n);
	XtAddCallback(button, XtNcallback, change_help,(XtPointer) 6);

	n = 0;
	button = XtCreateManagedWidget("helpexit", commandWidgetClass,
		helpform, args, n);
	XtAddCallback(button, XtNcallback, change_help,(XtPointer) 7);

	n = 0;
	XtSetArg(args[n], XtNshowGrip, False);	n++;
	XtSetArg(args[n], XtNallowHoriz, True);	n++;
	XtSetArg(args[n], XtNallowVert, True);	n++;
	helpview = XtCreateManagedWidget("helpview", viewportWidgetClass,
		helpbox, args, n);

	n = 0;
	XtSetArg(args[n], XtNheight, 300);	n++;
	XtSetArg(args[n], XtNwidth, 550);	n++;
	XtSetArg(args[n], XtNtype, XawAsciiFile);	n++;
	XtSetArg(args[n], XtNstring, fname);	n++;
	XtSetArg(args[n], XtNwrap, XawtextWrapLine);	n++;
	XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded);	n++;
	helptext = XtCreateManagedWidget("helptext", asciiTextWidgetClass,
		helpview, args, n);

	return help;
}
