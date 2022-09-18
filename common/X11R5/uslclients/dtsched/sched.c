/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define DOPRINT
#ifndef NOIDENT
#pragma ident	"@(#)dtsched:sched.c	1.15"
#endif

/*
 * sched.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <buffutil.h>
#include <textbuff.h>

#include <OlDnDVCX.h>

#include <OpenLook.h>
#include <Flat.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <BaseWGizmo.h>
#include <ModalGizmo.h>
#include <InputGizmo.h>
#include <TimeGizmo.h>
#include <ListGizmo.h>
#include <ChoiceGizm.h>
#include <NumericGiz.h>

#include <sched.h>
#include <prop.h>
#include <quit.h>

extern char *  strndup();

static void    RetrieveCrontab(Crontab * crontab);
static char *  NextField(char * p, int last);
static void    CreateApplication(Crontab * crontab, int argc, char * argv[]);

static void    FileCB(Widget w, XtPointer client_data, XtPointer call_data);
static void    EditCB(Widget w, XtPointer client_data, XtPointer call_data);
static void    ViewCB(Widget w, XtPointer client_data, XtPointer call_data);
static void    HelpCB(Widget w, XtPointer client_data, XtPointer call_data);
static void    HandleWMProtocols(Widget w, XtPointer client_data, XtPointer call_data);
static void    TestForExit(MainWindow * mw);
static Boolean DropNotify (Widget w, Window win, Position x, Position y,
                                Atom selection, Time timestamp,
                                OlDnDDropSiteID drop_site_id,
                                OlDnDTriggerOperation op,
                                Boolean send_done, Boolean forwarded,
                                XtPointer closure);
static void    SelectionCB(Widget w, XtPointer client_data,
                                Atom * selection, Atom * type, XtPointer value,
                                unsigned long * length, int * format);
static void    ProtocolActionCB(Widget w, Atom selection, 
                                OlDnDProtocolAction action, 
                                Boolean convert_not_fail, XtPointer closure);
static void    InputCB(Widget w, XtPointer client_data, XEvent * event);
/*
 *
 #    #  ######  #       #####
 #    #  #       #       #    #
 ######  #####   #       #    #
 #    #  #       #       #####
 #    #  #       #       #
 #    #  ######  ######  #
 *
 */

static HelpInfo ApplicationHelp =
   { FormalClientName, TXT_MAIN_HELP_TITLE,  HELPPATH, TXT_MAIN_HELP_SECT };
static HelpInfo TOCHelp  =
   { FormalClientName, TXT_TOC_HELP_TITLE,   HELPPATH, "TOC" };
static HelpInfo HelpDeskHelp =
   { FormalClientName, TXT_HELPDESK_TITLE,   HELPPATH, TXT_HELPDESK_SECT };

/*
 *
 #    #  ######  #    #  #    #   ####
 ##  ##  #       ##   #  #    #  #
 # ## #  #####   # #  #  #    #   ####
 #    #  #       #  # #  #    #       #
 #    #  #       #   ##  #    #  #    #
 #    #  ######  #    #   ####    ####
 *
 */

typedef enum
   {
   FileSave, FilePrint, FileExit
   } FileMenuItemIndex;

typedef enum
   {
   EditUndo, EditClear, EditInsert, EditProperties
   } EditMenuItemIndex;

typedef enum
   {
   ViewSortTask, ViewSortTime
   } ViewMenuItemIndex;

typedef enum
   {
   HelpApp, HelpTOC, HelpHelpDesk
   } HelpMenuItemIndex;

static MenuItems  FileMenuItems[] =
   {
   { True, TXT_SAVE,          MNE_SAVE  },
   { True, TXT_PRINT,         MNE_PRINT },
   { True, TXT_EXIT,          MNE_EXIT  },
   { 0 }
   };

static MenuItems  EditMenuItems[] =
   {
   { True, TXT_UNDO,         MNE_UNDO       },
   { True, TXT_DELETE,       MNE_DELETE     },
   { True, TXT_INSERT,       MNE_INSERT     },
   { True, TXT_PROPERTIES,   MNE_PROPERTIES },
   { 0 }
   };

static MenuItems  ViewMenuItems[] =
   {
   { True, TXT_SORT_TASK,   MNE_SORT_TASK },
   { True, TXT_SORT_TIME,   MNE_SORT_TIME },
   { 0 }
   };

static MenuItems  HelpMenuItems[] =
   {
   { True, TXT_APP_HELP,        MNE_APP_HELP },
   { True, TXT_TOC_HELP,        MNE_TOC_HELP },
   { True, TXT_HELPDESK,        MNE_HELPDESK },
   { 0 }
   };

