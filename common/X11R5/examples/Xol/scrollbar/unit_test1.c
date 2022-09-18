/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:scrollbar/unit_test1.c	1.9"
#endif
  
#ifndef lint
  static char rcsid[] = "$Header: xscroll.c,v 1.6 88/02/14 15:13:11 rws Exp $";
#endif /*lint*/

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/Scrollbar.h>
#include <Xol/OblongButt.h>
#include <Xol/BulletinBo.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <stdio.h>
#include "ut.bitmap"

char *ProgramName;
static Widget scrollWidget;
static Widget toplevel, bboard;

/*
 * Report the syntax for calling xcommand
 */
Syntax()
{
  fprintf( stderr, "Usage: %s\n", ProgramName );
}

void Drag(w, closure, call_data)
  Widget w;
  caddr_t closure;
  OlScrollbarVerify *call_data;
{
	static int count=0;

  call_data->ok = TRUE;
  call_data->new_page = call_data->new_location * 10 + 1;

  /*printf( "Dragged to %d: %d: %d  %d%%\n", call_data->new_location, 
	 call_data->slidermin,
	 call_data->slidermax,
	 call_data->rel);*/

}

usage()
{
	printf("usage: unit_test1 [x#]		x position\n");
	printf("                  [y#]		y position\n");
	printf("                  [p#]		proportion indicator\n");
	printf("                  [w#]		width\n");
	printf("                  [h#]		height\n");
	printf("                  [b#]		background color\n");
	printf("                  [f#]		foreground color\n");
	printf("                  [r#]		repeat rate\n");
	printf("                  [i#]		initial delay\n");
	printf("                  [M#]		slider max\n");
	printf("                  [m#]		slider min\n");
	printf("                  [g#]		granularity\n");
	printf("                  [o#]		orientation (v=0,h=1)\n");
	printf("                  [B#]		border width\n");
	printf("                  [s#]		showpage (none=0,left=3,right=4)\n");
	printf("                  [t]		tiling\n");
	printf("                  [W]		pointer warping\n");
	exit(0);
}

static void NewButton()
{printf("A New Button\n");}

void main(argc, argv)
  unsigned int argc;
  char **argv;
{
	int indlen = 25;
	Dimension width = 200;
	Dimension height = 200;
	int bg = 6;
	int fg = 4;
	int gran = 1;
	int repeat = 50;
	int idelay = 500;
	int max = 100;
	int min = 0;
	int orient = OL_VERTICAL;
	int showpage = OL_NONE;
	int tiling = 0;
	Position posx = 0;
	Position posy = 0;
	Dimension border = 0;
	int warp = True;
  	static Arg args[16];
	Pixmap pmap;
  	Arg queryMenuPane[1];
  	Widget pane;

  	ProgramName = argv[0];
	while (argc > 1) {
		switch(*argv[1]) {
		case 'p':
			/* proportion indicator length */
			indlen = atoi(argv[1]+1);
			break;
		case 'w':
			/* width */
			width = atoi(argv[1]+1);
			break;
		case 'h':
			/* height */
			height = atoi(argv[1]+1);
			break;
		case 'b':
			/* background color */
			bg = atoi(argv[1]+1);
			break;
		case 'f':
			/* foreground color */
			fg = atoi(argv[1]+1);
			break;
		case 'r':
			/* repeat rate */
			repeat = atoi(argv[1]+1);
			break;
		case 'i':
			/* initial delay */
			idelay = atoi(argv[1]+1);
			break;
		case 'M':
			max = atoi(argv[1]+1);
			break;
		case 'm':
			min = atoi(argv[1]+1);
			break;
		case 'g':
			gran = atoi(argv[1]+1);
			break;
		case 'o':
			orient = atoi(argv[1]+1);
			break;
		case 'B':
			border = atoi(argv[1]+1);
			break;
		case 's':
			showpage = atoi(argv[1]+1);
			break;
		case 'x':
			posx = atoi(argv[1]+1);
			break;
		case 'y':
			posy = atoi(argv[1]+1);
			break;
		case 't':
			tiling++;
			break;
		case 'W':
			if (warp == True)
				warp = False;
			else
				warp = True;
			break;
		case '?':
			/* usage */
			usage();
			break;
		case '-':
			goto skip;
		}
		argc--;
		argv++;
	}
  
  
skip:
	setbuf(stdout,NULL);

  toplevel = OlInitialize( NULL, "Demo", NULL, NULL, &argc, argv );
  
  if (tiling) {
	XWindowAttributes wattr;

	XtResizeWidget(toplevel, width, height, 1);
  	XtRealizeWidget(toplevel);
	XGetWindowAttributes(XtDisplay(toplevel),XtWindow(toplevel),&wattr);
	pmap = XCreatePixmapFromBitmapData(XtDisplay(toplevel),
				     XtWindow(toplevel),
				     ut_bits,ut_width,ut_height,
				     0, 1, wattr.depth);
	XtSetArg(args[0],XtNbackgroundPixmap,pmap);
        bboard = XtCreateManagedWidget("bboard",bulletinBoardWidgetClass,
				 toplevel, (ArgList)args, 1);
	toplevel = bboard;
  }
  if (argc != 1) Syntax ();
  
  if (orient == 1) {
	height = 20;
	if (posy == 0)
		posy = 100;
  }
  else {
	width = 20;
	if (posx == 0)
		posx = 100;
  }
  XtSetArg(args[0], XtNwidth, width);
  XtSetArg(args[1], XtNheight, height);
  XtSetArg(args[2], XtNproportionLength, indlen);
  XtSetArg(args[3], XtNbackground, bg);
  XtSetArg(args[4], XtNforeground, fg);
  XtSetArg(args[5], XtNgranularity, gran);
  XtSetArg(args[6], XtNrepeatRate, repeat);
  XtSetArg(args[7], XtNorientation, orient);
  XtSetArg(args[8], XtNinitialDelay, idelay);
  XtSetArg(args[9], XtNsliderMin, min);
  XtSetArg(args[10], XtNsliderMax, max);
  XtSetArg(args[11], XtNshowPage, showpage);
  XtSetArg(args[12], XtNx, posx);
  XtSetArg(args[13], XtNy, posy);
  XtSetArg(args[14], XtNborderWidth, border);
  XtSetArg(args[15], XtNpointerWarping, warp);
  
  scrollWidget = XtCreateManagedWidget( "scroll", scrollbarWidgetClass,
				       toplevel, (ArgList)args, 16);
  XtAddCallback(scrollWidget ,XtNsliderMoved,Drag,NULL);

  XtSetArg(queryMenuPane[0], XtNmenuPane, (XtArgVal) &pane);
  XtGetValues(scrollWidget, queryMenuPane, 1);
  XtAddCallback( XtCreateManagedWidget("A NEW BUTTON",
				       oblongButtonWidgetClass,
				       pane,
				       NULL,
				       NULL),
		XtNselect,
		(XtCallbackProc) NewButton,
		NULL);

  XtRealizeWidget(toplevel);
  XtMainLoop();
}
