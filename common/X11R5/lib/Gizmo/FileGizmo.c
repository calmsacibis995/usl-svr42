/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:FileGizmo.c	1.16"
#endif

/*
 * FileGizmo.c
 *
 */

#include <libgen.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> 

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/ScrolledWi.h>
#include <Xol/FList.h>

#include <DtLock.h>

#include "Gizmos.h"
#include "MenuGizmo.h"
#include "FileGizmo.h"

#define VIEW_HEIGHT(g)      8
#define PARENT_DIRECTORY(g) "gizmo:1"  FS "Parent Folder"
#define PATH_CAPTION(g)     "gizmo:2"  FS "Path: "
#define FILE_CAPTION(g)     "gizmo:3"  FS "File: "
#define FILE_TITLE(g)       "gizmo:4"  FS "File(s)"
#define FOLDER_TITLE(g)     "gizmo:5"  FS "Folder(s)"
#define ALTFILE_CAPTION(g)  "gizmo:6"  FS "Folder:"
#define INVALID_PATH        "gizmo:7"  FS "Specified path is invalid."
#define IN_FOLDER           "gizmo:8"  FS "file(s) in folder..."
#define SO_MANY_MATCH       "gizmo:9"  FS "match criteria."
#define CANT_OPEN_DIR       "gizmo:10" FS "Can't open folder."
#define CANT_STAT_FILE      "gizmo:11" FS "Can't get information for file."
#define NAME_IS_NEW         "gizmo:12" FS "New file name."
#define NAME_NOT_UNIQUE     "gizmo:13" FS "File name is invalid."
#define NAME_IS_UNIQUE      "gizmo:14" FS "File name is valid."
#define ENTER_A_NAME        "gizmo:15" FS "Please enter a file name or path."

static char * listFields[] = { XtNlabel };
static char * star = "*";

static Gizmo      CopyFileGizmo(Gizmo gizmo);
static void       FreeFileGizmo(Gizmo gizmo);
static Widget     CreateFileGizmo (Widget parent, FileGizmo * gizmo, Arg * args, int num);
static Gizmo      GetTheMenuGizmo (FileGizmo * gizmo);
static void       MapFileGizmo(FileGizmo * gizmo);
static void       BuildFileGizmo();
static void       ManipulateFileGizmo(FileGizmo * gizmo, ManipulateOption option);
static XtPointer  QueryFileGizmo(FileGizmo * gizmo, int option, char * name);

static int        qstrcmp(const void * s1, const void * s2);
static Filelist * ReadDirectory(FileGizmo * gizmo, char * directory, char * re);
static int        RereadDirectory(FileGizmo * gizmo, char * re, int item);
static void       ListSelect(Widget w, XtPointer client_data, XtPointer call_data);
static void       ListDblSelect(Widget w, XtPointer client_data, XtPointer call_data);
static void       SelectTextFieldString(Widget textField);
static void       FixDirectory(FileGizmo * gizmo);
static void       FreeLists(FileGizmo * gizmo);
static void       FreeListArray(char ** list, int used);

GizmoClassRec FileGizmoClass[] =
   {
      "FileGizmo",
      CreateFileGizmo,     /* Create        */
      CopyFileGizmo,       /* Copy          */
      FreeFileGizmo,       /* Free          */
      MapFileGizmo,        /* Map           */
      NULL,                /* Get           */
      NULL,                /* Get Menu      */
      BuildFileGizmo,      /* Build         */
      ManipulateFileGizmo, /* Manipulate    */
      QueryFileGizmo,      /* Query         */
   };


/*
 * CopyFileGizmo
 *
 * The \fICopyFileGizmo\fP function is used to create a copy
 * of a given FileGizmo \fIgizmo\fP.
 *
 * See also:
 *
 * FreeFileGizmo(3), CreateFileGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <FileGizmo.h>
 * ...
 */

