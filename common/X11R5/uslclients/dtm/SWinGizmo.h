/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:SWinGizmo.h	1.1"
#endif

#ifndef _swingizmo_h
#define _swingizmo_h

typedef struct _SWinGizmo {
	char		*name;
	Widget		widget;
} SWinGizmo;

extern GizmoClassRec	SWinGizmoClass[];

#endif /* _swingizmo_h */
