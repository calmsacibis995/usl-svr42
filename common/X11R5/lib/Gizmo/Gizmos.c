/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:Gizmos.c	1.38"
#endif

/*
 * Gizmos.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h> 

#include <Desktop.h>

#include <Xol/OpenLook.h>
#include <Xol/BulletinBo.h>

#include "Gizmos.h"
#include "MenuGizmo.h"

static char *       AppName              = " ";
static char *       GizmoAppName         = "*";    /* for Xdefaults */
static const char * not_found            = "Message not found!!\n";

static char *       GetAppName();
static int          LookupHelpType(char * section);

extern char *       (*GetGizmoAppName)() = GetAppName;

extern void         exit(int status);

/*
 * GetAppName
 *
 * The \fIGetAppName\fP function returns the name of the application
 * registered using the InitializeGizmoClient or InitializeGizmos
 * routines
 *
 * See also:
 * 
 * InitializeGizmoClient(3), InitializeGizmos(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

static char *
GetAppName()
{

   return (GizmoAppName);

} /* end of GetAppName */
/*
 * GetGizmoText
 * 
 * The \fIGetGizmoText\fP function retrieves the localized text associated
 * with the given \fIlabel\fP.  This stoarge must not be freed.
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern char * 
GetGizmoText(char * label)
{
   char * p;
   char   c;

   if (label == NULL)
       return (not_found);

   for (p = label; *p; p++)
      if (*p == '\001')
      {
         c = *p;
         *p++ = 0;
         label = (char *)gettxt(label, p);
         *--p = c;
         break;
      }

   return (label);

} /* end of GetGizmoText */
/*
 * MessageReceived
 *
 */

