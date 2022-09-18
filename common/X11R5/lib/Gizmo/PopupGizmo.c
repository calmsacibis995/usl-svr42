/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:PopupGizmo.c	1.15"
#endif

/*
 * PopupGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>

#include <buffutil.h>
#include <textbuff.h>

#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#ifdef USE_TEW
#include <Xol/TextEdit.h>
#else
#include <Xol/StaticText.h>
#endif

#include "Gizmos.h"
#include "MenuGizmo.h"
#include "PopupGizmo.h"

static Widget    CreatePopupGizmo();
static Gizmo     CopyPopupGizmo();
static void      FreePopupGizmo();
static void      MapPopupGizmo();
static Gizmo     GetPopupGizmo();
static Gizmo     GetTheMenuGizmo();
static void      BuildPopupGizmo();
static void      ManipulatePopupGizmo();
static XtPointer QueryPopupGizmo();

GizmoClassRec PopupGizmoClass[] = 
   { 
   "PopupGizmo",
   CreatePopupGizmo,
   CopyPopupGizmo,
   FreePopupGizmo, 
   MapPopupGizmo,
   GetPopupGizmo,
   GetTheMenuGizmo,
   BuildPopupGizmo,
   ManipulatePopupGizmo,
   QueryPopupGizmo,
   };


/*
 * CopyPopupGizmo
 *
 * The \fICopyPopupGizmo\fP function is used to create a copy
 * of a given PopupGizmo \fIgizmo\fP.
 *
 * See also:
 *
 * FreePopupGizmo(3), CreatePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ...
 */

static Gizmo
CopyPopupGizmo(Gizmo gizmo)
{
   PopupGizmo * new = (PopupGizmo*)MALLOC(sizeof(PopupGizmo));
   PopupGizmo * old = (PopupGizmo*)gizmo;
   int          i;

   new->help       = old->help;
   new->name       = CSTRDUP(old->name);
   new->title      = CSTRDUP(old->title);
   new->menu       = CopyGizmo(MenuGizmoClass, old->menu);
   CopyGizmoArray(&new->gizmos, &new->num_gizmos, old->gizmos, old->num_gizmos);
   new->args       = old->args;
   new->num_args   = old->num_args;
   new->message    = NULL;
   new->shell      = NULL;

   return ((Gizmo)new);

} /* end of CopyPopupGizmo */
/*
 * FreePopupGizmo
 *
 * The \fIFreePopupGizmo\fP procedure is used free the PopupGizmo \fIgizmo\fP.
 *
 * See also:
 *
 * CopyPopupGizmo(3), CreatePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ...
 */

static void 
FreePopupGizmo(Gizmo gizmo)
{
   PopupGizmo * old = (PopupGizmo *)gizmo;
   int i;

   CFREE(old->name);
   CFREE(old->title);
   FreeGizmo(MenuGizmoClass, old->menu);
   FreeGizmoArray(old->gizmos, old->num_gizmos);
   FREE(old);

} /* end of FreePopupGizmo */
/*
 * CreatePopupGizmo
 *
 * The \fICreatePopupGizmo\fP function is used to create the Widget tree
 * defined by the PopupGizmo structure \fIp\fP.  \fIparent\fI is the
 * Widget parent of this new Widget tree.  \fIargs\fP and \fInum\fP,
 * if non-NULL, are used as Args in the creation of the popup window
 * Widget which is returned by this function.
 *
 * Standard Appearance:
 *
 * The \fICreatePopupGizmo\fP function creates a standard dialog
 * composed of a popupWindowShell widget containing widgets constructed
 * based on the MiscGizmos in the PopupGizmo structure in the
 * in the upperControlArea and a flatButtons Widget, constructed as 
 * a menu bar in the lowerControlArea.  A typical window appears as:~
 *
 * .BP /r4/richs/desktop/reqmnts/prop.ps 2.5i 6.0i
 * .EP
 *
 * See also:
 *
 * CopyPopupGizmo(3), FreePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ...
 */

static Widget
CreatePopupGizmo(Widget parent, PopupGizmo * gizmo, Arg * args, int num)
{
   int          i;
   Arg          arg[100];
   Cardinal     num_arg;
   Widget       menuArea;
   Widget       upperArea;
   Widget       shell;

   gizmo->shell = 
      XtCreatePopupShell(gizmo->name, popupWindowShellWidgetClass, parent, args, num);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->shell,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   XtAddCallback(gizmo->shell, XtNverify, DisallowGizmoPopdown, NULL);

   XtSetArg(arg[0], XtNtitle, GetGizmoText(gizmo->title));
   XtSetValues(gizmo->shell, arg, 1);

   XtSetArg(arg[0], XtNupperControlArea, &upperArea);
   XtSetArg(arg[1], XtNlowerControlArea, &menuArea);
   XtGetValues(gizmo->shell, arg, 2);

   CreateGizmoArray(upperArea, gizmo->gizmos, gizmo->num_gizmos);

   XtSetArg(arg[0], XtNlayoutType, OL_FIXEDROWS);
   XtSetArg(arg[1], XtNmeasure,    2);
   XtSetValues(menuArea, arg, 2);

   if (gizmo->menu != NULL)
   {
      CreateGizmo(menuArea, MenuBarGizmoClass, (Gizmo)gizmo->menu, NULL, 0);
   }

#ifdef USE_TEW
   XtSetArg(arg[0], XtNlinesVisible,  1);
   XtSetArg(arg[1], XtNsource,        "");
   XtSetArg(arg[2], XtNsourceType,    OL_STRING_SOURCE);
   XtSetArg(arg[3], XtNeditType,      OL_TEXT_READ);
   gizmo->message =
      XtCreateManagedWidget("_X_", textEditWidgetClass, menuArea, arg, 4);
#else
   XtSetArg(arg[0], XtNstring,        "");
   gizmo->message =
      XtCreateManagedWidget("_X_", staticTextWidgetClass, menuArea, arg, 1);
#endif

   return (gizmo->shell);

} /* end of CreatePopupGizmo */
/*
 * BringDownPopup
 *
 * The \fIBringDownPopup\fP procedure is used to popdown a popup
 * shell Widget \fIwidget\fP.  This convenience routine checks the
 * pupshpin state to determine if the popup should be popped down.
 *
 * Private:
 *   The toolkit currently doesn't provide a routine for us to
 *   do this, so we've got to do it ourselves...
 *   Can this be done more efficiently???
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ...
 */

