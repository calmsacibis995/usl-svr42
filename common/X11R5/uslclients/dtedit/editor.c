/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:editor.c	1.25"
#endif

/*
 * editor.c
 *
 */

#undef PLACE_AT_POINTER

#define DROP_RESOURCE    "dtedit"

#include <stdio.h>
#include <fcntl.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <buffutil.h>
#include <textbuff.h>

#include <OlDnDVCX.h>
#include <DtI.h>

#include <OpenLook.h>
#include <Form.h>
#include <ScrolledWi.h>
#include <TextEdit.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <ModalGizmo.h>
#include <BaseWGizmo.h>

#define ClientName           "dtedit"
#define ClientClass          "dtedit"

#include <editor.h>
#include <menu.h>
#include <prop.h>
#include <Margin.h>

extern EditWindow *  filelist = NULL;
extern Widget        root     = NULL;
#ifdef TXT_FIXED_FONT
extern XFontStruct * textfont    = NULL;
#endif

extern HelpInfo PropWinHelp =
   { FormalClientName, TXT_PROP_HELP_TITLE,   HELPPATH, TXT_PROP_HELP_SECT };
extern HelpInfo OpenWinHelp =
   { FormalClientName, TXT_OPEN_HELP_TITLE,   HELPPATH, TXT_OPEN_HELP_SECT };
extern HelpInfo SaveWinHelp =
   { FormalClientName, TXT_SAVE_HELP_TITLE,   HELPPATH, TXT_SAVE_HELP_SECT };
extern HelpInfo SNoteHelp =
   { FormalClientName, TXT_SNOTE_HELP_TITLE,  HELPPATH, TXT_SNOTE_HELP_SECT };
extern HelpInfo ONoteHelp =
   { FormalClientName, TXT_ONOTE_HELP_TITLE,  HELPPATH, TXT_ONOTE_HELP_SECT };
extern HelpInfo ENoteHelp =
   { FormalClientName, TXT_ENOTE_HELP_TITLE,  HELPPATH, TXT_ENOTE_HELP_SECT };

extern HelpInfo ApplicationHelp =
   { FormalClientName, TXT_MAIN_HELP_TITLE,  HELPPATH, TXT_MAIN_HELP_SECT };
extern HelpInfo TOCHelp  =
   { FormalClientName, TXT_TOC_HELP_TITLE,   HELPPATH, "TOC" };
extern HelpInfo HelpDeskHelp =
   { FormalClientName, TXT_HELPDESK_TITLE,   HELPPATH, TXT_HELPDESK_SECT };

static Boolean      Warnings = False;
static int          CharsVisible = 80;
static int          LinesVisible = 24;

static char         WrapDefaultString[] = "word";
static char         NumbDefaultString[] = "none";

extern EditorSettings EditorSetting =
   {
   { WrapDefaultString, (XtPointer)WrapWord },
   { NumbDefaultString, (XtPointer)NumbNone },
   };

extern char * print_command = DEFAULT_PRINT_COMMAND;

static XtResource resources[] =
   {
      { "warnings", "Warnings", XtRBoolean, sizeof(Boolean),
        (Cardinal) &Warnings, XtRBoolean, (XtPointer) &Warnings
      },
      { "charsVisible", "CharsVisible", XtRInt, sizeof(int),
        (Cardinal) &CharsVisible, XtRInt, (XtPointer) &CharsVisible
      },
      { "linesVisible", "LinesVisible", XtRInt, sizeof(int),
        (Cardinal) &LinesVisible, XtRInt, (XtPointer) &LinesVisible
      },
      { WRAP, WRAP, WRAP, sizeof(int),
        (Cardinal) &EditorSetting.wrap.previous_value,
         XtRString, (XtPointer)WrapDefaultString
      },
      { NUMB, NUMB, NUMB, sizeof(int),
        (Cardinal) &EditorSetting.numb.previous_value,
         XtRString, (XtPointer)NumbDefaultString
      },
      { "printCommand", "printCommand", XtRString, sizeof(char *),
        (Cardinal) &print_command, XtRString, DEFAULT_PRINT_COMMAND
      },
   };

static void         HandleWMProtocols(Widget w, XtPointer client_data, 
                                      XtPointer call_data);
static EditWindow * FindOpenBuffer(EditWindow * ew);
static Boolean      DropNotify (Widget w, Window win, Position x, Position y, 
                                Atom selection, Time timestamp, 
                                OlDnDDropSiteID drop_site_id, 
                                OlDnDTriggerOperation op, 
                                Boolean send_done, Boolean forwarded,
                                XtPointer closure);
static void         SelectionCB(Widget w, XtPointer client_data, XtPointer call_data);
static void         InputCB(Widget w, XtPointer client_data, XEvent * event);


