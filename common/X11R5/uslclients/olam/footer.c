/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:footer.c	1.7"
#endif

/*
** footer.c - This file contains the routines for displaying footer
** messages.
** A footer is considered to by the static text widget inside of a footer
** panel.
*/


#include <errno.h>
#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

#include "errors.h"
#include "common.h"
#include "config.h"
#include "error.h"
#include "footer.h"
#include "util.h"


/*
** Clears `footer'
*/
void
ClearFooter(footer)
  Widget	footer;			/* Static text widget */
{
  static Arg	arg[] = {{XtNstring, (XtArgVal)" "}};


  /*
  ** Write a space into the footer
  */
  XtSetValues(footer, arg, XtNumber(arg));

}	/* ClearFooter() */


/*
** Write message into `footer' using printf(3)-style template (`tmpl') and
** an optional string `str' that may be referenced in `tmpl'.
** The resulting message can be at most MAXLINE chars.
** If the user asked for beeping footers, we obey.
*/
void
FooterMsg(footer, tmpl, str)
  Widget	footer;			/* Static text widget */
  char		*tmpl;
  char		*str;
{
  Arg		arg[1];
  String	msg[MAXLINE];


  (void)sprintf(msg, tmpl, str);	/* Construct message */

  DEBUG_MSG( OlGetMessage(XtDisplay(footer), NULL,
             0,
             OleNfilefooter,
             OleTmsg1,
             OleCOlClientOlamMsgs,
             OleMfilerfooter_msg1,
             (XrmDatabase)NULL),
             msg);

  /*
  ** Display message
  */
  XtSetArg(arg[0], XtNstring, (XtArgVal)msg);
  XtSetValues(footer, arg, (Cardinal)1);

  /*
  ** Beep if the user asked for beeping footers
  */
    _OlBeepDisplay(footer, 1);

}	/* FooterMsg() */
