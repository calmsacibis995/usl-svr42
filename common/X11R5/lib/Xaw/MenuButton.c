/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:MenuButton.c	1.2"
/* $XConsortium: MenuButton.c,v 1.18 91/06/22 18:03:46 rws Exp $ */

/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 */


/***********************************************************************
 *
 * MenuButton Widget
 *
 ***********************************************************************/

/*
 * MenuButton.c - Source code for MenuButton widget.
 *
 * This is the source code for the Athena MenuButton widget.
 * It is intended to provide an easy method of activating pulldown menus.
 *
 * Date:    May 2, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/MenuButtoP.h>

static void ClassInitialize();
static void PopupMenu();

#define superclass ((CommandWidgetClass)&commandClassRec)

static char defaultTranslations[] = 
    "<EnterWindow>:     highlight()             \n\
     <LeaveWindow>:     reset()                 \n\
     <BtnDown>:         reset() PopupMenu()     ";

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

#define offset(field) XtOffsetOf(MenuButtonRec, field)
static XtResource resources[] = {
  {
    XtNmenuName, XtCMenuName, XtRString, sizeof(String), 
    offset(menu_button.menu_name), XtRString, (XtPointer)"menu"},
};
#undef offset

static XtActionsRec actionsList[] =
{
  {"PopupMenu",	PopupMenu}
};

MenuButtonClassRec menuButtonClassRec = {
  {
    (WidgetClass) superclass,		/* superclass		  */	
    "MenuButton",			/* class_name		  */
    sizeof(MenuButtonRec),       	/* size			  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    NULL,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    XtInheritRealize,			/* realize		  */
    actionsList,			/* actions		  */
    XtNumber(actionsList),		/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    NULL,				/* destroy		  */
    XtInheritResize,			/* resize		  */
    XtInheritExpose,			/* expose		  */
    NULL,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,               	/* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
    XtInheritDisplayAccelerator,	/* display_accelerator	  */
    NULL				/* extension		  */
  },  /* CoreClass fields initialization */
  {
    XtInheritChangeSensitive		/* change_sensitive	  */ 
  },  /* SimpleClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* LabelClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* CommandClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* MenuButtonClass fields initialization */
};

  /* for public consumption */
WidgetClass menuButtonWidgetClass = (WidgetClass) &menuButtonClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static void ClassInitialize()
{
    XawInitializeWidgetSet();
    XtRegisterGrabAction(PopupMenu, True, ButtonPressMask | ButtonReleaseMask,
			 GrabModeAsync, GrabModeAsync);
}

/* ARGSUSED */
static void
PopupMenu(w, event, params, num_params)
Widget w;
XEvent * event;
String * params;
Cardinal * num_params;
{
  MenuButtonWidget mbw = (MenuButtonWidget) w;
  Widget menu, temp;
  Arg arglist[2];
  Cardinal num_args;
  int menu_x, menu_y, menu_width, menu_height, button_height;
  Position button_x, button_y;

  temp = w;
  while(temp != NULL) {
    menu = XtNameToWidget(temp, mbw->menu_button.menu_name);
    if (menu == NULL) 
      temp = XtParent(temp);
    else
      break;
  }

  if (menu == NULL) {
    char error_buf[BUFSIZ];
    sprintf(error_buf, "MenuButton: %s %s.",
	    "Could not find menu widget named", mbw->menu_button.menu_name);
    XtAppWarning(XtWidgetToApplicationContext(w), error_buf);
    return;
  }
  if (!XtIsRealized(menu))
    XtRealizeWidget(menu);
  
  menu_width = menu->core.width + 2 * menu->core.border_width;
  button_height = w->core.height + 2 * w->core.border_width;
  menu_height = menu->core.height + 2 * menu->core.border_width;

  XtTranslateCoords(w, 0, 0, &button_x, &button_y);
  menu_x = button_x;
  menu_y = button_y + button_height;

  if (menu_x >= 0) {
    int scr_width = WidthOfScreen(XtScreen(menu));
    if (menu_x + menu_width > scr_width)
      menu_x = scr_width - menu_width;
  }
  if (menu_x < 0) 
    menu_x = 0;

  if (menu_y >= 0) {
    int scr_height = HeightOfScreen(XtScreen(menu));
    if (menu_y + menu_height > scr_height)
      menu_y = scr_height - menu_height;
  }
  if (menu_y < 0)
    menu_y = 0;

  num_args = 0;
  XtSetArg(arglist[num_args], XtNx, menu_x); num_args++;
  XtSetArg(arglist[num_args], XtNy, menu_y); num_args++;
  XtSetValues(menu, arglist, num_args);

  XtPopupSpringLoaded(menu);
}

