/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XChgFCtl.c	1.1"
/* $XConsortium: XChgFCtl.c,v 1.7 91/02/09 17:48:17 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XChangeFeedbackControl - Change the control attributes of feedbacks on
 * an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"

int
XChangeFeedbackControl (dpy, dev, mask, f)
    register	Display 	*dpy;
    XDevice			*dev;
    unsigned	long		mask;
    XFeedbackControl		*f;
    {
    int length;
    xChangeFeedbackControlReq	*req;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    LockDisplay (dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    GetReq(ChangeFeedbackControl,req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_ChangeFeedbackControl;
    req->deviceid = dev->device_id;
    req->mask = mask;
    req->feedbackid = f->class;

    if (f->class == KbdFeedbackClass)
	{
	XKbdFeedbackControl	*K;
	xKbdFeedbackCtl		k;

	K = (XKbdFeedbackControl *) f;
	k.class = KbdFeedbackClass;
	k.length = sizeof (xKbdFeedbackCtl);
	k.id = K->id;
	k.click = K->click;
	k.percent = K->percent;
	k.pitch = K->pitch;
	k.duration = K->duration;
	k.led_mask = K->led_mask;
	k.led_values = K->led_value;
	k.key = K->key;
	k.auto_repeat_mode = K->auto_repeat_mode;
	length = ((unsigned)(k.length + 3) >> 2);
	req->length += length;
	length <<= 2;
	Data (dpy, (char *) &k, length);
	}
    else if (f->class == PtrFeedbackClass)
	{
	XPtrFeedbackControl	*P;
	xPtrFeedbackCtl		p;

	P = (XPtrFeedbackControl *) f;
	p.class = PtrFeedbackClass;
	p.length = sizeof (xPtrFeedbackCtl);
	p.id = P->id;
	p.num = P->accelNum;
	p.denom = P->accelDenom;
	p.thresh = P->threshold;
	length = ((unsigned)(p.length + 3) >> 2);
	req->length += length;
	length <<= 2;
	Data (dpy, (char *) &p, length);
	}
    else if (f->class == IntegerFeedbackClass)
	{
	XIntegerFeedbackControl	*I;
	xIntegerFeedbackCtl	i;

	I = (XIntegerFeedbackControl *) f;
	i.class = IntegerFeedbackClass;
	i.length = sizeof (xIntegerFeedbackCtl);
	i.id = I->id;
	i.int_to_display = I->int_to_display;
	length = ((unsigned)(i.length + 3) >> 2);
	req->length += length;
	length <<= 2;
	Data (dpy, (char *) &i, length);
	}
    else if (f->class == StringFeedbackClass)
	{
	XStringFeedbackControl	*S;
	xStringFeedbackCtl	s;

	S = (XStringFeedbackControl *) f;
	s.class = StringFeedbackClass;
	s.length = sizeof (xStringFeedbackCtl) + 
		(S->num_keysyms * sizeof (KeySym));
	s.id = S->id;
	s.num_keysyms = S->num_keysyms;
	req->length += ((unsigned)(s.length + 3) >> 2);
	length = sizeof (xStringFeedbackCtl);
	Data (dpy, (char *) &s, length);
	length = (s.num_keysyms * sizeof (KeySym));
	Data (dpy, (char *) S->syms_to_display, length);
	}
    else if (f->class == BellFeedbackClass)
	{
	XBellFeedbackControl	*B;
	xBellFeedbackCtl	b;

	B = (XBellFeedbackControl *) f;
	b.class = BellFeedbackClass;
	b.length = sizeof (xBellFeedbackCtl);
	b.id = B->id;
	b.percent = B->percent;
	b.pitch = B->pitch;
	b.duration = B->duration;
	length = ((unsigned)(b.length + 3) >> 2);
	req->length += length;
	length <<= 2;
	Data (dpy, (char *) &b, length);
	}
    else if (f->class == LedFeedbackClass)
	{
	XLedFeedbackControl	*L;
	xLedFeedbackCtl		l;

	L = (XLedFeedbackControl *) f;
	l.class = LedFeedbackClass;
	l.length = sizeof (xLedFeedbackCtl);
	l.id = L->id;
	l.led_mask = L->led_mask;
	l.led_values = L->led_values;
	length = ((unsigned)(l.length + 3) >> 2);
	req->length += length;
	length <<= 2;
	Data (dpy, (char *) &l, length);
	}
    else
	{
	xFeedbackCtl		u;

	u.class = f->class;
	u.length = f->length - sizeof (int);
	u.id = f->id;
	length = ((unsigned)(u.length + 3) >> 2);
	req->length += length;
	length <<= 2;
	Data (dpy, (char *) &u, length);
	}

    UnlockDisplay(dpy);
    SyncHandle();
    return (Success);
    }