static Gizmo
CopyFileGizmo(Gizmo gizmo)
{
   FileGizmo * old = (FileGizmo *)gizmo;
   FileGizmo * new = (FileGizmo *)MALLOC(sizeof(FileGizmo));
   int i;

   new->help       = old->help;
   new->name       = CSTRDUP(old->name);
   new->title      = CSTRDUP(old->title);
   new->menu       = CopyGizmo(MenuGizmoClass, (Gizmo)(old->menu));
   CopyGizmoArray(&new->gizmos, &new->num_gizmos, old->gizmos, old->num_gizmos);   new->args       = old->args;
   new->num_args   = old->num_args;
   new->directory  = STRDUP(old->directory);
   new->path       = CSTRDUP(old->path);
   new->dialog_type = old->dialog_type;

   if (old->message)
      new->message = CSTRDUP(old->message);
   else
      new->message = NULL;

   new->controlWidget    = NULL;
   new->textFieldWidget  = NULL;
   new->staticTextWidget = NULL;
   new->subdirListWidget = NULL;
   new->fnameListWidget  = NULL;
   new->messageWidget    = NULL;
   new->shell            = NULL;

   return ((Gizmo)new);

} /* end of CopyFileGizmo */
/*
 * FreeFileGizmo
 *
 * The \fIFreeFileGizmo\fP procedure is used free the FileGizmo \fIgizmo\fP.
 *
 * See also:
 *
 * CopyFileGizmo(3), CreateFileGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <FileGizmo.h>
 * ...
 */

static void 
FreeFileGizmo(Gizmo gizmo)
{
   FileGizmo * old = (FileGizmo *)gizmo;

   CFREE(old->name);
   CFREE(old->title);
   if (old->message)
      CFREE(old->message);
   CFREE(old->directory);
   FREE(old->path);

   FreeGizmo(MenuGizmoClass, (Gizmo)(old->menu));
   FreeGizmoArray(old->gizmos, old->num_gizmos);
   FreeLists(old);

   FREE(gizmo);

} /* end of FreeFileGizmo */
/*
 * CreateFileGizmo
 *
 * The \fICreateFileGizmo\fP function is used to create the Widget tree
 * defined by the FileGizmo structure \fIp\fP.  \fIparent\fI is the
 * Widget parent of this new Widget tree.  \fIargs\fP and \fInum\fP,
 * if non-NULL, are used as Args in the creation of the popup window
 * Widget which is returned by this function.
 *
 * Standard Appearance:
 *
 * The \fICreateFileGizmo\fP function creates a standard modal dialog
 * (modalShell).  A staticText widget is created and its string
 * resource set.  The second child to the modalShell is a flatButtons
 * pased in which is constructed as a menu bar in the lowerArea.  A
 * typical window appears as:~
 *
 * .BP /r4/richs/desktop/reqmnts/notice2.ps 2.5i 6.0i
 * .EP
 *
 * See also:
 *
 * CopyFileGizmo(3), FreeFileGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <FileGizmo.h>
 * ...
 */

