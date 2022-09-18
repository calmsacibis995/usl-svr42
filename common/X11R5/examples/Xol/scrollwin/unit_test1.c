/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:scrollwin/unit_test1.c	1.16"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLookP.h>
#include <Xol/OblongButt.h>
#include <Xol/BulletinBP.h>
#include <Xol/ScrollbarP.h>
#include <Xol/ScrolledWP.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>
#include <Xol/CheckBox.h>
#include "ut.bitmap"


static Widget	CreateButton();
static Widget	CreateCheckBox();
static void	Destroy();
static void	Create();
static void	Viewport();
static void	Info();
static void	ForceHSB();
static void	ForceVSB();
static void	Newbutton();
static void	HSBMoved();
static void	VSBMoved();

static XtCallbackRec	hsb_moved[] =
{
  {HSBMoved, (caddr_t)NULL},
  {NULL, (caddr_t)NULL}
};

static Widget child = NULL;
static Widget swindow;
static int textborderwidth = 1;
static Boolean forceh = FALSE;
static Boolean forcev = FALSE;

static char	string[] = "line 1\nline 2\nline 3\n\
This text has no particular meaning.  Its sole \
purpose in life it to provide something for me to put inside of this \
static text widget which in turn only exists so that I can put something \
inside of the scrolled window widget that I am currently testing.  So you \
can just ignore me.\nThe above template should be located at the top of \
each file to be easily accessible by the file programmer. If this is \
included at the top of te file, this comment shouild follow since formatting \
and editing shell scripts look for special delimiters.\nThe purpose of this \
text is to fill up the scrolled window so that there is something to scroll.\
But if the window does not exist, so is this text.\n\
The source code for the tool 'cprint' is contained in the file\n\
To unpack and uncrypt this file, execute the \n\
following:\n\
\n\
crypt key < cprint00 | cpio -icdv \n\
\n\
\n\
\nbin/		src/\nblank\nlast line";

usage()
{
	printf("usage: sw1 [x#]		initial x\n");
	printf("           [y#]		initial y\n");
	printf("           [b#]		SBbackground color\n");
	printf("           [f#]		SBforeground color\n");
	printf("           [fh]		force hor. scrollbar\n");
	printf("           [fv]		force ver. scrollbar\n");
	printf("           [sh#]	step size for hor. scrollbar\n");
	printf("           [sv#]	step size for ver. scrollbar\n");
	printf("           [rh]		recompute for hor. scrollbar\n");
	printf("           [rv]		recompute for ver. scrollbar\n");
	printf("           [vh#]	view height\n");
	printf("           [vw#]	view width\n");
	printf("           [t]		tiling\n");
	printf("           [c]		childless\n");
	printf("           [B#]		child border width\n");
	printf("           [A]		add button to scrollbar menu\n");
	printf("           [-*]		all other X options\n");
	exit(0);
}

