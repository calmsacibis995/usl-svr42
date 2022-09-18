/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:SpaceGizmo.c	1.3"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/RectObj.h>

#include <Xol/OpenLook.h>

#include "Gizmos.h"
#include "SpaceGizmo.h"

/* The code in this file produces a space gizmo
 * the specify horizontal and vertical spacing betwix gizmos
 */

static Gizmo  CopySpaceGizmo(SpaceGizmo * g);
static void   FreeSpaceGizmo(SpaceGizmo * gizmo);
static Widget CreateSpaceGizmo(Widget parent, SpaceGizmo * g);

GizmoClassRec SpaceGizmoClass[] =
   {
      "SpaceGizmo",
      CreateSpaceGizmo, /* Create       */
      CopySpaceGizmo,   /* Copy         */
      FreeSpaceGizmo,   /* Free         */
      NULL,             /* Map          */
      NULL,             /* Get          */
      NULL,             /* Get Menu     */
      NULL,             /* Build        */
      NULL,             /* Manipulate   */
      NULL              /* Query        */
   };

/*
 * CopySpaceGizmo
 *
 */

static Gizmo
CopySpaceGizmo(SpaceGizmo * g)
{
   SpaceGizmo * new = (SpaceGizmo *)MALLOC(sizeof (SpaceGizmo));

   new->height  = g->height;
   new->width   = g->width;
   new->rectObj = (Widget)0;

   return (Gizmo)new;

} /* end of CopySpaceGizmo */
/*
 * FreeSpaceGizmo
 *
 */

static void
FreeSpaceGizmo (SpaceGizmo * gizmo)
{

   FREE (gizmo);

} /* end of FreeSpaceGizmo */
/*
 * CreateSpaceGizmo
 *
 */

static Widget
CreateSpaceGizmo (Widget parent, SpaceGizmo * g)
{
   Arg       arg[100];
   Cardinal  num_arg;
   Dimension height = OlMMToPixel (OL_VERTICAL, g->height);
   Dimension width  = OlMMToPixel (OL_HORIZONTAL, g->width);

   XtSetArg(arg[0], XtNheight, height);
   XtSetArg(arg[1], XtNwidth,  width);
   g->rectObj = XtCreateManagedWidget("rectObj", rectObjClass, parent, arg, 2);

   return g->rectObj;

} /* end of CreateSpaceGizmo */