static Widget
CreateFileGizmo (Widget parent, FileGizmo * gizmo, Arg * args, int num)
{
   Arg           arg[100];
   Cardinal      num_arg;
   Widget        ctlarea;
   Widget        upperArea;
   Widget        lowerArea;
   Widget        caption;
   Widget        subdir;
   Widget        fname;
   Filelist *    list;
   char *        filecaption;
   char *        pathcaption;

   gizmo->shell = 
      XtCreatePopupShell(gizmo->name, popupWindowShellWidgetClass, parent, args, num);

   XtSetArg(arg[0], XtNtitle, GetGizmoText(gizmo->title));
   XtSetValues(gizmo->shell, arg, 1);

   XtAddCallback(gizmo->shell, XtNverify, DisallowGizmoPopdown, NULL);

   XtSetArg(arg[0], XtNupperControlArea, &upperArea);
   XtSetArg(arg[1], XtNlowerControlArea, &lowerArea);
   XtGetValues(gizmo->shell, arg, 2);

   XtSetArg(arg[0], XtNlayoutType, OL_FIXEDROWS);
   XtSetArg(arg[1], XtNmeasure,    2);
   XtSetValues(lowerArea, arg, 2);

   if (gizmo->menu != NULL)
      CreateGizmo(lowerArea, MenuBarGizmoClass, (Gizmo)(gizmo->menu), NULL, 0);

   XtSetArg(arg[0], XtNstring, gizmo->message ? GetGizmoText(gizmo->message) : "");
   gizmo->messageWidget =
      XtCreateManagedWidget("_X_", staticTextWidgetClass, lowerArea, arg, 1);

   if (gizmo->dialog_type != FOLDERS_ONLY && 
       gizmo->dialog_type != FOLDERS_AND_FILES)
      filecaption = GetGizmoText(ALTFILE_CAPTION(gizmo));
   else
      filecaption = GetGizmoText(FILE_CAPTION(gizmo));
   pathcaption = GetGizmoText(PATH_CAPTION(gizmo));

   FixDirectory(gizmo);

   list = gizmo->list = 
      ReadDirectory(gizmo, gizmo->directory, gizmo->path);

   XtSetArg(arg[0], XtNlabel,         pathcaption);
   caption = XtCreateManagedWidget("_X_", captionWidgetClass, upperArea, arg, 1);
   XtSetArg(arg[0], XtNstring, gizmo->directory);
   gizmo->staticTextWidget =
      XtCreateManagedWidget("_X_", staticTextWidgetClass, caption, arg, 1);

   XtSetArg(arg[0], XtNlabel,         filecaption);
   caption = XtCreateManagedWidget("_X_", captionWidgetClass, upperArea, arg, 1);
   XtSetArg(arg[0], XtNstring,        gizmo->path);
/*
 * FIX is '20' OK? or should this be a app supplied value?
 */
   XtSetArg(arg[1], XtNcharsVisible,  20);
   gizmo->textFieldWidget =
      XtCreateManagedWidget("_X_", textFieldWidgetClass, caption, arg, 2);

   SelectTextFieldString(gizmo->textFieldWidget);

   if (gizmo->dialog_type == FOLDERS_AND_FILES)
   {
      XtSetArg(arg[ 0], XtNmeasure,         2);
      XtSetArg(arg[ 1], XtNlayoutType,      OL_FIXEDCOLS);
      XtSetArg(arg[ 2], XtNshadowThickness, 0);
      ctlarea = XtCreateManagedWidget("_X_", controlAreaWidgetClass, upperArea, arg, 3);
   }
   else
   {
      ctlarea = upperArea;
   }

   XtSetArg(arg[0], XtNstring, GetGizmoText(FOLDER_TITLE(gizmo)));
   XtCreateManagedWidget("_X_", staticTextWidgetClass, ctlarea, arg, 1);

   if (gizmo->dialog_type == FOLDERS_AND_FILES)
   {
      XtSetArg(arg[0], XtNstring, GetGizmoText(FILE_TITLE(gizmo)));
      XtCreateManagedWidget("_X_", staticTextWidgetClass, ctlarea, arg, 1);
   }

   subdir = 
      XtCreateManagedWidget("_X_", scrolledWindowWidgetClass, ctlarea, arg, 0);
   XtSetArg(arg[ 0], XtNitems,         list->dirs);
   XtSetArg(arg[ 1], XtNnumItems,      list->dir_used);
   XtSetArg(arg[ 2], XtNexclusives,    True);
   XtSetArg(arg[ 3], XtNnoneSet,       True);
   XtSetArg(arg[ 4], XtNitemFields,    listFields);
   XtSetArg(arg[ 5], XtNnumItemFields, XtNumber(listFields));
   XtSetArg(arg[ 6], XtNselectProc,    ListSelect);
   XtSetArg(arg[ 7], XtNdblSelectProc, ListDblSelect);
   XtSetArg(arg[ 8], XtNviewHeight,    VIEW_HEIGHT(gizmo));
   XtSetArg(arg[ 9], XtNclientData,    gizmo);
   gizmo->subdirListWidget =
      XtCreateManagedWidget("_X_", flatListWidgetClass, subdir, arg, 10);

   if (gizmo->dialog_type == FOLDERS_AND_FILES)
   {
      fname = 
         XtCreateManagedWidget("_X_", scrolledWindowWidgetClass, ctlarea, arg, 0);
      XtSetArg(arg[ 0], XtNitems,         list->matchs);
      XtSetArg(arg[ 1], XtNnumItems,      list->match_used);
      XtSetArg(arg[ 2], XtNexclusives,    True);
      XtSetArg(arg[ 3], XtNnoneSet,       True);
      XtSetArg(arg[ 4], XtNitemFields,    listFields);
      XtSetArg(arg[ 5], XtNnumItemFields, XtNumber(listFields));
      XtSetArg(arg[ 6], XtNselectProc,    ListSelect);
      XtSetArg(arg[ 7], XtNdblSelectProc, ListDblSelect);
      XtSetArg(arg[ 8], XtNviewHeight,    VIEW_HEIGHT(gizmo));
      XtSetArg(arg[ 9], XtNclientData,    gizmo);
      gizmo->fnameListWidget =
         XtCreateManagedWidget("_X_", flatListWidgetClass, fname, arg, 10);
   }

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->shell,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   CreateGizmoArray(upperArea, gizmo->gizmos, gizmo->num_gizmos);

   return (gizmo->shell);

} /* end of CreateFileGizmo */
/*
 * GetTheMenuGizmo
 *
 */

