/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:prop.c	1.10"
#endif

/*
 * prop.c
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
#include <ChoiceGizm.h>

#include <editor.h>
#include <prop.h>
#include <Margin.h>

static void PostPropCB(Widget w, XtPointer client_data, XtPointer call_data);

typedef enum 
   {
      PropApply, PropSet, PropReset, PropFactory, PropCancel, PropHelp
   } PropMenuItemIndex;
static MenuItems  PropMenuItems[] =
   {
      { True, TXT_APPLY,            MNE_APPLY            },
      { True, TXT_SET_DEFAULT,      MNE_SET_DEFAULT      },
      { True, TXT_RESET,            MNE_RESET            },
      { True, TXT_RESET_TO_FACTORY, MNE_RESET_TO_FACTORY },
      { True, TXT_CANCEL,           MNE_CANCEL           },
      { True, TXT_HELP_DDD,         MNE_HELP_DDD         },
      { 0 }
   };
static MenuGizmo  PropMenu    = 
   { NULL, "propmenu", "_X_", PropMenuItems, PostPropCB };

static MenuItems  WrapMenuItems[] =
   {
      { True, TXT_WORDS,   MNE_WORDS,   "word" },
      { True, TXT_CHARS,   MNE_CHARS,   "char" },
      { True, TXT_NO_WRAP, MNE_NO_WRAP, "none" },
      { 0 }
   };
static MenuGizmo  WrapMenu    = 
   { NULL, "wrapmenu", "_X_", WrapMenuItems, NULL, NULL, EXC };

static MenuItems  NumbMenuItems[] =
   {
      { True, TXT_NO_NUMBER, MNE_NO_NUMBER, "none" },
      { True, TXT_LEFT,      MNE_LEFT,      "left"},
#ifdef SUPPORT_RIGHT_MARGIN_LINE_NUMBER
      { True, TXT_RIGHT,     MNE_RIGHT,     "right" },
#endif
      { 0 }
   };
static MenuGizmo   NumbMenu    = 
   { NULL, "numbmenu", "_X_", NumbMenuItems, NULL, NULL, EXC };

static ChoiceGizmo WrapField   = 
   { NULL, WRAP, TXT_WRAPPING,  &WrapMenu, &EditorSetting.wrap };
static ChoiceGizmo NumbField   = 
   { NULL, NUMB, TXT_NUMBERING, &NumbMenu, &EditorSetting.numb };
static GizmoRec    PropInput[] = 
   { 
      { ChoiceGizmoClass,  &WrapField },
      { ChoiceGizmoClass,  &NumbField },
   };
extern PopupGizmo PropertiesPrompt  = 
   { &PropWinHelp, "props", TXT_PROP_TITLE, 
     &PropMenu, PropInput, XtNumber(PropInput) };


/*
 * PropCB
 *
 * This callback procedure posts the properties prompt dialog window.
 * It's called as the selectProc for the Properties button.
 *
 */

extern void
PropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);

   if (ew->propertiesPrompt == NULL)
   {
      ew->propertiesPrompt = CopyGizmo(PopupGizmoClass, &PropertiesPrompt);
      CreateGizmo(w, PopupGizmoClass, ew->propertiesPrompt, NULL, 0);
   }
   MapGizmo(PopupGizmoClass, ew->propertiesPrompt);
   SetMessage(ew, TXT_PROP_POPUP, 0);

} /* end of PropCB */
/*
 * PostPropCB
 *
 * This callback procedure handles the selection of buttons in
 * the menubar located in the properties prompt dialog.
 *
 */

static void
PostPropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p            = (OlFlatCallData *)call_data;
   EditWindow *     ew           = FindEditWindow(w);
   PopupGizmo *     popup        = ew->propertiesPrompt;
   Widget           shell        = GetPopupGizmoShell(popup);
   ManipulateOption apply_or_set = ApplyGizmoValue;
   Setting *        wrap;
   Setting *        numb;
   Arg              arg[10];
   int              n;

   switch (p->item_index)
      {
      case PropSet:
         apply_or_set = SetGizmoValue;
         /*
          * fall through
          */
      case PropApply:
         ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);

         /*
          * FIX: need to handle the other properties
          */

         wrap = (Setting *)QueryGizmo(popup->gizmos[0].gizmo_class,
                                      popup->gizmos[0].gizmo,
                                      GetGizmoSetting, NULL);
         numb = (Setting *)QueryGizmo(popup->gizmos[1].gizmo_class,
                                      popup->gizmos[1].gizmo,
                                      GetGizmoSetting, NULL);
         n = 0;
         if (wrap->current_value != wrap->previous_value)
         {
            XtSetArg(arg[n], XtNwrapMode, DecodeWrapMode((int)wrap->current_value));
            n++;
            if (p->item_index == PropSet)
               EditorSetting.wrap.previous_value = wrap->current_value;
         }
         if (numb->current_value != numb->previous_value)
         {
            if (numb->current_value == 0)
            {
            XtSetArg(arg[n], XtNleftMargin, 10);
            _OlUnregisterTextLineNumberMargin(ew->text);
            n++;
            }
            else
            {
            XtSetArg(arg[n], XtNleftMargin, 50);
            _OlRegisterTextLineNumberMargin(ew->text);
            n++;
            }
            if (p->item_index == PropSet)
               EditorSetting.numb.previous_value = numb->current_value;
         }
        
         if (n)
            XtSetValues(ew->text, arg, n);

         ManipulateGizmo(PopupGizmoClass, popup, apply_or_set);
         BringDownPopup(shell);
         SetMessage(ew, TXT_PROP_APPLIED, 0);
         break;
      case PropReset:
         ManipulateGizmo(PopupGizmoClass, popup, ResetGizmoValue);
         SetMessage(ew, TXT_PROP_RESET, 0);
         break;
      case PropFactory:
         ManipulateGizmo(PopupGizmoClass, popup, ReinitializeGizmoValue);
         SetMessage(ew, TXT_PROP_FACTORY, 0);
         break;
      case PropCancel:
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         SetMessage(ew, TXT_PROP_CANCEL, 0);
         break;
      case PropHelp:
         PostGizmoHelp(GetBaseWindowShell(ew->baseWindow), &PropWinHelp);
         SetMessage(ew, " ", 0);
         break;
      default:
         (void)fprintf(stderr,"default at %d in %s\n", __LINE__, __FILE__);
         SetMessage(ew, "default in prop", 1);
         break;
      }

} /* end of PostPropCB */
/*
 * DecodeWrapMode
 *
 */

extern int
DecodeWrapMode(int mode)
{
   int wrap_mode;

   switch(mode)
   {
      default:
      case WrapWord: wrap_mode = OL_WRAP_WHITE_SPACE; break;
      case WrapChar: wrap_mode = OL_WRAP_ANY;         break;
      case WrapNone: wrap_mode = OL_WRAP_OFF;         break;
   }

   return (wrap_mode);

} /* end of DecodeWrapMode */
