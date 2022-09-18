/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef EV_Notify_h
#define EV_Notify_h

#ident	"@(#)debugger:inc/common/Ev_Notify.h	1.1"

#include "Link.h"

// Process level primitive events are connected to 
// the higher level EventManager events through Event Notifiers.
// An NotifyEvent object contains a pointer to an interface function
// and a "this" pointer.
// The interface function calls a class member function, that in turn 
// provides notification actions.
//
// typedef used to register callback functions with event handlers

typedef int (*Notifier)(void *, ...);

struct NotifyEvent : public Link {
	Notifier	func;
	void		*thisptr;
			NotifyEvent(Notifier f, void *t) { func = f; thisptr = t;}
			~NotifyEvent() {}
	NotifyEvent	*next() { return (NotifyEvent *)Link::next(); }
};

extern int notify_sig_e_trigger(void *thisptr);
extern int notify_sys_e_trigger(void *thisptr);
extern int notify_onstop_e_trigger(void *thisptr);
extern int notify_stoploc_trigger(void *thisptr);
extern int notify_stop_e_clean_foreign(void *thisptr);
extern int notify_endlist_trigger(void *thisptr);
extern int notify_watchframe_start(void *thisptr);
extern int notify_watchframe_watch(void *thisptr);
extern int notify_watchframe_end(void *thisptr);

#endif