static MenuGizmo FileMenu =
   { NULL,      "filemenu",   NULL,   FileMenuItems, FileCB };
static MenuGizmo EditMenu =
   { NULL,      "editmenu",   NULL,   EditMenuItems, EditCB };
static MenuGizmo ViewMenu =
   { NULL,      "viewmenu",   NULL,   ViewMenuItems, ViewCB };
static MenuGizmo HelpMenu =
   { NULL,      "helpmenu",   NULL,   HelpMenuItems, HelpCB };

static MenuItems  BarMenuItems[] =
   {
   { True, TXT_FILE, MNE_FILE, (char *)&FileMenu },
   { True, TXT_EDIT, MNE_EDIT, (char *)&EditMenu },
   { True, TXT_VIEW, MNE_VIEW, (char *)&ViewMenu },
   { True, TXT_HELP, MNE_HELP, (char *)&HelpMenu },
   { 0 }
   };

static MenuGizmo MenuBar =
   {
   NULL, "menubar", NULL, BarMenuItems, NULL, NULL, CMD, OL_FIXEDROWS, 1
   };

static void ListSelect(Widget w, XtPointer client_data, XtPointer call_data);

static char listFormat[] = "%-s %-s %-s";
static ListHead listHead;
static Setting listSetting = {NULL, NULL, (XtPointer)&listHead, NULL, 0};
static ListGizmo listGizmo =
   {
   NULL, "list", NULL, &listSetting, listFormat, TRUE, ITEMS_VISIBLE, NULL, False, NULL, ListSelect
   };
static GizmoRec gizmos[] = 
   {
      { ListGizmoClass,    &listGizmo     },
   };
static BaseWindowGizmo BaseWindow =
   {
   &ApplicationHelp, ClientName, TXT_CLIENT_UNTITLED, &MenuBar,
   gizmos, XtNumber(gizmos), TXT_ICON_NAME, ICONPATH
   };


static Boolean Warnings;
static XtResource resources[] =
   {
      { "warnings", "Warnings", XtRBoolean, sizeof(Boolean),
        (Cardinal) &Warnings, XtRBoolean, (XtPointer) &Warnings
      },
   };

static XtCallbackRec protocolCB[] =
   {
      { HandleWMProtocols, NULL },
      { NULL }
   };

static MainWindow    mainWindow;

Widget        root;

/*
 * main
 *
 */

extern int
main(int argc, char * argv[])
{

   Crontab * crontab = (Crontab *)AllocateBuffer(sizeof(Cronentry), 10);

   RetrieveCrontab(crontab);

   CreateApplication(crontab, argc, argv);

} /* end of main */
/*
 * Decode
 *
 */

static char *
Decode(int value, int incr)
{
   char * r = MALLOC(20);

   if (value == 0)
      strcpy(r, "*");
   else
   {
      sprintf(r, "%d", value - incr);
   }

   return (r);

} /* end of Decode */
/*
 * AddCronEntry
 *
 */

static void
AddCronEntry(Crontab * crontab, int min, int hr, 
   int day, int mth, int weekday, char * command) 
{
   int i = crontab->used;

   if (BufferFilled((Buffer *)crontab))
      GrowBuffer((Buffer *)crontab, 10);

   crontab->p[i].fields = (char **)malloc(sizeof(char *) * NUM_FLDS);
   crontab->p[i].set               = False;
   crontab->p[i].MINUTE_FLD        = Decode(min, 1);
   crontab->p[i].HOUR_FLD          = Decode(hr, 1);
   crontab->p[i].DAY_OF_MONTH_FLD  = Decode(day, 0);
   crontab->p[i].MONTH_OF_YEAR_FLD = Decode(mth, 0);
   crontab->p[i].DAY_OF_WEEK_FLD   = Decode(weekday, 1);
   crontab->p[i].COMMAND_FLD       = STRDUP(command);

   crontab->p[i].BASENAME_FLD      = BasenameOf(crontab->p[i].COMMAND_FLD);
   crontab->p[i].TIME_FLD          = 
      TimeOf(crontab->p[i].HOUR_FLD, crontab->p[i].MINUTE_FLD);
   crontab->p[i].DAY_DATE_FLD     = 
      DayOrDate(crontab->p[i].DAY_OF_MONTH_FLD, 
                crontab->p[i].MONTH_OF_YEAR_FLD, 
                crontab->p[i].DAY_OF_WEEK_FLD);
   crontab->p[i].POPUP_FLD         = NULL;
   crontab->p[i].clientData        = NULL;
   crontab->used++;

} /* end of AddCronEntry */
/*
 * ExpandField
 *
 */

