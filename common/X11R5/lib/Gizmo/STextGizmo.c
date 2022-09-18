/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:STextGizmo.c	1.12"
#endif

/*
 * STextGizmo.c
 *
 */

#include <stdio.h> 

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>

#include "Gizmos.h"
#include "MenuGizmo.h"
#include "STextGizmo.h"

static Widget    CreateStaticTextGizmo();
static Gizmo     CopyStaticTextGizmo();
static void      FreeStaticTextGizmo();
static void      BuildStaticTextGizmo();
static void      ManipulateStaticTextGizmo();
static XtPointer QueryStaticTextGizmo();

GizmoClassRec StaticTextGizmoClass[] =
   {
   "StaticTextGizmo",
   CreateStaticTextGizmo,
   CopyStaticTextGizmo,
   FreeStaticTextGizmo, 
   NULL,
   NULL,
   NULL,
   BuildStaticTextGizmo,
   ManipulateStaticTextGizmo,
   QueryStaticTextGizmo,
   };


/*
 * CopyStaticTextGizmo
 *
 * The \fICopyStaticTextGizmo\fP function is used to create a copy
 * of a given StaticTextGizmo \fIms\fP.
 *
 * See also:
 *
 * FreeStaticTextGizmo(3), CreateStaticTextGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ...
 */

static Gizmo 
CopyStaticTextGizmo(gizmo)
StaticTextGizmo * gizmo;
{
   StaticTextGizmo * new = (StaticTextGizmo*)MALLOC(sizeof(StaticTextGizmo));

   new-> help    = gizmo-> help;
   new-> name    = CSTRDUP(gizmo->name);
   new-> text    = CSTRDUP(gizmo->text);
   new-> gravity = gizmo-> gravity;
   new-> font    = gizmo->font ? CSTRDUP(gizmo->font) : NULL;
   new-> widget  = NULL;

   return ((Gizmo)new);

} /* end of CopyStaticTextGizmo */
/*
 * FreeStaticTextGizmo
 *
 * The \fIFreeStaticTextGizmo\fP procedure is used free the StaticTextGizmo \fIms\fP.
 *
 * See also:
 *
 * CopyStaticTextGizmo(3), CreateStaticTextGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ...
 */

static void 
FreeStaticTextGizmo(gizmo)
StaticTextGizmo * gizmo;
{

   CFREE(gizmo-> name);
   CFREE(gizmo-> text);
   if (gizmo-> font)
      CFREE(gizmo-> font);
   FREE(gizmo);

} /* end of FreeStaticTextGizmo */
/*
 * CreateStaticTextGizmo
 *
 * The \fICreateStaticTextGizmo\fP function is used to create the Widget tree
 * defined by the StaticTextGizmo structure \fIp\fP.  \fIparent\fI is the
 * Widget parent of this new Widget tree.  \fIargs\fP and \fInum\fP,
 * if non-NULL, are used as Args in the creation of the popup window
 * Widget which is returned by this function.
 *
 * Standard Appearance:
 *
 * The \fICreateStaticTextGizmo\fP function creates a standard modal dialog
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
 * CopyStaticTextGizmo(3), FreeStaticTextGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ...
 */

static Widget
CreateStaticTextGizmo (parent, gizmo, args, num)
Widget parent;
StaticTextGizmo * gizmo;
char * args;
int num;
{
   Arg           arg[100];
   Cardinal      num_arg;
   XFontStruct * bigger_font;
   int           n;

/*
 * FIX: Merge the args!
 */
   if (gizmo->font != NULL)
   {
      XrmValue from;
      XrmValue to;
      from.addr = (caddr_t)gizmo->font;
      from.size = strlen(gizmo->font);
      to.addr   = (caddr_t)&bigger_font;
      to.size   = sizeof(XFontStruct *);
      XtConvertAndStore(parent, XtRString, &from, XtRFontStruct, &to);
      n = 3;
   }
   else
      n = 2;

   XtSetArg(arg[0], XtNstring,  GetGizmoText(gizmo->text));
   XtSetArg(arg[1], XtNgravity, gizmo->gravity);
   XtSetArg(arg[2], XtNfont,    bigger_font);
#ifdef HAS_ARGS
   num_arg = AppendArgsToList(arg, n, gizmo->args, gizmo->num_args);
#else
   num_arg = n;
#endif
   gizmo->widget = 
      XtCreateManagedWidget(gizmo->name, staticTextWidgetClass, parent, arg, num_arg);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->widget,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   return (gizmo->widget);

} /* end of CreateStaticTextGizmo */
/*
 * SetStaticTextGizmoMessage
 *
 */

extern void
SetStaticTextGizmoText(gizmo, text)
StaticTextGizmo * gizmo;
char * text;
{
   Arg arg[2];

   XtSetArg(arg[0], XtNstring, text);
   XtSetValues(gizmo->widget, arg, 1);

} /* end of SetStaticTextGizmoMessage */
/*
 * GetStaticTextGizmo
 *
 */

extern Widget
GetStaticTextGizmo(gizmo)
StaticTextGizmo * gizmo;
{

   return(gizmo->widget);

} /* end of GetStaticTextGizmo */
/*
 * BuildStaticTextGizmo
 *
 */

static void
BuildStaticTextGizmo()
{
   char * label;
   char * menu_name = strdup("menuName");

   BuildGizmo(MenuGizmoClass);
   fprintf(stdout, "static StaticTextGizmo %s =\n", Scan()); /* var_name */
   fprintf(stdout, "   {");
   fprintf(stdout, " \"%s\", ", SCAN(label));           /* name     */
   fprintf(stdout, "%s, ", SCAN(label));                /* string   */
   fprintf(stdout, "%s ", menu_name);                   /* string   */
   fprintf(stdout, "};\n");
   free(menu_name);

} /* end of BuildStaticTextGizmo */
/*
 * ManipulateStaticTextGizmo
 *
 */

static void
ManipulateStaticTextGizmo(gizmo, option)
Gizmo           gizmo;
ManipulateOption option;
{
} /* end of ManipulateStaticTextGizmo */
/*
 * QueryStaticTextGizmo
 *
 */

static XtPointer
QueryStaticTextGizmo(StaticTextGizmo * gizmo, int option, char * name)
{
   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->widget);
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
      return (XtPointer)(NULL);

} /* end of QueryStaticTextGizmo */
