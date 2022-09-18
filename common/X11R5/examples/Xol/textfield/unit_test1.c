/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:textfield/unit_test1.c	1.9"
#endif

#include	<X11/Intrinsic.h>
#include	<X11/StringDefs.h>
#include	<Xol/OpenLook.h>
#include	<Xol/Caption.h>
#include 	<Xol/TextField.h>
#include	<Xol/ControlAre.h>

Widget		toplevel, control;
TextFieldWidget	textw1, textw2, textw3;
CaptionWidget	caption1, caption2, caption3;
static void	VerifyCallback1();

static	/*SC, added*/
void 
VerifyCallback1(widget, clientData, callData)
TextFieldWidget widget;
caddr_t clientData, callData;
{
	int	i;
	Arg	args[10];
	char	*string;

	OlTextFieldVerify	*foo = (OlTextFieldVerify *) callData;

	printf ("In verify callback one with %s, %s\n",foo->string,
		(foo->ok == True ? "ok" : "not ok"));

	i = 0;
	XtSetArg(args[i],XtNstring, &string);	i++;
	XtGetValues(widget, args, i);

	i = 0;
	XtSetArg(args[i],XtNstring, string);	i++;
	XtSetValues(textw2, args, i);
}

/*
**	sample driver for TextField widget
*/

main(argc, argv)
unsigned int argc;
char **argv;
{
	static Arg	args[15];
	int	i;

	toplevel = OlInitialize("TopLevel", "TopLevel", 0, 0, &argc, argv);

	i = 0;
	XtSetArg(args[i], XtNlayoutType, OL_FIXEDCOLS);	i++;
	XtSetArg(args[i], XtNalignCaptions, True);	i++;
	XtSetArg(args[i], XtNsameSize, OL_NONE);	i++;

	control = XtCreateManagedWidget(	"control", 
						controlAreaWidgetClass, 
						toplevel, 
						args, 
						i);
	i = 0;
	XtSetArg(args[i], XtNlabel, "15 chars:");		i++;
	XtSetArg(args[i], XtNalignment, OL_TOP);		i++;
	caption1 = (CaptionWidget) XtCreateManagedWidget(	"caption1",
						captionWidgetClass,
						control,
						args,	
						i);

	i = 0;
	XtSetArg(args[i], XtNlabel, "200 Width:");		i++;
	XtSetArg(args[i], XtNalignment, OL_CENTER);		i++;
	caption2 = (CaptionWidget) XtCreateManagedWidget(	"caption2",
						captionWidgetClass,
						control,
						args,	
						i);

	i = 0;
	XtSetArg(args[i], XtNlabel, "50/200:");		i++;
	XtSetArg(args[i], XtNalignment, OL_BOTTOM);		i++;
	caption3 = (CaptionWidget) XtCreateManagedWidget(	"caption2",
						captionWidgetClass,
						control,
						args,	
						i);

	i = 0;
	XtSetArg(args[i], XtNmaximumSize, 15);		i++;
	XtSetArg(args[i], XtNstring, "First value");	i++;
	XtSetArg(args[i],
		XtNfont,
		XLoadQueryFont(XtDisplay(toplevel), "ger-s35"));	i++;
	textw1 = (TextFieldWidget) XtCreateManagedWidget(	"text1",
					textFieldWidgetClass,
					caption1,
					args,	
					i);

	i = 0;
	XtSetArg(args[i], XtNwidth, 200);		i++;
	XtSetArg(args[i], XtNstring, "Second value");	i++;
	textw2 = (TextFieldWidget) XtCreateManagedWidget(	"text2",
					textFieldWidgetClass,
					caption2,
					args,	
					i);

	i = 0;
	XtSetArg(args[i], XtNmaximumSize, 50);		i++;
	XtSetArg(args[i], XtNstring, "Third value");	i++;
	XtSetArg(args[i], XtNwidth, 200);		i++;
	textw3 = (TextFieldWidget) XtCreateManagedWidget(	"text3",
					textFieldWidgetClass,
					caption3,
					args,	
					i);

	XtRealizeWidget(toplevel);

	XtAddCallback(textw1, XtNverification, VerifyCallback1, NULL);	

	XtMainLoop();
}
