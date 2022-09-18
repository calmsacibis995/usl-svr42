/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:help.c	1.5"
#endif

/*
 * help.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>

#include <buffutil.h>
#include <textbuff.h>

#include <OpenLook.h>
#include <TextEdit.h>
#include <FButtons.h>

#include <Gizmos.h>
#include <BaseWGizmo.h>

#include <editor.h>
#include <help.h>


/*
 * HelpCB
 *
 * This callback procedure handles the selection of buttons in the
 * Help menu located in the base window's menu bar.
 *
 */

extern void 
HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);
   Widget           shell      = GetBaseWindowShell(ew->baseWindow);

#ifdef DEBUG
   SetMessage(ew, "Help CB called!", 0);
#else
   SetMessage(ew, " ", 0);
#endif

   switch (p-> item_index)
      {
      case HelpApp:
         PostGizmoHelp(shell, &ApplicationHelp);
         break;
      case HelpTOC:
         PostGizmoHelp(shell, &TOCHelp);
         break;
      case HelpHelpDesk:
         PostGizmoHelp(shell, &HelpDeskHelp);
         break;
      default:
         fprintf(stderr, "error: default in HelpCB\n");
         break;
      }

} /* end of HelpCB */
