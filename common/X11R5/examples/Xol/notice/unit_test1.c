/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:notice/unit_test1.c	1.20"
#endif
/*
 unit_test1.c (C source file)
	Acc: 596204511 Tue Nov 22 07:21:51 1988
	Mod: 596204511 Tue Nov 22 07:21:51 1988
	Sta: 596204511 Tue Nov 22 07:21:51 1988
	Owner: 4777
	Group: 1985
	Permissions: 664
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/Notice.h>


static String VerboseText =
"*              Notice Text \n\n \
this is what a typical notice might contain to \n \
alert the user of impending doom.  Check for \n \
center justification and for the centering of \n \
buttons below.  Can this text be selected?";

static String TerseText = "Text:\n next\n line";

static String GadgetText = 
"This Notice emanated from a gadget.  Though its \n \
parent is not the gadget button, it should still be \n \
positioned around the button.";
static Widget text;
static Widget notice;

PopItUp(w, msg, call_data)
	Widget	w;			/* emanating control */
	XtPointer	msg;			/* Notice text */
	XtPointer call_data;
{
	Arg text_arg, popup_arg;

	XtSetArg(text_arg, XtNstring, (XtArgVal)msg);
	XtSetValues(text, &text_arg, 1);

	XtSetArg(popup_arg, XtNemanateWidget, (XtArgVal)w);
	XtSetValues(notice, &popup_arg, 1);

	XtPopup(notice, XtGrabExclusive);
}

/* Quit: "quit" button callback
 */
Quit(w, client_data, call_data)
	Widget w;
	XtPointer client_data, call_data;
{
	printf("bye \n");
	exit(0);
}

main(argc, argv)
	int	argc;
	char	*argv[];
{
    static Widget toplevel, box;
    static Widget verbose, terse, gadget, quit;	/* driving widgets */
    static Widget control, ok, cancel;
    static Arg notice_args[] = {
	{XtNstring, (XtArgVal)"Text:\n next\n line"},
    };
    static Arg get_widgets[] = {
	{XtNtextArea, (XtArgVal)&text},
	{XtNcontrolArea, (XtArgVal)&control},
    };

#ifdef _OL_NOTICE_DEBUG
    extern Boolean _OLnoticeDebug;
    _OLnoticeDebug = (argc == 2) ? TRUE : FALSE;
#endif				/* _OL_NOTICE_DEBUG */

    toplevel = OlInitialize(NULL, "NOtice", NULL, 0, &argc, argv);

    box = XtCreateManagedWidget("box", controlAreaWidgetClass,
				toplevel, NULL, 0);
    verbose = XtCreateManagedWidget("verbose", oblongButtonWidgetClass,
				    box, NULL, 0);
    terse = XtCreateManagedWidget("terse", oblongButtonWidgetClass,
				  box, NULL, 0);
    gadget = XtCreateManagedWidget("gadget", oblongButtonGadgetClass,
				   box, NULL, 0);
    quit = XtCreateManagedWidget("quit", oblongButtonGadgetClass,
				 box, NULL, 0);

    /* create notice popup */
    notice = XtCreatePopupShell("notice", noticeShellWidgetClass,
				box, notice_args, XtNumber(notice_args));
    XtGetValues(notice, get_widgets, XtNumber(get_widgets));

    /* add buttons to notice */
    ok = XtCreateManagedWidget("ok", oblongButtonGadgetClass,
			       control, NULL, 0);
    cancel = XtCreateManagedWidget("cancel", oblongButtonWidgetClass,
				   control, NULL, 0);
    XtRealizeWidget(notice);

    /* attach callbacks to buttons */
    XtAddCallback(verbose, XtNselect, PopItUp, VerboseText);
    XtAddCallback(terse, XtNselect, PopItUp, TerseText);
    XtAddCallback(gadget, XtNselect, PopItUp, GadgetText);
    XtAddCallback(quit, XtNselect, Quit, NULL);

    XtRealizeWidget(toplevel);
    XtMainLoop();
}