static Gizmo
GetTheMenuGizmo (FileGizmo * gizmo)
{

   return (gizmo->menu);

} /* end of GetTheMenuGizmo */
/*
 * MapFileGizmo
 *
 */

static void
MapFileGizmo(FileGizmo * gizmo)
{
   Widget    shell = gizmo->shell;

   XtPopup(shell, XtGrabNone);
   XRaiseWindow(XtDisplay(shell), XtWindow(shell));

} /* end of MapFileGizmo */
/*
 * BuildFileGizmo
 *
 */

static void
BuildFileGizmo()
{
   char * label;
   char * menu_name = STRDUP("menuName");

   BuildGizmo(MenuGizmoClass);
   fprintf(stdout, "static FileGizmo %s =\n", Scan()); /* var_name */
   fprintf(stdout, "   {");
   fprintf(stdout, " \"%s\", ", SCAN(label));           /* name     */
   fprintf(stdout, "%s, ", SCAN(label));                /* string   */
   fprintf(stdout, "%s ", menu_name);                   /* string   */
   fprintf(stdout, "};\n");
   free(menu_name);

} /* end of BuildFileGizmo */
/*
 * ManipulateFileGizmo
 *
 */

static void
ManipulateFileGizmo(FileGizmo * gizmo, ManipulateOption option)
{
   GizmoArray   gp = gizmo->gizmos;
   int i;

   for (i = 0; i < gizmo->num_gizmos; i++)
   {
      ManipulateGizmo(gp[i].gizmo_class, gp[i].gizmo, option);
   }

} /* end of ManipulateFileGizmo */
/*
 * QueryFileGizmo
 *
 */

static XtPointer
QueryFileGizmo(FileGizmo * gizmo, int option, char * name)
{
   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->shell);
            break;
         case GetGizmoGizmo:
            return (XtPointer)(gizmo);
            break;
         default:
            return (XtPointer)(NULL);
            break;
      }
   }
   else
   {
      XtPointer value = QueryGizmo(MenuGizmoClass, gizmo->menu, option, name);
      if (value)
         return (value);
      else
      {
         return(QueryGizmoArray(gizmo->gizmos, gizmo->num_gizmos, option, name));
      }
   }

} /* end of QueryFileGizmo */
/*
 * qstrcmp
 *
 */

static int
qstrcmp(const void * s1, const void * s2)
{

   return (strcmp(*((char **)s1), *((char **)s2)));

} /* end of qstrcmp */
/*
 * ReadDirectory
 *
 */

