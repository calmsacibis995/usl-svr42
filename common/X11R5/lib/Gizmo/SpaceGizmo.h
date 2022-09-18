/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:SpaceGizmo.h	1.3"
#endif

#ifndef _SpaceGizmo_h
#define _SpaceGizmo_h

typedef struct _SpaceGizmo
   {
      Dimension height;        /* In millimeters */
      Dimension width;         /* In millimeters */
      Widget    rectObj;
   } SpaceGizmo;

extern GizmoClassRec SpaceGizmoClass[];

#endif /* _SpaceGizmo_h */
