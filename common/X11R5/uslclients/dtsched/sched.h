/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtsched:sched.h	1.10"
#endif

/*
 * sched.h
 *
 */

#ifndef _sched_h
#define _sched_h

#ifdef USE_SENSITIVITY
#define HIDE                 DesensitizeGizmo
#define UNHIDE               SensitizeGizmo
#else
#define HIDE                 HideGizmo
#define UNHIDE               UnhideGizmo
#endif

#define RETRIEVE_COMMAND     "crontab -l"
#define REPLACE_COMMAND      "crontab"
#define PRINT_COMMAND        "/usr/X/bin/PtrMgr -p %DEFAULT_PRINTER"
#define ITEMS_VISIBLE        5

#define BASENAME_FLD         fields[0]
#define TIME_FLD             fields[1]
#define DAY_DATE_FLD         fields[2]

#define MINUTE_FLD           fields[3]
#define HOUR_FLD             fields[4]
#define DAY_OF_MONTH_FLD     fields[5]
#define DAY_OF_WEEK_FLD      fields[6]   /* 0=Sunday */
#define MONTH_OF_YEAR_FLD    fields[7]
#define COMMAND_FLD          fields[8]
#define POPUP_FLD            fields[9]
#define NUM_FLDS             10
typedef struct _Cronentry
   {
   XtArgVal set;
   char **  fields;     /* will be NUM_FLDS fields */
   XtArgVal clientData;
   } Cronentry;

typedef Bufferof(Cronentry) Crontab;

typedef Buffer Cronline;

typedef struct _InputWindow
   {
   PopupGizmo *      popupGizmo;
   InputGizmo *      task;
   TimeGizmo *       time;
   ChoiceGizmo *     ignore;
   ChoiceGizmo *     when;
   ChoiceGizmo *     weekday;
   ChoiceGizmo *     month;
#ifdef USE_NUMERIC_DATE
   NumericGizmo *    date;
#else
   ChoiceGizmo *     date;
#endif
   Boolean           dirty;
   struct _InputWindow * next;
   } InputWindow;

typedef struct _MainWindow
   {
   BaseWindowGizmo * baseWindow;
   ListGizmo *       listGizmo;
   ListHead *        listHead;
   Crontab *         crontab;
   Crontab *         oldcrontab;
   ModalGizmo *      exitNotice;
   PopupGizmo *      popupGizmo;
   InputGizmo *      task;
   TimeGizmo *       time;
   ChoiceGizmo *     ignore;
   ChoiceGizmo *     when;
   ChoiceGizmo *     weekday;
   ChoiceGizmo *     month;
#ifdef USE_NUMERIC_DATE
   NumericGizmo *    date;
#else
   ChoiceGizmo *     date;
#endif
   Boolean           dirty;
   Boolean           withdrawn;
   InputWindow *     iw;
   } MainWindow;

#define HELPPATH             "dtsched" "/" "sched.hlp"
#define ICONPATH             "dtsched.48"

#define DROP_RESOURCE        "dtsched"
/*
 * for resources (not used)
 */
#define IGNORE               "ignore"
#define WHEN                 "when"
#define WEEKDAY              "weekday"
#define MONTH                "month"
#define DATE                 "date"

#define ClientName           "dtsched"
#define ClientClass          "dtsched"

#define FormalClientName     "dtsched:1"   FS "Task Scheduler"

#define TXT_CLIENT_NAME      "dtsched:2"   FS "Task Scheduler: "
#define TXT_ICON_NAME        "dtsched:3"   FS "Task Scheduler"

#define TXT_CLEAR            "dtsched:9"   FS "Clear"

#define TXT_NEW              "dtsched:10"  FS "New..."
#define TXT_OPEN             "dtsched:11"  FS "Open..."
#define TXT_SAVE             "dtsched:12"  FS "Save"
#define TXT_SAVE_AS          "dtsched:13"  FS "Save As..."
#define TXT_PRINT            "dtsched:14"  FS "Print"
#define TXT_EXIT             "dtsched:15"  FS "Exit"

