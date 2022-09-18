/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define USE_TIME_GIZMO
#ifndef NOIDENT
#pragma ident	"@(#)dtclock:setalarm.c	1.8"
#endif

/*
 *      setalarm.c
 *
 */

#include <stdio.h>
#include <time.h>
#include <limits.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <OpenLook.h>
#include <FButtons.h>
#include <ControlAre.h>
#include <PopupWindo.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <InputGizmo.h>
#ifdef USE_TIME_GIZMO
#include <TimeGizmo.h>
#else
#include <NumericGiz.h>
#include <ChoiceGizm.h>
#endif

#include <clock.h>
#include <alarm.h>
#include <crontab.h>
#include <setalarm.h>

#define ENV_FILE   ".dtclock.env"

extern char ** environ;

static char * GetEnvironment(char * var, char * def_value);
static int    WriteEnvironment(char * path, char * filename);
static void   CommandCB(Widget, XtPointer, XtPointer);

typedef enum 
   { CommandSet, CommandReset, CommandCancel, CommandHelp } 
   CommandMenuItemIndex;

static MenuItems  CommandMenuItems[] =
   {
      {True, TXT_COMMAND_SET,    MNE_COMMAND_SET    },
      {True, TXT_COMMAND_RESET,  MNE_COMMAND_RESET  },
      {True, TXT_COMMAND_CANCEL, MNE_COMMAND_CANCEL },
      {True, TXT_COMMAND_HELP,   MNE_COMMAND_HELP   },
      { 0 }
   };

static MenuGizmo CommandMenu = 
   { NULL, "_X_", "_X_", CommandMenuItems, CommandCB };

static Setting MessageSetting;
#ifdef USE_TIME_GIZMO
static Setting TimeSetting;
#else
static Setting HourSetting =
   { NULL, NULL, NULL, (XtPointer)1 };
static Setting MinuteSetting;
#endif

static InputGizmo   MessageInput = 
   { NULL, "_X_", TXT_MESSAGE_PROMPT, TXT_DEFAULT_MESSAGE, &MessageSetting, 30 };
#ifdef USE_TIME_GIZMO
static TimeGizmo   TimeInput = 
   { NULL, "_X_", TXT_TIME_PROMPT, TXT_TIME, &TimeSetting };
#else
static NumericGizmo HourInput = 
   { NULL, "_X_", TXT_HOUR_PROMPT,   1, 12, &HourSetting };
static NumericGizmo MinuteInput = 
   { NULL, "_X_", TXT_MINUTE_PROMPT, 0, 59, &MinuteSetting };

static Setting AmPmSetting;
static MenuItems  AmPmItems[] =
   {
      {True, TXT_AM,  MNE_AM, "am" },
      {True, TXT_PM,  MNE_PM, "pm" },
      { 0 }
   };
static MenuGizmo  AmPmMenu =
   { NULL, "_X_", "_X_", AmPmItems, NULL, NULL, EXC };
static ChoiceGizmo AmPmChoice = 
   { NULL, "_X_", TXT_AMPM, &AmPmMenu, &AmPmSetting };
#endif

static GizmoRec Commands[] =
   {
      { InputGizmoClass,   &MessageInput },
#ifdef USE_TIME_GIZMO
      { TimeGizmoClass,    &TimeInput    },
#else
      { NumericGizmoClass, &HourInput    },
      { NumericGizmoClass, &MinuteInput  },
      { ChoiceGizmoClass,  &AmPmChoice   },
#endif
   };

extern PopupGizmo AlarmPrompt =
   { &SetAlarmHelp, "_X_", TXT_ALARM_TITLE, &CommandMenu, Commands, XtNumber(Commands) };


/*
 * AlarmCB
 *
 */

extern void
AlarmCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);

   if (mw-> alarmPrompt == NULL)
   {
      MessageInput.text = GetGizmoText(MessageInput.text);
      mw-> alarmPrompt = CopyGizmo(PopupGizmoClass, &AlarmPrompt);
      CreateGizmo(w, PopupGizmoClass, mw-> alarmPrompt, NULL, 0);
   }
   MapGizmo(PopupGizmoClass, mw-> alarmPrompt);

} /* end of AlarmCB */
/*
 * GetEnvironment
 *
 */

static char *
GetEnvironment(char * var, char * def_value)
{
   char ** p;
   char *  q;
   int     len = strlen(var);

   for (p = environ; *p != NULL; p++)
      if (strncmp(*p, var, len) == 0)
         break;

   if (*p == NULL)
   {
      q = def_value;
   }
   else
   {
      q = *p;
      q = &q[len];
   }

   return (q);

} /* end of GetEnvironment */
/*
 * WriteEnvironment
 *
 */

