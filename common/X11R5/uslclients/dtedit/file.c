/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:file.c	1.11"
#endif

/*
 * file.c
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
#include <MenuGizmo.h>
#include <BaseWGizmo.h>
#include <ModalGizmo.h>
#include <FileGizmo.h>

#include <editor.h>
#include <file.h>

/*
 #    #  ######  #    #
 ##   #  #       #    #
 # #  #  #####   #    #
 #  # #  #       # ## #
 #   ##  #       ##  ##
 #    #  ######  #    #
 */

/* no interface needed!!! */

/*
  ####   #####   ######  #    #
 #    #  #    #  #       ##   #
 #    #  #    #  #####   # #  #
 #    #  #####   #       #  # #
 #    #  #       #       #   ##
  ####   #       ######  #    #
 */


static void OpenPromptCB(Widget w, XtPointer client_data, XtPointer call_data);

typedef enum 
   {
      OpenPromptOpen,
      OpenPromptCancel,
      OpenPromptHelp
   } OpenMenuItemIndex;

static MenuItems  OpenMenuItems[] =
   {
      {True, TXT_OPEN,            MNE_OPEN    },
      {True, TXT_CANCEL,          MNE_CANCEL  },
      {True, TXT_HELPDDD,         MNE_HELPDDD },
      { 0 }
   };
static MenuGizmo  OpenMenu    = 
   { NULL, "filemenu", "_X_", OpenMenuItems, OpenPromptCB };

FileGizmo OpenPrompt =
   { &OpenWinHelp, "file", TXT_OPEN_TITLE, &OpenMenu, "", "", ".", FOLDERS_AND_FILES };

/*
  ####     ##    #    #  ######
 #        #  #   #    #  #
  ####   #    #  #    #  #####
      #  ######  #    #  #
 #    #  #    #   #  #   #
  ####   #    #    ##    ######
 */

static void OverwriteNoticeCB(Widget w, XtPointer client_data, XtPointer call_data);
static int  Save(Widget w, EditWindow * ew, char * filename, int force);
static void SaveNoticeCB(Widget w, XtPointer client_data, XtPointer call_data);
static void SavePromptCB(Widget w, XtPointer client_data, XtPointer call_data);

typedef enum 
   {
      SavePromptSave,
      SavePromptCancel,
      SavePromptHelp
   } SaveMenuItemIndex;

static MenuItems  SaveMenuItems[] =
   {
      {True, TXT_SAVE,      MNE_SAVE    },
      {True, TXT_CANCEL,    MNE_CANCEL  },
      {True, TXT_HELPDDD,   MNE_HELPDDD },
      { 0 }
   };
static MenuGizmo  SaveMenu    =
   { NULL, "savemenu", "_X_", SaveMenuItems, SavePromptCB };
static FileGizmo SavePrompt  = 
   { &SaveWinHelp, "saveprompt", TXT_SAVE_TITLE, &SaveMenu, "", "", ".", FOLDERS_ONLY };

typedef enum
   {
      SaveNoticeContinue,
      SaveNoticeHelp
   } SaveNoticeMenuItemIndex;
static MenuItems  SaveNoticeMenuItems[] =
   {
      {True, TXT_CONTINUE, MNE_CONTINUE },
      {True, TXT_HELPDDD,  MNE_HELPDDD  },
      { 0 }
   };
static MenuGizmo  SaveNoticeMenu = 
   { NULL, "savenoticemenu", "_X_", SaveNoticeMenuItems, SaveNoticeCB };
static ModalGizmo SaveNotice = 
   { &SNoteHelp, "savenotice", TXT_SAVE_TITLE, &SaveNoticeMenu, TXT_SAVE_NOTICE };

typedef enum
   {
      OverwriteNoticeOverwrite,
      OverwriteNoticeCancel,
      OverwriteNoticeHelp
   } OverwriteNoticeMenuItemIndex;
static MenuItems  OverwriteNoticeMenuItems[] =
   {
      {True, TXT_OVERWRITE, MNE_OVERWRITE },
      {True, TXT_CANCEL,    MNE_CANCEL    },
      {True, TXT_HELPDDD,   MNE_HELPDDD   },
      { 0 }
   };
static MenuGizmo  OverwriteNoticeMenu = 
   { NULL, "overwritenoticemenu", "_X_", OverwriteNoticeMenuItems, OverwriteNoticeCB };
static ModalGizmo OverwriteNotice = 
   { &ONoteHelp, "overwritenotice", TXT_OVERWRITE_TITLE, &OverwriteNoticeMenu, TXT_OVERWRITE_NOTICE };

/*
 * FileCB
 *
 */

extern void
FileCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);
   TextBuffer *     textBuffer = ew-> textBuffer;
   FILE *           fp;