static char *
ExpandField(char * fld, int incr)
{
   char * r = MALLOC(100);
   char * p;
   char * s;
   int    start;
   int    end;
   int    i;

   if (strcmp(fld, "*") == 0)
   {
      strcpy(r, "x");
   }
   else
   {
      char * c = STRDUP(fld);

      for (p = r; p < &r[60]; p++)
         *p = '-';
      *p = 0;

      for (s = strtok(c, "-,"); s; s = strtok(NULL, "-,"))
      {
#ifdef DEBUG
         fprintf(stderr,"x = %d fld[x] = '%c'\n", s-c-1, fld[s-c-strlen(s)]);
#endif
         switch(fld[s - c + strlen(s)])
         {
            default:
            case ',':
               r[atoi(s) + incr] = 'x';
               break;
            case '-':
               start = atoi(s);
               end   = atoi(strtok(NULL, "-,"));
               if (start > end)
               {
                  i = start;
                  start = end;
                  end = i;
               }
#ifdef DEBUG
               fprintf(stderr,"setting %d to %d\n", start, end);
#endif
               for (i = start + incr; i <= end + incr; i++)
                  r[i] = 'x';
               break;
         }
      }
      FREE(c);
   }

#ifdef DEBUG
   fprintf(stderr,"bit: '%s' from '%s'\n", r, fld);
#endif
   return r;

} /* end of ExpandField */
/*
 * AddToCrontab
 *
 */

static void
AddToCrontab(Crontab * crontab, char * string)
{
   char * minute_fld         = NextField(string, 0);
   char * hour_fld           = NextField(NULL, 0);
   char * day_of_month_fld   = NextField(NULL, 0);
   char * month_of_year_fld  = NextField(NULL, 0);
   char * day_of_week_fld    = NextField(NULL, 0);
   char * command_fld        = NextField(NULL, 1);

   char * minute_bit         = ExpandField(minute_fld, 1);
   char * hour_bit           = ExpandField(hour_fld, 1);
   char * day_of_month_bit   = ExpandField(day_of_month_fld, 0);
   char * month_of_year_bit  = ExpandField(month_of_year_fld, 0);
   char * day_of_week_bit    = ExpandField(day_of_week_fld, 1);

   int    minute_inx         = 0;
   int    hour_inx           = 0;
   int    day_of_month_inx   = 0;
   int    month_of_year_inx  = 0;
   int    day_of_week_inx    = 0;
   int    ignore_weekday;
   int    ignore_date;

   for (minute_inx = 0; minute_bit[minute_inx]; minute_inx++)
   {
      if (minute_bit[minute_inx] == 'x')
         for (hour_inx = 0; hour_bit[hour_inx]; hour_inx++)
         {
            if (hour_bit[hour_inx] == 'x')
            {
               ignore_weekday = (*day_of_week_bit == 'x');
               ignore_date    = 
                  (*day_of_month_bit == 'x' && *month_of_year_bit == 'x');
               if (ignore_weekday && ignore_date) /* every day */
                  AddCronEntry(crontab, minute_inx, hour_inx, 0, 0, 0, command_fld);
               else  
               {
                  if (!ignore_weekday) /* on day(s) of the week */
                     for (day_of_week_inx = 1; 
                          day_of_week_bit[day_of_week_inx]; 
                          day_of_week_inx++)
                     {
                        if (day_of_week_bit[day_of_week_inx] == 'x')
                           AddCronEntry(crontab, minute_inx, hour_inx, 0, 0, 
                              day_of_week_inx, command_fld);
                     }
#ifdef DEBUG
else fprintf(stderr,"ignore weekday\n");
#endif
                  if (!ignore_date) /* on day(s) of month(s) */
                  {
                     for (day_of_month_inx = 0; 
                          day_of_month_bit[day_of_month_inx]; 
                          day_of_month_inx++)
                     {
                        if (day_of_month_bit[day_of_month_inx] == 'x')
                           for (month_of_year_inx = 0; 
                                month_of_year_bit[month_of_year_inx]; 
                                month_of_year_inx++)
                              if (month_of_year_bit[month_of_year_inx] == 'x')
                                 AddCronEntry(crontab, minute_inx, hour_inx, 
                                   day_of_month_inx, month_of_year_inx, 0, command_fld);
                     }
                  }
#ifdef DEBUG
else fprintf(stderr,"ignore date\n");
#endif
               }
            }
         }
   }

   FREE(minute_bit);
   FREE(hour_bit);
   FREE(day_of_month_bit);
   FREE(month_of_year_bit);
   FREE(day_of_week_bit);
   
} /* end of AddToCrontab */
/*
 * RetrieveCrontab
 *
 */

