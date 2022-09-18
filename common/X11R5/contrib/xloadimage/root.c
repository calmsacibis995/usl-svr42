/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:root.c	1.1"
/* root.c:
 *
 * this loads an image onto the root window
 *
 * jim frost 10.03.89
 *
 * Copyright 1989, 1990 Jim Frost.  See included file "copyright.h" for
 * complete copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"

void imageOnRoot(disp, scrn, image, verbose)
     Display      *disp;
     int           scrn;
     Image        *image;
     unsigned int  verbose;
{ Pixmap   pixmap;
  Colormap xcmap;

  /* xloadimage retains its information "temporarily" so that it can clean
   * up after itself.  this is supposed to clean up those resources on a
   * subsequent load.  may effect other clients.  a better way might be to
   * create a property and save the pixmap in it, exit retainpermanent, and
   * xkillclient the pixmap id if it exists.
   */

  XKillClient(disp, AllTemporary); /* kill temporarily held resources; used to
				    * clean up previous "onroot" loads */

  if (! sendImageToX(disp, scrn, DefaultVisual(disp, scrn), image,
		     &pixmap, &xcmap, verbose))
    exit(1);

  /* changing the root colormap is A Bad Thing, so deny it.
   */

  if (xcmap != DefaultColormap(disp, scrn)) {
    printf("Loading image onto root would change default colormap (sorry)\n");
    XFreePixmap(disp, pixmap);
    exit(1);
  }

  XSetWindowBackgroundPixmap(disp, RootWindow(disp, scrn), pixmap);
  XClearWindow(disp, RootWindow(disp, scrn));
  XFreePixmap(disp, pixmap);
  XSetCloseDownMode(disp, RetainTemporary);
}
