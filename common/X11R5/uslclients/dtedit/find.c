/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:find.c	1.6"
#endif

/*
 * find.c
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
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <InputGizmo.h>

#include <editor.h>
#include <find.h>

extern HelpInfo FindWinHelp = 
   { FormalClientName, TXT_FIND_HELP_TITLE,   HELPPATH, TXT_FIND_HELP_SECT };

static void PostFindCB(Widget w, XtPointer client_data, XtPointer call_data);

typedef enum 
   {
      FindNext, FindPrev, FindCancel, FindHelp
   } FindMenuItemIndex;

static MenuItems  FindMenuItems[] =
   {
      { True, TXT_FIND_NEXT,        MNE_FIND_NEXT },
      { True, TXT_FIND_PREV,        MNE_FIND_PREV },
      { True, TXT_CANCEL,           MNE_CANCEL    },
      { True, TXT_HELP_DDD,         MNE_HELP_DDD  },
      { 0 }
   };
static MenuGizmo  FindMenu    = 
   { NULL, "findmenu", "_X_", FindMenuItems, PostFindCB };

static Setting FindSetting;

static InputGizmo FindInput = 
   { NULL, "_X_", TXT_FIND_PROMPT, TXT_NULL_STRING, &FindSetting, 30 };

static GizmoRec    FindGizmos[] =
   {
      { InputGizmoClass,  &FindInput },
   };

extern PopupGizmo FindPrompt  = 
   { NULL, "find", TXT_FIND_TITLE, &FindMenu, FindGizmos, XtNumber(FindGizmos)};


/*
 * FindCB
 *
 * This callback procedure posts the find prompt dialog window.
 * It's called as the selectProc for the Find button.
 *
 */

extern void
FindCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);

   if (ew->findPrompt == NULL)
   {
      ew->findPrompt = CopyGizmo(PopupGizmoClass, &FindPrompt);
      CreateGizmo(w, PopupGizmoClass, ew->findPrompt, NULL, 0);
   }
   MapGizmo(PopupGizmoClass, ew->findPrompt);
   SetMessage(ew, TXT_FIND_POPUP, 0);

} /* end of FindCB */
/*
 * xSetPopupMessage
 *
 * a temporary routine used to set the message of a popup
 *
 */

static void
xSetPopupMessage(PopupGizmo * gizmo, char * message, int error)
{
   SetPopupMessage(gizmo, GetGizmoText(message));
} /* end of xSetPopupMessage */
/*
 * PostFindCB
 *
 * This callback procedure handles the selection of buttons in
 * the menubar located in the find prompt dialog.
 *
 */

static void
PostFindCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p            = (OlFlatCallData *)call_data;
   EditWindow *     ew           = FindEditWindow(w);
   PopupGizmo *     popup        = ew->findPrompt;
   Widget           shell        = GetPopupGizmoShell(popup);
   int              prev_or_next = p->item_index;
   char *           text;
   Arg              arg[10];
   int              n;
   ScanResult       sr;

   switch (p->item_index)
      {
      case FindNext:
      case FindPrev:
         text = GetInputText(popup, 0);
#ifdef DEBUG
         (void)fprintf(stderr, "Find '%s'\n", text);
#endif
         if (*text)
         {
            TextLocation location;
            TextPosition start;
            TextPosition end;
            TextPosition cursor;

            OlTextEditGetCursorPosition(ew->text, &start, &end, &cursor);

            if (prev_or_next == FindNext)
            {
               location = LocationOfPosition(ew->textBuffer, start);
               sr = ForwardScanTextBuffer(ew->textBuffer, text, &location);
            }
            else
            {
               location = LocationOfPosition(ew->textBuffer, end);
               location = PreviousLocation(ew->textBuffer, location);
               sr = BackwardScanTextBuffer(ew->textBuffer, text, &location);
            }
            switch (sr)
            {
               case SCAN_NOTFOUND:
                  xSetPopupMessage(popup, TXT_FIND_FAILED, 0);
                  break;
               case SCAN_WRAPPED:
                  start = PositionOfLocation(ew->textBuffer, location);
                  end = cursor = start + _mbstrlen(text); /* FIX: this is at least not precise */
                  OlTextEditSetCursorPosition(ew->text, start, end, cursor);
                  xSetPopupMessage(popup, TXT_FIND_WRAPPED, 0);
                  break;
               case SCAN_FOUND:
                  start = PositionOfLocation(ew->textBuffer, location);
                  end = cursor = start + _mbstrlen(text); /* FIX: this is at least not precise */
                  OlTextEditSetCursorPosition(ew->text, start, end, cursor);
                  xSetPopupMessage(popup, TXT_FIND_FOUND, 0);
                  break;
               default:
                  break;
            }
         }
         else
            xSetPopupMessage(popup, TXT_FIND_NOTHING, 0);
         FREE(text);
         break;
      case FindCancel:
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         SetMessage(ew, TXT_FIND_CANCEL, 0);
         break;
      case FindHelp:
         PostGizmoHelp(GetBaseWindowShell(ew->baseWindow), &FindWinHelp);
         xSetPopupMessage(popup, " ", 0);
         break;
      default:
         (void)fprintf(stderr,"default at %d in %s\n", __LINE__, __FILE__);
         xSetPopupMessage(popup, "default in find", 1);
         break;
      }

} /* end of PostFindCB */