static void
RetrieveCrontab(Crontab * crontab)
{
   static Cronline * buffer;
   FILE *            fp;

   if ((fp = popen(RETRIEVE_COMMAND, "r")) != NULL)
   {
      buffer = (Cronline *)AllocateBuffer(sizeof(BufferElement), 80);

      while (ReadFileIntoBuffer(fp, (Buffer *)buffer) != EOF)
      {
         buffer->p = (wchar_t *)wstostr((char *)buffer->p, buffer->p);
         if (*((char *)buffer->p) != '#')
            AddToCrontab(crontab, (char *)buffer->p);
      }
      FreeBuffer((Buffer *)buffer);
   }

} /* end of RetrieveCrontab */
/*
 * NextField
 *
 */

static char *
NextField(char * p, int last)
{
   static char * s;
   static char * e;

   if (p)
      s = p;

   if (!last)
   {
      while (*s == ' ' || *s == '\t')     s++;  /* strip leading whitespace */
      e = s;
      while (*e != ' ' && *e != '\t')     e++;  /* find end of token        */

      p = strndup(s, e-s);
      s = e;
   }
   else
   {
      while (*s == ' ' || *s == '\t')     s++;  /* strip leading whitespace */
      p = strdup(s);
   }

   return (p);

} /* end of NextField */
/*
 * CreateApplication
 *
 */