static Filelist *
ReadDirectory(FileGizmo * gizmo, char * directory, char * re)
{
   Filelist *      list = NULL;
   DIR *           dp;
   struct dirent * dep;
   struct stat     stat_buffer;
   char *          old_directory;
   char            buffer[500];

   if ((dp = opendir(directory)) == NULL)
   {
      perror("opendir:");
      SetFileGizmoMessage(gizmo, CANT_OPEN_DIR);
      return (NULL);
   }
   else
   {
      old_directory = (char *)getcwd(NULL, FILENAME_MAX);
      chdir(directory);
      if (gizmo->directory)
         FREE(gizmo->directory);
      gizmo->directory = (char *)getcwd(NULL, FILENAME_MAX);
      list = (Filelist *)MALLOC(sizeof(Filelist));
      list->dir_size = 10;
      list->dirs = (char **)MALLOC(sizeof(char *) * list->dir_size);
      if (strcmp(gizmo->directory, "/") != 0)
      {
         list->dir_used = 1;
         list->dirs[0] = STRDUP(GetGizmoText(PARENT_DIRECTORY(gizmo)));
      }
      else
      {
         list->dir_used = 0;
      }
      list->match_size = 50;
      list->match_used = 0;
      list->matchs = (char **)MALLOC(sizeof(char *) * list->match_size);
      list->nomatch_size = 50;
      list->nomatch_used = 0;
      list->nomatchs = (char **)MALLOC(sizeof(char *) * list->nomatch_size);
      while ((dep = readdir(dp)) != NULL)
      {
         if ((strcmp(dep->d_name, ".") == 0) ||
             (strcmp(dep->d_name, "..") == 0))
            ;  /* skip current and parent directories */
         else
         {
            if (stat(dep->d_name, &stat_buffer) < 0)
            {
               /* skip files that we can't stat (FIX) */
               perror("stat:");
               SetFileGizmoMessage(gizmo, CANT_STAT_FILE);
            }
            else
            {
               if ((stat_buffer.st_mode & S_IFMT) == S_IFDIR)
               {
                  if (list->dir_used == list->dir_size)
                  {
                     list->dir_size *= 2;
                     list->dirs = (char **)
                        REALLOC((void *)list->dirs, sizeof(char *) * list->dir_size);
                  }
                  list->dirs[list->dir_used++] = STRDUP(dep->d_name);
               }
               else
               {
                  if (re == NULL || *re == NULL || gmatch(dep->d_name, re))
                  {
                     if (list->match_used == list->match_size)
                     {
                        list->match_size *= 2;
                        list->matchs = (char **)
                        REALLOC((void *)list->matchs, sizeof(char *) * list->match_size);
                     }
                     list->matchs[list->match_used++] = STRDUP(dep->d_name);
                  }
                  else
                  {
                     if (list->nomatch_used == list->nomatch_size)
                     {
                        list->nomatch_size *= 2;
                        list->nomatchs = (char **)
                        REALLOC((void *)list->nomatchs, sizeof(char *) * list->nomatch_size);
                     }
                     list->nomatchs[list->nomatch_used++] = STRDUP(dep->d_name);
                  }
               }
            }
         }
      }
      if (list->dir_used > 2)
         qsort((void *)&list->dirs[1], (size_t)list->dir_used - 1, sizeof(char *), qstrcmp);
      if (list->match_used > 1)
         qsort((void *)list->matchs, (size_t)list->match_used, sizeof(char *), qstrcmp);
      if (old_directory)
      {
         chdir(old_directory);
         free(old_directory);
      }

      if (gizmo->dialog_type == FOLDERS_AND_FILES)
      {
         sprintf(buffer, "%d %s %d %s", 
            list->match_used + list->nomatch_used,
            GetGizmoText(IN_FOLDER),
            list->match_used,
            GetGizmoText(SO_MANY_MATCH));
         SetFileGizmoMessage(gizmo, buffer);
      }
      else
      {
         if (re == NULL || *re == NULL || strcmp(re, star) == 0)
            SetFileGizmoMessage(gizmo, ENTER_A_NAME);
         else
            if (list->match_used == 0)
               SetFileGizmoMessage(gizmo, NAME_IS_NEW);
            else
               if (list->match_used == 1)
                  SetFileGizmoMessage(gizmo, NAME_IS_UNIQUE);
               else
                  SetFileGizmoMessage(gizmo, NAME_NOT_UNIQUE);
      }

      closedir(dp);

      return (list);
   }

} /* end of ReadDirectory */
/*
 * RereadDirectory
 *
 */

