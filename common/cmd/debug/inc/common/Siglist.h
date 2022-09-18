/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Siglist_h
#define Siglist_h
#ident	"@(#)debugger:inc/common/Siglist.h	1.1"

#include "Ev_Notify.h"
#include "Proctypes.h"
#include <signal.h>

class List;

class Siglist {
	sig_ctl		_sigset;
	NotifyEvent	*_events[ NSIG-1 ];
	friend class	LWP;
public:
			Siglist();
			~Siglist();
	int		add( sig_ctl *, Notifier, void *);
	int		ignored( int sig );	// boolean result
	int		remove( sig_ctl *, Notifier, void *);
	NotifyEvent 	*events( int sig );
	sig_ctl		*sigset() { return &_sigset; }
};

#endif

// end of Siglist.h

