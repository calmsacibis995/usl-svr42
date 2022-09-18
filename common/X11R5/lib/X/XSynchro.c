/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSynchro.c	1.2"

#include "Xlibint.h"


int _XSyncFunction(dpy)
register Display *dpy;
{
	XSync(dpy,0);
	return 0;
}

#if NeedFunctionPrototypes
int (*XSynchronize(Display *dpy, int onoff))()
#else
int (*XSynchronize(dpy,onoff))()
     register Display *dpy;
     int onoff;
#endif
{
        int (*temp)();

	LockDisplay(dpy);
	temp = dpy->synchandler;
	if (onoff) dpy->synchandler = _XSyncFunction;
	else dpy->synchandler = NULL;
	UnlockDisplay(dpy);
	return (temp);
}

#if NeedFunctionPrototypes
int (*XSetAfterFunction(
     Display *dpy,
     int (*func)(
#if NeedNestedPrototypes
		 Display*
#endif
		 )
))()
#else
int (*XSetAfterFunction(dpy,func))()
     register Display *dpy;
     int (*func)(
#if NeedNestedPrototypes
		 Display*
#endif
		 );
#endif
{
        int (*temp)();

	LockDisplay(dpy);
	temp = dpy->synchandler;
	dpy->synchandler = func;
	UnlockDisplay(dpy);
	return (temp);
}

