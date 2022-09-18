/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XIMQueue.c	1.2"

#define NEED_EVENTS
#include "Xlibint.h"
#include "Xi18nint.h"
#include "XIMlibint.h"
#include "XIMproto.h"

short
_XipTypeOfNextICQueue(ic)
    XipIC ic;
{
    if (ic->out == NULL) return((short)0);
    return((short)ic->out->type);
}

unsigned int
_XipStateOfNextICQueue(ic)
    XipIC ic;
{
    if (ic->out == NULL) return(0);
    return(ic->out->state);
}

KeySym
_XipKeySymOfNextICQueue(ic)
    XipIC ic;
{
    if (ic->out == NULL) return(0);
    return((KeySym)ic->out->keysym);
}

char *
_XipStringOfNextICQueue(ic)
    XipIC ic;
{
    if (ic->out == NULL || ic->out->length == 0) return(0);
    return((char *)ic->out->ptr);
}

void
_XipFreeNextICQueue(ic)
    XipIC ic;
{
    if (ic->out) {
	if (ic->out->next == ic->in) {
	    ic->out = NULL;
	} else {
	    ic->out = ic->out->next;
	}
    }
}

/*
 * Stack 
 */
int
_XipPutICQueue(ic, type, length, keysym, state, str)
    XipIC ic;
    short type;
    int length;
    KeySym keysym;
    unsigned int state;
    unsigned char *str;
{
    XipIM im = ipIMofIC(ic);

    /*
     * If a queue of stack is NULL, allocate a structure XIMQueue.
     */
    if (ic->in == NULL) {
	if ((ic->in = (XIMQueue *)Xmalloc(sizeof(XIMQueue))) == NULL)
	    return(-1);
	ic->in->buf_max = sizeof(XEvent);
	if ((ic->in->ptr = (char *)Xmalloc(sizeof(XEvent))) == NULL) return(-1);
	ic->in->length = 0;
	ic->in->keysym = 0;
	ic->in->state = 0;
	ic->in->next  = ic->in;
	ic->prev = ic->in;
    }
    if (ic->in == ic->out) {
	if ((ic->in = (XIMQueue *)Xmalloc(sizeof(XIMQueue))) == NULL)
	    return(-1);
	ic->in->buf_max = sizeof(XEvent);
	if ((ic->in->ptr = (char *)Xmalloc(sizeof(XEvent))) == NULL) return(-1);
	ic->in->length = 0;
	ic->in->keysym = 0;
	ic->in->state = 0;
	ic->in->next = ic->out;
	ic->prev->next = ic->in;
    }

    ic->in->type = type;
    if (type == XIM_KEYSYM) {
	if (str) {
	    length = strlen((char *)str);
	}
	ic->in->length = length;
	ic->in->keysym = keysym;
	ic->in->state = state;
	if (length > 0) {
	    if (ic->in->buf_max < length) {
		ic->in->ptr = (char *)Xrealloc((char *)ic->in->ptr,
					       (unsigned)length);
		ic->in->buf_max = length;
	    }
	    if (!str) {
		if (_XipReadFromIM(im, ic->in->ptr, ic->in->length) < 0)
		    return(-1);
	    } else {
		(void) strcpy(ic->in->ptr, (char *)str);
	    }
	}
    } else if (type == XIM_STRING) {
	ic->in->length = length;
	ic->in->keysym = 0;
	ic->in->state = 0;
	if (ic->in->buf_max < length) {
	    ic->in->ptr = (char *)Xrealloc((char *)ic->in->ptr,
					   (unsigned)length);
	    ic->in->buf_max = length;
	}
	if (_XipReadFromIM(im, ic->in->ptr, ic->in->length) < 0)
	    return(-1);
    }
    if (ic->out == NULL) ic->out = ic->in;
    ic->prev = ic->in;
    ic->in = ic->in->next;
    return(0);
}

/*
 * Get 
 */
void
_XipGetNextICQueue(ic, type, length, keysym, ptr)
    XipIC ic;
    short *type;
    int *length;
    KeySym *keysym;
    char **ptr;
{
    if (ic->out) {
	*type = ic->out->type;
	*length = ic->out->length;
	if (keysym != NULL) *keysym = ic->out->keysym;
	*ptr = ic->out->ptr;
	if (ic->out->next == ic->in) {
	    ic->out = NULL;
	} else {
	    ic->out = ic->out->next;
	}
    } else {
	*type = 0;
    }
}


