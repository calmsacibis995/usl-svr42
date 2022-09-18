/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:control/unit_test1.c	1.11"
#endif

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/OblongButt.h>
#include <X11/StringDefs.h>

Widget	toplevel;

int mode;

static char *names[] = {
	{"We"},
	{"the People"},
	{"of the United States of"},
	{"America"},
	{"in Order to Form a"},
	{"More Perfect"},
	{"Union"},
};

static char *captions[] = {
	{"one:"},
	{"two:"},
	{"three:"},
	{"four:"},
	{"five:"},
	{"six:"},
	{"seven:"},
};

/*
**	sample driver for control panel widget
*/
main(argc, argv)
unsigned int argc; 
char **argv; 
{

	static Arg args[15];
	int i;
	Widget	controlw;		/* control panel */
	int	pad, space, size;
	Boolean	alignment = False;
	Boolean	center = False;

	if (argc < 2) {
		printf ("usage: %s [HWRC] [measure [pad [space [alignment? [center?]]]]\n",argv[0]);
		exit (1);
	}

	switch (*argv[1]) {
		case 'H' :
			mode = OL_FIXEDHEIGHT;
			break;
		case 'W' :
			mode = OL_FIXEDWIDTH;
			break;
		case 'R' :
			mode = OL_FIXEDROWS;
			break;
		case 'C' :
			mode = OL_FIXEDCOLS;
			break;
		default:
			mode = OL_FIXEDWIDTH;
			size = 0;
			break;
	}

	toplevel = OlInitialize("TopLevel", "TopLevel", 0, 0, &argc, argv);

	size = atoi(argv[2]);
	pad = atoi(argv[3]);
	space = atoi(argv[4]);
	if (argc >= 5 && (*argv[5] == 'y' || *argv[5] == 'Y'))
		alignment = True;
	else
		alignment = False;

	if (argc >= 6 && (*argv[6] == 'y' || *argv[6] == 'Y'))
		center = True;
	else
		center = False;

	printf ("Measure = %d, Pad = %d, Space = %d, Aligncaptions %s, Centering %s\n",
			size, pad, space, 
			alignment ? "on" : "off",
			center ? "on" : "off");

	i = 0;
	XtSetArg(args[i], XtNmeasure, size);	i++;
	XtSetArg(args[i], XtNlayoutType, mode);	i++;
	XtSetArg(args[i], XtNhSpace, space);	i++;
	XtSetArg(args[i], XtNvSpace, space);	i++;
	XtSetArg(args[i], XtNhPad, pad);	i++;
	XtSetArg(args[i], XtNvPad, pad);	i++;
	XtSetArg(args[i], XtNalignCaptions, alignment);	i++;
	XtSetArg(args[i], XtNcenter, center);	i++;
	XtSetArg(args[i], XtNsameSize, OL_NONE);	i++;
	XtSetArg(args[i], XtNborderWidth, 10);	i++;

	controlw = XtCreateManagedWidget(	"control panel",
						controlAreaWidgetClass,
						toplevel,
						args,	
						i);

	PopulateControlPanel(controlw);

	XtRealizeWidget(toplevel);

	XtMainLoop();
}


PopulateControlPanel(parent)
Widget	parent;
{
	int j = 0;
	int i = 0;
	Widget	w, captionw;
	static Arg args[15];


	for (j = 0; j < 7; j++) {
		i = 0;
	/*
		XtSetArg(args[i], XtNheight, 5 * j + 15);	i++;
		XtSetArg(args[i], XtNwidth, 15 * j + 5);	i++;
	*/
		XtSetArg(args[i], XtNlabel, captions[j]);		i++;
		XtSetArg(args[i], XtNborderWidth, 1);		i++;
		captionw = XtCreateManagedWidget(	
						captions[j],
						captionWidgetClass,
						parent,
						args,	
						i);

		i = 0;
		XtSetArg(args[i], XtNheight, 5 * j + 15);	i++;
		XtSetArg(args[i], XtNwidth, 15 * j + 5);	i++;
		w = XtCreateManagedWidget(	names[j],
						oblongButtonWidgetClass, 
						captionw,
						args,
						i);

	}
}
