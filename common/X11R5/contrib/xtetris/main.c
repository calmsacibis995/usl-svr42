/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:main.c	1.2"
#endif

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

#include <X11/IntrinsicP.h>
#include "defs.h"

static XtActionsRec actions[] = {
  {"Refresh", restore_canvas},
  {"ShowScore", show_score},
  {"Quit", quit_proc},
  {"Done", done_proc},
  {"EndGame", end_game},
  {"NewGame", restart_proc},
  {"Start", start_proc},
  {"Pause", pause_proc},
  {"MoveLeft", left_proc},
  {"MoveRight", right_proc},
  {"RotateCW", clock_proc},
  {"RotateCCW", anti_proc},
  {"Drop", fast_proc},
  {"Scores", print_high_scores},
  };

#define offset(field) XtOffset(struct resource_struct *, field)
#define soffset(field) XtOffset(struct shape_table *, field)

static XtResource Resources[] = {
{"foreground",	"Foreground",	XtRPixel,	sizeof(Pixel),
     offset(foreground),	XtRString,	XtDefaultForeground},
{"background",	"Background",	XtRPixel,	sizeof(Pixel),
     offset(background),	XtRString,	XtDefaultBackground},
{"eraseStipple","Bitmap",	XtRBitmap,	sizeof(Pixmap),
     offset(erasestipple),	XtRString,	"gray"},
{"useScoreFile",	"Boolean",	XtRBoolean,	sizeof(Boolean),
     offset(usescorefile),	XtRString,	"True"},
{"boxSize",	"BoxSize",	XtRDimension,	sizeof(Dimension),
     offset(boxsize),	        XtRString,	"16"},
{"speed",       "Speed",        XtRDimension,   sizeof(Dimension),
     offset(speed),             XtRString,      "10"},
{"scorefile",    "ScoreFile",   XtRString,      sizeof(String),
     offset(scorefile),         XtRString,      HIGH_SCORE_TABLE},
};
static XrmOptionDescRec Options[] = {
{"-score",	"useScoreFile",	 	      XrmoptionNoArg,	"TRUE"},
{"-noscore",	"useScoreFile",	 	      XrmoptionNoArg,	"FALSE"},
{"-speed",      "speed",                      XrmoptionSepArg,  NULL },
};

static XtResource ShapeResources[] = {
{"foreground",  "Foreground",   XtRPixel,       sizeof(Pixel),
   soffset(foreground),         XtRString,      XtDefaultForeground},
{"background",	"Background",	XtRPixel,	sizeof(Pixel),
   soffset(background),	XtRString,	XtDefaultBackground},
{"stipple",	"Bitmap",	XtRBitmap,	sizeof(Pixmap),
   soffset(stipple),	XtRString,	"gray"}

};

static char * fallback_resources[] = {
 NULL,
};

#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Simple.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>


