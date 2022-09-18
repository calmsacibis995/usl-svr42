/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:quit.c	1.7"
#endif

/*
 * quit.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <buffutil.h>
#include <textbuff.h>

#include <OpenLook.h>
#include <FButtons.h>
#include <TextEdit.h>

typedef void (*PFV)();

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <ModalGizmo.h>
#include <BaseWGizmo.h>
#include <editor.h>
#include <quit.h>


static void ExitNoticeCB(Widget w, XtPointer client_data, XtPointer call_data);
typedef enum
   {
      ExitNoticeContinue,
      ExitNoticeCancel,
      ExitNoticeHelp
   } ExitNoticeMenuItemIndex;
static MenuItems  ExitNoticeMenuItems[] =
   {
      {True, TXT_EXIT,     MNE_EXIT    },
      {True, TXT_CANCEL,   MNE_CANCEL  },
      {True, TXT_HELPDDD,  MNE_HELPDDD },
      { 0 }
   };
static MenuGizmo  ExitNoticeMenu =
   { NULL, "exitnoticemenu", "_X_", ExitNoticeMenuItems, ExitNoticeCB };
static ModalGizmo ExitNotice =
   { &ENoteHelp, "exitnotice", TXT_EXIT_TITLE, &ExitNoticeMenu, TXT_EXIT_NOTICE };


/*
 * QuitCB
 *
 */

extern void 
QuitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   EditWindow * ew         = FindEditWindow(w);
   TextBuffer * textBuffer = ew-> textBuffer;

   if (TextBufferModified(textBuffer))
   {
      if (ew-> exitNotice == NULL)
         {
            ew-> exitNotice = CopyGizmo(ModalGizmoClass, &ExitNotice);
            CreateGizmo(w, ModalGizmoClass, ew-> exitNotice, NULL, 0);
         }
      MapGizmo(BaseWindowGizmoClass, ew-> baseWindow);
      MapGizmo(ModalGizmoClass, ew-> exitNotice);
      SetMessage(ew, TXT_DISCARD_CHANGES, 0);
   }
   else
   {
      DestroyEditWindow(ew);
   }

} /* end of QuitCB */
/*
 * ExitNoticeCB
 *
 */

static void
ExitNoticeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);
   Widget           shell      = (Widget)_OlGetShellOfWidget(w);

   switch(p-> item_index)
   {
      case ExitNoticeContinue:
         DestroyEditWindow(ew);
         break;
      case ExitNoticeCancel:
         SetMessage(ew, TXT_EXIT_CANCEL, 0);
         XtPopdown(shell);
         break;
      case ExitNoticeHelp:
         PostGizmoHelp(GetBaseWindowShell(ew->baseWindow), &ENoteHelp);
         SetMessage(ew, TXT_HELP_POSTED, 0);
         break;
      default:
         (void)fprintf(stderr, "exit notice default at %d in %s", __LINE__, __FILE__);
         SetMessage(ew, "exit notice default called", 1);
         break;
   }

} /* end of ExitNoticeCB */
