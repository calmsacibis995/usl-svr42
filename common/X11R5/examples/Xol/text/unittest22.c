/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unittest22.c	1.4"
#endif

/*
** This test tests MR# bl89-03755 and MR# ug89-06402
*/

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/Text.h>


static Widget	CreateButton();
static void	Destroy();
static void	New();
static void	NewText();
static Widget	text;
static Widget	toplevel;


main (argc, argv)
  int argc;
  char **argv;
{
  Arg		arg[20];
  Widget	control;
  unsigned	n;
  Widget	toplevel1;


  toplevel = OlInitialize("unittest22", "Unittest22",
			  NULL, 0,
			  &argc, argv);

  NewText();

  toplevel1 = XtCreateApplicationShell("TopLevelShell",
				       topLevelShellWidgetClass,
				       (ArgList)NULL,
				       (Cardinal)0);

  n = 0;
  XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS);			n++;
  control = XtCreateManagedWidget("ControlArea",
				  controlAreaWidgetClass,
				  toplevel1,
				  arg,
				  n);

  (void)CreateButton(control, "Destroy", Destroy, (caddr_t)NULL);
  (void)CreateButton(control, "New", New, (caddr_t)NULL);

  XtRealizeWidget(toplevel);
  XtRealizeWidget(toplevel1);
  XtMainLoop();

}	/* main() */



static Widget
CreateButton(parent, label, callback, client_data)
  Widget                parent;
  String                label;
  XtCallbackProc        callback;
  caddr_t               client_data;
{
  Arg		arg[1];
  Widget	button;


  XtSetArg(arg[0], XtNlabel, label);
  button = XtCreateManagedWidget("OblongButton",
				 oblongButtonWidgetClass,
				 parent,
				 arg,
				 (Cardinal)1);

  if (callback != (XtCallbackProc)NULL)
    XtAddCallback(button, XtNselect, callback, client_data);

  return button;

}	/* CreateButton() */


static void
Destroy(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  if (text != (Widget)NULL)
    {
      XtDestroyWidget(text);
      text = (Widget)NULL;
    }

}	/* Destroy() */


static void
New(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  if (text == (Widget)NULL)
    NewText();

}	/* New() */


static void
NewText()
{
  static char	string[] = "This text has no particular meaning.  Its sole \
purpose in life is to provide something for me to put inside of this \
static text widget which in turn only exists so that I can put something \
inside of the scrolled window widget that I am currently testing.  So you \
can just ignore me.";

  static Arg	arg[] = {{XtNstring, (XtArgVal)string}};


  if (text == (Widget)NULL)
      text = XtCreateManagedWidget("Text",
				 textWidgetClass,
				 toplevel,
				 arg,
				 XtNumber(arg));
}	/* NewText() */

