/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:alarm.c	1.7"
#endif

/*
 *      Desktop UNIX(r) System Alarm
 *
 *      alarm.c
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <OpenLook.h>
#include <FButtons.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>

#include <alarm.h>
#include <crontab.h>

#define FormalClientName   "dtalarm:1" FS "Alarm"

#define ClientName         "dtalarm"
#define ClientClass        "DTalarm"

#define TXT_NULL_MESSAGE   "dtalarm:2"  FS "default alarm message"
#define TXT_ALARM_REARM    "dtalarm:3"  FS "Rearm"
#define TXT_ALARM_DISARM   "dtalarm:4"  FS "Disarm"
#define TXT_ALARM_HELP     "dtalarm:5"  FS "Help..."
#define TXT_TITLE          "dtalarm:6"  FS "Alarm"

#define MNE_ALARM_REARM    "dtalarm:13" FS "R"
#define MNE_ALARM_DISARM   "dtalarm:14" FS "D"
#define MNE_ALARM_HELP     "dtalarm:15" FS "H"

static void InitializeAlarm(Widget);
static void AlarmCB(Widget, XtPointer, XtPointer);

typedef enum { AlarmRearm, AlarmDisarm, AlarmHelp } AlarmMenuItemIndex;

static MenuItems  AlarmMenuItems[] =
   {
   { True, TXT_ALARM_REARM,  MNE_ALARM_REARM  },
   { True, TXT_ALARM_DISARM, MNE_ALARM_DISARM },
#ifdef HelpAvailable
   { True, TXT_ALARM_HELP,   MNE_ALARM_HELP   },
#endif
   { 0 }
   };

static MenuGizmo AlarmMenu =
   { NULL, "_X_", "_X_", AlarmMenuItems, AlarmCB, NULL, CMD, OL_FIXEDROWS, 1 };

static ModalGizmo AlarmNotice = 
   { NULL, "_X_", TXT_TITLE, &AlarmMenu, TXT_NULL_MESSAGE };


/*
 * main
 *
 * This client posts a simple modal dialog window with a message
 * provided either by default or by the first argument.  The routine
 * presumes to be called by cron(1) and allows the user to cancel
 * the alarm or to reset it.  The routine is designed to be a generic
 * alarm mechanism and is used by the Desktop UNIX System Clock (dtclock)
 * to handle the clock alarm.
 *
 */

main(argc, argv)
int    argc;
char * argv[];
{
   Widget root;

   root = InitializeGizmoClient(ClientName, ClientClass,
      FormalClientName,
      NULL, NULL,
      NULL, 0,
      &argc, argv,
      NULL,
      NULL, NULL, 0, NULL, 0, NULL, NULL, NULL);

   if (argc > 1)
      AlarmNotice.message = argv[1];

   InitializeAlarm(root);

   XtMainLoop();

} /* end of main */
/*
 * InitializeAlarm
 *
 * This procedure creates the AlarmNotice dialog window and maps it.
 *
 */

static void 
InitializeAlarm(Widget Shell)
{

   (void)CreateGizmo(Shell, ModalGizmoClass, &AlarmNotice, NULL, 0);

   MapGizmo(ModalGizmoClass, &AlarmNotice);

} /* end of InitializeAlarm */
/*
 * AlarmCB
 *
 *
 * The callback procedure is called when any of the buttons in the menu bar
 * of the AlarmNotice dialog are selected.  The callback switched on the
 * index of the flat button in the menu bar and either:~
 * .BL
 * .LI
 * Rearms the alarm by simply exiting.
 * .LI
 * Disarms the alarm by removing the crontab entry, then exiting.
 * LI
 * Retrieves help for the user.
 * .LE
 * 
 */

static void
AlarmCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;

   switch(p-> item_index)
   {
      case AlarmRearm:
         exit(0);
         break;
      case AlarmDisarm:
         DeleteCrontabEntry(ALARM_CLIENT);
         exit(0);
         break;
      case AlarmHelp:
         (void)fprintf(stderr,"help in AlarmCB taken!!!\n");
         break;
      default:
         (void)fprintf(stderr,"default in AlarmCB taken!!!\n");
   }

} /* end of AlarmCB */