extern void
BringDownPopup(Widget wid)
{
   long pushpin_state = WMPushpinIsOut;

   if (XtIsRealized(wid)) 
   {
      GetWMPushpinState(XtDisplay(wid), XtWindow(wid), &pushpin_state);
      switch (pushpin_state) 
      {
         case WMPushpinIsIn:
            break;
         case WMPushpinIsOut:
         default:
            XtPopdown(wid);
            break;
      }
   }

} /* end of BringDownPopup */
/*
 * GetTheMenuGizmo
 *
 */
static Gizmo 
GetTheMenuGizmo(PopupGizmo * gizmo)
{

   return (gizmo->menu);

} /* end of GetTheMenuGizmo */
/*
 * GetPopupGizmoShell
 *
 */

extern Widget
GetPopupGizmoShell(PopupGizmo * gizmo)
{

   return (gizmo->shell);

} /* end of GetPopupGizmoShell */
/*
 * MapPopupGizmo
 *
 */

static void 
MapPopupGizmo(PopupGizmo * gizmo)
{
   Widget    shell = gizmo->shell;

   XtPopup(shell, XtGrabNone);
   XRaiseWindow(XtDisplay(shell), XtWindow(shell));

} /* end of MapPopupGizmo */
/*
 * GetPopupGizmo
 *
 */

static Gizmo 
GetPopupGizmo(gizmo, item)
PopupGizmo * gizmo;
int          item;
{

   return (gizmo->gizmos[item].gizmo);

} /* end of GetPopupGizmo */
/* 
 * BuildPopupGizmo
 *
 */

static void
BuildPopupGizmo()
{
   char * label;
   int    i = 0;
   char * rec_label;
   char * menu_name = STRDUP("menu_name");

   BuildGizmo(MenuGizmoClass);

   rec_label = STRDUP(SCAN(label));

   fprintf(stderr, "static GizmoRec %s[] =\n", rec_label); /* rec_var_name */
   fprintf(stderr, "   {\n");
   for (label = (char * )Scan(); *label != 0; label = (char * )Scan())
   {
      i++;
      fprintf(stdout, "      { %s, ", label);    /* gizmo class */
      fprintf(stdout, "%s },\n", SCAN(label));   /* gizmo ptr   */
   }
   fprintf(stderr, "   };\n");

   fprintf(stdout, "static PopupGizmo %s =", SCAN(label)); /* popup var name */
   fprintf(stdout, "   { \"%s\", ", SCAN(label));/* name        */
   fprintf(stdout, "%s, ", rec_label);           /* gizmo array */
   fprintf(stdout, "%d, ", i);                   /* num gizmos  */
   fprintf(stdout, "%s", SCAN(label));           /* menu name   */
   fprintf(stdout, "};\n");

   free(rec_label);
   
} /* end of BuildPopupGizmo */
/*
 * ManipulatePopupGizmo
 *
 */

static void
ManipulatePopupGizmo(PopupGizmo * gizmo, ManipulateOption option)
{
   GizmoArray   gp = gizmo->gizmos;
   int i;

   for (i = 0; i < gizmo->num_gizmos; i++)
   {
      ManipulateGizmo(gp[i].gizmo_class, gp[i].gizmo, option);
   }

} /* end of ManipulatePopupGizmo */
/*
 * QueryPopupGizmo
 *
 */

static XtPointer
QueryPopupGizmo(PopupGizmo * gizmo, int option, char * name)
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

} /* end of QueryPopupGizmo */
/*
 * SetPopupMessage
 *
 */

extern void
SetPopupMessage(PopupGizmo * gizmo, char * message)
{
   Arg arg[10];

#ifdef USE_TEW
   XtSetArg(arg[0], XtNsource,     message);
   XtSetArg(arg[1], XtNsourceType, OL_STRING_SOURCE);
   XtSetValues(gizmo->message, arg, 2);
#else
   XtSetArg(arg[0], XtNstring,
      ((message == NULL) || (*message == 0)) ? "" : message);
   XtSetValues(gizmo->message, arg, 1);
#endif

} /* end of SetPopupMessage */
