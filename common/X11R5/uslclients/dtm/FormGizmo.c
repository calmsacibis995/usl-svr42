/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:FormGizmo.c	1.3"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/RubberTile.h>
#include <Gizmo/Gizmos.h>
#include "FormGizmo.h"

static Widget		CreateFormGizmo();
static void		FreeFormGizmo();
static Gizmo		CopyFormGizmo();
static XtPointer	QueryFormGizmo();

GizmoClassRec FormGizmoClass[] = {
	"FormGizmo",
	CreateFormGizmo,	/* Create	*/
	CopyFormGizmo,		/* Copy		*/
	FreeFormGizmo,		/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QueryFormGizmo		/* Query	*/
};

static Gizmo
CopyFormGizmo(gizmo)
FormGizmo *gizmo;
{
	FormGizmo *new = (FormGizmo *)MALLOC(sizeof(FormGizmo));

	new->name        = STRDUP(gizmo->name);
	new->orientation = gizmo->orientation;
	new->widget      = NULL;
	return (Gizmo)new;
}

static void
FreeFormGizmo(gizmo)
FormGizmo *gizmo;
{
	FREE(gizmo->name);
	FREE((void *)gizmo);
}

static Widget
CreateFormGizmo(parent, gizmo)
Widget		parent;
FormGizmo	*gizmo;
{
	Arg arg[1];

	XtSetArg(arg[0], XtNorientation, gizmo->orientation);
	return(gizmo->widget = XtCreateManagedWidget("formGizmo",
			 rubberTileWidgetClass, parent, arg, 1));
}

static XtPointer
QueryFormGizmo(FormGizmo *gizmo, int option, char * name)
{
   if (!name || !strcmp(name, gizmo->name)) {
      switch(option) {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->widget);
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

} /* end of QueryFormGizmo */

