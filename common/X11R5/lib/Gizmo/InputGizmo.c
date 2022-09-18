/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:InputGizmo.c	1.16"
#endif

/*
 * InputGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>

#include "Gizmos.h"
#include "PopupGizmo.h"
#include "InputGizmo.h"

static Widget    CreateInput();
static Gizmo     CopyInput();
static void      FreeInput();
static void      BuildInput();
static void      ManipulateInput();
static XtPointer QueryInput();
static void      ManageInput();

GizmoClassRec InputGizmoClass[] = 
   { 
      "InputGizmo",
      CreateInput,     /* Create      */
      CopyInput,       /* Copy        */
      FreeInput,       /* Free        */
      NULL,            /* Map         */
      NULL,            /* Get         */
      NULL,            /* Get Menu    */
      BuildInput,      /* Build       */
      ManipulateInput, /* Manipulate  */
      QueryInput,      /* Query       */
      ManageInput,     /* Manage      */
   };


/*
 * CopyInput
 *
 */

static Gizmo
CopyInput(Gizmo gizmo)
{
   InputGizmo * old = (InputGizmo *)gizmo;
   InputGizmo * new = (InputGizmo *)MALLOC(sizeof(InputGizmo));

   new->help             = old->help;
   new->name             = CSTRDUP(old->name);
   new->caption          = CSTRDUP(old->caption);
   new->text             = CSTRDUP(old->text);
   new->settings         = (Setting *)MALLOC(sizeof(Setting));
   new->settings->initial_string = NULL;
   new->settings->initial_value  = NULL;
   new->settings->current_value  = NULL;
   new->settings->previous_value = NULL;
   new->settings->flag   = old->settings->flag;
   new->charsVisible     = old->charsVisible;
   new->verify           = old->verify;
   new->args             = old->args;
   new->num_args         = old->num_args;
   new->captionWidget    = NULL;
   new->textFieldWidget  = NULL;

   return (Gizmo)new;

} /* end of CopyInput */
/*
 * FreeInput
 *
 */

static void 
FreeInput(Gizmo gizmo)
{
   InputGizmo * old = (InputGizmo *)gizmo;

   CFREE(old->name);
   CFREE(old->caption);
   CFREE(old->text);
   if (old->settings->initial_string)
      FREE(old->settings->initial_string);
   if (old->settings->initial_value)
      FREE(old->settings->initial_value);
   if (old->settings->current_value)
      FREE(old->settings->current_value);
   if (old->settings->previous_value)
      FREE(old->settings->previous_value);
   FREE(old->settings);
   FREE(old);

} /* end of FreeInput */
/*
 * CreateInput
 *
 * Creates a popup caption.
 *
 * GetInputText() - returns the text from the captions text field.
 */

static Widget
CreateInput(Widget promptArea, InputGizmo * gizmo, ArgList args, int num_args)
{
   Arg      arg[100];
   Cardinal num_arg;
   char *   caption_name = gizmo->name ? gizmo->name : "caption";
   char *   text_name    = gizmo->name ? gizmo->name : "text_input";

   gizmo->settings->previous_value = STRDUP(gizmo->text);
   gizmo->settings->current_value = NULL;

   if (gizmo->caption == NULL)
       gizmo->captionWidget = promptArea;
   else
   {
       XtSetArg(arg[0], XtNlabel, GetGizmoText(gizmo->caption));
       gizmo->captionWidget = 
	   XtCreateManagedWidget(caption_name, captionWidgetClass,
				 promptArea, arg, 1);
   }

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->captionWidget,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   XtSetArg(arg[0], XtNcharsVisible, gizmo->charsVisible);
   XtSetArg(arg[1], XtNstring, gizmo->text);
   num_arg = AppendArgsToList(arg, 2, gizmo->args, gizmo->num_args);
      
   gizmo->textFieldWidget = 
      XtCreateManagedWidget(text_name, textFieldWidgetClass, 
         gizmo->captionWidget, arg, num_arg);

   if (gizmo->verify)
      XtAddCallback(gizmo->textFieldWidget, XtNverification, 
         gizmo->verify, (XtPointer)0);

   /* 
    * register help (FIX)
    */

   return (gizmo->captionWidget);

} /* end of CreateInput */
/*
 * GetInputText
 *
 * The \fIGetInputText\fP function is used to retrieve the text
 * stored in a InputGizmo which is in the PopupGizmo \fIshell\fP.
 * The \fIitem\fP index is used to determine which of the MiscGizmos
 * in the \fIshell\fP is to be retrieved.
 *
 * See also:
 *
 * CreatePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 *#include <InputGizmo.h>
 * ...
 */

extern char *
GetInputText(PopupGizmo * shell, int item)
{
   InputGizmo * gizmo = (InputGizmo *)shell->gizmos[item].gizmo;
   char *       text;
   Arg          arg[1];

   XtSetArg(arg[0], XtNstring, &text);
   XtGetValues(gizmo->textFieldWidget, arg, 1);

   return (text);          /* Must be freed */

} /* end of GetInputText */
/*
 * SetInputText
 *
 * The \fISetInputText\fP procedure is used to replace the text
 * stored in a InputGizmo which is in the PopupGizmo \fIshell\fP.
 * The \fIitem\fP index is used to determine which of the MiscGizmos
 * in the \fIshell\fP is to be replaced with \fItext\fP.  If \fIselected\fP
 * is True, then the text is selected.
 *
 * See also:
 *
 * CreatePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 *#include <InputGizmo.h>
 * ...
 */

