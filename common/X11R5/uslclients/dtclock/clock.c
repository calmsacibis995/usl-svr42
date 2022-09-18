/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:clock.c	1.27"
#endif

/*
 *      Desktop UNIX(r) System Clock
 *
 *      clock.c
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Desktop.h>

#include <OpenLook.h>
#include <FButtons.h>
#include <BulletinBo.h>

#include <time.h>

#include <Gizmos.h>
#include <PopupGizmo.h>
#include <ChoiceGizm.h>
#include <MenuGizmo.h>

#include <clock.h>
#include <prop.h>
#include <setalarm.h>

#define ClientName         "dtclock"
#define ClientClass        "dtclock"

#define MIN_ANALOG_HT    100
#define MIN_ANALOG_WID   100
#define MIN_ANALOG_ASP_X   1
#define MIN_ANALOG_ASP_Y   1
#define MAX_ANALOG_ASP_X   1
#define MAX_ANALOG_ASP_Y   1

#define MIN_DIGITAL_HT    50
#define MIN_DIGITAL_WID  100
#define MIN_DIGITAL_ASP_X  10
#define MIN_DIGITAL_ASP_Y   5
#define MAX_DIGITAL_ASP_X  10
#define MAX_DIGITAL_ASP_Y   5

#define min(x,y)        ((x) < (y) ? (x) : (y))
#define max(x,y)        ((x) > (y) ? (x) : (y))

#define HR_HAND_LENGTH_FACTOR     60
#define MIN_HAND_LENGTH_FACTOR    80
#define SEC_HAND_LENGTH_FACTOR    90
#define HR_HAND_WIDTH_FACTOR       8
#define MIN_HAND_WIDTH_FACTOR      7
#define SEC_HAND_WIDTH_FACTOR      2
#define HR_TAIL_FACTOR             3
#define MIN_TAIL_FACTOR            4
#define SEC_TAIL_FACTOR            8

extern HelpInfo SetAlarmHelp =
   { FormalClientName, TXT_ALARM_HELP_TITLE, HELPPATH, TXT_ALARM_HELP_SECT };
extern HelpInfo PropWinHelp =
   { FormalClientName, TXT_PROP_HELP_TITLE,  HELPPATH, TXT_PROP_HELP_SECT };

extern HelpInfo ApplicationHelp = 
   { FormalClientName, TXT_MAIN_HELP_TITLE,  HELPPATH, TXT_MAIN_HELP_SECT };
extern HelpInfo TOCHelp  = 
   { FormalClientName, TXT_TOC_HELP_TITLE,   HELPPATH, "TOC" };
extern HelpInfo HelpDeskHelp = 
   { FormalClientName, TXT_HELPDESK_TITLE,   HELPPATH, TXT_HELPDESK_SECT };

ApplicationResources ClockResources;

static char * Weekday[] =
   {
   TXT_SUNDAY, 
   TXT_MONDAY, 
   TXT_TUESDAY, 
   TXT_WEDNESDAY, 
   TXT_THURSDAY, 
   TXT_FRIDAY, 
   TXT_SATURDAY
   };

static char * weekday[] =
   {
   TXT_SUN, 
   TXT_MON, 
   TXT_TUE, 
   TXT_WED, 
   TXT_THU, 
   TXT_FRI, 
   TXT_SAT
   };

static char * Month[] =
   {
   TXT_JANUARY,
   TXT_FEBRUARY,
   TXT_MARCH,
   TXT_APRIL,
   TXT_MAY,
   TXT_JUNE,
   TXT_JULY,
   TXT_AUGUST,
   TXT_SEPTEMBER,
   TXT_OCTOBER,
   TXT_NOVEMBER,
   TXT_DECEMBER
   };

static char * month[] =
   {
   TXT_JAN,
   TXT_FEB,
   TXT_MAR,
   TXT_APR,
   TXT_MA,
   TXT_JUN,
   TXT_JUL,
   TXT_AUG,
   TXT_SEP,
   TXT_OCT,
   TXT_NOV,
   TXT_DEC
   };

static float sin[] = 
   {
   0.000000, 0.104528, 0.207912, 0.309017, 0.406737,
   0.500000, 0.587785, 0.669131, 0.743145, 0.809017,
   0.866025, 0.913545, 0.951056, 0.978148, 0.994522,
   1.000000, 0.994522, 0.978148, 0.951056, 0.913545,
   0.866025, 0.809017, 0.743145, 0.669131, 0.587785,
   0.500000, 0.406737, 0.309017, 0.207912, 0.104528,
   0.000000, -0.104528, -0.207912, -0.309017, -0.406737,
   -0.500000, -0.587785, -0.669131, -0.743145, -0.809017,
   -0.866025, -0.913545, -0.951056, -0.978148, -0.994522,
   -1.000000, -0.994522, -0.978148, -0.951056, -0.913545,
   -0.866025, -0.809017, -0.743145, -0.669131, -0.587785,
   -0.500000, -0.406737, -0.309017, -0.207912, -0.104528,
   };

static float cos[] = 
   {
   1.000000, 0.994522, 0.978148, 0.951056, 0.913545,
   0.866025, 0.809017, 0.743145, 0.669131, 0.587785,
   0.500000, 0.406737, 0.309017, 0.207912, 0.104528,
   0.000000, -0.104528, -0.207912, -0.309017, -0.406737,
   -0.500000, -0.587785, -0.669131, -0.743145, -0.809017,
   -0.866025, -0.913545, -0.951056, -0.978148, -0.994522,
   -1.000000, -0.994522, -0.978148, -0.951056, -0.913545,
   -0.866025, -0.809017, -0.743145, -0.669131, -0.587785,
   -0.500000, -0.406737, -0.309017, -0.207912, -0.104528,
   0.000000, 0.104528, 0.207912, 0.309017, 0.406737,
   0.500000, 0.587785, 0.669131, 0.743145, 0.809017,
   0.866025, 0.913545, 0.951056, 0.978148, 0.994522,
   };

static void ClockCB();
static void HelpCB();

typedef enum { HelpClock, HelpTOC, HelpHelpDesk } HelpMenuItemIndex;

static MenuItems  HelpMenuItems[] =
   {
   { True, TXT_APP_HELP,    MNE_APP_HELP },
   { True, TXT_TOC_HELP,    MNE_TOC_HELP },
   { True, TXT_HELPDESK,    MNE_HELPDESK },
   { 0 }
   };
static MenuGizmo HelpMenu =
   { NULL,      "_X_",   NULL,   HelpMenuItems, HelpCB };

typedef enum { ClockSetAlarm, ClockProperties } ClockMenuItemIndex;

static MenuItems  ClockMenuItems[] =
   {
   { True, TXT_SET_ALARM,   MNE_SET_ALARM  },
   { True, TXT_PROPERTIES,  MNE_PROPERTIES },
   { True, TXT_HELP,        MNE_HELP,      (char *)&HelpMenu },
   { 0 }
   };

static MenuGizmo ClockMenu =
   { NULL,      "_X_",   NULL,   ClockMenuItems, ClockCB };

static char ChimeDefaultString[] = "silent";
static char TicksDefaultString[] = "minute";
static char ModesDefaultString[] = "analog";

extern ClockSettings ClockSetting =
   {
   { ChimeDefaultString, (XtPointer)ChimeNone        },
   { ModesDefaultString, (XtPointer)ModesAnalog      },
   { TicksDefaultString, (XtPointer)TicksMinute      },
   };

static XtResource resources[] =
   {
   { "warnings", "warnings", XtRBoolean, sizeof(Boolean),
     (Cardinal) &ClockResources.warnings, 
     XtRString, (XtPointer)"false" },

   { "beepVolume", "beepVolume", XtRInt, sizeof(int),
     (Cardinal) &ClockResources.beepVolume, 
     XtRString, "0" },

   { MODES, MODES, MODES, sizeof(int),
     (Cardinal) &ClockSetting.modes.previous_value, 
      XtRString, (XtPointer)ModesDefaultString },

   { CHIME, CHIME, CHIME, sizeof(int),
     (Cardinal) &ClockSetting.chime.previous_value, 
      XtRString, (XtPointer)ChimeDefaultString },

   { TICKS, TICKS, TICKS, sizeof(int),
     (Cardinal) &ClockSetting.ticks.previous_value, 
      XtRString, (XtPointer)TicksDefaultString },

   { "width", "Width", XtRInt, sizeof(int),
     (Cardinal) &ClockResources.DefaultWidth, 
     XtRString, (XtPointer)"100" },

   { "height", "Height", XtRInt, sizeof(int),
     (Cardinal) &ClockResources.DefaultHeight, 
     XtRString, (XtPointer)"100" },

   { "TextFontColor", "TextFontColor", XtRPixel, sizeof(Pixel),
     (Cardinal) &ClockResources.DefaultForeground, XtRString, "black" },
   { "Background", "BackGround", XtRPixel, sizeof(Pixel),
     (Cardinal) &ClockResources.DefaultBackground, XtRString, "white" },
   };

static MainWindow * MainWindows = NULL;

static void           RegetApplicationResources(XtPointer client_data);
static void           GetGCs(Widget);
extern void           SetHints(MainWindow *);
extern void           ResetTimer(MainWindow *);
static MainWindow *   NewMainWindow();
extern MainWindow *   FindToplevel(Widget);
extern MainWindow *   FindMainWindow(Widget);
static void           InitializeClock();
static XtEventHandler HandleExpose(Widget, XtPointer, XEvent *);
extern XtEventHandler HandleConfigure(Widget, XtPointer, XEvent *);
static XtEventHandler HandleButton(Widget, XtPointer, XEvent *, Boolean *);
static void           ClockCB(Widget, XtPointer, XtPointer);
extern void           HandleClockTick(XtPointer, XtIntervalId *);
static void           SetupClockChime(Widget, int, int);
static void           HandleClockChime(XtPointer, XtIntervalId *);
static void           CalculateSizes(Widget);
static void           DrawClockFace(Widget);
static void           DrawAnalogClock(Widget, int);
static void           DrawDigitalClock(Widget, int);
static void           DrawDigit(Widget, int, int, int);
static void           DrawSegment(Widget, int, int, int);
static void           Tick(Widget);
static void           RefreshTitle(MainWindow *);

main(argc, argv)
int    argc;
char * argv[];
{
   Widget root;
   int    i;

   root = InitializeGizmoClient(ClientName, ClientClass, 
      FormalClientName, 
      PopupGizmoClass, &PropertiesPrompt, 
      NULL, 0, 
      &argc, argv, 
      NULL,
      NULL, resources, XtNumber(resources), NULL, 0, NULL, NULL, NULL);

   OlRegisterDynamicCallback(RegetApplicationResources, (XtPointer)root);

   for (i = 0; i < 12; i++)
   {
      Month[i] = GetGizmoText(Month[i]);
      month[i] = GetGizmoText(month[i]);
   }

   for (i = 0; i < 7; i++)
   {
      Weekday[i] = GetGizmoText(Weekday[i]);
      weekday[i] = GetGizmoText(weekday[i]);
   }

   InitializeClock(root);

   XtMainLoop();

} /* end of main */
/*
 * RegetApplicationResources
 *
 */

