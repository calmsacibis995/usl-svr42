/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:TimeGizmo.h	1.2"
#endif

/*
 * TimeGizmo.h
 *
 */

#ifndef _TimeGizmo_h
#define _TimeGizmo_h

/*
 * TimeGizmo
 *
 * The \fITimeGizmo\fP is used to construct a text field input interface
 * element.  The text field is enclosed in a Caption Widget.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <TimeGizmo.h>
 * ... 
 */

typedef struct _TimeGizmo 
   {
   HelpInfo *      help;             /* help information                */
   char *          name;             /* name for the Widget             */
   char *          caption;          /* caption laebl (I18N)            */
   char *          text;             /* text                            */
   Setting *       settings;         /* settings                        */
   ArgList         args;             /* Args used to tune the textField */
   Cardinal        num_args;         /* number of Args                  */
   Widget          captionWidget;    /* (return) Caption Widget         */
   Widget          textFieldWidget;  /* (return) TextField Widget       */
   } TimeGizmo;

extern GizmoClassRec TimeGizmoClass[];

/*
 * FIX: needed ?
 */

extern char * FormatTime(char * time, int *hourp, int * minp);
extern char * GetTime(PopupGizmo *shell, int item);
extern void   SetTime(PopupGizmo *shell, int item, char *text, int selected);

#endif /* _TimeGizmo_h */
