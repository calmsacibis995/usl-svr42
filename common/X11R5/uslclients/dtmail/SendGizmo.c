/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:SendGizmo.c	1.14"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextEditP.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include "mail.h"
#include "SendGizmo.h"

static Widget		CreateSendGizmo();
static Gizmo		CopySendGizmo();
static void		FreeSendGizmo();
static XtPointer	QuerySend();

GizmoClassRec SendGizmoClass[] = {
	"SendGizmo",
	CreateSendGizmo,	/* Create	*/
	CopySendGizmo,		/* Copy		*/
	FreeSendGizmo,		/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* GetMneu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QuerySend		/* Query	*/
};

/* Private procedures */

static void
CreateScrollingSendArea (gizmo)
SendGizmo *gizmo;
{
	Widget sw;

	sw = XtVaCreateManagedWidget (
		"Scrolled Window",
		scrolledWindowWidgetClass,
		gizmo->mailForm,
		XtNyResizable,    True,
		XtNxResizable,    True,
		XtNxAttachRight,  True,
		XtNyAttachBottom, True,
		XtNyAddHeight,	  True,
		XtNyRefWidget,	  gizmo->control,
		XtNshadowThickness,0,
		(String)0
	);
	gizmo->displayArea = XtVaCreateManagedWidget (
		"data",
		textEditWidgetClass,
		sw,
		XtNsourceType,	  OL_STRING_SOURCE,
		XtNsource,	  "",
		(String)0
	);
}

static void
CreateCc (parent, cc, name, label)
Widget	parent;
Widget	*cc;		/* Points to cc, bcc or to widget */
char *	name;
char *	label;
{
	Widget to;

	if (*cc == (Widget)0) {
		to = XtVaCreateManagedWidget (
			"caption",
			captionWidgetClass,
			parent,
			XtNlabel,	 label,
			(String)0
		);
		*cc = XtVaCreateManagedWidget (
			name,
			textFieldWidgetClass,
			to,
			XtNcharsVisible, 70,
			(String)0
		);
	}
}

void
CreateSubjectArea (gizmo)
SendGizmo *gizmo;
{
	gizmo->control = XtVaCreateManagedWidget (
		"control",
		controlAreaWidgetClass,
		gizmo->mailForm,
		XtNalignCaptions,	True,
		XtNvSpace,		1,
		XtNlayoutType,		OL_FIXEDCOLS,
		XtNshadowThickness,	0,
		(String)0
	);
	CreateCc (gizmo->control, &gizmo->to, "to", GetGizmoText (TXT_TO));
	CreateCc (
		gizmo->control, &gizmo->subject, "subject",
		GetGizmoText (TXT_SUBJECT)
	);
	CreateCc (gizmo->control, &gizmo->cc, "cc", GetGizmoText (TXT_CC));
	CreateCc (gizmo->control, &gizmo->bcc, "bcc", GetGizmoText (TXT_BCC));
}

static Widget
CreateSendGizmo (parent, gizmo)
Widget		parent;
SendGizmo *	gizmo;
{
	Arg arg[10];

	/* Create the popup shell to contain the mail message */

	gizmo->mailForm = XtVaCreateManagedWidget (
		"text_form",
		formWidgetClass,
		parent,
		XtNshadowThickness,	0,
		(String)0
	);
	CreateSubjectArea (gizmo);
	CreateScrollingSendArea (gizmo);

	return gizmo->mailForm;
}

static Gizmo
CopySendGizmo (gizmo)
SendGizmo *	gizmo;
{
	SendGizmo * new = (SendGizmo *)MALLOC (sizeof (SendGizmo));

	new->name = STRDUP (gizmo->name);
	new->subject = (Widget)0;
	new->to = (Widget)0;
	new->cc = (Widget)0;
	new->bcc = (Widget)0;
	new->control = (Widget)0;
	new->displayArea = (Widget)0;
	new->mailForm = (Widget)0;
	return (Gizmo)new;
}

static void
FreeSendGizmo (gizmo)
SendGizmo *	gizmo;
{
	FREE (gizmo->name);
	FREE (gizmo);
}

/*
 * Given the text edit widget 'w' return the 'num'th logical
 * line.  The line is made up of all the characters between the
 * left and right margins of the text edit.  This means that if
 * the line wraps then only the characters on one line are returned.
 */
char *
OlGetWrappedLine (w, num)
Widget	w;
int	num;
{
	char *	cp;
	TextEditRec *	tp;
	WrapTable *	wt;
	char		buf[BUF_SIZE];
	int		i;
	int		j;
	int		cur;
	int		cnt;

	tp = (TextEditRec *)w;
	/* Get the wrap table */
	wt = tp->textedit.wrapTable;
	/*
	 * Loop thru each line in the text edit buffer until
	 * the num'th line is found.
	 */
	for (i=0, cur=0; i<wt->used; i++) {
		/*
		 * Count each wrapped line as a separate line.
		 */
		for (j=0; j<wt->p[i]->used; j++) {
			/*
			 * When the line has been found copy
			 * only the portion of the line that
			 * doesn't wrap above or below this line.
			 */
			if (cur == num) {
				/* Get the ith line in the text buffer */
				cp = GetTextBufferLine (
					tp->textedit.textBuffer, i
				);
				/*
				 * If this is the last part of the wrapped
				 * line then copy up to the end of the
				 * string 
				 */
				if (j == wt->p[i]->used-1) {
					strcpy (buf, cp+wt->p[i]->p[j]);
				}
				/*
				 * Otherwise, only copy this portion of the
				 * string
				 */
				else {
					cnt = wt->p[i]->p[j+1]-wt->p[i]->p[j];
					strncpy (
						buf,
						cp+wt->p[i]->p[j],
						cnt
					);
					buf[cnt] = '\0';
				}
				return buf;
			}
			cur += 1;
		}
	}
	return NULL;
}

/* Public procedures */

void
GetSendText (
	SendGizmo *gizmo, char **subject, char **to,
	char **cc, char **bcc, char **text
)
{
	OlTextEditCopyBuffer (gizmo->displayArea, text);
	*subject = (char *)OlTextFieldGetString (
		(Widget)gizmo->subject, NULL
	);
	*to = (char *)OlTextFieldGetString ((Widget)gizmo->to, NULL);
	*cc = (char *)OlTextFieldGetString ((Widget)gizmo->cc, NULL);
	*bcc = (char *)OlTextFieldGetString ((Widget)gizmo->bcc, NULL);
}

Widget
GetSendTextWidget (SendGizmo *gizmo)
{
	return gizmo->displayArea;
}

void
SetSendTextAndHeader (SendGizmo * gizmo, char *subject, char *to,
char *cc, char *bcc, char *text)
{
	XtVaSetValues (gizmo->displayArea, XtNsource, text, (String)0);
	XtVaSetValues (gizmo->subject, XtNstring, subject, (String)0);
	XtVaSetValues (gizmo->to, XtNstring, to, (String)0);
	XtVaSetValues (gizmo->cc, XtNstring, cc, (String)0);
	XtVaSetValues (gizmo->bcc, XtNstring, bcc, (String)0);
}

static XtPointer
QuerySend (SendGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)gizmo->mailForm;
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
} /* end of QuerySend */
