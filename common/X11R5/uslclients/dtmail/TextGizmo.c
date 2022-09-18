/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:TextGizmo.c	1.4"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/Gizmos.h>
#include "TextGizmo.h"

#ifdef DEBUG
#include "mail.h"
#endif

extern Widget	CreateTextGizmo();
extern void	FreeTextGizmo();
extern Gizmo	CopyTextGizmo();

GizmoClassRec TextGizmoClass[] = {
	"TextGizmo",
	CreateTextGizmo,	/* Create	*/
	CopyTextGizmo,		/* Copy		*/
	FreeTextGizmo,		/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	NULL			/* Query	*/
};

static Gizmo
CopyTextGizmo (gizmo)
TextGizmo *	gizmo;
{
	TextGizmo * new = (TextGizmo *) MALLOC (sizeof (TextGizmo));

	new->source = STRDUP (gizmo->source);
	new->lines = gizmo->lines;
	new->width = gizmo->width;
	new->textField = (Widget)0;

	return (Gizmo)new;
}

static void
FreeTextGizmo (gizmo)
TextGizmo *	gizmo;
{
	FREE (gizmo->source);
	FREE (gizmo);
}

static Widget
CreateTextGizmo (parent, g)
Widget		parent;
TextGizmo *	g;
{
	Widget	sw;

	sw = XtVaCreateManagedWidget (
		"scrolled window",
		scrolledWindowWidgetClass,
		parent,
		(String)0
	);
	g->textField = XtVaCreateManagedWidget (
		"text edit",
		textEditWidgetClass,
		sw,
		XtNlinesVisible,	(XtArgVal)g->lines,
		XtNcharsVisible,	(XtArgVal)g->width,
		XtNsource,		(XtArgVal)g->source,
		(String)0
	);
	return sw;
}

void
SetTextFieldValue (g, val)
TextGizmo *	g;
char *		val;
{
	XtVaSetValues (g->textField, XtNsource, val, (String)0);
}

char *
GetTextFieldValue (g)
TextGizmo *	g;
{
	char * text;

	OlTextEditCopyBuffer (g->textField, &text);
	return text;
}
