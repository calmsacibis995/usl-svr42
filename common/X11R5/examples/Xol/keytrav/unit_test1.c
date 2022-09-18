/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:keytrav/unit_test1.c	1.3"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/BaseWindow.h>
#include <Xol/BulletinBo.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
#include <Xol/Manager.h>
#include <Xol/PopupWindo.h>
#include <Xol/Primitive.h>
#include <Xol/OblongButt.h>
#include <Xol/Text.h>
#include <Xol/TextField.h>


static Widget	CreateButton();
static Widget	CreateCaption();
static Widget	CreateControlArea();
static Widget	CreatePopup();
static Widget	CreateTextField();
static void	DeleteAndAdd();
static void	Popup();

static Widget	field6;
static Widget	field7;
static Widget	field8;
static Widget	right_control;


main(argc, argv)
   int	argc;
   char	*argv[];
{
   Arg		arg[10];
   Widget	base_window;
   Widget	bulletin_board;
   Widget	caption;
   Widget	field;
   Widget	form;
   Dimension	height;
   Cardinal	n;
   Widget	upper_control;
   Position	y;


   base_window = OlInitialize("keytrav", "BaseWindowShell",
			      NULL, 0,
			      &argc, argv);

   n = 0;
   form = XtCreateManagedWidget("Form", formWidgetClass,
				base_window,
				arg, n);

   /*
   ** Create and populate a control area across the top
   */
   upper_control = CreateControlArea("UpperControl", form,
				     FALSE, OL_FIXEDROWS, TRUE);

   (void)CreateButton("DeleteAndAddButton", upper_control, "Delete",
		      DeleteAndAdd);

   (void)CreateButton("PopupButton", upper_control, "Popup", Popup);

   (void)CreateTextField("TextField1", upper_control, "Field 1:");

   (void)CreateTextField("TextField2", upper_control, "Field 2:");

   /*
   ** Create and populate a bulletin board on the left side
   */
   n = 0;
   XtSetArg(arg[n], XtNborderWidth, (Dimension)1);			n++;
   XtSetArg(arg[n], XtNtraversalManager, TRUE);				n++;
   XtSetArg(arg[n], XtNyAddHeight, TRUE);				n++;
   XtSetArg(arg[n], XtNyOffset, 4);					n++;
   XtSetArg(arg[n], XtNyRefWidget, upper_control);			n++;
   bulletin_board = XtCreateManagedWidget("BulletinBoard",
					  bulletinBoardWidgetClass,
					  form,
					  arg, n);

   field = CreateTextField("TextField3", bulletin_board, "Field 3:");
   XtSetArg(arg[0], XtNheight, &height);
   XtGetValues(field, arg, (Cardinal)1);

   caption = CreateCaption(bulletin_board, "Field 4:");
   XtSetArg(arg[0], XtNy, y = height);
   XtSetValues(caption, arg, (Cardinal)1);

   n = 0;
   XtSetArg(arg[n], XtNverticalSB, TRUE); n++;
   XtSetArg(arg[n], XtNtraversalOn, TRUE); n++;
   field = XtCreateManagedWidget("TextField4", textWidgetClass,
			       caption,
			       arg, n);

   XtSetArg(arg[0], XtNheight, &height);
   XtGetValues(field, arg, (Cardinal)1);

   field = CreateTextField("TextField5", bulletin_board, "Field 5");
   XtSetArg(arg[0], XtNy, y + height);
   XtSetValues(field, arg, (Cardinal)1);


   /*
   ** Create and populate a control area on the right side
   */
   right_control = CreateControlArea("RightControl", form,
				    TRUE, OL_FIXEDCOLS, TRUE);
   n = 0;
   XtSetArg(arg[n], XtNborderWidth, (Dimension)1);			n++;
   XtSetArg(arg[n], XtNxAddWidth, TRUE);				n++;
   XtSetArg(arg[n], XtNxOffset, 4);					n++;
   XtSetArg(arg[n], XtNxRefWidget, bulletin_board);			n++;
   XtSetArg(arg[n], XtNyAddHeight, TRUE);				n++;
   XtSetArg(arg[n], XtNyOffset, 4);					n++;
   XtSetArg(arg[n], XtNyRefWidget, upper_control);			n++;
   XtSetValues(right_control, arg, n);

   field6 = CreateTextField("TextField6", right_control, "Field 6:");
   field7 = CreateTextField("TextField7", right_control, "Field 7:");
   field8 = CreateTextField("TextField8", right_control, "Field 8:");

   XtRealizeWidget(base_window);

   XtMainLoop();

}	/* main() */


