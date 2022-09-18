/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:clock.h	1.11"
#endif

/*
 * clock.h
 *
 */

#ifndef _clock_h
#define _clock_h

typedef enum { ANALOG, DIGITAL } ModeSetting;
typedef enum { SILENT, TRADITIONAL, SHIPBELLS } ChimeSetting;

typedef struct _MainWindow
   {
   struct _MainWindow * next;
   Widget               shell;
   Widget               pane;
   Widget               icon_shell;
   Widget               icon_pane;
   Widget               active_pane;
   XtIntervalId         timerId;
   Pixel                Foreground;
   Pixel                Background;
   GC                   gc;
   int                  amount;
   unsigned int         Width;
   unsigned int         Height;
   Time                 UpdateTime;

   unsigned int         PreviousWidth;
   unsigned int         PreviousHeight;
   Time                 PreviousUpdateTime;

   XPoint               HourHand[5];
   XPoint               MinuteHand[5];
   XPoint               SecondHand[2];

   int                  sync;
   int                  cnt;

   char                 TimeString[200];
   long                 TimeValue;
   struct tm            tm;
   struct tm            otm;
   char                 mapped;
   char                 onscreen;

   double               AR;
   unsigned int         Radius;
   unsigned int         Padding;
   unsigned int         CenterX;
   unsigned int         CenterY;
   unsigned int         HourHandWidth;
   unsigned int         MinuteHandWidth;
   unsigned int         SecondHandWidth;    /* remove ??? */
   unsigned int         HourHandLength;
   unsigned int         MinuteHandLength;
   unsigned int         SecondHandLength;
   unsigned int         HourY;
   unsigned int         MinuteY;
   unsigned int         SecondY;
   unsigned int         HourX;
   unsigned int         MinuteX;
   unsigned int         SecondX;
   unsigned int         DigitPad;
   unsigned int         DigitWidth;
   unsigned int         DigitHeight;
   unsigned int         DigitVThick;
   unsigned int         DigitHThick;

   ModeSetting          mode;
   ChimeSetting         chime;
   Gizmo                menu;
   Widget               menuShell;
   PopupGizmo *         propertiesPrompt;
   PopupGizmo *         alarmPrompt;
   } MainWindow;

typedef struct _applicationResources
   {
   Boolean      warnings;
   int          beepVolume;
   ModeSetting  DefaultMode;                                                       ChimeSetting DefaultChime;
   int          DefaultWidth;
   int          DefaultHeight;
   Pixel        DefaultForeground;
   Pixel        DefaultBackground;
   } ApplicationResources;

typedef struct _ClockSettings
   {
   Setting chime;
   Setting modes;
   Setting ticks;
   } ClockSettings;

#define HELPPATH             "dtclock" "/" "clock.hlp"
#define FormalClientName     "dtclock:1" FS "Clock"

#define TXT_PROP_TITLE       "dtclock:2"  FS "Clock: Properties"
#define TXT_ALARM_TITLE      "dtclock:3"  FS "Clock: Set Alarm"

#define TXT_NONE             "dtclock:10" FS "None"
#define TXT_TRADITIONAL      "dtclock:11" FS "Traditional"
#define TXT_SHIPSBELLS       "dtclock:12" FS "Ship's Bells"

#define TXT_ANALOG           "dtclock:20" FS "Analog"
#define TXT_DIGITAL          "dtclock:21" FS "Digitial"

#define TXT_SECOND           "dtclock:30" FS "Second"
#define TXT_MINUTE           "dtclock:31" FS "Minute"

#define TXT_APPLY            "dtclock:40" FS "Apply"
#define TXT_SET_DEFAULTS     "dtclock:41" FS "Set Defaults"
#define TXT_RESET            "dtclock:42" FS "Reset"
#define TXT_RESET_TO_FACTORY "dtclock:43" FS "Reset to Factory"
#define TXT_CANCEL           "dtclock:44" FS "Cancel"
#define TXT_PROP_HELP        "dtclock:45" FS "Help"

#define TXT_SET_ALARM        "dtclock:50" FS "Set Alarm..."
#define TXT_PROPERTIES       "dtclock:51" FS "Properties..."
#define TXT_HELP             "dtclock:52" FS "Help"

#define TXT_COMMAND_SET      "dtclock:60" FS "Set Alarm"
#define TXT_COMMAND_RESET    "dtclock:61" FS "Reset Alarm"
#define TXT_COMMAND_CANCEL   "dtclock:62" FS "Cancel"
#define TXT_COMMAND_HELP     "dtclock:63" FS "Help..."

#define TXT_DEFAULT_MESSAGE  "dtclock:70" FS "default alarm message:"
#define TXT_MESSAGE_PROMPT   "dtclock:71" FS "Message:"
#define TXT_HOUR_PROMPT      "dtclock:72" FS "Hour:"
#define TXT_MINUTE_PROMPT    "dtclock:73" FS "Minute:"
#define TXT_AM               "dtclock:74" FS "AM"
#define TXT_PM               "dtclock:75" FS "PM"
#define TXT_AMPM             "dtclock:76" FS "AM/PM:"
#define TXT_TIME_PROMPT      "dtclock:77" FS "Time:"
#define TXT_TIME             "0:00"