main(argc, argv)
int    argc;
char * argv[];
{
   int i;

   root = InitializeGizmoClient(ClientName, ClientClass,
      FormalClientName,
      PopupGizmoClass, &PropertiesPrompt,
      NULL, 0,
      &argc, argv,
      NULL,
      NULL, resources, XtNumber(resources), NULL, 0, DROP_RESOURCE,
      DropNotify, NULL);

   /*
    * make a copy in case of dynamic update to resources
    */

   print_command = strdup(print_command);

   if (root)
   {
#ifdef TXT_FIXED_FONT
      XrmValue      from;
      XrmValue      to;
      
      from.addr = GetGizmoText(TXT_FIXED_FONT);
      from.size = strlen(from.addr);
      to.addr   = (caddr_t)&textfont;
      to.size   = sizeof(XFontStruct *);
      XtConvertAndStore(root, XtRString, &from, XtRFontStruct, &to);
#endif

      if (argc > 1)
      {
         for (i = 1; i < argc; i++)
            CreateEditWindow(root, argv[i], NULL);
      }
      else
         CreateEditWindow(root, "", NULL);
   }

   GizmoMainLoop(InputCB, NULL, NULL, NULL);

} /* end of main */
/*
 * SetMessage
 *
 */

extern void 
SetMessage(EditWindow * ew, char * message, int error)
{

#define NULL_MESSAGE NULL

   switch (error)
   {
      case ERROR_MESSAGE:
         SetBaseWindowMessage(ew-> baseWindow, GetGizmoText(message));
         SetBaseWindowStatus(ew-> baseWindow, NULL_MESSAGE);
         break;
      case STATUS_MESSAGE:
         SetBaseWindowMessage(ew-> baseWindow, NULL_MESSAGE);
         SetBaseWindowStatus(ew-> baseWindow, GetGizmoText(message));
         break;
      case CLEAR_MESSAGE:
         switch(ew->last_message)
         {
            case ERROR_MESSAGE: /* has an error message */
               SetBaseWindowMessage(ew-> baseWindow, NULL_MESSAGE);
               break;
            case STATUS_MESSAGE: /* has a status message */
               SetBaseWindowStatus(ew-> baseWindow, NULL_MESSAGE);
               break;
            default:
               break;
         }
         break;
      default:
         break;
   }

   ew->last_message = error;

} /* end of SetMessage */
/*
 * ChangeEditWindowName
 *
 */

extern void
ChangeEditWindowName(EditWindow * ew, char * filename)
{
/*
 * FIX for I18N
 */
   char * window_title_prefix = GetGizmoText(TXT_CLIENT_NAME);
   char * window_title;
   char * attribute;

   if (ew-> filename)
      FREE(ew-> filename);

   if (filename && filename[0])
   {
      ew-> status         &= ~UNTITLED_BUFFER;
      ew-> filename        = STRDUP(filename);

      if (access(ew-> filename, 4 | 2) == 0)
         ew-> textBuffer->status = READWRITE;
      else
         if (access(ew-> filename, 0) == 0)
            if (access(ew-> filename, 4) == 0)
               ew-> textBuffer-> status = READONLY;
            else
               ew-> textBuffer-> status = NOTOPEN;
         else
            ew-> textBuffer-> status = NEWFILE;


      switch(ew-> textBuffer-> status)
      {
         default:
            attribute = GetGizmoText(TXT_DEFAULT_STATUS);
            break;
         case READWRITE:
            attribute = GetGizmoText(TXT_READWRITE_STATUS);
            break;
         case READONLY:
            attribute = GetGizmoText(TXT_READONLY_STATUS);
            break;
         case NEWFILE:
            attribute = GetGizmoText(TXT_NEWFILE_STATUS);
            break;
      }

      window_title = 
         (char *)MALLOC(strlen(window_title_prefix) + strlen(filename) +
                        strlen(attribute) + 1);
      (void) strcpy(window_title, window_title_prefix);
      (void) strcat(window_title, filename);
      (void) strcat(window_title, attribute);
      SetBaseWindowTitle(ew-> baseWindow, window_title);
      SetMessage(ew, TXT_FILE_OPENED, 0);
      FREE(window_title);

/*
 * FIX need to sync the open and save dialogs (if they exist)
 */
   }
   else
   {
      ew-> status  |= UNTITLED_BUFFER;
      ew-> filename = strdup("");
      window_title = GetGizmoText(TXT_CLIENT_UNTITLED);
      SetBaseWindowTitle(ew-> baseWindow, window_title);
      SetMessage(ew, TXT_UNTITLED_BUFFER, 0);
/*
 * FIX need to clear the open and save dialogs (if they exist)
 */
   }

} /* end of ChangeEditWindowName */
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
 * CreateEditWindow
 *
 */

