/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:InputGizmo.h	1.6"
#endif

/*
 * InputGizmo.h
 *
 */

#ifndef _InputGizmo_h
#define _InputGizmo_h

/*
 * InputGizmo
 *
 * The \fIInputGizmo\fP is used to construct a text field input interface
 * element.  The text field is enclosed in a Caption Widget.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <InputGizmo.h>
 * ... 
 */

typedef struct _InputGizmo 
   {
   HelpInfo *      help;            /* help information                */
   char *          name;            /* name for the Widget             */
   char *          caption;         /* caption laebl (I18N)            */
   char *          text;            /* text                            */
   Setting *       settings;        /* settings                        */
   int             charsVisible;    /* number of chars to show         */
   void            (*verify)();     /* verify function                 */
   ArgList         args;            /* Args used to tune the textField */
   Cardinal        num_args;        /* number of Args                  */
   Widget          captionWidget;   /* (return) Caption Widget         */
   Widget          textFieldWidget; /* (return) TextField Widget       */
   } InputGizmo;

extern GizmoClassRec InputGizmoClass[];

/*
 * FIX: needed ?
 */

extern char * GetInputText(PopupGizmo *shell, int item);
extern void   SetInputText(PopupGizmo *shell, int item, char *text, int selected);

#endif /* _InputGizmo_h */