static void
RegetApplicationResources(XtPointer client_data)
{
   MainWindow * mw;
   Arg          arg[5];

   XtGetApplicationResources
      ((Widget)client_data, NULL, resources, XtNumber(resources), NULL, 0);

   for (mw = MainWindows; mw != NULL; mw = mw->next)
   {
      mw-> Background = ClockResources.DefaultBackground;
      mw-> Foreground = ClockResources.DefaultForeground;

      XtSetArg(arg[0], XtNbackground, mw-> Background);
      XtSetValues(mw-> pane, arg, 1);
      XtSetValues(mw-> icon_pane, arg, 1);
   }

} /* end of RegetApplicationResources */
/*
 * GetGCs
 *
 */

static void 
GetGCs(Widget w)
{
   MainWindow *  mw = FindMainWindow(w);
   Arg           arg[2];

   unsigned long XGCVMask;
   XGCValues     XGCValue;

   if (CellsOfScreen(XtScreen(mw-> pane)) == 2)
   {
      mw-> Foreground = BlackPixelOfScreen(XtScreen(mw-> pane));
      mw-> Background = WhitePixelOfScreen(XtScreen(mw-> pane));
   }
   else
   {
      mw-> Foreground = ClockResources.DefaultForeground;
      mw-> Background = ClockResources.DefaultBackground;
   }

   XGCVMask = GCFunction | GCForeground | GCBackground;
   XGCValue.function   = GXcopy;
   XGCValue.foreground = mw-> Foreground;
   XGCValue.background = mw-> Background;
   mw-> gc = 
      XCreateGC(XtDisplay(mw-> pane), XtWindow(mw-> pane), XGCVMask, &XGCValue);

   XtSetArg(arg[0], XtNbackground,   mw-> Background);
   XtSetValues(mw-> pane, arg, 1);
   XtSetValues(mw-> icon_pane, arg, 1);

} /* end of GetGCs */
/*
 * SetHints
 *
 */

