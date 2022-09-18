/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:create.c	1.14"
#endif

/*
** create.c - This file contains functions to create various widgets in the
** form that is most the common and convenient for the other Administration
** Manager routines.
*/


#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Notice.h>
#include <Xol/PopupWindo.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/FButtons.h>

#include "errors.h"
#include "common.h"

/*
** Create a caption aligned at the top, with no border, and to the left of
** its child.  `caption' is the label.
*/
Widget
CreateCaption(parent, caption)
  Widget	parent;
  String	caption;
{
  static Arg	arg[3] =
    {
      {XtNalignment, (XtArgVal)OL_TOP},
      {XtNborderWidth, (XtArgVal)0},
      {XtNposition, (XtArgVal)OL_LEFT}
    };


  return XtVaCreateManagedWidget( caption,
				captionWidgetClass,
				parent,
				XtNalignment, (XtArgVal)OL_TOP,
				XtNborderWidth, (XtArgVal)0,
				XtNposition, (XtArgVal)OL_LEFT,
				XtNlabel, (XtArgVal)caption,
				(char *)NULL);
}	/* CreateCaption() */


/*
** Create a control area with captions aligned, no border, and XtNlayoutType
** set to `layout_type'.
*/
Widget
CreateControlArea(parent, layout_type)
  Widget	parent;
  int		layout_type;
{


  return XtVaCreateManagedWidget("ControlArea",
				controlAreaWidgetClass,
				parent,
				XtNalignCaptions, (XtArgVal)TRUE,
				XtNborderWidth, (XtArgVal)0,
				XtNlayoutType, (XtArgVal)layout_type,
				(char *)0);

}	/* CreateControlArea() */


/*
** Create a popup window with "Apply" and "Reset" buttons.  The upper control
** area of the popup (the lower control area contains the standard popup
** window buttons) is given a 1 pixel border, is made a traversal context,
** and is split into two sub-control-areas that are returned in
** `upper_control' and `lower_control'.  A static text widget is placed inside
** the popup's footer panel and returned in `footer'.
*/
Widget
CreatePopup(title, apply, reset, popdown, client_data, upper_control,
	    lower_control, footer)
  String	title;			/* Popup's title */
  void		(*apply)();		/* Callback for "Apply" button */
  void		(*reset)();		/* Callback for "Reset" button */
  void		(*popdown)();		/* Callback for verifying popdowns */
					/* because of a button press on */
					/* "Apply" or "Reset" */
  XtPointer	client_data;		/* Client data for the three */
					/* callbacks above */
  Widget	*upper_control;		/* Upper part of popup's upper */
					/* control area */
  Widget	*lower_control;		/* Lower part of popup's upper */
					/* control area */
  Widget	*footer;		/* Static text widget in the footer */
{
  XtCallbackRec		apply_callback[2];
					/* Used to setup callback for */
					/* "Apply" */
  Widget		footer_panel;	/* Footer panel created by popup */
					/* window */
  Widget		main_control;	/* Upper control area of popup */
					/* window */
  Widget		parent;		/* Parent shell for popup window */
  register Widget	popup;		/* Popup window to return */
  XtCallbackRec		reset_callback[2];
					/* Used to setup callback for */
					/* "Reset" */

    
  /* parent = CreateShell(); mlp -use topshell*/	/* Create parent for popup @ */

  /*
  ** Create callback for "Apply" button
  */
  apply_callback[0].callback = apply;
  apply_callback[0].closure = client_data;
  apply_callback[1].callback = (XtCallbackProc)NULL;
  apply_callback[1].closure = (XtPointer)NULL;

  /*
  ** Create callback for "Reset" button
  */
  reset_callback[0].callback = reset;
  reset_callback[0].closure = client_data;
  reset_callback[1].callback = (XtCallbackProc)NULL;
  reset_callback[1].closure = (XtPointer)NULL;

  /*
  ** Create the popup window
  */

  popup = XtVaCreatePopupShell( title, 
			popupWindowShellWidgetClass,
			/*parent,*/ topshell,
			XtNapply, (XtArgVal)apply_callback,
			XtNreset, (XtArgVal)reset_callback,
			XtNtitle, (XtArgVal)title,
			(char *)0);

  /*
  ** Get the upper control area and footer panel
  */
  XtVaGetValues(popup,	XtNupperControlArea, (XtArgVal)&main_control,
  			XtNfooterPanel, (XtArgVal)&footer_panel,
		(char *)0);

  /*
  ** Add callbacks for popdown (ie. window dismissed from the window menu)
  ** and verification of popdown on an "Apply" or "Reset" button press
  */
  XtAddCallback(popup, XtNpopdownCallback, popdown, client_data);
  XtAddCallback(popup, XtNverify, VerifyPopdown, client_data);

  /*
  ** Prepare the upper control area for division into two sub-areas
  */
	XtVaSetValues(main_control,
		XtNborderWidth, (XtArgVal)1,
  		XtNcenter, (XtArgVal)TRUE,
		XtNhPad, (XtArgVal)0,
		XtNhSpace, (XtArgVal)0,
		XtNsameSize, (XtArgVal)OL_NONE,
		XtNtraversalManager, (XtArgVal)TRUE,
		XtNvPad, (XtArgVal)0,
		XtNvSpace, (XtArgVal)0,
		(char *) 0);

  /*
  ** Create the two sub-areas of `main_control'
  */
  *upper_control = CreateControlArea(main_control, OL_FIXEDCOLS);
  *lower_control = CreateControlArea(main_control, OL_FIXEDROWS);

  /*
  ** Stick an appropriately set static text widget in the footer panel
  */
  *footer = XtVaCreateManagedWidget("StaticText",
				  staticTextWidgetClass,
				  footer_panel,
				XtNalignment, (XtArgVal)OL_LEFT,
				XtNborderWidth, (XtArgVal)0,
				XtNgravity, (XtArgVal)WestGravity,
				XtNrecomputeSize, (XtArgVal)FALSE,
				XtNstring, (XtArgVal)" ",
				(char *)0);

  return popup;

}	/* CreatePopup() */


