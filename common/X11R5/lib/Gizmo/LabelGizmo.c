/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:LabelGizmo.c	1.18"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>

#include "Gizmos.h"
#include "LabelGizmo.h"

/* The code in this file produces a 
 * labeled container gizmo for an 
 * array of gizmos.
 */

static Gizmo     CopyLabelGizmo();
static void      FreeLabelGizmo();
static Widget    CreateLabelGizmo();
static Gizmo     GetLabelGizmo(LabelGizmo *, int);
static void      ManipulateLabelGizmo();
static XtPointer QueryLabelGizmo();

GizmoClassRec LabelGizmoClass[] =
   {
   "LabelGizmo",
   CreateLabelGizmo,     /* Create     */
   CopyLabelGizmo,       /* Copy       */
   FreeLabelGizmo,       /* Free       */
   NULL,                 /* Map        */
   GetLabelGizmo,        /* Get        */
   NULL,                 /* Get Menu   */
   NULL,                 /* Build      */
   ManipulateLabelGizmo, /* Manipulate */
   QueryLabelGizmo,      /* Query      */
   };

/*
 * CopyLabelGizmo
 *
 */

static Gizmo
CopyLabelGizmo(Gizmo gizmo)
{
   LabelGizmo * old = (LabelGizmo *)gizmo;
   LabelGizmo * new = (LabelGizmo *)MALLOC(sizeof (LabelGizmo));

   new->help          = old->help;
   new->caption       = old->caption ? CSTRDUP(old->caption) : NULL;
   new->name          = CSTRDUP(old->name);
   CopyGizmoArray(&new->gizmos, &new->num_gizmos, old->gizmos, old->num_gizmos);
   new->layoutType    = old->layoutType;
   new->measure       = old->measure;
   new->alignCaptions = old->alignCaptions;
   new->captionWidget = (Widget)0;
   new->controlWidget = (Widget)0;
   new->args          = old->args;
   new->num_args      = old->num_args;

   return ((Gizmo)new);

} /* end of CopyLabelGizmo */
/*
 * FreeLabelGizmo
 *
 */

static void
FreeLabelGizmo(Gizmo gizmo)
{
   LabelGizmo * old = (LabelGizmo *)gizmo;

   if (old->caption)
      CFREE(old->caption);
   FreeGizmoArray(old->gizmos, old->num_gizmos);
   FREE(old);

} /* end of FreeLabelGizmo */
/*
 * CreateLabelGizmo
 *
 */

static Widget
CreateLabelGizmo(Widget parent, LabelGizmo * gizmo, ArgList args, int num_args)
{
   int      i;
   Arg      arg[100];
   Cardinal num_arg;

   if (gizmo->caption == NULL)
   {
      gizmo->captionWidget = parent;
   }
   else
   {
      XtSetArg(arg[0], XtNlabel,     GetGizmoText(gizmo->caption));
      XtSetArg(arg[1], XtNalignment, OL_TOP);
      gizmo->captionWidget = 
         XtCreateManagedWidget("_X_", captionWidgetClass, parent, arg, 2);
   }

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->captionWidget,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   XtSetArg(arg[0], XtNlayoutType,      gizmo->layoutType);
   XtSetArg(arg[1], XtNmeasure,         gizmo->measure ? gizmo->measure : 1);
   XtSetArg(arg[2], XtNvPad,            0);
   XtSetArg(arg[3], XtNvSpace,          0);
   XtSetArg(arg[4], XtNalignCaptions,   gizmo->alignCaptions);
   XtSetArg(arg[5], XtNshadowThickness, 0);
   num_arg = AppendArgsToList(arg, 6, gizmo->args, gizmo->num_args);

   gizmo->controlWidget = 
      XtCreateManagedWidget("_X_", controlAreaWidgetClass, gizmo->captionWidget, arg, num_arg);

   CreateGizmoArray(gizmo->controlWidget, gizmo->gizmos, gizmo->num_gizmos);

   return (gizmo->captionWidget);

} /* end of CreatLabelGizmo */
/*
 * GetLabelGizmo
 *
 */

static Gizmo
GetLabelGizmo(LabelGizmo * gizmo, int item)
{

   return (gizmo->gizmos[item].gizmo);

} /* GetLabelGizmo */
/*
 * ManipulateLabelGizmo
 *
 */

static void
ManipulateLabelGizmo(LabelGizmo * gizmo, ManipulateOption option)
{
   GizmoArray gp = gizmo->gizmos;
   int        i;

   for (i = 0; i < gizmo->num_gizmos; i++)
   {
      ManipulateGizmo(gp[i].gizmo_class, gp[i].gizmo, option);
   }

} /* end of ManipulateLabelGizmo */
/*
 * QueryLabelGizmo
 *
 */

static XtPointer
QueryLabelGizmo(LabelGizmo * gizmo, int option, char * name)
{

   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoWidget:
            return (XtPointer)(gizmo->captionWidget);
            break;
         case GetGizmoGizmo:
            return (XtPointer)(gizmo);
            break;
         case GetGizmoSetting:
            /*
             * fall through
             */
         default:
            return (XtPointer)(NULL);
            break;
      }
   }
   else
   {
      return (QueryGizmoArray(gizmo->gizmos, gizmo->num_gizmos, option, name));
   }

} /* end of QueryLabelGizmo */
/*
 * GetControlWidget
 *
 */

extern Widget
GetControlWidget(LabelGizmo * gizmo)
{

   return (gizmo->controlWidget);

} /* end of GetControlWidget */