/*
** Create an oblong button with the necessary attributes.
*/
static Widget
CreateButton(name, parent, label, callback)
   String		name;
   Widget		parent;
   String		label;
   XtCallbackProc	callback;
{
   Arg		arg[1];
   Widget	button;


   XtSetArg(arg[0], XtNlabel, label);
   button = XtCreateManagedWidget(name, oblongButtonWidgetClass,
				  parent,
				  arg, (Cardinal)1);

   if (callback != (XtCallbackProc)NULL)
      XtAddCallback(button, XtNselect, callback, (caddr_t)NULL);

   return button;

}	/* CreateButton() */



/*
** Create a caption aligned at the top, with no border, and to the left of
** its child.  `caption' is the label.
*/
static Widget
CreateCaption(parent, caption)
   Widget	parent;
   String	caption;
{
   static Arg	arg[4] =
      {
	 {XtNalignment, (XtArgVal)OL_TOP},
	 {XtNborderWidth, (XtArgVal)(Dimension)0},
	 {XtNposition, (XtArgVal)OL_LEFT}
      };


   XtSetArg(arg[XtNumber(arg)-1], XtNlabel, caption);
   return XtCreateManagedWidget("Caption", captionWidgetClass,
				parent,
				arg, XtNumber(arg));

}	/* CreateCaption() */


static Widget
CreateControlArea(name, parent, align_captions, layout_type, traversal_mgr)
   String	name;
   Widget	parent;
   Boolean	align_captions;
   int		layout_type;
   Boolean	traversal_mgr;
{
   Arg	arg[3];


   XtSetArg(arg[0], XtNalignCaptions, align_captions);
   XtSetArg(arg[1], XtNlayoutType, layout_type);
   XtSetArg(arg[2], XtNtraversalManager, traversal_mgr);
   return XtCreateManagedWidget(name, controlAreaWidgetClass,
				parent,
				arg, (Cardinal)3);

}	/* CreateControlArea() */


static Widget
CreatePopup(name, parent, title, upper_control, lower_control)
   String	name;
   Widget	parent;
   String	title;
   Widget	*upper_control;
   Widget	*lower_control;
{
   Arg		arg[3];
   Widget	popup;
    

   XtSetArg(arg[0], XtNtitle, (XtArgVal)title);
   popup = XtCreatePopupShell(name, popupWindowShellWidgetClass,
			      parent,
			      arg, (Cardinal)1);

   XtSetArg(arg[0], XtNlowerControlArea, lower_control);
   XtSetArg(arg[1], XtNupperControlArea, upper_control);
   XtGetValues(popup, arg, (Cardinal)2);

   XtSetArg(arg[0], XtNborderWidth, (Dimension)1);
   XtSetArg(arg[1], XtNmeasure, 3);
   XtSetArg(arg[2], XtNtraversalManager, TRUE);
   XtSetValues(*upper_control, arg, (Cardinal)3);

   return popup;

}	/* CreatePopup() */


static Widget
CreateTextField(name, parent, label)
   String	name;
   Widget	parent;
   String	label;
{
   Widget	caption;
   static Arg arg[1] = { XtNtraversalOn, TRUE };

   caption = CreateCaption(parent, label);

   (void)XtCreateManagedWidget("TextField", textFieldWidgetClass,
			       caption,
			       arg, XtNumber(arg));

   return caption;

}	/* CreateTextField() */


static void
DeleteAndAdd(w, client_data, call_data)
   Widget	w;
   caddr_t	client_data;
   caddr_t	call_data;
{
   Arg		arg[1];
   static int	i = 0;


   switch (i++)
      {
      case 0:
	 XtDestroyWidget(field8);
	 break;
      case 1:
	 XtDestroyWidget(field7);
	 break;
      case 2:
	 XtDestroyWidget(field6);
	 XtSetArg(arg[0], XtNlabel, "Add");
	 XtSetValues(w, arg, (Cardinal)1);
	 break;
      default:
	 (void)CreateTextField("NewTextField", right_control, "New Field:");
	 break;
      }

}	/* DeleteAndAdd() */


static void
Popup(w, client_data, call_data)
   Widget	w;
   caddr_t	client_data;
   caddr_t	call_data;
{
   Widget		lower_control;
   static Widget	popup = (Widget)0;
   Widget		upper_control;


   if (popup == (Widget)0)
      {
	 register int	i;


	 popup = CreatePopup("PopupWindow", w, "A Popup",
			     &upper_control, &lower_control);
	 for (i = 1;  i <= 30;  ++i)
	    (void)CreateTextField("PopupField", upper_control, "Popup Field:");
      }

   XtPopup(popup, XtGrabNone);

}	/* Popup() */