static int
WriteEnvironment(char * path, char * filename)
{
   char *  var;
   char *  val;
   FILE *  fp;
   char ** p;
   char    fullfilename[PATH_MAX];
   char    buffer[1024];

   (void)strcpy(fullfilename, path);
   (void)strcat(fullfilename, "/");
   (void)strcat(fullfilename, filename);

   if ((fp = fopen(fullfilename, "w")) == NULL)
   {
#ifdef DEBUG
      (void)fprintf(stderr, "Can't open environment file '%s'\n", fullfilename);
#endif
      return False;
   }
   else
   {
#ifdef DEBUG
      (void)fprintf(stderr, "Writing environment file '%s'\n", fullfilename);
#endif
      for (p = environ; *p; p++)
      {
#define STACK_STRDUP(p, buffer, size, string)              \
                   char * p;                               \
                   char   buffer[size];                    \
                   if (strlen(string) + 1 > size)          \
                      p = strdup(string);                  \
                   else                                    \
                      p = strcpy(buffer, string);

#define STACK_FREE(namep, name)        if (namep != name)  \
                                          free(namep);

         STACK_STRDUP(stringp, string, 1024, *p);
         var = strtok(stringp, "=");
         val = strtok(NULL, "");
         fprintf(fp, "%s=\"%s\";   export %s\n", var, val, var);
         STACK_FREE(stringp, string);
      }
      fclose(fp);
      return True;
   }

} /* end of WriteEnvironment */
/*
 * CommandCB
 *
 */

static void
CommandCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);
   PopupGizmo *     popup      = mw-> alarmPrompt;
   Widget           shell      = GetPopupGizmoShell(popup);
/*
 * FIX: is command big enough?
 */
   char             command[4096];
   Setting *        text;
#ifdef USE_TIME_GIZMO
   Setting *        time;
   int              hour;
   int              minute;
#else
   Setting *        hour;
   Setting *        minute;
   Setting *        ampm;
#endif
   char *           home;

   switch (p-> item_index)
      {
      case CommandSet:
         home = GetEnvironment("HOME=", "/tmp");
         if (WriteEnvironment(home, ENV_FILE) == False)
         {
            /* FIX: needed? should a notice be posted
             * if the write fails?  The user's home should be writable!
             */
         }

         ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
         text = (Setting *)QueryGizmo(popup->gizmos[0].gizmo_class,
                                      popup->gizmos[0].gizmo,
                                      GetGizmoSetting, NULL);
#ifdef USE_TIME_GIZMO
         time = (Setting *)QueryGizmo(popup->gizmos[1].gizmo_class,
                                      popup->gizmos[1].gizmo,
                                      GetGizmoSetting, NULL);
         sscanf(time->current_value, "%d:%d", &hour, &minute);

         sprintf(command, 
            "%d %d * * * " ALARM_CLIENT " %s/%s" " \\\"%s\\\"", 
            minute, hour, 
            home, ENV_FILE,
            text->current_value);
#else
         hour = (Setting *)QueryGizmo(popup->gizmos[1].gizmo_class,
                                      popup->gizmos[1].gizmo,
                                      GetGizmoSetting, NULL);
         minute = (Setting *)QueryGizmo(popup->gizmos[2].gizmo_class,
                                      popup->gizmos[2].gizmo,
                                      GetGizmoSetting, NULL);
         ampm = (Setting *)QueryGizmo(popup->gizmos[3].gizmo_class,
                                      popup->gizmos[3].gizmo,
                                      GetGizmoSetting, NULL);

         sprintf(command, 
            "%d %d * * * " ALARM_CLIENT " %s/%s" " \\\"%s\\\"", 
            minute->current_value,
            ((int)hour->current_value == 12) ?
            ((int)ampm->current_value == 0)  ? 0 : 12
                                             :
            (int)hour->current_value + (int)ampm->current_value * 12,
            home, ENV_FILE,
            text->current_value);
#endif
         ReplaceCrontabEntry(ALARM_CLIENT, command);
         BringDownPopup(shell);
         break;
      case CommandReset:
         DeleteCrontabEntry(ALARM_CLIENT);
/*
         ManipulateGizmo(PopupGizmoClass, popup, ResetGizmoValue);
*/
         break;
      case CommandCancel:
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         break;
      case CommandHelp:
         PostGizmoHelp(mw->shell, popup->help);
         break;
      default:
         (void)fprintf(stderr,"default at %d in %s\n", __LINE__, __FILE__);
         break;
      }

} /* end of CommandCB */
