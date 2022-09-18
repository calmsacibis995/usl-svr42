/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:prop.c	1.5"
#endif

/*
 *      prop.c
 *
 */

#include <stdio.h>
#include <time.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <OpenLook.h>
#include <FButtons.h>
#include <ControlAre.h>
#include <PopupWindo.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <ChoiceGizm.h>

#include <clock.h>
#include <prop.h>

extern ClockSettings ClockSetting;

static void PropCB(Widget, XtPointer, XtPointer);

typedef enum 
   { PropApply, PropSet, PropReset, PropFactory, PropCancel, PropHelp } 
   PropMenuItemIndex;

static MenuItems  PropMenuItems[] =
   {
      {True, TXT_APPLY,            MNE_APPLY            },
      {True, TXT_SET_DEFAULTS,     MNE_SET_DEFAULTS     },
      {True, TXT_RESET,            MNE_RESET            },
      {True, TXT_RESET_TO_FACTORY, MNE_RESET_TO_FACTORY },
      {True, TXT_CANCEL,           MNE_CANCEL           },
      {True, TXT_PROP_HELP,        MNE_PROP_HELP        },
      { 0 }
   };
static MenuGizmo  PropMenu    = { NULL, "_X_", "_X_", PropMenuItems, PropCB };

static MenuItems  ChimeItems[] =
   {
      {True, TXT_NONE,         MNE_NONE,        "silent" },
      {True, TXT_TRADITIONAL,  MNE_TRADITIONAL, "normal" },
      {True, TXT_SHIPSBELLS,   MNE_SHIPSBELLS,  "shipsbells" },
      { 0 }
   };
static MenuGizmo  ChimeMenu = 
   { NULL, "_X_", "_X_", ChimeItems, NULL, NULL, EXC };

static MenuItems  ModesItems[] =
   {
      {True, TXT_ANALOG,       MNE_ANALOG,  "analog" },
      {True, TXT_DIGITAL,      MNE_DIGITAL, "digital" },
      { 0 }
   };
static MenuGizmo  ModesMenu = 
   { NULL, "_X_", "_X_", ModesItems, NULL, NULL, EXC };

static MenuItems  TicksItems[] =
   {
      {True, TXT_SECOND, MNE_SECOND, "second" },
      {True, TXT_MINUTE, MNE_MINUTE, "minute" },
      { 0 }
   };
static MenuGizmo  TicksMenu = 
   { NULL, "_X_", "_X_", TicksItems, NULL, NULL, EXC };

static ChoiceGizmo ChimeChoice = 
   { NULL, CHIME, TXT_CHIMES, &ChimeMenu, &ClockSetting.chime };
static ChoiceGizmo ModesChoice = 
   { NULL, MODES, TXT_MODE,   &ModesMenu, &ClockSetting.modes };
static ChoiceGizmo TicksChoice = 
   { NULL, TICKS, TXT_TICK,   &TicksMenu, &ClockSetting.ticks };

static GizmoRec Props[] =
   {
      { ChoiceGizmoClass, &ChimeChoice },
      { ChoiceGizmoClass, &ModesChoice },
      { ChoiceGizmoClass, &TicksChoice },
   };

extern PopupGizmo PropertiesPrompt =
   { &PropWinHelp, "_X_", TXT_PROP_TITLE, &PropMenu, Props, XtNumber(Props) };


/*
 * PropertyCB
 *
 */

extern void
PropertyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);

   if (mw-> propertiesPrompt == NULL)
   {
      mw-> propertiesPrompt = CopyGizmo(PopupGizmoClass, &PropertiesPrompt);
      CreateGizmo(w, PopupGizmoClass, mw-> propertiesPrompt, NULL, 0);
   }
   MapGizmo(PopupGizmoClass, mw-> propertiesPrompt);

} /* end of PropertyCB */
/*
 * PropCB
 *
 */

static void
PropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p            = (OlFlatCallData *)call_data;
   MainWindow *     mw           = FindMainWindow(w);
   PopupGizmo *     popup        = mw-> propertiesPrompt;
   Widget           shell        = GetPopupGizmoShell(popup);
   ManipulateOption apply_or_set = ApplyGizmoValue;
   Boolean          redisplay;
   Setting *        chime;
   Setting *        mode;
   Setting *        tick;

   switch (p-> item_index)
      {
      case PropSet:
         apply_or_set = SetGizmoValue;
         /*
          * fall through
          */
      case PropApply:
         ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);

         chime = (Setting *) QueryGizmo(popup->gizmos[0].gizmo_class,
                                        popup->gizmos[0].gizmo,
                                        GetGizmoSetting, NULL);
         mode = (Setting *) QueryGizmo(popup->gizmos[1].gizmo_class, 
                                       popup->gizmos[1].gizmo, 
                                       GetGizmoSetting, NULL);
         tick = (Setting *) QueryGizmo(popup->gizmos[2].gizmo_class, 
                                       popup->gizmos[2].gizmo, 
                                       GetGizmoSetting, NULL);

         redisplay = (mode->current_value != mode->previous_value) ||
                     (tick->current_value != tick->previous_value);

         mw-> chime = (int)chime->current_value;
         mw-> mode = 
            ((int)mode->current_value == ModesDigital) ? DIGITAL : ANALOG;
         mw-> UpdateTime = 
            ((int)tick->current_value == TicksMinute)  ? 60000   : 1000;

         ManipulateGizmo(PopupGizmoClass, popup, apply_or_set);

         SetHints(mw);
         ResetTimer(mw);

         if (redisplay)
            XClearArea(XtDisplay(mw-> pane), XtWindow(mw-> pane), 0, 0, 0, 0, True);
         BringDownPopup(shell);
         break;
      case PropReset:
         ManipulateGizmo(PopupGizmoClass, popup, ResetGizmoValue);
         break;
      case PropFactory:
         ManipulateGizmo(PopupGizmoClass, popup, ReinitializeGizmoValue);
         break;
      case PropCancel:
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         break;
      case PropHelp:
         PostGizmoHelp(mw->shell, popup->help);
         break;
      default:
         (void)fprintf(stderr,"default at %d in %s\n", __LINE__, __FILE__);
         break;
      }

} /* end of PropCB */
