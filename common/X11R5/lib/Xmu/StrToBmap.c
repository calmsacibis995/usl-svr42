/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StrToBmap.c	1.2"
/* static char rcsid[] = "$XConsortium: StrToBmap.c,v 1.11 91/03/09 17:08:48 rws Exp $"; */


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include	<X11/Intrinsic.h>
#include	<X11/StringDefs.h>
#include	<X11/Xmu/Converters.h>
#include	<X11/Xmu/Drawing.h>


/*
 * XmuConvertStringToBitmap:
 *
 * creates a depth-1 Pixmap suitable for window manager icons.
 * "string" represents a bitmap(1) filename which may be absolute,
 * or relative to the global resource bitmapFilePath, class
 * BitmapFilePath.  If the resource is not defined, the default
 * value is the build symbol BITMAPDIR.
 *
 * shares lots of code with XmuConvertStringToCursor.  
 *
 * To use, include the following in your ClassInitialize procedure:

static XtConvertArgRec screenConvertArg[] = {
    {XtBaseOffset, (caddr_t) XtOffset(Widget, core.screen), sizeof(Screen *)}
};

    XtAddConverter("String", "Bitmap", XmuCvtStringToBitmap,
		   screenConvertArg, XtNumber(screenConvertArg));
 *
 */

#define	done(address, type) \
	{ (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }


/*ARGSUSED*/
void XmuCvtStringToBitmap(args, num_args, fromVal, toVal)
    XrmValuePtr args;
    Cardinal    *num_args;
    XrmValuePtr	fromVal;
    XrmValuePtr	toVal;
{
    static Pixmap pixmap;		/* static for cvt magic */
    char *name = (char *)fromVal->addr;
    Screen *screen;
    Display *dpy;
    XrmDatabase db;
    String fn;
    unsigned int width, height;
    int xhot, yhot;
    unsigned char *data;

    if (*num_args != 1)
     XtErrorMsg("wrongParameters","cvtStringToBitmap","XtToolkitError",
             "String to pixmap conversion needs screen argument",
              (String *)NULL, (Cardinal *)NULL);

    if (strcmp(name, "None") == 0) {
	pixmap = None;
	done(&pixmap, Pixmap);
	return;
    }

    if (strcmp(name, "ParentRelative") == 0) {
	pixmap = ParentRelative;
	done(&pixmap, Pixmap);
	return;
    }

    screen = *((Screen **) args[0].addr);
    pixmap = XmuLocateBitmapFile (screen, name,
				  NULL, 0, NULL, NULL, NULL, NULL);
    if (pixmap == None) {
	dpy = DisplayOfScreen(screen);
	db = XrmGetDatabase(dpy);
	XrmSetDatabase(dpy, XtScreenDatabase(screen));
	fn = XtResolvePathname(dpy, "bitmaps", name, "", NULL, NULL, 0, NULL);
	if (!fn)
	    fn = XtResolvePathname(dpy, "", name, ".xbm", NULL, NULL, 0, NULL);
	XrmSetDatabase(dpy, db);
	if (fn &&
	    XmuReadBitmapDataFromFile (fn, &width, &height, &data,
				       &xhot, &yhot) == BitmapSuccess) {
	    pixmap = XCreatePixmapFromBitmapData (dpy,
						  RootWindowOfScreen(screen),
						  (char *) data, width, height,
						  1, 0, 1);
	    XFree ((char *)data);
	}
    }

    if (pixmap != None) {
	done (&pixmap, Pixmap);
    } else {
	XtStringConversionWarning (name, "Pixmap");
	return;
    }
}

