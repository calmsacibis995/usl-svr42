/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:notice/unit_test2.c	1.2"
#endif

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/Notice.h>

popup(w, client_data, call_data)
    Widget	w;
    caddr_t	client_data;
    caddr_t	call_data;
{
    XtPopup((Widget)client_data, XtGrabExclusive);
}

/* Quit: "quit" button callback
 */
Quit(w, client_data, call_data)
    Widget w;
    caddr_t client_data, call_data;
{
    printf("bye \n");
    exit(0);
}

main(argc, argv)
    int		argc;
    char *	argv[];
{
    static Widget	toplevel;
    static Widget	box, verbose, terse, quit; /* driving widgets */
    static Widget	terse_notice, terse_ok, terse_cancel;
    static Widget	verbose_notice, verbose_ok, verbose_cancel;
    static Widget	control;
    static Arg		args[10];
    int			cnt;
    static Arg get_widgets[] = {
	{XtNcontrolArea, (XtArgVal)&control},
    };
    static String verbose_msg = "*              Notice Text \n\n \
this is what a typical notice might contain to \n \
alert the user of impending doom.  Check for \n \
center justification and for the centering of \n \
buttons below.  Can this text be selected?";

#ifdef _OL_NOTICE_DEBUG
    _OLnoticeDebug = (argc == 2) ? TRUE : FALSE;
#endif				/* _OL_NOTICE_DEBUG */

    toplevel = OlInitialize(NULL, "NOtice", NULL, 0, &argc, argv);

    box = XtCreateManagedWidget("box", controlAreaWidgetClass,
				toplevel, NULL, 0);
    verbose = XtCreateManagedWidget("verbose", oblongButtonWidgetClass,
				    box, NULL, 0);
    terse = XtCreateManagedWidget("terse", oblongButtonWidgetClass,
				  box, NULL, 0);
    quit = XtCreateManagedWidget("quit", oblongButtonWidgetClass,
				 box, NULL, 0);

    /* create terse notice */
    cnt = 0;
    XtSetArg(args[cnt], XtNstring, (XtArgVal)"Text:\n next\n line"); cnt++;
    XtSetArg(args[cnt], XtNemanateWidget, (XtArgVal)terse); cnt++;
    terse_notice = XtCreatePopupShell("terse_notice",
				      noticeShellWidgetClass, box,
				      args, cnt);

    /* get control area where buttons will live */
    XtGetValues(terse_notice, get_widgets, XtNumber(get_widgets));

    /* add buttons to notice */
    terse_ok = XtCreateManagedWidget("ok", oblongButtonWidgetClass,
				     control, NULL, 0);
    terse_cancel = XtCreateManagedWidget("cancel", oblongButtonWidgetClass,
					 control, NULL, 0);

    /* create terse notice */
    cnt = 0;
    XtSetArg(args[cnt], XtNstring, (XtArgVal)verbose_msg); cnt++;
    XtSetArg(args[cnt], XtNemanateWidget, (XtArgVal)verbose); cnt++;
    verbose_notice = XtCreatePopupShell("verbose_notice",
					noticeShellWidgetClass, box,
					args, cnt);

    /* get control area where buttons will live */
    XtGetValues(verbose_notice, get_widgets, XtNumber(get_widgets));

    /* add buttons to notice */
    verbose_ok = XtCreateManagedWidget("ok", oblongButtonWidgetClass,
				       control, NULL, 0);
    verbose_cancel = XtCreateManagedWidget("cancel", oblongButtonWidgetClass,
					   control, NULL, 0);

    /* attach callbacks to buttons */
    XtAddCallback(verbose, XtNselect, popup, verbose_notice);
    XtAddCallback(terse, XtNselect, popup, terse_notice);
    XtAddCallback(quit, XtNselect, Quit, NULL);

    XtRealizeWidget(toplevel);
    XtMainLoop();
}