static void
CreateApplication(Crontab * crontab, int argc, char * argv[])
{
   Arg arg[10];

   root = InitializeGizmoClient(ClientName, ClientClass,
      FormalClientName,
      NULL, NULL,
/*
 or eventually (if there is a need for persistent properties of the application)
      PopupGizmoClass, &PropertiesPrompt,
*/
      NULL, 0,
      &argc, argv,
      NULL,
      NULL, resources, XtNumber(resources), NULL, 0, DROP_RESOURCE,
      DropNotify, NULL);

   if (crontab->used)
   {
      listHead.list = (ListItem *)crontab->p;
      listHead.size = (int)crontab->used;
      listHead.numFields = 6;
   }
   else
   {
      static char *   fields[] = { " ", " ", " ", " ", " ", " " };
      static ListItem empty_list[] = { False, (XtArgVal)fields, NULL };
      listHead.list = empty_list;
      listHead.size = 1;
      listHead.numFields = 6;
   }

   mainWindow.baseWindow = &BaseWindow;
   mainWindow.listHead   = &listHead;
   mainWindow.crontab    = crontab;
   mainWindow.oldcrontab = NULL;
   mainWindow.listGizmo  = &listGizmo;
   mainWindow.exitNotice = NULL;
   mainWindow.dirty      = False;
   mainWindow.withdrawn  = True;
   mainWindow.iw         = NULL;

   if (root)
   {

      mainWindow.baseWindow->title = GetGizmoText(mainWindow.baseWindow->title);
      XtSetArg(arg[0], XtNwmProtocol, protocolCB);
      (void)CreateGizmo(root, BaseWindowGizmoClass, mainWindow.baseWindow, arg, 1);
      if (!crontab->used)
      {
         XtSetArg(arg[0], XtNitems,        crontab->p);
         XtSetArg(arg[1], XtNnumItems,     crontab->used);
         XtSetArg(arg[2], XtNitemsTouched, True);
         XtSetArg(arg[3], XtNviewHeight,   ITEMS_VISIBLE);
         XtSetValues(mainWindow.listGizmo->flatList, arg, 4);
      }

      OlDnDRegisterDDI(mainWindow.listGizmo->flatList, 
         OlDnDSitePreviewNone, DropNotify,
         (OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL);

      OlDnDRegisterDDI(mainWindow.baseWindow->icon_shell, 
         OlDnDSitePreviewNone, DropNotify,
         (OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL);

      if (argc == 1)
      {
         mainWindow.withdrawn = False;
         MapGizmo(BaseWindowGizmoClass, &BaseWindow);
      }
      else
         CreateInputPropertyWindow(mainWindow.baseWindow->shell, argv[1]);
      CreatePropertyWindow(mainWindow.baseWindow->shell, 0);
   }

   GizmoMainLoop(InputCB, NULL, NULL, NULL);

} /* end of CreateApplication */
/*
 * SetMessage
 *
 */

extern void
SetMessage(MainWindow * mw, char * message)
{

   SetBaseWindowMessage(mw-> baseWindow, GetGizmoText(message));

} /* end of SetMessage */
/*
 * FindMainWindow
 *
 */

extern MainWindow *
FindMainWindow(Widget w)
{

   return (&mainWindow);

} /* end of FindMainWindow */
/*
 * DestroyInputWindow
 *
 */

extern void
DestroyInputWindow(MainWindow * mw, InputWindow * iw)
{
   InputWindow * p;
   InputWindow ** q = &mw->iw;

   for (p = mw->iw; p && p != iw; q = &p->next, p = p->next)
      ; /* loop till found */

   if (p == NULL)
   {
      (void)fprintf(stderr,"can't find input window!!!\n");
   }
   else
   {
   /*
    * FIX: destroy the widgets and free the input window
    */
      *q = p->next;
   }

   TestForExit(mw);

} /* end of DestroyInputWindow */
/*
 * DestroyMainWindow
 *
 */

extern void
DestroyMainWindow(MainWindow * mw)
{

   Widget w = mw->baseWindow->shell;

   XWithdrawWindow(XtDisplay(w), XtWindow(w), XScreenNumberOfScreen(XtScreen(w)));
   mw->withdrawn = True;
   TestForExit(mw);

} /* end of DestroyMainWindow */
/*
 * TestForExit
 *
 */

static void
TestForExit(MainWindow * mw)
{

   if (mw->withdrawn == True && mw->iw == NULL)
      exit(0);
#ifdef DEBUG
   else
      (void)fprintf(stderr, "withdrawn = %d mw->iw = %x\n", mw->withdrawn, mw->iw);
#endif

} /* end of TestForExit */
/*
 * SetToCurrent
 *
 */

extern void
SetToCurrent(MainWindow * mw, int item_index)
{
   Arg    arg[5];
   int    weekday_flag = strcmp(mw->crontab->p[item_index].DAY_OF_WEEK_FLD, "*");
   char * ignore_bit;

   if (mw->task->settings->previous_value)
      FREE(mw->task->settings->previous_value);
   mw->task->settings->previous_value = 
      STRDUP(mw->crontab->p[item_index].COMMAND_FLD);

   if (mw->time->settings->previous_value)
      FREE(mw->time->settings->previous_value);
   mw->time->settings->previous_value = MALLOC(10);
   sprintf(mw->time->settings->previous_value, "%02d:%02d", 
      atoi(mw->crontab->p[item_index].HOUR_FLD),
      atoi(mw->crontab->p[item_index].MINUTE_FLD));

   if (mw->ignore->settings->previous_value)
      FREE(mw->ignore->settings->previous_value);
   ignore_bit = STRDUP("__");
   if (strcmp(mw->crontab->p[item_index].HOUR_FLD, "*") == 0)
      ignore_bit[0] = 'x';
   if (strcmp(mw->crontab->p[item_index].MINUTE_FLD, "*") == 0)
      ignore_bit[1] = 'x';
   mw->ignore->settings->previous_value = ignore_bit;

#ifdef DEBUG
   fprintf(stderr,"hour = '%s' min = '%s' ignore = '%s'\n", 
      mw->crontab->p[item_index].HOUR_FLD, 
      mw->crontab->p[item_index].MINUTE_FLD, ignore_bit);
#endif

   mw->when->settings->previous_value = (XtPointer) (weekday_flag ? 0 : 1);

   mw->month->settings->previous_value = 
      (XtPointer)atoi(mw->crontab->p[item_index].MONTH_OF_YEAR_FLD);

   mw->date->settings->previous_value = 
      (XtPointer)atoi(mw->crontab->p[item_index].DAY_OF_MONTH_FLD);
/*
   if ((int)mw->date->settings->previous_value < 1)
       mw->date->settings->previous_value = (XtPointer)1;
*/
   mw->weekday->settings->previous_value = (XtPointer)
      (weekday_flag ? atoi(mw->crontab->p[item_index].DAY_OF_WEEK_FLD) + 1 : 0);

   ManipulateGizmo(PopupGizmoClass, mw->popupGizmo, ResetGizmoValue);

   if (weekday_flag)
   {
      ManageGizmo(ChoiceGizmoClass,   mw->weekday, UNHIDE, NULL);
      ManageGizmo(ChoiceGizmoClass,   mw->month,   HIDE, NULL);
#ifdef USE_NUMERIC_DATE
      ManageGizmo(NumericGizmoClass,  mw->date,    HIDE, NULL);
#else
      ManageGizmo(ChoiceGizmoClass,   mw->date,    HIDE, NULL);
#endif /* USE_NUMERIC_DATE */
   }
   else
   {
      ManageGizmo(ChoiceGizmoClass,   mw->weekday, HIDE, NULL);
      ManageGizmo(ChoiceGizmoClass,   mw->month,   UNHIDE, NULL);
#ifdef USE_NUMERIC_DATE
      ManageGizmo(NumericGizmoClass,  mw->date,    UNHIDE, NULL);
#else
      ManageGizmo(ChoiceGizmoClass,   mw->date,    UNHIDE, NULL);
#endif /* USE_NUMERIC_DATE */
   }

} /* end of SetToCurrent */
/*
 * FileCB
 *
 */

static void
FileCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);
   FILE *           fp;
   int              i;

   switch(p->item_index)
   {
      case FileSave:
         fp = popen(REPLACE_COMMAND, "w");
         if (fp == NULL)
            (void)fprintf(stderr, "couldn't open replace command\n");
         else
         {
            for (i = 0; i < mw->crontab->used; i++)
            {
               (void)fprintf(fp, "%s %s %s %s %s %s\n",
                  mw->crontab->p[i].MINUTE_FLD,
                  mw->crontab->p[i].HOUR_FLD,
                  mw->crontab->p[i].DAY_OF_MONTH_FLD,
                  mw->crontab->p[i].MONTH_OF_YEAR_FLD,
                  mw->crontab->p[i].DAY_OF_WEEK_FLD,
                  mw->crontab->p[i].COMMAND_FLD);
            }
            fclose(fp);
            mw->dirty = False;
         }
         break;
      case FilePrint:
#ifndef DOPRINT
#define fp stderr
#else
         while(XtPending())
         {
            XEvent event;

            XtNextEvent(&event);
            XtDispatchEvent(&event);
         }
         fp = popen(PRINT_COMMAND, "w");
         if (fp == NULL)
            (void)fprintf(stderr, "couldn't open replace command\n");
         else
#endif
         {
            for (i = 0; i < mw->crontab->used; i++)
            {
               (void)fprintf(fp, "%s %s %s %s %s %s\n",
                  mw->crontab->p[i].MINUTE_FLD,
                  mw->crontab->p[i].HOUR_FLD,
                  mw->crontab->p[i].DAY_OF_MONTH_FLD,
                  mw->crontab->p[i].MONTH_OF_YEAR_FLD,
                  mw->crontab->p[i].DAY_OF_WEEK_FLD,
                  mw->crontab->p[i].COMMAND_FLD);
            }
#ifdef DOPRINT
            fclose(fp);
#endif
         }
         break;
      case FileExit:
         QuitCB(w, NULL, NULL);
         break;
   }

} /* end of FileCB */
/*
 * EditCB
 *
 */

