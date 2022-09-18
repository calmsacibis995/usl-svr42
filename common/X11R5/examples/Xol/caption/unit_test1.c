/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:caption/unit_test1.c	1.6"
#endif

#include	<stdio.h>
#include	<X11/IntrinsicP.h>
#include	<X11/Xlib.h>
#include	<X11/StringDefs.h>
#include	<Xol/OpenLook.h>
#include	<Xol/Caption.h>
#include	<Xol/OblongButt.h>

Widget	toplevel;

/*
**	sample driver for caption widget
*/

main(argc, argv)
unsigned int argc;
char **argv;
{
	static Arg	args[15];
	int	i;
	Widget	captionw;
	Widget	childw;
	int	position;
	int	alignment;

	if (argc < 2) {
		printf ("usage: %s caption [TBRL] [TBCRL [space [child?]]]\n",argv[0]);
		exit(1);
	}

	if (argc > 2)
	switch (*argv[2]) {
		case 'T' :
			position = OL_TOP;
			printf ("Caption is on top\n");
			break;
		case 'B' :
			position = OL_BOTTOM;
			printf ("Caption is on the bottom\n");
			break;
		case 'R' :
			position = OL_RIGHT;
			printf ("Caption is on the right\n");
			break;
		case 'L' :
		default:
			position = OL_LEFT;
			printf ("Caption is on the left\n");
			break;
	}

	if (argc > 3)
	switch (*argv[3]) {
		case 'T' :
			alignment = OL_TOP;
			printf ("Alignment is top\n");
			break;
		case 'B' :
			alignment = OL_BOTTOM;
			printf ("Alignment is bottom\n");
			break;
		case 'R' :
			alignment = OL_RIGHT;
			printf ("Alignment is right\n");
			break;
		case 'L' :
			alignment = OL_LEFT;
			printf ("Alignment is left\n");
			break;
		case 'C' :
		default:
			alignment = OL_CENTER;
			printf ("Alignment is centered\n");
			break;
	}

	toplevel = OlInitialize("TopLevel", "TopLevel", 0, 0, &argc, argv);

	i = 0;
	XtSetArg(args[i], XtNlabel, argv[1]);		i++;
	if (argc > 2)
		XtSetArg(args[i], XtNposition, position);	i++;
	if (argc > 3)
		XtSetArg(args[i], XtNalignment, alignment);	i++;
	if (argc > 4) {
		XtSetArg(args[i], XtNspace, atoi(argv[4]));	i++;
		printf ("spacing is %d\n",atoi(argv[4]));
	}

	captionw = XtCreateManagedWidget(	"caption widget",
						captionWidgetClass,
						toplevel,
						args,	
						i);

	if (argc > 5 && (*argv[5] == 'y' || *argv[5] == 'Y')) {
		printf ("Adding child\n");
		i = 0;
		XtSetArg(args[i], XtNlabel, "Little");	i++;
		XtSetArg(args[i], XtNstring, "default");	i++;
		childw = XtCreateManagedWidget(	"button1", 
					oblongButtonWidgetClass, 
					captionw, args, i);
	}
	else {
		printf ("No child\n");
	}

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