static int 
RereadDirectory(FileGizmo * gizmo, char * re, int item)
{
   Arg    arg[10];
      
   FreeLists(gizmo);

   gizmo->list = ReadDirectory(gizmo, gizmo->directory, re);

   if (gizmo->list)
   {

      SelectTextFieldString(gizmo->textFieldWidget);

      XtSetArg(arg[0], XtNstring, gizmo->directory);
      XtSetValues(gizmo->staticTextWidget, arg, 1);

      XtSetArg(arg[0], XtNitems,    gizmo->list->dirs);
      XtSetArg(arg[1], XtNnumItems, gizmo->list->dir_used);
      XtSetArg(arg[2], XtNviewHeight,    VIEW_HEIGHT(gizmo));
      XtSetValues(gizmo->subdirListWidget, arg, 3);

      if (gizmo->dialog_type == FOLDERS_AND_FILES)
      {
         XtSetArg(arg[0], XtNitems,    gizmo->list->matchs);
         XtSetArg(arg[1], XtNnumItems, gizmo->list->match_used);
         XtSetArg(arg[2], XtNviewHeight,    VIEW_HEIGHT(gizmo));
         XtSetValues(gizmo->fnameListWidget, arg, 3);
      }
   }

   return (gizmo->list != NULL);

} /* end of RereadDirectory */
/*
 * ListSelect
 *
 */

static void
ListSelect(Widget w, XtPointer client_data, XtPointer call_data)
{
   FileGizmo *      gizmo    = (FileGizmo *)client_data;
   OlFlatCallData * p        = (OlFlatCallData *)call_data;
   int              item     = p->item_index;
   Arg              arg[10];
   char *           re;
   char             copy[PATH_MAX];

   XtSetArg(arg[0], XtNstring, &re);
   XtGetValues(gizmo->textFieldWidget, arg, 1);

   if (gizmo->subdirListWidget == w)
   {
      strcpy(copy, gizmo->directory);
      if (item == 0) /* parent */
      {
         char * x = strrchr(gizmo->directory, '/');
         if (x == gizmo->directory)
         {
            *++x = '\0';
         }
         else
            *x = '\0';
      }
      else
      {
         gizmo->directory = 
            (char *)REALLOC(gizmo->directory, strlen(gizmo->directory) +
                            strlen(gizmo->list->dirs[item]) + 2);
         (void)strcat(gizmo->directory, "/");
         (void)strcat(gizmo->directory, gizmo->list->dirs[item]);
      }
      if (RereadDirectory(gizmo, re, item) == 0)
      {
         FREE(gizmo->directory);
         gizmo->directory = strdup(copy);
      }
      FREE(re);
   }
   else /* must be the filename list */
   {
      XtSetArg(arg[0], XtNselectStart,    0);
      XtSetArg(arg[1], XtNselectEnd,      0);
      XtSetArg(arg[2], XtNcursorPosition, 0);
      XtSetValues(gizmo->textFieldWidget, arg, 3);

      XtSetArg(arg[0], XtNstring, gizmo->list->matchs[item]);
      XtSetValues(gizmo->textFieldWidget, arg, 1);
      SelectTextFieldString(gizmo->textFieldWidget);
   }

} /* end of ListSelect */
/*
 * ListDblSelect
 *
 */

