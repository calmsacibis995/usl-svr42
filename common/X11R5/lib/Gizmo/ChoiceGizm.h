/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:ChoiceGizm.h	1.7"
#endif

/*
 * ChoiceGizmo.h
 *
 */

#ifndef _ChoiceGizmo_h
#define _ChoiceGizmo_h

/*
 * ChoiceGizmo
 *
 * The \fIChoiceGizmo\fP is used to construct a Choice interface
 * element.  Choices can be represented as either Checkbox, Non-exclusive
 * or Exclusive buttons.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <ChoiceGizmo.h>
 * ... 
 */

typedef struct _ChoiceGizmo
   {
   HelpInfo *      help;            /* help information                 */
   char *          name;            /* name for the Widget              */
   char *          caption;         /* caption label (I18N'd)           */
   Gizmo           menu;            /* MenuGizmo pointer                */
   Setting *       settings;        /* setting structure                */
   void            (*verify)();     /* verify function                  */
   ArgList         args;            /* Arg array for buttons Widget     */
   Cardinal        num_args;        /* number of Args                   */
   Widget          captionWidget;   /* (return) Caption Widget          */
   Widget          buttonsWidget;   /* (return) FButtons Widget         */
   Widget          previewWidget;   /* (return) preview for abbreviated */
   } ChoiceGizmo;

extern GizmoClassRec ChoiceGizmoClass[];
extern GizmoClassRec AbbrevChoiceGizmoClass[];

/*
 * FIX: needed ?
 */
extern Widget GetChoiceButtons();
extern void   SetPreview(Widget w, XtPointer client_data, XtPointer call_data);

#endif /* _ChoiceGizmo_h */
