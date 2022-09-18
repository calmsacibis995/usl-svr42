/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:SimpleMenP.h	1.2"
/*
 * $XConsortium: SimpleMenP.h,v 1.12 89/12/11 15:01:39 kit Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 */

/*
 * SimpleMenuP.h - Private Header file for SimpleMenu widget.
 *
 * Date:    April 3, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium
 *          kit@expo.lcs.mit.edu
 */

#ifndef _SimpleMenuP_h
#define _SimpleMenuP_h

#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeP.h>
#include <X11/ShellP.h>

#define ForAllChildren(smw, childP) \
  for ( (childP) = (SmeObject *) (smw)->composite.children ; \
        (childP) < (SmeObject *) ( (smw)->composite.children + \
				 (smw)->composite.num_children ) ; \
        (childP)++ )

typedef struct {
    XtPointer extension;		/* For future needs. */
} SimpleMenuClassPart;

typedef struct _SimpleMenuClassRec {
  CoreClassPart	          core_class;
  CompositeClassPart      composite_class;
  ShellClassPart          shell_class;
  OverrideShellClassPart  override_shell_class;
  SimpleMenuClassPart	  simpleMenu_class;
} SimpleMenuClassRec;

extern SimpleMenuClassRec simpleMenuClassRec;

typedef struct _SimpleMenuPart {

  /* resources */

  String       label_string;	/* The string for the label or NULL. */
  SmeObject   label;		/* If label_string is non-NULL then this is
				   the label widget. */
  WidgetClass  label_class;	/* Widget Class of the menu label object. */

  Dimension    top_margin;	/* Top and bottom margins. */
  Dimension    bottom_margin;
  Dimension    row_height;	/* height of each row (menu entry) */

  Cursor       cursor;		/* The menu's cursor. */
  SmeObject popup_entry;	/* The entry to position the cursor on for
				   when using XawPositionSimpleMenu. */
  Boolean      menu_on_screen;	/* Force the menus to be fully on the screen.*/
  int          backing_store;	/* What type of backing store to use. */

  /* private state */

  Boolean recursive_set_values;	/* contain a possible infinite loop. */

  Boolean menu_width;		/* If true then force width to remain 
				   core.width */
  Boolean menu_height;		/* Just like menu_width, but for height. */

  SmeObject entry_set;		/* The entry that is currently set or
				   highlighted. */
} SimpleMenuPart;

typedef struct _SimpleMenuRec {
  CorePart		core;
  CompositePart 	composite;
  ShellPart 	        shell;
  OverrideShellPart     override;
  SimpleMenuPart	simple_menu;
} SimpleMenuRec;

#endif /* _SimpleMenuP_h */
