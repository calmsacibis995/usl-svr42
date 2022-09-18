/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:pushpin/unit_test1.c	1.7"
#endif

/*
 *************************************************************************
 *
 * Date:	September 1988
 *
 * Description:
 *		This file tests the Pushpin Widget.
 *
 ******************************file*header********************************
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/Pushpin.h>
#include <Xol/ButtonStac.h>

#define NAME(w) ((w)->core.name)

/*
 *************************************************************************
 * Trigger - this routine triggers a pushpin's callbacks
 ****************************procedure*header*****************************
 */
static void
Trigger(w, client_data, call_data)
	Widget   w;			/* Widget doing trigger		*/
	caddr_t  client_data;		/* Client's data		*/
	caddr_t  call_data;		/* Callback sent data		*/
{
	Widget     pushpin = (Widget) client_data;
	Arg trigger[1];

	printf("\nTriggering the pushpin \"%s\"'s callbacks\n",
		NAME(pushpin));

	XtSetArg(trigger[0], XtNtrigger, w);

	XtSetValues(pushpin, trigger, 1);

} /* END OF Trigger() */

/*
 *************************************************************************
 * PinIn - this procedure is called when a pin goes in 
 ****************************procedure*header*****************************
 */
static void
PinIn(w, client_data, call_data)
	Widget   w;			/* Widget doing trigger		*/
	caddr_t  client_data;		/* Client's data		*/
	caddr_t  call_data;		/* Callback sent data		*/
{
	printf("\n Pin In callbacks called for pushpin \"%s\"\n", NAME(w));
} /* END OF PinIn() */

/*
 *************************************************************************
 * Unpinned - this procedure is called when a pin comes out
 ****************************procedure*header*****************************
 */
static void
Unpinned(w, client_data, call_data)
	Widget   w;			/* Widget doing trigger		*/
	caddr_t  client_data;		/* Client's data		*/
	caddr_t  call_data;		/* Callback sent data		*/
{
	printf("\n Pin Out callbacks called for pushpin \"%s\"\n", NAME(w));
} /* END OF Unpinned() */

/*
 *************************************************************************
 * main - this is the main driver for the demo program
 ****************************procedure*header*****************************
 */
int main(argc, argv)
	int argc;
	char **argv;
{
	extern void	exit ();

	Widget toplevel, box, pushpin;
	static XtCallbackRec in_callback[] = {
		{ PinIn,	(caddr_t) NULL },
		{ NULL,		(caddr_t) NULL }
	};
	static XtCallbackRec out_callback[] = {
		{ Unpinned,	(caddr_t) NULL },
		{ NULL,		(caddr_t) NULL }
	};
	static Arg arg[] = {
		{ XtNdefault,		(XtArgVal) False	},
		{ XtNpushpinIn,		(XtArgVal) in_callback	},
		{ XtNpushpinOut,	(XtArgVal) out_callback	}
	};
	static XtCallbackRec trigger_callback[] = {
		{ Trigger,	(caddr_t) NULL },
		{ NULL,		(caddr_t) NULL }
	};
	static Arg bstack[] = {
		{ XtNpushpin,	(XtArgVal) True }
	};
	static Arg trigger_arg[] = {
		{ XtNselect,		(XtArgVal) trigger_callback }
	};
	static XtCallbackRec quit_callback[] = {
		{ (XtCallbackProc) exit,	(caddr_t) NULL },
		{ NULL,				(caddr_t) NULL }
	};
	static Arg quit_arg[] = {
		{ XtNselect,		(XtArgVal) quit_callback }
	};
	static Arg control[] = {
		{ XtNlayoutType,(XtArgVal) OL_FIXEDCOLS },
		{ XtNsameSize,	(XtArgVal) OL_NONE },
		{ XtNmeasure,	(XtArgVal) 3 }
	};

	toplevel = OlInitialize("quitButton", "QuitButton", NULL,
				0, &argc, argv);


	box = XtCreateManagedWidget("box", controlAreaWidgetClass,
		toplevel, control, XtNumber(control));

	pushpin = XtCreateManagedWidget("pushpin #1", pushpinWidgetClass,
			box, arg, XtNumber(arg));

	XtSetArg(arg[0], XtNdefault, True);
	XtCreateManagedWidget("pushpin #2", pushpinWidgetClass, box, arg, 
		XtNumber(arg));

				/* Create a button to test the preview
				 * feature				*/

	XtCreateManagedWidget("preview", buttonStackWidgetClass, box,
			bstack, XtNumber(bstack));

	XtRealizeWidget(toplevel);

				/* Create a button to test the 
				 * external triggering feature		*/

	trigger_callback[0].closure = (caddr_t) pushpin;

	XtCreateManagedWidget("trigger", oblongButtonWidgetClass, box,
			trigger_arg, XtNumber(trigger_arg));

				/* Create a quit button			*/

	XtCreateManagedWidget("quit", oblongButtonWidgetClass, box,
			quit_arg, XtNumber(quit_arg));

	XtMainLoop();
}

