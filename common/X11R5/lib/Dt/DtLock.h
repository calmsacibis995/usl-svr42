/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Dt:DtLock.h	1.3"
#endif

#ifndef _DtLock_h
#define _DtLock_h

#define DtMinimumCursorLock 500L  /* 1/2 second */

typedef struct _DtCallbackInfo {
	void         (*f)();
	Display *    display;
	XtPointer    client_data;
	XtIntervalId timer_id;
} DtCallbackInfo;

#ifdef __cplusplus
extern "C" {
#endif

extern DtCallbackInfo * DtLockCursor(Widget w, unsigned long interval, 
                                     void (*f)(), XtPointer client_data, 
                                     Cursor cursor);
extern void             DtUnlockCursor(XtPointer client_data, 
                                       XtIntervalId * timer_id);

#ifdef __cplusplus
}
#endif

#endif
