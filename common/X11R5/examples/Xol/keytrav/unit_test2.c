/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:keytrav/unit_test2.c	1.1"
#endif

/* unit_test2.c:  from Sam Chang's test of referenceWidget */

#include "X11/Intrinsic.h"
#include "X11/StringDefs.h"
#include "Xol/OpenLook.h"
#include "Xol/ControlAre.h"
#include "Xol/TextField.h"
#include "Xol/TextPane.h"

char	buff [80];

Widget	createTF ();

main (argc, argv)
    int		argc;
    char *	argv[];
{
    Widget	toplevel, ctlA, tf_1, tf_2, tf_3, tf_4;
    Arg		args[2];

    toplevel = OlInitialize ("test1", "test1", NULL, 0, &argc, argv);

    XtSetArg (args [0], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);
    XtSetArg (args [1], XtNtraversalManager, (XtArgVal) True);
    ctlA = XtCreateManagedWidget(
				 "Parent",
				 controlAreaWidgetClass,
				 toplevel,
				 args,
				 2);

    tf_1 = createTF (ctlA, "tf-1", NULL);
    tf_2 = createTF (ctlA, "tf-2", tf_1);
    tf_3 = createTF (ctlA, "tf-3", tf_2);
    tf_4 = createTF (ctlA, "tf-4", tf_3);

    XtRealizeWidget (toplevel);
    XtMainLoop ();
}

Widget	createTF (parent, content, ref)
    Widget	parent;
    char	*content;
    Widget	ref;
{
    Arg		args[5];
    Widget	w;

    strcpy (buff, content);
    XtSetArg (args [0], XtNstring, (XtArgVal) buff);
    XtSetArg (args [1], XtNreferenceWidget, (XtArgVal) ref);

    w = XtCreateManagedWidget(
			      content,
			      textFieldWidgetClass,
			      parent,
			      args,
			      2);
    return (w);
}