extern void SetHints(MainWindow * mw)
{
   Arg arg[10];

   switch(mw-> mode)
   {
      case DIGITAL:
         XtSetArg(arg[0], XtNminWidth,    MIN_DIGITAL_WID);
         XtSetArg(arg[1], XtNminHeight,   MIN_DIGITAL_HT);
         XtSetArg(arg[2], XtNminAspectX,  MIN_DIGITAL_ASP_X);
         XtSetArg(arg[3], XtNminAspectY,  MIN_DIGITAL_ASP_Y);
         XtSetArg(arg[4], XtNmaxAspectX,  MAX_DIGITAL_ASP_X);
         XtSetArg(arg[5], XtNmaxAspectY,  MAX_DIGITAL_ASP_Y);
         XtSetArg(arg[6], XtNwidth,   
            mw-> Width < MIN_DIGITAL_WID ? MIN_DIGITAL_WID : mw-> Width);
         XtSetArg(arg[7], XtNheight,  
            mw-> Height < MIN_DIGITAL_HT ? MIN_DIGITAL_HT  : mw-> Height);
         XtSetValues(mw-> shell, arg, 8);
         break; 
      case ANALOG:
         XtSetArg(arg[0], XtNminWidth,    MIN_ANALOG_WID);
         XtSetArg(arg[1], XtNminHeight,   MIN_ANALOG_HT);
         XtSetArg(arg[2], XtNminAspectX,  MIN_ANALOG_ASP_X);
         XtSetArg(arg[3], XtNminAspectY,  MIN_ANALOG_ASP_Y);
         XtSetArg(arg[4], XtNmaxAspectX,  MAX_ANALOG_ASP_X);
         XtSetArg(arg[5], XtNmaxAspectY,  MAX_ANALOG_ASP_Y);
         XtSetArg(arg[6], XtNwidth,
            mw-> Width < MIN_ANALOG_WID ? MIN_ANALOG_WID : mw-> Width);
         XtSetArg(arg[7], XtNheight,
            mw-> Height < MIN_ANALOG_HT ? MIN_ANALOG_HT  : mw-> Height);
         XtSetValues(mw-> shell, arg, 8);
         break; 
   }

} /* end of SetHints */
/*
 * ResetTimer
 *
 */

extern void
ResetTimer(MainWindow * mw)
{

   if (mw-> timerId)
      XtRemoveTimeOut(mw-> timerId);
   mw-> timerId = 
      XtAddTimeOut(mw-> UpdateTime, HandleClockTick, (XtPointer)mw);

} /* end of ResetTimer */
/*
 * NewMainWindow
 *
 */

static MainWindow * 
NewMainWindow()
{
   
   MainWindow * mw = (MainWindow *)MALLOC(sizeof(MainWindow));

   mw-> next = MainWindows;
   MainWindows = mw;

   return (mw);

} /* end of NewMainWindow */
/*
 * FindToplevel
 *
 */

extern MainWindow *
FindToplevel(Widget shell)
{
   MainWindow * p;

   for (p = MainWindows; p != NULL; p = p-> next)
      if (p-> shell == shell || p-> icon_pane == shell)
         break;

   return (p);

} /* end of FindToplevel */
/*
 * FindMainWindow
 *
 */

extern MainWindow * 
FindMainWindow(Widget w)
{
   Widget       shell;
   MainWindow * p = NULL;

   for (shell = (Widget)_OlGetShellOfWidget(w);
        shell != NULL && (p = FindToplevel(shell)) == NULL;
        shell = (Widget)_OlGetShellOfWidget(XtParent(shell)))
      ; /* loop */

   return (p);

} /* end of FindMainWindow */

/*
 * InitializeClock
 *
 */