#define TXT_UNDO             "dtsched:20"  FS "Undo"
#define TXT_CUT              "dtsched:21"  FS "Cut"
#define TXT_COPY             "dtsched:22"  FS "Copy"
#define TXT_PASTE            "dtsched:23"  FS "Paste"
#define TXT_DELETE           "dtsched:24"  FS "Delete"
#define TXT_SELECTALL        "dtsched:25"  FS "Select All"
#define TXT_UNSELECTALL      "dtsched:26"  FS "Unselect All"
#define TXT_INSERT           "dtsched:27"  FS "Insert..."

#define TXT_SPLIT            "dtsched:30"  FS "Another..."
#define TXT_PROPERTIES       "dtsched:31"  FS "Properties..."

#define TXT_APP_HELP         "dtsched:40"  FS "Task Scheduler..."
#define TXT_TOC_HELP         "dtsched:41"  FS "Table of Contents..."
#define TXT_HELPDESK         "dtsched:42"  FS "Help Desk..."
#define TXT_TOC_HELP_TITLE   "dtsched:43"  FS "Table of Contents"
#define TXT_HELPDESK_TITLE   "dtsched:44"  FS "Help Desk"
#define TXT_HELPDESK_SECT    "HelpDesk"
#define TXT_MAIN_HELP_TITLE  "dtsched:46"  FS "Application"
#define TXT_MAIN_HELP_SECT   "10"
#define TXT_PROP_HELP_TITLE  "dtsched:48"  FS "Properties"
#define TXT_PROP_HELP_SECT   "10"

#define TXT_FILE             "dtsched:50"  FS "File"
#define TXT_EDIT             "dtsched:51"  FS "Edit"
#define TXT_VIEW             "dtsched:52"  FS "View"
#define TXT_HELP             "dtsched:53"  FS "Help"

#define TXT_APPLY            "dtsched:60"  FS "Apply"
#define TXT_SET_DEFAULT      "dtsched:61"  FS "Set"
#define TXT_RESET            "dtsched:62"  FS "Reset"
#define TXT_RESET_TO_FACTORY "dtsched:63"  FS "Reset to Factory"
#define TXT_CANCEL           "dtsched:64"  FS "Cancel"
#define TXT_HELP_DDD         "dtsched:65"  FS "Help..."

#define TXT_PROP_TITLE       "dtsched:70"  FS "Task Scheduler: Properties"
#define TXT_OPEN_TITLE       "dtsched:71"  FS "Task Scheduler: Open"
#define TXT_SAVE_TITLE       "dtsched:72"  FS "Task Scheduler: Save"
#define TXT_OVERWRITE_TITLE  "dtsched:73"  FS "Task Scheduler: Overwrite"
#define TXT_EXIT_TITLE       "dtsched:74"  FS "Task Scheduler: Exit"
#define TXT_CLIENT_UNTITLED  "dtsched:75"  FS "Task Scheduler"
#define TXT_UNTITLED_BUFFER  "dtsched:76"  FS "New document."
#define TXT_FILE_OPENED      "dtsched:77"  FS "Document opened."

#define TXT_BROWSE           "dtsched:80"  FS "Browse..."
#define TXT_DISCARD          "dtsched:81"  FS "Discard"
#define TXT_FILENAME         "dtsched:82"  FS "Filename:"
#define TXT_CONTINUE         "dtsched:83"  FS "Continue"
#define TXT_OVERWRITE        "dtsched:84"  FS "Overwrite"
#define TXT_HELPDDD          "dtsched:85"  FS "Help..."

