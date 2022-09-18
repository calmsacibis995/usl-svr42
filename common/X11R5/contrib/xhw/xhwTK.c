#ifndef	NOIDENT
#ident	"@(#)r4xhw:xhwTK.c	1.1"
#endif
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 xhwTK.c (C source file)
	Acc: 581626651 Mon Jun  6 14:57:31 1988
	Mod: 568016787 Fri Jan  1 01:26:27 1988
	Sta: 581622145 Mon Jun  6 13:42:25 1988
	Owner: 5815
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Label.h>

#define	STRING	"Hello,  World"

Arg wargs[] = {
    XtNlabel,	(XtArgVal) STRING,
};

main(argc, argv)
    int argc;
    char **argv;
{
    Widget      toplevel, label;

    /*
     * Create the Widget that represents the window.
     * See Section 14 of the Toolkit manual.
     */
    toplevel = XtInitialize(argv[0], "XLabel", NULL, 0, &argc, argv);

    /*
     * Create a Widget to display the string,  using wargs to set
     * the string as its value.  See Section 9.1.
     */
    label = XtCreateWidget(argv[0], labelWidgetClass,
			   toplevel, wargs, XtNumber(wargs));

    /*
     * Tell the toplevel widget to display the label.  See Section 13.5.2.
     */
    XtManageChild(label);

    /*
     * Create the windows,  and set their attributes according
     * to the Widget data.  See Section 9.2.
     */
    XtRealizeWidget(toplevel);

    /*
     * Now process the events.  See Section 16.6.2.
     */
    XtMainLoop();
}