static void 
InitializeClock(Widget root)
{

   MainWindow * mw = NewMainWindow();
   Arg          arg[20];
   XIconSize *  icon_size_hint;
   int          count;
   char         geometry[100];

   mw-> sync                 = 15;
   mw-> cnt                  = 0;
   mw-> Foreground           = ClockResources.DefaultForeground;
   mw-> Background           = ClockResources.DefaultBackground;
   mw-> mode                 = ClockSetting.modes.previous_value;
   mw-> chime                = ClockSetting.chime.previous_value;
   mw-> Width                = ClockResources.DefaultWidth;
   mw-> PreviousWidth        = 0;
   mw-> Height               = ClockResources.DefaultHeight;
   mw-> PreviousHeight       = 0;
   mw-> UpdateTime           = 
   mw-> PreviousUpdateTime   = 
      ((int)ClockSetting.ticks.previous_value == TicksMinute) ? 60000 : 1000;
   mw-> timerId              = NULL;
   mw-> propertiesPrompt     = NULL;
   mw-> alarmPrompt          = NULL;
   mw-> Radius               = 0;
   mw-> Padding              = 10;
   mw-> CenterX              = 0;
   mw-> CenterY              = 0;
   mw-> HourHandWidth        = 1;
   mw-> MinuteHandWidth      = 1;
   mw-> SecondHandWidth      = 1;
   mw-> HourHandLength       = 0;
   mw-> MinuteHandLength     = 0;
   mw-> SecondHandLength     = 1;
   mw-> HourY                = 0;
   mw-> MinuteY              = 0;
   mw-> SecondY              = 0;
   mw-> HourX                = 0;
   mw-> MinuteX              = 0;
   mw-> SecondX              = 0;
   mw-> DigitPad             = 0;
   mw-> DigitWidth           = 0;
   mw-> DigitHeight          = 0;
   mw-> DigitVThick          = 0;
   mw-> DigitHThick          = 0;

   mw-> amount               = 0;
   mw-> TimeValue            = 0;
   mw-> mapped               = FALSE;

   mw-> active_pane          = NULL;

   if (XGetIconSizes(DisplayOfScreen(XtScreen(root)), 
      RootWindowOfScreen(XtScreen(root)), &icon_size_hint, &count) && count > 0)
   {

      sprintf(geometry, "%dx%d", 
         icon_size_hint->max_width, icon_size_hint->max_height);
      XFree(icon_size_hint);
   }
   else
      strcpy(geometry, "48x48");

   XtSetArg(arg[0], XtNmappedWhenManaged, False);
   XtSetArg(arg[1], XtNborderWidth,       0);
   XtSetArg(arg[2], XtNtranslations,      XtParseTranslationTable(""));
   XtSetArg(arg[3], XtNgeometry,          geometry);
   mw-> icon_pane =
      XtCreateApplicationShell("_X_", topLevelShellWidgetClass, arg, 4);
   GizmoRegisterHelp(OL_WIDGET_HELP, mw->icon_pane,
         ApplicationHelp.title, OL_DESKTOP_SOURCE, &ApplicationHelp);

   XtAddEventHandler
      (mw-> icon_pane, ExposureMask, True, HandleExpose, NULL);
   XtAddEventHandler
      (mw-> icon_pane, StructureNotifyMask, False, HandleConfigure, (XtPointer)False);

   XtRealizeWidget(mw-> icon_pane);

   XtSetArg(arg[0], XtNiconWindow, XtWindow(mw-> icon_pane));
   mw-> shell = 
      XtCreateApplicationShell("_X_", topLevelShellWidgetClass, arg, 1);
   GizmoRegisterHelp(OL_WIDGET_HELP, mw->shell,
         ApplicationHelp.title, OL_DESKTOP_SOURCE, &ApplicationHelp);

   SetHints(mw);

   XtSetArg(arg[0], XtNwidth,  mw-> Width);
   XtSetArg(arg[1], XtNheight, mw-> Height);
   mw-> pane = 
      XtCreateManagedWidget("_X_", bulletinBoardWidgetClass, mw-> shell, arg, 2);

   XtAddEventHandler
      (mw-> pane, ExposureMask,        True,  HandleExpose,    NULL);
   XtAddEventHandler
      (mw-> pane, StructureNotifyMask, False, HandleConfigure, (XtPointer)True);
   XtAddEventHandler
      (mw-> pane, ButtonPressMask,     False, HandleButton,    NULL);

   mw-> AR =  /* aspect ratio */
     (
        (double)WidthOfScreen(XtScreen(mw-> shell)) 
           / 
        (double)WidthMMOfScreen(XtScreen(mw-> shell)) 
     ) 
        /
     (
        (double)HeightOfScreen(XtScreen(mw-> shell)) 
           / 
        (double)HeightMMOfScreen(XtScreen(mw-> shell)) 
     );

   XtRealizeWidget(mw-> shell);

   GetGCs(mw-> shell);

} /* end of InitializeClock */
/*
 * HandleExpose
 *
 */

static XtEventHandler
HandleExpose(Widget w, XtPointer client_data, XEvent * event)
{
   XExposeEvent *      exp_event      = (XExposeEvent *) event;
   MainWindow *        mw             = FindMainWindow(w);
   Widget              active_pane    = mw->active_pane;

   if (
       ((event-> type == GraphicsExpose  && (exp_event-> count == 0))) ||
       ((event-> type == Expose)         && (exp_event-> count == 0))
      )
   {
      mw-> active_pane = mw-> pane == w ? mw-> pane : mw-> icon_pane;
      if (active_pane != mw-> active_pane)
      {
         Display *           dpy = XtDisplay(w);
         Window              win = XtWindow(w);
         Window              Root;
         int                 XOrigin;
         int                 YOrigin;
         unsigned int        Border;
         unsigned int        depth;
         XEvent              myarea;

         XGetGeometry
            (dpy, win, &Root, &XOrigin, &YOrigin, 
             &mw-> Width, &mw-> Height, &Border, &depth);
         ResetTimer(mw);
      }

      CalculateSizes(w);

      if (mw-> mode == DIGITAL)
         DrawDigitalClock(w, TRUE);
      else
         DrawAnalogClock(w, TRUE);
   }

} /* end of HandleExpose */
/*
 * HandleConfigure
 *
 */

extern XtEventHandler
HandleConfigure(Widget w, XtPointer client_data, XEvent * event)
{
   MainWindow *        mw  = FindMainWindow(w);
   Display *           dpy = XtDisplay(w);
   Window              win = XtWindow(w);
   int                 flg = (int)client_data;
   Window              Root;
   int                 XOrigin;
   int                 YOrigin;
   unsigned int        Border;
   unsigned int        depth;
   XEvent              myarea;

   switch(event-> type)
   {
      case MapNotify:
         mw-> mapped = TRUE;
         /* fall through */
      case ConfigureNotify:
         do 
         {
            XGetGeometry
               (dpy, win, &Root, &XOrigin, &YOrigin, 
                &mw-> Width, &mw-> Height, &Border, &depth);
         } while (XCheckWindowEvent(dpy, win, ExposureMask, &myarea));

         break;
      case UnmapNotify:
         if (flg)
            mw-> mapped = FALSE;
         break;
   }

   if (mw-> mapped == TRUE)
      XClearArea(dpy, win, 0, 0, 0, 0, True);

} /* end of HandleConfigure */
/*
 * HandleButton
 *
 * on receipt of a MENU press create the menu if not already created 
 * then pop it up
 *
 */

