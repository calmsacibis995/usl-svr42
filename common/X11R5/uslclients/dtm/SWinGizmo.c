/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:SWinGizmo.c	1.2"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/RubberTile.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/Gizmos.h>
#include "SWinGizmo.h"

static Widget		CreateSWinGizmo();
static void		FreeSWinGizmo();
static Gizmo		CopySWinGizmo();
static XtPointer	QuerySWinGizmo();

GizmoClassRec SWinGizmoClass[] = {
	"SWinGizmo",
	CreateSWinGizmo,	/* Create	*/
	CopySWinGizmo,		/* Copy		*/
	FreeSWinGizmo,		/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QuerySWinGizmo		/* Query	*/
};

static Gizmo
CopySWinGizmo(gizmo)
SWinGizmo *gizmo;
{
	SWinGizmo *new = (SWinGizmo *)MALLOC(sizeof(SWinGizmo));

	new->name   = STRDUP(gizmo->name);
	new->widget = NULL;
	return((Gizmo)new);
}

static void
FreeSWinGizmo(gizmo)
SWinGizmo *gizmo;
{
	FREE(gizmo->name);
	FREE((void *)gizmo);
}

static Widget
CreateSWinGizmo(parent, gizmo)
Widget		parent;
SWinGizmo	*gizmo;
{
	Arg arg[1];
	int i = 0;

	if (XtIsSubclass(parent, rubberTileWidgetClass) != False) {
		XtSetArg(arg[0], XtNweight, 1);
		i++;
	}

	return(gizmo->widget = XtCreateManagedWidget("swinGizmo",
			 scrolledWindowWidgetClass, parent, arg, i));
}

static XtPointer
QuerySWinGizmo(SWinGizmo *gizmo, int option, char * name)
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

} /* end of QuerySWinGizmo */

