/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:view.c	1.3"
#endif

/*
 * view.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <buffutil.h>
#include <textbuff.h>

#include <OpenLook.h>
#include <TextEdit.h>
#include <FButtons.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <ModalGizmo.h>
#include <BaseWGizmo.h>
#include <editor.h>
#include <view.h>


/*
 * ViewCB
 *
 */

extern void 
ViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);
   TextBuffer *     textBuffer = OlTextEditTextBuffer(ew-> text);

#ifdef DEBUG
   SetMessage(ew, "View CB called!", 0);
#else
   SetMessage(ew, " ", 0);
#endif

   switch (p-> item_index)
      {
      case ViewSplit:
         CreateEditWindow(root, ew-> filename, textBuffer);
         SetMessage(ew, TXT_OPENED_ANOTHER, 0);
         /*
          * NOTE: relies on new window being added to the front of the list
          */
         SetMessage(filelist, TXT_NEW_VIEW, 0);
         break;
      case ViewPreferences:
         SetMessage(ew, "called view preferences!", 0);
         break;
      default:
         SetMessage(ew, "called view default!", 1);
         break;
      }

} /* end of ViewCB */
