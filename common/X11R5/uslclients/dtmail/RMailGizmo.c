/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:RMailGizmo.c	1.14"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Error.h>
#include <Xol/RubberTile.h>
#include "mail.h"
#include "RMailGizmo.h"

static Widget		CreateReadMailGizmo();
static Gizmo		CopyReadMailGizmo();
static void		FreeReadMailGizmo();
static XtPointer	Query();

GizmoClassRec ReadMailGizmoClass[] = {
	"ReadMailGizmo",
	CreateReadMailGizmo,	/* Create	*/
	CopyReadMailGizmo,	/* Copy		*/
	FreeReadMailGizmo,	/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	Query			/* Query	*/
};

/* Private procedures */

static void
CreateHeaderArea (gizmo)
ReadMailGizmo *	gizmo;
{
	gizmo->headSw = XtVaCreateManagedWidget (
		"Scrolled Window",
		scrolledWindowWidgetClass,
		gizmo->rubberTile,
		XtNweight,	  0,
		XtNshadowThickness,0,
		(String)0
	);
	gizmo->headArea =
		XtVaCreateManagedWidget (
		"data",
		textEditWidgetClass,
		gizmo->headSw,
#ifdef DEBUG
		XtNcharsVisible,  40,
		XtNlinesVisible,  5,
#else
		XtNcharsVisible,  gizmo->width,
		XtNlinesVisible,  gizmo->head,
#endif
		XtNsourceType,	  OL_STRING_SOURCE,
		XtNeditType,	  OL_TEXT_READ,
		XtNsource,	  "",
		(String)0
	);
}

static void
CreateScrollingMailArea (gizmo)
ReadMailGizmo *gizmo;
{
	Widget sw;

	sw = XtVaCreateManagedWidget (
		"Scrolled Window",
		scrolledWindowWidgetClass,
		gizmo->rubberTile,
		XtNweight,	  1,
		XtNshadowThickness,0,
		(String)0
	);
	gizmo->bodyArea = XtVaCreateManagedWidget (
		"data",
		textEditWidgetClass,
		sw,
#ifdef DEBUG
		XtNcharsVisible,  40,
		XtNlinesVisible,  5,
#else
		XtNcharsVisible,  gizmo->width,
		XtNlinesVisible,  gizmo->body,
#endif
		XtNsourceType,	  OL_STRING_SOURCE,
		XtNeditType,	  OL_TEXT_READ,
		XtNsource,	  "",
		(String)0
	);
}

static Widget
CreateReadMailGizmo (parent, gizmo)
Widget		parent;
ReadMailGizmo *	gizmo;
{
	Arg arg[10];

	/* Create the popup shell to contain the mail message */

	gizmo->width = 80;
	gizmo->head = 5;
	gizmo->body = 20;
	gizmo->rubberTile = XtVaCreateManagedWidget (
		"text_tile",
		rubberTileWidgetClass,
		parent,
		XtNwidth,	gizmo->width,
		XtNshadowThickness,0,
		(String)0
	);
	CreateHeaderArea (gizmo);
	CreateScrollingMailArea (gizmo);

	return gizmo->rubberTile;
}

static Gizmo
CopyReadMailGizmo (gizmo)
ReadMailGizmo *	gizmo;
{
	ReadMailGizmo * new = (ReadMailGizmo *)MALLOC (sizeof (ReadMailGizmo));

	new->name = STRDUP (gizmo->name);
	new->width = gizmo->width;
	new->head = gizmo->head;
	new->body = gizmo->body;
	new->bodyArea = (Widget)0;
	new->headArea = (Widget)0;
	new->rubberTile = (Widget)0;
	new->headSw = (Widget)0;
	return (Gizmo)new;
}

static void
FreeReadMailGizmo (gizmo)
ReadMailGizmo *	gizmo;
{
	FREE (gizmo->name);
	FREE (gizmo);
}

/* Public procedures */

void
DisplayMailText (gizmo, head, body, type)
ReadMailGizmo *	gizmo;
XtArgVal	head;
XtArgVal	body;
OlSourceType	type;
{

	XtVaSetValues (
		gizmo->bodyArea,
		XtNsourceType,		type,
		XtNsource,		body,
		XtNdisplayPosition,	0,
		XtNcursorPosition,	0,
		(String)0
	);

	XtVaSetValues (
		gizmo->headArea,
		XtNsourceType,		type,
		XtNsource,		head,
		XtNdisplayPosition,	0,
		XtNcursorPosition,	0,
		(String)0
	);
}

Widget
GetReadGizmoBody (gizmo)
ReadMailGizmo *	gizmo;
{
	return gizmo->bodyArea;
}

Widget
GetReadGizmoHead (gizmo)
ReadMailGizmo *	gizmo;
{
	return gizmo->headArea;
}

static XtPointer
Query (ReadMailGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)gizmo->rubberTile;
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)gizmo;
				break;
			}
			default: {
				return (NULL);
				break;
			}
		}
	}
	else {
		return (NULL);
	}
}
