/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:scrollbar/unit_test2.c	1.9"
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
static Widget scrollWidget[2];


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
    static Arg arg[] = {
	{XtNsliderValue,100},
    };

    i = (w==scrollWidget[0])?1 : 0;
    arg[0].value = call_data->new_location;
#ifdef DEBUG
printf("moving to %d\n",call_data->new_location);
#endif
    XtSetValues (scrollWidget[i], arg, 1);
    call_data->ok = TRUE;
}

void Page(w, closure, call_data)
    Widget w;
    caddr_t closure;
    OlScrollbarVerify *call_data;
{
    int i;
    static Arg arg[] = {
	{XtNsliderValue,100},
    };

    i = (w==scrollWidget[0])?1 : 0;
    arg[0].value = 100 - call_data->new_location;
    XtSetValues (scrollWidget[i], arg, 1);
    call_data->ok = TRUE;
}


void main(argc, argv)
    unsigned int argc;
    char **argv;
{

    static Arg args[] = {
	{XtNwidth,50},
	{XtNheight,200},
	{XtNproportionLength, 25},
    };

    ProgramName = argv[0];

  toplevel = OlInitialize( NULL, "Demo", NULL, NULL, &argc, argv );

    if (argc != 1) Syntax ();

    bboard = XtCreateManagedWidget ( "bboard",
		bulletinBoardWidgetClass, toplevel,
		(ArgList)args, 2);

    args[0].value = 50;
    args[1].value = 200;
    scrollWidget[0] = XtCreateManagedWidget( "scroll", scrollbarWidgetClass, bboard,
			   (ArgList)args, 3);
    scrollWidget[1] = XtCreateManagedWidget( "scroll", scrollbarWidgetClass, bboard,
			   (ArgList)args, 3);
    XtMoveWidget (scrollWidget[0], 0, 0);
    XtMoveWidget (scrollWidget[1], 100, 0);
    XtAddCallback(scrollWidget[0] ,XtNsliderMoved,Drag,NULL);
    XtAddCallback(scrollWidget[1] ,XtNsliderMoved,Drag,NULL);
    XtRealizeWidget(toplevel);
    XtMainLoop();
}
