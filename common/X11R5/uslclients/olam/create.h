/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:create.h	1.4"
#endif

/*
** create.h - This file has declarations for the functions defined in
** "create.c".
*/


#ifndef _OLAM_CREATE_H
#define _OLAM_CREATE_H


/*
** Returns an oblong button.  `parent' is the button's parent; `label' is
** the button's label; `callback' is called with client data `client_data'
** when the button is pressed .
*/
Widget	CreateButton();
/*
  Widget                parent;
  String                label;
  XtCallbackProc        callback;
  caddr_t               client_data;
/*


/*
** Returns a buttons stack.  `parent' is the button's parent; `label' is
** the button's label;  the associated menu pane is returned in `menu_pane'.
*/
Widget	CreateButtonStack();
/*
  Widget        parent;
  String        label;
  Widget        *menu_pane;
*/


/*
** Returns a caption aligned at the top, with no border, and to the left of
** its child.  `caption' is the label.
*/
Widget	CreateCaption();
/*
  Widget        parent;
  String        caption;
*/


/*
** Create a control area with captions aligned, no border, and XtNlayoutType
** set to `layout_type'.
*/
Widget	CreateControlArea();
/*
  Widget	parent;
  int		layout_type;
*/


/*
** Create a popup window with "Apply" and "Reset" buttons.  The upper control
** area of the popup (the lower control area contains the standard popup
** window buttons) is given a 1 pixel border, and is split into two
** sub-control-areas that are returned in `upper_control' and
** `lower_control'.  A static text widget is placed inside the popup's
** footer panel and returned in `footer'.
*/
Widget	CreatePopup();
#if 0
  String	title;			/* Popup's title */
  void		(*apply)();		/* Callback for "Apply" button */
  void		(*reset)();		/* Callback for "Reset" button */
  void		(*popdown)();		/* Callback for verifying popdowns */
					/* because of a button press on */
					/* "Apply" or "Reset" */
  caddr_t	client_data;		/* Client data for the three */
					/* callbacks above */
  Widget	*upper_control;		/* Upper part of popup's upper */
					/* control area */
  Widget	*lower_control;		/* Lower part of popup's upper */
					/* control area */
  Widget	*footer;		/* Static text widget in the footer */
#endif


/*
** Returns a top level shell.
*/
Widget	CreateShell();


/*
** Returns a text field inside a caption with `label' to the left.
** `validate' is text field's XtNverification callback with `client_data' as
** the client data.  Note that the text field and not the caption is
** returned.
*/
Widget	CreateTextField();
/*
  Widget        parent;
  String        label;
  void          (*validate)();
  caddr_t       client_data;
*/


/*
** Returns a notice with `string' as the message and having "Yes" and "No"
** buttons.  The "Yes" button is the default.
*/
Widget	CreateYesNoNotice();
/*
  String                string;
  XtCallbackProc        yes_callback;
  XtCallbackProc        no_callback;
  caddr_t               client_data;
*/


#endif	/* _OLAM_CREATE_H */