static void
MessageReceived(Widget w, XtPointer client_data, XtPointer call_data)
{

   exit(0);

} /* end of MessageReceived */
/*
 * InitializeGizmoClient
 * 
 * The \fIInitializeGizmoClient\fP function is used to initialize
 * a gizmo client.  This initialization includes initialization
 * of Gizmo-specific data structures, initialization of any 
 * application properties, toolkit initialization, desktop
 * initialization.  It also, optionally, handles the client
 * locking and psuedo drop facilities.
 *
 * See also:
 * 
 * InitializeGizmos(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern Widget
InitializeGizmoClient(char * name, char * class, 
   char * app_name,
   GizmoClass gizmoClass, Gizmo gizmo,
   XrmOptionDescRec * options, Cardinal num_options, 
   int * argc, char * argv[],
   XtPointer other,
   XtPointer base, XtResourceList resources, Cardinal num_resources, 
   ArgList args, Cardinal num_args, char * resource_name,
   Boolean (*dropCB)(), XtPointer dropCD)
{

   Widget Shell;

   InitializeGizmos(name, app_name);

   if (gizmoClass)
      ManipulateGizmo(gizmoClass, gizmo, RegisterGizmoConverter);

   OlToolkitInitialize(argc, argv, other);

/* Notes from SAMC:
 *
 * You may want to use XtAppInitialize later on because of other
 * functionalities. When you do this, you may need to do changes
 * on your MainLoop, e.g., replace XtNextEvent with XtAppNextNext,
 * OR you need to replace XtMainLoop with XtAppMainLoop in
 * your applications (this may mean change this functional interface).
 */
   Shell = XtInitialize(name, class, options, num_options, argc, argv);

   if (resource_name)
   {
      Arg    arg[5];
      Atom   id;
      Window window;

      XtSetArg(arg[0], XtNmappedWhenManaged, False);
      XtSetValues(Shell, arg, 1);
      XtSetArg(arg[0], XtNwidth,  500);
      XtSetArg(arg[1], XtNheight, 500);
      XtCreateManagedWidget("filler", bulletinBoardWidgetClass, Shell, arg, 2);
      XtRealizeWidget(Shell);
      OlDnDRegisterDDI(Shell, OlDnDSitePreviewNone, dropCB, NULL, True, dropCD);

      if (window = DtSetAppId(XtDisplay(Shell), XtWindow(Shell), resource_name))
      {
         char **       files     = &argv[1];
         static char * nullarg[] = { "", NULL };
         int           cnt       = 10;
         
         if (*argc == 1)
            files = nullarg;

         while (cnt--)
         {
            if (DtNewDnDTransaction(Shell, files, 
                DT_B_SEND_EVENT | DT_B_STATIC_LIST,
                0, 0, CurrentTime, window, DT_MOVE_OP, 
                NULL, (XtCallbackProc)MessageReceived, NULL))
            {
#ifdef DEBUG
               printf("Someone already owns %s: message sent\n", resource_name);
#endif
               return (NULL);
            }
            else
               printf("Someone already owns %s: try again...\n", resource_name);
         }
         printf("Someone already owns %s: just continue.\n", resource_name);
      }
   }

   DtInitialize(Shell);

   XtGetApplicationResources
      (Shell, base, resources, num_resources, args, num_args);

   return (Shell);

} /* end of InitializeGizmoClient */
/*
 * InitializeResourceName
 *
 * This function is used to set the RESOURCE_NAME environment variable.
 * The value of this variable is used when an application does an
 * XtInitialize() in lieu of using the value in argv[0] (or "main" if
 * the arg list at that point in processing is empty).
 *
 * See also:
 * 
 * InitializeGizmos(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern char *
InitializeResourceName(char * x_app_name)
{
   char * resource_name;

   resource_name = 
      (char *)MALLOC(strlen(x_app_name) + sizeof("RESOURCE_NAME=") + 1);
   strcpy(resource_name, "RESOURCE_NAME=");
   strcat(resource_name, x_app_name);
   putenv(resource_name);

   return (resource_name);

} /* end of InitializeResourceName */
/*
 * InitializeGizmos
 * 
 * The \fIInitializeGizmos\fP procedure is used to initialize the
 * internal data structures used by the Gizmo library.  The routine
 * sets the memory debugging utility, based on the environment
 * variable MEMUTIL.  It also stores the application name in both
 * resource manager format and localized for interface construction.
 * Finally, the routine stores the localized application name for 
 * the Olit toolkit.
 *
 * See also:
 * 
 * InitializeGizmoClient(3), GetAppName(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void 
InitializeGizmos(char * x_app_name, char * app_name)
{
#ifdef MEMUTIL
   char * memutil = (char *)getenv("MEMUTIL");

   _SetMemutilDebug(memutil ? atoi(memutil) : 0);
#endif /* MEMUTIL */

   GizmoAppName = (char *)MALLOC(strlen(x_app_name) + 2);
   strcpy(GizmoAppName, x_app_name);
   strcat(GizmoAppName, "*");

   InitializeResourceName(x_app_name);

   AppName = GetGizmoText(app_name);
   _OlSetApplicationTitle(AppName);

} /* end of InitializeGizmos */
/*
 * GizmoMainLoop
 *
 */

