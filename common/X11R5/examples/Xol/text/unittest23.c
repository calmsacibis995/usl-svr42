/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unittest23.c	1.3"
#endif

/*
** This test tests MR bl89-06302
*/

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/Text.h>


main (argc, argv)
  int	argc;
  char	*argv[];
{
  Arg		arg[20];
  unsigned	n;
  Widget	text;
  Widget	toplevel;


  toplevel = OlInitialize("unittest23", "UNittest23",
			  NULL, 0,
			  &argc, argv);

  n = 0;
  XtSetArg(arg[n], XtNverticalSB, TRUE);				n++;
  XtSetArg(arg[n], XtNwrap, TRUE);					n++;
  text = XtCreateManagedWidget("Text",
			       textWidgetClass,
			       toplevel,
			       arg,
			       n);

  n = 0;
  XtSetArg(arg[n], XtNfile, "text_file");				n++;
  XtSetArg(arg[n], XtNsourceType, OL_DISK_SOURCE);			n++;
  XtSetValues(text, arg, n);

  XtRealizeWidget(toplevel);
  XtMainLoop();

}	/* main() */