#define TXT_OPEN_NOTICE      "dtsched:90"  FS "The document has been changed.\nDiscard Changes?"
#define TXT_SAVE_NOTICE      "dtsched:91"  FS "The document could not be saved."
#define TXT_OVERWRITE_NOTICE "dtsched:92"  FS "The file already exists.\nOverwrite it?"
#define TXT_CLEAR_MESSAGE    "dtsched:93"  FS "Document cleared."
#define TXT_SAVED_MESSAGE    "dtsched:94"  FS "Document saved."
#define TXT_OPENED_MESSAGE   "dtsched:95"  FS "Document opened."
#define TXT_OPEN_CANCEL      "dtsched:96"  FS "Open cancelled."
#define TXT_NO_SAVE_MESSAGE  "dtsched:97"  FS "Document not saved!"
#define TXT_SAVE_CANCEL      "dtsched:98"  FS "Save cancelled."
#define TXT_OVER_MESSAGE     "dtsched:99"  FS "Document overwritten!"
#define TXT_NO_OVER_MESSAGE  "dtsched:100" FS "Document not overwritten!"
#define TXT_OVER_CANCEL      "dtsched:101" FS "Overwrite cancelled."
#define TXT_NEW_DOC_MESSAGE  "dtsched:102" FS "New document."
#define TXT_EXIT_NOTICE      "dtsched:103" FS "The document has not been saved.\nExit anyway?"
#define TXT_EXIT_CANCEL      "dtsched:104" FS "Exit cancelled."
#define TXT_PROP_NOTICE      "dtsched:105" FS "Only one item can be edited at a time."

#define TXT_PROP_POPUP       "dtsched:110" FS "Property window mapped."
#define TXT_PROP_APPLIED     "dtsched:111" FS "Properties applied."
#define TXT_PROP_RESET       "dtsched:112" FS "Properties reset to previous settings."

#define TXT_PROP_FACTORY     "dtsched:120" FS "Properties reset to the factory settings."
#define TXT_PROP_CANCEL      "dtsched:121" FS "Property window cancelled."

#define TXT_CANT_SAVE        "dtsched:130" FS "Document could not be saved."
#define TXT_BROWSE_MESSAGE   "dtsched:131" FS "Browsing..."
#define TXT_NEEDS_OVERWRITE  "dtsched:132" FS "Document exists.  Overwrite?"
#define TXT_OPENED_ANOTHER   "dtsched:133" FS "Another window into document opened."
#define TXT_NEW_VIEW         "dtsched:134" FS "New window into document opened."
#define TXT_DISCARD_CHANGES  "dtsched:135" FS "Document has beed changed.  Discard changes?"

#define TXT_CANT_PRINT       "dtsched:140" FS "Document cannot be printed."
#define TXT_NOTHING_TO_PRINT "dtsched:141" FS "Document is empty.  Nothing to print."
#define TXT_DOC_PRINTED      "dtsched:142" FS "Document has beed printed."

#define TXT_COMMAND_APPLY    "dtsched:150" FS "Apply"
#define TXT_COMMAND_RESET    "dtsched:151" FS "Reset"
#define TXT_COMMAND_CANCEL   "dtsched:152" FS "Cancel"
#define TXT_COMMAND_HELP     "dtsched:153" FS "Help"
#define TXT_MESSAGE_PROMPT   "dtsched:154" FS "Task:"
#define TXT_DEFAULT_MESSAGE  "dtsched:155" FS ""
#define TXT_TIME_PROMPT      "dtsched:156" FS "Time:"
#define TXT_DEFAULT_TIME     "0:00"

#define TXT_ALARM_TITLE      "dtsched:160" FS "Task Scheduler: Edit Properties"
#define TXT_SORT_TASK        "dtsched:161" FS "Sort by Taskname"
#define TXT_SORT_TIME        "dtsched:162" FS "Sort by Time"

#define TXT_OF               "dtsched:170" FS "of"