extern void
GizmoMainLoop(void (*inputCB)(), XtPointer input_data, void (*otherCB)(), XtPointer other_data)
{
   extern XtAppContext _XtDefaultAppContext();

   XEvent       event;
   XtAppContext context = _XtDefaultAppContext();

   for (;;)
   {
      XtAppNextEvent(context, &event);

      if (inputCB && (event.type == ButtonPress || event.type == KeyPress))
         (*inputCB)(XtWindowToWidget(event.xany.display, event.xany.window),
           input_data, &event);
      else
         if (otherCB)
            (*otherCB)(XtWindowToWidget(event.xany.display, event.xany.window),
              other_data, &event);

      XtDispatchEvent(&event);
   }

} /* end of GizmoMainLoop */
/*
 * CopyGizmo
 * 
 * The \fICopyGizmo\fP function copies a given Gizmo \fIgizmo\fP structure 
 * into a newly allocated structure.  The copy is accomplished by calling
 * the \f(CWcopy_function\fP class function associated with the 
 * \fIgizmoClass\fP.
 *
 * See also:
 * 
 * FreeGizmo(3), CreateGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern Gizmo
CopyGizmo(GizmoClass gizmoClass, Gizmo gizmo)
{

   return ((gizmoClass->copy_function)(gizmo));

} /* end of CopyGizmo */
/*
 * FreeGizmo
 *
 * The \fIFreeGizmo\fP frees a given Gizmo \fIgizmo\fP structure.  The
 * deallocation is performed by calling the \f(CWfree_function\fP class
 * function associated with the \fIgizmoClass\fP.
 *
 * See also:
 * 
 * CopyGizmo(3), CreateGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void 
FreeGizmo(GizmoClass gizmoClass, Gizmo gizmo)
{

   if (gizmoClass->free_function)
      (gizmoClass->free_function)(gizmo);

} /* end of FreeGizmo */
/*
 * CreateGizmo
 *
 * The \fICreateGizmo\fP function is used to create the Widget tree
 * defined by the Gizmo structure \fIgizmo\fP. \fIparent\fP is the
 * Widget parent of this new Widget tree.  The creation is performed by
 * the \f(CWcreate_function\fP class procedure associated with the
 * \fIgizmoClass\fP.  The ArgList specified by \fIarg\fP and \fInum\fP
 * are appended to the ArgList in the \fIgizmo\fP.
 *
 * See also:
 * 
 * CopyGizmo(3), FreeGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern Widget
CreateGizmo (Widget parent, GizmoClass gizmoClass, Gizmo gizmo, ArgList arg, int num)
{

   return ((Widget)((gizmoClass->create_function)(parent, gizmo, arg, num)));

} /* end of CreateGizmo */
/*
 * MapGizmo
 *
 * The \fIMapGizmo\fP function is used to make the Widget tree associated
 * with the \fIgizmo\fP visible.  The \f(CWmap_function\fP assocaited with
 * the \fIgizmoClass\fP is called to do the actual map.  These maps are done
 * by calling either:~
 * .BL
 * .LI
 * XtRealizedWidget
 * .LI
 * XtPopup (with the appropriate XtGrabKind)
 * .LI
 * XtManageWidget
 * .LE
 * .P
 *
 * See also:
 * 
 * CopyGizmo(3), FreeGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
MapGizmo(GizmoClass gizmoClass, Gizmo gizmo)
{

   if (gizmoClass->map_function)
      (gizmoClass->map_function)(gizmo);

} /* end of MapGizmo */
/*
 * ManageGizmo
 * 
 * The \fIManageGizmo\fP procedure is used to effect the managed-state
 * of the given \fIgizmo\fP.  This is accomplished by calling
 * the \f(CWmanage_function\fP class function associated with the 
 * \fIgizmoClass\fP passing the manage \fIoption\fP and gizmo \fIname\fP.
 *
 * See also:
 * 
 * CreateGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
ManageGizmo(GizmoClass gizmoClass, Gizmo gizmo, ManageOption option, char * name)
{

   if (gizmoClass->manage_function)
      (gizmoClass->manage_function)(gizmo, option, name);

} /* end of ManageGizmo */
/*
 * GetGizmo
 * 
 * The \fIGetGizmo\fP function is used to retrieve the Gizmo, indexed
 * by \fIitem\fP for the given \fIshell\fP Gizmo.  This accomplished 
 * by calling the \f(CWget_function\fP class function associated with
 * the \fIgizmoClass\fP.
 *
 * See also:
 * 
 * CreateGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern Gizmo
GetGizmo(GizmoClass gizmoClass, void * shell, int item)
{

   if (gizmoClass->get_function)
      return (gizmoClass->get_function)(shell, item);
   else
      return (Gizmo *)0;

} /* end of GetGizmo */
/*
 * GetMenuGizmo
 *
 * The \fIGetMenuGizmo\fP function is used to retrieve the MenuGizmo
 * of the given \fIshell\fP Gizmo.  This accomplished by calling the 
 * \f(CWget_menu_function\fP class function associated with the 
 * \fIshell\fP's \fIgizmoClass\fP.
 *
 * See also:
 * 
 * CreateGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern MenuGizmo *
GetMenuGizmo(GizmoClass gizmoClass, void * shell)
{

   if (gizmoClass->get_menu_function)
      return (MenuGizmo *)(gizmoClass->get_menu_function)(shell);
   else
      return (MenuGizmo *)0;

} /* end of GetMenuGizmo */
/*
 * BuildGizmo
 *
 * The \fIBuildGizmo\fP function is used to construct a Gizmo by parsing
 * the standard input stream.  The \f(CWbuild_function\fP class function
 * associated with the \fIgizmoClass\fP is called to perform the actual
 * construction.
 *
 * See also:
 * 
 * CopyGizmo(3), FreeGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
BuildGizmo(GizmoClass gizmoClass)
{

   if (gizmoClass->build_function)
      (gizmoClass->build_function)();
   else
      fprintf(stderr, "can't build gizmo %s\n", gizmoClass->name);

} /* end of BuildGizmo */
/*
 * ManipulateGizmo
 * 
 * The \fIManipulateGizmo\fP procedure is used to perform various
 * operations on the Gizmo hierarchy eminating from the given \fIgizmo\fP.
 * Operations that can be performed are:
 *
 * .BL
 * .LI
 * GetGizmoValue to retrieve the current setting of the gizmos.
 * .LE
 *
 * See also:
 * 
 * QueryGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
ManipulateGizmo(GizmoClass gizmoClass, Gizmo gizmo, ManipulateOption option)
{
   static int recursing;

   if (!recursing && option == SetGizmoValue)
      InitializeResourceBuffer();

   recursing++;

   if (gizmoClass->manipulate_function)
      (gizmoClass->manipulate_function)(gizmo, option);

   recursing--;

   if (!recursing && option == SetGizmoValue)
   {
      Widget widget = (Widget)QueryGizmo(gizmoClass, gizmo, GetGizmoWidget, NULL);
      if (widget)
         SendResourceBuffer(XtDisplay(widget), XtWindow(widget), 0, AppName);
      else
         (void)fprintf
            (stderr, "Couldn't find widget for Gizmo %s\n", gizmoClass->name);
   }

} /* end of ManipulateGizmo */
/*
 * QueryGizmo
 * 
 * The \fIQueryGizmo\fP function is used to retrieve information
 * from the gizmo hierarchy headed by the given \fIgizmo\fP.
 *
 * See also:
 * 
 * ManipulateGizmo(3), QueryGizmoArray(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern XtPointer
QueryGizmo(GizmoClass gizmoClass, Gizmo gizmo, int option, char * name)
{

   if (gizmoClass->query_function)
      return ((gizmoClass->query_function)(gizmo, option, name));
   else
      return (NULL);

} /* end of QueryGizmo */
/*
 * QueryGizmoArray
 *
 * The \fIQueryGizmoArray\fP function is used to retrieve information
 * from the given \fIgizmos\fP.
 *
 * See also:
 * 
 * ManipulateGizmo(3), QueryGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern XtPointer
QueryGizmoArray(GizmoArray gizmos, int num_gizmos, int option, char * name)
{
   GizmoArray gp;
   XtPointer  value = NULL;

   if (num_gizmos > 0)
   {
      for (gp = gizmos; value == NULL && gp < &gizmos[num_gizmos]; gp++)
      {
         value = QueryGizmo(gp->gizmo_class, gp->gizmo, option, name);
      }
      return (value);
   }
   else
   {
      return(XtPointer)(NULL);
   }

} /* end of QueryGizmoArray */
/*
 * CopyGizmoArray
 *
 * The \fICopyGizmoArray\fP function copies a given GizmoArray \fIgizmos\fP 
 * structure into a newly allocated array.  The copy is accomplished by calling
 * the \f(CWCopyGizmo\fP function for each of the gizmos in the array.
 * the values are returned via the arguments.
 *
 * See also:
 * 
 * FreeGizmoArray(3), CopyGizmo(3), FreeGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
CopyGizmoArray(GizmoArray * new, int * num_new, GizmoArray old, int num_old)
{
   GizmoRec * p;
   GizmoRec * gp;

   if (num_old == 0)
   {
      *new = (GizmoArray)NULL;
      *num_new = 0;
   }
   else
   {
      p = (GizmoRec *)CALLOC(num_old, sizeof(GizmoRec));

      *new = (GizmoArray)p;
      *num_new = num_old;

      for (gp = old; gp < &old[num_old]; p++, gp++)
      {
         p->gizmo_class = gp->gizmo_class;
         p->gizmo       = CopyGizmo(gp->gizmo_class, gp->gizmo);
         p->args        = gp->args;
         p->num_args    = gp->num_args;
      }
   }

} /* end of CopyGizmoArray */
/*
 * FreeGizmoArray
 *
 * The \fIFreeGizmoArray\fP function frees a given GizmoArray \fIgizmos\fP.
 * The copy is accomplished by calling the \f(CWFreeGizmo\fP function for 
 * each of the gizmos in the array.
 *
 * See also:
 * 
 * CopyGizmoArray(3), CopyGizmo(3), FreeGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void 
FreeGizmoArray(GizmoArray old, int num_old)
{
   GizmoArray gp;

   if (num_old > 0)
   {
      for (gp = old; gp < &old[num_old]; gp++)
      {
         FreeGizmo(gp->gizmo_class, gp->gizmo);
      }
      FREE((void *)old);
   }

} /* end of FreeGizmoArray */
/*
 * CreateGizmoArray
 *
 * The \fICreateGizmoArray\fP function create the gizmos specified in
 * the given GizmoArray \fIgizmos\fP.  The Widget tree generated by this 
 * eminates for the \fIparent\fP Widget.  The creation is accomplished 
 * by calling the \f(CWCreateGizmo\fP function for each of the gizmos 
 * in the array.
 *
 * See also:
 * 
 * CopyGizmoArray(3), FreeGizmoArray(3), CreateGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
CreateGizmoArray(Widget parent, GizmoArray gizmos, int num_gizmos)
{
   GizmoRec * gp;

   for (gp = gizmos; gp < &gizmos[num_gizmos]; gp++)
   {
      CreateGizmo(parent, gp->gizmo_class, gp->gizmo, gp->args, gp->num_args);
   }

} /* end of CreateGizmoArray */
/*
 * AppendArgsToList
 *
 * The \fIAppendArgsToList\fP function is used to merge two arg lists
 * by simply appending the second \fIadditional\fP list to the \fIoriginal\fP
 * list.  Note that the routine presumes that the original list array is 
 * large enough to accommodate the addition.
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern Cardinal
AppendArgsToList(ArgList original, Cardinal num_original, ArgList additional, Cardinal num_additional)
{
   int i;
   int j;

   for (i = 0, j = num_original; i < num_additional; i++, j++)
      original[j] = additional[i];

   return (num_original + num_additional);

} /* end of AppendArgsToList */
/*
 * Scan
 *
 * The \fIScan\fP function scans the stdin stream to return the next
 * token.  This function is used by the \f(CWbuild_function\fPs of the
 * Gizmo library.
 *
 * See also:
 * 
 * BuildGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern char * 
Scan()
{
   static char buffer[256];
   int i = 0;
   int c;
   static char * whitespace = "\n\t ";
   static char * eof        = "~";

   for (c = getchar(); strchr(whitespace, c); c = getchar())
      ; /* loop */

   for (; c != EOF && !strchr(eof, c); buffer[i++] = c, c = getchar())
      ; /* loop */

   buffer[i] = 0;

   return (buffer);

} /* end of Scan */
/*
 * LookupHelpType
 * 
 * The \fILookupHelpType\fP function determines which desktop help type
 * is associated with the given \fIsection\fP.  A null-string or the 
 * string "TOC" indicates that the type is DT_TOC_HELP, the string
 * "HelpDesk" indicates that the type is DT_OPEN_HELPDESK, otherwise the
 * type is set to DT_SECTION_HELP.
 *
 * See also:
 * 
 * RegisterGizmoHelp(3), PostGizmoHelp(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

static int
LookupHelpType(char * section)
{
   if (*section == '\0')
      return (DT_TOC_HELP); /* backward compatibility */
   else
   {
      if (strcmp(section, "TOC") == 0)
         return (DT_TOC_HELP);
      else
         if (strcmp(section, "HelpDesk") == 0)
            return (DT_OPEN_HELPDESK);
         else
            return (DT_SECTION_HELP);
   }

} /* end of LookupHelpType */
/*
 * GizmoRegisterHelp
 * 
 * The \fIGizmoRegisterHelp\fP procedure localizes the title and
 * app_title members of the HelpInfo structure pointed to by
 * \fIhelp\fP.  It then registers this data structure with the
 * Olit toolkit.
 *
 * See also:
 * 
 * PostGizmoHelp(3), LookupHelpType(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
GizmoRegisterHelp(OlDefine type, Widget w, char * title, 
                  OlDefine src, HelpInfo * help)
{

   if (help->app_title != NULL)
      help->app_title = GetGizmoText(help->app_title);
   if (help->title != NULL)
      help->title = GetGizmoText(help->title);
   OlRegisterHelp(type, w, title, src, help);
   
} /* end of GizmoRegisterHelp */
/*
 * PostGizmoHelp
 * 
 * The \fIPostGizmoHelp\fP procedure is used to request the posting
 * of the help information for the given topic.  The HelpInfo
 * structure pointed to by \fIhelp\fP defines the topic.
 *
 * See also:
 * 
 * GizmoRegisterHelp(3), LookupHelpType(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
PostGizmoHelp(Widget w, HelpInfo * help)
{
   static DtDisplayHelpRequest req;

   req.rqtype         = DT_DISPLAY_HELP;
   req.serial         = 0;
   req.version        = 1;
   req.client         = XtWindow(w);
   req.nodename       = NULL;
   req.source_type    = LookupHelpType(help->section);
   req.app_name       = AppName;
   req.app_title      = GetGizmoText(help->app_title);
   req.title          = GetGizmoText(help->title);
   req.help_dir       = NULL;
   req.file_name      = help->filename;
   req.sect_tag       = help->section;

#ifdef DEBUG
   (void)fprintf(stderr,"posting '%s' file: %s section: '%s'\n", 
      help->title, help->filename,help->section ? help->section : "TOC");
#endif

   (void)DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(XtDisplay(w)),
      _HELP_QUEUE(XtDisplay(w)), XtWindow(w), (DtRequest *)&req);

} /* end of PostGizmoHelp */
/*
 * ConcatGizmoString
 * 
 * The \fIConcatGizmoString\fP function concatenates two strings
 * \fIs1\fP and \fIs2\fP into a new string returned via the argument
 * \fIr\fP.  If the value pointed to by \fIr\fP is non-NULL, then
 * this value is freed before the assignment of the new value takes
 * place.
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern char *
ConcatGizmoString(char ** r, char * s1, char * s2)
{
   char * s = (char *)MALLOC(strlen(s1) + strlen(s2) + 1);

   (void) strcpy(s, s1);
   (void) strcat(s, s2);

   if (*r)
      FREE(*r);

   *r = s;

   return (s);

} /* end of ConcatGizmoString */
/*
 * DisallowGizmoPopdown
 *
 * The \fIDisallowGizmoPopdown\fP procedure is a utility callback routine
 * which simply turns off the automatic popdown of popup shell widgets.
 *
 * Synopsis:
 *#include <Gizmos.h>
 * ...
 */

extern void
DisallowGizmoPopdown(Widget w, XtPointer client_data, XtPointer call_data)
{
Boolean * flag = (Boolean *)call_data;

if (flag)
   *flag = False;

} /* end of DisallowGizmoPopdown */
