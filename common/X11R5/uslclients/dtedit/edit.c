/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:edit.c	1.6"
#endif

#define FIX_TEW

/*
 * edit.c
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
#include <BaseWGizmo.h>

#include <editor.h>
#include <edit.h>


/*
 * EditCB
 *
 * This callback procedure handles the buttons in the edit submenu of the
 * base window's menu bar.
 *
 */

extern void 
EditCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);

#ifdef DEBUG
   SetMessage(ew, "Edit CB called!", 0);
#else
   SetMessage(ew, " ", 0);
#endif

   switch (p-> item_index)
      {
      case EditUndo:
         OlActivateWidget(ew-> text, OL_UNDO, NULL);
         SetMessage(ew, TXT_UNDO_PERFORMED, 0);
         break;
      case EditCut:
#ifdef FIX_TEW
         OlActivateWidget(ew-> text, OL_CUT, NULL);
#else
         (void) OlTextEditCopySelection(ew-> text, True);
#endif
         SetMessage(ew, TXT_CUT_PERFORMED, 0);
         break;
      case EditCopy:
#ifdef FIX_TEW
         OlActivateWidget(ew-> text, OL_COPY, NULL);
#else
         (void) OlTextEditCopySelection(ew-> text, False);
#endif
         SetMessage(ew, TXT_COPY_PERFORMED, 0);
         break;
      case EditPaste:
         OlActivateWidget(ew-> text, OL_PASTE, NULL);
         SetMessage(ew, TXT_PASTE_PERFORMED, 0);
         break;
      case EditClear:
         OlTextEditInsert(ew-> text, "", 0);
         SetMessage(ew, TXT_CLEAR_PERFORMED, 0);
         break;
      case EditSelectAll:
         OlActivateWidget(ew-> text, OLM_KSelectAll, NULL);
         SetMessage(ew, TXT_SELECT_PERFORMED, 0);
         break;
      case EditUnselectAll:
         OlActivateWidget(ew-> text, OLM_KDeselectAll, NULL);
         SetMessage(ew, TXT_UNSELECT_PERFORMED, 0);
         break;
      default:
         break;
      }

} /* end of EditCB */
