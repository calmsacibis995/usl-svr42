/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:FormGizmo.h	1.2"
#endif

#ifndef _formgizmo_h
#define _formgizmo_h

typedef struct _FormGizmo {
	char		*name;
	OlDefine	orientation;
	Widget		widget;
} FormGizmo;

extern GizmoClassRec	FormGizmoClass[];

#endif /* _formgizmo_h */