static XtEventHandler
HandleButton(Widget w, XtPointer client_data, XEvent * event, Boolean * cont_to_dispatch)
{
   MainWindow *      mw = FindMainWindow(w);
   OlVirtualEventRec ve;

   OlLookupInputEvent(w, event, &ve, OL_DEFAULT_IE);

   if (ve.virtual_name == OL_MENU)
   {
      if (mw-> menu == (Gizmo)NULL)
      {
         mw-> menu = CopyGizmo(MenuGizmoClass, &ClockMenu);
         mw-> menuShell = CreateGizmo(w, MenuGizmoClass, mw-> menu, NULL, 0);
      }

   OlPostPopupMenu(mw-> shell, mw-> menuShell, OL_MENU, NULL, 
      event-> xbutton.x_root, event-> xbutton.y_root, 
      event-> xbutton.x, event-> xbutton.y);
   }
   else
   {
   /*
    * FIX: post the '?' pointer otherwise
    */
   }

} /* end of HandleButton */
/*
 * ClockCB
 *
 */

static void
ClockCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);

   switch(p-> item_index)
   {
      case ClockSetAlarm:
         AlarmCB(w, client_data, call_data);
         break;
      case ClockProperties:
         PropertyCB(w, client_data, call_data);
         break;
      default:
         (void)fprintf(stderr,"default in ClockCB taken!!!\n");
   }

} /* end of ClockCB */
/*
 * HelpCB
 *
 */

static void
HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);

   switch(p-> item_index)
   {
      case HelpClock:
         PostGizmoHelp(mw-> shell, &ApplicationHelp);
         break;
      case HelpTOC:
         PostGizmoHelp(mw->shell, &TOCHelp);
         break;
      case HelpHelpDesk:
         PostGizmoHelp(mw->shell, &HelpDeskHelp);
         break;
      default:
         (void)fprintf(stderr,"default in ClockCB taken!!!\n");
   }

} /* end of HelpCB */
/*
 * HandleClockTick
 *
 */

extern void
HandleClockTick(XtPointer client_data, XtIntervalId * id)
{
   MainWindow * mw = (MainWindow *)client_data;
   Widget       w  = mw->active_pane;

   mw-> tm.tm_sec += (mw-> UpdateTime / 1000);

   if (mw-> mode == DIGITAL)
      DrawDigitalClock(w, FALSE);
   else
      DrawAnalogClock(w, FALSE);

   mw-> timerId = XtAddTimeOut(mw-> UpdateTime, HandleClockTick, client_data);

} /* end of HandleClockTick */
/*
 * SetupClockChime
 *
 */

static void 
SetupClockChime(Widget w, int hour, int half)
{
   MainWindow * mw = FindMainWindow(w);
   register amount;

   switch (mw-> chime)
   {
      case ChimeShipBells:
         switch(hour % 12)
         {
            case 1: case 5: case  9:   amount = 2;
                                       break;
            case 2: case 6: case 10:   amount = 4;
                                       break;
            case 3: case 7: case 11:   amount = 6;
                                       break;
            case 0: case 4: case  8:   amount = 8;
                                       break;
         }
         if (half)
            if (amount == 8)
               amount = 1;
            else
               amount++;
         break;
      case ChimeTraditional:
         if (half)
            amount = 1;
         else
            if (hour == 0)
               amount = 12;
            else
               amount = hour % 12;
         break;
      case ChimeNone:
      default:
         amount = 0;
   }

   mw-> amount = amount;

   if (amount > 0)
      XtAddTimeOut(1000, HandleClockChime, (caddr_t) w);

} /* end of SetupClockChime */
/*
 * HandleClockChime
 *
 */

static void 
HandleClockChime(XtPointer client_data, XtIntervalId * id)
{
   Widget       w      = (Widget)client_data;
   MainWindow * mw     = FindMainWindow(w);
   register int amount = mw-> amount;

   XBell(XtDisplay(mw-> shell), ClockResources.beepVolume);

   if (--amount > 0 && mw-> chime == ChimeShipBells)
   {
      XBell(XtDisplay(mw-> shell), ClockResources.beepVolume);
      amount--;
   }

   mw-> amount = amount;

   if (mw-> amount > 0)
      XtAddTimeOut(1000, HandleClockChime, client_data);
   
} /* end of HandleClockChime */
/*
 * CalculateSizes
 *
 */

static void 
CalculateSizes(Widget w)
{
   MainWindow * mw = FindMainWindow(w);
   double       Factor;
   int          IFactor;
   int          minute_tick = (mw-> UpdateTime != 1000);

   if (mw-> PreviousWidth      != mw-> Width || 
       mw-> PreviousHeight     != mw-> Height ||
       mw-> PreviousUpdateTime != mw-> UpdateTime)
   {
      mw-> PreviousWidth      = mw-> Width;
      mw-> PreviousHeight     = mw-> Height;
      mw-> PreviousUpdateTime = mw-> UpdateTime;

      IFactor = minute_tick ?
                                (mw-> Width * 1000) / 22 : /* p#p#:#p#p     */
                                (mw-> Width * 1000) / 33;  /* p#p#:#p#:#p#p */

      mw-> DigitWidth         = (IFactor * 4)  / 1000;
      mw-> HourX              = (IFactor)      / 1000;
      mw-> MinuteX            = (IFactor * 12) / 1000;
      mw-> SecondX            = (IFactor * 23) / 1000;

      mw-> DigitPad           = minute_tick ?
                  (mw-> Width - 5 * mw-> DigitWidth) / 4 : /* p#p#:#p#p     */
                  (mw-> Width - 7 * mw-> DigitWidth) / 5;  /* p#p#:#p#:#p#p */

      mw-> DigitHeight        = min(mw-> Height - 6, mw-> Width / 4);
      mw-> DigitVThick        = 
      mw-> DigitHThick        = max(mw-> DigitHeight, mw-> DigitWidth) / 15;
      if (mw->DigitVThick < 1)
      {
         mw-> DigitVThick        = 
         mw-> DigitHThick        = 1;
      }

      mw-> HourY              = 
      mw-> MinuteY            = 
      mw-> SecondY            = (mw-> Height - mw-> DigitHeight) / 2;

      mw-> Radius             = (min(mw-> Width / mw-> AR, mw-> Height) - 
                                (2 * mw-> Padding)) / 2;
      Factor                  = (double) mw-> Radius / 100.0;
      mw-> HourHandLength     = Factor * HR_HAND_LENGTH_FACTOR;
      mw-> MinuteHandLength   = Factor * MIN_HAND_LENGTH_FACTOR;
      mw-> SecondHandLength   = Factor * SEC_HAND_LENGTH_FACTOR;
      mw-> HourHandWidth      = Factor * HR_HAND_WIDTH_FACTOR;
      mw-> MinuteHandWidth    = Factor * MIN_HAND_WIDTH_FACTOR;
      mw-> SecondHandWidth    = Factor * SEC_HAND_WIDTH_FACTOR;
      mw-> CenterX            = mw-> Width  / 2;
      mw-> CenterY            = mw-> Height / 2;

      RefreshTitle(mw);
   }

} /* end of CalculateSizes */
/*
 * DrawClockFace
 * 
 *  Draw the clock face (every fifth mark is longer than the others).
 *
 */

