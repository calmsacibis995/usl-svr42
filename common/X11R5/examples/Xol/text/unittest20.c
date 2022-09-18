/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unittest20.c	1.5"
#endif

/*
 * This test tests the TextWidget methods.
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/Text.h>


static void Clear();
static void Copy();
static Widget CreateButton();
static void GetInsert();
static void GetLast();
static void Insert();
static void ReadSub();
static void Redraw();
static void Replace();
static void SetInsert();
static void Update();


static void (*clearFunc)();
static char *(*copyFunc)();
static OlTextPosition (*getInsertFunc)();
static OlTextPosition (*getLastFunc)();
static void (*insertFunc)();
static int (*readSubFunc)();
static void (*redrawFunc)();
static int (*replaceFunc)();
static void (*setInsertFunc)();
static void (*updateFunc)();



static char string[] = "This text has no particular meaning.  Its sole \
purpose in life it to provide something for me to put inside of this \
static text widget which in turn only exists so that I can put something \
inside of the scrolled window widget that I am currently testing.  So you \
can just ignore me.";


void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, toplevel1, box, text;
	Arg arg[20];
	unsigned int n;


	toplevel = OlInitialize("quitButton",
		"QuitButton",
		NULL,
		0,
		&argc,
		argv);

	toplevel1 = XtCreateApplicationShell("TopLevelShell",
					     topLevelShellWidgetClass,
					     (ArgList)NULL,
					     (Cardinal)0);

	n = 0;
	XtSetArg(arg[n], XtNstring, string);			n++;
	text = XtCreateManagedWidget("Text",
		textWidgetClass,
		toplevel,
		arg,
		n);

	n = 0; 
	XtSetArg(arg[n], XtNtextClearBuffer, &clearFunc);		n++;
	XtSetArg(arg[n], XtNtextCopyBuffer, &copyFunc);			n++;
	XtSetArg(arg[n], XtNtextGetInsertPoint, &getInsertFunc);	n++;
	XtSetArg(arg[n], XtNtextGetLastPos, &getLastFunc);		n++;
	XtSetArg(arg[n], XtNtextInsert, &insertFunc);			n++;
	XtSetArg(arg[n], XtNtextReadSubStr, &readSubFunc);		n++;
	XtSetArg(arg[n], XtNtextRedraw, &redrawFunc);			n++;
	XtSetArg(arg[n], XtNtextReplace, &replaceFunc);			n++;
	XtSetArg(arg[n], XtNtextSetInsertPoint, &setInsertFunc);	n++;
	XtSetArg(arg[n], XtNtextUpdate, &updateFunc);			n++;
	XtGetValues(text, arg, n);

	n = 0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS);		n++;
	box = XtCreateManagedWidget("ControlArea",
		controlAreaWidgetClass,
		toplevel1,
		arg,
		n);

	(void)CreateButton(box, "Clear", Clear, (caddr_t)text);
	(void)CreateButton(box, "Copy", Copy, (caddr_t)text);
	(void)CreateButton(box, "Get Insert", GetInsert, (caddr_t)text);
	(void)CreateButton(box, "Get Last", GetLast, (caddr_t)text);
	(void)CreateButton(box, "Insert", Insert, (caddr_t)text);
	(void)CreateButton(box, "Read Sub", ReadSub, (caddr_t)text);
	(void)CreateButton(box, "Redraw", Redraw, (caddr_t)text);
	(void)CreateButton(box, "Replace", Replace, (caddr_t)text);
	(void)CreateButton(box, "Set Insert", SetInsert, (caddr_t)text);
	(void)CreateButton(box, "Update", Update, (caddr_t)text);

	XtRealizeWidget(toplevel);
	XtRealizeWidget(toplevel1);
	XtMainLoop();
}


static void
Clear(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  (*clearFunc)((Widget)client_data);
}


static void
Copy(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  fprintf(stderr, "buf = <%s>\n", (*copyFunc)((Widget)client_data));
  fflush(stderr);
}

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

}


static void
GetInsert(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  fprintf(stderr, "pos = %d\n", (int)(*getInsertFunc)((Widget)client_data));
  fflush(stderr);
}


static void
GetLast(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  fprintf(stderr, "pos = %d\n", (int)(*getLastFunc)((Widget)client_data));
  fflush(stderr);
}


static void
Insert(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  (*insertFunc)((Widget)client_data, "<INSERTED TEXT>");
}


static void
ReadSub(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
  char	target[11];
  int	tused;


  fprintf(stderr, "pos = %d\n",
	  (*readSubFunc)((Widget)client_data, 3, 13, target, 10, &tused));
  fprintf(stderr, "target = <%s>\ntused = %d\n", target, tused);
  fflush(stderr);
}


static void
Redraw(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  (*redrawFunc)((Widget)client_data);
}


static void
Replace(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  fprintf(stderr, "Replacing between 4 and 7\n");
  fflush(stderr);
  (void)(*replaceFunc)((Widget)client_data, 4, 7, "<REPLACED TEXT>");
}


static void
SetInsert(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{

  fprintf(stderr, "Setting Insert Point to 3\n");
  fflush(stderr);
  (*setInsertFunc)((Widget)client_data, (OlTextPosition)3);
}


static void
Update(w, client_data, call_data)
  Widget	w;
  caddr_t	client_data;
  caddr_t	call_data;
{
  static Boolean update_p = FALSE;

  fprintf(stderr, "Setting Update to %s\n", update_p ? "TRUE" : "FALSE");
  fflush(stderr);
  (*updateFunc)((Widget)client_data, update_p);
  update_p = (update_p ? FALSE : TRUE);
}