static void
EditCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);

   Arg              arg[10];
   int              i;
   int              t;
   int              x = 0;
   Crontab *        temp;

   switch(p->item_index)
   {
      case EditUndo:
         temp       = mw->oldcrontab;
         mw->oldcrontab = mw->crontab;
         mw->crontab    = temp;
         mw->listHead->list = (ListItem *)mw->crontab->p;
         mw->listHead->size = (int)mw->crontab->used;
         XtSetArg(arg[0], XtNitems,        mw->listHead->list);
         XtSetArg(arg[1], XtNnumItems,     mw->listHead->size);
         XtSetArg(arg[2], XtNitemsTouched, True);
         XtSetArg(arg[3], XtNviewHeight,   ITEMS_VISIBLE);
         XtSetValues(mw->listGizmo->flatList, arg, 4);
         for (i = 0; i < mw->listHead->size; i++)
            if (mw->listHead->list[i].set)
               break;
         SetToCurrent(mw, i == mw->listHead->size ? 0 : i);
         break;
      case EditClear:
         if (mw->oldcrontab)
            FreeBuffer((Buffer *)mw->oldcrontab);
         mw->oldcrontab = (Crontab *)CopyBuffer((Buffer *)mw->crontab);
         for (i = 0; i < mw->listHead->size; i++)
            if (mw->listHead->list[i].set)
            {
               bcopy(&mw->listHead->list[i + 1],
	              &mw->listHead->list[i],
		      sizeof(Cronentry) * (mw->listHead->size-1-i));
               mw->listHead->size--;
	       break;
            }
         mw->crontab->used = mw->listHead->size;
         XtSetArg(arg[0], XtNitems,        mw->listHead->list);
         XtSetArg(arg[1], XtNnumItems,     mw->listHead->size);
         XtSetArg(arg[2], XtNitemsTouched, True);
         XtSetArg(arg[3], XtNviewHeight,   ITEMS_VISIBLE);
         XtSetValues(mw->listGizmo->flatList, arg, 4);
         SetToCurrent(mw, 0);
         break;
      case EditInsert:
         if (mw->oldcrontab)
            FreeBuffer((Buffer *)mw->oldcrontab);
         mw->oldcrontab = (Crontab *)CopyBuffer((Buffer *)mw->crontab);
         for (i = 0; i < mw->listHead->size; i++)
            if (mw->listHead->list[i].set)
               mw->listHead->list[i].set = False;
         if (BufferFilled((Buffer *)mw->crontab))
            GrowBuffer((Buffer *)mw->crontab, 10);

         mw->crontab->p[mw->crontab->used].fields = (char **)malloc(sizeof(char *) * NUM_FLDS);
         mw->crontab->p[mw->crontab->used].set               = True;
         mw->crontab->p[mw->crontab->used].MINUTE_FLD        = STRDUP("");
         mw->crontab->p[mw->crontab->used].HOUR_FLD          = STRDUP("");
         mw->crontab->p[mw->crontab->used].DAY_OF_MONTH_FLD  = STRDUP("");
         mw->crontab->p[mw->crontab->used].MONTH_OF_YEAR_FLD = STRDUP("");
         mw->crontab->p[mw->crontab->used].DAY_OF_WEEK_FLD   = STRDUP("");
         mw->crontab->p[mw->crontab->used].COMMAND_FLD       = STRDUP("");

         mw->crontab->p[mw->crontab->used].BASENAME_FLD      = 
            BasenameOf(mw->crontab->p[mw->crontab->used].COMMAND_FLD);
         mw->crontab->p[mw->crontab->used].TIME_FLD          = 
            TimeOf(mw->crontab->p[mw->crontab->used].HOUR_FLD,
                   mw->crontab->p[mw->crontab->used].MINUTE_FLD);
         mw->crontab->p[mw->crontab->used].DAY_DATE_FLD     = 
            DayOrDate(mw->crontab->p[mw->crontab->used].DAY_OF_MONTH_FLD, 
                      mw->crontab->p[mw->crontab->used].MONTH_OF_YEAR_FLD, 
                      mw->crontab->p[mw->crontab->used].DAY_OF_WEEK_FLD);
         mw->crontab->p[mw->crontab->used].POPUP_FLD         = NULL;
         mw->crontab->p[mw->crontab->used].clientData        = NULL;

         SetToCurrent(mw, mw->crontab->used);
         MapGizmo(PopupGizmoClass, mw->popupGizmo);

         mw->crontab->used++;
         mw->listHead->size = mw->crontab->used;
         mw->listHead->list = (ListItem *)mw->crontab->p;
         XtSetArg(arg[0], XtNitems,        mw->listHead->list);
         XtSetArg(arg[1], XtNnumItems,     mw->listHead->size);
         XtSetArg(arg[2], XtNitemsTouched, mw->crontab->used > 1);
         XtSetArg(arg[3], XtNviewHeight,   ITEMS_VISIBLE);
         XtSetValues(mw->listGizmo->flatList, arg, 4);
         break;
      case EditProperties:
         for (i = 0; i < mw->listHead->size; i++)
            if (mw->listHead->list[i].set)
            {
               SetToCurrent(mw, i);
               MapGizmo(PopupGizmoClass, mw->popupGizmo);
               break;
            }
         break;
      default:
         (void)fprintf(stderr,
                 "default switch error: i = %d %s:%d\n",
                 p->item_index, __FILE__, __LINE__);
         break;
   }

} /* end of EditCB */
/*
 * Byname
 *
 */