extern void
SetInputText(PopupGizmo * shell, int item, char * text, int selected)
{
   InputGizmo * gizmo = (InputGizmo *)shell->gizmos[item].gizmo;
   Arg          arg[10];
   Widget       textEditWidget;

#ifdef DEBUG
   (void)fprintf(stderr,"setting text to '%s'.\n", text);
#endif

   XtSetArg(arg[0], XtNstring,      text);
   XtSetValues(gizmo->textFieldWidget, arg, 1);
 
   if (selected)
   {
      XtSetArg(arg[0], XtNtextEditWidget, &textEditWidget);
      XtGetValues(gizmo->textFieldWidget, arg, 1);

      XtSetArg(arg[0], XtNselectStart, 0);
      XtSetArg(arg[1], XtNselectEnd,   strlen(text));
      XtSetValues(textEditWidget, arg, 2);

      OlSetInputFocus(textEditWidget, RevertToNone, CurrentTime);
   }

} /* end of SetInputText */
/*
 * BuildInput
 *
 */

static void
BuildInput()
{

   fprintf(stdout, "static InputGizmo %s = ", Scan());
   fprintf(stdout, "{ \"%s\", ", Scan());
   fprintf(stdout, "\"%s\", ", Scan()); 
   fprintf(stdout, "%s, ", Scan());
   fprintf(stdout, " %s };\n", Scan());

} /* end of BuildInput */
/*
 * ManipulateInput
 *
 */

static void
ManipulateInput(InputGizmo * gizmo, ManipulateOption option)
{
   Arg         arg[2];

   switch (option)
   {
   case GetGizmoValue:
      if (gizmo->settings->current_value != NULL)
         FREE(gizmo->settings->current_value);
      XtSetArg(arg[0], XtNstring, &gizmo->settings->current_value);
      XtGetValues(gizmo->textFieldWidget, arg, 1);
      break;
   case ApplyGizmoValue:
      if (gizmo->settings->previous_value != NULL)
         FREE(gizmo->settings->previous_value);
      gizmo->settings->previous_value = 
         STRDUP(gizmo->settings->current_value);
      break;
   case SetGizmoValue:
      if (gizmo->settings->previous_value != NULL)
         FREE(gizmo->settings->previous_value);
      gizmo->settings->previous_value = 
         STRDUP(gizmo->settings->current_value);
      break;
   case ResetGizmoValue:
      if (gizmo->settings->current_value != NULL)
         FREE(gizmo->settings->current_value);
      gizmo->settings->current_value = 
         STRDUP(gizmo->settings->previous_value);
      XtSetArg(arg[0], XtNstring, gizmo->settings->current_value);
      XtSetValues(gizmo->textFieldWidget, arg, 1);
      break;
   case ReinitializeGizmoValue:
      if (gizmo->settings->current_value != NULL)
         FREE(gizmo->settings->current_value);
      gizmo->settings->current_value = 
         STRDUP(gizmo->settings->initial_value);
      XtSetArg(arg[0], XtNstring, gizmo->settings->current_value);
      XtSetValues(gizmo->textFieldWidget, arg, 1);
      break;
   default:
      break;
   }

} /* end of ManipulateInput */

/*
 * QueryInput
 *
 */

static XtPointer
QueryInput(InputGizmo * gizmo, int option, char * name)
{
   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(gizmo->settings);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->textFieldWidget);
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

} /* end of QueryInput */
/*
 * ManageInput
 *
 */

static void
ManageInput(InputGizmo * gizmo, int option, char * name)
{
   Arg arg[5];

   if (name == NULL || strcmp(gizmo->name, name) == 0)
      switch(option)
      {
         case SensitizeGizmo:
            XtSetSensitive(gizmo->captionWidget, True);
            break;
         case DesensitizeGizmo:
            XtSetSensitive(gizmo->captionWidget, False);
            break;
         case UnhideGizmo:
#ifdef MANAGE
            XtManageChild(gizmo->captionWidget);
#else
            if (XtIsRealized(gizmo->captionWidget))
               XtMapWidget(gizmo->captionWidget);
            else
            {
               XtSetArg(arg[0], XtNmappedWhenManaged, True);
               XtSetValues(gizmo->captionWidget, arg, 1);
            }
#endif
            break;
         case HideGizmo:
#ifdef MANAGE
            XtUnmanageChild(gizmo->captionWidget);
#else
            if (XtIsRealized(gizmo->captionWidget))
               XtUnmapWidget(gizmo->captionWidget);
            else
            {
               XtSetArg(arg[0], XtNmappedWhenManaged, False);
               XtSetValues(gizmo->captionWidget, arg, 1);
            }
#endif
         default:
            break;
      }

} /* end of ManageInput */