static void 
DrawClockFace(Widget w)
{
   MainWindow * mw    = FindMainWindow(w);
   register int Delta = (mw-> Radius - mw-> SecondHandLength) / 3;
   XSegment     segments[120];
   register int i;

   XClearWindow(XtDisplay(w), XtWindow(w));

   for (i = 0; i < 60; i++)
   {
      if ((i % 5) == 0)
      {
         segments[i].x1 = mw-> CenterX + mw-> SecondHandLength * sin[i] * mw-> AR;
         segments[i].y1 = mw-> CenterY - mw-> SecondHandLength * cos[i];
      }
      else
      {
         segments[i].x1 = mw-> CenterX + (mw-> Radius - Delta) * sin[i] * mw-> AR;
         segments[i].y1 = mw-> CenterY - (mw-> Radius - Delta) * cos[i];
      }
      segments[i].x2 = mw-> CenterX + mw-> Radius * sin[i] * mw-> AR;
      segments[i].y2 = mw-> CenterY - mw-> Radius * cos[i];
   }

   XSetForeground(XtDisplay(w), mw-> gc, mw-> Foreground);
   XDrawSegments(XtDisplay(w), XtWindow(w), mw-> gc, segments, 60);

} /* end of DrawClockFace */
/*
 * DrawAnalogClock
 *
 */

static void 
DrawAnalogClock(Widget w, int exposed)
{
   MainWindow *  mw         = FindMainWindow(w);

   XPoint *      HourHand   = mw-> HourHand;
   XPoint *      MinuteHand = mw-> MinuteHand;
   XPoint *      SecondHand = mw-> SecondHand;

   Display *     dpy        = XtDisplay(w);
   Window        win        = XtWindow(w);

   int           i;
   int           sinXwidth;
   int           cosXwidth;
   GC            gc  = mw-> gc;
   int           increment;

   Tick(w);
   if (mw-> tm.tm_hour >= 12)
      increment = 12;
   else
      increment = 0;

   mw->tm.tm_hour -= increment;

   if (exposed)
   {
      DrawClockFace(w);
      mw-> onscreen = FALSE;
   }

   XSetForeground(dpy, gc, mw-> Background);
   if (!mw-> onscreen || 
       (mw-> tm.tm_min % 12 == 0) || 
       (mw-> tm.tm_hour != mw-> otm.tm_hour))
   {
      if (mw-> onscreen)
      {
         XFillPolygon(dpy, win, gc, 
                      HourHand, 5, Convex, CoordModeOrigin);
         XDrawLines(dpy, win, gc,
                    HourHand, 5, CoordModeOrigin);
      }
      i = mw-> tm.tm_hour * 5 + mw-> tm.tm_min / 12;
      sinXwidth = mw-> HourHandWidth * sin[i];
      cosXwidth = mw-> HourHandWidth * cos[i];
      HourHand[0].x = 
      HourHand[4].x = mw-> CenterX - (sinXwidth + cosXwidth) * mw-> AR;
      HourHand[0].y = HourHand[4].y = mw-> CenterY + (cosXwidth - sinXwidth);
      HourHand[1].x = mw-> CenterX + mw-> HourHandLength * sin[i] * mw-> AR;
      HourHand[1].y = mw-> CenterY - mw-> HourHandLength * cos[i];
      HourHand[2].x = mw-> CenterX - (sinXwidth - cosXwidth) * mw-> AR;
      HourHand[2].y = mw-> CenterY + (cosXwidth + sinXwidth);
      HourHand[3].x = mw-> CenterX - mw-> HourHandLength * sin[i] * mw-> AR / HR_TAIL_FACTOR;
      HourHand[3].y = mw-> CenterY + mw-> HourHandLength * cos[i] / HR_TAIL_FACTOR;
   }
   if (!mw-> onscreen || mw-> tm.tm_min != mw-> otm.tm_min)
   {
      if ((mw-> tm.tm_min != mw-> otm.tm_min) && 
          (mw-> tm.tm_min == 0 || mw-> tm.tm_min == 30))
         SetupClockChime(w, mw-> tm.tm_hour, (mw-> tm.tm_min == 30));
      if (mw-> onscreen)
      {
         XFillPolygon(dpy, win, gc, MinuteHand, 5, Convex, CoordModeOrigin);
         XDrawLines(dpy, win, gc, MinuteHand, 5, CoordModeOrigin);
      }
      i = mw-> tm.tm_min;
      sinXwidth = mw-> MinuteHandWidth * sin[i];
      cosXwidth = mw-> MinuteHandWidth * cos[i];
      MinuteHand[0].x = 
      MinuteHand[4].x = mw-> CenterX - (sinXwidth + cosXwidth) * mw-> AR;
      MinuteHand[0].y = 
      MinuteHand[4].y = mw-> CenterY + (cosXwidth - sinXwidth);
      MinuteHand[1].x = mw-> CenterX + mw-> MinuteHandLength * sin[i] * mw-> AR;
      MinuteHand[1].y = mw-> CenterY - mw-> MinuteHandLength * cos[i];
      MinuteHand[2].x = mw-> CenterX - (sinXwidth - cosXwidth) * mw-> AR;
      MinuteHand[2].y = mw-> CenterY + (cosXwidth + sinXwidth);
      MinuteHand[3].x = mw-> CenterX - 
         mw-> MinuteHandLength * sin[i] * mw-> AR / MIN_TAIL_FACTOR;
      MinuteHand[3].y = mw-> CenterY + 
         mw-> MinuteHandLength * cos[i] / MIN_TAIL_FACTOR;
   }
   if ((mw-> UpdateTime == 1000) && 
       (!mw-> onscreen || mw-> tm.tm_sec != mw-> otm.tm_sec))
   {
      if (mw-> onscreen)
         XDrawLine(dpy, win, gc,
                   SecondHand[0].x, SecondHand[0].y,
                   SecondHand[1].x, SecondHand[1].y);
      i = mw-> tm.tm_sec;
      SecondHand[0].x = mw-> CenterX - 
         mw-> SecondHandLength * sin[i] * mw-> AR / SEC_TAIL_FACTOR;
      SecondHand[0].y = mw-> CenterY + 
         mw-> SecondHandLength * cos[i] / SEC_TAIL_FACTOR;
      SecondHand[1].x = mw-> CenterX + mw-> SecondHandLength * sin[i] * mw-> AR;
      SecondHand[1].y = mw-> CenterY - mw-> SecondHandLength * cos[i];
   }

   XSetForeground(dpy, gc, mw-> Foreground);
   XFillPolygon(dpy, win, gc, HourHand, 5, Convex, CoordModeOrigin);

   XSetForeground(dpy, gc, mw-> Foreground);
   XDrawLines(dpy, win, gc, HourHand, 5, CoordModeOrigin);

   XSetForeground(dpy, gc, mw-> Foreground);
   XFillPolygon(dpy, win, gc, MinuteHand, 5, Convex, CoordModeOrigin);

   XSetForeground(dpy, gc, mw-> Foreground);
   XDrawLines(dpy, win, gc, MinuteHand, 5, CoordModeOrigin);

   if (mw-> UpdateTime == 1000)
      XDrawLine(dpy, win, gc,
                SecondHand[0].x, SecondHand[0].y,
                SecondHand[1].x, SecondHand[1].y);

   mw-> onscreen = TRUE;

   mw-> otm = mw-> tm;

   mw-> tm.tm_hour += increment;

} /* end of DrawAnalogClock */
/*
 * DrawDigitalClock
 *
 */