/*
** Create a top level shell.
*/
/*
Widget
CreateShell()
{

  return XtCreateApplicationShell("TopLevelShell",
				  topLevelShellWidgetClass,
				  (ArgList)NULL,
				  (Cardinal)0);

}*/	/* CreateShell() */


/*
** Create a text field labeled with a caption.
*/
Widget
CreateTextField(parent, label, validate, client_data)
  Widget	parent;
  String	label;			/* Label for caption */
  void		(*validate)();		/* Callback for validation */
  XtPointer	client_data;		/* Client data for validation */
					/* callback */
{
  Widget	caption;
  Widget	text_field;


  caption = CreateCaption(parent, label);

  text_field = XtCreateManagedWidget("TextField",
				     textFieldWidgetClass,
				     caption,
				     (ArgList)NULL,
				     (Cardinal)0);

  XtAddCallback(text_field, XtNverification, validate, client_data);

  return text_field;

}	/* CreateTextField() */

typedef struct {
	String label;
	Boolean dflt;
	void (*cb)();
	XtPointer data;
	OlMnemonic mnemonic;
} NoticeItem;

char *noticefields[] = {
	XtNlabel,
	XtNdefault,
	XtNselectProc,
	XtNclientData,
	XtNmnemonic
};
NoticeItem nitems[] = {
	{NULL, True, (void (*)())NULL, NULL, 'Y'},
	{NULL, False, (void (*)())NULL, NULL, 'N'}
};

/*
** Create a notice with `string' as the message and having "Yes" and "No"
** buttons.  The "Yes" button is the default.
*/
Widget
CreateYesNoNotice(string, yes_callback, no_callback, client_data, parent)
  String		string;		/* Message */
  XtCallbackProc	yes_callback;	/* Callback for "Yes" button */
  XtCallbackProc	no_callback;	/* Callback for "No" button */
  XtPointer		client_data;	/* Client data for above callbacks */
  Widget		parent;		/* notice's parent */
{
  Arg		arg[2];
  Widget	button;
  Widget	control_area;		/* Control area section of notice */
  Widget	notice;
  /*Widget	parent;*/		/* Parent shell for notice widget */
  Widget	text_area;


  /*parent = CreateShell();*/		/* Create notice's parent @ */

  /*
  ** Create the notice
  */
  notice = XtCreatePopupShell("NoticeShell",
			      noticeShellWidgetClass,
			      parent,
			      (ArgList)NULL,
			      (Cardinal)0);

  /*
  ** Get the control area and message area
  */
  XtVaGetValues(notice, XtNcontrolArea, (XtArgVal)&control_area,
  			XtNtextArea, (XtArgVal)&text_area,
			(char *)0);

nitems[0].label = OlGetMessage(XtDisplay(notice), NULL, 0, OleNbutton,
                                          OleTyes,
                                          OleCOlClientOlamMsgs,
                                          OleMbutton_yes,
                                          (XrmDatabase)NULL);
nitems[0].mnemonic = *(OlGetMessage(XtDisplay(notice), NULL, 0, OleNmnemonic,
                                          OleTyes,
                                          OleCOlClientOlamMsgs,
                                          OleMmnemonic_yes,
                                          (XrmDatabase)NULL));
nitems[0].cb = yes_callback;
nitems[0].data =  nitems[1].data = client_data;
nitems[1].label = OlGetMessage(XtDisplay(notice), NULL, 0, OleNbutton,
                                          OleTno,
                                          OleCOlClientOlamMsgs,
                                          OleMbutton_no,
                                          (XrmDatabase)NULL);
nitems[1].mnemonic = *(OlGetMessage(XtDisplay(notice), NULL, 0, OleNmnemonic,
                                          OleTno,
                                          OleCOlClientOlamMsgs,
                                          OleMmnemonic_no,
                                          (XrmDatabase)NULL));
nitems[1].cb =  no_callback;

button = XtVaCreateManagedWidget("notice_controls", flatButtonsWidgetClass,
				control_area,
				XtNrecomputeSize, True,
				XtNlayoutType, OL_FIXEDROWS,
				XtNmeasure, 1,
				XtNitemFields, noticefields,
				XtNnumItemFields, XtNumber(noticefields),
				XtNitems,  nitems,
				XtNnumItems, XtNumber(nitems),
				(char *) NULL);


  /*
  ** Set the message
  */
  XtSetArg(arg[0], XtNstring, (XtArgVal)string);
  XtSetValues(text_area, arg, (Cardinal)1);

  return notice;

}	/* CreateYesNoNotice() */
