/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unittest21.c	1.3"
#endif

/*
 * This test tests MR# bl89-02712
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/Text.h>



static char string[] = "This text has no particular meaning.  Its sole \
purpose in life it to provide something for me to put inside of this \
static text widget which in turn only exists so that I can put something \
inside of the scrolled window widget that I am currently testing.  So you \
can just ignore me.";


void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, text;
	Arg arg[20];
	unsigned int n;


	toplevel = OlInitialize("unittest23",
		"Unittest23",
		NULL,
		0,
		&argc,
		argv);

	n = 0;
	XtSetArg(arg[n], XtNheight, (Cardinal)50);			n++;
	XtSetArg(arg[n], XtNstring, string);				n++;
#if 0
	XtSetArg(arg[n], XtNhorizontalSB, TRUE);			n++;
#endif
	XtSetArg(arg[n], XtNverticalSB, TRUE);				n++;
	text = XtCreateManagedWidget("Text",
		textWidgetClass,
		toplevel,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
