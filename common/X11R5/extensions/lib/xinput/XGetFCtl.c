/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XGetFCtl.c	1.1"
/* $Header: XGetFCtl.c,v 1.11 91/01/26 13:34:42 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XGetFeedbackControl - get the feedback attributes of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "Xlib.h"
#include "XInput.h"
#include "extutil.h"

XFeedbackState
*XGetFeedbackControl (dpy, dev, num_feedbacks)
    register	Display 	*dpy;
    XDevice			*dev;
    int				*num_feedbacks;
    {
    int	size = 0;
    int	nbytes, i;
    XFeedbackState *Feedback = NULL;
    XFeedbackState *Sav = NULL;
    xFeedbackState *f = NULL;
    xFeedbackState *sav = NULL;
    xGetFeedbackControlReq *req;
    xGetFeedbackControlReply rep;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return ((XFeedbackState *) NoSuchExtension);

    GetReq(GetFeedbackControl,req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetFeedbackControl;
    req->deviceid = dev->device_id;

    if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) 
	{
	UnlockDisplay(dpy);
	SyncHandle();
	return (XFeedbackState *) NULL;
	}
    if (rep.length > 0) 
	{
	*num_feedbacks = rep.num_feedbacks;
	nbytes = (long)rep.length << 2;
	f = (xFeedbackState *) Xmalloc((unsigned) nbytes);
        if (!f)
	    {
	    _XEatData (dpy, (unsigned long) nbytes);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (XFeedbackState *) NULL;
	    }
	sav = f;
	_XRead (dpy, (char *) f, nbytes);

	for (i=0; i<*num_feedbacks; i++)
	    {
	    switch (f->class)
		{
		case KbdFeedbackClass:
		    size += sizeof (XKbdFeedbackState);
		    break;
		case PtrFeedbackClass:
		    size += sizeof (XPtrFeedbackState);
		    break;
		case IntegerFeedbackClass:
		    size += sizeof (XIntegerFeedbackState);
		    break;
		case StringFeedbackClass:
		    {
		    xStringFeedbackState *strf = (xStringFeedbackState *) f;

		    size += sizeof (XStringFeedbackState) + 
			(strf->num_syms_supported * sizeof (KeySym));
		    }
		    break;
		case LedFeedbackClass:
		    size += sizeof (XLedFeedbackState);
		    break;
		case BellFeedbackClass:
		    size += sizeof (XBellFeedbackState);
		    break;
		default:
		    size += f->length;
		    break;
		}
	    f = (xFeedbackState *) ((char *) f + f->length);
	    }

	Feedback = (XFeedbackState *) Xmalloc((unsigned) size);
        if (!Feedback)
	    {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (XFeedbackState *) NULL;
	    }
	Sav = Feedback;

	f = sav;
	for (i=0; i<*num_feedbacks; i++)
	    {
	    switch (f->class)
		{
		case KbdFeedbackClass:
		    {
		    xKbdFeedbackState *k;
		    XKbdFeedbackState *K;
		    k = (xKbdFeedbackState *) f;
		    K = (XKbdFeedbackState *) Feedback;

		    K->class = k->class;
		    K->length = sizeof (XKbdFeedbackState);
		    K->id = k->id;
		    K->click = k->click;
		    K->percent = k->percent;
		    K->pitch = k->pitch;
		    K->duration = k->duration;
		    K->led_mask = k->led_mask;
		    K->global_auto_repeat = k->global_auto_repeat;
		    bcopy ((char *) &k->auto_repeats[0], 
			(char *) &K->auto_repeats[0], 32);
		    break;
		    }
		case PtrFeedbackClass:
		    {
		    xPtrFeedbackState *p;
		    XPtrFeedbackState *P;
		    p = (xPtrFeedbackState *) f;
		    P = (XPtrFeedbackState *) Feedback;

		    P->class = p->class;
		    P->length = sizeof (XPtrFeedbackState);
		    P->id = p->id;
		    P->accelNum = p->accelNum;
		    P->accelDenom = p->accelDenom;
		    P->threshold = p->threshold;
		    break;
		    }
		case IntegerFeedbackClass:
		    {
		    xIntegerFeedbackState *i;
		    XIntegerFeedbackState *I;
		    i = (xIntegerFeedbackState *) f;
		    I = (XIntegerFeedbackState *) Feedback;

		    I->class = i->class;
		    I->length = sizeof (XIntegerFeedbackState);
		    I->id = i->id;
		    I->resolution = i->resolution;
		    I->minVal = i->min_value;
		    I->maxVal = i->max_value;
		    break;
		    }
		case StringFeedbackClass:
		    {
		    xStringFeedbackState *s;
		    XStringFeedbackState *S;
		    s = (xStringFeedbackState *) f;
		    S = (XStringFeedbackState *) Feedback;

		    S->class = s->class;
		    S->length = sizeof (XStringFeedbackState) + 
			(s->num_syms_supported * sizeof (KeySym));
		    S->id = s->id;
		    S->max_symbols = s->max_symbols;
		    S->num_syms_supported = s->num_syms_supported;
		    S->syms_supported = (KeySym *) (S+1);
		    bcopy ((char *) (s+1), (char *) S->syms_supported,
			(S->num_syms_supported * sizeof (KeySym)));
		    break;
		    }
		case LedFeedbackClass:
		    {
		    xLedFeedbackState *l;
		    XLedFeedbackState *L;
		    l = (xLedFeedbackState *) f;
		    L = (XLedFeedbackState *) Feedback;

		    L->class = l->class;
		    L->length = sizeof (XLedFeedbackState);
		    L->id = l->id;
		    L->led_values = l->led_values;
		    L->led_mask = l->led_mask;
		    break;
		    }
		case BellFeedbackClass:
		    {
		    xBellFeedbackState *b;
		    XBellFeedbackState *B;
		    b = (xBellFeedbackState *) f;
		    B = (XBellFeedbackState *) Feedback;

		    B->class = b->class;
		    B->length = sizeof (XBellFeedbackState);
		    B->id = b->id;
		    B->percent = b->percent;
		    B->pitch = b->pitch;
		    B->duration = b->duration;
		    break;
		    }
		default:
		    break;
		}
	    f = (xFeedbackState *) ((char *) f + f->length);
	    Feedback = (XFeedbackState *) ((char *) Feedback+Feedback->length);
	    }
	XFree ((char *)sav);
	}

    UnlockDisplay(dpy);
    SyncHandle();
    return (Sav);
    }

XFreeFeedbackList (list)
    XFeedbackState *list;
    {
    XFree ((char *)list);
    }
