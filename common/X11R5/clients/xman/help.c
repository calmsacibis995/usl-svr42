/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xman:help.c	1.2"

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

/*
 * xman - X window system manual page display program.
 *
 * $XConsortium: help.c,v 1.9 91/07/01 14:05:09 dave Exp $
 *
 * Copyright 1987, 1988 Massachusetts Institute of Technology
 *
 *
 * Author:    Chris D. Peterson, MIT Project Athena
 * Created:   January 19, 1988
 */

#include "globals.h"

static Atom wm_delete_window;

ManpageGlobals * InitPsuedoGlobals();

/*	Function Name: MakeHelpWidget.
 *	Description: This function creates the help widget so that it will be
 *                   ready to be displayed.
 *	Arguments: none.
 *	Returns: none.
 */

Boolean
MakeHelpWidget()
{

  ManpageGlobals * man_globals;	/* The psuedo global structure. */
  
  if (help_widget != NULL)	/* If we already have a help widget. 
				   then do not create one. */
    return(TRUE);

  man_globals = InitPsuedoGlobals();

  CreateManpageWidget(man_globals, HELPNAME, FALSE);
  help_widget = man_globals->This_Manpage;

  if (OpenHelpfile(man_globals) == FALSE) {
    XtDestroyWidget(help_widget);
    help_widget = NULL;
    return(FALSE);
  }

  ChangeLabel(man_globals->label, "Xman Help");

  XtManageChild( man_globals->manpagewidgets.manpage );
  XtRealizeWidget(  help_widget );
  SaveGlobals( man_globals->This_Manpage, man_globals );
  AddCursor( help_widget, resources.cursors.manpage);

/*
 * Set up ICCCM delete window.
 */
  wm_delete_window = XInternAtom(XtDisplay(help_widget), "WM_DELETE_WINDOW",
				 False);
  XtOverrideTranslations
      (man_globals->This_Manpage, 
       XtParseTranslationTable ("<Message>WM_PROTOCOLS: RemoveThisManpage()"));
  (void) XSetWMProtocols (XtDisplay(man_globals->This_Manpage),
			  XtWindow(man_globals->This_Manpage),
			  &wm_delete_window, 1);

  return(TRUE);
}

/*	Function Name: OpenHelpfile
 *	Description: opens the helpfile.
 *	Arguments: man_globals - the psuedo globals structure.
 *	Returns: False if no helpfile was found.
 */

Boolean
OpenHelpfile(man_globals)
ManpageGlobals * man_globals;
{
  FILE * help_file_ptr;

  if( (help_file_ptr = fopen(resources.help_file, "r")) == NULL ) {
    PopupWarning(man_globals,
		 "Could not open help file, NO HELP WILL BE AVALIABLE.");
    return(FALSE);
  }
    
  OpenFile(man_globals, help_file_ptr);
  return(TRUE);
}
