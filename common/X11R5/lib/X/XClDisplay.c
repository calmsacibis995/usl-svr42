/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XClDisplay.c	1.2"

#include "Xlibint.h"

/* 
 * XCloseDisplay - XSync the connection to the X Server, close the connection,
 * and free all associated storage.  Extension close procs should only free
 * memory and must be careful about the types of requests they generate.
 */

XCloseDisplay (dpy)
	register Display *dpy;
{
	register _XExtension *ext;
	register int i;
	extern void _XFreeQ();

	if (!(dpy->flags & XlibDisplayClosing))
	{
	    dpy->flags |= XlibDisplayClosing;
	    for (i = 0; i < dpy->nscreens; i++) {
		    register Screen *sp = &dpy->screens[i];
		    XFreeGC (dpy, sp->default_gc);
	    }
	    if (dpy->cursor_font != None) {
		XUnloadFont (dpy, dpy->cursor_font);
	    }
	    ext = dpy->ext_procs;
	    while (ext) {	/* call out to any extensions interested */
		    if (ext->close_display != NULL) 
			    (*ext->close_display)(dpy, &ext->codes);
		    ext = ext->next;
	    }    
	    XSync(dpy, 1);  /* throw away pending input events */
	}
	_XDisconnectDisplay(dpy->fd);
	_XFreeDisplayStructure (dpy);
	_XFreeQ ();
	return;
}
