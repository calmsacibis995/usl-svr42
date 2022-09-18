/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:scrollbar/unit_test3.c	1.9"
#endif

#ifndef lint
static char rcsid[] = "$Header: xscroll.c,v 1.6 88/02/14 15:13:11 rws Exp $";
#endif /*lint */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/BulletinBo.h>
#include <Xol/Scrollbar.h>


/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

char *ProgramName;
static Widget toplevel, bboard;
static Widget scrollWidget[3];


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
    int i;
    static Arg arg[1];

    if (w == scrollWidget[0]) {
	XtSetArg(arg[0],XtNforeground, call_data->new_location % 16);
    	XtSetValues(scrollWidget[2], arg, 1);
    }
    else if (w == scrollWidget[1]) {
	XtSetArg(arg[0],XtNbackground, call_data->new_location % 16);
    	XtSetValues(scrollWidget[2], arg, 1);
    }
    call_data->ok = TRUE;
}


void main(argc, argv)
    unsigned int argc;
    char **argv;
{

    static Arg args[] = {
	{XtNwidth,300},
	{XtNheight,300},
	{XtNproportionLength, 1},
	{XtNforeground, 4},
	{XtNbackground, 6},
	{XtNsliderMax, 100},
    };


    toplevel = OlInitialize( NULL, "Demo", NULL, 0,
			     &argc, argv );

    if (argc != 1) Syntax ();

    bboard = XtCreateManagedWidget ( "bboard",
		bulletinBoardWidgetClass, toplevel,
		(ArgList)args, 2);

    args[0].value = 20;
    args[1].value = 200;
    args[3].value = 1 << DefaultDepthOfScreen(XtScreen(toplevel));
    scrollWidget[0] = XtCreateManagedWidget( "scroll", scrollbarWidgetClass, bboard,
			   (ArgList)args, 4);
    scrollWidget[1] = XtCreateManagedWidget( "scroll", scrollbarWidgetClass, bboard,
			   (ArgList)args, 4);
    scrollWidget[2] = XtCreateManagedWidget( "scroll", scrollbarWidgetClass, bboard,
			   (ArgList)args, 3);
    XtMoveWidget (scrollWidget[0], 0, 0);
    XtMoveWidget (scrollWidget[1], 25, 0);
    XtMoveWidget (scrollWidget[2], 50, 0);
    XtAddCallback(scrollWidget[0] ,XtNsliderMoved,Drag,NULL);
    XtAddCallback(scrollWidget[1] ,XtNsliderMoved,Drag,NULL);
    XtAddCallback(scrollWidget[2] ,XtNsliderMoved,Drag,NULL);
    XtRealizeWidget(toplevel);
XtSetArg(args[0], XtNbackground, 6);
XtSetArg(args[1], XtNforeground, 4);
XtSetValues (scrollWidget[0], args, 2);
XtSetValues (scrollWidget[1], args, 2);
XtSetValues (scrollWidget[2], args, 2);
    XtMainLoop();
}