static int
Byname(const void * s1, const void * s2)
{
   Cronentry * p1 = (Cronentry *)s1;
   Cronentry * p2 = (Cronentry *)s2;

   return(strcmp(p1->COMMAND_FLD, p2->COMMAND_FLD));

} /* end of Byname */
/*
 * Bytime
 *
 */

static int
Bytime(const void * s1, const void * s2)
{
   Cronentry * p1 = (Cronentry *)s1;
   Cronentry * p2 = (Cronentry *)s2;

   return(atoi(p1->HOUR_FLD) - atoi(p2->HOUR_FLD));

} /* end of Bytime */
/*
 * ViewCB
 *
 */

static void ViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);
   Arg              arg[2];

   switch(p->item_index)
   {
      case ViewSortTask:
         qsort((char *)mw->crontab->p, (unsigned)mw->crontab->used,
            sizeof(mw->crontab->p[0]), Byname);
         break;
      case ViewSortTime:
         qsort((char *)mw->crontab->p, (unsigned)mw->crontab->used,
            sizeof(mw->crontab->p[0]), Bytime);
         break;
   }

   XtSetArg(arg[0], XtNitemsTouched, True);
   XtSetArg(arg[1], XtNitems, mw->listHead->list);
   XtSetValues(mw->listGizmo->flatList, arg, 2);

} /* end of ViewCB */
/*
 * HelpCB
 *
 */