static void
ListDblSelect(Widget w, XtPointer client_data, XtPointer call_data)
{
   FileGizmo *      gizmo    = (FileGizmo *)client_data;
   Widget           c        = gizmo->menu->child;

   if (gizmo->subdirListWidget == w)
   {
      /*
       * do nothing for double clicks on the subdirs
       */
   }
   else
   {
      DtLockCursor(w, 1000L, NULL, NULL, GetOlBusyCursor(XtScreenOfObject(c)));
      OlActivateWidget(c, OL_SELECTKEY, (XtPointer)1);
   }

} /* end of ListDblSelect */
/*
 * SelectTextFieldString
 *
 */

static void
SelectTextFieldString(Widget textField)
{
   Widget textEdit;
   char * string;
   Arg    arg[5];

   XtSetArg(arg[0], XtNtextEditWidget, &textEdit);
   XtSetArg(arg[1], XtNstring,         &string);
   XtGetValues(textField, arg, 2);

   XtSetArg(arg[0], XtNselectStart,    0);
   XtSetArg(arg[1], XtNselectEnd,      strlen(string));
   XtSetArg(arg[2], XtNcursorPosition, arg[1].value);
   XtSetValues(textEdit, arg, 3);

   FREE(string);

} /* end of SelectTextFieldString */
/*
 * FixDirectory
 *
 */

static void
FixDirectory(FileGizmo * gizmo)
{
   char * current_dir = (char *)getcwd(NULL, FILENAME_MAX);
   char * directory;

   if (gizmo->directory == NULL || (strcmp(gizmo->directory, ".") == 0))
   {
      if (gizmo->directory)
         FREE(gizmo->directory);
      gizmo->directory = current_dir;
   }
   else
      if (*gizmo->directory != '/')
      {
         directory = (char *)
            MALLOC(strlen(current_dir) + strlen(gizmo->directory) + 2);
         (void)strcpy(directory, current_dir);
         (void)strcat(directory, "/");
         (void)strcat(directory, gizmo->directory);
         free(current_dir);
         FREE(gizmo->directory);
         gizmo->directory = directory;
      }

} /* end of FixDirectory */
/*
 * FreeLists
 *
 */

static void FreeLists(FileGizmo * gizmo)
{

   FreeListArray(gizmo->list->dirs, gizmo->list->dir_used);
   FreeListArray(gizmo->list->matchs, gizmo->list->match_used);
   FreeListArray(gizmo->list->nomatchs, gizmo->list->nomatch_used);
   FREE((void *)gizmo->list);

} /* end of FreeLists */
/*
 * FreeListArray
 *
 */

static void
FreeListArray(char ** list, int used)
{
   int i;

   for (i = 0; i < used; i++)
      FREE(list[i]);
   FREE((void *)list);

} /* end of FreeListArray */
/*
 * GetFileGizmoShell
 *
 */

extern Widget
GetFileGizmoShell(FileGizmo * gizmo)
{

   return (gizmo->shell);

} /* end of GetFileGizmoShell */
/*
 * SetFileGizmoMessage
 *
 * this facilitates setting the message string
 * in the StaticText widget which was created for the "Notice" case.
 *
 */

extern void
SetFileGizmoMessage(FileGizmo * gizmo, char * message)
{
   Arg arg[1];

   if (gizmo->messageWidget != NULL)
   {
      XtSetArg(arg[0], XtNstring, GetGizmoText(message));
      XtSetValues(gizmo->messageWidget, arg, 1);
   }

} /* end of SetFileGizmoMessage */
/*
 * ExpandFileGizmoFilename
 *
 */