static void 
DrawDigitalClock(Widget w, int exposed)
{
   MainWindow * mw  = FindMainWindow(w);
   Display *    dpy = XtDisplay(w);
   Window       win = XtWindow(w);
   GC           gc  = mw-> gc;
   int          first_colon_x;
   int          second_colon_x;

   Tick(w);

   if (exposed)
   {

      first_colon_x  = mw-> MinuteX - mw-> DigitPad;
      second_colon_x = mw-> SecondX - mw-> DigitPad;
      first_colon_x  = mw-> HourX + 2 * mw-> DigitWidth + 2 * mw-> DigitPad;
      second_colon_x = mw-> MinuteX + 2 * mw-> DigitWidth + 2 * mw-> DigitPad;

      XClearWindow(dpy, win);
      XSetForeground(dpy, gc, mw-> Foreground);
      XFillRectangle(dpy, win, gc, first_colon_x,
         mw-> HourY + mw-> DigitHeight / 3, 
         mw-> DigitVThick, mw-> DigitHThick);
      XFillRectangle(dpy, win, gc, first_colon_x,
         mw-> HourY + (mw-> DigitHeight * 2) / 3, 
         mw-> DigitVThick, mw-> DigitHThick);
      if (mw-> UpdateTime == 1000)
      {
         XFillRectangle(dpy, win, gc, second_colon_x,
            mw-> MinuteY + mw-> DigitHeight / 3, 
            mw-> DigitVThick, mw-> DigitHThick);
         XFillRectangle(dpy, win, gc, second_colon_x,
            mw-> MinuteY + (mw-> DigitHeight * 2) / 3, 
            mw-> DigitVThick, mw-> DigitHThick);
      }
      mw-> onscreen = FALSE;
   }

   if (!mw-> onscreen || mw-> tm.tm_hour != mw-> otm.tm_hour)
   {
      if (mw-> onscreen)
         XClearArea(dpy, win, 
            mw-> HourX, mw-> HourY, 
            mw-> DigitWidth * 2 + mw-> DigitPad, mw-> DigitHeight, False);
   }

   if (!mw-> onscreen || mw-> tm.tm_min != mw-> otm.tm_min)
   {
      if (mw-> onscreen)
         XClearArea(dpy, win, 
            mw-> MinuteX, mw-> MinuteY, 
            mw-> DigitWidth * 2 + mw-> DigitPad, mw-> DigitHeight, False);
   }

   if ((mw-> tm.tm_min != mw-> otm.tm_min) && 
       (mw-> tm.tm_min == 0 || mw-> tm.tm_min == 30))
      SetupClockChime(w, mw-> tm.tm_hour, (mw-> tm.tm_min == 30));

   if ((mw-> UpdateTime == 1000) && 
       (!mw-> onscreen || mw-> tm.tm_sec != mw-> otm.tm_sec))
   {
      if (mw-> onscreen)
         if (mw-> tm.tm_sec / 10 != mw-> otm.tm_sec / 10)
            XClearArea(dpy, win, 
               mw-> SecondX, mw-> SecondY, 
               mw-> DigitWidth * 2 + mw-> DigitPad, mw-> DigitHeight, False);
         else
            XClearArea(dpy, win, 
               mw-> SecondX + mw-> DigitWidth + mw-> DigitPad, mw-> SecondY, 
               mw-> DigitWidth, mw-> DigitHeight, False);
   }

   if (mw-> tm.tm_hour == 0)
   {
      DrawDigit(w, mw-> HourX, mw-> HourY, 1);
      DrawDigit(w, mw-> HourX + mw-> DigitWidth + mw-> DigitPad, mw-> HourY, 2);
   }
   else
   {
      if (mw-> tm.tm_hour / 10)
         DrawDigit(w, mw-> HourX, mw-> HourY, mw-> tm.tm_hour / 10);
      DrawDigit(w, mw-> HourX + mw-> DigitWidth + mw-> DigitPad, 
                mw-> HourY, mw-> tm.tm_hour % 10);
   }
   DrawDigit(w, mw-> MinuteX, mw-> MinuteY, mw-> tm.tm_min / 10);
   DrawDigit(w, mw-> MinuteX + mw-> DigitWidth + mw-> DigitPad, 
             mw-> MinuteY, mw-> tm.tm_min % 10);
   if (mw-> UpdateTime == 1000)
   {
      DrawDigit(w, mw-> SecondX, mw-> SecondY, mw-> tm.tm_sec / 10);
      DrawDigit(w, mw-> SecondX + mw-> DigitWidth + mw-> DigitPad, 
                mw-> SecondY, mw-> tm.tm_sec % 10);
   }

   mw-> onscreen = TRUE;

   mw-> otm = mw-> tm;

} /* end of DrawDigitalClock */
/*
 * DrawDigit
 *
 */

