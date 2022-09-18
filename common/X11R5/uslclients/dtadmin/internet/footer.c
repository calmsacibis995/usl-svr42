/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/footer.c	1.6"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <StaticText.h>
#include "inet.h"

void
InitFooter(parent)
Widget parent;
{

	hf->footer = XtVaCreateManagedWidget(
		"footer",
		staticTextWidgetClass,
		parent,
		XtNstring,		hf->filename,
		XtNgravity,		NorthWestGravity,
		XtNxResizable,		True,
		XtNxAttachRight,	True,
		XtNyAttachBottom,	True,
		XtNyVaryOffset,		True,
		XtNweight,		0,
		(String)0
	);
} /* InitFooter */

/*
** Clears footer area by writing a space into the footer
*/
void
ClearFooter(footer)
Widget        footer;                 /* Static text widget */
{
	static Arg    arg[] = {{XtNstring, (XtArgVal)" "}};


	if (footer)
		XtSetValues(footer, arg, XtNumber(arg));

} /* ClearFooter */

/*
** Write message into `footer' using printf(3)-style template (`tmpl') and
** an optional string `str' that may be referenced in `tmpl'.
** The resulting message can be at most MAXLINE chars.
** If the user asked for beeping footers, we obey.
*/
void
FooterMsg(footer, tmpl, str)
Widget		footer;			/* Static text widget */
char		*tmpl;
char		*str;
{
	Arg	arg[1];
	char 	msg[MAXLINE];


	(void)sprintf(msg, tmpl, str);	/* Construct message */

	if (footer) {
		/*
		** Display message
		*/
		XtSetArg(arg[0], XtNstring, (XtArgVal)msg);
		XtSetValues(footer, arg, (Cardinal)1);

		/*
		** Beep if the user asked for beeping footers
		*/
		_OlBeepDisplay(footer, 1);
	} else { /* temporary, to be replaced by notice popup */
		fprintf(stderr, msg);
	}

} /* FooterMsg */