static void HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);

   switch (p->item_index)
   {
      case HelpApp:
         PostGizmoHelp(mw->baseWindow->shell, &ApplicationHelp);
         break;
      case HelpTOC:
         PostGizmoHelp(mw->baseWindow->shell, &TOCHelp);
         break;
      case HelpHelpDesk:
         PostGizmoHelp(mw->baseWindow->shell, &HelpDeskHelp);
         break;
      default:
         (void)fprintf(stderr, "error: default in HelpCB\n");
         break;
   }

} /* end of HelpCB */
/*
 * HandleWMProtocols
 *
 */

static void
HandleWMProtocols(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlWMProtocolVerify * cd = (OlWMProtocolVerify *)call_data;

   if (cd->msgtype == OL_WM_DELETE_WINDOW)
      QuitCB(w, NULL, NULL);
   else
      ;
#ifdef DEBUG
   (void)fprintf(stderr,"wm protocol - %d \n", cd->msgtype);
#endif

} /* end of HandleWMProtocols */
/*
 * ListSelect
 *
 */

static void
ListSelect(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p  = (OlFlatCallData *)call_data;
   MainWindow *     mw = FindMainWindow(w);
   
   SetToCurrent(mw, p->item_index);

} /* end of ListSelect */
/*
 * SelectionCB
 *
 */

static void
SelectionCB(Widget w, XtPointer client_data,
   Atom * selection, Atom * type,
   XtPointer value, unsigned long * length, int * format)
{
   OlDnDTriggerOperation op = (OlDnDTriggerOperation)client_data;
   MainWindow *          mw = FindMainWindow(w);

   if (w != root) /* drop on the list or icon */
   {
      if (*type == OL_XA_FILE_NAME(XtDisplay(w)))
      {
#ifdef DEBUG
         (void)fprintf(stderr,"task filename = '%s'\n", value);
#endif
         CreateInputPropertyWindow(mw->baseWindow->shell, value);
      }
      else
      {
#ifdef DEBUG
         (void)fprintf(stderr," ignoring atom %d '%s' (%d)\n",
            *type, value ? value : "null", *length);
#endif
      }
   }
   else
   {
#ifdef DEBUG
      (void)fprintf(stderr,"psuedo-drop: atom %d '%s' (%d)\n",
         *type, value ? value : "null", *length);
#endif
      if (value && *((char *)value))
         CreateInputPropertyWindow(mw->baseWindow->shell, value);
      else
      {
         MapGizmo(BaseWindowGizmoClass, mw->baseWindow);
         XRaiseWindow(XtDisplay(mw->baseWindow->shell), XtWindow(mw->baseWindow->shell));
      }
   }

#ifdef DEBUG
   (void)fprintf(stderr," terminate conversation\n");
#endif
   OlDnDDragNDropDone(w, *selection, XtLastTimestampProcessed(XtDisplay(w)),
      ProtocolActionCB, NULL);

} /* end of SelectionCB */
/*
 * DropNotify
 *
 */

static Boolean
DropNotify (Widget w, Window win, Position x, Position y, Atom selection,
            Time timestamp, OlDnDDropSiteID drop_site_id,
            OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
            XtPointer closure)
{
   Atom         target = OL_XA_FILE_NAME(XtDisplay(w));
   MainWindow * mw     = FindMainWindow(w);

#ifdef DEBUG
   (void)fprintf(stderr, "in drop notify\n");
#endif
   if (w != root) /* drop on the list or icon */
   {
      if ((op == OlDnDTriggerMoveOp) || (op == OlDnDTriggerCopyOp))
      {
         XtGetSelectionValue(w, selection, OL_XA_FILE_NAME(XtDisplay(w)),
            SelectionCB, (XtPointer)op, timestamp);
      }
#ifdef DEBUG
      else
         (void)fprintf(stderr, "ignoring %d DropNotify: %s\n", op, XtName(w));
#endif
   }
   else
      XtGetSelectionValue(w, selection, OL_XA_FILE_NAME(XtDisplay(w)),
         SelectionCB, (XtPointer)op, timestamp);

   return(True);

} /* end of DropNotify */
/*
 * ProtocolActionCB
 *
 */

static void
ProtocolActionCB(Widget w, Atom selection, OlDnDProtocolAction action, 
                 Boolean convert_not_fail, XtPointer closure)
{
        /* do nothing...        */
} /* end of ProtocolActionCB */
/*
 * InputCB
 *
 */

static void
InputCB(Widget w, XtPointer client_data, XEvent * event)
{
   MainWindow * mw = w ? FindMainWindow(w) : NULL;

   if (mw)
      SetMessage(mw, " ");

} /* end of InputCB */