#define TXT_JAN              "dtclock:80" FS "Jan"
#define TXT_FEB              "dtclock:81" FS "Feb"
#define TXT_MAR              "dtclock:82" FS "Mar"
#define TXT_APR              "dtclock:83" FS "Apr"
#define TXT_MA               "dtclock:84" FS "May"
#define TXT_JUN              "dtclock:85" FS "Jun"
#define TXT_JUL              "dtclock:86" FS "Jul"
#define TXT_AUG              "dtclock:87" FS "Aug"
#define TXT_SEP              "dtclock:88" FS "Sep"
#define TXT_OCT              "dtclock:89" FS "Oct"
#define TXT_NOV              "dtclock:90" FS "Nov"
#define TXT_DEC              "dtclock:91" FS "Dec"

#define TXT_JANUARY          "dtclock:100" FS "January"
#define TXT_FEBRUARY         "dtclock:101" FS "February"
#define TXT_MARCH            "dtclock:102" FS "March"
#define TXT_APRIL            "dtclock:103" FS "April"
#define TXT_MAY              "dtclock:104" FS "May"
#define TXT_JUNE             "dtclock:105" FS "June"
#define TXT_JULY             "dtclock:106" FS "July"
#define TXT_AUGUST           "dtclock:107" FS "August"
#define TXT_SEPTEMBER        "dtclock:108" FS "September"
#define TXT_OCTOBER          "dtclock:109" FS "October"
#define TXT_NOVEMBER         "dtclock:110" FS "November"
#define TXT_DECEMBER         "dtclock:111" FS "December"

#define TXT_SUNDAY           "dtclock:120" FS "Sunday"
#define TXT_MONDAY           "dtclock:121" FS "Monday"
#define TXT_TUESDAY          "dtclock:122" FS "Tuesday"
#define TXT_WEDNESDAY        "dtclock:123" FS "Wednesday"
#define TXT_THURSDAY         "dtclock:124" FS "Thursday"
#define TXT_FRIDAY           "dtclock:125" FS "Friday"
#define TXT_SATURDAY         "dtclock:126" FS "Saturday"

#define TXT_SUN              "dtclock:130" FS "Sun"
#define TXT_MON              "dtclock:131" FS "Mon"
#define TXT_TUE              "dtclock:132" FS "Tue"
#define TXT_WED              "dtclock:133" FS "Wed"
#define TXT_THU              "dtclock:134" FS "Thu"
#define TXT_FRI              "dtclock:135" FS "Fri"
#define TXT_SAT              "dtclock:136" FS "Sat"

#define TXT_CHIMES           "dtclock:140" FS "Chime:"
#define TXT_MODE             "dtclock:141" FS "Mode:"
#define TXT_TICK             "dtclock:142" FS "Tick:"

#define TXT_PROP_MENU_HELP   "dtclock:150" FS "Applying Properties"

#define TXT_APP_HELP         "dtclock:160" FS "Clock Help..."
#define TXT_TOC_HELP         "dtclock:161" FS "Table of Contents..."
#define TXT_HELPDESK         "dtclock:162" FS "Help Desk..."

#define TXT_TOC_HELP_TITLE   "dtclock:164" FS "Table of Contents"
#define TXT_HELPDESK_TITLE   "dtclock:165" FS "Help Desk"
#define TXT_HELPDESK_SECT    "HelpDesk"

#define TXT_MAIN_HELP_TITLE  "dtclock:170" FS "Application"
#define TXT_MAIN_HELP_SECT   "10"
#define TXT_ALARM_HELP_TITLE "dtclock:172" FS "Alarm"
#define TXT_ALARM_HELP_SECT  "30"
#define TXT_PROP_HELP_TITLE  "dtclock:174" FS "Properties"
#define TXT_PROP_HELP_SECT   "40"

#define MNE_NONE             "dtclock:210" FS "N"
#define MNE_TRADITIONAL      "dtclock:211" FS "T"
#define MNE_SHIPSBELLS       "dtclock:212" FS "S"

#define MNE_ANALOG           "dtclock:220" FS "A"
#define MNE_DIGITAL          "dtclock:221" FS "D"

#define MNE_SECOND           "dtclock:230" FS "S"
#define MNE_MINUTE           "dtclock:231" FS "M"

#define MNE_AM               "dtclock:274" FS "A"
#define MNE_PM               "dtclock:275" FS "P"

#define MNE_APPLY            "dtclock:240" FS "A"
#define MNE_SET_DEFAULTS     "dtclock:241" FS "S"
#define MNE_RESET            "dtclock:242" FS "R"
#define MNE_RESET_TO_FACTORY "dtclock:243" FS "F"
#define MNE_CANCEL           "dtclock:244" FS "C"
#define MNE_PROP_HELP        "dtclock:245" FS "H"

#define MNE_SET_ALARM        "dtclock:250" FS "S"
#define MNE_PROPERTIES       "dtclock:251" FS "P"
#define MNE_HELP             "dtclock:252" FS "H"

#define MNE_COMMAND_SET      "dtclock:260" FS "S"
#define MNE_COMMAND_RESET    "dtclock:261" FS "R"
#define MNE_COMMAND_CANCEL   "dtclock:262" FS "C"
#define MNE_COMMAND_HELP     "dtclock:263" FS "H"

#define MNE_APP_HELP         "dtclock:270" FS "C"
#define MNE_TOC_HELP         "dtclock:271" FS "T"
#define MNE_HELPDESK         "dtclock:272" FS "H"

extern MainWindow * FindMainWindow();
extern void         SetHints();
extern void         ResetTimer();

extern HelpInfo     PropWinHelp;
extern HelpInfo     SetAlarmHelp;
extern HelpInfo     MainClockHelp;

#endif /* _clock_h */