main(argc, argv)
  int	argc;
  char	*argv[];
{
  Arg		arg[25];
  XColor	blue;
  XColor	exact_blue;
  unsigned	n;
  int		screen;
  Widget	text;
  Widget	toplevel;
  Widget	toplevel1;
  Widget	bboard;
  Widget	controlarea;
  Widget	hsb,vsb;
  Pixel		white;
  Boolean	hrecompute = TRUE;
  Boolean	vrecompute = TRUE;
  int		initx = 0;
  int		inity = 0;
  int		hstep = 1;
  int		vstep = 1;
  int		hview = -1;
  int		vview = -1;
  int		fg = 4;
  int		bg = 6;
  int		tiling = 0;
  int		childl = 1;
  int		addbutton = 0;
  Pixmap	pmap;

  while (argc > 1) {
	switch(*argv[1]) {
	case 'x':
		/* initial x */
		initx = atoi(argv[1]+1);
		break;
	case 'y':
		/* initial y */
		inity = atoi(argv[1]+1);
		break;
	case 'b':
		/* background color */
		bg = atoi(argv[1]+1);
		break;
	case 'f':
		/* force scrollbar */
		if (*(argv[1]+1) == 'h')
			forceh = TRUE;
		else if (*(argv[1]+1) == 'v')
			forcev = TRUE;
		else
			/* foreground color */
			fg = atoi(argv[1]+1);
		break;
	case 's':
		/* step size */
		if (*(argv[1]+1) == 'h')
			hstep = atoi(argv[1]+2);
		else
			vstep = atoi(argv[1]+2);
		break;
	case 'r':
		/* recompute */
		if (*(argv[1]+1) == 'h')
			hrecompute = FALSE;
		else
			vrecompute = FALSE;
		break;
	case 'c':
		childl = 0;
		break;
	case 'v':
		/* view */
		if (*(argv[1]+1) == 'h')
			vview = atoi(argv[1]+2);
		else
			hview = atoi(argv[1]+2);
		break;
	case 'A':
		/* add button */
		addbutton = 1;
		break;
	case 'B':
		/* child border width */
		textborderwidth = atoi(argv[1]+1);
		break;
	case 't':
		tiling = 1;
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

  toplevel = OlInitialize("swtest", "SW Test", NULL, NULL, &argc, argv);
  
  if (tiling) {
	XWindowAttributes wattr;

	XtResizeWidget(toplevel, 320, 220, 1);
	XtRealizeWidget(toplevel);
	XGetWindowAttributes(XtDisplay(toplevel),
				RootWindowOfScreen(XtScreen(toplevel)), &wattr);
	pmap = XCreatePixmapFromBitmapData(XtDisplay(toplevel),
				RootWindowOfScreen(XtScreen(toplevel)),
				ut_bits,ut_width,ut_height,
				0, 1, wattr.depth);
	XtSetArg(arg[0], XtNbackgroundPixmap, pmap);
	bboard = XtCreateManagedWidget("bboard",bulletinBoardWidgetClass,
			toplevel, (ArgList)arg,1);
	toplevel = bboard;
  }
  screen = DefaultScreen(toplevelDisplay);

  n = 0;
  XtSetArg(arg[n], XtNforceVerticalSB, forcev);				n++;
  XtSetArg(arg[n], XtNforceHorizontalSB, forceh);			n++;
  XtSetArg(arg[n], XtNhSliderMoved, hsb_moved);				n++;
  XtSetArg(arg[n], XtNinitialX, initx);					n++;
  XtSetArg(arg[n], XtNinitialY, inity);					n++;
  XtSetArg(arg[n], XtNrecomputeWidth, hrecompute);			n++;
  XtSetArg(arg[n], XtNrecomputeHeight, vrecompute);			n++;
  XtSetArg(arg[n], XtNvStepSize, vstep);				n++;
  XtSetArg(arg[n], XtNhStepSize, hstep);				n++;
  XtSetArg(arg[n], XtNx, 20);						n++;
  XtSetArg(arg[n], XtNy, 20);						n++;
  if (hview != -1) {
  	XtSetArg(arg[n], XtNviewWidth, hview);				n++;
  }
  if (vview != -1) {
  	XtSetArg(arg[n], XtNviewHeight, vview);				n++;
  }

  swindow = XtCreateManagedWidget("ScrolledWindow",
				  scrolledWindowWidgetClass,
				  toplevel,
				  arg,
				  n);

  n = 0;
  XtSetArg(arg[n], XtNhScrollbar, &hsb);			n++;
  XtSetArg(arg[n], XtNvScrollbar, &vsb);			n++;
  XtGetValues(swindow, arg, n);

  n = 0;
  XtSetArg(arg[n], XtNforeground, fg);				n++;
  XtSetArg(arg[n], XtNbackground, bg);				n++;
  XtSetValues(hsb, arg, n);
  XtSetValues(vsb, arg, n);

  if (addbutton) {
	Widget vpane;
	Widget hpane;

	XtSetArg(arg[0], XtNvMenuPane, &vpane);
	XtSetArg(arg[1], XtNhMenuPane, &hpane);
	XtGetValues(swindow, arg, 2);

	XtAddCallback( XtCreateManagedWidget("A NEW BUTTON",
			oblongButtonWidgetClass, vpane, NULL, NULL),
			XtNselect, (XtCallbackProc) Newbutton, (caddr_t)0);
	XtAddCallback( XtCreateManagedWidget("A NEW BUTTON",
			oblongButtonWidgetClass, hpane, NULL, NULL),
			XtNselect, (XtCallbackProc) Newbutton, (caddr_t)1);
  }

  XtAddCallback(swindow, XtNvSliderMoved, VSBMoved, (caddr_t)NULL);
  
  toplevel1 = XtCreateApplicationShell("TopLevelShell",topLevelShellWidgetClass,
				(ArgList)NULL,(Cardinal)0);

  n = 0;
  XtSetArg(arg[n], XtNlayoutType, OL_FIXEDROWS);		n++;
  XtSetArg(arg[n], XtNmeasure, 1);				n++;
  controlarea = XtCreateManagedWidget("control panel", controlAreaWidgetClass,
			toplevel1, arg, n);
  CreateButton(controlarea, "Destroy", Destroy, (caddr_t)swindow);
  CreateButton(controlarea, "Create", Create, (caddr_t)swindow);
  CreateButton(controlarea, "View Port", Viewport, (caddr_t)swindow);
  CreateButton(controlarea, "Info", Info, (caddr_t)swindow);
  CreateCheckBox(controlarea, "Force HSB", ForceHSB, (caddr_t)swindow, forceh);
  CreateCheckBox(controlarea, "Force VSB", ForceVSB, (caddr_t)swindow, forcev);

  if (childl) {
	Create(NULL,(caddr_t)swindow,NULL);
  }

  XtRealizeWidget(toplevel);
  XtRealizeWidget(toplevel1);

  XtMainLoop();

}	/* main() */

static void
Newbutton(w, client_data, call_data)
Widget w;
caddr_t client_data;
caddr_t call_data;
{
	printf("New %s button pressed\n",((int)client_data == 1) ?
		 "horizontal" : "vertical");
}

static Widget
CreateButton(parent, label, callback, client_data)
Widget parent;
String label;
XtCallbackProc callback;
caddr_t client_data;
{
	Arg arg[1];
	Widget button;

	XtSetArg(arg[0], XtNlabel, label);
	button = XtCreateManagedWidget("OblongButton", oblongButtonWidgetClass,
			parent, arg, (Cardinal)1);

	if (callback != (XtCallbackProc)NULL)
		XtAddCallback(button, XtNselect, callback, client_data);

	return(button);
} /* CreateButton() */

static Widget
CreateCheckBox(parent, label, callback, client_data, set)
Widget parent;
String label;
XtCallbackProc callback;
caddr_t client_data;
int set;
{
	Arg arg[2];
	Widget check;

	XtSetArg(arg[0], XtNlabel, label);
	XtSetArg(arg[1], XtNset, set);
	check = XtCreateManagedWidget("CheckBox", checkBoxWidgetClass,
			parent, arg, (Cardinal)2);

	if (callback != (XtCallbackProc)NULL) {
		XtAddCallback(check, XtNselect, callback, client_data);
		XtAddCallback(check, XtNunselect, callback, client_data);
	}

	return(check);
} /* CreateCheckBox() */

static void
Info(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
	printf("child.width=%d\n",(int)(child->core.width));
	printf("child.height=%d\n",(int)(child->core.height));
	printf("sw.width=%d\n",(int)(swindow->core.width));
	printf("sw.height=%d\n",(int)(swindow->core.height));
}

static void
Destroy(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
	if (child) {
  		XtDestroyWidget((Widget)child);
		child = NULL;
	}
}	/* Destroy() */

static void
Viewport(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
	Arg args[2];
	int viewwidth;
	int viewheight;

	XtSetArg(args[0], XtNviewWidth, &viewwidth);
	XtSetArg(args[1], XtNviewHeight, &viewheight);
	XtGetValues((Widget)client_data, args, 2);
	printf("viewWidth=%d\n",(int)viewwidth);
	printf("viewHeight=%d\n",(int)viewheight);
}

static void
ForceHSB(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
	Arg arg[1];
	char *label;
	
	if (forceh == FALSE)
		forceh = TRUE;
	else
		forceh = FALSE;

	XtSetArg(arg[0],XtNforceHorizontalSB,forceh);
	XtSetValues((Widget)client_data,arg,1);
}

static void
ForceVSB(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
	Arg arg[1];
	char *label;
	
	if (forcev == FALSE)
		forcev = TRUE;
	else
		forcev = FALSE;
	XtSetArg(arg[0],XtNforceVerticalSB,forcev);
	XtSetValues((Widget)client_data,arg,1);
}

static void
Create(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
	int n;
	Arg arg[10];

  	n = 0;
  	XtSetArg(arg[n], XtNborderWidth, textborderwidth);	n++;
  	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
  	XtSetArg(arg[n], XtNstring, string);			n++;
  	XtSetArg(arg[n], XtNwidth, 250);			n++;
  	child = XtCreateManagedWidget("StaticText", staticTextWidgetClass,
				 (Widget)client_data, arg, n);
} /* Create */

static void
HSBMoved(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
  OlScrollbarVerify	*olsbv;


  olsbv = (OlScrollbarVerify *)call_data;
  fprintf(stderr, "Horizontal scrollbar moved to %d\n", olsbv->new_location);
  fflush(stderr);

}	/* HSBMoved() */


static void
VSBMoved(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
  OlScrollbarVerify	*olsbv;


  olsbv = (OlScrollbarVerify *)call_data;
  fprintf(stderr, "Vertical scrollbar moved to %d\n", olsbv->new_location);
  fflush(stderr);

}	/* VSBMoved() */