extern int
ExpandFileGizmoFilename(FileGizmo * gizmo, int * flag)
{

   Arg         arg[5];
   char *      re;
   char *      old_directory = NULL;
   char *      basename;
   struct stat stat_buffer;
   int         stat_result;
   int         read_result;

   XtSetArg(arg[0], XtNstring, &re);
   XtGetValues(gizmo->textFieldWidget, arg, 1);

   if (*re == NULL)
   {
      basename = star;
   }
   else
   {
      basename = re;
      old_directory = (char *)getcwd(NULL, FILENAME_MAX);
      chdir(gizmo->directory);

      stat_result = stat(basename, &stat_buffer);
      if ((stat_result >= 0) && (stat_buffer.st_mode & S_IFMT) == S_IFDIR)
      {
         if (*basename == '/')
         {
            if (gizmo->directory)
               FREE(gizmo->directory);
            gizmo->directory = STRDUP(basename);
         }
         else
         {
            gizmo->directory = 
               REALLOC(gizmo->directory, strlen(gizmo->directory) + strlen(basename) + 2);
            strcat(gizmo->directory, "/");
            strcat(gizmo->directory, basename);
         }
         basename = star;
      }
      else
      {
         basename = strrchr(re, '/');
         if (basename)
         {
            *basename++ = 0;
            stat_result = stat(re, &stat_buffer);
            if ((stat_result < 0) || !(stat_buffer.st_mode & S_IFMT) == S_IFDIR)
            {
               SetFileGizmoMessage(gizmo, GetGizmoText(INVALID_PATH));
               *flag = 0;
               return(0);
            }
            if (*re == '/')
            {
               if (gizmo->directory)
                  FREE(gizmo->directory);
               gizmo->directory = STRDUP(re);
            }
            else
            {
               gizmo->directory = 
                  REALLOC(gizmo->directory, strlen(gizmo->directory) + strlen(re) + 2);
               strcat(gizmo->directory, "/");
               strcat(gizmo->directory, re);
            }
            if (*basename == NULL)
               basename = star;
         }
         else
         {
            basename = re;
         }
      }
   }
   XtSetArg(arg[0], XtNselectStart,    0);
   XtSetArg(arg[1], XtNselectEnd,      0);
   XtSetArg(arg[2], XtNcursorPosition, 0);
   XtSetValues(gizmo->textFieldWidget, arg, 3);

   XtSetArg(arg[0], XtNstring, basename == star ? "" : basename);
   XtSetValues(gizmo->textFieldWidget, arg, 1);

   read_result = RereadDirectory(gizmo, basename, 0);
   FREE(re);
   if (old_directory)
   {
      chdir(old_directory);
      free(old_directory);
   }
   if (read_result)
   {
      *flag = (gizmo->list->match_used == 1) &&
              (strcmp(basename, gizmo->list->matchs[0]) == 0);
      return (gizmo->list->match_used);
   }
   else
   {
      *flag = 0;
      return (0);
   }

} /* end of ExpandFileGizmoFilename */
/*
 * GetFilePath
 *
 */

extern char *
GetFilePath(FileGizmo * gizmo)
{
   Arg    arg[5];
   char * string;
   char * path;

   XtSetArg(arg[0], XtNstring,         &string);
   XtGetValues(gizmo->textFieldWidget, arg, 1);

   path = MALLOC(strlen(gizmo->directory) + strlen(string) + 2);
   strcpy(path, gizmo->directory);
   strcat(path, "/");
   strcat(path, string);

   FREE(string);

   return (path);  /* must be freed */
   
} /* end of GetFilePath */
/*
 * SetFileCriteria
 *
 */

extern void
SetFileCriteria(FileGizmo * gizmo, char * directory, char * path)
{
   Arg arg[10];

   if (directory)
   {
      if (gizmo->directory)
         FREE(gizmo->directory);
      gizmo->directory = STRDUP(directory);
      FixDirectory(gizmo);
   }

   if (path == NULL || *path == 0)
      path = star;
   XtSetArg(arg[0], XtNselectStart,    0);
   XtSetArg(arg[1], XtNselectEnd,      0);
   XtSetArg(arg[2], XtNcursorPosition, 0);
   XtSetValues(gizmo->textFieldWidget, arg, 3);

   XtSetArg(arg[0], XtNstring, path == star ? "" : path);
   XtSetValues(gizmo->textFieldWidget, arg, 1);

   (void)RereadDirectory(gizmo, path, 0);

} /* end of SetFileCriteria */