#ifdef DEBUG
   SetMessage(ew, "File CB called!", 0);
#else
   SetMessage(ew, " ", 0);
#endif

   switch (p-> item_index)
   {
      case FileNew:
         CreateEditWindow(root, "", NULL);
         SetMessage(ew, " ", 0);
         /*
          * NOTE: relies on new window being added to the front of the list
          */
         SetMessage(filelist, TXT_NEW_DOC_MESSAGE, 0);
         break;
      case FileOpen:
         if (ew-> openPrompt == NULL)
         {
            ew-> openPrompt = CopyGizmo(FileGizmoClass, &OpenPrompt);
            CreateGizmo(w, FileGizmoClass, ew-> openPrompt, NULL, 0);
         }
         else
            SetFileCriteria(ew->openPrompt, NULL, "*");
         MapGizmo(FileGizmoClass, ew-> openPrompt);
         break;
      case FileSave:
      case FileSaveAs:
         if ((p-> item_index == FileSaveAs) || (ew-> status & UNTITLED_BUFFER))
         {
            if (ew-> savePrompt == NULL)
            {
               ew-> savePrompt = CopyGizmo(FileGizmoClass, &SavePrompt);
               CreateGizmo(w, FileGizmoClass, ew-> savePrompt, NULL, 0);
            }
            if (ew-> status & UNTITLED_BUFFER)
               SetFileCriteria(ew-> savePrompt, NULL, "");
            else
               SetFileCriteria(ew-> savePrompt, NULL, strrchr(ew-> filename, '/') + 1);
            MapGizmo(FileGizmoClass, ew-> savePrompt);
         }
         else
         {
            Save(w, ew, ew-> filename, False);
         }
         break;
      case FilePrint:
         while (XtPending ())
         {
            XEvent event;

            XtNextEvent (&event);
            XtDispatchEvent (&event);
         }

         fp = popen(PRINT_COMMAND, "w");
         if (fp == NULL)
            SetMessage(filelist, TXT_CANT_PRINT, 1);
         else
         {
            if (WriteTextBuffer(ew->textBuffer, fp) == WRITE_FAILURE)
               SetMessage(filelist, TXT_NOTHING_TO_PRINT, 1);
            else
               SetMessage(filelist, TXT_DOC_PRINTED, 0);
            pclose(fp);
         }
         break;
      case FileExit:
         QuitCB(w, client_data, call_data);
         break;
      default:
         break;
   }

} /* end of FileCB */
/*
 * OpenPromptCB
 *
 */

static void
OpenPromptCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);
   Widget           shell      = (Widget)_OlGetShellOfWidget(w);
   int              n;
   int              flag;
   char *           filename;

   switch(p-> item_index)
   {
      case OpenPromptOpen:
         n = ExpandFileGizmoFilename(ew->openPrompt, &flag);
         if (flag && n == 1)
         {
            filename = GetFilePath(ew-> openPrompt);
            CreateEditWindow(root, filename, NULL);
            SetMessage(ew, " ", 0);
            /*
             * NOTE: relies on new window being added to the front of the list
             */
            SetMessage(filelist, TXT_OPENED_MESSAGE, 0);
            BringDownPopup(shell);
            FREE(filename);
            SetFileCriteria(ew->openPrompt, NULL, "*");
         }
         else
         {
#ifdef DEBUG
            fprintf(stderr, "more than one file (%d)...continuing\n", n);
#endif
         }
         break;
      case OpenPromptCancel:
         SetMessage(ew, TXT_OPEN_CANCEL, 0);
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         break;
      case OpenPromptHelp:
         PostGizmoHelp(GetBaseWindowShell(ew->baseWindow), &OpenWinHelp);
         SetMessage(ew, TXT_HELP_POSTED, 0);
         break;
      default:
         (void)fprintf(stderr, "at %d in %s", __LINE__, __FILE__);
         SetMessage(ew, "default in OpenPromptCB!", 0);
         break;
      }

} /* end of OpenPromptCB */
/*
 * SavePromptCB
 *
 */