static void 
DrawDigit(Widget w, int x, int y, int digit)
{
   register int i;

   static char segments[10][9] =
   {
  /* 0  1  2  3  4  5  6  7  8 */
   { 1, 1, 1, 1, 1, 1, 0, 0, 0 }, /* 0 */
   { 0, 0, 0, 0, 0, 0, 0, 1, 1 }, /* 1 */
   { 1, 0, 1, 1, 0, 1, 1, 0, 0 }, /* 2 */
   { 1, 1, 1, 0, 0, 1, 1, 0, 0 }, /* 3 */
   { 1, 1, 0, 0, 1, 0, 1, 0, 0 }, /* 4 */
   { 0, 1, 1, 0, 1, 1, 1, 0, 0 }, /* 5 */
   { 0, 1, 1, 1, 1, 1, 1, 0, 0 }, /* 6 */
   { 1, 1, 0, 0, 0, 1, 0, 0, 0 }, /* 7 */
   { 1, 1, 1, 1, 1, 1, 1, 0, 0 }, /* 8 */
   { 1, 1, 1, 0, 1, 1, 1, 0, 0 }, /* 9 */
   };

   for (i = 0; i < 9; i++)
      if (segments[digit][i])
         DrawSegment(w, x, y, i);

} /* end of DrawDigit */
/*
 * DrawSegment
 *
 */

static void 
DrawSegment(Widget W, int x, int y, int segment)
{
   MainWindow * mw = FindMainWindow(W);
   int x1, y1, w, h;

   int HT = (mw-> DigitHeight - (mw-> DigitHThick * 3)) / 2;
   int WD = (mw-> DigitWidth  - (mw-> DigitVThick * 2));

   int ht = (mw-> DigitHThick);
   int wd = (mw-> DigitVThick);

   /*
    *       5
    *     -----
    *    |     |
    *  4 |  7  | 0
    *    |     |
    *     --6--
    *    |     |
    *  3 |  8  | 1
    *    |     |
    *     -----
    *       2
    */

   switch (segment)
   {
      case 0:   x1 = x + wd + WD;
                y1 = y + ht;
                w  = wd;
                h  = HT;
                break;
      case 1:   x1 = x + wd + WD;
                y1 = y + ht + HT + ht;
                w  = wd;
                h  = HT;
                break;
      case 2:   x1 = x + wd;
                y1 = y + ht + HT + ht + HT;
                w  = WD;
                h  = ht;
                break;
      case 3:   x1 = x;
                y1 = y + ht + HT + ht;
                w  = wd;
                h  = HT;
                break;
      case 4:   x1 = x;
                y1 = y + ht;
                w  = wd;
                h  = HT;
                break;
      case 5:   x1 = x + wd;
                y1 = y;
                w  = WD;
                h  = ht;
                break;
      case 6:   x1 = x + wd;
                y1 = y + ht + HT;
                w  = WD;
                h  = ht;
                break;
      case 7:   x1 = x + WD / 2;
                y1 = y + ht;
                w  = wd;
                h  = HT;
                break;
      case 8:   x1 = x + WD / 2;
                y1 = y + ht + HT + ht;
                w  = wd;
                h  = HT + ht;
                break;
   }

   w = w < 1 ? 1 : w;
   h = h < 1 ? 1 : h;
   XFillRectangle(XtDisplay(W), XtWindow(W), mw-> gc, x1, y1, w, h);

} /* end of DrawSegment */
/*
 * Tick
 *
 */

static void 
Tick(Widget w)
{
   MainWindow * mw = FindMainWindow(w);

/*
 * FIX: is this complexity worth it?
 */

   if (!mw-> onscreen || 
       (mw-> tm.tm_sec > 59) || 
       ((mw-> tm.tm_sec % mw-> sync) == 0))
   {
      int guess = mw-> otm.tm_min * 60 + mw-> tm.tm_sec;

      time(&mw-> TimeValue);
      mw-> tm = *localtime(&mw-> TimeValue);

      if ((mw-> tm.tm_min * 60 + mw-> tm.tm_sec) == guess)
         if (mw-> cnt == 0)
         {
            mw-> sync = (mw-> sync == 60) ? mw-> sync : mw-> sync + 1;
            mw-> cnt = 5;
         }
         else
            mw-> cnt--;
      else
      {
         mw-> sync = (mw-> sync ==  1) ? mw-> sync : mw-> sync - 1;
         mw-> cnt = 0;
      }
   }

   if (mw-> tm.tm_yday != mw-> otm.tm_yday)
      RefreshTitle(mw);

/*
   if (mw-> tm.tm_hour >= 12)
      mw-> tm.tm_hour -= 12;
*/

} /* end of Tick */
/*
 * RefreshTitle
 *
 */

static void
RefreshTitle(MainWindow * mw)
{
   Arg arg[5];

   if (mw-> Width > 250)
      sprintf(mw-> TimeString, 
         "%s %d %s", 
         Weekday[mw-> tm.tm_wday], mw-> tm.tm_mday, Month[mw-> tm.tm_mon]);
   else
      sprintf(mw-> TimeString, 
         "%s %d %s", 
         weekday[mw-> tm.tm_wday], mw-> tm.tm_mday, month[mw-> tm.tm_mon]);

   XtSetArg(arg[0], XtNtitle, mw->TimeString);
   XtSetArg(arg[1], XtNiconName, mw->TimeString);
   XtSetValues(mw->shell, arg, 2);

} /* end of RefreshTitle */