#define TXT_1                "dtsched:201" FS "1st"
#define TXT_2                "dtsched:202" FS "2nd"
#define TXT_3                "dtsched:203" FS "3rd"
#define TXT_4                "dtsched:204" FS "4th"
#define TXT_5                "dtsched:205" FS "5th"
#define TXT_6                "dtsched:206" FS "6th"
#define TXT_7                "dtsched:207" FS "7th"
#define TXT_8                "dtsched:208" FS "8th"
#define TXT_9                "dtsched:209" FS "9th"
#define TXT_10               "dtsched:210" FS "10th"
#define TXT_11               "dtsched:211" FS "11th"
#define TXT_12               "dtsched:212" FS "12th"
#define TXT_13               "dtsched:213" FS "13th"
#define TXT_14               "dtsched:214" FS "14th"
#define TXT_15               "dtsched:215" FS "15th"
#define TXT_16               "dtsched:216" FS "16th"
#define TXT_17               "dtsched:217" FS "17th"
#define TXT_18               "dtsched:218" FS "18th"
#define TXT_19               "dtsched:219" FS "19th"
#define TXT_20               "dtsched:220" FS "20th"
#define TXT_21               "dtsched:221" FS "21st"
#define TXT_22               "dtsched:222" FS "22nd"
#define TXT_23               "dtsched:223" FS "23rd"
#define TXT_24               "dtsched:224" FS "24th"
#define TXT_25               "dtsched:225" FS "25th"
#define TXT_26               "dtsched:226" FS "26th"
#define TXT_27               "dtsched:227" FS "27th"
#define TXT_28               "dtsched:228" FS "28th"
#define TXT_29               "dtsched:229" FS "29th"
#define TXT_30               "dtsched:230" FS "30th"
#define TXT_31               "dtsched:231" FS "31st"

#define TXT_COMMAND_INSERT   "dtsched:240" FS "Add Task"
#define TXT_INSERT_TITLE     "dtsched:241" FS "Task Scheduler: Add Task"

#define TXT_SUNDAY           "dtsched:250" FS "Sunday"
#define TXT_MONDAY           "dtsched:251" FS "Monday"
#define TXT_TUESDAY          "dtsched:252" FS "Tuesday"
#define TXT_WEDNESDAY        "dtsched:253" FS "Wednesday"
#define TXT_THURSDAY         "dtsched:254" FS "Thursday"
#define TXT_FRIDAY           "dtsched:255" FS "Friday"
#define TXT_SATURDAY         "dtsched:256" FS "Saturday"

#define TXT_JANUARY          "dtsched:260" FS "January"
#define TXT_FEBRUARY         "dtsched:261" FS "February"
#define TXT_MARCH            "dtsched:262" FS "March"
#define TXT_APRIL            "dtsched:263" FS "April"
#define TXT_MAY              "dtsched:264" FS "May"
#define TXT_JUNE             "dtsched:265" FS "June"
#define TXT_JULY             "dtsched:266" FS "July"
#define TXT_AUGUST           "dtsched:267" FS "August"
#define TXT_SEPTEMBER        "dtsched:268" FS "September"
#define TXT_OCTOBER          "dtsched:269" FS "October"
#define TXT_NOVEMBER         "dtsched:270" FS "November"
#define TXT_DECEMBER         "dtsched:271" FS "December"

#define TXT_EVERY_DAY_OF     "dtsched:280" FS "Every Day"
#define TXT_EVERY_DAY        "dtsched:281" FS "Every Day"