extern void 
CreateEditWindow(Widget root, char * filename, TextBuffer * textBuffer)
{
   Arg    arg[10];
   static XtCallbackRec protocolCB[] =
      {
         { HandleWMProtocols, NULL },
         { NULL }
      };
   static BaseWindowGizmo BaseWindow = 
      { 
      &ApplicationHelp, ClientName, TXT_CLIENT_UNTITLED, &MenuBar, 
      NULL, 0, TXT_ICON_NAME, "dtedit.48", " ", " ", 50
      };

   EditWindow * new = (EditWindow *) MALLOC(sizeof(EditWindow));

   new-> status             = 0;
   new-> next               = filelist;

   if (textBuffer == NULL)
      {
      if (stat(filename, &new-> stat_buffer) == -1)
      {
#ifdef DEBUG
         perror("editor:");
#endif
      }
      else
      {
         EditWindow * ew = FindOpenBuffer(new);
         /*
          * see if it's open in another window
          */
         if (ew)
         {
            textBuffer = ew-> textBuffer;
         }
         else
            textBuffer = ReadFileIntoTextBuffer(filename, NULL, NULL);
      }
      if (textBuffer == NULL)
         textBuffer = ReadStringIntoTextBuffer("", NULL, NULL);
      }

   new-> textBuffer         = textBuffer;
   new-> baseWindow         = CopyGizmo(BaseWindowGizmoClass, &BaseWindow);
   new-> filename           = NULL;
   new-> last_message       = CLEAR_MESSAGE;

   XtSetArg(arg[0], XtNwmProtocol, protocolCB);
   (void)CreateGizmo(root, BaseWindowGizmoClass, new-> baseWindow, arg, 1);

   ChangeEditWindowName(new, filename);

   /*
    * the text edit widget will size the scrolled window
    * the lines/chars visible should be set via preferences
    */

   XtSetArg(arg[0], XtNsource,        textBuffer);
   XtSetArg(arg[1], XtNsourceType,    OL_TEXT_BUFFER_SOURCE);
   XtSetArg(arg[2], XtNlinesVisible,  LinesVisible);
   XtSetArg(arg[3], XtNcharsVisible,  CharsVisible);
   XtSetArg(arg[4], XtNwrapMode,      
            DecodeWrapMode((int)EditorSetting.wrap.previous_value));
   XtSetArg(arg[5], XtNleftMargin,
            EditorSetting.numb.previous_value ? 50 : 10);
/* 
 * FIX: doesn't handle right margin line drawing
 */
#ifdef TXT_FIXED_FONT
   XtSetArg(arg[6], XtNfont,          textfont);
   new-> text = XtCreateManagedWidget("Text", textEditWidgetClass, 
      GetBaseWindowScroller(new-> baseWindow), arg, 7);
#else
   new-> text = XtCreateManagedWidget("Text", textEditWidgetClass, 
      GetBaseWindowScroller(new-> baseWindow), arg, 6);
#endif /* TXT_FIXED_FONT */

   if (EditorSetting.numb.previous_value)
      _OlRegisterTextLineNumberMargin(new->text);

   OlDnDRegisterDDI(new-> text, OlDnDSitePreviewNone, DropNotify,
      (OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL);
   OlDnDRegisterDDI(new-> baseWindow->icon_shell, OlDnDSitePreviewNone, DropNotify,
      (OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL);

   new-> openPrompt         = NULL;
   new-> savePrompt         = NULL;
   new-> overwriteNotice    = NULL;
   new-> saveNotice         = NULL;
   new-> exitNotice         = NULL;
   new-> propertiesPrompt   = NULL;
   new-> findPrompt         = NULL;

   filelist   = new;

   MapGizmo(BaseWindowGizmoClass, new-> baseWindow);

} /* end of CreateEditWindow */
/*
 * DestroyEditWindow
 *
 */

extern void
DestroyEditWindow(EditWindow * ew)
{
   EditWindow ** q;

   for (q = &filelist; *q != ew; q = &((*q)-> next))
      ; /* loop */

   if (*q == ew)
   {
      BaseWindowGizmo * gizmo = (BaseWindowGizmo *)(ew-> baseWindow);

      *q = ew-> next;
      XtDestroyWidget(gizmo-> icon_shell);
      XtDestroyWidget(gizmo-> shell);
      /*
       * FIX   - free other stuff that was allocated!!!
       */
      FREE((void *)ew);
   }
   else
      (void)fprintf(stderr, "couldn't find %x in window list!\n", ew);

   if (!filelist)
      exit(0);

} /* end of DestroyEditWindow */
/*
 * FindToplevel
 *
 */

extern EditWindow *
FindToplevel(Widget shell)
{
   EditWindow * p;

   for (p = filelist; p != NULL; p = p-> next)
      if (GetBaseWindowShell(p-> baseWindow) == shell)
         break;

   return (p);

} /* end of FindToplevel */
/*
 * FindEditWindow
 *
 */

extern EditWindow * 
FindEditWindow(Widget w)
{
   Widget       shell;
   EditWindow * p = NULL;

   for (shell = (Widget)_OlGetShellOfWidget(w); 
        shell != NULL && (p = FindToplevel(shell)) == NULL;
        shell = (Widget)_OlGetShellOfWidget(XtParent(shell)))
      ; /* loop */

   return (p);

} /* end of FindEditWindow */
/*
 * FindOpenBuffer
 *
 */

static EditWindow *
FindOpenBuffer(EditWindow * ew)
{
   EditWindow * p;

   for (p = filelist; p != NULL; p = p-> next)
      if (p->stat_buffer.st_dev == ew->stat_buffer.st_dev &&
          p->stat_buffer.st_ino == ew->stat_buffer.st_ino)
         break;

   return (p);

} /* end of FindOpenBuffer */
/*
 * SelectionCB
 *
 */

static void SelectionCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlDnDTriggerOperation op  = (OlDnDTriggerOperation)client_data;
   DtDnDInfoPtr          dip = (DtDnDInfoPtr)call_data;
   EditWindow *          mw  = w == root ? NULL : FindEditWindow(w);
   int                   i;
   char *                value;

   if (dip->error != 0)
   {
      dip->send_done = False;
      return;
   }
   else
      if (mw)
      {
         for (i = 0; i < dip->nitems; i++)
         {
            value = dip->files[i];
            if (op == OlDnDTriggerMoveOp)
            {
               CreateEditWindow(root, value, NULL);
            }
            else
            {
               MergeFileIntoTextEditWidget(value, mw->text);
            }
         }
      }
      else
      {
         for (i = 0; i < dip->nitems; i++)
         {
            value = dip->files[i];
            CreateEditWindow(root, value, NULL);
         }
      }

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
   EditWindow * mw     = w == root ? NULL : FindEditWindow(w);
   DtDnDInfoPtr dip;

#ifdef PLACE_AT_POINTER
   if (op == OlDnDTriggerCopyOp)
   {
      TextPosition new_pos = _PositionFromXY(mw-> text, x, y);

      if (new_pos > 0)
         OlTextEditSetCursorPosition(mw->text, new_pos, new_pos, new_pos);
   }
#endif /* PLACE_AT_POINTER */

   dip = DtGetFileNames(w, selection, timestamp, send_done, 
            SelectionCB, (XtPointer)op);

   if (dip)
   {
      if (dip->error)
      {
         _OlTextEditTriggerNotify(w, win, x, y, selection, timestamp,
            drop_site_id, op, send_done, forwarded, closure);
      }
      /*
       * FIX: free dip (but how?)
       */
   }

   return(True);

} /* end of DropNotify */
/*
 * MergeFileIntoTextBuffer
 *
 */

static int
MergeFileIntoTextEditWidget(char * filename, Widget w)
{

   int fd;
   struct stat stat_buffer;

   if (stat(filename, &stat_buffer) == -1)
      return (-1);
   else
   {
      if ((fd = open(filename, O_RDONLY | O_NDELAY, 0)) == -1)
         return (-1);
      else
      {
         char * buffer;
         char * p;
         int    amount = stat_buffer.st_size;
         int    nbytes;
         
         p = buffer = MALLOC(amount + 1);
         
         while(amount > 0)
         {
            nbytes = read(fd, p, amount);
            if (nbytes == -1)
            {
               FREE(buffer);
               close(fd);
               return (-1);
            }
            else
            {
               amount -= nbytes;
               p += nbytes;
            }
         }
         close(fd);
         buffer[stat_buffer.st_size] = '\0';
         if (OlTextEditInsert(w, buffer, stat_buffer.st_size) != 0)
            {
               FREE(buffer);
               return (-1);
            }
         else
            {
               FREE(buffer);
               return (0);
            }
      }
   }

} /* end of MergeFileIntoTextBuffer */
/*
 * InputCB
 *
 */

static void
InputCB(Widget w, XtPointer client_data, XEvent * event)
{
   EditWindow * ew = w ? FindEditWindow(w) : NULL;
  
   if (ew)
      SetMessage(ew, "", CLEAR_MESSAGE);

} /* end of InputCB */