static void
SavePromptCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);
   Widget           shell      = (Widget)_OlGetShellOfWidget(w);
   int              n;
   int              flag;
   char *           filename;

   switch(p-> item_index)
   {
      case SavePromptSave:
         /*
          * Message set in Save()
          */
         n = ExpandFileGizmoFilename(ew->savePrompt, &flag);
         if ((flag && n == 1) || n == 0)
         {
            filename = GetFilePath(ew-> savePrompt);
#ifdef DEBUG
            (void)fprintf(stderr,"save file to '%s'. flag = %d, n = %d\n", 
               filename, flag, n);
#endif
            if (Save(w, ew, filename, False) == 0)
            {
#ifdef DEBUG
               (void)fprintf(stderr, "error saving file '%s'\n", filename);
#endif
               SetFileGizmoMessage(ew-> savePrompt, TXT_NO_SAVE_MESSAGE);
            }
            else
            {
               BringDownPopup(shell);
            }
            FREE(filename);
         }
         break;
      case SavePromptCancel:
         SetMessage(ew, TXT_SAVE_CANCEL, 0);
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         break;
      case SavePromptHelp:
         PostGizmoHelp(GetBaseWindowShell(ew->baseWindow), &SaveWinHelp);
         SetMessage(ew, TXT_HELP_POSTED, 0);
         break;
      default:
         (void)fprintf(stderr, "at %d in %s", __LINE__, __FILE__);
         SetMessage(ew, "default in SavePromptCB!", 1);
         break;
   }

} /* end of SavePromptCB */
/*
 * Save
 *
 */

static int
Save(Widget w, EditWindow * ew, char * filename, int force)
{
   TextBuffer * textBuffer = ew-> textBuffer;

   if (strcmp(filename, ew-> filename) != 0 &&
       access(filename, 0) == 0 && 
       !(force)) /* overwriting a file */
   {
      if (ew-> overwriteNotice == NULL)
      {
         ew-> overwriteNotice = CopyGizmo(ModalGizmoClass, &OverwriteNotice);
         CreateGizmo(w, ModalGizmoClass, ew-> overwriteNotice, NULL, 0);
      }
      MapGizmo(ModalGizmoClass, ew-> overwriteNotice);
      SetMessage(ew, TXT_NEEDS_OVERWRITE, 0);
      return(0);
   }
   else
   {
      if (SaveTextBuffer(textBuffer, filename) == SAVE_FAILURE)
      {
         if (ew-> saveNotice == NULL)
         {
            ew-> saveNotice = CopyGizmo(ModalGizmoClass, &SaveNotice);
            CreateGizmo(w, ModalGizmoClass, ew-> saveNotice, NULL, 0);
         }
         MapGizmo(ModalGizmoClass, ew-> saveNotice);
         SetMessage(ew, TXT_CANT_SAVE, 1);
         return(0);
      }
      else
      {
         if ((ew-> textBuffer-> status != READWRITE) ||
             (strcmp(filename, ew-> filename) != 0))
            ChangeEditWindowName(ew, filename);
         SetMessage(ew, TXT_SAVED_MESSAGE, 0);
         return(1);
      }
   }

} /* end of Save */
/*
 * SaveNoticeCB
 *
 */

static void
SaveNoticeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);
   Widget           shell      = (Widget)_OlGetShellOfWidget(w);
   TextBuffer *     textBuffer = ew-> textBuffer;

   switch(p-> item_index)
   {
      case SaveNoticeContinue:
         SetMessage(ew, TXT_NO_SAVE_MESSAGE, 0);
         BringDownPopup(shell);
         break;
      case SaveNoticeHelp:
         PostGizmoHelp(GetBaseWindowShell(ew->baseWindow), &SNoteHelp);
         SetMessage(ew, TXT_HELP_POSTED, 0);
         break;
      default:
         (void)fprintf(stderr, "at %d in %s", __LINE__, __FILE__);
         SetMessage(ew, "save notice default called", 1);
         break;
   }

} /* end of SaveNoticeCB */
/*
 * OverwriteNoticeCB
 *
 */

static void
OverwriteNoticeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   EditWindow *     ew         = FindEditWindow(w);
   Widget           shell      = (Widget)_OlGetShellOfWidget(w);
   TextBuffer *     textBuffer = ew-> textBuffer;
   char *           filename;

   switch(p-> item_index)
   {
      case OverwriteNoticeOverwrite:
         BringDownPopup(shell);
         filename = GetFilePath(ew-> savePrompt);
         if (Save(w, ew, filename, True) == 1)
         {
            SetMessage(ew, TXT_OVER_MESSAGE, 0);
            BringDownPopup(((FileGizmo *)(ew-> savePrompt))->shell);
         }
         else
            SetMessage(ew, TXT_NO_OVER_MESSAGE, 0);
         FREE(filename);
         break;
      case OverwriteNoticeCancel:
         SetMessage(ew, TXT_OVER_CANCEL, 0);
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         break;
      case OverwriteNoticeHelp:
         PostGizmoHelp(GetBaseWindowShell(ew->baseWindow), &ONoteHelp);
         SetMessage(ew, TXT_HELP_POSTED, 0);
         break;
      default:
         (void)fprintf(stderr, "at %d in %s", __LINE__, __FILE__);
         SetMessage(ew, "overwrite notice default called", 1);
         break;
   }

} /* end of OverwriteNoticeCB */