#define TXT_WHEN             "dtsched:290" FS "When:"
#define TXT_WEEKDAY          "dtsched:291" FS "Day of Week:"
#define TXT_MONTH            "dtsched:292" FS "Month:"
#define TXT_EVERY_MONTH      "dtsched:293" FS "Every Month"
#define TXT_DATE_PROMPT      "dtsched:294" FS "Date:"
#define TXT_DAY_OF_WEEK      "dtsched:295" FS "Day of Week"
#define TXT_SPECIFIC_DATE    "dtsched:296" FS "Date"
#define TXT_IGNORE           "dtsched:297" FS "Every:"
#define TXT_IGNORE_HOUR      "dtsched:298" FS "Hour"
#define TXT_IGNORE_MINUTE    "dtsched:299" FS "Minute"
#define MNE_APP_HELP         "dtsched:300" FS "H"
#define MNE_APRIL            "dtsched:301" FS "A"
#define MNE_AUGUST           "dtsched:302" FS "G"
#define MNE_CANCEL           "dtsched:303" FS "C"
#define MNE_COMMAND_APPLY    "dtsched:304" FS "A"
#define MNE_COMMAND_CANCEL   "dtsched:305" FS "C"
#define MNE_COMMAND_HELP     "dtsched:306" FS "H"
#define MNE_COMMAND_INSERT   "dtsched:307" FS "I"
#define MNE_COMMAND_RESET    "dtsched:308" FS "R"
#define MNE_DAY_OF_WEEK      "dtsched:309" FS "W"
#define MNE_DECEMBER         "dtsched:310" FS "D"
#define MNE_DELETE           "dtsched:311" FS "D"
#define MNE_EDIT             "dtsched:312" FS "E"
#define MNE_EVERY_DAY        "dtsched:313" FS "E"
#define MNE_EVERY_MONTH      "dtsched:314" FS "E"
#define MNE_EXIT             "dtsched:315" FS "E"
#define MNE_FEBRUARY         "dtsched:316" FS "F"
#define MNE_FILE             "dtsched:317" FS "F"
#define MNE_FRIDAY           "dtsched:318" FS "F"
#define MNE_HELP             "dtsched:319" FS "H"
#define MNE_HELPDDD          "dtsched:320" FS "H"
#define MNE_HELPDESK         "dtsched:321" FS "D"
#define MNE_IGNORE_HOUR      "dtsched:322" FS "O"
#define MNE_IGNORE_MINUTE    "dtsched:323" FS "M"
#define MNE_INSERT           "dtsched:324" FS "I"
#define MNE_JANUARY          "dtsched:325" FS "J"
#define MNE_JULY             "dtsched:326" FS "L"
#define MNE_JUNE             "dtsched:327" FS "N"
#define MNE_MARCH            "dtsched:328" FS "M"
#define MNE_MAY              "dtsched:329" FS "Y"
#define MNE_MONDAY           "dtsched:330" FS "M"
#define MNE_NOVEMBER         "dtsched:331" FS "N"
#define MNE_OCTOBER          "dtsched:332" FS "O"
#define MNE_PRINT            "dtsched:333" FS "P"
#define MNE_PROPERTIES       "dtsched:334" FS "P"
#define MNE_SATURDAY         "dtsched:335" FS "U"
#define MNE_SAVE             "dtsched:336" FS "S"
#define MNE_SEPTEMBER        "dtsched:337" FS "S"
#define MNE_SORT_TASK        "dtsched:338" FS "K"
#define MNE_SORT_TIME        "dtsched:339" FS "T"
#define MNE_SPECIFIC_DATE    "dtsched:340" FS "D"
#define MNE_SUNDAY           "dtsched:341" FS "S"
#define MNE_THURSDAY         "dtsched:342" FS "R"
#define MNE_TOC_HELP         "dtsched:343" FS "T"
#define MNE_TUESDAY          "dtsched:344" FS "T"
#define MNE_UNDO             "dtsched:345" FS "U"
#define MNE_VIEW             "dtsched:346" FS "V"
#define MNE_WEDNESDAY        "dtsched:347" FS "W"

extern Widget       root;

extern void         SetMessage(MainWindow * mw, char * message);
extern MainWindow * FindMainWindow(Widget w);
extern void         DestroyInputWindow(MainWindow * mw, InputWindow * iw);
extern void         DestroyMainWindow(MainWindow * mw);
extern void         SetToCurrent(MainWindow * mw, int item_index);

#endif
