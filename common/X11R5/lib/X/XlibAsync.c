/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XlibAsync.c	1.1"

#define NEED_REPLIES
#include <X11/Xlibint.h>
#include <X11/Xos.h>

/*ARGSUSED*/
Bool
_XAsyncErrorHandler(dpy, rep, buf, len, data)
    register Display *dpy;
    register xReply *rep;
    char *buf;
    int len;
    XPointer data;
{
    register _XAsyncErrorState *state;

    state = (_XAsyncErrorState *)data;
    if (rep->generic.type == X_Error &&
	(!state->error_code ||
	 rep->error.errorCode == state->error_code) &&
	(!state->major_opcode ||
	 rep->error.majorCode == state->major_opcode) &&
	(!state->minor_opcode ||
	 rep->error.minorCode == state->minor_opcode) &&
	(!state->min_sequence_number ||
	 (state->min_sequence_number <= dpy->last_request_read)) &&
	(!state->max_sequence_number ||
	 (state->max_sequence_number >= dpy->last_request_read))) {
	state->last_error_received = rep->error.errorCode;
	state->error_count++;
	return True;
    }
    return False;
}

_XDeqAsyncHandler(dpy, handler)
    Display *dpy;
    register _XAsyncHandler *handler;
{
    register _XAsyncHandler **prev;
    register _XAsyncHandler *async;

    for (prev = &dpy->async_handlers;
	 (async = *prev) && (async != handler);
	 prev = &async->next)
	;
    if (async)
	*prev = async->next;
}

char *
_XGetAsyncReply(dpy, replbuf, rep, buf, len, extra, discard)
    register Display *dpy;
    register char *replbuf;
    register xReply *rep;
    char *buf;
    int len;
    int extra;
    Bool discard;
{
    if (extra == 0) {
	if (discard && (rep->generic.length << 2) > len)
	    _XEatData (dpy, (rep->generic.length << 2) - len);
	return (char *)rep;
    }

    if (extra <= rep->generic.length) {
	int size = SIZEOF(xReply) + (extra << 2);
	if (size > len) {
	    bcopy(buf, replbuf, len);
	    _XRead(dpy, replbuf + len, size - len);
	    buf = replbuf;
	    len = size;
#ifdef MUSTCOPY
	} else {
	    bcopy(buf, replbuf, size);
	    buf = replbuf;
#endif
	}

	if (discard && rep->generic.length > extra &&
	    (rep->generic.length << 2) > len)
	    _XEatData (dpy, (rep->generic.length << 2) - len);

	return buf;
    }
    /* 
     *if we get here, then extra > rep->generic.length--meaning we
     * read a reply that's shorter than we expected.  This is an 
     * error,  but we still need to figure out how to handle it...
     */
    if ((rep->generic.length << 2) > len)
	_XEatData (dpy, (rep->generic.length << 2) - len);
    _XIOError (dpy);
    return (char *)rep;
}
