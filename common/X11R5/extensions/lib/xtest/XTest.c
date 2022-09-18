/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xtest/XTest.c	1.1"

#define NEED_REPLIES
#include "Xlibint.h"
#include "XTest.h"
#include "xteststr.h"
#include "Xext.h"
#include "extutil.h"

static XExtensionInfo _xtest_info_data;
static XExtensionInfo *xtest_info = &_xtest_info_data;
static /* const */ char *xtest_extension_name = XTESTNAME;

#define XTestCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xtest_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks xtest_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    close_display,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, xtest_info,
				   xtest_extension_name, 
				   &xtest_extension_hooks, XTestNumberEvents,
				   NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xtest_info)

/*****************************************************************************
 *                                                                           *
 *		    public routines               			     *
 *                                                                           *
 *****************************************************************************/

Bool
XTestQueryExtension (dpy, event_basep, error_basep, majorp, minorp)
    Display *dpy;
    int *event_basep, *error_basep;
    int *majorp, *minorp;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestGetVersionReq *req;
    xXTestGetVersionReply rep;

    if (XextHasExtension(info)) {
	LockDisplay(dpy);
	GetReq(XTestGetVersion, req);
	req->reqType = info->codes->major_opcode;
	req->xtReqType = X_XTestGetVersion;
	req->majorVersion = XTEST_MAJOR;
	req->minorVersion = XTEST_MINOR;
	if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return False;
	}
	UnlockDisplay(dpy);
	SyncHandle();
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	*majorp = rep.majorVersion;
	*minorp = rep.minorVersion;
	return True;
    } else {
	return False;
    }
}

Bool
XTestCompareCursorWithWindow(dpy, window, cursor)
    Display *dpy;
    Window window;
    Cursor cursor;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestCompareCursorReq *req;
    xXTestCompareCursorReply rep;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestCompareCursor, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestCompareCursor;
    req->window = window;
    req->cursor = cursor;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return rep.same;
}

Bool
XTestCompareCurrentCursorWithWindow(dpy, window)
    Display *dpy;
    Window window;
{
    return XTestCompareCursorWithWindow(dpy, window, XTestCurrentCursor);
}

XTestFakeKeyEvent(dpy, keycode, is_press, delay)
    Display *dpy;
    unsigned int keycode;
    Bool is_press;
    unsigned long delay;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestFakeInputReq *req;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestFakeInput, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestFakeInput;
    req->type = is_press ? KeyPress : KeyRelease;
    req->detail = keycode;
    req->time = delay;
    UnlockDisplay(dpy);
    SyncHandle();
}

XTestFakeButtonEvent(dpy, button, is_press, delay)
    Display *dpy;
    unsigned int button;
    Bool is_press;
    unsigned long delay;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestFakeInputReq *req;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestFakeInput, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestFakeInput;
    req->type = is_press ? ButtonPress : ButtonRelease;
    req->detail = button;
    req->time = delay;
    UnlockDisplay(dpy);
    SyncHandle();
}

XTestFakeMotionEvent(dpy, screen, x, y, delay)
    Display *dpy;
    int screen;
    int x, y;
    unsigned long delay;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestFakeInputReq *req;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestFakeInput, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestFakeInput;
    req->type = MotionNotify;
    req->root = RootWindow(dpy, screen);
    req->rootX = x;
    req->rootY = y;
    req->time = delay;
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XTestSetGContextOfGC(gc, gid)
    GC gc;
    GContext gid;
{
    gc->gid = gid;
}

void
XTestSetVisualIDOfVisual(visual, visualid)
    Visual *visual;
    VisualID visualid;
{
    visual->visualid = visualid;
}

static xReq _dummy_request = {
	0, 0, 0
};

Status
XTestDiscard(dpy)
    Display *dpy;
{
    Bool something;
    register char *ptr;

    LockDisplay(dpy);
    if (something = (dpy->bufptr != dpy->buffer)) {
	for (ptr = dpy->buffer;
	     ptr < dpy->bufptr;
	     ptr += (((xReq *)ptr)->length << 2))
	    dpy->request--;
	dpy->bufptr = dpy->buffer;
	dpy->last_req = (char *)&_dummy_request;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return something;
}