main(argc, argv)
  int     argc;
  char  **argv;
{
  int     i, j;
  Arg  	  arg_new[2];

  Atom wm_delete_window;

  Widget 
    status, buttons, nextobjectlabel,
    tbar, scores_bt, quit_bt, done_bt;
    static XtConvertArgRec screenConvertArg[] = {
        {XtWidgetBaseOffset, (caddr_t) XtOffset(Widget, core.screen),
             sizeof(Screen *)}
	};
  
  toplevel = XtAppInitialize( &context, "Xtetris", 
			     Options, XtNumber(Options), &argc, argv, fallback_resources, 
				 NULL, 0 ); 

  XtAppAddConverter( context, "String", "Bitmap", XmuCvtStringToBitmap, 
		     screenConvertArg, XtNumber(screenConvertArg) );
  XtGetApplicationResources( toplevel, (caddr_t) &resources,
			    Resources, XtNumber(Resources),
			    NULL, (Cardinal) 0);
  XtAppAddActions( context, actions, XtNumber(actions) );
  
  /* Wanted to use objects, but couldn't figure out how to add new
     resource fields.  This seems easier. dgreen (?) */

  for (i = 0; i < 7; i++) {
    static char *names[7] = { "object0","object1","object2","object3","object4",
		       "object5","object6" };
    XtGetSubresources( toplevel, (caddr_t) &shape[i], names[i], "Object",
		      ShapeResources, XtNumber(ShapeResources),
		      NULL, (Cardinal) 0 );
    }
  frame = XtCreateManagedWidget("Frame", formWidgetClass, toplevel, NULL, (Cardinal)0);
  tbar = XtCreateManagedWidget("TitleBar", labelWidgetClass, frame, NULL, (Cardinal)0);
  status = XtCreateManagedWidget("Status", formWidgetClass, frame, NULL, (Cardinal)0);
  score_item = XtCreateManagedWidget("Score", labelWidgetClass, status, NULL, (Cardinal)0);
  level_item = XtCreateManagedWidget("Level", labelWidgetClass, status, NULL, (Cardinal)0);
  rows_item = XtCreateManagedWidget("Rows", labelWidgetClass, status, NULL, (Cardinal)0);
  game_over = XtCreateManagedWidget("Game", labelWidgetClass, status, NULL, (Cardinal)0);
  
  buttons = XtCreateManagedWidget("Buttons", formWidgetClass, frame, NULL, (Cardinal)0);
  start_bt = XtCreateManagedWidget("Start", commandWidgetClass, buttons, NULL, (Cardinal)0);
  pause_bt = XtCreateManagedWidget("Pause", commandWidgetClass, buttons, NULL, (Cardinal)0);
  newgame_bt = XtCreateManagedWidget("NewGame", commandWidgetClass, buttons, NULL, (Cardinal)0);
  quit_bt = XtCreateManagedWidget("Quit", commandWidgetClass, buttons, NULL, (Cardinal)0);
  if (resources.usescorefile)
    scores_bt = XtCreateManagedWidget("Scores", commandWidgetClass, buttons, NULL, (Cardinal)0);
  canvas = XtCreateManagedWidget("Canvas", simpleWidgetClass, frame, NULL, (Cardinal)0);
  shadow = XtCreateManagedWidget("Shadow", simpleWidgetClass, frame, NULL, (Cardinal)0);
  nextobjectlabel = XtCreateManagedWidget("NextObjectLabel", labelWidgetClass, frame, NULL, (Cardinal)0);
  nextobject = XtCreateManagedWidget("NextObject", simpleWidgetClass, frame, NULL, (Cardinal)0);
  
  if (resources.usescorefile) 
    {
      score_frame = XtCreatePopupShell("Score_frame", transientShellWidgetClass, toplevel, NULL,(Cardinal)0);
      score_panel = XtCreateManagedWidget("Score_panel", boxWidgetClass, score_frame, NULL, (Cardinal)0);
  
      for (j = 0; j < HIGH_TABLE_SIZE + 1; j++)
	high_score_item[j] = XtCreateManagedWidget("Score_item", labelWidgetClass, score_panel, NULL, (Cardinal)0);
      
      done_bt = XtCreateManagedWidget("Done", commandWidgetClass, score_panel, NULL, (Cardinal)0);
    }
  XtInstallAllAccelerators( canvas, toplevel );
  XtInstallAllAccelerators( shadow, toplevel );
  XtInstallAllAccelerators( nextobject, toplevel );
  XtInstallAllAccelerators( frame, toplevel );
  XtRealizeWidget(toplevel);
/*
 *	CHANGE # UNKNOWN
 *	FILE # main.c
 * 
 *  Honouring ICCCM WM_DELETE_WINDOW protocol.
 *	ENDCHANGE # UNKNOWN
 */
	wm_delete_window  = XInternAtom(XtDisplay(toplevel), 
			"WM_DELETE_WINDOW", False);
	XSetWMProtocols(XtDisplay(toplevel), XtWindow(toplevel),
			&wm_delete_window, 1);
	XtOverrideTranslations(toplevel,
			XtParseTranslationTable("<Message>WM_PROTOCOLS:Quit()") );

/*
 *	CHANGE # UNKNOWN
 *	FILE # main.c
 *  Mapping is not requirred after the Realize widget.
 *
 *  XtMapWidget(toplevel);
 *	ENDCHANGE # UNKNOWN
 */

/*
 *	CHANGE # UNKNOWN
 *	FILE # main.c
 *  On being resized,
 *		->	either objects should be drawn proportionately
 *		->	either number of small boxes that can be fitted in new width and
 *			height, should be changed.
 *		->	or do not allow resize of the client, by setting WMNormal Hints.
 *			Currently done this.
 *
 *	ENDCHANGE # UNKNOWN
 */
  {
  	XWindowAttributes win_attr;
  	XSizeHints 		 wm_hints;

	if (XGetWindowAttributes (XtDisplay(toplevel), XtWindow(toplevel),
		&win_attr) != 0)
	{
		wm_hints.min_width = win_attr.width;
		wm_hints.max_width = win_attr.width;
		wm_hints.min_height = win_attr.height;
		wm_hints.max_height = win_attr.height;
		wm_hints.flags |= PMinSize | PMaxSize;
	    XSetWMNormalHints (XtDisplay(toplevel), XtWindow(toplevel), 
				&(wm_hints));
	}
  }
  {
    XGCValues gcv;

    gcv.foreground = resources.foreground;
    gcv.background = resources.background;
    gc = XCreateGC( XtDisplay(toplevel),XtWindow(toplevel), (unsigned long) GCForeground|GCBackground, &gcv );

    gcv.foreground = resources.background;
    gcv.background = resources.foreground;
    gcv.fill_style = FillStippled;
    gcv.stipple = resources.erasestipple;
    erasegc = XCreateGC( XtDisplay(toplevel),XtWindow(toplevel), (unsigned long) GCForeground|GCBackground|GCStipple|GCFillStyle, &gcv );
  }

  initialise(); 
  XtAppMainLoop( context );
}
