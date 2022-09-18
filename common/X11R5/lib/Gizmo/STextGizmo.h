/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:STextGizmo.h	1.6"
#endif

/*
 * STextGizmo.h
 *
 */

#ifndef _STextGizmo_h
#define _STextGizmo_h

/*
 * StaticTextGizmo
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ... 
 */

typedef struct _StaticTextGizmo
   {
   HelpInfo *  help;         /* help information              */
   char *      name;         /* name of the widget            */
   char *      text;         /* text string                   */
   OlDefine    gravity;      /* text gravity                  */
   char *      font;         /* text font                     */
   Widget      widget;       /* static text widget (returned) */
   } StaticTextGizmo;

extern GizmoClassRec StaticTextGizmoClass[];

#endif /* _STextGizmo_h */
